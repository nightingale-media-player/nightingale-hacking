/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbGStreamerMediacore.h"
#include "sbGStreamerMediacoreUtils.h"

#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsIInterfaceRequestorUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsILocalFile.h>
#include <nsIURI.h>
#include <nsIURL.h>
#include <nsIRunnable.h>
#include <nsIIOService.h>
#include <nsIPrefBranch2.h>
#include <nsIObserver.h>
#include <nsThreadUtils.h>
#include <nsCOMPtr.h>
#include <prlog.h>
#include <prprf.h>
#include <nsIClassInfo.h>
#include <nsIClassInfoImpl.h>
#include <mozilla/ReentrantMonitor.h>

// Required to crack open the DOM XUL Element and get a native window handle.
#include <nsIBaseWindow.h>
#include <nsIBoxObject.h>
#include <nsIDocument.h>
#include <nsIDOMEvent.h>
#include <nsIDocShellTreeItem.h>
#include <nsIDocShellTreeOwner.h>
#include <nsIDOMDocument.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMXULElement.h>
#include <nsIScriptGlobalObject.h>
#include <nsIWebNavigation.h>
#include <nsIWidget.h>

#include <sbClassInfoUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <sbVariantUtils.h>
#include <sbBaseMediacoreEventTarget.h>
#include <sbMediacoreError.h>
#include <sbProxiedComponentManager.h>
#include <sbStringBundle.h>
#include <sbErrorConsole.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreManager.h>
#include <sbIGStreamerService.h>
#include <sbIMediaItem.h>
#include <sbStandardProperties.h>
#include <sbVideoBox.h>

#include <gst/pbutils/missing-plugins.h>

#include "nsNetUtil.h"

#include <algorithm>

#ifdef MOZ_WIDGET_GTK2
#include "sbGStreamerPlatformGDK.h"
#endif

#ifdef XP_WIN
#include "sbGStreamerPlatformWin32.h"
#endif

#ifdef XP_MACOSX
#include "sbGStreamerPlatformOSX.h"
#endif

#ifdef CreateEvent
#undef CreateEvent
#endif

#define MAX_FILE_SIZE_FOR_ACCURATE_SEEK (20 * 1024 * 1024)

#define EQUALIZER_FACTORY_NAME "equalizer-10bands"

#define EQUALIZER_DEFAULT_BAND_COUNT \
  sbBaseMediacoreMultibandEqualizer::EQUALIZER_BAND_COUNT_DEFAULT

#define EQUALIZER_BANDS \
  sbBaseMediacoreMultibandEqualizer::EQUALIZER_BANDS_10

/**
 * To log this class, set the following environment variable in a debug build:
 *
 *  NSPR_LOG_MODULES=sbGStreamerMediacore:5 (or :3 for LOG messages only)
 *
 */
#ifdef PR_LOGGING

static PRLogModuleInfo* gGStreamerMediacore =
  PR_NewLogModule("sbGStreamerMediacore");

#define LOG(args)                                          \
  if (gGStreamerMediacore)                             \
    PR_LOG(gGStreamerMediacore, PR_LOG_WARNING, args)

#define TRACE(args)                                        \
  if (gGStreamerMediacore)                             \
    PR_LOG(gGStreamerMediacore, PR_LOG_DEBUG, args)

#else /* PR_LOGGING */

#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */

#endif /* PR_LOGGING */

// CID for sbGStreamerMediacore ?
NS_IMPL_CLASSINFO(sbGStreamerMediacore, NULL, nsIClassInfo::THREADSAFE, SBGSTREAMERSERVICE_CID);

NS_IMPL_THREADSAFE_ADDREF(sbGStreamerMediacore)
NS_IMPL_THREADSAFE_RELEASE(sbGStreamerMediacore)

