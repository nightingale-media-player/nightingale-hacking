/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

#ifndef _SB_GSTREAMER_AUDIO_PROCESSOR_H_
#define _SB_GSTREAMER_AUDIO_PROCESSOR_H_

#include <nsCOMPtr.h>
#include <nsCOMArray.h>

#include <nsIClassInfo.h>

#include <gst/gst.h>
#include <gst/app/gstappsink.h>
#include <gst/base/gstadapter.h>

#include <sbIMediaItem.h>
#include <sbIMediacoreAudioProcessor.h>
#include <sbIMediacoreAudioProcessorListener.h>

#include "sbGStreamerPipeline.h"

class sbGStreamerAudioProcessor : public sbGStreamerPipeline,
                                  public sbIMediacoreAudioProcessor
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIMEDIACOREAUDIOPROCESSOR

  sbGStreamerAudioProcessor();

private:
  virtual ~sbGStreamerAudioProcessor();

  virtual nsresult BuildPipeline();

  virtual void HandleMessage(GstMessage *message);

  virtual nsresult OnDestroyPipeline(GstElement *pipeline);

  /* Instance methods for GObject signals */
  nsresult DecoderPadAdded (GstElement *uridecodebin, GstPad *pad);
  nsresult DecoderNoMorePads (GstElement *uridecodebin);
  nsresult AppsinkNewBuffer(GstElement *appsink);
  nsresult AppsinkEOS(GstElement *appsink);

  /* Static helpers that simply forward to the relevant instance methods */
  static void decodebin_pad_added_cb (GstElement *element, GstPad *pad,
                                      sbGStreamerAudioProcessor *processor);
  static void decodebin_no_more_pads_cb (GstElement *element,
                                         sbGStreamerAudioProcessor *processor);
  static void appsink_new_buffer_cb (GstElement *element,
                                     sbGStreamerAudioProcessor *processor);
  static void appsink_eos_cb (GstElement *element,
                              sbGStreamerAudioProcessor *processor);

  /* Misc helper methods */

  // Configure capsfilter to match the constraints specified via
  // the sbIMediacoreAudioProcessor interface.
  nsresult ConfigureInitialCapsfilter(GstElement *capsfilter);

  // Called once running, reconfigures capsfilter to the current caps, so that
  // changes in the underlying stream cannot change what we receive.
  nsresult ReconfigureCapsfilter();

  // Asynchronously send an event to the listener. May be called from any
  // thread.
  nsresult SendEventAsync(PRUint32 eventType, nsIVariant *eventDetails);

  // Synchronously send an event to the listener. Main thread only.
  nsresult SendEventSync(PRUint32 eventType, nsIVariant *eventDetails);

  // Helper to actually send the event; called only on main thread.
  nsresult SendEventInternal(PRUint32 eventType,
                             nsCOMPtr<nsIVariant> eventDetails);

  // Asynchronously send an error event, with an associated message looked up
  // in the string bundle with the given name.
  nsresult SendErrorEvent(PRUint32 errorCode, const char *errorName);

  // If we have enough data (either in the adapter, or in mBuffersAvailable (the
  // latter referring to buffers in mAppSink, plus mPendingBuffer if non-null)),
  // then schedule that to be sent (we don't send immediately as this might be
  // called on a non-main thread).
  nsresult ScheduleSendDataIfAvailable();

  // Do we have enough data in mAdapter to immediately send it to a listener?
  PRBool HasEnoughData();

  // If we can, get more data from the appsink or the pending buffer, and add it
  // to mAdapter. Must only be called if mBuffersAvailable is > 0.
  void GetMoreData();

  // Dispatch a call to SendDataToListener() to the main thread.
  nsresult ScheduleSendData();

  // Actually send data to the listener, if not suspended or stopped.
  // Once data has been sent, calls ScheduleSendDataIfAvailable() to see if we
  // have enough to send more.
  void SendDataToListener();

  // Get the duration in samples from the buffer.
  PRUint32 GetDurationFromBuffer(GstBuffer *buf);

  // Get the sample number corresponding to the start of this buffer.
  PRUint64 GetSampleNumberFromBuffer(GstBuffer *buf);

  // Handle the things we need to do before we first send data to the listener,
  // including sending the start event.
  nsresult DoStreamStart();

  // Determine what audio format we're going to send to the listener.
  nsresult DetermineFormat();

  // The listener. The interface only supports one per instance.
  nsCOMPtr<sbIMediacoreAudioProcessorListener> mListener;

  // Our constraints - all zero by default indicating no constraint
  PRUint32 mConstraintChannelCount;
  PRUint32 mConstraintSampleRate;
  PRUint32 mConstraintAudioFormat;
  PRUint32 mConstraintBlockSize;

  // Block size is in samples; this stores the value in bytes (which is format
  // dependent).
  PRUint32 mConstraintBlockSizeBytes;

  // The item being processed.
  nsCOMPtr<sbIMediaItem> mMediaItem;

  // Monitor used to protect all the following data items, which are modified
  // from both the main thread and from GStreamer streaming threads.
  PRMonitor *mMonitor;

  // Adapter used to ensure that data is sent in chunks sized according to
  // mConstraintBlockSize.
  GstAdapter *mAdapter;

  // The sink element we pull data from.
  GstAppSink *mAppSink;

  // The capsfilter element; we keep a reference to this so that we can
  // reconfigure it later.
  GstElement *mCapsFilter;

  // Track whether we're currently suspended or stopped - data will not be
  // sent to the listener when this is true.
  PRBool mSuspended;

  // Track if we've found an audio pad yet. If so, we ignore subsequent audio
  // tracks (e.g. in a video file with multiple audio tracks).
  PRBool mFoundAudioPad;

  // Set once we've fully initialised and sent the EVENT_START event to the
  // listener. At this point, the format being set is fully determined (see
  // mAudioFormat, mSampleRate, mChannels)
  PRBool mHasStarted;

  // Set if we've reached EOS.
  PRBool mIsEOS;

  // Set if we've reached the end of a 'section'. This is any sequence of
  // contiguous audio data (no gaps in the timestamps, nor explicit gaps from
  // the DISCONT flag on the audio buffers). We use this to know whether to
  // drain mAdapter, even if we don't have enough data to satisfy
  // mConstraintBlockSize, before sending a GAP event.
  PRBool mIsEndOfSection;

  // Set if we've already sent an error message, to tell us to suppress further
  // errors.
  PRBool mHasSentError;

  // Set if we should send a GAP event before sending any other data. This will
  // be set after we've encountered a gap, then drained the data from mAdapter.
  PRBool mSendGap;

  // The sample number of the start of the data currently held in mAdapter, that
  // will be sent to listeners when data is next delivered.
  PRUint64 mSampleNumber;

  // The expected sample number of the next audio to be added to mAdapter, this
  // is equal to mSampleNumber plus the duration in samples of the audio in
  // mAdapter currently.
  PRUint64 mExpectedNextSampleNumber;

  // Once mHasStarted is set, this is the specific audio format in use,
  // either FORMAT_INT16 or FORMAT_FLOAT.
  PRUint32 mAudioFormat;

  // Once mHasStarted is set, this is the sample rate being delivered to the
  // listener.
  gint mSampleRate;

  // Once mHasStarted is set, this is the number of channels being delivered to
  // the listener.
  gint mChannels;

  // The number of buffers available in the appsink and (if non-null) the
  // one in mPendingBuffer
  PRUint32 mBuffersAvailable;

  // A pending buffer. This is (when non-NULL) a buffer we've pulled from the
  // appsink, but have not yet submitted to mAdapter, because we're waiting for
  // mAdapter to drain before sending a gap event.
  // mPendingBuffer is only non-NULL while mIsEndOfSection is true.
  GstBuffer *mPendingBuffer;

};

#define SB_GSTREAMER_AUDIO_PROCESSOR_CLASSNAME \
      "sbGStreamerAudioProcessor"
#define SB_GSTREAMER_AUDIO_PROCESSOR_DESCRIPTION \
      "Songbird GStreamer Audio Processing API Implementation"
#define SB_GSTREAMER_AUDIO_PROCESSOR_CONTRACTID \
      "@songbirdnest.com/Songbird/Mediacore/GStreamer/AudioProcessor;1"
#define SB_GSTREAMER_AUDIO_PROCESSOR_CID \
      {0x490fef6b, 0xfed9, 0x4243, {0xa2, 0x80, 0x6b, 0xb1, 0x8a, 0x28, 0x3d, 0xfa}}

#endif // _SB_GSTREAMER_AUDIO_PROCESSOR_H_

