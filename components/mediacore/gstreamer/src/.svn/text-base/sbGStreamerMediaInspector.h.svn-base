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

#ifndef _SB_GSTREAMER_MEDIAINSPECTOR_H_
#define _SB_GSTREAMER_MEDIAINSPECTOR_H_

#include <nsCOMPtr.h>
#include <nsTArray.h>
#include <nsStringGlue.h>
#include <nsIClassInfo.h>
#include <nsITimer.h>

#include "sbIJobProgress.h"
#include "sbIJobCancelable.h"
#include "sbIMediaInspector.h"
#include "sbIMediaFormatMutable.h"

#include "sbGStreamerPipeline.h"

#include <gst/gst.h>

class sbGStreamerMediaInspector : public sbGStreamerPipeline,
                                  public sbIMediaInspector,
                                  public sbIJobProgress,
                                  public sbIJobCancelable,
                                  public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIMEDIAINSPECTOR
  NS_DECL_SBIJOBPROGRESS
  NS_DECL_SBIJOBCANCELABLE
  NS_DECL_NSITIMERCALLBACK

  sbGStreamerMediaInspector();

private:
  virtual ~sbGStreamerMediaInspector();

  nsresult OnJobProgress();

  NS_IMETHOD StopPipeline();
  virtual nsresult BuildPipeline();

  nsresult StartTimeoutTimer();
  nsresult StopTimeoutTimer();
  nsresult CleanupPipeline();
  void     ResetStatus();
  nsresult CompleteInspection();

  nsresult PadAdded(GstPad *srcPad);
  nsresult FakesinkEvent(GstPad *srcPad, GstEvent *event, PRBool isAudio);
  nsresult ProcessPipelineForInfo();
  nsresult ProcessContainerProperties(
             sbIMediaFormatContainerMutable *aContainerFormat,
             GstStructure *aStructure);
  nsresult ProcessVideo(sbIMediaFormatVideo **aVideoFormat);
  nsresult ProcessVideoCaps(sbIMediaFormatVideoMutable *format, GstCaps *caps);
  nsresult ProcessVideoProperties(sbIMediaFormatVideoMutable *aVideoFormat,
                                  GstStructure *aStructure);
  nsresult ProcessAudio(sbIMediaFormatAudio **aAudioFormat);
  nsresult ProcessAudioProperties(sbIMediaFormatAudioMutable *aAudioFormat,
                                  GstStructure *aStructure);
  nsresult InspectorateElement (GstElement *element);

  /* Get the real pad associated with a (possibly ghost) pad */
  GstPad * GetRealPad (GstPad *pad);

  void HandleStateChangeMessage(GstMessage *message);
  void HandleErrorMessage(GstMessage *message);

  static void fakesink_audio_event_cb (GstPad *pad, GstEvent *event,
                                 sbGStreamerMediaInspector *inspector);

  static void fakesink_video_event_cb (GstPad *pad, GstEvent *event,
                                 sbGStreamerMediaInspector *inspector);

  static void decodebin_pad_added_cb (GstElement *element, GstPad *pad,
                                      sbGStreamerMediaInspector *inspector);

  nsCOMPtr<sbIMediaFormatMutable>         mMediaFormat;

  PRUint16                                mStatus;
  nsTArray<nsString>                      mErrorMessages;
  nsCOMArray<sbIJobProgressListener>      mProgressListeners;
  nsCOMPtr<nsITimer>                      mTimeoutTimer;

  nsString                                mSourceURI;

  PRBool                                  mFinished;
  PRBool                                  mIsPaused;
  PRBool                                  mTooComplexForCurrentImplementation;

  GstElement                             *mDecodeBin;
  GstPad                                 *mVideoSrc;
  GstPad                                 *mAudioSrc;

  GstPad                                 *mAudioDecoderSink;
  GstPad                                 *mVideoDecoderSink;
  GstPad                                 *mDemuxerSink;
  guint                                   mAudioBitRate;
  guint                                   mVideoBitRate;

};

#define SB_GSTREAMER_MEDIAINSPECTOR_CLASSNAME \
  "sbGStreamerMediaInspector"
#define SB_GSTREAMER_MEDIAINSPECTOR_DESCRIPTION \
  "Songbird GStreamer Media Inspector"
// SB_MEDIAINSPECTOR_CONTRACTID is defined in sbIMediaInspector.idl/.h
#define SB_GSTREAMER_MEDIAINSPECTOR_CONTRACTID \
  SB_MEDIAINSPECTOR_CONTRACTID
#define SB_GSTREAMER_MEDIAINSPECTOR_CID \
  {0x200782a4, 0x07c6, 0x4ec7, {0xa3, 0x31, 0xb8, 0x88, 0x87, 0x36, 0x80, 0xa6}}

#endif // _SB_GSTREAMER_MEDIAINSPECTOR_H_