NS_IMPL_QUERY_INTERFACE11_CI(sbGStreamerMediacore,
                            sbIMediacore,
                            sbIMediacoreMultibandEqualizer,
                            sbIMediacorePlaybackControl,
                            sbIMediacoreVolumeControl,
                            sbIMediacoreVotingParticipant,
                            sbIMediacoreEventTarget,
                            sbIMediacoreVideoWindow,
                            sbIGStreamerMediacore,
                            nsIDOMEventListener,
                            nsIObserver,
                            nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER8(sbGStreamerMediacore,
                             sbIMediacore,
                             sbIMediacoreMultibandEqualizer,
                             sbIMediacorePlaybackControl,
                             sbIMediacoreVideoWindow,
                             sbIMediacoreVolumeControl,
                             sbIMediacoreVotingParticipant,
                             sbIGStreamerMediacore,
                             sbIMediacoreEventTarget)


NS_IMPL_THREADSAFE_CI(sbGStreamerMediacore)

sbGStreamerMediacore::sbGStreamerMediacore() :
    mMonitor("sbGStreamerMediacore::mMonitor"),
    mIsVideoSupported(PR_FALSE),
    mPipeline(nsnull),
    mPlatformInterface(nsnull),
    mPrefs(nsnull),
    mReplaygainElement(nsnull),
    mEqualizerElement(nsnull),
    mTags(NULL),
    mProperties(nsnull),
    mStopped(PR_FALSE),
    mBuffering(PR_FALSE),
    mIsLive(PR_FALSE),
    mMediacoreError(NULL),
    mTargetState(GST_STATE_NULL),
    mVideoDisabled(PR_FALSE),
    mVideoSinkDescription(),
    mAudioSinkDescription(),
    mAudioSinkBufferTime(0),
    mStreamingBufferSize(0),
    mResourceIsLocal(PR_FALSE),
    mResourceSize(-1),
    mGaplessDisabled(PR_FALSE),
    mPlayingGaplessly(PR_FALSE),
    mAbortingPlayback(PR_FALSE),
    mHasReachedPlaying(PR_FALSE),
    mCurrentAudioCaps(NULL),
    mAudioBinGhostPad(NULL),
    mHasVideo(PR_FALSE),
    mHasAudio(PR_FALSE)
{
  mBaseEventTarget = new sbBaseMediacoreEventTarget(this);
  NS_WARN_IF_FALSE(mBaseEventTarget,
          "mBaseEventTarget is null, may be out of memory");
}

sbGStreamerMediacore::~sbGStreamerMediacore()
{
  if (mTags)
    gst_tag_list_free(mTags);

  if (mReplaygainElement)
    gst_object_unref (mReplaygainElement);

  if (mEqualizerElement)
    gst_object_unref (mEqualizerElement);

  std::vector<GstElement *>::const_iterator it = mAudioFilters.begin();
  for ( ; it < mAudioFilters.end(); ++it)
    gst_object_unref (*it);

}

nsresult
sbGStreamerMediacore::Init()
{
  nsresult rv;

  rv = sbBaseMediacore::InitBaseMediacore();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbBaseMediacoreMultibandEqualizer::InitBaseMediacoreMultibandEqualizer();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbBaseMediacorePlaybackControl::InitBaseMediacorePlaybackControl();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbBaseMediacoreVolumeControl::InitBaseMediacoreVolumeControl();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = InitPreferences();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacore> core = do_QueryInterface(
          NS_ISUPPORTS_CAST(sbIMediacore *, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined (MOZ_WIDGET_GTK2)
  mIsVideoSupported = PR_TRUE;
  mPlatformInterface = new GDKPlatformInterface(this);
#elif defined (XP_WIN)
  mIsVideoSupported = PR_TRUE;
  mPlatformInterface = new Win32PlatformInterface(this);
#elif defined (XP_MACOSX)
  mIsVideoSupported = PR_TRUE;
  mPlatformInterface = new OSXPlatformInterface(this);
#else
  LOG(("No video backend available for this platform"));
  mIsVideoSupported = PR_FALSE;
#endif

  return NS_OK;
}

nsresult
sbGStreamerMediacore::InitPreferences()
{
  nsresult rv;
  mPrefs = do_ProxiedGetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  rv = mPrefs->AddObserver("songbird.mediacore", this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ReadPreferences();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGStreamerMediacore::ReadPreferences()
{
  NS_ENSURE_STATE (mPrefs);
  nsresult rv;

  rv = mPrefs->GetBoolPref("songbird.mediacore.gstreamer.disablevideo",
	                       &mVideoDisabled);
  if (rv == NS_ERROR_UNEXPECTED)
    mVideoDisabled = PR_FALSE;
  else
    NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 prefType;
  const char *VIDEO_SINK_PREF = "songbird.mediacore.gstreamer.videosink";
  const char *AUDIO_SINK_PREF = "songbird.mediacore.gstreamer.audiosink";

  rv = mPrefs->GetPrefType(VIDEO_SINK_PREF, &prefType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (prefType == nsIPrefBranch::PREF_STRING) {
    rv = mPrefs->GetCharPref(VIDEO_SINK_PREF,
	getter_Copies(mVideoSinkDescription));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mPrefs->GetPrefType(AUDIO_SINK_PREF, &prefType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (prefType == nsIPrefBranch::PREF_STRING) {
    rv = mPrefs->GetCharPref(AUDIO_SINK_PREF,
	getter_Copies(mAudioSinkDescription));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  /* In milliseconds */
  const char *AUDIO_SINK_BUFFERTIME_PREF =
      "songbird.mediacore.output.buffertime";
  /* In kilobytes */
  const char *STREAMING_BUFFERSIZE_PREF =
      "songbird.mediacore.streaming.buffersize";

  /* Defaults if the prefs aren't present */
  PRInt64 audioSinkBufferTime = 1000 * 1000; /* 1000 ms */
  PRInt32 streamingBufferSize = 256 * 1024; /* 256 kB */

  rv = mPrefs->GetPrefType(AUDIO_SINK_BUFFERTIME_PREF, &prefType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (prefType == nsIPrefBranch::PREF_INT) {
    PRInt32 time = 0;
    rv = mPrefs->GetIntPref(AUDIO_SINK_BUFFERTIME_PREF, &time);
    NS_ENSURE_SUCCESS(rv, rv);

    // Only use the pref if it's a valid value.
    if(time >= 100 || time <= 10000) {
      // Convert to usecs
      audioSinkBufferTime = time * 1000;
    }
  }

  rv = mPrefs->GetPrefType(STREAMING_BUFFERSIZE_PREF, &prefType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (prefType == nsIPrefBranch::PREF_INT) {
    rv = mPrefs->GetIntPref(STREAMING_BUFFERSIZE_PREF, &streamingBufferSize);
    NS_ENSURE_SUCCESS(rv, rv);

    streamingBufferSize *= 1024; /* pref is in kB, we want it in B */
  }

  mAudioSinkBufferTime = audioSinkBufferTime;
  mStreamingBufferSize = streamingBufferSize;

  const char *NORMALIZATION_ENABLED_PREF =
      "songbird.mediacore.normalization.enabled";
  const char *NORMALIZATION_MODE_PREF =
      "songbird.mediacore.normalization.preferredGain";
  PRBool normalizationEnabled = PR_TRUE;
  rv = mPrefs->GetPrefType(NORMALIZATION_ENABLED_PREF, &prefType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (prefType == nsIPrefBranch::PREF_BOOL) {
    rv = mPrefs->GetBoolPref(NORMALIZATION_ENABLED_PREF, &normalizationEnabled);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (normalizationEnabled) {
    if (!mReplaygainElement) {
      mReplaygainElement = gst_element_factory_make ("rgvolume", NULL);

      // Ref and sink the object to take ownership; we'll keep track of it
      // from here on.
      gst_object_ref (mReplaygainElement);
      gst_object_sink (mReplaygainElement);

      rv = AddAudioFilter(mReplaygainElement);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    /* Now check the mode; set the appropriate property on the element */
    nsCString normalizationMode;
    rv = mPrefs->GetPrefType(NORMALIZATION_MODE_PREF, &prefType);
    NS_ENSURE_SUCCESS(rv, rv);
    if (prefType == nsIPrefBranch::PREF_STRING) {
      rv = mPrefs->GetCharPref(NORMALIZATION_MODE_PREF,
              getter_Copies(normalizationMode));
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (normalizationMode.EqualsLiteral("track")) {
      g_object_set (mReplaygainElement, "album-mode", FALSE, NULL);
    }
    else {
      g_object_set (mReplaygainElement, "album-mode", TRUE, NULL);
    }
  }
  else {
    if (mReplaygainElement) {
      rv = RemoveAudioFilter(mReplaygainElement);
      NS_ENSURE_SUCCESS(rv, rv);

      gst_object_unref (mReplaygainElement);
      mReplaygainElement = NULL;
    }
  }

  return NS_OK;
}


// Utility methods

/* static */ void
sbGStreamerMediacore::aboutToFinishHandler(GstElement *playbin, gpointer data)
{
  sbGStreamerMediacore *core = static_cast<sbGStreamerMediacore*>(data);
  core->HandleAboutToFinishSignal();
  return;
}

GstElement *
sbGStreamerMediacore::CreateSinkFromPrefs(const char *aSinkDescription)
{
  // Only try to create it if we have a non-null, non-zero-length description
  if (aSinkDescription && *aSinkDescription)
  {
    GstElement *sink = gst_parse_bin_from_description (aSinkDescription,
            TRUE, NULL);
    // If parsing failed, sink is NULL; return it either way.
    return sink;
  }

  return NULL;
}

nsresult
sbGStreamerMediacore::GetFileSize(nsIURI *aURI, PRInt64 *aFileSize)
{
  nsresult rv;

  nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(aURI, &rv);
  if (rv != NS_ERROR_NO_INTERFACE) {
    NS_ENSURE_SUCCESS (rv, rv);

    nsCOMPtr<nsIFile> file;
    rv = fileUrl->GetFile(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = file->GetFileSize(aFileSize);
    NS_ENSURE_SUCCESS (rv, rv);
  }
  else {
    // That's ok, not a local file.
    return rv;
  }

  return NS_OK;
}

GstElement *
sbGStreamerMediacore::CreateVideoSink()
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  GstElement *videosink = CreateSinkFromPrefs(mVideoSinkDescription.get());

  if (mPlatformInterface)
    videosink = mPlatformInterface->SetVideoSink(videosink);

  return videosink;
}

GstElement *
sbGStreamerMediacore::CreateAudioSink()
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  GstElement *sinkbin = gst_bin_new ("audiosink-bin");
  GstElement *audiosink = CreateSinkFromPrefs(mAudioSinkDescription.get());
  GstPad *targetpad, *ghostpad;

  if (mPlatformInterface)
    audiosink = mPlatformInterface->SetAudioSink(audiosink);

  /* audiosink is our actual sink, or something else close enough to it.
   * Now, we build a bin looking like this, with each of the added audio
   * filters in it:
   *
   * |-------------------------------------------------------------------|
   * |audiosink-bin                                                      |
   * |                                                                   |
   * | [-------]  [------------]  [-------]  [------------]  [---------] |
   * |-[Filter1]--[audioconvert]--[Filter2]--[audioconvert]--[audiosink] |
   * | [-------]  [------------]  [-------]  [------------]  [---------] |
   * |                                                                   |
   * |-------------------------------------------------------------------|
   *
   */

  gst_bin_add ((GstBin *)sinkbin, audiosink);

  targetpad = gst_element_get_pad (audiosink, "sink");

  /* Add each filter, followed by an audioconvert. The first-added filter ends
   * last in the pipeline, so we iterate in reverse.
   */
  std::vector<GstElement *>::const_reverse_iterator it = mAudioFilters.rbegin(),
      end = mAudioFilters.rend();
  for ( ; it != end; ++it)
  {
    GstElement *audioconvert = gst_element_factory_make ("audioconvert", NULL);
    GstElement *filter = *it;
    GstPad *srcpad, *sinkpad;

    gst_bin_add_many ((GstBin *)sinkbin, filter, audioconvert, NULL);

    srcpad = gst_element_get_pad (filter, "src");
    sinkpad = gst_element_get_pad (audioconvert, "sink");
    gst_pad_link (srcpad, sinkpad);
    gst_object_unref (srcpad);
    gst_object_unref (sinkpad);

    srcpad = gst_element_get_pad (audioconvert, "src");
    gst_pad_link (srcpad, targetpad);
    gst_object_unref (targetpad);
    gst_object_unref (srcpad);

    targetpad = gst_element_get_pad (filter, "sink");
  }

  // Now, targetpad is the left-most real pad in our bin. Ghost it to provide
  // a sinkpad on our bin.
  ghostpad = gst_ghost_pad_new ("sink", targetpad);
  gst_element_add_pad (sinkbin, ghostpad);

  mAudioBinGhostPad = GST_GHOST_PAD (gst_object_ref (ghostpad));

  gst_object_unref (targetpad);

  return sinkbin;
}

/* static */ void
sbGStreamerMediacore::currentAudioSetHelper(GObject* obj, GParamSpec* pspec,
        sbGStreamerMediacore *core)
{
  int current_audio;
  GstPad *pad;

  // If a current audio stream has been selected, then we're playing audio
  core->mHasAudio = PR_TRUE;

  /* Which audio stream has been activated? */
  g_object_get(obj, "current-audio", &current_audio, NULL);
  NS_ASSERTION(current_audio >= 0, "current audio is negative");

  /* Get the audio pad for this stream number */
  g_signal_emit_by_name(obj, "get-audio-pad", current_audio, &pad);

  if (pad) {
    GstCaps *caps;
    caps = gst_pad_get_negotiated_caps(pad);
    if (caps) {
      core->OnAudioCapsSet(caps);
      gst_caps_unref(caps);
    }

    g_signal_connect(pad, "notify::caps",
            G_CALLBACK(audioCapsSetHelper), core);

    gst_object_unref(pad);
  }
}

/* static */ void
sbGStreamerMediacore::audioCapsSetHelper(GObject* obj, GParamSpec* pspec,
        sbGStreamerMediacore *core)
{
  GstPad *pad = GST_PAD(obj);
  GstCaps *caps = gst_pad_get_negotiated_caps(pad);

  if (caps) {
    core->OnAudioCapsSet(caps);
    gst_caps_unref (caps);
  }
}

/* static */ void
sbGStreamerMediacore::currentVideoSetHelper(GObject* obj, GParamSpec* pspec,
        sbGStreamerMediacore *core)
{
  int current_video;
  GstPad *pad;

  // If a current video stream has been selected, then we're playing video
  core->mHasVideo = PR_TRUE;

  /* Which video stream has been activated? */
  g_object_get(obj, "current-video", &current_video, NULL);
  NS_ASSERTION(current_video >= 0, "current video is negative");

  /* Get the video pad for this stream number */
  g_signal_emit_by_name(obj, "get-video-pad", current_video, &pad);

  if (pad) {
    GstCaps *caps;
    caps = gst_pad_get_negotiated_caps(pad);
    if (caps) {
      core->OnVideoCapsSet(caps);
      gst_caps_unref(caps);
    }

    g_signal_connect(pad, "notify::caps",
            G_CALLBACK(videoCapsSetHelper), core);

    gst_object_unref(pad);
  }
}

/* static */ void
sbGStreamerMediacore::videoCapsSetHelper(GObject* obj, GParamSpec* pspec,
        sbGStreamerMediacore *core)
{
  GstPad *pad = GST_PAD(obj);
  GstCaps *caps = gst_pad_get_negotiated_caps(pad);

  if (caps) {
    core->OnVideoCapsSet(caps);
    gst_caps_unref (caps);
  }
}

nsresult
sbGStreamerMediacore::DestroyPipeline()
{
  GstElement *pipeline = NULL;

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);;
    if (mPipeline)
      pipeline = (GstElement *)g_object_ref (mPipeline);
  }

  /* Do state-change with the lock dropped */
  if (pipeline) {
    gst_element_set_state (pipeline, GST_STATE_NULL);
    gst_object_unref (pipeline);
  }

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  if (mPipeline) {
    /* If we put any filters in the pipeline, remove them now, so that we can
     * re-add them later to a different pipeline
     */
    std::vector<GstElement *>::const_iterator it = mAudioFilters.begin();
    for ( ; it < mAudioFilters.end(); ++it)
    {
      GstElement *parent;
      GstElement *filter = *it;

      parent = (GstElement *)gst_element_get_parent (filter);
      if (parent) {
        gst_bin_remove ((GstBin *)parent, filter);
        gst_object_unref (parent);
      }
    }

    /* Work around bug in ghostpads (upstream #570910) by explicitly
     * untargetting this ghostpad */
    if (mAudioBinGhostPad) {
      gst_ghost_pad_set_target (mAudioBinGhostPad, NULL);
      gst_object_unref (mAudioBinGhostPad);
      mAudioBinGhostPad = NULL;
    }

    gst_object_unref (mPipeline);
    mPipeline = nsnull;
  }
  if (mTags) {
    gst_tag_list_free (mTags);
    mTags = nsnull;
  }
  mProperties = nsnull;

  if (mCurrentAudioCaps) {
    gst_caps_unref (mCurrentAudioCaps);
    mCurrentAudioCaps = NULL;
  }

  mStopped = PR_FALSE;
  mBuffering = PR_FALSE;
  mIsLive = PR_FALSE;
  mMediacoreError = NULL;
  mTargetState = GST_STATE_NULL;
  mGaplessDisabled = PR_FALSE;
  mPlayingGaplessly = PR_FALSE;
  mAbortingPlayback = PR_FALSE;
  mHasReachedPlaying = PR_FALSE;
  mVideoSize = NULL;
  mHasVideo = PR_FALSE;
  mHasAudio = PR_FALSE;

  return NS_OK;
}

nsresult
sbGStreamerMediacore::SetBufferingProperties(GstElement *aPipeline)
{
  NS_ENSURE_ARG_POINTER(aPipeline);

  if (g_object_class_find_property(
              G_OBJECT_GET_CLASS (aPipeline), "buffer-size"))
    g_object_set (aPipeline, "buffer-size", mStreamingBufferSize, NULL);

  return NS_OK;
}

// Set the property on the first applicable element we find. That's the first
// in sorted-iteration order, at minimal depth.
bool
sbGStreamerMediacore::SetPropertyOnChild(GstElement *aElement,
        const char *aPropertyName, gint64 aPropertyValue)
{
  bool done = false;
  bool ret = false;

  if (g_object_class_find_property(
      G_OBJECT_GET_CLASS (aElement), aPropertyName))
  {
    g_object_set (aElement, aPropertyName, aPropertyValue, NULL);
    return true;
  }

  if (GST_IS_BIN (aElement)) {
    // Iterate in sorted order, so we look at sinks first
    GstIterator *it = gst_bin_iterate_sorted ((GstBin *)aElement);

    while (!done) {
      gpointer data;
      GstElement *child;
      switch (gst_iterator_next (it, &data)) {
        case GST_ITERATOR_OK:
          child = GST_ELEMENT_CAST (data);
          if (SetPropertyOnChild(child,
                  aPropertyName, aPropertyValue))
          {
            ret = true;
            done = true;
          }
          gst_object_unref (child);
          break;
        case GST_ITERATOR_DONE:
          done = TRUE;
          break;
        case GST_ITERATOR_RESYNC:
          gst_iterator_resync (it);
          break;
        case GST_ITERATOR_ERROR:
          done = true;
          break;
      }
    }

    gst_iterator_free (it);
  }

  return ret;
}

nsresult
sbGStreamerMediacore::CreatePlaybackPipeline()
{
  nsresult rv;
  gint flags;

  // destroy pipeline will acquire the monitor.
  rv = DestroyPipeline();
  NS_ENSURE_SUCCESS (rv, rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  mPipeline = gst_element_factory_make ("playbin2", "player");

  if (!mPipeline)
    return NS_ERROR_FAILURE;

  if (mPlatformInterface) {
    GstElement *audiosink = CreateAudioSink();

    // Set audio sink
    g_object_set(mPipeline, "audio-sink", audiosink, NULL);

    // Set audio sink buffer time based on pref
    SetPropertyOnChild(audiosink, "buffer-time",
            (gint64)mAudioSinkBufferTime);

    if (!mVideoDisabled) {
      GstElement *videosink = CreateVideoSink();
      g_object_set(mPipeline, "video-sink", videosink, NULL);
    }
  }

  // Configure what to output - we want audio only, unless video
  // is turned on
  flags = 0x2 | 0x10; // audio | soft-volume
  if (!mVideoDisabled && mIsVideoSupported) {
    // Enable video only if we're set up for it is turned off. Also enable
    // text (subtitles), which require a video window to display.
    flags |= 0x1 | 0x4; // video | text
  }
  g_object_set (G_OBJECT(mPipeline), "flags", flags, NULL);

  GstBus *bus = gst_element_get_bus (mPipeline);

  // We want to receive state-changed messages when shutting down, so we
  // need to turn off bus auto-flushing
  g_object_set(mPipeline, "auto-flush-bus", FALSE, NULL);

  rv = SetBufferingProperties(mPipeline);
  NS_ENSURE_SUCCESS (rv, rv);

  // Handle GStreamer messages synchronously, either directly or
  // dispatching to the main thread.
  gst_bus_set_sync_handler (bus, SyncToAsyncDispatcher,
                            static_cast<sbGStreamerMessageHandler*>(this));

  g_object_unref ((GObject *)bus);

  // Handle about-to-finish signal emitted by playbin2
  g_signal_connect (mPipeline, "about-to-finish",
          G_CALLBACK (aboutToFinishHandler), this);
  // Get notified when the current audio/video stream changes.
  // This will let us get information about the specific audio or video stream
  // being played.
  g_signal_connect (mPipeline, "notify::current-video",
          G_CALLBACK (currentVideoSetHelper), this);
  g_signal_connect (mPipeline, "notify::current-audio",
          G_CALLBACK (currentAudioSetHelper), this);

  return NS_OK;
}

PRBool sbGStreamerMediacore::HandleSynchronousMessage(GstMessage *aMessage)
{
  GstMessageType msg_type;
  msg_type = GST_MESSAGE_TYPE(aMessage);

  // Don't do message processing during an abort
  if (mAbortingPlayback) {
    TRACE(("Dropping message due to abort\n"));
    return PR_TRUE;
  }

  switch (msg_type) {
    case GST_MESSAGE_ELEMENT: {
      // Win32 and GDK use prepare-xwindow-id, OSX has its own private thing,
      // have-ns-view
      if (gst_structure_has_name(aMessage->structure, "prepare-xwindow-id") ||
          gst_structure_has_name(aMessage->structure, "have-ns-view"))
      {
        if(mPlatformInterface)
        {
          DispatchMediacoreEvent(sbIMediacoreEvent::STREAM_HAS_VIDEO);
          mPlatformInterface->PrepareVideoWindow(aMessage);
        }
        return PR_TRUE;
      }
    }
    default:
      break;
  }

  /* Return PR_FALSE since we haven't handled the message */
  return PR_FALSE;
}

void sbGStreamerMediacore::DispatchMediacoreEvent (unsigned long type,
        nsIVariant *aData, sbIMediacoreError *aError)
{
  nsresult rv;
  nsCOMPtr<sbIMediacoreEvent> event;
  rv = sbMediacoreEvent::CreateEvent(type,
                                     aError,
                                     aData,
                                     this,
                                     getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = DispatchEvent(event, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, /* void */);
}

void sbGStreamerMediacore::HandleAboutToFinishSignal()
{
  LOG(("Handling about-to-finish signal"));

  nsCOMPtr<sbIMediacoreSequencer> sequencer;

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

    // Never try to handle the next file if we've seen an error, or if gapless
    // is disabled for this resource
    if (mMediacoreError || mGaplessDisabled) {
      LOG(("Ignoring about-to-finish signal"));
      return;
    }

    sequencer = mSequencer;
  }

  if(!sequencer) {
    return;
  }

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = sequencer->GetNextItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, /*void*/ );
  NS_ENSURE_TRUE(item, /*void*/);

  nsString contentURL;
  rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
          contentURL);
  NS_ENSURE_SUCCESS(rv, /*void*/ );

  if(StringBeginsWith(contentURL, NS_LITERAL_STRING("file:"))) {
    rv = sequencer->RequestHandleNextItem(this);
    NS_ENSURE_SUCCESS(rv, /*void*/ );

    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

    // Clear old tags so we don't merge them with the new ones
    if (mTags) {
      gst_tag_list_free (mTags);
      mTags = nsnull;
    }
    mProperties = nsnull;
    mResourceIsLocal = PR_TRUE;

    nsCOMPtr<nsIURI> itemuri;
    rv = item->GetContentSrc(getter_AddRefs(itemuri));
    NS_ENSURE_SUCCESS (rv, /*void*/);

    rv = GetFileSize (itemuri, &mResourceSize);
    NS_ENSURE_TRUE(mPipeline, /*void*/);

    nsCString uri = NS_ConvertUTF16toUTF8(contentURL);
    LOG(("Setting URI to \"%s\"", uri.BeginReading()));

    /* Set the URI to play */
    NS_ENSURE_TRUE(mPipeline, /*void*/);
    g_object_set (G_OBJECT (mPipeline), "uri", uri.BeginReading(), NULL);
    mCurrentUri = uri;
    mUri = itemuri;

    mPlayingGaplessly = PR_TRUE;

    /* Ideally we wouldn't dispatch this until actual audio output of this new
     * file has started, but playbin2 doesn't tell us about that yet */
    DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_START);
  }

  return;
}

void sbGStreamerMediacore::HandleTagMessage(GstMessage *message)
{
  GstTagList *tag_list;
  nsresult rv;

  LOG(("Handling tag message"));
  gst_message_parse_tag(message, &tag_list);

  if (mTags) {
    GstTagList *newTags = gst_tag_list_merge (mTags, tag_list,
            GST_TAG_MERGE_REPLACE);
    gst_tag_list_free (mTags);
    mTags = newTags;
  }
  else
    mTags = gst_tag_list_copy (tag_list);

  rv = ConvertTagListToPropertyArray(mTags, getter_AddRefs(mProperties));
  gst_tag_list_free(tag_list);

  if (NS_SUCCEEDED (rv)) {
    nsCOMPtr<nsISupports> properties = do_QueryInterface(mProperties, &rv);
    NS_ENSURE_SUCCESS (rv, /* void */);
    nsCOMPtr<nsIVariant> propVariant = sbNewVariant(properties).get();
    DispatchMediacoreEvent (sbIMediacoreEvent::METADATA_CHANGE, propVariant);
  }
  else // Non-fatal, just log a message
    LOG(("Failed to convert"));
}

void sbGStreamerMediacore::HandleStateChangedMessage(GstMessage *message)
{
  // Only listen to state-changed messages from top-level pipelines
  if (GST_IS_PIPELINE (message->src))
  {
    GstState oldstate, newstate, pendingstate;
    gst_message_parse_state_changed (message,
            &oldstate, &newstate, &pendingstate);

    // Dispatch START, PAUSE, STOP/END (but only if it's our target state)
    if (pendingstate == GST_STATE_VOID_PENDING && newstate == mTargetState) {
      if (newstate == GST_STATE_PLAYING) {
        mHasReachedPlaying = PR_TRUE;
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_START);
      }
      else if (newstate == GST_STATE_PAUSED)
        DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_PAUSE);
      else if (newstate == GST_STATE_NULL)
      {
        // Distinguish between 'stopped via API' and 'stopped due to error or
        // reaching EOS'
        if (mStopped) {
          DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_STOP);
        }
        else {
          // If we stopped due to an error, send the actual error event now.
          if (mMediacoreError) {
            DispatchMediacoreEvent(sbIMediacoreEvent::ERROR_EVENT, nsnull,
                    mMediacoreError);
            mMediacoreError = NULL;
          }
          DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_END);
        }
      }
    }
    // We've reached our current pending state, but not our target state.
    else if (pendingstate == GST_STATE_VOID_PENDING)
    {
      // If we're not waiting for buffering to complete (where we handle
      // the state changes differently), then continue on to PLAYING.
      if (newstate == GST_STATE_PAUSED && mTargetState == GST_STATE_PLAYING &&
          !mBuffering)
      {
        gst_element_set_state (mPipeline, GST_STATE_PLAYING);
      }
    }
  }
}

void sbGStreamerMediacore::HandleBufferingMessage (GstMessage *message)
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  gint percent = 0;
  gint maxpercent;
  gst_message_parse_buffering (message, &percent);
  TRACE(("Buffering (%u percent done)", percent));

  // We don't want to handle buffering specially for live pipelines
  if (mIsLive)
    return;

  // 'maxpercent' is how much of our maximum buffer size we must fill before
  // we start playing. We want to be able to keep buffering more data (if the
  // server is sending it fast enough) even after we start playing - so we
  // start with maxpercent at 33, then increase it to 100 if we ever have to
  // rebuffer (i.e. return to buffering AFTER starting playback)
  if (mHasReachedPlaying)
    maxpercent = 100;
  else
    maxpercent = 33;

  /* If we receive buffering messages, go to PAUSED.
   * Then, return to PLAYING once we have sufficient buffering (which will be
   * before we actually hit PAUSED)
   */
  if (percent >= maxpercent && mBuffering) {
    mBuffering = PR_FALSE;

    if (mTargetState == GST_STATE_PLAYING) {
      TRACE(("Buffering complete, setting state to playing"));
      gst_element_set_state (mPipeline, GST_STATE_PLAYING);
    }
    else if (mTargetState == GST_STATE_PAUSED) {
      TRACE(("Buffering complete, setting state to paused"));
      DispatchMediacoreEvent (sbIMediacoreEvent::STREAM_PAUSE);
    }
  }
  else if (percent < maxpercent) {
    GstState cur_state;
    gst_element_get_state (mPipeline, &cur_state, NULL, 0);

    /* Only pause if we've already reached playing (this means we've underrun
     * the buffer and need to rebuffer) */
    if (!mBuffering && cur_state == GST_STATE_PLAYING) {
      TRACE(("Buffering... setting to paused"));
      gst_element_set_state (mPipeline, GST_STATE_PAUSED);
      mTargetState = GST_STATE_PLAYING;

      // And inform listeners that we've underrun */
      DispatchMediacoreEvent(sbIMediacoreEvent::BUFFER_UNDERRUN);
    }
    mBuffering = PR_TRUE;

    // Inform listeners of current progress
    double bufferingProgress = (double)percent / (double)maxpercent;
    nsCOMPtr<nsIVariant> variant = sbNewVariant(bufferingProgress).get();
    DispatchMediacoreEvent(sbIMediacoreEvent::BUFFERING, variant);
  }
}

// Demuxers (such as qtdemux) send redirect messages when the media
// file itself redirects to another location. Handle these here.
void sbGStreamerMediacore::HandleRedirectMessage(GstMessage *message)
{
  const gchar *location;
  nsresult rv;
  nsCString uriString;

  location = gst_structure_get_string (message->structure, "new-location");

  if (location && *location) {
    if (strstr (location, "://") != NULL) {
      // Then we assume it's an absolute URL
      uriString = location;
    }
    else {
      TRACE (("Resolving redirect to '%s'", location));

      rv = mUri->Resolve(nsDependentCString(location), uriString);
      NS_ENSURE_SUCCESS (rv, /* void */ );
    }

    // Now create a URI from our string form.
    nsCOMPtr<nsIIOService> ioService = do_GetService(
            "@mozilla.org/network/io-service;1", &rv);
    NS_ENSURE_SUCCESS (rv, /* void */ );

    nsCOMPtr<nsIURI> finaluri;
    rv = ioService->NewURI(uriString, nsnull, nsnull,
            getter_AddRefs(finaluri));
    NS_ENSURE_SUCCESS (rv, /* void */ );

    PRBool isEqual;
    rv = finaluri->Equals(mUri, &isEqual);
    NS_ENSURE_SUCCESS (rv, /* void */ );

    // Don't loop forever redirecting to ourselves. If the URIs are the same,
    // then just ignore the redirect message.
    if (isEqual)
      return;

    // Ok, we have a new uri, and we're ready to use it...
    rv = SetUri(finaluri);
    NS_ENSURE_SUCCESS (rv, /* void */ );

    // Inform listeners that we've switched URI
    nsCOMPtr<nsIVariant> propVariant = sbNewVariant(finaluri).get();
    DispatchMediacoreEvent (sbIMediacoreEvent::URI_CHANGE, propVariant);

    // And finally, attempt to play it
    rv = Play();
    NS_ENSURE_SUCCESS (rv, /* void */ );
  }
}

void sbGStreamerMediacore::HandleMissingPluginMessage(GstMessage *message)
{
  nsCOMPtr<sbMediacoreError> error;
  gchar *debugMessage;
  nsString errorMessage;
  nsresult rv;
  nsString stringName;

  sbStringBundle bundle;
  nsTArray<nsString> params;

  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  // Build an error message to output to the console
  // TODO: This is currently not localised (but we're probably not setting
  // things up right to get translated gstreamer messages anyway).
  debugMessage = gst_missing_plugin_message_get_description(message);
  if (debugMessage) {
    stringName = NS_LITERAL_STRING("mediacore.error.known_codec_not_found");
    params.AppendElement(NS_ConvertUTF8toUTF16(debugMessage));
    g_free(debugMessage);
  } else {
    stringName = NS_LITERAL_STRING("mediacore.error.codec_not_found");
  }

  if (!mMediacoreError) {
    nsCOMPtr<sbIMediacoreSequencer> sequencer;
    {
      mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
      sequencer = mSequencer;
    }

    if (sequencer) {
      // Got a valid sequencer, so grab the item's trackName to use in
      // the error message
      nsCOMPtr<sbIMediaItem> item;
      nsresult rv = sequencer->GetCurrentItem(getter_AddRefs(item));
      if (NS_SUCCEEDED(rv) && item) {
        nsString trackNameProp;
        rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                               trackNameProp);
        if (NS_SUCCEEDED(rv)) {
          nsAutoString stripped (trackNameProp);
          CompressWhitespace(stripped);
          if (!stripped.IsEmpty()) {
            error = new sbMediacoreError;
            if (NS_SUCCEEDED(rv)) {
              params.InsertElementAt(0, trackNameProp);
              errorMessage = bundle.Format(stringName, params);
              rv = error->Init(sbMediacoreError::SB_STREAM_CODEC_NOT_FOUND,
                               errorMessage);
            }
          }
        }
      }
    }

    // We'll hit this if we didn't find the sequencer, or if we got the
    // sequencer, but had an empty trackName
    if (!error) {
      // If we couldn't find the sequencer, then use the item's file path
      // to generate the error message
      nsCOMPtr<nsIURI> uri;
      rv = GetUri(getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, /* void */);

      nsCOMPtr<nsIFileURL> url = do_QueryInterface(uri, &rv);
      if (NS_SUCCEEDED(rv))
      {
        nsCOMPtr<nsIFile> file;
        nsString path;

        rv = url->GetFile(getter_AddRefs(file));
        if (NS_SUCCEEDED(rv))
        {
          rv = file->GetPath(path);

          if (NS_SUCCEEDED(rv)) {
            error = new sbMediacoreError;
            if (NS_SUCCEEDED(rv)) {
              params.InsertElementAt(0, path);
              errorMessage = bundle.Format(stringName, params);
              rv = error->Init(sbMediacoreError::SB_STREAM_CODEC_NOT_FOUND,
                               errorMessage);
            }
          }
        }
      }

      if (NS_FAILED(rv)) // not an else, so that it serves as a fallback
      {
        nsCString temp;
        nsString spec;

        rv = uri->GetSpec(temp);
        if (NS_SUCCEEDED(rv))
          spec = NS_ConvertUTF8toUTF16(temp);
        else
          spec = NS_ConvertUTF8toUTF16(mCurrentUri);

        error = new sbMediacoreError;
        if (NS_SUCCEEDED(rv)) {
          params.InsertElementAt(0, spec);
          errorMessage = bundle.Format(stringName, params);
          rv = error->Init(sbMediacoreError::SB_STREAM_CODEC_NOT_FOUND,
                           errorMessage);
        }
      }

      NS_ENSURE_SUCCESS(rv, /* void */);
    }

    // We don't actually send the error right now. If we did, the sequencer
    // might tell us to continue on to the next file before we've finished
    // processing shutdown for this stream.
    // Instead, we just cache it here, and actually process it once shutdown
    // is complete.
    mMediacoreError = error;
  }

  // Then, shut down the pipeline, which will cause
  // a STREAM_END event to be fired. Immediately before firing that, we'll
  // send our error message.
  GstElement *pipeline;
  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
    mTargetState = GST_STATE_NULL;
    pipeline = (GstElement *)g_object_ref (mPipeline);
  }

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);

  // Log the error message
  sbErrorConsole::Error("Mediacore:GStreamer", errorMessage);
}

void sbGStreamerMediacore::HandleEOSMessage(GstMessage *message)
{
  GstElement *pipeline;
  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
    pipeline = (GstElement *)g_object_ref (mPipeline);
    mTargetState = GST_STATE_NULL;
  }

  // Shut down the pipeline. This will cause us to send a STREAM_END
  // event when we get the state-changed message to GST_STATE_NULL
  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);
}

