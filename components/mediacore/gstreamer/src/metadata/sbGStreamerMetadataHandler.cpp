// vim: set sw=2 :
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

#include "sbGStreamerMetadataHandler.h"

#include "sbGStreamerMediacoreCID.h"
#include "sbGStreamerMediacoreUtils.h"

#include <sbIGStreamerService.h>
#include <sbIMediacoreCapabilities.h>

#include <sbMemoryUtils.h>
#include <sbStandardProperties.h>
#include <sbPropertiesCID.h>

#include <nsComponentManagerUtils.h>
#include <nsProxyRelease.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsIStringEnumerator.h>
#include <nsIURI.h>
#include <nsUnicharUtils.h>

#include <prlog.h>
#include <prtime.h>

#include <gst/pbutils/missing-plugins.h>

#define METADATA_TIMEOUT (30 * PR_MSEC_PER_SEC)

#ifdef PR_LOGGING
static PRLogModuleInfo* gGSTMetadataLog = nsnull;
#define TRACE(args) \
  PR_BEGIN_MACRO \
    if (!gGSTMetadataLog) \
      gGSTMetadataLog = PR_NewLogModule("gstmetadata"); \
    PR_LOG(gGSTMetadataLog, PR_LOG_DEBUG, args); \
  PR_END_MACRO
#define LOG(args) \
  PR_BEGIN_MACRO \
    if (!gGSTMetadataLog) \
      gGSTMetadataLog = PR_NewLogModule("gstmetadata"); \
    PR_LOG(gGSTMetadataLog, PR_LOG_WARN, args); \
  PR_END_MACRO
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

SB_AUTO_CLASS(sbGstElement,
              GstElement*,
              !!mValue,
              gst_object_unref(mValue),
              mValue = NULL);
SB_AUTO_CLASS(sbGstBus,
              GstBus*,
              !!mValue,
              gst_object_unref(mValue),
              mValue = NULL);
SB_AUTO_CLASS(sbGstPad,
              GstPad*,
              !!mValue,
              gst_object_unref(mValue),
              mValue = NULL);

NS_IMPL_THREADSAFE_ISUPPORTS2(sbGStreamerMetadataHandler,
                              sbIMetadataHandler,
                              nsITimerCallback)

sbGStreamerMetadataHandler::sbGStreamerMetadataHandler()
  : mLock(nsnull),
    mPipeline(nsnull),
    mTags(nsnull),
    mHasAudio(PR_FALSE),
    mHasVideo(PR_FALSE),
    mCompleted(PR_FALSE)
{
  /* member initializers and constructor code */
}

sbGStreamerMetadataHandler::~sbGStreamerMetadataHandler()
{
  Close();
  if (mLock) {
    nsAutoLock::DestroyLock(mLock);
    mLock = nsnull;
  }
}

