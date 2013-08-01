/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbGStreamerMediaInspector.h"

#include <sbClassInfoUtils.h>
#include <sbStringBundle.h>
#include <sbTArrayStringEnumerator.h>
#include <sbStringUtils.h>
#include <sbStandardProperties.h>
#include <sbProxiedComponentManager.h>

#include <sbIGStreamerService.h>
#include <sbIMediaItem.h>
#include <sbIMediaInspector.h>
#include <sbIMediaFormatMutable.h>

#include <nsIWritablePropertyBag2.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <prlog.h>

/* Timeout for an inspectMedia() call, in ms */
#define GST_MEDIA_INSPECTOR_TIMEOUT (2000)

#define GST_TYPE_PROPERTY   "gst_type"

/**
 * To log this class, set the following environment variable in a debug build:
 *  NSPR_LOG_MODULES=sbGStreamerMediaInspector:5 (or :3 for LOG messages only)
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gGStreamerMediaInspector = PR_NewLogModule("sbGStreamerMediaInspector");
#define LOG(args) PR_LOG(gGStreamerMediaInspector, PR_LOG_WARNING, args)
#define TRACE(args) PR_LOG(gGStreamerMediaInspector, PR_LOG_DEBUG, args)
#else /* PR_LOGGING */
#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */
#endif /* PR_LOGGING */

NS_IMPL_THREADSAFE_ISUPPORTS7(sbGStreamerMediaInspector,
                              nsIClassInfo,
                              sbIGStreamerPipeline,
                              sbIMediacoreEventTarget,
                              sbIMediaInspector,
                              sbIJobProgress,
                              sbIJobCancelable,
                              nsITimerCallback)

NS_IMPL_CI_INTERFACE_GETTER4(sbGStreamerMediaInspector,
                             sbIGStreamerPipeline,
                             sbIMediaInspector,
                             sbIJobProgress,
                             sbIJobCancelable)

NS_DECL_CLASSINFO(sbGstreamerMediaInspector);
NS_IMPL_THREADSAFE_CI(sbGStreamerMediaInspector);

sbGStreamerMediaInspector::sbGStreamerMediaInspector() :
    sbGStreamerPipeline(),
    mStatus(sbIJobProgress::STATUS_RUNNING),
    mFinished(PR_FALSE),
    mIsPaused(PR_FALSE),
    mTooComplexForCurrentImplementation(PR_FALSE),
    mDecodeBin(NULL),
    mVideoSrc(NULL),
    mAudioSrc(NULL),
    mAudioDecoderSink(NULL),
    mVideoDecoderSink(NULL),
    mDemuxerSink(NULL),
    mAudioBitRate(0),
    mVideoBitRate(0)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
}

sbGStreamerMediaInspector::~sbGStreamerMediaInspector()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  /* destructor code */
}

/* sbIJobCancelable interface implementation */

NS_IMETHODIMP
sbGStreamerMediaInspector::GetCanCancel(PRBool *aCanCancel)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aCanCancel);

  *aCanCancel = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::Cancel()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  mStatus = sbIJobProgress::STATUS_FAILED; // We don't have a 'cancelled' state.

  nsresult rv = StopPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

/* sbIJobProgress interface implementation */

NS_IMETHODIMP
sbGStreamerMediaInspector::GetStatus(PRUint16 *aStatus)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aStatus);

  *aStatus = mStatus;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::GetBlocked(PRBool *aBlocked)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aBlocked);

  *aBlocked = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::GetStatusText(nsAString& aText)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv = NS_ERROR_FAILURE;

  switch (mStatus) {
    case sbIJobProgress::STATUS_FAILED:
      rv = SBGetLocalizedString(aText,
              NS_LITERAL_STRING("mediacore.gstreamer.inspect.failed"));
      break;
    case sbIJobProgress::STATUS_SUCCEEDED:
      rv = SBGetLocalizedString(aText,
              NS_LITERAL_STRING("mediacore.gstreamer.inspect.succeeded"));
      break;
    case sbIJobProgress::STATUS_RUNNING:
      rv = SBGetLocalizedString(aText,
              NS_LITERAL_STRING("mediacore.gstreamer.inspect.running"));
      break;
    default:
      NS_NOTREACHED("Status is invalid");
  }

  return rv;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::GetTitleText(nsAString& aText)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  return SBGetLocalizedString(aText,
          NS_LITERAL_STRING("mediacore.gstreamer.inspect.title"));
}

NS_IMETHODIMP
sbGStreamerMediaInspector::GetProgress(PRUint32* aProgress)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aProgress);

  *aProgress = 0; // Unknown
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::GetTotal(PRUint32* aTotal)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aTotal);

  *aTotal = 0;
  return NS_OK;
}