void sbGStreamerMediacore::HandleErrorMessage(GstMessage *message)
{
  GError *gerror = NULL;
  nsString errormessage;
  nsCOMPtr<sbIMediacoreError> error;
  nsCOMPtr<sbIMediacoreEvent> event;
  gchar *debugMessage;
  nsresult rv = NS_ERROR_UNEXPECTED;

  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  gst_message_parse_error(message, &gerror, &debugMessage);

  if (!mMediacoreError) {

    // Try and fetch track name from sequencer information.
    nsCOMPtr<sbIMediacoreSequencer> sequencer;
    {
      mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
      sequencer = mSequencer;
    }

    if (sequencer) {
      // Got a valid sequencer, so grab the item's trackName to use in
      // the error message
      nsCOMPtr<sbIMediaItem> item;
      rv = sequencer->GetCurrentItem(getter_AddRefs(item));
      if (NS_SUCCEEDED(rv) && item) {
        nsString trackNameProp;
        rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME),
                               trackNameProp);
        if (NS_SUCCEEDED(rv)) {
          nsAutoString stripped (trackNameProp);
          CompressWhitespace(stripped);
          rv = GetMediacoreErrorFromGstError(gerror, stripped,
                                             GStreamer::OP_UNKNOWN,
                                             getter_AddRefs(error));
        }
      }
    }

    //
    // If there was no sequencer or there was an error while fetching the
    // track name we'll fall back to using the file path.
    //
    if (NS_FAILED(rv)) {
      // Create an error for later dispatch.
      nsCOMPtr<nsIURI> uri;
      rv = GetUri(getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, /* void */);

      nsCOMPtr<nsIFileURL> url = do_QueryInterface(uri, &rv);
      if (NS_SUCCEEDED(rv))
      {
        nsCOMPtr<nsIFile> file;
        nsString path;

        rv = url->GetFile(getter_AddRefs(file));
        if (NS_SUCCEEDED(rv))
        {
          rv = file->GetPath(path);

          if (NS_SUCCEEDED(rv))
            rv = GetMediacoreErrorFromGstError(gerror, path,
                                               GStreamer::OP_UNKNOWN,
                                               getter_AddRefs(error));
        }
      }

      //
      // Blast! Still no information for the user, use the URI Spec as a last
      // ditch effort to give them useful information.
      //
      if (NS_FAILED(rv)) // not an else, so that it serves as a fallback
      {
        nsCString temp;
        nsString spec;

        rv = uri->GetSpec(temp);
        if (NS_SUCCEEDED(rv))
          spec = NS_ConvertUTF8toUTF16(temp);
        else
          spec = NS_ConvertUTF8toUTF16(mCurrentUri);

        rv = GetMediacoreErrorFromGstError(gerror, spec,
                                           GStreamer::OP_UNKNOWN,
                                           getter_AddRefs(error));
      }
    }

    NS_ENSURE_SUCCESS(rv, /* void */);

    // We don't actually send the error right now. If we did, the sequencer
    // might tell us to continue on to the next file before we've finished
    // processing shutdown for this stream.
    // Instead, we just cache it here, and actually process it once shutdown
    // is complete.
    mMediacoreError = error;
  }

  // Build an error message to output to the console
  // TODO: This is currently not localised (but we're probably not setting
  // things up right to get translated gstreamer messages anyway).
  nsString errmessage = NS_LITERAL_STRING("GStreamer error: ");
  errmessage.Append(NS_ConvertUTF8toUTF16(gerror->message));
  errmessage.Append(NS_LITERAL_STRING(" Additional information: "));
  errmessage.Append(NS_ConvertUTF8toUTF16(debugMessage));

  g_error_free (gerror);
  g_free (debugMessage);

  // Then, shut down the pipeline, which will cause
  // a STREAM_END event to be fired. Immediately before firing that, we'll
  // send our error message.
  GstElement *pipeline;
  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
    mTargetState = GST_STATE_NULL;
    pipeline = (GstElement *)g_object_ref (mPipeline);
  }

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);

  // Log the error message
  sbErrorConsole::Error("Mediacore:GStreamer", errmessage);
}

