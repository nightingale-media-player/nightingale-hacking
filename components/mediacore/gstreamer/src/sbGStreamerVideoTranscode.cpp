/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://songbirdnest.com
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

#include "sbGStreamerVideoTranscode.h"

#include <sbIGStreamerService.h>

#include <sbFileUtils.h>
#include <sbGStreamerMediacoreUtils.h>
#include <sbStringUtils.h>
#include <sbClassInfoUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <sbMemoryUtils.h>
#include <sbStringBundle.h>
#include <sbTranscodeUtils.h>

#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsStringAPI.h>
#include <nsUnicharUtils.h>
#include <nsArrayUtils.h>
#include <nsNetUtil.h>

#include <nsIFile.h>
#include <nsIURI.h>
#include <nsIFileURL.h>
#include <nsIBinaryInputStream.h>
#include <sbIMediaItem.h>
#include <nsIProperty.h>

#include <gst/tag/tag.h>

#include <prlog.h>

#define PROGRESS_INTERVAL 200 /* milliseconds */

/**
 * To log this class, set the following environment variable in a debug build:
 *  NSPR_LOG_MODULES=sbGStreamerVideoTranscode:5 (or :3 for LOG messages only)
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gGStreamerVideoTranscode =
      PR_NewLogModule("sbGStreamerVideoTranscode");
#define LOG(args) PR_LOG(gGStreamerVideoTranscode, PR_LOG_WARNING, args)
#define TRACE(args) PR_LOG(gGStreamerVideoTranscode, PR_LOG_DEBUG, args)
#else /* PR_LOGGING */
#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */
#endif /* PR_LOGGING */

NS_IMPL_THREADSAFE_ISUPPORTS8(sbGStreamerVideoTranscoder,
                              nsIClassInfo,
                              sbIGStreamerPipeline,
                              sbITranscodeVideoJob,
                              sbIMediacoreEventTarget,
                              sbIJobProgress,
                              sbIJobProgressTime,
                              sbIJobCancelable,
                              nsITimerCallback)

NS_IMPL_CI_INTERFACE_GETTER6(sbGStreamerVideoTranscoder,
                             sbIGStreamerPipeline,
                             sbITranscodeVideoJob,
                             sbIMediacoreEventTarget,
                             sbIJobProgress,
                             sbIJobProgressTime,
                             sbIJobCancelable)

NS_DECL_CLASSINFO(sbGStreamerVideoTranscoder);
NS_IMPL_THREADSAFE_CI(sbGStreamerVideoTranscoder);

static GstPadProbeInfo *mAudioQueueSrcInfo;
static GstPadProbeInfo *mVideoQueueSrcInfo;

sbGStreamerVideoTranscoder::sbGStreamerVideoTranscoder() :
  sbGStreamerPipeline(),
  mStatus(sbIJobProgress::STATUS_RUNNING), // There is no NOT_STARTED
  mPipelineBuilt(PR_FALSE),
  mWaitingForCaps(PR_FALSE),
  mAudioSrc(NULL),
  mVideoSrc(NULL),
  mAudioQueueSrc(NULL),
  mVideoQueueSrc(NULL),
  mUseAudio(PR_FALSE),
  mUseVideo(PR_FALSE),
  mUseMuxer(PR_FALSE)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  mBuildLock = nsAutoLock::NewLock("VideoTranscoder lock");
  NS_ENSURE_TRUE (mBuildLock, /* void */);
}

sbGStreamerVideoTranscoder::~sbGStreamerVideoTranscoder()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv = CleanupPipeline();
  NS_ENSURE_SUCCESS (rv, /* void */);

  if (mBuildLock) {
    nsAutoLock::DestroyLock(mBuildLock);
  }
}

/* nsITimerCallback interface implementation */

NS_IMETHODIMP
sbGStreamerVideoTranscoder::Notify(nsITimer *aTimer)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aTimer);

  OnJobProgress();

  return NS_OK;
}

/* sbITranscodeJob interface implementation */

NS_IMETHODIMP
sbGStreamerVideoTranscoder::SetConfigurator(
                            sbITranscodingConfigurator *aConfigurator)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  mConfigurator = aConfigurator;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetConfigurator(
                            sbITranscodingConfigurator **aConfigurator)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aConfigurator);

  NS_IF_ADDREF (*aConfigurator = mConfigurator);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::SetSourceURI(const nsAString& aSourceURI)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // Can only set this while we don't have a pipeline built/running.
  NS_ENSURE_STATE (!mPipelineBuilt);

  mSourceURI = aSourceURI;

  // Use the source URI as the resource name too.
  mResourceDisplayName = mSourceURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetSourceURI(nsAString& aSourceURI)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  aSourceURI = mSourceURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::SetDestURI(const nsAString& aDestURI)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // Can only set this while we don't have a pipeline built/running.
  NS_ENSURE_STATE (!mPipelineBuilt);

  mDestURI = aDestURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetDestURI(nsAString& aDestURI)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  aDestURI = mDestURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetDestStream(nsIOutputStream **aStream)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aStream);

  NS_IF_ADDREF(*aStream = mDestStream);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::SetDestStream(nsIOutputStream *aStream)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // Can only set this while we don't have a pipeline built/running.
  NS_ENSURE_STATE (!mPipelineBuilt);

  mDestStream = aStream;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetMetadata(sbIPropertyArray **aMetadata)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aMetadata);

  NS_IF_ADDREF(*aMetadata = mMetadata);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::SetMetadata(sbIPropertyArray *aMetadata)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // Can only set this while we don't have a pipeline built/running.
  NS_ENSURE_STATE (!mPipelineBuilt);

  mMetadata = aMetadata;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetMetadataImage(nsIInputStream **aImageStream)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aImageStream);

  // Can only set this while we don't have a pipeline built/running.
  NS_ENSURE_STATE (!mPipelineBuilt);

  NS_IF_ADDREF(*aImageStream = mImageStream);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::SetMetadataImage(nsIInputStream *aImageStream)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // Can only set this while we don't have a pipeline built/running.
  NS_ENSURE_STATE (!mPipelineBuilt);

  mImageStream = aImageStream;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::Vote(sbIMediaItem *aMediaItem, PRInt32 *aVote)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aVote);

  nsString contentType;
  nsresult rv = aMediaItem->GetContentType(contentType);
  NS_ENSURE_SUCCESS(rv, rv);

  /* For now just vote 1 for anything audio or video */
  if (contentType.EqualsLiteral("video") ||
      contentType.EqualsLiteral("audio"))
  {
    *aVote = 1;
  }
  else {
    *aVote = 0;
  }

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::BuildPipeline()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_STATE (!mPipelineBuilt);

  nsresult rv = BuildTranscodePipeline ("transcode-pipeline");
  NS_ENSURE_SUCCESS (rv, rv);

  SetPipelineOp(GStreamer::OP_TRANSCODING);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::Transcode()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv = ClearStatus();
  NS_ENSURE_SUCCESS (rv, rv);

  // PlayPipeline builds and then starts the pipeline; assert that we haven't
  // been called with an already running pipeline.
  NS_ENSURE_STATE (!mPipelineBuilt);

  return PlayPipeline();
}

// Override base class to start the progress reporter
NS_IMETHODIMP
sbGStreamerVideoTranscoder::PlayPipeline()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;

  rv = sbGStreamerPipeline::PlayPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  rv = StartProgressReporting();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

// Override base class to stop the progress reporter
NS_IMETHODIMP
sbGStreamerVideoTranscoder::StopPipeline()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;

  rv = sbGStreamerPipeline::StopPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  rv = StopProgressReporting();
  NS_ENSURE_SUCCESS (rv, rv);

  // Clean up any remnants of this pipeline so that a new transcoding
  // attempt is possible
  rv = CleanupPipeline();

  // Inform listeners of new job status
  rv = OnJobProgress();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

/* sbIJobCancelable interface implementation */

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetCanCancel(PRBool *aCanCancel)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aCanCancel);

  *aCanCancel = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::Cancel()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  mStatus = sbIJobProgress::STATUS_FAILED; // We don't have a 'cancelled' state.

  nsresult rv = StopPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