nsresult
sbGStreamerMetadataHandler::Init()
{
  TRACE((__FUNCTION__));
  mLock = nsAutoLock::NewLock("sbGStreamerMetadataHandler::mLock");
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);
  
  // initialize GStreamer
  nsresult rv;
  nsCOMPtr<nsISupports> gstService =
    do_GetService(SBGSTREAMERSERVICE_CONTRACTID , &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* readonly attribute ACString contractID */
NS_IMETHODIMP 
sbGStreamerMetadataHandler::GetContractID(nsACString &aContractID)
{
  aContractID.AssignLiteral(SB_GSTREAMER_METADATA_HANDLER_CONTRACTID);
  return NS_OK;
}

/* attribute sbIMutablePropertyArray props; */
NS_IMETHODIMP
sbGStreamerMetadataHandler::GetProps(sbIMutablePropertyArray * *aProps)
{
  TRACE((__FUNCTION__));
  NS_ENSURE_ARG_POINTER(aProps);
  *aProps = mProperties;
  NS_ENSURE_STATE(*aProps);

  NS_ADDREF(*aProps);
  return NS_OK;
}
NS_IMETHODIMP
sbGStreamerMetadataHandler::SetProps(sbIMutablePropertyArray * aProps)
{
  TRACE((__FUNCTION__));
  // we don't support writing, go away
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* readonly attribute PRBool completed; */
NS_IMETHODIMP
sbGStreamerMetadataHandler::GetCompleted(PRBool *aCompleted)
{
  TRACE((__FUNCTION__));
  NS_ENSURE_ARG_POINTER(aCompleted);
  *aCompleted = mCompleted;
  TRACE(("%s: completed? %s", __FUNCTION__, *aCompleted ? "true" : "false"));
  return NS_OK;
}

/* readonly attribute PRBool requiresMainThread; */
NS_IMETHODIMP
sbGStreamerMetadataHandler::GetRequiresMainThread(PRBool *aRequiresMainThread)
{
  TRACE((__FUNCTION__));
  NS_ENSURE_ARG_POINTER(aRequiresMainThread);
  // This handler MUST run on the main thread, since it can kick off
  // http channels and other not-so-threadsafe async tasks.
  *aRequiresMainThread = PR_TRUE;
  return NS_OK;
}

/* attribute nsIChannel channel; */
NS_IMETHODIMP
sbGStreamerMetadataHandler::GetChannel(nsIChannel * *aChannel)
{
  TRACE((__FUNCTION__));
  NS_ENSURE_ARG_POINTER(aChannel);
  NS_IF_ADDREF(*aChannel = mChannel);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMetadataHandler::SetChannel(nsIChannel * aChannel)
{
  TRACE((__FUNCTION__));
  nsresult rv;
  rv = Close();
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsAutoLock lock(mLock);
  mChannel = aChannel;
  if (mChannel) {
    nsCOMPtr<nsIURI> uri;
    rv = mChannel->GetURI(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = uri->GetSpec(mSpec);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    mSpec.SetIsVoid(PR_TRUE);
  }
  return NS_OK;
}

static PRBool
HasExtensionInEnumerator(const nsAString& aHaystack, nsIStringEnumerator *aEnum)
{
  TRACE((__FUNCTION__));
  nsresult rv;

  while (PR_TRUE) {
    PRBool hasMore;
    rv = aEnum->HasMore(&hasMore);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
    if (!hasMore) {
      break;
    }
    
    nsString string;
    rv = aEnum->GetNext(string);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
    
    string.Insert('.', 0);
    if (aHaystack.Find(string, CaseInsensitiveCompare) >= 0) {
      return PR_TRUE;
    }
  }
  
  return PR_FALSE;
}

/* PRInt32 vote (in AString aUrl); */
NS_IMETHODIMP
sbGStreamerMetadataHandler::Vote(const nsAString & aUrl, PRInt32 *_retval)
{
  TRACE((__FUNCTION__));
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = -1;
  
  nsCString url = NS_ConvertUTF16toUTF8(aUrl); // we deal with 8-bit strings
  
  // check for a working protocol (scheme)
  PRInt32 colonPos = url.Find(":");
  NS_ENSURE_TRUE(colonPos >= 0, NS_OK);
  nsCString proto = PromiseFlatCString(StringHead(url, colonPos));
  gboolean protoSupported =
    gst_uri_protocol_is_supported(GST_URI_SRC, proto.get());
  if (!protoSupported) {
    TRACE(("%s: protocol %s not supported", __FUNCTION__, proto.get()));
    return NS_OK;
  }
  
  // at this point, we _possibly_ support it, but not as sure as we like
  // let's guess at file extensions!  Oh wait, no, we have to guess substrings!
  *_retval = 1;
  nsresult rv;
  
  { /* for scope */
    nsAutoLock lock(mLock);
    if (!mFactory) {
      mFactory = do_GetService(SB_GSTREAMERMEDIACOREFACTORY_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  
  nsCOMPtr<sbIMediacoreCapabilities> caps;
  rv = mFactory->GetCapabilities(getter_AddRefs(caps));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIStringEnumerator> strings;
  PRBool found = PR_FALSE;
  rv = caps->GetAudioExtensions(getter_AddRefs(strings));
  if (NS_SUCCEEDED(rv) && strings) {
    found |= HasExtensionInEnumerator(aUrl, strings);
  }
  rv = caps->GetVideoExtensions(getter_AddRefs(strings));
  if (NS_SUCCEEDED(rv) && strings) {
    found |= HasExtensionInEnumerator(aUrl, strings);
  }
  rv = caps->GetImageExtensions(getter_AddRefs(strings));
  if (NS_SUCCEEDED(rv) && strings) {
    found |= HasExtensionInEnumerator(aUrl, strings);
  }
  if (!found) {
    TRACE(("%s: failed to match any supported extensions", __FUNCTION__));
    return NS_OK;
  }
  
  // okay, we probably support this.  But quite possibly not as well as, say,
  // TagLib (which at this point does magical charset detection).
  *_retval = 80;
  return NS_OK;
}

/* PRInt32 read (); */
NS_IMETHODIMP
sbGStreamerMetadataHandler::Read(PRInt32 *_retval)
{
  TRACE((__FUNCTION__));
  NS_ENSURE_ARG_POINTER(_retval);
  
  nsresult rv;
  
  rv = Close();
  NS_ENSURE_SUCCESS(rv, rv);
  
  sbGstElement decodeBin, pipeline;
  sbGstBus bus;
  GstStateChangeReturn stateReturn;

  { /* scope */
    nsAutoLock lock(mLock);
    mProperties = nsnull;
    
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
   
    nsCOMPtr<nsITimerCallback> timerCallback =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsITimerCallback*, this));
    rv = mTimer->InitWithCallback(timerCallback,
                                  METADATA_TIMEOUT,
                                  nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    rv = NS_ERROR_OUT_OF_MEMORY;
    NS_ASSERTION(!mPipeline, "pipeline already exists!");
    if (mPipeline) {
      // should never land here
      gst_object_unref(mPipeline);
      mPipeline = NULL;
    }
    pipeline = gst_pipeline_new("metadata-pipeline");
    //// no longer protect anything below
  }
  NS_ENSURE_TRUE(pipeline, NS_ERROR_OUT_OF_MEMORY);
  
  decodeBin = gst_element_factory_make("uridecodebin", "metadata-decodebin");
  NS_ENSURE_TRUE(decodeBin, NS_ERROR_OUT_OF_MEMORY);
  gst_bin_add(GST_BIN_CAST(pipeline.get()), decodeBin.get());
  gst_object_ref(decodeBin.get()); // the ref was taken by the bin
  
  bus = gst_pipeline_get_bus(GST_PIPELINE_CAST(pipeline.get()));
  NS_ENSURE_TRUE(bus, NS_ERROR_FAILURE);
  
  g_signal_connect(decodeBin.get(), "pad-added", G_CALLBACK(on_pad_added), this);

  // We want to receive state-changed messages when shutting down, so we
  // need to turn off bus auto-flushing
  g_object_set(pipeline.get(), "auto-flush-bus", FALSE, NULL);

  // Handle GStreamer messages synchronously, either directly or
  // dispatching to the main thread.
  gst_bus_set_sync_handler(bus.get(), SyncToAsyncDispatcher,
                           static_cast<sbGStreamerMessageHandler*>(this), NULL);
  
  TRACE(("%s: Setting URI to [%s]", __FUNCTION__, mSpec.get()));
  g_object_set(G_OBJECT(decodeBin.get()), "uri", mSpec.get(), NULL);
  
  stateReturn = gst_element_set_state(pipeline.get(), GST_STATE_PAUSED);
  NS_ENSURE_TRUE(stateReturn == GST_STATE_CHANGE_SUCCESS ||
                 stateReturn == GST_STATE_CHANGE_ASYNC,
                 NS_ERROR_FAILURE);
  
  *_retval = -1; // asynchronous
  { /* scope */
    nsAutoLock lock(mLock);
    mPipeline = pipeline.forget();
  }
  
  return NS_OK;
}

/* PRInt32 write (); */
NS_IMETHODIMP
sbGStreamerMetadataHandler::Write(PRInt32 *_retval)
{
  TRACE((__FUNCTION__));
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void getImageData (in PRInt32 aType, out AUTF8String aMimeType,
                      out unsigned long aDataLen,
                      [array, size_is (aDataLen), retval] out octet aData); */
NS_IMETHODIMP
sbGStreamerMetadataHandler::GetImageData(PRInt32 aType, nsACString & aMimeType,
                                         PRUint32 *aDataLen, PRUint8 **aData)
{
  TRACE((__FUNCTION__));
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void setImageData (in PRInt32 aType, in AString aUrl); */
NS_IMETHODIMP
sbGStreamerMetadataHandler::SetImageData(PRInt32 aType, const nsAString & aUrl)
{
  TRACE((__FUNCTION__));
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void onChannelData (in nsISupports aChannel); */
NS_IMETHODIMP
sbGStreamerMetadataHandler::OnChannelData(nsISupports *aChannel)
{
  TRACE((__FUNCTION__));
  /* this is never used, don't bother implementing, ever */
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void close (); */
NS_IMETHODIMP
sbGStreamerMetadataHandler::Close()
{
  TRACE((__FUNCTION__));
  GstElement *pipeline = NULL;
  
  { /* scope */
    nsAutoLock lock(mLock);
    mCompleted = PR_FALSE;
    pipeline = mPipeline;
    if (pipeline) {
      gst_object_ref(pipeline);
    }
    if (mTimer) {
      nsresult rv = mTimer->Cancel();
      if (NS_FAILED(rv)) {
        NS_WARNING("Failed to cancel metadata timeout timer");
        /* fail silently */
      }
      mTimer = nsnull;
    }
  }
  if (pipeline) {
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
  }
  { /* scope */
    nsAutoLock lock(mLock);
    if (mPipeline) {
      gst_object_unref(mPipeline);
    }
    mPipeline = NULL;

    if (mTags) {
      NS_ASSERTION(GST_IS_TAG_LIST(mTags), "tags is not a tag list!?");
      gst_tag_list_free (mTags);
      mTags = nsnull;
    }
  }
  
  nsresult rv;
  nsCOMPtr<nsIThread> mainThread;
  rv = NS_GetMainThread(getter_AddRefs(mainThread));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIEventTarget> mainTarget = do_QueryInterface(mainThread, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // the channel is not threadsafe, release it on the main thread instead
  nsIChannel* channel = nsnull;
  { /* scope */
    nsAutoLock lock(mLock);
    mChannel.forget(&channel);
  }
  if (channel) {
    rv = NS_ProxyRelease(mainTarget, channel);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  TRACE(("%s: successfully closed", __FUNCTION__));

  return NS_OK;
}

/* sbGStreamerMessageHandler */
void
sbGStreamerMetadataHandler::HandleMessage(GstMessage *message)
{
  TRACE(("%s: received message %s", __FUNCTION__,
         message ? GST_MESSAGE_TYPE_NAME(message) : "(null)"));
  nsresult rv;
  NS_ENSURE_TRUE(message, /* void */);
  switch (GST_MESSAGE_TYPE(message)) {
    case GST_MESSAGE_TAG:
      HandleTagMessage(message);
      break;
    case GST_MESSAGE_STATE_CHANGED: {
      nsAutoLock lock(mLock);
      if (!mPipeline || mCompleted) {
        // umm, we stopped, don't bother
        return;
      }
      GstObject *src = GST_MESSAGE_SRC(message);
      if (src == GST_OBJECT_CAST(mPipeline)) {
        GstState oldState, newState, pendingState;
        gst_message_parse_state_changed(message, &oldState, &newState,
                                        &pendingState);
        TRACE(("%s: state %s -> %s (-> %s)", __FUNCTION__,
               gst_element_state_get_name(oldState),
               gst_element_state_get_name(newState),
               gst_element_state_get_name(pendingState)));
        if (newState == GST_STATE_PAUSED) {
          TRACE(("%s: Successfully paused", __FUNCTION__));
          rv = FinalizeTags(PR_TRUE);
          // don't hold the lock around Close()
          nsresult rv2;
          {
            nsAutoUnlock unlock(mLock);
            rv2 = Close();
          }
          mCompleted = PR_TRUE;
          NS_ENSURE_SUCCESS(rv, /* void */);
          NS_ENSURE_SUCCESS(rv2, /* void */);
        }
      }
      break;
    }
    case GST_MESSAGE_ELEMENT: {
      if (gst_is_missing_plugin_message(message)) {
        /* If we got a missing plugin message about a missing video decoder,
           we should still mark it as a video file */
        const gchar *type = gst_structure_get_string(gst_message_get_structure(message), "type");
        if (type && !strcmp(type, "decoder")) {
          /* Missing decoder: see if it's video */
          const GValue *val = gst_structure_get_value(gst_message_get_structure(message), "detail");
          const GstCaps *caps = gst_value_get_caps (val);
          GstStructure *structure = gst_caps_get_structure (caps, 0);
          const gchar *capsname = gst_structure_get_name (structure);

          if (g_str_has_prefix(capsname, "video/")) {
            mHasVideo = PR_TRUE;
          }
        }
      }
      break;
    }
    case GST_MESSAGE_ERROR: {
      GError *gerror = NULL;
      gchar *debugMessage = NULL;
      gst_message_parse_error(message, &gerror, &debugMessage);
      LOG(("%s: GStreamer error: %s / %s", __FUNCTION__, gerror->message, debugMessage));
      g_error_free (gerror);
      g_free (debugMessage);

      /* If we got an error from a video decoder, then it has undecodeable
         video - but we should still mark it as a video file! */
      if (GST_IS_ELEMENT (message->src)) {
        GstElementClass *elementclass = GST_ELEMENT_CLASS (
            G_OBJECT_GET_CLASS (message->src));
        GstElementFactory *factory = elementclass->elementfactory;

        if (gst_element_factory_get_metadata(factory, "Video") &&
            gst_element_factory_get_metadata(factory, "Decoder"))
        {
          mHasVideo = PR_TRUE;
        }
      }

      nsAutoLock lock(mLock);
      if (!mCompleted) {
        rv = FinalizeTags(PR_FALSE);
        // don't hold the lock around Close()
        nsresult rv2;
        {
          nsAutoUnlock unlock(mLock);
          rv2 = Close();
        }
        mCompleted = PR_TRUE;
        NS_ENSURE_SUCCESS(rv, /* void */);
        NS_ENSURE_SUCCESS(rv2, /* void */);
      }
      break;
    }
    default:
      break;
  }
}

PRBool
sbGStreamerMetadataHandler::HandleSynchronousMessage(GstMessage *message)
{
  /* we never want to handle any messages synchronously */
  return PR_FALSE;
}

void
sbGStreamerMetadataHandler::on_pad_added(GstElement *decodeBin,
                                         GstPad *newPad,
                                         sbGStreamerMetadataHandler *self)
{
  TRACE((__FUNCTION__));
  
  sbGstElement queue, sink, pipeline;
  sbGstPad queueSink, oldPad, pad;
  
  NS_ENSURE_TRUE(self, /* void */); // can't use goto before mon is initialized
  { /* scope */
    nsAutoLock lock(self->mLock);
    if (self->mCompleted) {
      NS_WARNING("pad added after completion (or abort)");
      return;
    }
    NS_ENSURE_TRUE(self->mPipeline, /* void */);
    pipeline = GST_ELEMENT(gst_object_ref(self->mPipeline));
  }
  NS_ENSURE_TRUE(pipeline, /* void */);
  
  // attach a queue and a fakesink to the pad
  queue = gst_element_factory_make("queue", NULL);
  NS_ENSURE_TRUE(queue, /* void */);

  sink = gst_element_factory_make("fakesink", NULL);
  NS_ENSURE_TRUE(sink, /* void */);

  // we want to keep references after we sink these in the sbin
  gst_object_ref(queue.get());
  gst_object_ref(sink.get());
  gst_bin_add_many(GST_BIN_CAST(pipeline.get()), queue.get(), sink.get(), NULL);

  gst_element_set_state(queue.get(), GST_STATE_PAUSED);
  gst_element_set_state(sink.get(), GST_STATE_PAUSED);
  
  queueSink = gst_element_get_static_pad(queue.get(), "sink");
  NS_ENSURE_TRUE(queueSink, /* void */);
  
  GstPadLinkReturn linkResult;
  linkResult = gst_pad_link(newPad, queueSink.get());
  NS_ENSURE_TRUE(GST_PAD_LINK_OK == linkResult, /* void */);
  
  gboolean succeeded;
  succeeded = gst_element_link_pads(queue.get(), "src", sink.get(), "sink");
  NS_ENSURE_TRUE(succeeded, /* void */);

  // due to gnome bug 564863, when the new pad is a ghost pad we never get the
  // notify::caps signal, and therefore never figure out the caps.  Get the
  // underlying pad (recusively) to work around this.  Fixed in GStreamer
  // 0.10.22.
  pad = (GstPad*)gst_object_ref(newPad);
  while (GST_IS_GHOST_PAD(pad.get())) {
    oldPad = pad.forget();
    pad = gst_ghost_pad_get_target(GST_GHOST_PAD(oldPad.get()));
  }

  // manually check for fixed (already negotiated) caps; it will harmlessly
  // return early if it's not already negotiated
  on_pad_caps_changed(pad.get(), NULL, self);

  g_signal_connect(pad.get(), "notify::caps", G_CALLBACK(on_pad_caps_changed), self);
}

static void
AddIntPropFromCaps(GstStructure *aStruct, const gchar *aFieldName,
                   const char* aPropName, sbIMutablePropertyArray *aProps)
{
  gint value;
  gboolean success = gst_structure_get_int(aStruct, aFieldName, &value);
  if (success) {
    nsString stringValue;
    stringValue.AppendInt(value);
    nsresult rv = aProps->AppendProperty(NS_ConvertASCIItoUTF16(aPropName),
                                         stringValue);
    NS_ENSURE_SUCCESS(rv, /* void */);
    TRACE(("%s: %s = %s", __FUNCTION__, aPropName,
           NS_LossyConvertUTF16toASCII(stringValue).get()));
  }
}

void
sbGStreamerMetadataHandler::on_pad_caps_changed(GstPad *pad,
                                                GParamSpec *pspec,
                                                sbGStreamerMetadataHandler *self)
{
  TRACE((__FUNCTION__));
  nsAutoLock lock(self->mLock);
  if (self->mCompleted) {
    NS_WARNING("caps changed after completion (or abort)");
    return;
  }
  GstStructure* capStruct = NULL;
  sbGstCaps caps = gst_pad_get_current_caps(pad);
  if (!caps) {
    // no negotiated caps yet, keep waiting
    TRACE(("%s: no caps yet", __FUNCTION__));
    return;
  }

  char *capsString = gst_caps_to_string(caps.get());
  const gchar *capName;
  TRACE(("%s: caps are %s", __FUNCTION__, capsString));
  if (capsString) {
    g_free(capsString);
  }
  
  NS_ENSURE_TRUE(gst_caps_get_size(caps.get()) > 0, /* void */);
  capStruct = gst_caps_get_structure(caps.get(), 0);
  NS_ENSURE_TRUE(capStruct, /* void */);
  if (!self->mProperties) {
    nsresult rv;
    self->mProperties = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, 
            &rv);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }
  NS_ENSURE_TRUE(self->mProperties, /* void */);

  capName = gst_structure_get_name(capStruct);
  if (g_str_has_prefix(capName, "audio/")) {
    AddIntPropFromCaps(capStruct, "channels",
                       SB_PROPERTY_CHANNELS, self->mProperties);
    AddIntPropFromCaps(capStruct, "rate",
                       SB_PROPERTY_SAMPLERATE, self->mProperties);
    self->mHasAudio = PR_TRUE;
  } else if (g_str_has_prefix(capName, "video/")) {
    self->mHasVideo = PR_TRUE;
  }
}

void
sbGStreamerMetadataHandler::HandleTagMessage(GstMessage *message)
{
  TRACE((__FUNCTION__));
  GstTagList *tag_list = NULL;

  nsAutoLock lock(mLock);
  if (mCompleted) {
    NS_WARNING("tag message after completion (or abort)");
    return;
  }

  gst_message_parse_tag(message, &tag_list);

  if (mTags) {
    GstTagList *newTags = gst_tag_list_merge (mTags, tag_list,
            GST_TAG_MERGE_REPLACE);
    gst_tag_list_free (mTags);
    mTags = newTags;
  }
  else {
    mTags = gst_tag_list_copy (tag_list);
  }
  gst_tag_list_free(tag_list);
}

nsresult
sbGStreamerMetadataHandler::FinalizeTags(PRBool aSucceeded)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;
  
  // the caller is responsible for holding the lock :(

  if (!mProperties) {
    mProperties = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aSucceeded && mTags) {
    nsCOMPtr<sbIPropertyArray> propArray;
    rv = ConvertTagListToPropertyArray(mTags, getter_AddRefs(propArray));
    NS_ENSURE_SUCCESS(rv, rv );
    
    // copy the tags over, don't clobber the old data
    PRUint32 newLength;
    rv = propArray->GetLength(&newLength);
    NS_ENSURE_SUCCESS(rv, rv);
    
    for (PRUint32 i = 0 ; i < newLength; ++i) {
      nsCOMPtr<sbIProperty> prop;
      rv = propArray->GetPropertyAt(i, getter_AddRefs(prop));
      NS_ENSURE_SUCCESS(rv, rv);
      
      nsString id, value;
      rv = prop->GetId(id);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = prop->GetValue(value);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = mProperties->AppendProperty(id, value);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsString contentType;
  if (mHasVideo) {
    TRACE(("%s[%p]: file %s is video",
           __FUNCTION__, this, mSpec.get()));
    contentType = NS_LITERAL_STRING("video");
  }
  else if (mHasAudio) {
    TRACE(("%s[%p]: file %s is audio",
           __FUNCTION__, this, mSpec.get()));
    contentType = NS_LITERAL_STRING("audio");
  }
  else {
    // we didn't find either video or audio; fall back to file extension based
    // detection of whether this file was likely to have contained some sort of
    // video.
    nsCOMPtr<sbIMediacoreCapabilities> caps;
    rv = mFactory->GetCapabilities(getter_AddRefs(caps));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIStringEnumerator> strings;
    PRBool found = PR_FALSE;
    rv = caps->GetVideoExtensions(getter_AddRefs(strings));
    if (NS_SUCCEEDED(rv) && strings) {
      found = HasExtensionInEnumerator(NS_ConvertUTF8toUTF16(mSpec), strings);
    }
    if (found) {
      TRACE(("%s[%p]: file %s found via fallback to be video",
             __FUNCTION__, this, mSpec.get()));
      // this is a file with a video extension
      contentType = NS_LITERAL_STRING("video");
    }
    else {
      TRACE(("%s[%p]: file %s found via fallback to be not video",
             __FUNCTION__, this, mSpec.get()));
    }
  }

  if (!contentType.IsEmpty()) {
    rv = mProperties->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                                     contentType);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMetadataHandler::Notify(nsITimer* aTimer)
{
  TRACE((__FUNCTION__));
  
  nsresult rv;
  
  // hold a reference to the timer, since Close() releases it
  nsCOMPtr<sbIMetadataHandler> kungFuDeathGrip(this);
  
  rv = Close();
  nsAutoLock lock(mLock);
  mCompleted = PR_TRUE;
  mProperties = nsnull;
  mHasAudio = PR_FALSE;
  mHasVideo = PR_FALSE;
  NS_ENSURE_SUCCESS(rv, rv);
 
  return NS_OK;
}