void sbGStreamerMediacore::HandleWarningMessage(GstMessage *message)
{
  GError *gerror = NULL;
  gchar *debugMessage;

  NS_ASSERTION(NS_IsMainThread(), "not on main thread");

  gst_message_parse_warning(message, &gerror, &debugMessage);

  // TODO: This is currently not localised (but we're probably not setting
  // things up right to get translated gstreamer messages anyway).
  nsString warning = NS_LITERAL_STRING("GStreamer warning: ");
  warning.Append(NS_ConvertUTF8toUTF16(gerror->message));
  warning.Append(NS_LITERAL_STRING(" Additional information: "));
  warning.Append(NS_ConvertUTF8toUTF16(debugMessage));

  g_error_free (gerror);
  g_free (debugMessage);

  sbErrorConsole::Error("Mediacore:GStreamer", warning);
}

/* Dispatch messages based on type.
 * For ELEMENT messages, further introspect the exact meaning for
 * dispatch
 */
void sbGStreamerMediacore::HandleMessage (GstMessage *message)
{
  GstMessageType msg_type;
  msg_type = GST_MESSAGE_TYPE(message);

  LOG(("Got message: %s", gst_message_type_get_name(msg_type)));

  switch (msg_type) {
    case GST_MESSAGE_STATE_CHANGED:
      HandleStateChangedMessage(message);
      break;
    case GST_MESSAGE_TAG:
      HandleTagMessage(message);
      break;
    case GST_MESSAGE_ERROR:
      HandleErrorMessage(message);
      break;
    case GST_MESSAGE_WARNING:
      HandleWarningMessage(message);
      break;
    case GST_MESSAGE_EOS:
      HandleEOSMessage(message);
      break;
    case GST_MESSAGE_BUFFERING:
      HandleBufferingMessage(message);
    case GST_MESSAGE_ELEMENT: {
      if (gst_structure_has_name (message->structure, "redirect")) {
        HandleRedirectMessage(message);
      } else if (gst_is_missing_plugin_message(message)) {
        HandleMissingPluginMessage(message);
      }
      break;
    }
    default:
      LOG(("Got message: %s", gst_message_type_get_name(msg_type)));
      break;
  }
}

