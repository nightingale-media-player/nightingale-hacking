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

#ifndef _SB_GSTREAMER_VIDEO_TRANSCODE_H_
#define _SB_GSTREAMER_VIDEO_TRANSCODE_H_

#include <nsAutoLock.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsITimer.h>
#include <nsTArray.h>
#include <nsStringGlue.h>

#include <gst/gst.h>

#include "sbITranscodeVideoJob.h"
#include "sbIJobProgress.h"
#include "sbIJobCancelable.h"
#include "sbITranscodingConfigurator.h"
#include "sbIMediaFormatMutable.h"

#include "sbGStreamerPipeline.h"

// {227551a3-24dc-42e7-9ab6-9525e989edfd}
#define SB_GSTREAMER_VIDEO_TRANSCODE_CID \
	{ 0x227551a3, 0x24dc, 0x42e7, \
	{ 0x9a, 0xb6, 0x95, 0x25, 0xe9, 0x89, 0xed, 0xfd } }

#define SB_GSTREAMER_VIDEO_TRANSCODE_CONTRACTID \
    "@songbirdnest.com/Songbird/Mediacore/Transcode/GStreamerVideo;1"
#define SB_GSTREAMER_VIDEO_TRANSCODE_CLASSNAME  "GStreamerVideoTranscode"


class sbGStreamerVideoTranscoder : public sbGStreamerPipeline,
                                   public sbITranscodeVideoJob,
                                   public sbIJobProgressTime,
                                   public sbIJobCancelable,
                                   public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBITRANSCODEVIDEOJOB
  NS_DECL_SBIJOBPROGRESS
  NS_DECL_SBIJOBPROGRESSTIME
  NS_DECL_SBIJOBCANCELABLE
  NS_DECL_NSITIMERCALLBACK

  sbGStreamerVideoTranscoder();

  NS_IMETHOD BuildPipeline();
  NS_IMETHOD PlayPipeline();
  NS_IMETHOD StopPipeline();