// Note that you can also get errors reported via the mediacore listener
// interfaces.
NS_IMETHODIMP
sbGStreamerMediaInspector::GetErrorCount(PRUint32* aErrorCount)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aErrorCount);
  NS_ASSERTION(NS_IsMainThread(),
               "sbIJobProgress::GetErrorCount is main thread only!");

  *aErrorCount = mErrorMessages.Length();

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::GetErrorMessages(nsIStringEnumerator** aMessages)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMessages);
  NS_ASSERTION(NS_IsMainThread(),
    "sbIJobProgress::GetProgress is main thread only!");

  *aMessages = nsnull;

  nsCOMPtr<nsIStringEnumerator> enumerator =
    new sbTArrayStringEnumerator(&mErrorMessages);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  enumerator.forget(aMessages);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(),
    "sbGStreamerMediaInspector::AddJobProgressListener is main thread only!");

  PRInt32 index = mProgressListeners.IndexOf(aListener);
  if (index >= 0) {
    // the listener already exists, do not re-add
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }
  PRBool succeeded = mProgressListeners.AppendObject(aListener);
  NS_ENSURE_TRUE(succeeded, NS_ERROR_FAILURE);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::RemoveJobProgressListener(
        sbIJobProgressListener* aListener)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(),
    "sbGStreamerMediaInspector::RemoveJobProgressListener is main thread only!");

  PRInt32 indexToRemove = mProgressListeners.IndexOf(aListener);
  if (indexToRemove < 0) {
    // No such listener, don't try to remove. This is OK.
    return NS_OK;
  }

  // remove the listener
  PRBool succeeded = mProgressListeners.RemoveObjectAt(indexToRemove);
  NS_ENSURE_TRUE(succeeded, NS_ERROR_FAILURE);

  return NS_OK;
}

// Call all job progress listeners
nsresult
sbGStreamerMediaInspector::OnJobProgress()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ASSERTION(NS_IsMainThread(),
    "sbGStreamerMediaInspector::OnJobProgress is main thread only!");

  // Announce our status to the world
  for (PRInt32 i = mProgressListeners.Count() - 1; i >= 0; --i) {
     // Ignore any errors from listeners
     mProgressListeners[i]->OnJobProgress(this);
  }
  return NS_OK;
}

void
sbGStreamerMediaInspector::HandleErrorMessage(GstMessage *message)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  GError *gerror = NULL;
  gchar *debug = NULL;

  mStatus = sbIJobProgress::STATUS_FAILED;

  gst_message_parse_error(message, &gerror, &debug);

  mErrorMessages.AppendElement(
      NS_ConvertUTF8toUTF16(nsDependentCString(gerror->message)));

  g_error_free (gerror);
  g_free(debug);

  nsresult rv = CompleteInspection();
  NS_ENSURE_SUCCESS (rv, /* void */);

  // This will stop the pipeline and update listeners
  sbGStreamerPipeline::HandleErrorMessage (message);
}

/* sbIGStreamerMediaInspector interface implementation */

NS_IMETHODIMP
sbGStreamerMediaInspector::GetMediaFormat(sbIMediaFormat **_retval)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER (_retval);

  if (mMediaFormat) {
    nsresult rv = CallQueryInterface(mMediaFormat.get(), _retval);
    NS_ENSURE_SUCCESS (rv, rv);
  }
  else {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::InspectMediaURI(const nsAString & aURI,
                                        sbIMediaFormat **_retval)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER (_retval);

  nsresult rv = NS_ERROR_UNEXPECTED;
  PRBool processed = PR_FALSE;
  PRBool isMainThread = NS_IsMainThread();

  nsCOMPtr<nsIThread> target;
  if (isMainThread) {
    rv = NS_GetMainThread(getter_AddRefs(target));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(!isMainThread,
               "Synchronous InspectMedia is background-thread only");

  // Reset internal state tracking.
  ResetStatus();

  rv = InspectMediaURIAsync (aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  while (PR_AtomicAdd (&mFinished, 0) == 0)
  {
    // This isn't something that's considered safe to do but it does
    // the job right now. Ideally one would always want to use the
    // async version that's usable on a background thread.
    //
    // The sync version is really meant to be used on a background thread!
    // However, the Base Device currently needs this method to be synchronous
    // and on the main thread. This will change in the future.
    if (isMainThread && target) {
      rv = target->ProcessNextEvent(PR_FALSE, &processed);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // This is an ugly hack, but works well enough.
    PR_Sleep (50);
  }

  if (mIsPaused && mMediaFormat) {
    rv = CallQueryInterface(mMediaFormat.get(), _retval);
    NS_ENSURE_SUCCESS (rv, rv);

    return NS_OK;
  }
  else {
    // Otherwise we timed out or errored out; return an error since we were
    // unable to determine anything about this media item.
    return NS_ERROR_NOT_AVAILABLE;
  }
}

NS_IMETHODIMP
sbGStreamerMediaInspector::InspectMedia(sbIMediaItem *aMediaItem,
                                        sbIMediaFormat **_retval)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER (aMediaItem);
  NS_ENSURE_ARG_POINTER (_retval);

  // Get the content location from the media item. (we don't use GetContentSrc
  // because we want the string form, not a URI, and URI objects aren't
  // threadsafe)
  nsString sourceURI;
  nsresult rv = aMediaItem->GetProperty(
          NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL), sourceURI);
  NS_ENSURE_SUCCESS (rv, rv);

  // let InspectMediaURI do the work
  return InspectMediaURI(sourceURI, _retval);
}

NS_IMETHODIMP
sbGStreamerMediaInspector::InspectMediaURIAsync(const nsAString & aURI)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  mSourceURI = aURI;
  // Reset internal state tracking.
  ResetStatus();

  nsresult rv = StartTimeoutTimer();
  NS_ENSURE_SUCCESS (rv, rv);

  // Set the pipeline to PAUSED. This will allow the pipeline to preroll,
  // at which point we can inspect the caps on the pads, and look at the events
  // that we have collected.
  rv = PausePipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediaInspector::InspectMediaAsync(sbIMediaItem *aMediaItem)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER (aMediaItem);

  // Get the content location from the media item. (we don't use GetContentSrc
  // because we want the string form, not a URI, and URI objects aren't
  // threadsafe)
  nsString sourceURI;
  nsresult rv = aMediaItem->GetProperty(
          NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL), sourceURI);
  NS_ENSURE_SUCCESS (rv, rv);

  // let InspectMediaURIAsync do the work
  return InspectMediaURIAsync(sourceURI);
}

