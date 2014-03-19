/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * END NIGHTINGALE GPL
 */

#include "sbGStreamerRTPStreamer.h"

#include <sbIGStreamerService.h>

#include <nsIClassInfoImpl.h>
#include <sbClassInfoUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsStringAPI.h>
#include <prlog.h>

#include <sbVariantUtils.h>

#include "gst/sdp/gstsdp.h"
#include "gst/sdp/gstsdpmessage.h"

/**
 * To log this class, set the following environment variable in a debug build:
 *  NSPR_LOG_MODULES=sbGStreamerRTPStreamer:5 (or :3 for LOG messages only)
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gGStreamerRTPStreamer = PR_NewLogModule("sbGStreamerRTPStreamer");
#define LOG(args) PR_LOG(gGStreamerRTPStreamer, PR_LOG_WARNING, args)
#define TRACE(args) PR_LOG(gGStreamerRTPStreamer, PR_LOG_DEBUG, args)
#else /* PR_LOGGING */
#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */
#endif /* PR_LOGGING */


NS_IMPL_CLASSINFO(sbGStreamerRTPStreamer, NULL, nsIClassInfo::THREADSAFE, SB_GSTREAMERRTPSTREAMER_CID);

NS_IMPL_ISUPPORTS4_CI(sbGStreamerRTPStreamer,
                      sbIGStreamerPipeline,
                      sbIGStreamerRTPStreamer,
                      sbIMediacoreEventTarget,
                      nsIClassInfo);

NS_IMPL_THREADSAFE_CI(sbGStreamerRTPStreamer);

sbGStreamerRTPStreamer::sbGStreamerRTPStreamer() :
  sbGStreamerPipeline(),
  mSourceURI(nsnull),
  mDestHost(nsnull),
  mDestPort(0)
{
}

sbGStreamerRTPStreamer::~sbGStreamerRTPStreamer()
{
}

/* sbIGStreamerRTPStreamer interface implementation */