private:
  virtual ~sbGStreamerVideoTranscoder();

  nsresult OnJobProgress();

  void HandleErrorMessage(GstMessage *message);
  void HandleEOSMessage(GstMessage *message);

  GstClockTime QueryPosition();
  GstClockTime QueryDuration();

  nsresult StartProgressReporting();
  nsresult StopProgressReporting();

  void AsyncStopPipeline();

  /* Build initial, partial, transcoding pipeline. The bulk of the pipeline is
     built dynamically when the pipeline is started via a call to Transcode().
   */
  nsresult BuildTranscodePipeline (const gchar *pipelineName);

  /* Clean up references to things that were in a pipeline after we're done
     with the pipeline.
     After this is called, the object is reusable via a new call to Transcode()
   */
  nsresult CleanupPipeline();

  /* Clean up references to pads inside the pipeline */
  void CleanupPads();

  /* Clear the status and any error messages. Called at the start of a
     transcode attempt, to ensure that the status is set correctly and that
     any errors resulting are as a result of _this_ transcode attempt */
  nsresult ClearStatus();

  /* Initialize the configurator object to decide on what format and other
     details to transcode to. */
  nsresult InitializeConfigurator ();

  /* Helper function to set an sbIMediaFormatVideo from some GStreamer caps */
  nsresult SetVideoFormatFromCaps (sbIMediaFormatVideoMutable *format,
                                   GstCaps *caps);

  /* Helper function to set an sbIMediaFormatAudio from some GStreamer caps */
  nsresult SetAudioFormatFromCaps (sbIMediaFormatAudioMutable *format,
                                   GstCaps *caps);

  /* Call to send an error event, and shut down the pipeline, if a fatal error
     is encountered during transcoding pipeline setup. The errorName is looked
     up in the properties for localisation */
  void TranscodingFatalError (const char *errorName);

  /* Static helpers that simply forward to the relevant instance methods */
  static void decodebin_pad_added_cb (GstElement *element, GstPad *pad,
                                      sbGStreamerVideoTranscoder *transcoder);
  static void decodebin_no_more_pads_cb (
          GstElement *element,
          sbGStreamerVideoTranscoder *transcoder);
  static void pad_blocked_cb (GstPad *pad, gboolean blocked, 
                              sbGStreamerVideoTranscoder *transcoder);
  static void pad_notify_caps_cb (GObject *obj, GParamSpec *pspec,
                                  sbGStreamerVideoTranscoder *transcoder);

  /* Called when the decoder finds and decodes a stream.
     Used to find which streams we're interested in using for our transcoded
     output.
   */
  nsresult DecoderPadAdded(GstElement *uridecodebin, GstPad *pad);

  /* Called when the decoder does not expect to find any more streams.
     We will ignore any additional streams found after this.
     Here, we set things up to wait until data is actually flowing on all
     streams via pad blocks. */
  nsresult DecoderNoMorePads(GstElement *uridecodebin);

  /* Called when pad block has fired. */
  nsresult PadBlocked (GstPad *pad, gboolean blocked);

  /* Called when we're notified that a pad's caps have been set */
  nsresult PadNotifyCaps (GstPad *pad);

  /* Check if we have caps on all our pads yet, and continue on to build the
     full pipeline if so */
  nsresult CheckForAllCaps ();

  /* Helper to get the proper caps from a pad */
  GstCaps *GetCapsFromPad (GstPad *pad);

  /* Actually build the rest of the pipeline! */
  nsresult BuildRemainderOfPipeline();

  /* Add the audio bin (buffering, conversion, and encoding). Link to
     'inputAudioSrcPad', which is the pad provided by the decodebin.
     The src pad from the audio bin is returned in 'outputAudioSrcPad' */
  nsresult AddAudioBin (GstPad *inputAudioSrcPad, GstPad **outputAudioSrcPad);

  /* Build the actual audio bin. If needed, the input caps (i.e. the caps of the
     decoded data from the decoder) are provided in 'inputAudioCaps'.
     Return the new bin in 'audioBin' */
  nsresult BuildAudioBin (GstCaps *inputAudioCaps, GstElement **audioBin);

  /* Add the video bin (buffering, conversion, and encoding). Link to
     'inputVideoSrcPad', which is the pad provided by the decodebin.
     The src pad from the video bin is returned in 'outputVideoSrcPad' */
  nsresult AddVideoBin (GstPad *inputVideoSrcPad, GstPad **outputVideoSrcPad);

  /* Build the actual video bin. If needed, the input caps (i.e. the caps of the
     decoded data from the decoder) are provided in 'inputVideoCaps'.
     Return the new bin in 'videoBin' */
  nsresult BuildVideoBin (GstCaps *inputVideoCaps, GstElement **videoBin);

  /* Add a muxer, if required. Return the src pad that should be used (whether
     that's a muxer src pad, or actually the src pad directly from the audio
     or video encoder) in 'muxerSrcPad'.
     Pads to link to it are passed in 'audioPad' and 'videoPad' either of which
     might be NULL.
   */
  nsresult AddMuxer (GstPad **muxerSrcPad, GstPad *audioPad, GstPad *videoPad);

  /* Add an appropriate sink for the data. Link it to 'muxerSrcPad' */
  nsresult AddSink (GstPad *muxerSrcPad);

  /* Create an appropriate sink based on the destination URI set. Return it
     in 'sink' (not initially connected to anything) */
  nsresult CreateSink (GstElement **sink);

  /* Get a pad (either an existing pad, or a newly-requested one) from 'element'
     that is potentially compatible with 'pad' */
  GstPad * GetCompatiblePad (GstElement *element, GstPad *pad);

  /* Get a pad (either an existing pad or a new request pad) from element's
     pad template 'templ' */
  GstPad * GetPadFromTemplate (GstElement *element, GstPadTemplate *templ);

  /* Helper function to configure the videobox element, based on input
     caps, and specified output geometry */
  void ConfigureVideoBox (GstElement *videobox, GstCaps *aInputVideoCaps,
      gint outputWidth, gint outputHeight, gint outputParN, gint outputParD);

  /* Set the metadata on any tag setters in the pipeline */
  nsresult SetMetadataOnTagSetters();

  /* Helper to add an image to a tag list for metadata */
  nsresult AddImageToTagList(GstTagList *aTags, nsIInputStream *aStream);

  nsCOMPtr<sbIPropertyArray>              mMetadata;
  nsCOMPtr<nsIInputStream>                mImageStream;
  nsCOMPtr<sbITranscodingConfigurator>    mConfigurator;

  nsString                                mSourceURI;
  nsString                                mDestURI;

  PRUint16                                mStatus;
  nsTArray<nsString>                      mErrorMessages;

  nsCOMArray<sbIJobProgressListener>      mProgressListeners;
  nsCOMPtr<nsITimer>                      mProgressTimer;

  PRBool                                  mPipelineBuilt;
  PRBool                                  mWaitingForCaps;

  GstPad                                 *mAudioSrc;
  GstPad                                 *mVideoSrc;
  GstPad                                 *mAudioQueueSrc;
  GstPad                                 *mVideoQueueSrc;

  // Lock to prevent trying to build the pipeline concurrently from multiple
  // threads.
  PRLock                                 *mBuildLock;

protected:
  /* additional members */
};

#endif // _SB_GSTREAMER_TRANSCODE_H_