/* sbIJobProgress interface implementation */

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetElapsedTime(PRUint32 *aElapsedTime)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aElapsedTime);

  /* Get the running time, and convert to milliseconds */
  *aElapsedTime = static_cast<PRUint32>(GetRunningTime() / GST_MSECOND);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetRemainingTime(PRUint32 *aRemainingTime)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstClockTime duration = QueryDuration();
  GstClockTime position = QueryPosition();
  GstClockTime elapsed = GetRunningTime();

  if (duration == GST_CLOCK_TIME_NONE || position == GST_CLOCK_TIME_NONE ||
      elapsed == GST_CLOCK_TIME_NONE)
  {
    /* Unknown, so set to -1 */
    *aRemainingTime = (PRUint32)-1;
  }
  else {
    GstClockTime totalTime = gst_util_uint64_scale (elapsed, duration,
            position);
    /* Convert to milliseconds */
    *aRemainingTime =
     static_cast<PRUint32>((totalTime - elapsed) / GST_MSECOND);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetStatus(PRUint16 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);

  *aStatus = mStatus;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetBlocked(PRBool *aBlocked)
{
  NS_ENSURE_ARG_POINTER(aBlocked);

  *aBlocked = PR_FALSE;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetStatusText(nsAString& aText)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv = NS_ERROR_FAILURE;

  switch (mStatus) {
    case sbIJobProgress::STATUS_FAILED:
      rv = SBGetLocalizedString(aText,
              NS_LITERAL_STRING("mediacore.gstreamer.transcode.failed"));
      break;
    case sbIJobProgress::STATUS_SUCCEEDED:
      rv = SBGetLocalizedString(aText,
              NS_LITERAL_STRING("mediacore.gstreamer.transcode.succeeded"));
      break;
    case sbIJobProgress::STATUS_RUNNING:
      rv = SBGetLocalizedString(aText,
              NS_LITERAL_STRING("mediacore.gstreamer.transcode.running"));
      break;
    default:
      NS_NOTREACHED("Status is invalid");
  }

  return rv;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetTitleText(nsAString& aText)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  return SBGetLocalizedString(aText,
          NS_LITERAL_STRING("mediacore.gstreamer.transcode.title"));
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetProgress(PRUint32* aProgress)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aProgress);

  GstClockTime duration = QueryDuration();
  GstClockTime position = QueryPosition();

  if (duration != GST_CLOCK_TIME_NONE && position != GST_CLOCK_TIME_NONE &&
      duration != 0)
  {
    // Scale to [0-1000], see comment in GetTotal for why we do this.
    *aProgress = (PRUint32)gst_util_uint64_scale (position, 1000, duration);
  }
  else {
    *aProgress = 0; // Unknown
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetTotal(PRUint32* aTotal)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aTotal);

  GstClockTime duration = QueryDuration();

  // The job progress stuff doesn't like overly large numbers, so we artifically
  // fix it to a max of 1000.

  if (duration != GST_CLOCK_TIME_NONE) {
    *aTotal = 1000;
  }
  else {
    *aTotal = 0;
  }

  return NS_OK;
}

// Note that you can also get errors reported via the mediacore listener
// interfaces.
NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetErrorCount(PRUint32* aErrorCount)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aErrorCount);
  NS_ASSERTION(NS_IsMainThread(),
          "sbIJobProgress::GetErrorCount is main thread only!");

  *aErrorCount = mErrors.Length();

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::GetErrorMessages(nsIStringEnumerator** aMessages)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aMessages);
  NS_ASSERTION(NS_IsMainThread(),
    "sbIJobProgress::GetProgress is main thread only!");

  nsresult rv;

  *aMessages = nsnull;

  nsRefPtr<sbJobErrorEnumerator<sbITranscodeError> > enumerator =
      new sbJobErrorEnumerator<sbITranscodeError>(mErrors);
  rv = CallQueryInterface(enumerator.get(), aMessages);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerVideoTranscoder::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbGStreamerVideoTranscoder::AddJobProgressListener is main thread only!");

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
sbGStreamerVideoTranscoder::RemoveJobProgressListener(
        sbIJobProgressListener* aListener)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbGStreamerVideoTranscoder::RemoveJobProgressListener is main thread only!");

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
sbGStreamerVideoTranscoder::OnJobProgress()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ASSERTION(NS_IsMainThread(), \
    "sbGStreamerVideoTranscoder::OnJobProgress is main thread only!");

  // Announce our status to the world
  for (PRInt32 i = mProgressListeners.Count() - 1; i >= 0; --i) {
     // Ignore any errors from listeners
     mProgressListeners[i]->OnJobProgress(this);
  }
  return NS_OK;
}

void sbGStreamerVideoTranscoder::HandleErrorMessage(GstMessage *message)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GError *gerror = NULL;
  gchar *debug = NULL;
  nsresult rv;

  mStatus = sbIJobProgress::STATUS_FAILED;

  gst_message_parse_error(message, &gerror, &debug);

  nsCOMPtr<sbITranscodeError> errorObj;
  rv = SB_NewTranscodeError(NS_ConvertUTF8toUTF16(gerror->message),
                            NS_ConvertUTF8toUTF16(gerror->message),
                            SBVoidString(),
                            mSourceURI,
                            nsnull,
                            getter_AddRefs(errorObj));
  if (NS_SUCCEEDED(rv)) {
    mErrors.AppendElement(errorObj);
  }

  g_error_free (gerror);
  g_free(debug);

  // This will stop the pipeline and update listeners
  sbGStreamerPipeline::HandleErrorMessage (message);
}

void sbGStreamerVideoTranscoder::HandleEOSMessage(GstMessage *message)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  mStatus = sbIJobProgress::STATUS_SUCCEEDED;

  // This will stop the pipeline and update listeners
  sbGStreamerPipeline::HandleEOSMessage (message);
}

GstClockTime sbGStreamerVideoTranscoder::QueryPosition()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstQuery *query;
  gint64 position = GST_CLOCK_TIME_NONE;

  if (!mPipeline)
    return position;

  query = gst_query_new_position(GST_FORMAT_TIME);

  if (gst_element_query(mPipeline, query))
    gst_query_parse_position(query, NULL, &position);

  gst_query_unref (query);

  return position;
}

GstClockTime sbGStreamerVideoTranscoder::QueryDuration()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstQuery *query;
  gint64 duration = GST_CLOCK_TIME_NONE;

  if (!mPipeline)
    return duration;

  query = gst_query_new_duration(GST_FORMAT_TIME);

  if (gst_element_query(mPipeline, query))
    gst_query_parse_duration(query, NULL, &duration);

  gst_query_unref (query);

  return duration;
}

nsresult
sbGStreamerVideoTranscoder::StartProgressReporting()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  NS_ENSURE_STATE(!mProgressTimer);

  nsresult rv;
  mProgressTimer =
              do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mProgressTimer->InitWithCallback(this,
          PROGRESS_INTERVAL, nsITimer::TYPE_REPEATING_SLACK);

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::StopProgressReporting()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  if (mProgressTimer) {
    mProgressTimer->Cancel();
    mProgressTimer = nsnull;
  }

  return NS_OK;
}

void
sbGStreamerVideoTranscoder::AsyncStopPipeline()
{
  nsresult rv = StopPipeline();
  NS_ENSURE_SUCCESS (rv, /* void */);
}