/* Main-thread only! */
void sbGStreamerMediacore::RequestVideoWindow()
{
  nsresult rv;
  PRUint32 videoWidth = 0;
  PRUint32 videoHeight = 0;

  nsCOMPtr<sbIMediacoreManager> mediacoreManager =
      do_GetService(SB_MEDIACOREMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  if (mVideoSize) {
    rv = mVideoSize->GetWidth(&videoWidth);
    NS_ENSURE_SUCCESS(rv, /* void */);
    rv = mVideoSize->GetHeight(&videoHeight);
    NS_ENSURE_SUCCESS(rv, /* void */);

    PRUint32 parNum, parDenom;
    rv = mVideoSize->GetParNumerator(&parNum);
    NS_ENSURE_SUCCESS(rv, /* void */);
    rv = mVideoSize->GetParDenominator(&parDenom);
    NS_ENSURE_SUCCESS(rv, /* void */);

    // We adjust width rather than height as adjusting height tends to make
    // interlacing artifacts worse.
    videoWidth = videoWidth * parNum / parDenom;
  }

  nsCOMPtr<sbIMediacoreVideoWindow> videoWindow;
  rv = mediacoreManager->GetPrimaryVideoWindow(PR_TRUE, videoWidth, videoHeight,
                                               getter_AddRefs(videoWindow));
  NS_ENSURE_SUCCESS(rv, /* void */);

  if (videoWindow != NULL) {
    nsCOMPtr<nsIDOMXULElement> videoDOMElement;
    rv = videoWindow->GetVideoWindow(getter_AddRefs(videoDOMElement));
    NS_ENSURE_SUCCESS(rv, /* void */);

    /* Set our video window. This will ensure that SetVideoBox
       is called on the platform interface, so it can then use that. */
    rv = SetVideoWindow(videoDOMElement);
    NS_ENSURE_SUCCESS(rv, /* void */);

    DispatchMediacoreEvent(sbIMediacoreEvent::VIDEO_SIZE_CHANGED,
                           sbNewVariant(mVideoSize).get());
  }
}

void
sbGStreamerMediacore::OnVideoCapsSet(GstCaps *caps)
{
  GstStructure *s;
  gint pixelAspectRatioN, pixelAspectRatioD;
  gint videoWidth, videoHeight;

  s = gst_caps_get_structure(caps, 0);
  if(s) {
    gst_structure_get_int(s, "width", &videoWidth);
    gst_structure_get_int(s, "height", &videoHeight);

    /* pixel-aspect-ratio is optional */
    const GValue* par = gst_structure_get_value(s, "pixel-aspect-ratio");
    if (par) {
      pixelAspectRatioN = gst_value_get_fraction_numerator(par);
      pixelAspectRatioD = gst_value_get_fraction_denominator(par);
    }
    else {
      /* PAR not set; default to square pixels */
      pixelAspectRatioN = pixelAspectRatioD = 1;
    }

    if (mPlatformInterface) {
      int num = videoWidth * pixelAspectRatioN;
      int denom = videoHeight * pixelAspectRatioD;
      mPlatformInterface->SetDisplayAspectRatio(num, denom);
    }
  }
  else {
    // Should never be reachable, but let's be defensive here...
    videoHeight = 240;
    videoWidth = 320;
    pixelAspectRatioN = pixelAspectRatioD = 0;
  }

  // We don't do gapless playback if video is involved. If we're already in
  // gapless mode (this is the 2nd-or-subsequent resource), abort and try again
  // in non-gapless mode. If it's the first resource, just flag that we
  // shouldn't attempt gapless playback.
  if (mPlayingGaplessly) {
    // Ignore all messages while we're aborting playback
    mAbortingPlayback = PR_TRUE;
    nsCOMPtr<nsIRunnable> abort = new nsRunnableMethod_AbortAndRestartPlayback(this);
    NS_DispatchToMainThread(abort);
  }

  mGaplessDisabled = PR_TRUE;

  // Send VIDEO_SIZE_CHANGED event
  nsRefPtr<sbVideoBox> videoBox;
  videoBox = new sbVideoBox;
  NS_ENSURE_TRUE(videoBox, /*void*/);

  nsresult rv = videoBox->Init(videoWidth,
                               videoHeight,
                               pixelAspectRatioN,
                               pixelAspectRatioD);
  NS_ENSURE_SUCCESS(rv, /*void*/);

  DispatchMediacoreEvent(sbIMediacoreEvent::VIDEO_SIZE_CHANGED,
                         sbNewVariant(videoBox).get());

  mVideoSize = do_QueryInterface(videoBox, &rv);
  NS_ENSURE_SUCCESS(rv, /*void*/);
}

void
sbGStreamerMediacore::OnAudioCapsSet(GstCaps *caps)
{
  if (mPlayingGaplessly && mCurrentAudioCaps &&
          !gst_caps_is_equal_fixed (caps, mCurrentAudioCaps))
  {
    // Ignore all messages while we're aborting playback
    mAbortingPlayback = PR_TRUE;
    nsCOMPtr<nsIRunnable> abort = new nsRunnableMethod_AbortAndRestartPlayback(this);
    NS_DispatchToMainThread(abort);
  }

  if (mCurrentAudioCaps) {
    gst_caps_unref (mCurrentAudioCaps);
  }
  mCurrentAudioCaps = gst_caps_ref (caps);
}

void sbGStreamerMediacore::AbortAndRestartPlayback()
{
  nsresult rv = Stop();
  NS_ENSURE_SUCCESS (rv, /* void */);
  mAbortingPlayback = PR_FALSE;

  rv = CreatePlaybackPipeline();
  NS_ENSURE_SUCCESS (rv, /* void */);

  g_object_set (G_OBJECT (mPipeline), "uri", mCurrentUri.get(), NULL);

  // Apply volume/mute to our new pipeline
  if (mMute) {
    g_object_set (G_OBJECT (mPipeline), "volume", 0.0, NULL);
  }
  else {
    g_object_set (G_OBJECT (mPipeline), "volume", mVolume, NULL);
  }

  rv = Play();
  NS_ENSURE_SUCCESS (rv, /* void */);
}

//-----------------------------------------------------------------------------
// sbBaseMediacore
//-----------------------------------------------------------------------------

/*virtual*/ nsresult
sbGStreamerMediacore::OnInitBaseMediacore()
{
  nsresult rv;

  // Ensure the service component is loaded; it initialises GStreamer for us.
  nsCOMPtr<sbIGStreamerService> service =
    do_GetService(SBGSTREAMERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnGetCapabilities()
{
  // XXXAus: Implement this when implementing the default sequencer!
  return NS_ERROR_NOT_IMPLEMENTED;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnShutdown()
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  if (mPipeline) {
    LOG (("Destroying pipeline on shutdown"));
    DestroyPipeline();
  }

  if (mPrefs) {
    nsresult rv = mPrefs->RemoveObserver("songbird.mediacore.gstreamer", this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbBaseMediacoreMultibandEqualizer
//-----------------------------------------------------------------------------
/*virtual*/ nsresult
sbGStreamerMediacore::OnInitBaseMediacoreMultibandEqualizer()
{
  mEqualizerElement =
    gst_element_factory_make (EQUALIZER_FACTORY_NAME, NULL);
  NS_WARN_IF_FALSE(mEqualizerElement, "No support for equalizer.");

  if (mEqualizerElement) {
    // Ref and sink the object to take ownership; we'll keep track of it
    // from here on.
    gst_object_ref (mEqualizerElement);
    gst_object_sink (mEqualizerElement);

    // Set the bands to the frequencies we want
    char band[16] = {0};

    GValue freqVal = {0};
    g_value_init (&freqVal, G_TYPE_DOUBLE);

    for(PRUint32 i = 0; i < EQUALIZER_DEFAULT_BAND_COUNT; ++i) {
      PR_snprintf (band, 16, "band%i::freq", i);
      g_value_set_double (&freqVal, EQUALIZER_BANDS[i]);
      gst_child_proxy_set_property (GST_OBJECT (mEqualizerElement),
                                    band,
                                    &freqVal);
    }

    g_value_unset (&freqVal);

    AddAudioFilter(mEqualizerElement);
  }

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnSetEqEnabled(PRBool aEqEnabled)
{
  // Not necessarily an error if we don't have an Equalizer Element.
  // The plugin may simply be missing from the user's installation.
  if(!mEqualizerElement) {
    return NS_OK;
  }

  // Disable gain on bands if we're being disabled.
  if(!aEqEnabled) {
    char band[8] = {0};
    double bandGain = 0;

    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

    for(PRUint32 i = 0; i < EQUALIZER_DEFAULT_BAND_COUNT; ++i) {
      PR_snprintf (band, 8, "band%i", i);
      g_object_set (G_OBJECT (mEqualizerElement), band, bandGain, NULL);
    }
  }

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnGetBandCount(PRUint32 *aBandCount)
{
  NS_ENSURE_ARG_POINTER(aBandCount);

  *aBandCount = 0;

  // Not necessarily an error if we don't have an Equalizer Element.
  // The plugin may simply be missing from the user's installation.
  if (!mEqualizerElement) {
    return NS_OK;
  }

  *aBandCount = EQUALIZER_DEFAULT_BAND_COUNT;

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnGetBand(PRUint32 aBandIndex, sbIMediacoreEqualizerBand *aBand)
{
  NS_ENSURE_ARG_POINTER(aBand);
  NS_ENSURE_ARG_RANGE(aBandIndex, 0, EQUALIZER_DEFAULT_BAND_COUNT - 1);

  // Not necessarily an error if we don't have an Equalizer Element.
  // The plugin may simply be missing from the user's installation.
  if (!mEqualizerElement) {
    return NS_OK;
  }

  char band[8] = {0};
  PR_snprintf(band, 8, "band%i", aBandIndex);

  gdouble bandGain = 0.0;
  g_object_get (G_OBJECT (mEqualizerElement), band, &bandGain, NULL);

  nsresult rv = aBand->Init(aBandIndex, EQUALIZER_BANDS[aBandIndex], bandGain);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnSetBand(sbIMediacoreEqualizerBand *aBand)
{
  NS_ENSURE_ARG_POINTER(aBand);

  // Not necessarily an error if we don't have an Equalizer Element.
  // The plugin may simply be missing from the user's installation.
  if (!mEqualizerElement) {
    return NS_OK;
  }

  PRUint32 bandIndex = 0;
  nsresult rv = aBand->GetIndex(&bandIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  double bandGain = 0.0;
  rv = aBand->GetGain(&bandGain);
  NS_ENSURE_SUCCESS(rv, rv);

  // Clamp and recenter.
  bandGain = (12.0 * SB_ClampDouble(bandGain, -1.0, 1.0));

  char band[8] = {0};
  PR_snprintf(band, 8, "band%i", bandIndex);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  g_object_set (G_OBJECT (mEqualizerElement), band, bandGain, NULL);

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbBaseMediacorePlaybackControl
//-----------------------------------------------------------------------------

/*virtual*/ nsresult
sbGStreamerMediacore::OnInitBaseMediacorePlaybackControl()
{
  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnSetUri(nsIURI *aURI)
{
  nsCAutoString spec;
  nsresult rv;

  // createplaybackpipeline will acquire the monitor.
  rv = CreatePlaybackPipeline();
  NS_ENSURE_SUCCESS (rv,rv);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  rv = aURI->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = GetFileSize (aURI, &mResourceSize);
  if (rv == NS_ERROR_NO_INTERFACE) {
    // Not being a file is fine - that's just something non-local
    mResourceIsLocal = PR_FALSE;
    mResourceSize = -1;
  }
  else
    mResourceIsLocal = PR_TRUE;

  LOG(("Setting URI to \"%s\"", spec.get()));

  /* Set the URI to play */
  g_object_set (G_OBJECT (mPipeline), "uri", spec.get(), NULL);
  mCurrentUri = spec;

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnGetDuration(PRUint64 *aDuration)
{
  GstQuery *query;
  gboolean res;
  nsresult rv;
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  if (!mPipeline)
    return NS_ERROR_NOT_AVAILABLE;

  query = gst_query_new_duration(GST_FORMAT_TIME);
  res = gst_element_query(mPipeline, query);

  if(res) {
    gint64 duration;
    gst_query_parse_duration(query, NULL, &duration);

    if ((GstClockTime)duration == GST_CLOCK_TIME_NONE) {
      /* Something erroneously returned TRUE for this query
       * despite not giving us a duration. Treat the same as
       * a failed query */
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
      /* Convert to milliseconds */
      *aDuration = duration / GST_MSECOND;
      rv = NS_OK;
    }
  }
  else
    rv = NS_ERROR_NOT_AVAILABLE;

  gst_query_unref (query);

  return rv;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnGetPosition(PRUint64 *aPosition)
{
  GstQuery *query;
  gboolean res;
  nsresult rv;
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  if (!mPipeline)
    return NS_ERROR_NOT_AVAILABLE;

  query = gst_query_new_position(GST_FORMAT_TIME);
  res = gst_element_query(mPipeline, query);

  if(res) {
    gint64 position;
    gst_query_parse_position(query, NULL, &position);

    if (position == 0 || (GstClockTime)position == GST_CLOCK_TIME_NONE) {
      // GStreamer bugs can cause us to get a position of zero when we in fact
      // don't know the current position. A real position of zero is unlikely
      // and transient, so we just treat this as unknown.
      // A value of -1 (GST_CLOCK_TIME_NONE) is also 'not available', though
      // really that should be reported as a a false return from the query.
      rv = NS_ERROR_NOT_AVAILABLE;
    }
    else {
      /* Convert to milliseconds */
      *aPosition = position / GST_MSECOND;
      rv = NS_OK;
    }
  }
  else
    rv = NS_ERROR_NOT_AVAILABLE;

  gst_query_unref (query);

  return rv;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnSetPosition(PRUint64 aPosition)
{
  return Seek(aPosition, sbIMediacorePlaybackControl::SEEK_FLAG_NORMAL);
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnSeek(PRUint64 aPosition, PRUint32 aFlags)
{
  GstClockTime position;
  gboolean ret;
  GstSeekFlags flags;

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  // Incoming position is in milliseconds, convert to GstClockTime (nanoseconds)
  position = aPosition * GST_MSECOND;

  // For gapless to work after seeking in MP3, we need to do accurate seeking.
  // However, accurate seeks on large files is very slow.
  // So, we do a KEY_UNIT seek (the fastest sort) unless it's all three of:
  //  - a local file
  //  - sufficiently small
  //  - flag passed is to do a normal seek

  if (mResourceIsLocal &&
      mResourceSize <= MAX_FILE_SIZE_FOR_ACCURATE_SEEK &&
      aFlags == SEEK_FLAG_NORMAL)
  {
    flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE);
  }
  else {
    flags = (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT);
  }

  ret = gst_element_seek_simple (mPipeline, GST_FORMAT_TIME,
      flags, position);

  if (!ret) {
    /* TODO: Is this appropriate for a non-fatal failure to seek? Should we
       fire an event? */
    return NS_ERROR_FAILURE;
  }


  // Set state to PAUSED. Leave mTargetState as it is, so if we did a seek when
  // playing, we'll return to playing (after rebuffering if required)
  ret = gst_element_set_state (mPipeline, GST_STATE_PAUSED);

  if (ret == GST_STATE_CHANGE_FAILURE)
    return NS_ERROR_FAILURE;

  nsresult rv = SendInitialBufferingEvent();
  NS_ENSURE_SUCCESS (rv, rv);

  return rv;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnGetIsPlayingAudio(PRBool *aIsPlayingAudio)
{
  if (mTargetState == GST_STATE_NULL) {
    *aIsPlayingAudio = PR_FALSE;
  }
  else {
    *aIsPlayingAudio = mHasAudio;
  }

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnGetIsPlayingVideo(PRBool *aIsPlayingVideo)
{
  if (mTargetState == GST_STATE_NULL) {
    *aIsPlayingVideo = PR_FALSE;
  }
  else {
    *aIsPlayingVideo = mHasVideo;
  }

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnPlay()
{
  GstStateChangeReturn ret;
  GstState curstate;

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  NS_ENSURE_STATE(mPipeline);

  gst_element_get_state (mPipeline, &curstate, NULL, 0);

  mTargetState = GST_STATE_PLAYING;

  if (curstate == GST_STATE_PAUSED && !mBuffering) {
    // If we're already paused, then go directly to PLAYING, unless
    // we're still waiting for buffering to complete.
    ret = gst_element_set_state (mPipeline, GST_STATE_PLAYING);
  }
  else {
    // Otherwise, we change our state to PAUSED (our target state is
    // PLAYING, though). Then, when we reach PAUSED, we'll either
    // continue on to PLAYING, or (if we're buffering) wait for buffering
    // to complete.
    ret = gst_element_set_state (mPipeline, GST_STATE_PAUSED);

    nsresult rv = SendInitialBufferingEvent();
    NS_ENSURE_SUCCESS (rv, rv);
  }

  /* Usually ret will be GST_STATE_CHANGE_ASYNC, but we could get a synchronous
   * error - however, in such a case we'll always still receive an error event,
   * so we just return OK here, and then fail playback once we process the
   * error event. */
  if (ret == GST_STATE_CHANGE_FAILURE)
    return NS_OK;
  else if (ret == GST_STATE_CHANGE_NO_PREROLL)
  {
    /* NO_PREROLL means we have a live pipeline, for which we have to
     * handle buffering differently */
    mIsLive = PR_TRUE;
  }

  return NS_OK;
}

nsresult
sbGStreamerMediacore::SendInitialBufferingEvent()
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  // If we're starting an HTTP stream, send an immediate buffering event,
  // since GStreamer won't do that until it's connected to the server.
  PRBool schemeIsHttp;
  nsresult rv = mUri->SchemeIs("http", &schemeIsHttp);
  NS_ENSURE_SUCCESS (rv, rv);

  if (schemeIsHttp) {
    double bufferingProgress = 0.0;
    nsCOMPtr<nsIVariant> variant = sbNewVariant(bufferingProgress).get();
    DispatchMediacoreEvent(sbIMediacoreEvent::BUFFERING, variant);
  }

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnPause()
{
  GstStateChangeReturn ret;

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  NS_ENSURE_STATE(mPipeline);

  mTargetState = GST_STATE_PAUSED;
  ret = gst_element_set_state (mPipeline, GST_STATE_PAUSED);

  if (ret == GST_STATE_CHANGE_FAILURE)
    return NS_ERROR_FAILURE;
  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnStop()
{
  GstElement *pipeline;

  {
    mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
    mTargetState = GST_STATE_NULL;
    mStopped = PR_TRUE;
    // If we get stopped without ever starting, that's ok...
    if (!mPipeline)
      return NS_OK;

    pipeline = (GstElement *)g_object_ref (mPipeline);
  }

  gst_element_set_state (pipeline, GST_STATE_NULL);
  g_object_unref (pipeline);

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbBaseMediacoreVolumeControl
//-----------------------------------------------------------------------------

/*virtual*/ nsresult
sbGStreamerMediacore::OnInitBaseMediacoreVolumeControl()
{
  mVolume = 1.0;
  mMute = PR_FALSE;

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnSetMute(PRBool aMute)
{
  mozilla::ReentrantMonitorAutoEnter lock(mMonitor);

  NS_ENSURE_STATE(mPipeline);

  if(!aMute && mMute) {
    mozilla::ReentrantMonitorAutoEnter mon(sbBaseMediacoreVolumeControl::mMonitor);

    /* Well, this is nice and easy! */
    g_object_set(mPipeline, "volume", mVolume, NULL);
  }
  else if(aMute && !mMute){
    /* We have no explicit mute control, so just set the volume to zero, but
    * don't update our internal mVolume value */
    g_object_set(mPipeline, "volume", 0.0, NULL);
  }

  return NS_OK;
}

/*virtual*/ nsresult
sbGStreamerMediacore::OnSetVolume(double aVolume)
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  NS_ENSURE_STATE(mPipeline);

  /* Well, this is nice and easy! */
  g_object_set(mPipeline, "volume", aVolume, NULL);

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbIMediacoreVotingParticipant
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbGStreamerMediacore::VoteWithURI(nsIURI *aURI, PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(aURI);
  NS_ENSURE_ARG_POINTER(_retval);

  // XXXAus: Run aURI through extension filtering first.
  //
  //         After that, that's as much as we can do, it's most likely
  //         playable.

  *_retval = 2000;

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediacore::VoteWithChannel(nsIChannel *aChannel, PRUint32 *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

//-----------------------------------------------------------------------------
// sbIMediacoreVideoWindow
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbGStreamerMediacore::GetFullscreen(PRBool *aFullscreen)
{
  NS_ENSURE_ARG_POINTER(aFullscreen);

  if (mPlatformInterface) {
    *aFullscreen = mPlatformInterface->GetFullscreen();
    return NS_OK;
  }
  else {
    // Not implemented for this platform
    return NS_ERROR_NOT_IMPLEMENTED;
  }
}

NS_IMETHODIMP
sbGStreamerMediacore::SetFullscreen(PRBool aFullscreen)
{
  if (mPlatformInterface) {
    mPlatformInterface->SetFullscreen(aFullscreen);
    return NS_OK;
  }
  else {
    // Not implemented for this platform
    return NS_ERROR_NOT_IMPLEMENTED;
  }
}

NS_IMETHODIMP
sbGStreamerMediacore::GetVideoWindow(nsIDOMXULElement **aVideoWindow)
{
  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);
  NS_IF_ADDREF(*aVideoWindow = mVideoWindow);

  return NS_OK;
}

NS_IMETHODIMP
sbGStreamerMediacore::SetVideoWindow(nsIDOMXULElement *aVideoWindow)
{
  NS_ENSURE_ARG_POINTER(aVideoWindow);

  mozilla::ReentrantMonitorAutoEnter mon(mMonitor);

  // Get the box object representing the actual display area for the video.
  nsCOMPtr<nsIBoxObject> boxObject;
  nsresult rv = aVideoWindow->GetBoxObject(getter_AddRefs(boxObject));
  NS_ENSURE_SUCCESS (rv, rv);

  nsCOMPtr<nsIDOMDocument> domDocument;
  rv = aVideoWindow->GetOwnerDocument(getter_AddRefs(domDocument));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIWebNavigation> webNavigation(do_GetInterface(domDocument));
  nsCOMPtr<nsIDocShellTreeItem> docShellTreeItem(do_QueryInterface(webNavigation));
  NS_ENSURE_TRUE(docShellTreeItem, NS_NOINTERFACE);

  nsCOMPtr<nsIDocShellTreeOwner> docShellTreeOwner;
  rv = docShellTreeItem->GetTreeOwner(getter_AddRefs(docShellTreeOwner));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIBaseWindow> baseWindow = do_QueryInterface(docShellTreeOwner);
  NS_ENSURE_TRUE(baseWindow, NS_NOINTERFACE);

  nsCOMPtr<nsIWidget> widget;
  rv = baseWindow->GetMainWidget(getter_AddRefs(widget));
  NS_ENSURE_SUCCESS(rv, rv);

  // Attach event listeners
  nsCOMPtr<nsIDocument> document(do_QueryInterface(domDocument));
  NS_ENSURE_TRUE(document, NS_NOINTERFACE);

  mDOMWindow = do_QueryInterface(document->GetScriptGlobalObject());
  NS_ENSURE_TRUE(mDOMWindow, NS_NOINTERFACE);

  nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mDOMWindow));
  NS_ENSURE_TRUE(target, NS_NOINTERFACE);
  target->AddEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
  target->AddEventListener(NS_LITERAL_STRING("unload"), this, PR_FALSE);
  target->AddEventListener(NS_LITERAL_STRING("hide"), this, PR_FALSE);

  NS_WARN_IF_FALSE(NS_IsMainThread(), "Wrong Thread!");

  mVideoWindow = aVideoWindow;

  if (mPlatformInterface) {
    rv = mPlatformInterface->SetVideoBox(boxObject, widget);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mPlatformInterface->SetDocument(domDocument);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbIGStreamerMediacore
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbGStreamerMediacore::GetGstreamerVersion(nsAString& aGStreamerVersion)
{
  nsString versionString;

  versionString.AppendInt(GST_VERSION_MAJOR);
  versionString.AppendLiteral(".");
  versionString.AppendInt(GST_VERSION_MINOR);
  versionString.AppendLiteral(".");
  versionString.AppendInt(GST_VERSION_MICRO);

  aGStreamerVersion.Assign(versionString);

  return NS_OK;
}

// Forwarding functions for sbIMediacoreEventTarget interface

NS_IMETHODIMP
sbGStreamerMediacore::DispatchEvent(sbIMediacoreEvent *aEvent,
                                    PRBool aAsync,
                                    PRBool* _retval)
{
  return mBaseEventTarget ?
         mBaseEventTarget->DispatchEvent(aEvent, aAsync, _retval) :
         NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbGStreamerMediacore::AddListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ?
         mBaseEventTarget->AddListener(aListener) :
         NS_ERROR_NULL_POINTER;
}

NS_IMETHODIMP
sbGStreamerMediacore::RemoveListener(sbIMediacoreEventListener *aListener)
{
  return mBaseEventTarget ?
         mBaseEventTarget->RemoveListener(aListener) :
         NS_ERROR_NULL_POINTER;
}


//-----------------------------------------------------------------------------
// nsIDOMEventListener
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbGStreamerMediacore::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);

  if(eventType.EqualsLiteral("unload") ||
     eventType.EqualsLiteral("hide")) {

    // Clean up here
    nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mDOMWindow));
    NS_ENSURE_TRUE(target, NS_NOINTERFACE);
    target->RemoveEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING("unload"), this, PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING("hide"), this, PR_FALSE);

    mDOMWindow = nsnull;
    mVideoWindow = nsnull;

    if (mPlatformInterface) {
      // Clear the video box/widget used by the platform-specific code.
      nsresult rv = mPlatformInterface->SetVideoBox(nsnull, nsnull);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if(eventType.EqualsLiteral("resize") &&
          mPlatformInterface) {
    mPlatformInterface->ResizeToWindow();
  }

  return NS_OK;
}


//-----------------------------------------------------------------------------
// nsIObserver
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbGStreamerMediacore::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
  if (!strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic)) {
    nsresult rv = ReadPreferences();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbIGstAudioFilter
//-----------------------------------------------------------------------------
nsresult
sbGStreamerMediacore::AddAudioFilter(GstElement *aElement)
{
  // Hold a reference to the element
  gst_object_ref (aElement);

  mAudioFilters.push_back(aElement);

  return NS_OK;
}

nsresult
sbGStreamerMediacore::RemoveAudioFilter(GstElement *aElement)
{
  mAudioFilters.erase(
          std::remove(mAudioFilters.begin(), mAudioFilters.end(), aElement));

  gst_object_unref (aElement);

  return NS_OK;
}



