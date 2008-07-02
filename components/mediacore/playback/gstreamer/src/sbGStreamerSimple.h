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

#ifndef _SB_GSTREAMER_SIMPLE_H_
#define _SB_GSTREAMER_SIMPLE_H_

#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsStringGlue.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMWindow.h>
#include <nsITimer.h>
#include <nsComponentManagerUtils.h>
#include <nsIStringBundle.h>

#include <gst/gst.h>

#include "sbIGstPlatformInterface.h"
#include "sbIGStreamerSimple.h"

class sbGStreamerSimple : public sbIGStreamerSimple,
                          public nsIDOMEventListener,
                          public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIGSTREAMERSIMPLE
  NS_DECL_NSIDOMEVENTLISTENER
  NS_DECL_NSITIMERCALLBACK

  sbGStreamerSimple();

private:
  ~sbGStreamerSimple();

  // Static helpers for C callbacks
  static void syncHandler(GstBus *bus, GstMessage *message, gpointer data); 
  static void asyncHandler(GstBus *bus, GstMessage *message, gpointer data);
  static void videoCapsSetHelper(GObject *obj, GParamSpec *pspec, 
          sbGStreamerSimple *gsts);
  static void streamInfoSetHelper(GObject *obj, GParamSpec *pspec, 
          sbGStreamerSimple *gsts);
  static void currentVideoSetHelper(GObject *obj, GParamSpec *pspec, 
          sbGStreamerSimple *gsts);

  NS_HIDDEN_(nsresult) Resize();

  NS_HIDDEN_(void) PrepareVideoWindow(GstMessage *msg);

  NS_HIDDEN_(void) HandleMessage(GstMessage* message);

  NS_HIDDEN_(void) OnVideoCapsSet(GstCaps *caps);

  NS_HIDDEN_(void) OnStreamInfoSet();

  NS_HIDDEN_(nsresult) SetupPlaybin();

  NS_HIDDEN_(nsresult) DestroyPlaybin();
  
  NS_HIDDEN_(nsresult) RestartPlaybin();

  NS_HIDDEN_(void) HandleErrorMessage(GstMessage *message);
  NS_HIDDEN_(void) HandleWarningMessage(GstMessage *message);
  NS_HIDDEN_(void) HandleStateChangeMessage(GstMessage *message);
  NS_HIDDEN_(void) HandleBufferingMessage(GstMessage *message);
  NS_HIDDEN_(void) HandleEOSMessage(GstMessage *message);
  NS_HIDDEN_(void) HandleTagMessage(GstMessage *message);

  PRBool mInitialized;

  nsCOMArray<sbIGStreamerEventListener> mListeners;

  GstElement* mPlay;
  guint       mPixelAspectRatioN;
  guint       mPixelAspectRatioD;
  gint        mVideoWidth;
  gint        mVideoHeight;

  sbIGstPlatformInterface *mPlatformInterface;

  PRBool mIsAtEndOfStream;
  PRBool mIsPlayingVideo;
  PRInt32 mLastErrorCode;
  PRUint16 mBufferingPercent;

  PRBool mIsUsingPlaybin2;

  double mLastVolume;

  nsCOMPtr<nsIDOMWindow> mDomWindow;

  nsString  mArtist;
  nsString  mAlbum;
  nsString  mTitle;
  nsString  mGenre;

protected:
  /* additional members */
};

#endif // _SB_GSTREAMER_SIMPLE_H_
