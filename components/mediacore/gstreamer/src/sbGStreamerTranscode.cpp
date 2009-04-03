/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

#include "sbGStreamerTranscode.h"

#include <sbIGStreamerService.h>

#include <sbStringUtils.h>
#include <sbClassInfoUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsStringAPI.h>
#include <prlog.h>

#define PROGRESS_INTERVAL 200 /* milliseconds */

/**
 * To log this class, set the following environment variable in a debug build:
 *  NSPR_LOG_MODULES=sbGStreamerTranscode:5 (or :3 for LOG messages only)
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gGStreamerTranscode = PR_NewLogModule("sbGStreamerTranscode");
#define LOG(args) PR_LOG(gGStreamerTranscode, PR_LOG_WARNING, args)
#define TRACE(args) PR_LOG(gGStreamerTranscode, PR_LOG_DEBUG, args)
#else /* PR_LOGGING */
#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */
#endif /* PR_LOGGING */

NS_IMPL_THREADSAFE_ISUPPORTS7(sbGStreamerTranscode,
                              nsIClassInfo,
                              sbIGStreamerPipeline,
                              sbIGStreamerTranscode,
                              sbIMediacoreEventTarget,
                              sbIJobProgress,
                              sbIJobCancelable,
                              nsITimerCallback)

NS_IMPL_CI_INTERFACE_GETTER5(sbGStreamerTranscode,
                             sbIGStreamerPipeline,
                             sbIGStreamerTranscode,
                             sbIMediacoreEventTarget,
                             sbIJobProgress,
                             sbIJobCancelable)

NS_DECL_CLASSINFO(sbGStreamerTranscode);
NS_IMPL_THREADSAFE_CI(sbGStreamerTranscode);

sbGStreamerTranscode::sbGStreamerTranscode() :
  sbGStreamerPipeline(),
  mStatus(sbIJobProgress::STATUS_RUNNING) // There is no NOT_STARTED
{
}

sbGStreamerTranscode::~sbGStreamerTranscode()
{
}

/* nsITimerCallback interface implementation */

NS_IMETHODIMP 
sbGStreamerTranscode::Notify(nsITimer *aTimer)
{
  NS_ENSURE_ARG_POINTER(aTimer);

  OnJobProgress();

  return NS_OK;
}

/* sbIGStreamerTranscode interface implementation */