void
sbGStreamerVideoTranscoder::TranscodingFatalError (const char *errorName)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;
  sbStringBundle bundle;
  nsString message = bundle.Get(errorName);

  // Add an error message for users of sbIJobProgress interface.
  nsCOMPtr<sbITranscodeError> errorObj;
  rv = SB_NewTranscodeError(message, message, SBVoidString(),
                            mSourceURI,
                            nsnull,
                            getter_AddRefs(errorObj));
  if (NS_SUCCEEDED(rv)) {
    mErrors.AppendElement(errorObj);
  }

  nsRefPtr<sbMediacoreError> error;

  NS_NEWXPCOM (error, sbMediacoreError);
  NS_ENSURE_TRUE (error, /* void */);

  error->Init (sbIMediacoreError::FAILED, message);

  // Dispatch event so that listeners can act directly on that.
  DispatchMediacoreEvent (sbIMediacoreEvent::ERROR_EVENT, nsnull, error);

  /* Stop the pipeline. We might be calling this from a non-main thread, so
     dispatch the shutdown asynchronously. */
  nsCOMPtr<nsIRunnable> event = NS_NEW_RUNNABLE_METHOD (
          sbGStreamerVideoTranscoder, this, AsyncStopPipeline);

  rv = NS_DispatchToMainThread(event, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

nsresult
sbGStreamerVideoTranscoder::AddImageToTagList(GstTagList *aTags,
        nsIInputStream *aStream)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  PRUint32 imageDataLen;
  PRUint8 *imageData;
  nsresult rv;

  nsCOMPtr<nsIBinaryInputStream> stream =
             do_CreateInstance("@mozilla.org/binaryinputstream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->SetInputStream(aStream);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aStream->Available(&imageDataLen);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = stream->ReadByteArray(imageDataLen, &imageData);
  NS_ENSURE_SUCCESS(rv, rv);

  sbAutoNSMemPtr imageDataDestroy(imageData);

  GstSample *imgsamp = gst_tag_image_data_to_image_sample(imageData,
                         imageDataLen, GST_TAG_IMAGE_TYPE_FRONT_COVER);
  if (!imgsamp)
    return NS_ERROR_FAILURE;

  gst_tag_list_add(aTags, GST_TAG_MERGE_REPLACE, GST_TAG_IMAGE,
                   imgsamp, NULL);
  gst_sample_unref(imgsamp);

  return NS_OK;
}


nsresult
sbGStreamerVideoTranscoder::SetMetadataOnTagSetters()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstTagList *tags = ConvertPropertyArrayToTagList(mMetadata);

  if (mImageStream) {
    if (!tags) {
      tags = gst_tag_list_new_empty();
    }

    // Ignore return value here, failure is not critical.
    AddImageToTagList (tags, mImageStream);
  }

  if (tags) {
    // Find all the tag setters in the pipeline
    GstIterator *it = gst_bin_iterate_all_by_interface (
            (GstBin *)mPipeline, GST_TYPE_TAG_SETTER);
    GstElement *element;

    while (gst_iterator_next(it, (GValue*) &element) == GST_ITERATOR_OK) {
      GstTagSetter *setter = GST_TAG_SETTER (element);

      /* Use MERGE_REPLACE: preserves existing tag where we don't have one
       * in our taglist */
      gst_tag_setter_merge_tags (setter, tags, GST_TAG_MERGE_REPLACE);
      g_object_unref (element);
    }
    gst_iterator_free (it);
    gst_tag_list_free (tags);
  }

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::ClearStatus()
{
  /* Cleanup done directly _before_ creating a pipeline */
  mStatus = sbIJobProgress::STATUS_RUNNING;
  mErrors.Clear();

  return NS_OK;
}

void
sbGStreamerVideoTranscoder::CleanupPads()
{
  if (mAudioSrc) {
    g_object_unref (mAudioSrc);
    mAudioSrc = NULL;
  }

  if (mVideoSrc) {
    g_object_unref (mVideoSrc);
    mVideoSrc = NULL;
  }

  if (mAudioQueueSrc) {
    g_object_unref (mAudioQueueSrc);
    mAudioQueueSrc = NULL;
  }

  if (mVideoQueueSrc) {
    g_object_unref (mVideoQueueSrc);
    mVideoQueueSrc = NULL;
  }
}

nsresult
sbGStreamerVideoTranscoder::CleanupPipeline()
{
  /* Cleanup done _after_ destroying a pipeline. Note that we don't clean up
     error messages/status, as things might want to query those */
  CleanupPads();

  mPipelineBuilt = PR_FALSE;
  mWaitingForCaps = PR_FALSE;

  return NS_OK;
}

/* static */ void
sbGStreamerVideoTranscoder::decodebin_pad_added_cb (GstElement * uridecodebin,
        GstPad * pad, sbGStreamerVideoTranscoder *transcoder)
{
  nsresult rv = transcoder->DecoderPadAdded(uridecodebin, pad);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

/* static */ void
sbGStreamerVideoTranscoder::decodebin_no_more_pads_cb (
        GstElement * uridecodebin, sbGStreamerVideoTranscoder *transcoder)
{
  nsresult rv = transcoder->DecoderNoMorePads(uridecodebin);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

/* static */ void
sbGStreamerVideoTranscoder::pad_blocked_audio_cb(GstPad * pad, GstPadProbeInfo *info,
        sbGStreamerVideoTranscoder *transcoder)
{
  mAudioQueueSrcInfo = info;
  nsresult rv = transcoder->PadBlocked(pad);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

/* static */ void
sbGStreamerVideoTranscoder::pad_blocked_video_cb(GstPad * pad, GstPadProbeInfo *info,
        sbGStreamerVideoTranscoder *transcoder)
{
  mVideoQueueSrcInfo = info;
  nsresult rv = transcoder->PadBlocked(pad);
  NS_ENSURE_SUCCESS (rv, /* void */);
}

/* static */ void
sbGStreamerVideoTranscoder::pad_notify_caps_cb (GObject *obj, GParamSpec *pspec,
        sbGStreamerVideoTranscoder *transcoder)
{
  nsresult rv = transcoder->PadNotifyCaps (GST_PAD (obj));
  NS_ENSURE_SUCCESS (rv, /* void */);
}

nsresult
sbGStreamerVideoTranscoder::DecoderPadAdded (GstElement *uridecodebin,
        GstPad *pad)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // A new decoded pad has been added from the decodebin.
  // At this point, we're able to look at the template caps (via
  // gst_pad_get_caps()), which is sufficient to figure out if we have an audio
  // or video pad, but not sufficient to actually get all the details of
  // the audio/video stream.
  // Decide whether we want to use it at all. If we do, add it to the list
  // of pending pads. We don't actually build anything until later, once we
  // have all the pads, and caps on all pads.
  if (mPipelineBuilt) {
    LOG(("pad-added after pipeline fully constructed; cannot use"));
    return NS_ERROR_FAILURE;
  }

  GstCaps *caps = gst_pad_query_caps(pad, NULL);
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  const gchar *name = gst_structure_get_name (structure);
  bool isVideo = g_str_has_prefix (name, "video/");
  bool isAudio = g_str_has_prefix (name, "audio/");

  gst_caps_unref (caps);

  if (isAudio) {
    if (mAudioSrc) {
      LOG(("Multiple audio streams: ignoring subsequent ones"));
      return NS_OK;
    }

#if 0
    // TODO: Some sort of API like this will be needed once we're using this
    // for audio-only transcoding as well.
    if (!mTranscoderConfigurator->SupportsAudio()) {
      LOG(("Transcoder not configured to support audio, ignoring stream"));
      return NS_OK;
    }
#endif

    LOG(("Using audio pad %s:%s for audio stream", GST_DEBUG_PAD_NAME (pad)));
    gst_object_ref (pad);
    mAudioSrc = pad;
  }
  else if (isVideo) {
    if (mVideoSrc) {
      LOG(("Multiple video streams: ignoring subsequent ones"));
      return NS_OK;
    }
#if 0
    // TODO: Some sort of API like this will be needed once we're using this
    // for audio-only transcoding as well.
    if (!mTranscoderConfigurator->SupportsVideo()) {
      LOG(("Transcoder not configured to support video, ignoring stream"));
      return NS_OK;
    }
#endif

    LOG(("Using video pad %s:%s for video stream", GST_DEBUG_PAD_NAME (pad)));
    gst_object_ref (pad);
    mVideoSrc = pad;
  }
  else {
    LOG(("Ignoring non-audio, non-video stream"));
    return NS_OK;
  }

  return NS_OK;
}

void
sbGStreamerVideoTranscoder::ConfigureVideoBox (GstElement *videobox,
    GstCaps *aInputVideoCaps, gint outputWidth, gint outputHeight,
    gint outputParN, gint outputParD)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  gint imageWidth, imageHeight, imageParN, imageParD;
  gboolean ret;
  GstStructure *structure = gst_caps_get_structure (aInputVideoCaps, 0);

  /* Ignore failures here, they aren't possible: if these weren't here, we'd
     have rejected this stream previously */
  ret = gst_structure_get_int (structure, "width", &imageWidth);
  NS_ASSERTION (ret, "Invalid image caps, no width");
  ret = gst_structure_get_int (structure, "height", &imageHeight);
  NS_ASSERTION (ret, "Invalid image caps, no height");

  const GValue* par = gst_structure_get_value (structure,
          "pixel-aspect-ratio");
  if (par) {
    imageParN = gst_value_get_fraction_numerator(par);
    imageParD = gst_value_get_fraction_denominator(par);
  }
  else {
    // Default to square pixels.
    imageParN = imageParD = 1;
  }

  gint imageDarN = imageWidth * imageParN;
  gint imageDarD = imageHeight * imageParD;

  gint outputDarN = outputWidth * outputParN;
  gint outputDarD = outputHeight * outputParD;

  LOG(("Determining output geometry. Output image is %dx%d (PAR %d:%d). "
       "Input image is %dx%d (PAR %d:%d)", outputWidth, outputHeight,
       outputParN, outputParD, imageWidth, imageHeight, imageParN, imageParD));

  // Ok, we have our basic variables. Now we want to check if the image DAR
  // is less than, equal to, or greater than, the output DAR.
  // TODO: check if any of these variables might plausibly get large enough
  // that this could overflow.
  if (imageDarN * outputDarD > outputDarN * imageDarD) {
    // Image DAR is greater than output DAR. So, we use the full width of the
    // image, and add padding at the top and bottom.
    //     padding = outputHeight - (outputWidth / imageDAR * outputPAR);
    gint outputImageHeight = outputWidth *
        (imageDarD * outputParN) / (imageDarN * outputParD);
    gint padding = outputHeight - outputImageHeight;
    // Because we're usually dealing with chroma-subsampled video formats,
    // adding an odd amount of padding is a bad idea. For example, if we need
    // to add 2 pixels of padding, it's best to have both on one side, rather
    // than one on each.
    gint paddingBottom, paddingTop;
    if (padding % 4 == 0) {
      paddingBottom = padding / 2;
    }
    else {
      paddingBottom = padding / 2 + 1;
    }
    paddingTop = padding - paddingBottom;

    LOG(("Padding %d pixels at top, %d pixels at bottom", paddingTop,
         paddingBottom));

    // Negative values indicate adding padding (rather than cropping)
    g_object_set (videobox, "top", -paddingTop, "bottom", -paddingBottom, NULL);
  }
  else if (imageDarN * outputDarD < outputDarN * imageDarD) {
    // Image DAR is less than output DAR. So, we use the full height of the
    // image, and add padding at the left and right.
    //     padding = outputWidth - (outputHeight * imageDAR / outputPAR);
    gint outputImageWidth = outputHeight *
        (imageDarN * outputParD) / (imageDarD * outputParN);
    gint padding = outputWidth - outputImageWidth;
    // Because we're usually dealing with chroma-subsampled video formats,
    // adding an odd amount of padding is a bad idea. For example, if we need
    // to add 2 pixels of padding, it's best to have both on one side, rather
    // than one on each.
    gint paddingRight, paddingLeft;
    if (padding % 4 == 0) {
      paddingRight = padding / 2;
    }
    else {
      paddingRight = padding / 2 + 1;
    }
    paddingLeft = padding - paddingRight;

    LOG(("Padding %d pixels at left, %d pixels at right", paddingLeft,
         paddingRight));

    // Negative values indicate adding padding (rather than cropping)
    g_object_set (videobox, "left", -paddingLeft, "right", -paddingRight, NULL);
  }
  else {
    LOG(("No padding required"));
  }
}

nsresult
sbGStreamerVideoTranscoder::BuildVideoBin(GstCaps *aInputVideoCaps,
                                          GstElement **aVideoBin)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // See the comment/ascii-art in BuildTranscodePipeline for details about what
  // this does
  nsresult rv;
  GstBin *bin = NULL;
  GstElement *videorate = NULL;
  GstElement *colorspace = NULL;
  GstElement *videoscale = NULL;
  GstElement *videobox = NULL;
  GstElement *capsfilter = NULL;
  GstElement *encoder = NULL;
  nsCOMPtr<nsIPropertyBag> encoderProperties;

  PRInt32 outputWidth, outputHeight;
  PRUint32 outputParN, outputParD;
  PRUint32 outputFramerateN, outputFramerateD;

  // Ask the configurator for what format we should feed into the encoder
  nsCOMPtr<sbIMediaFormatVideo> videoFormat;
  rv = mConfigurator->GetVideoFormat (getter_AddRefs(videoFormat));
  NS_ENSURE_SUCCESS (rv, rv);

  rv = videoFormat->GetVideoWidth(&outputWidth);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = videoFormat->GetVideoHeight(&outputHeight);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = videoFormat->GetVideoPAR(&outputParN, &outputParD);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = videoFormat->GetVideoFrameRate(&outputFramerateN, &outputFramerateD);
  NS_ENSURE_SUCCESS (rv, rv);

  /* Ensure the configurator didn't give us bogus data for any of these, and
     just fail if it did. */
  NS_ENSURE_TRUE(outputWidth > 0, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(outputHeight > 0, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(outputParN > 0, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(outputParD > 0, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(outputFramerateN > 0, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(outputFramerateD > 0, NS_ERROR_FAILURE);

  // Ask the configurator what encoder (if any) we should use.
  nsString encoderName;
  rv = mConfigurator->GetVideoEncoder(encoderName);
  NS_ENSURE_SUCCESS (rv, rv);

  GstPad *srcpad, *sinkpad, *ghostpad;
  GstElement *last;
  GstCaps *caps;

  bin = GST_BIN (gst_bin_new("video-encode-bin"));
  videorate = gst_element_factory_make ("videorate", NULL);
  colorspace = gst_element_factory_make ("ffmpegcolorspace", NULL);
  videoscale = gst_element_factory_make ("videoscale", NULL);
  videobox = gst_element_factory_make ("videobox", NULL);
  capsfilter = gst_element_factory_make ("capsfilter", NULL);
  encoder = NULL;

  if (!videorate || !colorspace || !videoscale || !videobox || !capsfilter)
  {
    // Failed to create an element that we expected to be present, means
    // the gstreamer installation is messed up.
    rv = NS_ERROR_FAILURE;
    goto failed;
  }

  if (encoderName.IsEmpty()) {
    LOG(("Video enabled but no video encoder specified"));
    rv = NS_ERROR_FAILURE;
    goto failed;
  }

  encoder = gst_element_factory_make (
          NS_ConvertUTF16toUTF8(encoderName).BeginReading(), NULL);

  if (!encoder) {
    LOG(("No encoder %s available",
                NS_ConvertUTF16toUTF8(encoderName).BeginReading()));
    TranscodingFatalError(
            "songbird.transcode.error.video_encoder_unavailable");
    rv = NS_ERROR_FAILURE;
    goto failed;
  }

  rv = mConfigurator->GetVideoEncoderProperties(
          getter_AddRefs(encoderProperties));
  if (NS_FAILED (rv)) {
    goto failed;
  }

  rv = ApplyPropertyBagToElement(encoder, encoderProperties);
  if (NS_FAILED (rv)) {
    goto failed;
  }

  /* Configure videoscale to use 4-tap scaling for higher quality */
  g_object_set (videoscale, "method", 2, NULL);

  /* Configure capsfilter for our output size, framerate, etc. */
  // TODO: Should we also permit video/x-raw-rgb? It doesn't matter for now,
  // as all the encoders we're using prefer YUV.
  caps = gst_caps_new_simple ("video/x-raw-yuv",
          "width", G_TYPE_INT, outputWidth,
          "height", G_TYPE_INT, outputHeight,
          "pixel-aspect-ratio", GST_TYPE_FRACTION, outputParN, outputParD,
          "framerate", GST_TYPE_FRACTION, outputFramerateN, outputFramerateD,
          NULL);
  g_object_set (capsfilter, "caps", caps, NULL);
  gst_caps_unref (caps);

  /* Configure videobox to add black bars around the image to preserve the
     actual image aspect ratio, if required. This is a little complex, so
     it's factored out into another function. */
  ConfigureVideoBox (videobox, aInputVideoCaps, outputWidth, outputHeight,
                     outputParN, outputParD);

  // Now, add to the bin, and then link everything up.
  gst_bin_add_many (bin, videorate, colorspace, videoscale, videobox,
          capsfilter, NULL);
  gst_element_link_many (videorate, colorspace, videoscale, videobox,
          capsfilter, NULL);

  last = capsfilter;

  gst_bin_add (bin, encoder);
  gst_element_link (capsfilter, encoder);
  last = encoder;

  // Finally, add ghost pads to our bin.
  sinkpad = gst_element_get_static_pad (videorate, "sink");
  ghostpad = gst_ghost_pad_new ("sink", sinkpad);
  g_object_unref (sinkpad);
  gst_element_add_pad (GST_ELEMENT (bin), ghostpad);

  srcpad = gst_element_get_static_pad (last, "src");
  ghostpad = gst_ghost_pad_new ("src", srcpad);
  g_object_unref (srcpad);
  gst_element_add_pad (GST_ELEMENT (bin), ghostpad);

  // Ok, done; set return value.
  *aVideoBin = GST_ELEMENT (bin);

  return NS_OK;

failed:
  if (videorate)
    g_object_unref (videorate);
  if (colorspace)
    g_object_unref (colorspace);
  if (videoscale)
    g_object_unref (videoscale);
  if (videobox)
    g_object_unref (videobox);
  if (capsfilter)
    g_object_unref (capsfilter);
  if (encoder)
    g_object_unref (encoder);
  if (bin)
    g_object_unref (bin);

  return rv;
}

nsresult
sbGStreamerVideoTranscoder::GetRawAudioCaps(GstCaps **aResultCaps)
{
  nsresult rv;

  nsCOMPtr<nsIPropertyBag> encoderProperties;
  rv = mConfigurator->GetAudioEncoderProperties(
          getter_AddRefs(encoderProperties));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> isFloatVar;
  rv = encoderProperties->GetProperty(NS_LITERAL_STRING ("IsFloat"),
                                      getter_AddRefs(isFloatVar));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool isFloat;
  rv = isFloatVar->GetAsBool(&isFloat);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> isLittleEndianVar;
  rv = encoderProperties->GetProperty(NS_LITERAL_STRING ("LittleEndian"),
                                      getter_AddRefs(isLittleEndianVar));
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool isLittleEndian;
  rv = isLittleEndianVar->GetAsBool(&isLittleEndian);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIVariant> sampleDepthVar;
  rv = encoderProperties->GetProperty(NS_LITERAL_STRING ("Depth"),
                                      getter_AddRefs(sampleDepthVar));
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 sampleDepth;
  rv = sampleDepthVar->GetAsInt32(&sampleDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 outputRate, outputChannels;

  // Ask the configurator for what format we should feed into the encoder
  nsCOMPtr<sbIMediaFormatAudio> audioFormat;
  rv = mConfigurator->GetAudioFormat (getter_AddRefs(audioFormat));
  NS_ENSURE_SUCCESS (rv, rv);

  rv = audioFormat->GetSampleRate (&outputRate);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = audioFormat->GetChannels (&outputChannels);
  NS_ENSURE_SUCCESS (rv, rv);

  gint32 endianness = isLittleEndian ? G_LITTLE_ENDIAN : G_BIG_ENDIAN;
  GstCaps *caps;
  if (isFloat) {
    caps = gst_caps_new_simple ("audio/x-raw-float",
          "endianness", G_TYPE_INT, endianness,
          "width", G_TYPE_INT, sampleDepth,
          "rate", G_TYPE_INT, outputRate,
          "channels", G_TYPE_INT, outputChannels);
  }
  else {
    caps = gst_caps_new_simple ("audio/x-raw-int",
          "endianness", G_TYPE_INT, endianness,
          "width", G_TYPE_INT, sampleDepth,
          "depth", G_TYPE_INT, sampleDepth,
          "rate", G_TYPE_INT, outputRate,
          "channels", G_TYPE_INT, outputChannels,
          "signed", G_TYPE_BOOLEAN, sampleDepth != 8);
  }

  *aResultCaps = caps;

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::BuildAudioBin(GstCaps *aInputAudioCaps,
                                          GstElement **aAudioBin)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // See the comment/ascii-art in BuildTranscodePipeline for details about what
  // this does
  nsresult rv;
  GstBin *bin = NULL;
  GstElement *audiorate = NULL;
  GstElement *audioconvert = NULL;
  GstElement *audioresample = NULL;
  GstElement *capsfilter = NULL;
  GstElement *audioresample2 = NULL;
  GstElement *encoder = NULL;

  PRInt32 outputRate, outputChannels;

  // Ask the configurator for what format we should feed into the encoder
  nsCOMPtr<sbIMediaFormatAudio> audioFormat;
  rv = mConfigurator->GetAudioFormat (getter_AddRefs(audioFormat));
  NS_ENSURE_SUCCESS (rv, rv);

  rv = audioFormat->GetSampleRate (&outputRate);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = audioFormat->GetChannels (&outputChannels);
  NS_ENSURE_SUCCESS (rv, rv);

  /* Ensure the configurator didn't give us bogus data for any of these, and
     just fail if it did. */
  NS_ENSURE_TRUE (outputRate > 0, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE (outputChannels > 0, NS_ERROR_FAILURE);

  // Ask the configurator what encoder (if any) we should use.
  nsString encoderName;
  rv = mConfigurator->GetAudioEncoder(encoderName);
  NS_ENSURE_SUCCESS (rv, rv);

  GstPad *srcpad, *sinkpad, *ghostpad;
  GstElement *last;
  GstCaps *caps;
  GstStructure *structure;

  bin = GST_BIN (gst_bin_new("audio-encode-bin"));
  audiorate = gst_element_factory_make ("audiorate", NULL);
  audioconvert = gst_element_factory_make ("audioconvert", NULL);
  audioresample = gst_element_factory_make ("audioresample", NULL);
  capsfilter = gst_element_factory_make ("capsfilter", NULL);
  audioresample2 = gst_element_factory_make ("audioresample", NULL);

  if (!audiorate || !audioconvert || !audioresample || !capsfilter ||
      !audioresample2)
  {
    // Failed to create an element that we expected to be present, means
    // the gstreamer installation is messed up.
    rv = NS_ERROR_FAILURE;
    goto failed;
  }

  // We may have no encoder - that's used if we're outputting raw audio. That's
  // perfectly ok!
  if (!encoderName.IsEmpty()) {
    encoder = gst_element_factory_make (
            NS_ConvertUTF16toUTF8 (encoderName).BeginReading(), NULL);
    if (!encoder) {
      LOG(("No encoder %s available",
                  NS_ConvertUTF16toUTF8(encoderName).BeginReading()));
      TranscodingFatalError(
              "songbird.transcode.error.audio_encoder_unavailable");
      rv = NS_ERROR_FAILURE;
      goto failed;
    }

    nsCOMPtr<nsIPropertyBag> encoderProperties;
    rv = mConfigurator->GetAudioEncoderProperties(
            getter_AddRefs(encoderProperties));
    if (NS_FAILED (rv)) {
      goto failed;
    }

    rv = ApplyPropertyBagToElement(encoder, encoderProperties);
    if (NS_FAILED (rv)) {
      goto failed;
    }
  }

  /* Configure capsfilter for our output sample rate and channels. Allow
     either int or float audio; this avoids having to introspect the encoder
     to find out what it supports.

     As a special case, if there is no audio encoder, that means we want
     raw PCM output. In this case, we select the format here in the caps.
   */
  if (encoder) {
    caps = gst_caps_new_empty ();
    structure = gst_structure_new ("audio/x-raw-int",
            "rate", G_TYPE_INT, outputRate,
            "channels", G_TYPE_INT, outputChannels,
            NULL);
    gst_caps_append_structure (caps, structure);
    structure = gst_structure_new ("audio/x-raw-float",
            "rate", G_TYPE_INT, outputRate,
            "channels", G_TYPE_INT, outputChannels,
            NULL);
    gst_caps_append_structure (caps, structure);
  }
  else {
    rv = GetRawAudioCaps(&caps);
    if (NS_FAILED (rv))
      goto failed;
  }

  g_object_set (capsfilter, "caps", caps, NULL);
  gst_caps_unref (caps);

  // Now, add to the bin, and then link everything up.
  gst_bin_add_many (bin, audiorate, audioconvert, audioresample,
          capsfilter, audioresample2, NULL);
  gst_element_link_many (audiorate, audioconvert, audioresample,
          capsfilter, audioresample2, NULL);

  last = audioresample2;

  if (encoder) {
    gst_bin_add (bin, encoder);
    gst_element_link (last, encoder);
    last = encoder;
  }

  // Finally, add ghost pads to our bin.
  sinkpad = gst_element_get_static_pad (audiorate, "sink");
  ghostpad = gst_ghost_pad_new ("sink", sinkpad);
  g_object_unref (sinkpad);
  gst_element_add_pad (GST_ELEMENT (bin), ghostpad);

  srcpad = gst_element_get_static_pad (last, "src");
  ghostpad = gst_ghost_pad_new ("src", srcpad);
  g_object_unref (srcpad);
  gst_element_add_pad (GST_ELEMENT (bin), ghostpad);

  // All done!
  *aAudioBin = GST_ELEMENT (bin);

  return NS_OK;

failed:
  if (audiorate)
    g_object_unref (audiorate);
  if (audioconvert)
    g_object_unref (audioconvert);
  if (audioresample)
    g_object_unref (audioresample);
  if (capsfilter)
    g_object_unref (capsfilter);
  if (audioresample2)
    g_object_unref (audioresample2);
  if (encoder)
    g_object_unref (encoder);
  if (bin)
    g_object_unref (bin);

  return rv;
}

nsresult
sbGStreamerVideoTranscoder::AddAudioBin(GstPad *inputAudioSrcPad,
                                        GstPad **outputAudioSrcPad)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER (inputAudioSrcPad);
  NS_ENSURE_ARG_POINTER (outputAudioSrcPad);

  nsresult rv;
  // Ok, we have an audio source. Add the audio processing bin (including
  // encoder, if any).
  GstElement *audioBin = NULL;
  GstCaps *caps = GetCapsFromPad (mAudioSrc);
  rv = BuildAudioBin(caps, &audioBin);
  gst_caps_unref (caps);
  NS_ENSURE_SUCCESS (rv, rv);

  GstPad *audioBinSinkPad = gst_element_get_request_pad(audioBin, "sink");
  GstPad *audioBinSrcPad = gst_element_get_request_pad(audioBin, "src");

  gst_bin_add (GST_BIN (mPipeline), audioBin);
  gst_element_sync_state_with_parent (audioBin);

  GstPadLinkReturn linkret = gst_pad_link (inputAudioSrcPad, audioBinSinkPad);
  if (linkret != GST_PAD_LINK_OK) {
    TranscodingFatalError("songbird.transcode.error.audio_incompatible");
    rv = NS_ERROR_FAILURE;
    goto failed;
  }

  g_object_unref (audioBinSinkPad);

  // Provide the src pad of our bin to the caller.
  *outputAudioSrcPad = audioBinSrcPad;

  return NS_OK;

failed:
  g_object_unref (audioBinSinkPad);
  g_object_unref (audioBinSrcPad);

  return rv;
}

/* Add a video bin.
 * Link the resulting bin to inputVideoSrcPad. Return the src pad of the
 * new bin in outputVideoSrcPad
 */
nsresult
sbGStreamerVideoTranscoder::AddVideoBin(GstPad *inputVideoSrcPad,
                                        GstPad **outputVideoSrcPad)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER (inputVideoSrcPad);
  NS_ENSURE_ARG_POINTER (outputVideoSrcPad);

  nsresult rv;

  // Ok, we have a video source, so add the video processing bin (usually
  // including an encoder).
  GstElement *videoBin = NULL;
  GstCaps *caps = GetCapsFromPad (mVideoSrc);
  rv = BuildVideoBin(caps, &videoBin);
  gst_caps_unref (caps);
  NS_ENSURE_SUCCESS (rv, rv);

  GstPad *videoBinSinkPad = gst_element_get_request_pad(videoBin, "sink");
  GstPad *videoBinSrcPad = gst_element_get_request_pad(videoBin, "src");

  gst_bin_add (GST_BIN (mPipeline), videoBin);
  gst_element_sync_state_with_parent (videoBin);

  GstPadLinkReturn linkret = gst_pad_link (inputVideoSrcPad, videoBinSinkPad);
  if (linkret != GST_PAD_LINK_OK) {
    TranscodingFatalError("songbird.transcode.error.video_incompatible");
    rv = NS_ERROR_FAILURE;
    goto failed;
  }

  g_object_unref (videoBinSinkPad);

  // Provide the src pad of our bin to the caller.
  *outputVideoSrcPad = videoBinSrcPad;

  return NS_OK;

failed:
  g_object_unref (videoBinSinkPad);
  g_object_unref (videoBinSrcPad);

  return rv;
}

GstPad *
sbGStreamerVideoTranscoder::GetPadFromTemplate (GstElement *element,
                                                GstPadTemplate *templ)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstPadPresence presence = GST_PAD_TEMPLATE_PRESENCE (templ);

  if (presence == GST_PAD_ALWAYS || presence == GST_PAD_SOMETIMES)
  {
    return gst_element_get_static_pad (element, templ->name_template);
  }
  else {
    // Request pad
    return gst_element_get_request_pad (element, templ->name_template);
  }
}

/* Get a pad from 'element' that is compatible with (can potentially be linked
 * to) 'pad'.
 * Returned pad may be an existing static pad, or a new request pad
 */
GstPad *
sbGStreamerVideoTranscoder::GetCompatiblePad (GstElement *element,
                                              GstPad *pad)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstElementClass *klass = GST_ELEMENT_GET_CLASS (element);
  GList *padlist = gst_element_class_get_pad_template_list (klass);
  GstPadTemplate *compatibleTemplate = NULL;

  while (padlist) {
    GstPadTemplate *padtempl = (GstPadTemplate *) padlist->data;

    // Check that pad's direction is opposite this template's direction.
    // Then check that they have potentially-compatible caps.
    if (GST_PAD_DIRECTION (pad) != padtempl->direction) {
      GstCaps *caps = gst_pad_query_caps(pad, NULL);

      gboolean compatible = gst_caps_can_intersect (
          caps, GST_PAD_TEMPLATE_CAPS (padtempl));
      gst_caps_unref (caps);

      if (compatible) {
        compatibleTemplate = padtempl;
        break;
      }
    }

    padlist = g_list_next (padlist);
  }

  if (compatibleTemplate) {
    // See if we can get an actual pad based on the compatible template
    return GetPadFromTemplate (element, compatibleTemplate);
  }
  else {
    // No compatible template means there couldn't possibly be a compatible
    // pad.
    return NULL;
  }
}

// Returns the muxer's source pad, or the audio or video encoder's source pad
// if there is no muxer, in 'muxerSrcPad'. Link up either or both of audioPad,
// videoPad.
nsresult
sbGStreamerVideoTranscoder::AddMuxer (GstPad **muxerSrcPad,
                                      GstPad *audioPad,
                                      GstPad *videoPad)
{
  TRACE(("%s[%p]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER (muxerSrcPad);
  NS_ASSERTION (audioPad || videoPad, "Must have at least one pad");

  // Ask the configurator what muxer (if any) we should use.
  nsString muxerName;
  nsresult rv = mConfigurator->GetMuxer(muxerName);
  NS_ENSURE_SUCCESS (rv, rv);

  GstElement *muxer = NULL;

  if (muxerName.IsEmpty()) {
    LOG(("Muxer enabled but no muxer specified"));
    return NS_ERROR_FAILURE;
  }

  muxer = gst_element_factory_make (
          NS_ConvertUTF16toUTF8 (muxerName).BeginReading(), NULL);

  if (!muxer) {
    LOG(("No muxer %s available",
          NS_ConvertUTF16toUTF8 (muxerName).BeginReading()));
    TranscodingFatalError("songbird.transcode.error.muxer_unavailable");
    return NS_ERROR_FAILURE;
  }

  GstPad *sinkpad;

  // Muxer, hook it up!
  gst_bin_add (GST_BIN (mPipeline), muxer);

  if (audioPad) {
    sinkpad = GetCompatiblePad (muxer, audioPad);
    if (!sinkpad) {
      TranscodingFatalError("songbird.transcode.error.audio_not_muxable");
      return NS_ERROR_FAILURE;
    }

    GstPadLinkReturn linkret = gst_pad_link (audioPad, sinkpad);
    if (linkret != GST_PAD_LINK_OK) {
      g_object_unref (sinkpad);
      TranscodingFatalError("songbird.transcode.error.audio_not_muxable");
      return NS_ERROR_FAILURE;
    }

    g_object_unref (sinkpad);
  }

  if (videoPad) {
    sinkpad = GetCompatiblePad (muxer, videoPad);
    if (!sinkpad) {
      TranscodingFatalError("songbird.transcode.error.video_not_muxable");
      return NS_ERROR_FAILURE;
    }

    GstPadLinkReturn linkret = gst_pad_link (videoPad, sinkpad);
    if (linkret != GST_PAD_LINK_OK) {
      g_object_unref (sinkpad);
      TranscodingFatalError("songbird.transcode.error.video_not_muxable");
      return NS_ERROR_FAILURE;
    }

    g_object_unref (sinkpad);
  }

  gst_element_sync_state_with_parent (muxer);

  // Get the output of the muxer as our source pad.
  *muxerSrcPad = gst_element_get_static_pad (muxer, "src");

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::CreateSink (GstElement **aSink)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstElement *sink = NULL;

  if (mDestStream) {
    sink = gst_element_factory_make ("mozillasink", "sink");
    if (sink) {
      // Set the 'stream' property to the raw nsIOutputStream pointer.
      g_object_set (sink, "stream", mDestStream.get(), NULL);
    }
  }
  else if (!mDestURI.IsEmpty()) {
    nsCString uri = NS_ConvertUTF16toUTF8 (mDestURI);
    sink = gst_element_make_from_uri (GST_URI_SINK,
                                      uri.BeginReading(),
                                      "sink",
                                      NULL);
  }

  if (!sink) {
    TranscodingFatalError("songbird.transcode.error.no_sink");
    return NS_ERROR_FAILURE;
  }

  *aSink = sink;
  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::AddSink (GstPad *muxerSrcPad)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  GstElement *sink = NULL;
  nsresult rv = CreateSink(&sink);
  NS_ENSURE_SUCCESS (rv, rv);

  gst_bin_add (GST_BIN (mPipeline), sink);
  gst_element_sync_state_with_parent (sink);

  GstPad *sinkpad = gst_element_get_static_pad (sink, "sink");

  GstPadLinkReturn linkret = gst_pad_link (muxerSrcPad, sinkpad);
  if (linkret != GST_PAD_LINK_OK) {
    TranscodingFatalError("songbird.transcode.error.sink_incompatible");
    return NS_ERROR_FAILURE;
  }

  g_object_unref (sinkpad);

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::SetVideoFormatFromCaps (
        sbIMediaFormatVideoMutable *format, GstCaps *caps)
{
  nsresult rv;

  GstStructure *structure = gst_caps_get_structure (caps, 0);
  gboolean success;

  gint width, height;

  success = gst_structure_get_int (structure, "width", &width);
  NS_ENSURE_TRUE (success, NS_ERROR_FAILURE);
  success = gst_structure_get_int (structure, "height", &height);
  NS_ENSURE_TRUE (success, NS_ERROR_FAILURE);

  const GValue* par = gst_structure_get_value(structure,
          "pixel-aspect-ratio");
  gint parN, parD;
  if (par) {
    parN = gst_value_get_fraction_numerator(par);
    parD = gst_value_get_fraction_denominator(par);
  }
  else {
    // Default to square pixels.
    parN = 1;
    parD = 1;
  }

  const GValue* fr = gst_structure_get_value(structure, "framerate");
  gint frN, frD;
  if (fr) {
    frN = gst_value_get_fraction_numerator(fr);
    frD = gst_value_get_fraction_denominator(fr);
  }
  else {
    // Default to 0/1 indicating unknown?
    frN = 0;
    frD = 1;
  }

  // TODO: should we describe the sub-type of raw video (i.e. YUV 4:2:0 or
  // even more specifically stuff like "I420")?
  rv = format->SetVideoType (NS_LITERAL_STRING ("video/x-raw"));
  NS_ENSURE_SUCCESS (rv, rv);
  rv = format->SetVideoWidth (width);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = format->SetVideoHeight (height);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = format->SetVideoPAR(parN, parD);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = format->SetVideoFrameRate(frN, frD);
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::SetAudioFormatFromCaps (
        sbIMediaFormatAudioMutable *format, GstCaps *caps)
{
  nsresult rv;
  GstStructure *structure = gst_caps_get_structure (caps, 0);
  gboolean success;

  gint rate;
  gint channels;

  success = gst_structure_get_int (structure, "rate", &rate);
  NS_ENSURE_TRUE (success, NS_ERROR_FAILURE);
  success = gst_structure_get_int (structure, "channels", &channels);
  NS_ENSURE_TRUE (success, NS_ERROR_FAILURE);

  // TODO: should we describe the sub-type of raw audio (i.e. integer, float,
  // and details like depth, endianness)
  rv = format->SetAudioType(NS_LITERAL_STRING ("audio/x-raw"));
  NS_ENSURE_SUCCESS (rv, rv);
  rv = format->SetSampleRate (rate);
  NS_ENSURE_SUCCESS (rv, rv);
  rv = format->SetChannels (channels);
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::InitializeConfigurator()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;

  // Build sbIMediaFormat describing the decoded data.
  nsCOMPtr<sbIMediaFormatMutable> mediaFormat =
      do_CreateInstance(SB_MEDIAFORMAT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  if (mVideoSrc) {
    nsCOMPtr<sbIMediaFormatVideoMutable> videoFormat =
        do_CreateInstance(SB_MEDIAFORMATVIDEO_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS (rv, rv);

    GstCaps *videoCaps = GetCapsFromPad (mVideoSrc);
    NS_ENSURE_TRUE (videoCaps, NS_ERROR_FAILURE);

    rv = SetVideoFormatFromCaps(videoFormat, videoCaps);
    gst_caps_unref (videoCaps);
    NS_ENSURE_SUCCESS (rv, rv);

    rv = mediaFormat->SetVideoStream(videoFormat);
    NS_ENSURE_SUCCESS (rv, rv);
  }


  if (mAudioSrc) {
    nsCOMPtr<sbIMediaFormatAudioMutable> audioFormat =
        do_CreateInstance(SB_MEDIAFORMATAUDIO_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS (rv, rv);

    GstCaps *audioCaps = GetCapsFromPad (mAudioSrc);
    NS_ENSURE_TRUE (audioCaps, NS_ERROR_FAILURE);

    rv = SetAudioFormatFromCaps(audioFormat, audioCaps);
    gst_caps_unref (audioCaps);
    NS_ENSURE_SUCCESS (rv, rv);

    rv = mediaFormat->SetAudioStream(audioFormat);
    NS_ENSURE_SUCCESS (rv, rv);
  }

  rv = mConfigurator->SetInputFormat(mediaFormat);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = mConfigurator->Configurate();
  if (NS_FAILED (rv)) {
    TranscodingFatalError("songbird.transcode.error.failed_configuration");
    NS_ENSURE_SUCCESS (rv, rv);
  }

  // Ensure that the proper file extension is being used for the destintation
  // output file (if it's a file)
  if (!mDestURI.IsEmpty()) {
    nsCOMPtr<nsIURI> fixedDestURI;
    rv = NS_NewURI(getter_AddRefs(fixedDestURI),
                   NS_ConvertUTF16toUTF8(mDestURI));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFileURL> fixedDestFileURI = do_QueryInterface(
            fixedDestURI, &rv);
    if (NS_SUCCEEDED(rv) && fixedDestFileURI) {
      nsCString curFileExt;
      rv = fixedDestFileURI->GetFileExtension(curFileExt);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCString configFileExt;
      rv = mConfigurator->GetFileExtension(configFileExt);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!curFileExt.Equals(configFileExt, CaseInsensitiveCompare)) {
        rv = fixedDestFileURI->SetFileExtension(configFileExt);
        NS_ENSURE_SUCCESS(rv, rv);

        // Check if the destination file already exists
        nsCOMPtr<nsIFile> destFile;
        rv = fixedDestFileURI->GetFile(getter_AddRefs(destFile));

        PRBool exists;
        rv = destFile->Exists(&exists);
        NS_ENSURE_SUCCESS(rv, rv);

        // Create a unique file if destination file already exists.
        if (exists) {
          rv = destFile->CreateUnique(nsIFile::NORMAL_FILE_TYPE,
                                      SB_DEFAULT_FILE_PERMISSIONS);
          NS_ENSURE_SUCCESS(rv, rv);
          rv = NS_NewFileURI(getter_AddRefs(fixedDestURI), destFile);
          NS_ENSURE_SUCCESS(rv, rv);
          fixedDestFileURI = do_QueryInterface(fixedDestURI, &rv);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        nsCString fixedSpec;
        rv = fixedDestFileURI->GetSpec(fixedSpec);
        NS_ENSURE_SUCCESS(rv, rv);

        CopyUTF8toUTF16(fixedSpec, mDestURI);
      }
    }
  }

  /* Determine whether we want audio/video/muxer. Whether we _actually_
     use these also depends on what the input file has - but we won't
     try to set up a video bin if there's no video encoder */
  nsString audioEncoder;
  rv = mConfigurator->GetAudioEncoder(audioEncoder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mConfigurator->GetUseAudioEncoder(&mUseAudio);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString videoEncoder;
  rv = mConfigurator->GetVideoEncoder(videoEncoder);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mConfigurator->GetUseVideoEncoder(&mUseVideo);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString muxer;
  rv = mConfigurator->GetMuxer(muxer);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mConfigurator->GetUseMuxer(&mUseMuxer);
  NS_ENSURE_SUCCESS(rv, rv);

  /* If we're not using a muxer, we must have ONLY one codec */
  if (!mUseMuxer && (mUseAudio && mUseVideo))
    return NS_ERROR_UNEXPECTED;

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::DecoderNoMorePads(GstElement *uridecodebin)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  if (!mAudioSrc && !mVideoSrc) {
    // We have neither audio nor video. That's not very good!
    TranscodingFatalError("songbird.transcode.error.no_streams");
    return NS_ERROR_FAILURE;
  }

  // Now, set up queues, and pad blocks on the queues. Get notification of
  // caps being set on each src pad (the original srcpad from decodebin, NOT
  // the queue's srcpad). When we have caps on all streams, configure the
  // rest of the pipeline and release the pad blocks.

  if (mAudioSrc) {
    g_signal_connect (mAudioSrc, "notify::caps",
            G_CALLBACK (pad_notify_caps_cb), this);

    GstElement *queue = gst_element_factory_make ("queue", "audio-queue");
    GstPad *queueSink = gst_element_get_request_pad(queue, "sink");

    gst_bin_add (GST_BIN (mPipeline), queue);
    gst_element_sync_state_with_parent (queue);

    gst_pad_link (mAudioSrc, queueSink);
    g_object_unref (queueSink);

    GstPad *queueSrc = gst_element_get_static_pad(queue, "src");
    mAudioQueueSrc = queueSrc;

    gst_pad_add_probe(queueSrc, GST_PAD_PROBE_TYPE_BLOCK,
                      (GstPadProbeCallback) pad_blocked_audio_cb, this, NULL);
  }

  if (mVideoSrc) {
    g_signal_connect (mVideoSrc, "notify::caps",
            G_CALLBACK (pad_notify_caps_cb), this);

    GstElement *queue = gst_element_factory_make ("queue", "video-queue");
    GstPad *queueSink = gst_element_get_request_pad(queue, "sink");

    gst_bin_add (GST_BIN (mPipeline), queue);
    gst_element_sync_state_with_parent (queue);

    gst_pad_link (mVideoSrc, queueSink);
    g_object_unref (queueSink);

    GstPad *queueSrc = gst_element_get_static_pad(queue, "src");
    mVideoQueueSrc = queueSrc;

    gst_pad_add_probe(queueSrc, GST_PAD_PROBE_TYPE_BLOCK,
                      (GstPadProbeCallback) pad_blocked_video_cb, this, NULL);
  }
  mWaitingForCaps = PR_TRUE;

  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::PadNotifyCaps (GstPad *pad)
{
  return CheckForAllCaps();
}


GstCaps *
sbGStreamerVideoTranscoder::GetCapsFromPad (GstPad *pad)
{
  // TODO: Check if this is still relevant when using gst_pad_get_current_caps
  // We want to get the caps from the decoder associated with this pad (but this
  // pad might be a ghost pad, or a queue pad linked to the ghost pad, etc)
  // gst_pad_get_negotiated_caps() is needed. gst_pad_get_caps would fail in 
  // in some cases (wmv) even with the fallback to GST_PAD_CAPS.
  // We still fallback and try GST_PAD_CAPS, which works in some of these cases,
  // if the get_caps returned us some non-fixed caps (usually these will be
  // template caps). The use of gst_pad_get_negotiated_caps was thought of
  // mainly because the media inspector uses it and that particular logic
  // is doing the same thing as the transcoder, getting caps for the stream.

  GstPad * realPad = GetRealPad(pad);
  GstCaps *caps = gst_pad_get_current_caps(realPad);
  if (caps) {
    if (gst_caps_is_fixed (caps)) {
      g_object_unref(realPad);
      return caps;
    }
    gst_caps_unref (caps);
  }

  caps = gst_pad_get_current_caps(realPad);
  if (caps) {
    gst_caps_ref (caps);
    g_object_unref(realPad);
    return caps;
  }
  g_object_unref(realPad);
  return NULL;
}

nsresult
sbGStreamerVideoTranscoder::CheckForAllCaps ()
{
  // Ensure this isn't called from multiple threads concurrently.
  nsAutoLock lock(mBuildLock);
  nsresult rv = NS_OK;

  if (mWaitingForCaps) {
    TRACE(("CheckForAllCaps: checking if we have fixed caps on all pads"));
    if (mAudioSrc) {
      GstCaps *audioCaps = GetCapsFromPad (mAudioSrc);
      if (!audioCaps) {
        // Not done yet
        return NS_OK;
      }
      gst_caps_unref (audioCaps);
    }

    if (mVideoSrc) {
      GstCaps *videoCaps = GetCapsFromPad (mVideoSrc);
      if (!videoCaps) {
        // Not done yet
        return NS_OK;
      }
      gst_caps_unref (videoCaps);
    }

    LOG(("Have fixed caps on all pads: proceeding to build pipeline"));

    // Ok, we have caps for everything.
    // Build the rest of the pipeline, and unblock the pads.
    rv = BuildRemainderOfPipeline();

    // success or failure, we're done waiting for caps
    mWaitingForCaps = PR_FALSE;

    if (NS_SUCCEEDED (rv)) {
      if (mAudioQueueSrc) {
        if (gst_pad_is_blocked(mAudioQueueSrc)) {
          gst_pad_remove_probe(mAudioQueueSrc, mAudioQueueSrcInfo->id);
        }
      }
      if (mVideoQueueSrc) {
        if (gst_pad_is_blocked(mVideoQueueSrc)) {
          gst_pad_remove_probe(mVideoQueueSrc, mVideoQueueSrcInfo->id);
        }
      }
    } else {
      // handle NS_FAILED(rv)
      // Set status so progress reporting shows we failed
      mStatus = sbIJobProgress::STATUS_FAILED;

      // unlock before making the proxied error dispatch
      lock.unlock();
      TranscodingFatalError("songbird.transcode.error.failed_configuration");
      lock.lock();
    }

    // Done with all building, clean these up.
    CleanupPads();
  }

  return rv;
}

nsresult
sbGStreamerVideoTranscoder::PadBlocked (GstPad *pad)
{
  if (gst_pad_is_blocked(pad)) {
    LOG(("Pad blocked, checking if we have full caps yet"));
    return CheckForAllCaps();
  }
  else {
    LOG(("PadBlocked: returning NS_OK after unblocking."));
    return NS_OK;
  }
}

nsresult
sbGStreamerVideoTranscoder::BuildRemainderOfPipeline ()
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  nsresult rv;
  // Set up the configurator - if this succeeds, then we have an output
  // format selected, so we can query that as needed to set up our encoders.
  rv = InitializeConfigurator();
  NS_ENSURE_SUCCESS (rv, rv);

  GstPad *newAudioSrc = NULL;
  GstPad *newVideoSrc = NULL;

  if (mAudioQueueSrc && mUseAudio) {
    rv = AddAudioBin (mAudioQueueSrc, &newAudioSrc);
    NS_ENSURE_SUCCESS (rv, rv);
  }

  if (mVideoQueueSrc && mUseVideo) {
    rv = AddVideoBin (mVideoQueueSrc, &newVideoSrc);
    NS_ENSURE_SUCCESS (rv, rv);
  }

  GstPad *srcpad = NULL;

  if (mUseMuxer) {
    rv = AddMuxer (&srcpad, newAudioSrc, newVideoSrc);
    NS_ENSURE_SUCCESS (rv, rv);
  }
  else {
    if (newAudioSrc)
      srcpad = (GstPad *)gst_object_ref (newAudioSrc);
    else if (newVideoSrc)
      srcpad = (GstPad *)gst_object_ref (newVideoSrc);
    else {
      NS_NOTREACHED ("No audio or video, not allowed");
      return NS_ERROR_FAILURE;
    }
  }

  rv = AddSink (srcpad);
  if (NS_FAILED (rv)) {
    NS_ENSURE_SUCCESS (rv, rv);
  }

  g_object_unref (srcpad);
  if (newVideoSrc)
    g_object_unref (newVideoSrc);
  if (newAudioSrc)
    g_object_unref (newAudioSrc);

  // We have the pipeline fully set up; now we can set the metadata.
  rv = SetMetadataOnTagSetters();

  LOG(("Finished building pipeline"));
  return NS_OK;
}

nsresult
sbGStreamerVideoTranscoder::BuildTranscodePipeline(const gchar *aPipelineName)
{
  TRACE(("%s[%p]", __FUNCTION__, this));

  // Our pipeline will look roughly like this. For details, see the functions
  // that build each of the bins.
  // The audio encoder and video encoder bins are optional - they will only
  // exist if the relevant media type is found.
  // The muxer is also optional if only one of audio or video is in use.
  //
  // transcode-decoder:  an instance of uridecodebin.

  // [------------------------------------------------------------------------]
  // [transcode-pipeline      [-----]  [--------------]                       ]
  // [                        [queue]--[audio-encoder ]\  [-----------------] ]
  // [                       /[-----]  [--------------] \ [output-bin       ] ]
  // [ [------------------] /                            \[ [-----]  [----] ] ]
  // [ [transcode-decoder ]/                              [-[muxer]--[sink] ] ]
  // [ [                  ]\                             /[ [-----]  [----] ] ]
  // [ [------------------] \ [-----]  [--------------] / [                 ] ]
  // [                       \[queue]--[video-encoder ]/  [-----------------] ]
  // [                        [-----]  [--------------]                       ]
  // [                                                                        ]
  // [------------------------------------------------------------------------]
  //
  // audiorate:     insert/drop samples as needed to make sure we have a
  //                continuous audio stream with no gaps or overlaps.
  // audioconvert:  convert format of the raw audio for further processing.
  //                Includes down- or up-mixing.
  // ar1:           audioresample used to resample audio to the desired sample
  //                rate.
  // capsfilter:    specify the format to convert to for the encoder to encode.
  // ar2:           audioresample to re-resample to a rate supported by the
  //                encoder. This is basically a hack to handle the fact that
  //                the configurator doesn't know how to handle the constraints
  //                of what the encoder is capable of. We don't similarly have
  //                another audioconvert, or other converters on the video side,
  //                because in practice we don't run into this problem there.
  //
  // [------------------------------------------------------------------------]
  // [audio-encoder                                                           ]
  // [                                                                        ]
  // [ [---------]  [------------]  [---]  [----------] [---]  [-------]      ]
  // [-[audiorate]--[audioconvert]--[ar1]--[capsfilter]-[ar2]--[encoder]------]
  // [ [---------]  [------------]  [---]  [----------] [---]  [-------]      ]
  // [                                                                        ]
  // [------------------------------------------------------------------------]
  //
  // videorate:  adjust framerate of video to a constant rate, by dropping or
  //             duplicating frames as needed.
  // colorspace: convert to a colour space and pixel format that we can process
  //             further and encode.
  // videoscale: scale the video to the size we want in at least one dimension.
  //             The other dimension MAY be smaller than the desired output.
  // videobox:   add black bars at top/bottom or left/right of the actual video
  //             so that the video has BOTH dimensions correct, and the actual
  //             image material itself retains the correct aspect ratio.
  // capsfilter: specify the format to convert to for the encoder to encode.

  // [-------------------------------------------------------------------------]
  // [video-encoder                                                            ]
  // [                                                                         ]
  // [ [---------] [----------] [----------] [--------] [----------] [-------] ]
  // [-[videorate]-[colorspace]-[videoscale]-[videobox]-[capsfilter]-[encoder]-]
  // [ [---------] [----------] [----------] [--------] [----------] [-------] ]
  // [                                                                         ]
  // [-------------------------------------------------------------------------]

  mPipeline = gst_pipeline_new (aPipelineName);
  NS_ENSURE_TRUE (mPipeline, NS_ERROR_FAILURE);

  GstElement *uridecodebin = gst_element_factory_make("uridecodebin",
          "transcode-decoder");
  if (!uridecodebin) {
    g_object_unref (mPipeline);
    mPipeline = NULL;

    return NS_ERROR_FAILURE;
  }

  // Set the source URI
  nsCString uri = NS_ConvertUTF16toUTF8 (mSourceURI);
  g_object_set (uridecodebin, "uri", uri.BeginReading(), NULL);

  // Connect to callbacks for when uridecodebin adds a new pad that we (may)
  // want to connect up to the rest of the pipeline, and for when we're not
  // going to get any more pads (and so can actually continue on to transcoding)
  g_signal_connect (uridecodebin, "pad-added",
          G_CALLBACK (decodebin_pad_added_cb), this);
  g_signal_connect (uridecodebin, "no-more-pads",
          G_CALLBACK (decodebin_no_more_pads_cb), this);

  gst_bin_add (GST_BIN (mPipeline), uridecodebin);

  return NS_OK;
}