nsresult
sbGStreamerMediaInspector::CompleteInspection()
{
  nsresult rv = StopTimeoutTimer();
  NS_ENSURE_SUCCESS (rv, rv);

  // If we got to paused, inspect what we can from the pipeline
  if (mIsPaused) {
    rv = ProcessPipelineForInfo();
    NS_ENSURE_SUCCESS (rv, rv);

    mStatus = sbIJobProgress::STATUS_SUCCEEDED;
  }
  else {
    // Otherwise we timed out or errored out; return an error since we were
    // unable to determine anything about this media item.

    mStatus = sbIJobProgress::STATUS_FAILED;
    // Null this out since we failed.
    mMediaFormat = NULL;
  }

  mFinished = PR_TRUE;

  rv = StopPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::StartTimeoutTimer()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  // We want the timer to fire on the main thread, so we must create it on
  // the main thread - this does so.
  mTimeoutTimer = do_ProxiedCreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mTimeoutTimer->InitWithCallback(this,
                                  GST_MEDIA_INSPECTOR_TIMEOUT,
                                  nsITimer::TYPE_ONE_SHOT);

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::StopTimeoutTimer()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  if (mTimeoutTimer) {
    mTimeoutTimer->Cancel();
    mTimeoutTimer = nsnull;
  }

  return NS_OK;
}

/* nsITimerCallback interface implementation */

NS_IMETHODIMP
sbGStreamerMediaInspector::Notify(nsITimer *aTimer)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aTimer);

  nsresult rv = CompleteInspection();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}


// Override base class to set the job progress appropriately
NS_IMETHODIMP
sbGStreamerMediaInspector::StopPipeline()
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  nsresult rv;

  rv = sbGStreamerPipeline::StopPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  // Inform listeners of new job status
  rv = OnJobProgress();
  NS_ENSURE_SUCCESS (rv, rv);

  rv = CleanupPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::CleanupPipeline()
{
  // Just clean up all our outstanding references to GStreamer objects.
  if (mDecodeBin) {
    g_object_unref (mDecodeBin);
    mDecodeBin = NULL;
  }
  if (mVideoSrc) {
    g_object_unref (mVideoSrc);
    mVideoSrc = NULL;
  }
  if (mAudioSrc) {
    g_object_unref (mAudioSrc);
    mAudioSrc = NULL;
  }
  if (mAudioDecoderSink) {
    g_object_unref (mAudioDecoderSink);
    mAudioDecoderSink = NULL;
  }
  if (mVideoDecoderSink) {
    g_object_unref (mVideoDecoderSink);
    mVideoDecoderSink = NULL;
  }
  if (mDemuxerSink) {
    g_object_unref (mDemuxerSink);
    mDemuxerSink = NULL;
  }

  return NS_OK;
}

void
sbGStreamerMediaInspector::ResetStatus()
{
  mStatus = sbIJobProgress::STATUS_RUNNING;
  mFinished = PR_FALSE;
  mIsPaused = PR_FALSE;
  mTooComplexForCurrentImplementation = PR_FALSE;
}

nsresult
sbGStreamerMediaInspector::BuildPipeline()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  /* Build pipeline looking roughly like this (though either the audio or video
     sides of this might be missing)
     The audio/video parts are create and connected only after appropriate pads
     are exposed by decodebin2 (if they exist at all), so initially our
     pipeline contains only the source and the decodebin2.

     [-----------------------------------------------------------]
     [media-inspector-pipeline                                   ]
     [                           /[audio-queue]-[audio-fakesink] ]
     [ [----------] [----------]/                                ]
     [ [uri-source]-[decodebin2]\                                ]
     [ [----------] [----------] \[video-queue]-[video-fakesink] ]
     [                                                           ]
     [-----------------------------------------------------------]
   */

  mPipeline = gst_pipeline_new ("media-inspector-pipeline");

  nsCString uri = NS_ConvertUTF16toUTF8 (mSourceURI);
  GstElement *src = gst_element_make_from_uri (GST_URI_SRC,
          uri.BeginReading(), "uri-source");

  if (!src) {
    // TODO: Signal failure somehow with more info?
    return NS_ERROR_FAILURE;
  }

  mDecodeBin = gst_element_factory_make ("decodebin2", NULL);
  // Take ownership of mDecodeBin via ref/sink
  gst_object_ref(mDecodeBin);
  gst_object_ref_sink(mDecodeBin);

  // TODO: Connect up autoplug-sort signal to handle some special cases