NS_IMETHODIMP
sbGStreamerTranscode::SetSourceURI(const nsAString& aSourceURI)
{
  mSourceURI = aSourceURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetSourceURI(nsAString& aSourceURI)
{
  aSourceURI = mSourceURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::SetDestURI(const nsAString& aDestURI)
{
  mDestURI = aDestURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetDestURI(nsAString& aDestURI)
{
  aDestURI = mDestURI;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::SetTranscodePipeline(
        const nsAString& aPipelineDescription)
{
  mPipelineDescription = aPipelineDescription;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetTranscodePipeline(nsAString& aPipelineDescription)
{
  aPipelineDescription = mPipelineDescription;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetMetadata(sbIPropertyArray **aMetadata)
{
  NS_ENSURE_ARG_POINTER(aMetadata);

  *aMetadata = mMetadata;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::SetMetadata(sbIPropertyArray *aMetadata)
{
  mMetadata = aMetadata;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::BuildPipeline()
{
  nsCString pipelineString;
  //GstParseContext *ctx; // Requires a newer version of gstreamer... enable
                          // once we update.
  GError *error = NULL;
  nsresult rv;
  GstTagList *tags;

  pipelineString = BuildPipelineString();
  if (pipelineString.IsEmpty())
    return NS_ERROR_FAILURE;

  //ctx = gst_parse_context_new();
  //mPipeline = gst_parse_launch_full (pipelineString.BeginReading(), ctx, 
  //        GST_PARSE_FLAG_FATAL_ERRORS, &error);
  mPipeline = gst_parse_launch (pipelineString.BeginReading(), &error);

  if (!mPipeline) {
    // TODO: Report the error more usefully using the GError, and perhaps
    // the GstParseContext.
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  tags = ConvertPropertyArrayToTagList(mMetadata);

  if (tags) {
    // Find all the tag setters in the pipeline
    GstIterator *it = gst_bin_iterate_all_by_interface (
            (GstBin *)mPipeline, GST_TYPE_TAG_SETTER);
    GstElement *element;

    while (gst_iterator_next (it, (void **)&element) == GST_ITERATOR_OK) {
      GstTagSetter *setter = GST_TAG_SETTER (element);

      /* Use MERGE_REPLACE: preserves existing tag where we don't have one
       * in our taglist */
      gst_tag_setter_merge_tags (setter, tags, GST_TAG_MERGE_REPLACE);
      g_object_unref (element);
    }
    gst_iterator_free (it);
    gst_tag_list_free (tags);
  }

  rv = NS_OK;
done:
  //gst_parse_context_free (ctx);

  return rv;
}

// Override base class to start the progress reporter
NS_IMETHODIMP
sbGStreamerTranscode::PlayPipeline()
{
  nsresult rv;

  rv = sbGStreamerPipeline::PlayPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  rv = StartProgressReporting();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

// Override base class to stop the progress reporter
NS_IMETHODIMP
sbGStreamerTranscode::StopPipeline()
{
  nsresult rv;

  rv = sbGStreamerPipeline::StopPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  rv = StopProgressReporting();
  NS_ENSURE_SUCCESS (rv, rv);

  // Inform listeners of new job status
  rv = OnJobProgress();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

/* sbIJobCancelable interface implementation */

NS_IMETHODIMP
sbGStreamerTranscode::GetCanCancel(PRBool *aCanCancel)
{
  NS_ENSURE_ARG_POINTER(aCanCancel);

  *aCanCancel = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::Cancel()
{
  mStatus = sbIJobProgress::STATUS_FAILED; // We don't have a 'cancelled' state.

  nsresult rv = StopPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  return NS_OK;
}

/* sbIJobProgress interface implementation */

NS_IMETHODIMP
sbGStreamerTranscode::GetStatus(PRUint16 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);

  *aStatus = mStatus;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetStatusText(nsAString& aText)
{
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
sbGStreamerTranscode::GetTitleText(nsAString& aText)
{
  return SBGetLocalizedString(aText,
          NS_LITERAL_STRING("mediacore.gstreamer.transcode.title"));
}

NS_IMETHODIMP 
sbGStreamerTranscode::GetProgress(PRUint32* aProgress)
{
  NS_ENSURE_ARG_POINTER(aProgress);

  GstClockTime duration = QueryDuration();
  GstClockTime position = QueryPosition();

  if (duration != GST_CLOCK_TIME_NONE && position != GST_CLOCK_TIME_NONE && 
          duration != 0)
    *aProgress = (PRUint32)gst_util_uint64_scale (position, 1000, duration);
  else
    *aProgress = 0; // Unknown 

  return NS_OK;
}

NS_IMETHODIMP 
sbGStreamerTranscode::GetTotal(PRUint32* aTotal)
{
  NS_ENSURE_ARG_POINTER(aTotal);

  GstClockTime duration = QueryDuration();

  // The job progress stuff doesn't like overly large numbers, so we artifically
  // fix it to a max of 1000.

  if (duration != GST_CLOCK_TIME_NONE)
    *aTotal = 1000;
  else
    *aTotal = 0;

  return NS_OK;
}

// We don't do error reporting via sbIJobProgress
NS_IMETHODIMP 
sbGStreamerTranscode::GetErrorCount(PRUint32* aErrorCount)
{
  NS_ENSURE_ARG_POINTER(aErrorCount);

  *aErrorCount = 0;

  return NS_OK;
}

NS_IMETHODIMP 
sbGStreamerTranscode::GetErrorMessages(nsIStringEnumerator** aMessages)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbGStreamerTranscode::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbGStreamerTranscode::AddJobProgressListener is main thread only!");

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
sbGStreamerTranscode::RemoveJobProgressListener(sbIJobProgressListener* aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbGStreamerTranscode::RemoveJobProgressListener is main thread only!");

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
sbGStreamerTranscode::OnJobProgress()
{
  TRACE(("sbGStreamerTranscode::OnJobProgress[0x%.8x]", this));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbGStreamerTranscode::OnJobProgress is main thread only!");

  // Announce our status to the world
  for (PRInt32 i = mProgressListeners.Count() - 1; i >= 0; --i) {
     // Ignore any errors from listeners 
     mProgressListeners[i]->OnJobProgress(this);
  }
  return NS_OK;
}

void sbGStreamerTranscode::HandleErrorMessage(GstMessage *message)
{
  mStatus = sbIJobProgress::STATUS_FAILED;

  // This will stop the pipeline and update listeners
  sbGStreamerPipeline::HandleErrorMessage (message);
}

void sbGStreamerTranscode::HandleEOSMessage(GstMessage *message)
{
  TRACE(("sbGStreamerTranscode::HandleEOSMessage[0x%.8x]", this));

  mStatus = sbIJobProgress::STATUS_SUCCEEDED;

  // This will stop the pipeline and update listeners
  sbGStreamerPipeline::HandleEOSMessage (message);
}

GstClockTime sbGStreamerTranscode::QueryPosition()
{
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

GstClockTime sbGStreamerTranscode::QueryDuration()
{
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
sbGStreamerTranscode::StartProgressReporting()
{
  NS_ENSURE_STATE(!mProgressTimer);
  TRACE(("sbGStreamerTranscode::StartProgressReporting[0x%.8x]", this));

  nsresult rv;
  mProgressTimer =
              do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mProgressTimer->InitWithCallback(this, 
          PROGRESS_INTERVAL, nsITimer::TYPE_REPEATING_SLACK);

  return NS_OK;
}

nsresult 
sbGStreamerTranscode::StopProgressReporting()
{
  TRACE(("sbGStreamerTranscode::StopProgressReporting[0x%.8x]", this));

  if (mProgressTimer) {
    mProgressTimer->Cancel();
    mProgressTimer = nsnull;
  }

  return NS_OK;
}

nsCString
sbGStreamerTranscode::BuildPipelineString()
{
  // Build a pipeline description string looking something like:
  //
  //    file:///tmp/test.mp3 ! decodebin ! audioconvert ! audioresample ! 
  //      vorbisenc ! oggmux ! file://tmp/test.ogg
  //
  // The URIs come from mSourceURI, mDestURI, the 'vorbisenc ! oggmux' bit
  // from the pipeline string.
  // We may add a configurable capsfilter later, but this might be enough for
  // the moment...

  nsCString pipeline = NS_ConvertUTF16toUTF8(mSourceURI);
  pipeline.AppendLiteral(" ! decodebin ! audioconvert ! audioresample ! ");
  pipeline.Append(NS_ConvertUTF16toUTF8(mPipelineDescription));
  pipeline.AppendLiteral(" ! ");
  pipeline.Append(NS_ConvertUTF16toUTF8(mDestURI));

  LOG(("Built pipeline string: '%s'", pipeline.BeginReading()));

  return pipeline;
}

