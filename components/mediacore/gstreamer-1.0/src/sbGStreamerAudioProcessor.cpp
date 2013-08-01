/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbGStreamerAudioProcessor.h"

#include <sbIGStreamerService.h>
#include <sbIMediaFormatMutable.h>

#include <sbClassInfoUtils.h>
#include <sbStandardProperties.h>
#include <sbStringBundle.h>
#include <sbThreadUtils.h>
#include <sbVariantUtils.h>

#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsStringAPI.h>
#include <prlog.h>

#include <gst/base/gstadapter.h>
#include <gst/app/gstappsink.h>

/**
 * To log this class, set the following environment variable in a debug build:
 *  NSPR_LOG_MODULES=sbGStreamerAudioProcessor:5 (or :3 for LOG messages only)
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gGStreamerAudioProcessor = PR_NewLogModule("sbGStreamerAudioProcessor");
#define LOG(args) PR_LOG(gGStreamerAudioProcessor, PR_LOG_WARNING, args)
#define TRACE(args) PR_LOG(gGStreamerAudioProcessor, PR_LOG_DEBUG, args)
#else /* PR_LOGGING */
#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */
#endif /* PR_LOGGING */

NS_IMPL_THREADSAFE_ISUPPORTS3(sbGStreamerAudioProcessor,
                              sbIGStreamerPipeline,
                              sbIMediacoreAudioProcessor,
                              nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER2(sbGStreamerAudioProcessor,
                             sbIGStreamerPipeline,
                             sbIMediacoreAudioProcessor);

NS_DECL_CLASSINFO(sbGstreamerAudioProcessor);
NS_IMPL_THREADSAFE_CI(sbGStreamerAudioProcessor);

sbGStreamerAudioProcessor::sbGStreamerAudioProcessor() :
  sbGStreamerPipeline(),
  mConstraintChannelCount(0),
  mConstraintSampleRate(0),
  mConstraintAudioFormat(sbIMediacoreAudioProcessor::FORMAT_ANY),
  mConstraintBlockSize(0),
  mConstraintBlockSizeBytes(0),
  mMonitor(NULL),
  mAdapter(NULL),
  mAppSink(NULL),
  mCapsFilter(NULL),
  mSuspended(PR_FALSE),
  mFoundAudioPad(PR_FALSE),
  mHasStarted(PR_FALSE),
  mIsEOS(PR_FALSE),
  mIsEndOfSection(PR_FALSE),
  mHasSentError(PR_FALSE),
  mSendGap(PR_FALSE),
  mSampleNumber(0),
  mExpectedNextSampleNumber(0),
  mAudioFormat(0),
  mSampleRate(0),
  mChannels(0),
  mBuffersAvailable(0),
  mPendingBuffer(NULL)
{
}

sbGStreamerAudioProcessor::~sbGStreamerAudioProcessor()
{
  if (mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

/* sbIMediacoreAudioProcessor interface implementation */

/* void init (in sbIMediacoreAudioProcessorListener aListener); */
NS_IMETHODIMP
sbGStreamerAudioProcessor::Init(sbIMediacoreAudioProcessorListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ENSURE_FALSE(mListener, NS_ERROR_ALREADY_INITIALIZED);

  mMonitor = nsAutoMonitor::NewMonitor("AudioProcessor::mMonitor");

  mListener = aListener;
  return NS_OK;
}

/* attribute unsigned long constraintSampleRate; */
NS_IMETHODIMP
sbGStreamerAudioProcessor::GetConstraintSampleRate(PRUint32 *aConstraintSampleRate)
{
  NS_ENSURE_ARG_POINTER(aConstraintSampleRate);

  *aConstraintSampleRate = mConstraintSampleRate;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerAudioProcessor::SetConstraintSampleRate(PRUint32 aConstraintSampleRate)
{
  NS_ENSURE_FALSE(mPipeline, NS_ERROR_ALREADY_INITIALIZED);

  mConstraintSampleRate = aConstraintSampleRate;
  return NS_OK;
}

/* attribute unsigned long constraintChannelCount; */
NS_IMETHODIMP
sbGStreamerAudioProcessor::GetConstraintChannelCount(PRUint32 *aConstraintChannelCount)
{
  NS_ENSURE_ARG_POINTER(aConstraintChannelCount);

  *aConstraintChannelCount = mConstraintChannelCount;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerAudioProcessor::SetConstraintChannelCount(PRUint32 aConstraintChannelCount)
{
  NS_ENSURE_FALSE(mPipeline, NS_ERROR_ALREADY_INITIALIZED);

  // More than 2 channels is not currently supported.
  if (aConstraintChannelCount > 2)
    return NS_ERROR_INVALID_ARG;

  mConstraintChannelCount = aConstraintChannelCount;
  return NS_OK;
}

/* attribute unsigned long constraintBlockSize; */
NS_IMETHODIMP
sbGStreamerAudioProcessor::GetConstraintBlockSize(PRUint32 *aConstraintBlockSize)
{
  NS_ENSURE_ARG_POINTER(aConstraintBlockSize);

  *aConstraintBlockSize = mConstraintBlockSize;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerAudioProcessor::SetConstraintBlockSize(PRUint32 aConstraintBlockSize)
{
  NS_ENSURE_FALSE(mPipeline, NS_ERROR_ALREADY_INITIALIZED);

  mConstraintBlockSize = aConstraintBlockSize;
  return NS_OK;
}

/* attribute unsigned long constraintAudioFormat; */
NS_IMETHODIMP
sbGStreamerAudioProcessor::GetConstraintAudioFormat(PRUint32 *aConstraintAudioFormat)
{
  NS_ENSURE_ARG_POINTER(aConstraintAudioFormat);

  *aConstraintAudioFormat = mConstraintAudioFormat;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerAudioProcessor::SetConstraintAudioFormat(PRUint32 aConstraintAudioFormat)
{
  // Ensure it's a valid format constant
  NS_ENSURE_ARG_RANGE(aConstraintAudioFormat, FORMAT_ANY, FORMAT_FLOAT);
  NS_ENSURE_FALSE(mPipeline, NS_ERROR_ALREADY_INITIALIZED);

  mConstraintAudioFormat = aConstraintAudioFormat;
  return NS_OK;
}

/* void start (in sbIMediaItem aItem); */
NS_IMETHODIMP
sbGStreamerAudioProcessor::Start(sbIMediaItem *aItem)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_TRUE (NS_IsMainThread(), NS_ERROR_FAILURE);
  NS_ENSURE_ARG_POINTER (aItem);
  NS_ENSURE_STATE (mListener);
  NS_ENSURE_FALSE (mPipeline, NS_ERROR_FAILURE);

  mMediaItem = aItem;

  nsresult rv = PlayPipeline();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void stop (); */
NS_IMETHODIMP
sbGStreamerAudioProcessor::Stop()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_TRUE (NS_IsMainThread(), NS_ERROR_FAILURE);

  // It's permissible to call stop() at any time; if we don't have a pipeline
  // then we just don't need to do anything.
  if (!mPipeline)
    return NS_OK;

  nsresult rv = StopPipeline();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = DestroyPipeline();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* void suspend (); */
NS_IMETHODIMP
sbGStreamerAudioProcessor::Suspend()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_TRUE (NS_IsMainThread(), NS_ERROR_FAILURE);
  NS_ENSURE_STATE (mPipeline);

  nsAutoMonitor mon(mMonitor);
  mSuspended = PR_TRUE;
  return NS_OK;
}

/* void resume (); */
NS_IMETHODIMP
sbGStreamerAudioProcessor::Resume()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_TRUE (NS_IsMainThread(), NS_ERROR_FAILURE);
  NS_ENSURE_STATE (mPipeline);

  nsAutoMonitor mon(mMonitor);
  mSuspended = PR_FALSE;

  nsresult rv = ScheduleSendDataIfAvailable();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::BuildPipeline()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  /* Our pipeline is going to look like this - it's pretty simple at this level.
   *
   * [uridecodebin]-[audioconvert]-[audioresample]-[capsfilter]-[appsink]
   *
   * Most of the complexity is in a) how we configure the capsfilter, and
   * b) what we do in appsink.
   *
   * The capsfilter is configured twice:
   *   1. Initially, it's set up to match the constraint values set on the
   *      interface.
   *   2. Once we've got a fixed format (which we send along with the START
   *      event, we re-configure the filter to ONLY accept this format. This
   *      ensures that, if the underlying format changes mid-stream, what we
   *      deliver to the listener does not change.
   */
  mPipeline = gst_pipeline_new ("audio-processor");
  NS_ENSURE_TRUE (mPipeline, NS_ERROR_FAILURE);

  GstElement *uridecodebin = gst_element_factory_make(
          "uridecodebin",
          "audio-processor-decoder");
  if (!uridecodebin) {
    g_object_unref (mPipeline);
    mPipeline = NULL;

    return NS_ERROR_FAILURE;
  }

  // Set the source URI from our media item
  nsString contentURL;
  nsresult rv = mMediaItem->GetProperty(
          NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
          contentURL);
  NS_ENSURE_SUCCESS(rv, rv);

  // Use the content URL as the display name for this item (used for
  // error reporting, etc.)
  mResourceDisplayName = contentURL;

  g_object_set (uridecodebin, "uri",
          NS_ConvertUTF16toUTF8(contentURL).BeginReading(), NULL);

  // Connect to callbacks for when uridecodebin adds a new pad that we (may)
  // want to connect up to the rest of the pipeline, and for when we're not
  // going to get any more pads (so that we can fire an error event if we
  // haven't received an audio pad)
  g_signal_connect (uridecodebin, "pad-added",
          G_CALLBACK (decodebin_pad_added_cb), this);
  g_signal_connect (uridecodebin, "no-more-pads",
          G_CALLBACK (decodebin_no_more_pads_cb), this);

  gst_bin_add (GST_BIN (mPipeline), uridecodebin);

  mAdapter = gst_adapter_new();

  return NS_OK;
}

void
sbGStreamerAudioProcessor::HandleMessage (GstMessage *message)
{
  // We override the base pipeline message handling - all we want to do here
  // is deal with errors.
  nsresult rv;
  GstMessageType msgtype = GST_MESSAGE_TYPE(message);
  switch(msgtype) {
    case GST_MESSAGE_ERROR:
    {
      gchar *debug = NULL;
      GError *gerror = NULL;

      if (mHasSentError)
      {
        LOG(("Ignoring multiple error messages"));
        return;
      }

      gst_message_parse_error(message, &gerror, &debug);

      nsCOMPtr<sbIMediacoreError> error;
      rv = GetMediacoreErrorFromGstError(gerror, mResourceDisplayName,
                                         GStreamer::OP_UNKNOWN,
                                         getter_AddRefs(error));
      NS_ENSURE_SUCCESS(rv, /* void */);

      rv = SendEventAsync(sbIMediacoreAudioProcessorListener::EVENT_ERROR,
                          sbNewVariant(error).get());
      NS_ENSURE_SUCCESS(rv, /* void */);

      g_error_free (gerror);
      g_free(debug);

      mHasSentError = PR_TRUE;

      break;
    }
    default:
      LOG(("Ignoring message: %s", gst_message_type_get_name(msgtype)));
      break;
  }
}

/* static */ void
sbGStreamerAudioProcessor::decodebin_pad_added_cb (
        GstElement * uridecodebin,
        GstPad * pad,
        sbGStreamerAudioProcessor *processor)
{
  nsresult rv = processor->DecoderPadAdded(uridecodebin, pad);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

/* static */ void
sbGStreamerAudioProcessor::decodebin_no_more_pads_cb (
        GstElement * uridecodebin,
        sbGStreamerAudioProcessor *processor)
{
  nsresult rv = processor->DecoderNoMorePads(uridecodebin);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

/* static */ void
sbGStreamerAudioProcessor::appsink_new_buffer_cb (
        GstElement * appsink,
        sbGStreamerAudioProcessor *processor)
{
  nsresult rv = processor->AppsinkNewBuffer(appsink);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

/* static */ void
sbGStreamerAudioProcessor::appsink_eos_cb (
        GstElement * appsink,
        sbGStreamerAudioProcessor *processor)
{
  nsresult rv = processor->AppsinkEOS(appsink);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

nsresult
sbGStreamerAudioProcessor::DecoderPadAdded (GstElement *uridecodebin,
                                            GstPad *pad)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  // A new decoded pad has been added from the decodebin. If it's the first
  // audio stream, we use it. Otherwise, we ignore it.
  GstCaps *caps = gst_pad_get_caps (pad);
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  const gchar *name = gst_structure_get_name (structure);
  bool isAudio = g_str_has_prefix (name, "audio/");

  gst_caps_unref (caps);

  if (!isAudio) {
    LOG(("Ignoring non audio pad"));
    return NS_OK;
  }

  if (mFoundAudioPad) {
    LOG(("Ignoring additional audio pad"));
    return NS_OK;
  }

  mFoundAudioPad = PR_TRUE;

  // Now we can build the rest of the pipeline
  GstElement *audioconvert = NULL;
  GstElement *audioresample = NULL;
  GstElement *capsfilter = NULL;
  GstElement *appsink = NULL;
  GstPad *sinkpad;

  audioconvert = gst_element_factory_make("audioconvert", NULL);
  audioresample = gst_element_factory_make("audioresample", NULL);
  capsfilter = gst_element_factory_make("capsfilter", NULL);
  appsink = gst_element_factory_make("appsink", NULL);

  if (!audioconvert || !audioresample || !capsfilter || !appsink) {
    LOG(("Missing base elements, corrupt GStreamer install"));
    goto failed;
  }

  rv = ConfigureInitialCapsfilter(capsfilter);
  NS_ENSURE_SUCCESS(rv, rv);

  // Disable sync (we're not realtime) and enable signal emission so that
  // new-buffer signal is sent.
  // Set max-buffers to 10 (to allow some buffering) - we don't want to buffer
  // the entire file, decoded, internally.
  g_object_set (appsink,
          "emit-signals", TRUE,
          "sync", FALSE,
          "max-buffers", 10,
          NULL);
  g_signal_connect (appsink, "new-buffer",
          G_CALLBACK (appsink_new_buffer_cb), this);
  g_signal_connect (appsink, "eos",
          G_CALLBACK (appsink_eos_cb), this);

  gst_bin_add_many (GST_BIN (mPipeline),
          audioconvert, audioresample, capsfilter, appsink, NULL);
  gst_element_link_many (audioconvert, audioresample, capsfilter, appsink, NULL);

  sinkpad = gst_element_get_static_pad (audioconvert, "sink");
  gst_pad_link (pad, sinkpad);
  gst_object_unref (sinkpad);

  gst_element_set_state (audioconvert, GST_STATE_PLAYING);
  gst_element_set_state (audioresample, GST_STATE_PLAYING);
  gst_element_set_state (capsfilter, GST_STATE_PLAYING);
  gst_element_set_state (appsink, GST_STATE_PLAYING);

  mAppSink = (GstAppSink *)gst_object_ref (appsink);
  mCapsFilter = (GstElement *)gst_object_ref (capsfilter);

  return NS_OK;

failed:
  if (audioconvert)
    g_object_unref (audioconvert);
  if (audioresample)
    g_object_unref (audioresample);
  if (capsfilter)
    g_object_unref (capsfilter);
  if (appsink)
    g_object_unref (appsink);

  return NS_ERROR_FAILURE;
}

nsresult
sbGStreamerAudioProcessor::OnDestroyPipeline(GstElement *pipeline)
{
  if (mAppSink) {
    gst_object_unref (mAppSink);
    mAppSink = NULL;
  }
  if (mCapsFilter) {
    gst_object_unref (mCapsFilter);
    mCapsFilter = NULL;
  }

  if (mAdapter) {
    g_object_unref (mAdapter);
    mAdapter = NULL;
  }

  if (mPendingBuffer) {
    gst_buffer_unref (mPendingBuffer);
    mPendingBuffer = NULL;
  }

  // And... reset all our state tracking variables.
  mSuspended = PR_FALSE;
  mFoundAudioPad = PR_FALSE;
  mHasStarted = PR_FALSE;
  mIsEOS = PR_FALSE;
  mIsEndOfSection = PR_FALSE;
  mHasSentError = PR_FALSE;
  mSendGap = PR_FALSE;
  mSampleNumber = 0;
  mExpectedNextSampleNumber = 0;
  mAudioFormat = FORMAT_ANY;
  mSampleRate = 0;
  mChannels = 0;
  mBuffersAvailable = 0;

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::ConfigureInitialCapsfilter(GstElement *capsfilter)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstCaps *caps;
  GstStructure *structure;

  caps = gst_caps_new_empty ();
  if (mConstraintAudioFormat == sbIMediacoreAudioProcessor::FORMAT_ANY ||
      mConstraintAudioFormat == sbIMediacoreAudioProcessor::FORMAT_INT16)
  {
    structure = gst_structure_new ("audio/x-raw-int",
            "endianness", G_TYPE_INT, G_BYTE_ORDER,
            "width", G_TYPE_INT, 16,
            "depth", G_TYPE_INT, 16,
            NULL);
    if (mConstraintSampleRate) {
      gst_structure_set(structure,
              "rate", G_TYPE_INT, mConstraintSampleRate, NULL);
    }

    if (mConstraintChannelCount) {
      gst_structure_set(structure,
              "channels", G_TYPE_INT, mConstraintChannelCount, NULL);
    }
    else {
      // We don't currently support > 2 channels, so even if no constraint is
      // set explicitly, we limit to 1 or 2 channels.
      gst_structure_set(structure, "channels", GST_TYPE_INT_RANGE, 1, 2, NULL);
    }
    gst_caps_append_structure (caps, structure);
  }

  if (mConstraintAudioFormat == sbIMediacoreAudioProcessor::FORMAT_ANY ||
      mConstraintAudioFormat == sbIMediacoreAudioProcessor::FORMAT_FLOAT)
  {
    structure = gst_structure_new ("audio/x-raw-float",
            "endianness", G_TYPE_INT, G_BYTE_ORDER,
            "width", G_TYPE_INT, 32,
            NULL);
    if (mConstraintSampleRate) {
      gst_structure_set(structure,
              "rate", G_TYPE_INT, mConstraintSampleRate, NULL);
    }

    if (mConstraintChannelCount) {
      gst_structure_set(structure,
              "channels", G_TYPE_INT, mConstraintChannelCount, NULL);
    }
    else {
      // We don't currently support > 2 channels, so even if no constraint is
      // set explicitly, we limit to 1 or 2 channels.
      gst_structure_set(structure, "channels", GST_TYPE_INT_RANGE, 1, 2, NULL);
    }
    gst_caps_append_structure (caps, structure);
  }

  g_object_set (capsfilter, "caps", caps, NULL);

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::ReconfigureCapsfilter()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  GstCaps *caps;

  if (mAudioFormat == sbIMediacoreAudioProcessor::FORMAT_INT16)
  {
    caps = gst_caps_new_simple ("audio/x-raw-int",
            "endianness", G_TYPE_INT, G_BYTE_ORDER,
            "width", G_TYPE_INT, 16,
            "depth", G_TYPE_INT, 16,
            "rate", G_TYPE_INT, mSampleRate,
            "channels", G_TYPE_INT, mChannels,
            NULL);
  }
  else {
    caps = gst_caps_new_simple ("audio/x-raw-float",
            "endianness", G_TYPE_INT, G_BYTE_ORDER,
            "width", G_TYPE_INT, 32,
            "rate", G_TYPE_INT, mSampleRate,
            "channels", G_TYPE_INT, mChannels,
            NULL);
  }

  g_object_set (mCapsFilter, "caps", caps, NULL);

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::DecoderNoMorePads (GstElement *uridecodebin)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  if (!mFoundAudioPad) {
    // Looks like we didn't find any audio pads at all. Fire off an error.
    nsresult rv = SendErrorEvent(sbIMediacoreError::SB_STREAM_WRONG_TYPE,
                                 "mediacore.error.wrong_type");
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

PRBool
sbGStreamerAudioProcessor::HasEnoughData()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsAutoMonitor mon(mMonitor);

  guint available = gst_adapter_available (mAdapter);

  // We have to have at least one byte of data to have enough in all cases.
  // Then, we need:
  //   - more bytes than the constraint block size (so we can send a block)
  //   - whatever we have if mIsEndOfSection is set, since this is going to
  //     be followed by a GAP event
  //   - OR, if we're at EOS, we have no more buffers available - this ensures
  //     that we don't send a partial block after getting EOS just because we
  //     haven't actually pulled remaining data from the appsink.
  return (available > 0 &&
          ((mIsEOS && !mBuffersAvailable) ||
           mIsEndOfSection ||
           available >= mConstraintBlockSizeBytes));
}

PRUint64
sbGStreamerAudioProcessor::GetSampleNumberFromBuffer(GstBuffer *buf)
{
  GstClockTime timestamp = GST_BUFFER_TIMESTAMP (buf);
  if (timestamp == GST_CLOCK_TIME_NONE) {
    // We have to assume it's contiguous with the previous buffer in this case
    return mExpectedNextSampleNumber;
  }

  return gst_util_uint64_scale_int_round (timestamp, mSampleRate, GST_SECOND) *
      mChannels;
}

PRUint32
sbGStreamerAudioProcessor::GetDurationFromBuffer(GstBuffer *buf)
{
  int size;
  if (mAudioFormat == FORMAT_FLOAT)
    size = sizeof(float);
  else
    size = sizeof(short);

  return gst_buffer_get_size(buf) / size;
}

void
sbGStreamerAudioProcessor::GetMoreData()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsAutoMonitor mon(mMonitor);

  NS_ENSURE_TRUE (mBuffersAvailable > 0, /* void */);

  if (mPendingBuffer) {
    // We use pending buffers only when it follows a gap/discontinuity, and thus
    // we won't get here until that data has all been sent.
    NS_ASSERTION(gst_adapter_available(mAdapter) == 0,
        "Had pending buffer but asked to get more data when adapter not empty");

    mSampleNumber = GetSampleNumberFromBuffer(mPendingBuffer);
    gst_adapter_push(mAdapter, mPendingBuffer);
    mSendGap = TRUE;
    mExpectedNextSampleNumber = mSampleNumber +
        GetDurationFromBuffer(mPendingBuffer);

    mBuffersAvailable--;
    mPendingBuffer = NULL;
    mIsEndOfSection = PR_FALSE;
  }
  else {
    GstBuffer *buf = gst_app_sink_pull_buffer(mAppSink);
    NS_ASSERTION(buf, "pulled buffer when asked to get more but got no buffer");

    // Consider this a discontinuity if the sample numbers are discontinuous,
    // or we get a DISCONT buffer _other than_ at the very start of the stream.
    PRUint64 nextSampleNumber = GetSampleNumberFromBuffer(buf);
    if (nextSampleNumber != mExpectedNextSampleNumber ||
        (GST_BUFFER_IS_DISCONT (buf) && mExpectedNextSampleNumber != 0))
    {
      LOG(("Discontinuity found"));
      // We include mPendingBuffer in mAvailableBuffers, so don't need to
      // decrement in this case.
      mPendingBuffer = buf;
      mIsEndOfSection = PR_TRUE;
    }
    else {
      mBuffersAvailable--;

      if (gst_adapter_available(mAdapter) == 0)
        mSampleNumber = nextSampleNumber;
      gst_adapter_push(mAdapter, buf);
      mExpectedNextSampleNumber += GetDurationFromBuffer(buf);
    }
  }
}

nsresult
sbGStreamerAudioProcessor::AppsinkNewBuffer(GstElement *appsink)
{
  nsresult rv;
  nsAutoMonitor mon(mMonitor);

  // Once we get the first chunk of data, we can determine what format we will
  // send to consumers.
  if (mAudioFormat == FORMAT_ANY) {
    rv = DetermineFormat();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mBuffersAvailable++;

  if (!HasEnoughData()) {
    GetMoreData();

    if (HasEnoughData()) {
      rv = ScheduleSendData();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::ScheduleSendDataIfAvailable()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;

  nsAutoMonitor mon(mMonitor);

  if (HasEnoughData()) {
    rv = ScheduleSendData();
    NS_ENSURE_SUCCESS(rv, rv);
    return NS_OK;
  }
  else {
    while(mBuffersAvailable) {
      GetMoreData();

      if (HasEnoughData()) {
        rv = ScheduleSendData();
        NS_ENSURE_SUCCESS(rv, rv);
        return NS_OK;
      }
    }
  }

  if (mIsEOS) {
    rv = SendEventAsync(sbIMediacoreAudioProcessorListener::EVENT_EOS, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::ScheduleSendData()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsCOMPtr<nsIThread> mainThread;
  nsresult rv = NS_GetMainThread(getter_AddRefs(mainThread));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbGStreamerAudioProcessor, this, SendDataToListener);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

  rv = mainThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::DetermineFormat()
{
  GstPad *appsinkSinkPad = gst_element_get_static_pad (GST_ELEMENT (mAppSink),
                                                       "sink");
  GstCaps *caps = gst_pad_get_current_caps(appsinkSinkPad);
  if (!caps)
    return NS_ERROR_FAILURE;

  GstStructure *structure = gst_caps_get_structure(caps, 0);
  const gchar *capsName = gst_structure_get_name (structure);

  if (g_str_equal(capsName, "audio/x-raw-float")) {
    mAudioFormat = FORMAT_FLOAT;
    mConstraintBlockSizeBytes = mConstraintBlockSize * sizeof(float);
  }
  else {
    mAudioFormat = FORMAT_INT16;
    mConstraintBlockSizeBytes = mConstraintBlockSize * sizeof(short);
  }

  gst_structure_get_int (structure, "rate", &mSampleRate);
  gst_structure_get_int (structure, "channels", &mChannels);

  gst_caps_unref (caps);

  // Now we fix the capsfilter to these settings, so that if the underlying
  // stream changes, our output does not.
  nsresult rv = ReconfigureCapsfilter();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::DoStreamStart()
{
  nsresult rv;
  nsCOMPtr<sbIMediaFormatAudioMutable> audioFormat =
        do_CreateInstance(SB_MEDIAFORMATAUDIO_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = audioFormat->SetAudioType(NS_LITERAL_STRING ("audio/x-raw"));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = audioFormat->SetSampleRate(mSampleRate);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = audioFormat->SetChannels(mChannels);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> audioFormatISupports = do_QueryInterface(audioFormat,
                                                                 &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SendEventSync(sbIMediacoreAudioProcessorListener::EVENT_START,
                     sbNewVariant(audioFormatISupports).get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbGStreamerAudioProcessor::SendDataToListener()
{
  nsresult rv;
  const guint8 *data;
  guint bytesRead = 0;

  nsAutoMonitor mon(mMonitor);

  // It's possible that the pipeline was stopped (on the main thread) before
  // this queued event was run; in that case we just return.
  if (!mPipeline)
    return;

  NS_ASSERTION(HasEnoughData(), "Asked to send data, but cannot");

  if (mSuspended)
    return;

  if (!mHasStarted) {
    mHasStarted = PR_TRUE;
    // Drop monitor to send event to the listener.
    mon.Exit();

    rv = DoStreamStart();
    NS_ENSURE_SUCCESS(rv, /*void*/);

    mon.Enter();

    if (!mHasStarted || mSuspended) {
      // Got stopped or suspended from the start event; nothing to do now.
      return;
    }
  }

  guint available = gst_adapter_available (mAdapter);
  if (mConstraintBlockSize == 0)
    bytesRead = available;
  else if (available >= mConstraintBlockSizeBytes)
    bytesRead = mConstraintBlockSizeBytes;
  else if (mIsEOS || mIsEndOfSection)
    bytesRead = available;
  else
    NS_NOTREACHED("not enough data here");

  data = gst_adapter_map(mAdapter, bytesRead);

  PRUint32 sampleNumber = mSampleNumber;
  PRUint32 numSamples;

  PRBool sendGap = mSendGap;
  mSendGap = PR_FALSE;

  // Call listener with the monitor released.
  mon.Exit();

  if (sendGap) {
    rv = SendEventSync(sbIMediacoreAudioProcessorListener::EVENT_GAP, nsnull);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }

  if (mAudioFormat == FORMAT_INT16) {
    numSamples = bytesRead / sizeof(PRInt16);
    PRInt16 *sampleData = (PRInt16 *)data;

    rv = mListener->OnIntegerAudioDecoded(sampleNumber, numSamples, sampleData);
  }
  else {
    numSamples = bytesRead / sizeof(float);
    float *sampleData = (float *)data;

    rv = mListener->OnFloatAudioDecoded(sampleNumber, numSamples, sampleData);
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("Listener failed to receive data");
  }

  mon.Enter();

  // If we're no longer started, that means that we got stopped while the
  // monitor was released. Don't try to touch any further state!
  if (!mHasStarted)
    return;

  mSampleNumber += numSamples;

  gst_adapter_unmap(mAdapter);
  
  // Listener might have paused or stopped us; in that case we don't want to
  // schedule another send.
  if (mSuspended)
    return;

  rv = ScheduleSendDataIfAvailable();
  NS_ENSURE_SUCCESS(rv, /* void */);
}

nsresult
sbGStreamerAudioProcessor::AppsinkEOS(GstElement *appsink)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;

  nsAutoMonitor mon(mMonitor);

  // If we have enough data already, then processing is in-progress; we don't
  // need to do anything specific.
  if (!HasEnoughData()) {
    mIsEOS = PR_TRUE;

    // Otherwise: either setting EOS will make us send the final partial chunk
    // of data, followed by an EOS, or we've already finished with ALL data,
    // so we should just immediately send EOS.
    if (HasEnoughData()) {
      rv = ScheduleSendData();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      if (mHasStarted) {
        rv = SendEventAsync(sbIMediacoreAudioProcessorListener::EVENT_EOS,
                            nsnull);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else if (!mHasSentError) {
        // Send an error - it's an error to reach EOS without having received
        // any audio samples at all.
        rv = SendErrorEvent(sbIMediacoreError::SB_STREAM_DECODE,
                            "mediacore.error.decode_failed");
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  else {
    mIsEOS = PR_TRUE;
  }

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::SendErrorEvent(PRUint32 errorCode,
                                          const char *errorName)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  sbStringBundle bundle;
  nsString errorMessage = bundle.Get(errorName);

  nsRefPtr<sbMediacoreError> error;
  NS_NEWXPCOM(error, sbMediacoreError);
  error->Init(errorCode, errorMessage);

  nsresult rv = SendEventAsync(sbIMediacoreAudioProcessorListener::EVENT_ERROR,
                               sbNewVariant(error).get());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::SendEventInternal(PRUint32 eventType,
                                             nsCOMPtr<nsIVariant> eventDetails)
{
  nsresult rv;

  LOG(("Sending event of type %d", eventType));
  rv = mListener->OnEvent(eventType, eventDetails);
  if (NS_FAILED(rv)) {
    LOG(("Listener returned error from OnEvent: %x", rv));
  }

  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::SendEventAsync(PRUint32 eventType,
                                          nsIVariant *eventDetails)
{
  nsresult rv;

  LOG(("Scheduling sending event of type %d", eventType));

  // Hold on to this so that the object doesn't get unreffed and go away before
  // the call completes.
  nsCOMPtr<nsIVariant> details(eventDetails);
  rv = sbInvokeOnMainThread2Async(*this,
                                  &sbGStreamerAudioProcessor::SendEventInternal,
                                  NS_ERROR_FAILURE,
                                  eventType,
                                  details);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult
sbGStreamerAudioProcessor::SendEventSync(PRUint32 eventType,
                                         nsIVariant *eventDetails)
{
  NS_ASSERTION(NS_IsMainThread(),
          "SendEventSync() must be called from the main thread");
  nsCOMPtr<nsIVariant> details(eventDetails);
  nsresult rv = SendEventInternal(eventType, details);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