//  g_signal_connect (decodebin, "autoplug-sort",
//          G_CALLBACK (decodebin_pad_added_cb), this);
  g_signal_connect (mDecodeBin, "pad-added",
          G_CALLBACK (decodebin_pad_added_cb), this);

  gst_bin_add_many (GST_BIN (mPipeline), src, mDecodeBin, NULL);

  GstPad *srcpad = gst_element_get_pad (src, "src");
  GstPad *sinkpad = gst_element_get_pad (mDecodeBin, "sink");

  gst_pad_link (srcpad, sinkpad);

  g_object_unref (srcpad);
  g_object_unref (sinkpad);

  SetPipelineOp(GStreamer::OP_INSPECTING);

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::PadAdded(GstPad *srcpad)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // At this point, we may not have complete caps. However, we only want
  // to identify audio vs. video vs. other, and the template caps returned by
  // this will be sufficient.
  // We don't look at the caps in detail until we are in the PAUSED state, at
  // which point we explicitly look at the negotiated caps.
  sbGstCaps caps = gst_pad_get_caps (srcpad);
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  const gchar *name = gst_structure_get_name (structure);
  bool isVideo = g_str_has_prefix (name, "video/");
  bool isAudio = g_str_has_prefix (name, "audio/");

  if (isAudio && !mAudioSrc) {
    GstElement *queue = gst_element_factory_make ("queue", "audio-queue");
    GstElement *fakesink = gst_element_factory_make ("fakesink", "audio-sink");

    gst_bin_add_many (GST_BIN (mPipeline), queue, fakesink, NULL);
    gst_element_sync_state_with_parent (queue);
    gst_element_sync_state_with_parent (fakesink);

    GstPad *sinkpad = gst_element_get_pad (queue, "sink");

    gst_pad_link (srcpad, sinkpad);
    g_object_unref (sinkpad);

    gst_element_link (queue, fakesink);

    GstPad *fakesinkpad = gst_element_get_request_pad(fakesink, "sink");
    gst_pad_add_probe(fakesinkpad, GST_PAD_PROBE_TYPE_EVENT_BOTH,
                      GstPadProbeCallback(fakesink_audio_event_cb), this, NULL);

    g_object_unref (fakesinkpad);

    mAudioSrc = GST_PAD (gst_object_ref (srcpad));
  }
  else if (isVideo && !mVideoSrc) {
    GstElement *queue = gst_element_factory_make ("queue", "video-queue");
    GstElement *fakesink = gst_element_factory_make ("fakesink", "video-sink");

    gst_bin_add_many (GST_BIN (mPipeline), queue, fakesink, NULL);
    gst_element_sync_state_with_parent (queue);
    gst_element_sync_state_with_parent (fakesink);

    GstPad *sinkpad = gst_element_get_pad (queue, "sink");

    gst_pad_link (srcpad, sinkpad);
    g_object_unref (sinkpad);

    gst_element_link (queue, fakesink);

    GstPad *fakesinkpad = gst_element_get_static_pad(fakesink, "sink");
    gst_pad_add_probe(fakesinkpad, GST_PAD_PROBE_TYPE_EVENT_BOTH,
                      GstPadProbeCallback(fakesink_video_event_cb), this, NULL);

    g_object_unref (fakesinkpad);

    mVideoSrc = GST_PAD (gst_object_ref (srcpad));
  }
  else {
    // Ignore this one.
  }

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::FakesinkEvent(GstPad *srcpad, GstEvent *event,
                                         PRBool isAudio)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // Bit rate is already available.
  if ((isAudio && mAudioBitRate) || (!isAudio && mVideoBitRate))
    return NS_OK;

  guint bitrate = 0;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_TAG: {
      GstTagList *list = NULL;

      gst_event_parse_tag (event, &list);
      if (list && !gst_tag_list_is_empty (list)) {
        gst_tag_list_get_uint (list, GST_TAG_BITRATE, &bitrate);
        if (!bitrate)
          gst_tag_list_get_uint (list, GST_TAG_NOMINAL_BITRATE, &bitrate);
      }
      break;
    }
    default:
      break;
  }

  if (bitrate) {
    if (isAudio)
      mAudioBitRate = bitrate;
    else
      mVideoBitRate = bitrate;
  }

  return NS_OK;
}

