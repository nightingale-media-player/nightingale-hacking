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

#ifndef __SB_GSTREAMERMEDIACORE_H__
#define __SB_GSTREAMERMEDIACORE_H__

#include <sbIGStreamerMediacore.h>

#include <nsIDOMEventListener.h>
#include <nsIDOMXULElement.h>

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>

#include <sbIMediacore.h>
#include <sbIMediacorePlaybackControl.h>
#include <sbIMediacoreVideoWindow.h>
#include <sbIMediacoreVolumeControl.h>
#include <sbIMediacoreVotingParticipant.h>
#include <sbIMediacoreEventTarget.h>
#include <sbIMediacoreError.h>

#include <sbIPropertyArray.h>

#include <sbBaseMediacore.h>
#include <sbBaseMediacorePlaybackControl.h>
#include <sbBaseMediacoreVolumeControl.h>
#include <sbBaseMediacoreEventTarget.h>

#include <gst/gst.h>
#include "sbIGstPlatformInterface.h"

class nsIURI;

class sbGStreamerMediacore : public sbBaseMediacore,
                             public sbBaseMediacorePlaybackControl,
                             public sbBaseMediacoreVolumeControl,
                             public sbIMediacoreVotingParticipant,
                             public sbIGStreamerMediacore,
                             public sbIMediacoreEventTarget,
                             public sbIMediacoreVideoWindow,
                             public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_SBIMEDIACOREEVENTTARGET
  NS_DECL_SBIMEDIACOREVOTINGPARTICIPANT
  NS_DECL_SBIMEDIACOREVIDEOWINDOW
  NS_DECL_SBIGSTREAMERMEDIACORE

  sbGStreamerMediacore();

  nsresult Init();

  // sbBaseMediacore overrides
  virtual nsresult OnInitBaseMediacore();
  virtual nsresult OnGetCapabilities();
  virtual nsresult OnShutdown();

  // sbBaseMediacorePlaybackControl overrides
  virtual nsresult OnInitBaseMediacorePlaybackControl();
  virtual nsresult OnSetUri(nsIURI *aURI);
  virtual nsresult OnGetDuration(PRUint64 *aDuration);
  virtual nsresult OnGetPosition(PRUint64 *aPosition);
  virtual nsresult OnSetPosition(PRUint64 aPosition);
  virtual nsresult OnPlay();
  virtual nsresult OnPause();
  virtual nsresult OnStop();

  // sbBaseMediacoreVolumeControl overrides
  virtual nsresult OnInitBaseMediacoreVolumeControl();
  virtual nsresult OnSetMute(PRBool aMute);
  virtual nsresult OnSetVolume(PRFloat64 aVolume);

  // GStreamer message handling
  virtual void HandleMessage(GstMessage *message);
  virtual PRBool HandleSynchronousMessage(GstMessage *message);

protected:
  virtual ~sbGStreamerMediacore();

  nsresult DestroyPipeline();
  nsresult CreatePlaybackPipeline();

  void DispatchMediacoreEvent (unsigned long type, 
          nsIVariant *aData = NULL, sbIMediacoreError *aError = NULL);

  void HandleAboutToFinishSignal ();
  void HandleTagMessage (GstMessage *message);
  void HandleStateChangedMessage (GstMessage *message);
  void HandleEOSMessage (GstMessage *message);
  void HandleErrorMessage (GstMessage *message);
  void HandleWarningMessage (GstMessage *message);
  void HandleBufferingMessage (GstMessage *message);
  void HandleRedirectMessage (GstMessage *message);

  nsresult LogMessageToErrorConsole(nsString message, PRUint32 flags);

  GstElement *CreateSinkFromPrefs(char *pref);
  GstElement *CreateVideoSink();
  GstElement *CreateAudioSink();

private:
  // Static helper for C callback
  static void syncHandler(GstBus *bus, GstMessage *message, gpointer data);
  static void aboutToFinishHandler(GstElement *playbin, gpointer data);

protected:
  // Protects all access to mPipeline
  PRMonitor*  mMonitor;

  PRBool mVideoEnabled;

  GstElement *mPipeline;
  nsAutoPtr<sbIGstPlatformInterface> mPlatformInterface;

  nsAutoPtr<sbBaseMediacoreEventTarget> mBaseEventTarget;

  // Metadata, both in original GstTagList form, and transformed into an
  // sbIPropertyArray. Both may be NULL.
  GstTagList *mTags;
  nsCOMPtr<sbIPropertyArray> mProperties;

  // Distinguish between being stopped, and stopping due to reaching the end.
  PRBool mStopped;
  // Track whether we're currently buffering
  PRBool mBuffering;

  // the video box
  nsCOMPtr<nsIDOMXULElement> mVideoWindow;
  nsCOMPtr<nsIDOMWindow> mDOMWindow;
};

#endif /* __SB_GSTREAMERMEDIACORE_H__ */