NS_IMETHODIMP
sbGStreamerRTPStreamer::SetSourceURI(const nsAString& aSourceURI)
{
  mSourceURI = aSourceURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerRTPStreamer::GetSourceURI(nsAString& aSourceURI)
{
  aSourceURI = mSourceURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerRTPStreamer::SetDestHost(const nsAString& aDestHost)
{
  mDestHost = aDestHost;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerRTPStreamer::GetDestHost(nsAString& aDestHost)
{
  aDestHost = mDestHost;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerRTPStreamer::SetDestPort(PRInt32 aDestPort)
{
  mDestPort = aDestPort;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerRTPStreamer::GetDestPort(PRInt32 *aDestPort)
{
  *aDestPort = mDestPort;
  return NS_OK;
}

nsresult
sbGStreamerRTPStreamer::BuildPipeline()
{
  nsCString pipelineString = NS_ConvertUTF16toUTF8(mSourceURI);
  // TODO: Figure out what format we want to use, or make configurable.
#if 1
  pipelineString += NS_LITERAL_CSTRING(
          " ! decodebin ! audioconvert ! audioresample ! vorbisenc ! "
          "rtpvorbispay name=payloader ! multiudpsink name=udpsink");
#else
  pipelineString += NS_LITERAL_CSTRING(
          " ! decodebin ! audioconvert ! audioresample ! lame ! mp3parse ! "
          "rtpmpapay name=payloader ! multiudpsink name=udpsink");
#endif

  GError *error = NULL;
  nsresult rv;
  GstElement *sink, *payloader;
  GstPad *srcpad;
  nsCString host;

  mPipeline = gst_parse_launch (pipelineString.BeginReading(), &error);

  if (!mPipeline) {
    // TODO: Report the error more usefully using the GError
    rv = NS_ERROR_FAILURE;
    return rv;
  }

  sink = gst_bin_get_by_name (GST_BIN (mPipeline), "udpsink");
  // Add the host and port to stream to
  host = NS_ConvertUTF16toUTF8(mDestHost);
  g_signal_emit_by_name (sink, "add", host.BeginReading(), mDestPort);
  gst_object_unref (sink);

  payloader = gst_bin_get_by_name (GST_BIN (mPipeline), "payloader");
  srcpad = gst_element_get_request_pad(payloader, "src");
  g_signal_connect (srcpad, "notify::caps", (GCallback) capsNotifyHelper, this);
  gst_object_unref (srcpad);
  gst_object_unref (payloader);

  SetPipelineOp(GStreamer::OP_STREAMING);

  return NS_OK;
}

/* static */ void
sbGStreamerRTPStreamer::capsNotifyHelper(GObject* obj, GParamSpec* pspec,
        sbGStreamerRTPStreamer *streamer)
{
  GstPad *pad = GST_PAD(obj);
  GstCaps *caps = gst_pad_get_current_caps(pad);

  if (caps) {
    streamer->OnCapsSet(caps);
    gst_caps_unref (caps);
  }
}

void
sbGStreamerRTPStreamer::OnCapsSet(GstCaps *caps)
{
  GstSDPMessage *sdp;
  GstSDPMedia *media;
  GstStructure *s;
  const gchar *capsstr, *encoding_name, *encoding_params;
  int pt, rate;
  gchar *tmp;
  const char *stdproperties[] = {"media", "payload", "clock-rate", 
      "encoding-name", "encoding-params", "ssrc", "clock-base", 
      "seqnum-base"};

  // Following is largely based on rtsp-sdp.c from gst-rtsp-server
 
  gst_sdp_message_new (&sdp);
  // SDP is always version 0
  gst_sdp_message_set_version (sdp, "0");
  gst_sdp_message_set_origin (sdp,
          "-",          // Username, optional
          "1234567890", // Session ID, recommended that this is an NTP timestamp
          "1",          // Version, hard code to 1 for now
          "IN",         // Network type, always IN (Internet) 
          "IP4",        // Address type, for now hardcode to IPv4
          "127.0.0.1"); // Actual origin address. TODO: Set accurately?
  gst_sdp_message_set_session_name (sdp, "Songbird RTP Stream");
  gst_sdp_message_set_information (sdp, "Streaming from Songbird");
  gst_sdp_message_add_time (sdp, "0", "0", NULL);
  gst_sdp_message_add_attribute (sdp, "tool", "songbird");

  // TODO: Add a 'range' attribute with the correct info about the media
  // item; that way we can EOS at the appropriate time (requires more recent
  // gstreamer than we have though - backport patches?)
  // To do that, we'll want to change sourceURI to be a mediaitem, I think.

  gst_sdp_media_new (&media);
  s = gst_caps_get_structure (caps, 0);
  // Take media type and payload type number from the caps.
  capsstr = gst_structure_get_string (s, "media");
  gst_sdp_media_set_media (media, capsstr);

  gst_structure_get_int (s, "payload", &pt);
  tmp = g_strdup_printf("%d", pt);
  gst_sdp_media_add_format (media, tmp);
  g_free (tmp);

  gst_sdp_media_set_port_info (media, mDestPort, 1);
  gst_sdp_media_set_proto (media, "RTP/AVP");

  // Connection info. Since we're not using multicast, we don't actually know
  // what IP the client will be receiving packets from. So, we specify the 
  // address as 0.0.0.0, or INADDR_ANY
  gst_sdp_media_add_connection (media, "IN", "IP4", "0.0.0.0", 0, 0);

  gst_structure_get_int (s, "clock-rate", &rate);
  encoding_name = gst_structure_get_string (s, "encoding-name");
  encoding_params = gst_structure_get_string (s, "encoding-params");
  if (encoding_params)
    tmp = g_strdup_printf ("%d %s/%d/%s", pt, encoding_name, rate, 
            encoding_params);
  else
      tmp = g_strdup_printf ("%d %s/%d", pt, encoding_name, rate);
  gst_sdp_media_add_attribute (media, "rtpmap", tmp);
  g_free (tmp);

  // Generate fmtp (format-specific configuration info) attribute
  int fields = gst_structure_n_fields (s);
  GString *fmtp = g_string_new ("");
  g_string_append_printf (fmtp, "%d ", pt);
  bool first = true;
  for (int i = 0; i < fields; i++)
  {
    bool skip = false;
    const gchar *fieldname = gst_structure_nth_field_name (s, i);
    const gchar *fieldval;
    for (unsigned int j = 0; 
            j < sizeof (stdproperties)/sizeof(*stdproperties); j++)
    {
      if (!strcmp (fieldname, stdproperties[i]))
        skip = true;
    }
    if (!skip && (fieldval = gst_structure_get_string (s, fieldname)))
    {
      if (!first) {
        g_string_append_printf (fmtp, ";");
      }
      first = false;
      g_string_append_printf (fmtp, "%s=%s", fieldname, fieldval);
    }
  }

  if (!first)
  {
    gst_sdp_media_add_attribute (media, "fmtp", fmtp->str);
  }
  g_string_free (fmtp, TRUE);

  gst_sdp_message_add_media (sdp, media);
  gst_sdp_media_free (media);

  gchar *text = gst_sdp_message_as_text (sdp);
  nsCString sdptext(text);
  g_free (text);

  gst_sdp_message_free (sdp);

  // Now send an event to let listeners know about the SDP.
  nsCOMPtr<nsIVariant> sdpVariant = sbNewVariant(sdptext).get();
  DispatchMediacoreEvent (sbIGStreamerRTPStreamer::EVENT_SDP_AVAILABLE,
          sdpVariant);
}