void
sbGStreamerMediaInspector::HandleStateChangeMessage(GstMessage *message)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  sbGStreamerPipeline::HandleStateChangeMessage (message);

  // Wait for the top-level pipeline to reach the PAUSED state, after letting
  // our parent class handle the default stuff.
  if (GST_IS_PIPELINE (message->src))
  {
    GstState oldstate, newstate, pendingstate;
    gst_message_parse_state_changed (message,
            &oldstate, &newstate, &pendingstate);

    if (pendingstate == GST_STATE_VOID_PENDING &&
        newstate == GST_STATE_PAUSED)
    {
      mIsPaused = PR_TRUE;
      nsresult rv = CompleteInspection();
      NS_ENSURE_SUCCESS (rv, /* void */);
    }
  }
}

nsresult
sbGStreamerMediaInspector::ProcessPipelineForInfo()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv = NS_OK;

  // Ok, we've reached PAUSED. That means all our pads will have caps - so, we
  // just need to look around for all the elements inside our decodebin, and
  // figure out what exciting things we can find!
  GstIterator *it = gst_bin_iterate_recurse (GST_BIN (mDecodeBin));
  gboolean done = FALSE;

  while (!done) {
    GValue element;

    switch (gst_iterator_next (it, (GValue*) &element)) {
      case GST_ITERATOR_OK:
        rv = InspectorateElement (GST_ELEMENT (element));
        gst_object_unref (element);
        if (NS_FAILED (rv)) {
          done = TRUE;
        }
        break;
      case GST_ITERATOR_DONE:
        done = TRUE;
        break;
      case GST_ITERATOR_RESYNC:
        gst_iterator_resync (it);
        break;
      case GST_ITERATOR_ERROR:
        done = TRUE;
        rv = NS_ERROR_FAILURE;
        break;
    }
  }

  gst_iterator_free (it);

  NS_ENSURE_SUCCESS (rv, rv);

  if (mAudioSrc) {
    GstPad *audioSrcPad = GetRealPad (mAudioSrc);
    GstElement *audioDecoder = GST_ELEMENT (gst_pad_get_parent (audioSrcPad));
    GstElementFactory *factory = gst_element_get_factory (audioDecoder);
    const gchar *klass = gst_element_factory_get_klass (factory);

    if (strstr (klass, "Decoder")) {
      // Ok, it really is a decoder! Grab the sink pad to poke at in a bit.
      mAudioDecoderSink = gst_element_get_pad (audioDecoder, "sink");
    }

    g_object_unref (audioSrcPad);
    g_object_unref (audioDecoder);
  }

  if (mVideoSrc) {
    GstPad *videoSrcPad = GetRealPad (mVideoSrc);
    GstElement *videoDecoder = GST_ELEMENT (gst_pad_get_parent (videoSrcPad));
    GstElementFactory *factory = gst_element_get_factory (videoDecoder);
    const gchar *klass = gst_element_factory_get_klass (factory);

    if (strstr (klass, "Decoder")) {
      // Ok, it really is a decoder! Grab the sink pad to poke at in a bit.
      mVideoDecoderSink = gst_element_get_pad (videoDecoder, "sink");
    }

    g_object_unref (videoSrcPad);
    g_object_unref (videoDecoder);
  }

  nsCOMPtr<sbIMediaFormatAudio> audioFormat;
  nsCOMPtr<sbIMediaFormatVideo> videoFormat;
  nsCOMPtr<sbIMediaFormatContainerMutable> containerFormat;

  if (mTooComplexForCurrentImplementation) {
    // File info appears to be too complex to be understood/represented by
    // the current implementation. Signal that with a special container type.
    containerFormat = do_CreateInstance(SB_MEDIAFORMATCONTAINER_CONTRACTID,
                                        &rv);
    NS_ENSURE_SUCCESS (rv, rv);

    containerFormat->SetContainerType(NS_LITERAL_STRING("video/x-too-complex"));
  }
  else if (mDemuxerSink) {
    // Ok, we have a demuxer - the container format should be described by
    // the caps on this pad.
    containerFormat = do_CreateInstance(SB_MEDIAFORMATCONTAINER_CONTRACTID,
                                        &rv);
    NS_ENSURE_SUCCESS (rv, rv);

    sbGstCaps caps = gst_pad_get_current_caps (mDemuxerSink);
    GstStructure *structure = gst_caps_get_structure (caps, 0);

    nsCString mimeType;
    rv = GetMimeTypeForCaps (caps, mimeType);
    NS_ENSURE_SUCCESS (rv, rv);

    rv = containerFormat->SetContainerType (NS_ConvertUTF8toUTF16(mimeType));
    NS_ENSURE_SUCCESS (rv, rv);

    // format-specific attributes.
    rv = ProcessContainerProperties(containerFormat, structure);
    NS_ENSURE_SUCCESS (rv, rv);
  }

  if (mVideoSrc) {
    rv = ProcessVideo(getter_AddRefs(videoFormat));
    NS_ENSURE_SUCCESS (rv, rv);
  }

  if (mAudioSrc) {
    rv = ProcessAudio(getter_AddRefs(audioFormat));
    NS_ENSURE_SUCCESS (rv, rv);
  }

  mMediaFormat = do_CreateInstance(SB_MEDIAFORMAT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = mMediaFormat->SetContainer (containerFormat);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = mMediaFormat->SetAudioStream (audioFormat);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = mMediaFormat->SetVideoStream (videoFormat);

  return rv;
}

