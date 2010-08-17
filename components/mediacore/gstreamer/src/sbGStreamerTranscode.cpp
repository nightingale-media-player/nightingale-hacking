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
#include <sbTArrayStringEnumerator.h>
#include <sbMemoryUtils.h>

#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <nsStringAPI.h>
#include <nsArrayUtils.h>
#include <nsNetUtil.h>

#include <nsIFile.h>
#include <nsIURI.h>
#include <nsIFileURL.h>
#include <nsIBinaryInputStream.h>

#include <gst/tag/tag.h>

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

NS_IMPL_THREADSAFE_ISUPPORTS8(sbGStreamerTranscode,
                              nsIClassInfo,
                              sbIGStreamerPipeline,
                              sbITranscodeJob,
                              sbIMediacoreEventTarget,
                              sbIJobProgress,
                              sbIJobProgressTime,
                              sbIJobCancelable,
                              nsITimerCallback)

NS_IMPL_CI_INTERFACE_GETTER6(sbGStreamerTranscode,
                             sbIGStreamerPipeline,
                             sbITranscodeJob,
                             sbIMediacoreEventTarget,
                             sbIJobProgress,
                             sbIJobProgressTime,
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

/* sbITranscodeJob interface implementation */

NS_IMETHODIMP
sbGStreamerTranscode::SetSourceURI(const nsAString& aSourceURI)
{
  mSourceURI = aSourceURI;

  // Use the source URI as the resource name too.
  mResourceDisplayName = mSourceURI;
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
sbGStreamerTranscode::SetProfile(sbITranscodeProfile *aProfile)
{
  NS_ENSURE_ARG_POINTER(aProfile);

  mProfile = aProfile;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetProfile(sbITranscodeProfile **aProfile)
{
  NS_ENSURE_ARG_POINTER(aProfile);

  NS_IF_ADDREF(*aProfile = mProfile);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetMetadata(sbIPropertyArray **aMetadata)
{
  NS_ENSURE_ARG_POINTER(aMetadata);

  NS_IF_ADDREF(*aMetadata = mMetadata);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::SetMetadata(sbIPropertyArray *aMetadata)
{
  mMetadata = aMetadata;
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetMetadataImage(nsIInputStream **aImageStream)
{
  NS_ENSURE_ARG_POINTER(aImageStream);

  NS_IF_ADDREF(*aImageStream = mImageStream);
  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::SetMetadataImage(nsIInputStream *aImageStream)
{
  mImageStream = aImageStream;
  return NS_OK;
}

GstElement *
sbGStreamerTranscode::BuildTranscodePipeline(sbITranscodeProfile *aProfile)
{
  nsCString pipelineString;
  nsCString pipelineDescription;
  GError *error = NULL;
  GstElement *pipeline;
  nsresult rv;

  rv = BuildPipelineFragmentFromProfile(aProfile, pipelineDescription);
  NS_ENSURE_SUCCESS (rv, NULL);

  rv = BuildPipelineString(pipelineDescription, pipelineString);
  NS_ENSURE_SUCCESS (rv, NULL);

  //ctx = gst_parse_context_new();
  //mPipeline = gst_parse_launch_full (pipelineString.BeginReading(), ctx, 
  //        GST_PARSE_FLAG_FATAL_ERRORS, &error);
  pipeline = gst_parse_launch (pipelineString.BeginReading(), &error);

  return pipeline;
}

nsresult
sbGStreamerTranscode::AddImageToTagList(GstTagList *aTags,
        nsIInputStream *aStream)
{
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

  GstBuffer *imagebuf = gst_tag_image_data_to_image_buffer (
          imageData, imageDataLen, GST_TAG_IMAGE_TYPE_FRONT_COVER);
  if (!imagebuf)
    return NS_ERROR_FAILURE;

  gst_tag_list_add (aTags, GST_TAG_MERGE_REPLACE, GST_TAG_IMAGE,
          imagebuf, NULL);
  gst_buffer_unref (imagebuf);

  return NS_OK;
}

nsresult
sbGStreamerTranscode::BuildPipeline()
{
  NS_ENSURE_STATE (mProfile);

  nsresult rv;
  GstTagList *tags;

  mPipeline = BuildTranscodePipeline(mProfile);

  if (!mPipeline) {
    // TODO: Report the error more usefully using the GError, and perhaps
    // the GstParseContext.
    rv = NS_ERROR_FAILURE;
    goto done;
  }

  // We have a pipeline, set it's main operation to transcoding.   
  SetPipelineOp(GStreamer::OP_TRANSCODING);

  tags = ConvertPropertyArrayToTagList(mMetadata);

  if (mImageStream) {
    // Ignore return value here, failure is not critical.
    AddImageToTagList (tags, mImageStream);
  }

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

NS_IMETHODIMP
sbGStreamerTranscode::Vote(sbIMediaItem *aMediaItem,
                           sbITranscodeProfile *aProfile, PRInt32 *aVote)
{
  NS_ENSURE_ARG_POINTER(aVote);

  GstElement *pipeline = BuildTranscodePipeline(aProfile);
  if (!pipeline) {
    /* Couldn't create a pipeline for this profile; do not accept it */
    *aVote = -1;
  }
  else {
    /* For now just vote 1 for anything we can handle */
    gst_object_unref (pipeline);
    *aVote = 1;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::Transcode()
{
  return PlayPipeline();
}

// Override base class to start the progress reporter
nsresult
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
nsresult
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
sbGStreamerTranscode::GetElapsedTime(PRUint32 *aElapsedTime)
{
  NS_ENSURE_ARG_POINTER(aElapsedTime);

  /* Get the running time, and convert to milliseconds */
  *aElapsedTime = static_cast<PRUint32>(GetRunningTime() / GST_MSECOND);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetRemainingTime(PRUint32 *aRemainingTime)
{
  GstClockTime duration = QueryDuration();
  GstClockTime position = QueryPosition();
  GstClockTime elapsed = GetRunningTime();

  if (duration == GST_CLOCK_TIME_NONE || position == GST_CLOCK_TIME_NONE ||
      elapsed == GST_CLOCK_TIME_NONE)
  {
    /* Unknown, so set to -1 */
    *aRemainingTime = -1;
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
sbGStreamerTranscode::GetStatus(PRUint16 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);

  *aStatus = mStatus;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetBlocked(PRBool *aBlocked)
{
  NS_ENSURE_ARG_POINTER(aBlocked);

  *aBlocked = PR_FALSE;

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

// Note that you can also get errors reported via the mediacore listener
// interfaces.
NS_IMETHODIMP 
sbGStreamerTranscode::GetErrorCount(PRUint32* aErrorCount)
{
  NS_ENSURE_ARG_POINTER(aErrorCount);
  NS_ASSERTION(NS_IsMainThread(), 
          "sbIJobProgress::GetErrorCount is main thread only!");

  *aErrorCount = mErrorMessages.Length();

  return NS_OK;
}

NS_IMETHODIMP 
sbGStreamerTranscode::GetErrorMessages(nsIStringEnumerator** aMessages)
{
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
sbGStreamerTranscode::RemoveJobProgressListener(
        sbIJobProgressListener* aListener)
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
  GError *gerror = NULL;
  gchar *debug = NULL;

  mStatus = sbIJobProgress::STATUS_FAILED;

  gst_message_parse_error(message, &gerror, &debug);

  mErrorMessages.AppendElement(
      NS_ConvertUTF8toUTF16(nsDependentCString(gerror->message)));

  g_error_free (gerror);
  g_free(debug);

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

nsresult
sbGStreamerTranscode::BuildPipelineString(nsCString pipelineDescription,
                                          nsACString &pipeline)
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

  pipeline.Append(NS_ConvertUTF16toUTF8(mSourceURI));
  pipeline.AppendLiteral(" ! decodebin ! audioconvert ! audioresample ! ");
  pipeline.Append(pipelineDescription);
  pipeline.AppendLiteral(" ! ");
  pipeline.Append(NS_ConvertUTF16toUTF8(mDestURI));

  LOG(("Built pipeline string: '%s'", pipeline.BeginReading()));

  return NS_OK;
}

nsresult
sbGStreamerTranscode::BuildPipelineFragmentFromProfile(
        sbITranscodeProfile *aProfile, nsACString &pipelineFragment)
{
  NS_ENSURE_ARG_POINTER(aProfile);

  nsresult rv;
  PRUint32 type;
  nsString container;
  nsString audioCodec;
  nsCString gstContainerMuxer;
  nsCString gstAudioEncoder;
  nsCOMPtr<nsIArray> containerProperties;
  nsCOMPtr<nsIArray> audioProperties;

  rv = aProfile->GetType(&type);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = aProfile->GetContainerFormat(container);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = aProfile->GetContainerProperties(getter_AddRefs(containerProperties));
  NS_ENSURE_SUCCESS (rv, rv);

  rv = aProfile->GetAudioCodec(audioCodec);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = aProfile->GetAudioProperties(getter_AddRefs(audioProperties));
  NS_ENSURE_SUCCESS (rv, rv);

  if (type != sbITranscodeProfile::TRANSCODE_TYPE_AUDIO)
    return NS_ERROR_FAILURE;

  /* Each of container and audio codec is optional */
  if (!audioCodec.IsEmpty()) {
    rv = GetAudioCodec(audioCodec, audioProperties, gstAudioEncoder);
    NS_ENSURE_SUCCESS (rv, rv);

    pipelineFragment.Append(gstAudioEncoder);
  }

  if (!container.IsEmpty()) {
    rv = GetContainer(container, containerProperties, gstContainerMuxer);
    NS_ENSURE_SUCCESS (rv, rv);

    pipelineFragment.AppendLiteral(" ! ");
    pipelineFragment.Append(gstContainerMuxer);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerTranscode::EstimateOutputSize(PRInt32 inputDuration,
        PRInt64 *_retval)
{
  /* It's pretty hard to do anything really accurate here.
     Note that we're currently ONLY doing audio transcoding, which simplifies
     things.
     Current approach
      - Calculate bitrate of audio.
         - if there's a bitrate property, assume it's in bits per second, and is
           roughly accurate
         - otherwise, assume we're encoding to a lossless format such as FLAC.
           Typically, those get an average compression ratio of a little better
           than 0.6 (for 44.1kHz 16-bit stereo content). Assume that's what
           we have; this will sometimes be WAY off, but we can't really do
           any better.
      - Calculate size of audio data based on this and the duration
      - Add 5% for container/etc overhead.
   */
  NS_ENSURE_STATE (mProfile);
  NS_ENSURE_ARG_POINTER(_retval);

  PRUint32 bitrate = 0;
  nsresult rv;
  nsCOMPtr<nsIArray> audioProperties;

  rv = mProfile->GetAudioProperties(getter_AddRefs(audioProperties));
  NS_ENSURE_SUCCESS (rv, rv);

  PRUint32 propertiesLength = 0;
  rv = audioProperties->GetLength(&propertiesLength);
  NS_ENSURE_SUCCESS (rv, rv);

  for (PRUint32 j = 0; j < propertiesLength; j++) {
    nsCOMPtr<sbITranscodeProfileProperty> property = 
        do_QueryElementAt(audioProperties, j, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString propName;
    rv = property->GetPropertyName(propName);
    NS_ENSURE_SUCCESS(rv, rv);

    if (propName.EqualsLiteral("bitrate")) {
      nsCOMPtr<nsIVariant> propValue;
      rv = property->GetValue(getter_AddRefs(propValue));
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 bitrate;
      rv = propValue->GetAsUint32(&bitrate);
      NS_ENSURE_SUCCESS(rv, rv);

      break;
    }
  }

  if (bitrate == 0) {
    // No bitrate found; assume something like FLAC as above. This is bad; how
    // can we do better though?
    bitrate = static_cast<PRUint32>(0.6 * (44100 * 2 * 16));
  }

  /* Calculate size from duration (in ms) and bitrate (in bits/second) */
  PRUint64 size = (PRUint64)inputDuration * bitrate / 8 / 1000;

  /* Add 5% for container overhead, etc. */
  size += (size/20);

  *_retval = size;

  return NS_OK;
}

struct GSTNameMap {
  const char *name;
  const char *gstCapsName;
};

static struct GSTNameMap SupportedContainers[] = {
  {"application/ogg", "application/ogg"},
  {"audio/mpeg", "application/x-id3"},
  {"video/x-ms-asf", "video/x-ms-asf"},
  {"audio/x-wav", "audio/x-wav"},
  {"video/mp4", "video/quicktime, variant=iso"},
  {"video/3gpp", "video/quicktime, variant=(string)3gpp"}
};

nsresult
sbGStreamerTranscode::GetContainer(nsAString &container, nsIArray *properties,
                                   nsACString &gstMuxer)
{
  nsCString cont = NS_ConvertUTF16toUTF8 (container);

  for (int i = 0;
       i < sizeof (SupportedContainers)/sizeof(*SupportedContainers);
       i++) 
  {
    if (strcmp (cont.BeginReading(), SupportedContainers[i].name) == 0)
    {
      const char *capsString = SupportedContainers[i].gstCapsName;
      const char *gstElementName = FindMatchingElementName (
              capsString, "Muxer");
      if (!gstElementName) {
        // Muxers for 'tag formats' like id3 are sometimes named 'Formatter'
        // rather than 'Muxer'. Search for that too.
        gstElementName = FindMatchingElementName (capsString, "Formatter");
      }

      if (!gstElementName)
        continue;

      gstMuxer.Append(gstElementName);
      /* Ignore properties for now, we don't have any we care about yet */
      return NS_OK;
    }
  }
  
  return NS_ERROR_FAILURE;
}

static struct GSTNameMap SupportedAudioCodecs[] = {
  {"audio/x-vorbis", "audio/x-vorbis"},
  {"audio/x-flac", "audio/x-flac"},
  {"audio/x-ms-wma", "audio/x-wma, wmaversion=(int)2"},
  {"audio/mpeg", "audio/mpeg, mpegversion=(int)1, layer=(int)3"},
  {"audio/aac", "audio/mpeg, mpegversion=(int)4"},
};

nsresult
sbGStreamerTranscode::GetAudioCodec(nsAString &aCodec, nsIArray *properties,
                                    nsACString &gstCodec)
{
  nsresult rv;
  nsCString codec = NS_ConvertUTF16toUTF8 (aCodec);

  for (int i = 0;
       i < sizeof (SupportedAudioCodecs)/sizeof(*SupportedAudioCodecs);
       i++) 
  {
    if (strcmp (codec.BeginReading(), SupportedAudioCodecs[i].name) == 0)
    {
      const char *capsString = SupportedAudioCodecs[i].gstCapsName;
      const char *gstElementName = FindMatchingElementName (
              capsString, "Encoder");
      if (!gstElementName)
        continue;

      gstCodec.Append(gstElementName);

      /* Now handle the properties */
      PRUint32 propertiesLength = 0;
      rv = properties->GetLength(&propertiesLength);
      NS_ENSURE_SUCCESS (rv, rv);

      for (PRUint32 j = 0; j < propertiesLength; j++) {
        nsCOMPtr<sbITranscodeProfileProperty> property = 
            do_QueryElementAt(properties, j, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        nsString propName;
        rv = property->GetPropertyName(propName);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIVariant> propValue;
        rv = property->GetValue(getter_AddRefs(propValue));
        NS_ENSURE_SUCCESS(rv, rv);

        nsString propValueString;
        rv = propValue->GetAsAString(propValueString);
        NS_ENSURE_SUCCESS(rv, rv);

        /* Append the property now. Later, we might need to convert some
           keys/values from 'generic' to gstreamer-element-specific ones,
           but we have no examples of that yet */
        gstCodec.AppendLiteral(" ");
        gstCodec.Append(NS_ConvertUTF16toUTF8(propName));
        gstCodec.AppendLiteral("=");
        gstCodec.Append(NS_ConvertUTF16toUTF8(propValueString));

      }

      return NS_OK;
    }
  }
  
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
sbGStreamerTranscode::GetAvailableProfiles(nsIArray * *aAvailableProfiles)
{
  if (mAvailableProfiles) {
    NS_IF_ADDREF (*aAvailableProfiles = mAvailableProfiles);
    return NS_OK;
  }

  /* If we haven't already cached it, then figure out what we have */

  nsresult rv;
  PRBool hasMoreElements;
  nsCOMPtr<nsISimpleEnumerator> dirEnum;

  nsCOMPtr<nsIURI> profilesDirURI;
  rv = NS_NewURI(getter_AddRefs(profilesDirURI),
          NS_LITERAL_STRING("resource://app/gstreamer/encode-profiles"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> profilesDirFileURL =
      do_QueryInterface(profilesDirURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> profilesDir;
  rv = profilesDirFileURL->GetFile(getter_AddRefs(profilesDir));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIMutableArray> array =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbITranscodeProfileLoader> profileLoader = 
      do_CreateInstance("@songbirdnest.com/Songbird/Transcode/ProfileLoader;1",
              &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = profilesDir->GetDirectoryEntries(getter_AddRefs(dirEnum));
  NS_ENSURE_SUCCESS (rv, rv);

  while (PR_TRUE) {
    rv = dirEnum->HasMoreElements(&hasMoreElements);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasMoreElements)
      break;

    nsCOMPtr<nsIFile> file;
    rv = dirEnum->GetNext(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbITranscodeProfile> profile;

    rv = profileLoader->LoadProfile(file, getter_AddRefs(profile));
    if (NS_FAILED(rv))
      continue;

    GstElement *pipeline = BuildTranscodePipeline(profile);
    if (!pipeline) {
      // Not able to use this profile; don't return it.
      continue;
    }

    // Don't actually want the pipeline, discard it.
    gst_object_unref (pipeline);

    rv = array->AppendElement(profile, PR_FALSE);
    NS_ENSURE_SUCCESS (rv, rv);
  }

  mAvailableProfiles = do_QueryInterface(array, &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  NS_ADDREF(*aAvailableProfiles = mAvailableProfiles);

  return NS_OK;
}