nsresult
sbGStreamerMediaInspector::ProcessVideoCaps (sbIMediaFormatVideoMutable *format,
                                             GstCaps *caps)
{
  nsresult rv;
  GstStructure *structure = gst_caps_get_structure (caps, 0);

  gint width, height;
  if (gst_structure_get_int (structure, "width", &width) &&
      gst_structure_get_int (structure, "height", &height))
  {
    rv = format->SetVideoWidth (width);
    NS_ENSURE_SUCCESS (rv, rv);
    rv = format->SetVideoHeight (height);
    NS_ENSURE_SUCCESS (rv, rv);
  }

  const GValue* framerate = gst_structure_get_value(structure, "framerate");
  gint framerateN, framerateD;
  if (framerate) {
    framerateN = gst_value_get_fraction_numerator(framerate);
    framerateD = gst_value_get_fraction_denominator(framerate);
  }
  else {
    // Default to 0/1 indicating unknown framerate
    framerateN = 0;
    framerateD = 1;
  }

  rv = format->SetVideoFrameRate (framerateN, framerateD);
  NS_ENSURE_SUCCESS (rv, rv);

  const GValue* par = gst_structure_get_value(structure, "pixel-aspect-ratio");
  gint parN, parD;
  if (par) {
    parN = gst_value_get_fraction_numerator(par);
    parD = gst_value_get_fraction_denominator(par);
  }
  else {
    // Default to square pixels
    parN = 1;
    parD = 1;
  }

  rv = format->SetVideoPAR (parN, parD);
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::ProcessContainerProperties (
                             sbIMediaFormatContainerMutable *aContainerFormat,
                             GstStructure *aStructure)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER (aContainerFormat);
  NS_ENSURE_ARG_POINTER (aStructure);

  nsresult rv;
  const gchar *name = gst_structure_get_name (aStructure);
  nsCOMPtr<nsIWritablePropertyBag2> writableBag =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1", &rv);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = writableBag->SetPropertyAsACString(NS_LITERAL_STRING(GST_TYPE_PROPERTY),
                                          nsCString(name));
  NS_ENSURE_SUCCESS (rv, rv);

  if ( !strcmp (name, "video/mpeg")) {
    gboolean systemstream;
    if ( gst_structure_get_boolean (aStructure, "systemstream",
                                    &systemstream)) {
      rv = writableBag->SetPropertyAsBool (
             NS_LITERAL_STRING("systemstream"), systemstream);
      NS_ENSURE_SUCCESS (rv, rv);
    }
  }

  // get total duration of media item
  GstQuery *query;
  query = gst_query_new_duration (GST_FORMAT_TIME);
  gboolean res = gst_element_query (mPipeline, query);
  // initialize to unknown
  gint64 duration = -1;
  if (res) {
    gst_query_parse_duration (query, NULL, &duration);
  }
  gst_query_unref (query);
  rv = writableBag->SetPropertyAsInt64 (
         NS_LITERAL_STRING("duration"), duration);
  NS_ENSURE_SUCCESS (rv, rv);

  // TODO: Additional properties for other container formats.

  rv = aContainerFormat->SetProperties (writableBag);
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::ProcessVideoProperties (
                             sbIMediaFormatVideoMutable *aVideoFormat,
                             GstStructure *aStructure)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER (aVideoFormat);
  NS_ENSURE_ARG_POINTER (aStructure);

  nsresult rv;
  const gchar *name = gst_structure_get_name (aStructure);
  nsCOMPtr<nsIWritablePropertyBag2> writableBag =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1", &rv);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = writableBag->SetPropertyAsACString(NS_LITERAL_STRING(GST_TYPE_PROPERTY),
                                          nsCString(name));
  NS_ENSURE_SUCCESS (rv, rv);

  if ( !strcmp (name, "video/mpeg")) {
    gint mpegversion;
    if ( gst_structure_get_int (aStructure, "mpegversion", &mpegversion)) {
      rv = writableBag->SetPropertyAsInt32 (
             NS_LITERAL_STRING("mpegversion"), mpegversion);
      NS_ENSURE_SUCCESS (rv, rv);

      // video/mpeg is NOT unique to mpeg4. also used for mpeg1/2.
      if (mpegversion == 4) {
        gint levelid;
        if ( gst_structure_get_int (aStructure, "profile-level-id",
                                    &levelid)) {
          rv = writableBag->SetPropertyAsInt32 (
                 NS_LITERAL_STRING("profile-level-id"), levelid);
          NS_ENSURE_SUCCESS (rv, rv);
        }
      }
    }
  }
  else if ( !strcmp (name, "video/x-h264")) {
    // TODO: Additional property: profile ID?
  }
  else if ( !strcmp (name, "image/jpeg")) {
    gboolean interlaced;
    if ( gst_structure_get_boolean (aStructure, "interlaced", &interlaced)) {
      rv = writableBag->SetPropertyAsBool (
             NS_LITERAL_STRING("interlaced"), interlaced);
      NS_ENSURE_SUCCESS (rv, rv);
    }
  }
  else if ( !strcmp (name, "video/x-wmv")) {
    gint wmvversion;
    if ( gst_structure_get_int (aStructure, "wmvversion", &wmvversion)) {
      rv = writableBag->SetPropertyAsInt32 (
             NS_LITERAL_STRING("wmvversion"), wmvversion);
      NS_ENSURE_SUCCESS (rv, rv);
    }
    // TODO: Additional property: profile, level.
  }
  else if ( !strcmp (name, "video/x-pn-realvideo")) {
    gint rmversion;
    if ( gst_structure_get_int (aStructure, "rmversion", &rmversion)) {
      rv = writableBag->SetPropertyAsInt32 (
             NS_LITERAL_STRING("rmversion"), rmversion);
      NS_ENSURE_SUCCESS (rv, rv);
    }
  }

  rv = aVideoFormat->SetProperties (writableBag);
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::ProcessVideo(sbIMediaFormatVideo **aVideoFormat)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER (aVideoFormat);
  NS_ENSURE_STATE (mVideoSrc);

  nsresult rv;
  nsCOMPtr<sbIMediaFormatVideoMutable> format =
      do_CreateInstance (SB_MEDIAFORMATVIDEO_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  // mVideoSrc is the decoded video pad from decodebin. We can process this for
  // information about the output video: resolution, framerate, etc.
  sbGstCaps caps = gst_pad_get_current_caps(mVideoSrc);
  rv = ProcessVideoCaps(format, caps);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = format->SetBitRate(mVideoBitRate);
  NS_ENSURE_SUCCESS (rv, rv);

  if (mVideoDecoderSink) {
    // This is the sink pad on the decoder. We can process this for
    // information about what codec is being used.
    // If we don't have a decoder sink pad, then that SHOULD mean that we have
    // raw video from the demuxer. Alternatively, it means we screwed up
    // somehow.
    sbGstCaps videoCaps = gst_pad_get_current_caps(mVideoDecoderSink);
    GstStructure *structure = gst_caps_get_structure (videoCaps, 0);

    nsCString mimeType;
    rv = GetMimeTypeForCaps (videoCaps, mimeType);
    NS_ENSURE_SUCCESS (rv, rv);

    rv = format->SetVideoType (NS_ConvertUTF8toUTF16(mimeType));
    NS_ENSURE_SUCCESS (rv, rv);

    // format-specific attributes.
    rv = ProcessVideoProperties(format, structure);
    NS_ENSURE_SUCCESS (rv, rv);
  }
  else {
    // Raw video, mark as such.
    // TODO: Can we add any more checks in here to be sure it's ACTUALLY raw
    // video, and not just an internal error?
    rv = format->SetVideoType (NS_LITERAL_STRING ("video/x-raw"));
    NS_ENSURE_SUCCESS (rv, rv);

    // TODO: Add additional properties for raw video?
  }

  rv = CallQueryInterface(format.get(), aVideoFormat);
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}


nsresult
sbGStreamerMediaInspector::ProcessAudioProperties (
                             sbIMediaFormatAudioMutable *aAudioFormat,
                             GstStructure *aStructure)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER (aAudioFormat);
  NS_ENSURE_ARG_POINTER (aStructure);

  nsresult rv;
  const gchar *name = gst_structure_get_name (aStructure);
  nsCOMPtr<nsIWritablePropertyBag2> writableBag =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/sbpropertybag;1", &rv);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = writableBag->SetPropertyAsACString(NS_LITERAL_STRING(GST_TYPE_PROPERTY),
                                          nsCString(name));
  NS_ENSURE_SUCCESS (rv, rv);

  if ( !strcmp (name, "audio/mpeg")) {
    gint mpegversion;
    if ( gst_structure_get_int (aStructure, "mpegversion", &mpegversion)) {
      rv = writableBag->SetPropertyAsInt32 (
             NS_LITERAL_STRING("mpegversion"), mpegversion);
      NS_ENSURE_SUCCESS (rv, rv);

      // MP3 audio
      if (mpegversion == 1) {
        gint layer;
        if ( gst_structure_get_int (aStructure, "layer",
                                    &layer)) {
          rv = writableBag->SetPropertyAsInt32 (
                 NS_LITERAL_STRING("layer"), layer);
          NS_ENSURE_SUCCESS (rv, rv);
        }
      }
      else if (mpegversion == 2 || mpegversion == 4) {
        // TODO: Additional property: profile.
      }
    }
  }
  else if ( !strcmp (name, "audio/x-adpcm")) {
    const gchar *layout = gst_structure_get_string (aStructure, "layout");
    if (layout) {
      rv = writableBag->SetPropertyAsAString (
             NS_LITERAL_STRING("layout"), NS_ConvertUTF8toUTF16(layout));
      NS_ENSURE_SUCCESS (rv, rv);
    }
  }
  else if ( !strcmp (name, "audio/x-wma")) {
    gint wmaversion;
    if (gst_structure_get_int (aStructure, "wmaversion", &wmaversion)) {
      rv = writableBag->SetPropertyAsInt32 (
             NS_LITERAL_STRING("wmaversion"), wmaversion);
      NS_ENSURE_SUCCESS (rv, rv);
    }
    // TODO: Additional properties??
  }
  else if ( !strcmp (name, "audio/x-pn-realaudio")) {
    gint raversion;
    if (gst_structure_get_int (aStructure, "raversion", &raversion)) {
      rv = writableBag->SetPropertyAsInt32 (
             NS_LITERAL_STRING("raversion"), raversion);
      NS_ENSURE_SUCCESS (rv, rv);
    }
  }

  /// @todo There's no need to check the codec type before extracting
  ///       interesting properties.  The above code here and in
  ///       ProcessVideoProperties() and ProcessContainerProperties()
  ///       should be updated to use the following pattern:

  static const gchar * const INTERESTING_AUDIO_PROPS [] = {
                                // pcm
                               "width",
                               "endianness",
                               "signed",

                               // aac
                               "codec_data"
                             };

  rv = SetPropertiesFromGstStructure(writableBag,
                                     aStructure,
                                     INTERESTING_AUDIO_PROPS,
                                     NS_ARRAY_LENGTH(INTERESTING_AUDIO_PROPS));
  NS_ENSURE_SUCCESS (rv, rv);

  rv = aAudioFormat->SetProperties (writableBag);
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::ProcessAudio(sbIMediaFormatAudio **aAudioFormat)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER (aAudioFormat);
  NS_ENSURE_STATE (mAudioSrc);

  nsresult rv;
  nsCOMPtr<sbIMediaFormatAudioMutable> format =
      do_CreateInstance (SB_MEDIAFORMATAUDIO_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  // mAudioSrc is the decoded audio pad from decodebin. We can process this for
  // information about the output audio: sample rate, number of channels, etc.
  sbGstCaps caps = gst_pad_get_current_caps(mAudioSrc);
  GstStructure *structure = gst_caps_get_structure (caps, 0);

  gint rate, channels;
  if (gst_structure_get_int (structure, "rate", &rate)) {
    format->SetSampleRate (rate);
  }
  if (gst_structure_get_int (structure, "channels", &channels)) {
    format->SetChannels (channels);
  }

  rv = format->SetBitRate(mAudioBitRate);
  NS_ENSURE_SUCCESS (rv, rv);

  if (mAudioDecoderSink) {
    // This is the sink pad on the decoder. We can process this for
    // information about what codec is being used.
    // If we don't have a decoder sink pad, then that SHOULD mean that we have
    // raw audio from the demuxer. Alternatively, it means we screwed up
    // somehow.
    sbGstCaps audioCaps = gst_pad_get_current_caps(mAudioDecoderSink);
    structure = gst_caps_get_structure (audioCaps, 0);

    nsCString mimeType;
    rv = GetMimeTypeForCaps (audioCaps, mimeType);
    NS_ENSURE_SUCCESS (rv, rv);

    rv = format->SetAudioType (NS_ConvertUTF8toUTF16(mimeType));
    NS_ENSURE_SUCCESS (rv, rv);
  }
  else {
    // Raw audio, mark as such.
    // TODO: Can we add any more checks in here to be sure it's ACTUALLY raw
    // audio, and not just an internal error?
    format->SetAudioType (NS_LITERAL_STRING ("audio/x-raw"));
  }

  // format-specific attributes.
  rv = ProcessAudioProperties(format, structure);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = CallQueryInterface(format.get(), aAudioFormat);
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerMediaInspector::InspectorateElement (GstElement *element)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstElementFactory *factory = gst_element_get_factory (element);

  const gchar *klass = gst_element_factory_get_klass (factory);

  if (strstr (klass, "Demuxer")) {
    // Found what is hopefully our only demuxer!
    if (mDemuxerSink) {
      // No good! We don't support multiple demuxers.
      mTooComplexForCurrentImplementation = PR_TRUE;
    }
    else {
      mDemuxerSink = gst_element_get_pad (element, "sink");
    }
  }

  return NS_OK;
}

/* static */ void
sbGStreamerMediaInspector::fakesink_audio_event_cb(GstPad *pad,
        GstPadProbeInfo *info, sbGStreamerMediaInspector *inspector)
{
  GstEvent *event = gst_pad_probe_info_get_event(info);

  nsresult rv = inspector->FakesinkEvent(pad, event, PR_TRUE);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

/* static */ void
sbGStreamerMediaInspector::fakesink_video_event_cb(GstPad * pad,
        GstPadProbeInfo *info, sbGStreamerMediaInspector *inspector)
{
  GstEvent *event = gst_pad_probe_info_get_event(info);

  nsresult rv = inspector->FakesinkEvent(pad, event, PR_FALSE);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

/* static */ void
sbGStreamerMediaInspector::decodebin_pad_added_cb (GstElement * uridecodebin,
        GstPad * pad, sbGStreamerMediaInspector *inspector)
{
  nsresult rv = inspector->PadAdded(pad);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

