#ifndef _SB_GSTREAMER_SIMPLE_H_
#define _SB_GSTREAMER_SIMPLE_H_

#include "nsCOMPtr.h"
#include "nsStringGlue.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMWindow.h"
#include "nsITimer.h"
#include "nsComponentManagerUtils.h"

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>

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

  NS_IMETHODIMP
  Resize();

  GstBusSyncReply
  SyncHandler(GstBus* bus, GstMessage* message);

  void
  StreamInfoSet(GObject* obj, GParamSpec* pspec);

  void
  CapsSet(GObject* obj, GParamSpec* pspec);

private:
  ~sbGStreamerSimple();

  NS_IMETHODIMP SetupPlaybin();

  NS_IMETHODIMP DestroyPlaybin();

  bool 
  SetInvisibleCursor(sbGStreamerSimple* gsts);

  bool 
  SetDefaultCursor(sbGStreamerSimple* gsts);

  void
  ReparentToRootWin(sbGStreamerSimple* gsts);

  void
  ReparentToChromeWin(sbGStreamerSimple* gsts);

  PRBool mInitialized;

  GstElement* mPlay;
  GstBus*     mBus;
  guint       mPixelAspectRatioN;
  guint       mPixelAspectRatioD;
  gint        mVideoWidth;
  gint        mVideoHeight;
  gint        mOldCursorX;
  gint        mOldCursorY;
  // mRedrawCursor - so as to not draw cursor if it's already drawn
  bool        mRedrawCursor;
  // mDelayHide - 300ms timer for mouse polling so use to delay hiding; 
  // currently counts down from 10 for a 3 second wait.
  int         mDelayHide; 

  GstElement* mVideoSink;
  GdkWindow*  mGdkWin;
  GdkWindow*  mNativeWin;
  GdkWindow*  mGdkWinFull;

  PRBool mIsAtEndOfStream;
  PRBool mIsPlayingVideo;
  PRBool mFullscreen;
  PRInt32 mLastErrorCode;
  PRUint16 mBufferingPercent;

  double mLastVolume;

  nsCOMPtr<nsIDOMXULElement> mVideoOutputElement;
  nsCOMPtr<nsIDOMWindow> mDomWindow;
  nsCOMPtr<nsITimer> mCursorIntervalTimer;

  nsString  mArtist;
  nsString  mAlbum;
  nsString  mTitle;
  nsString  mGenre;

protected:
  /* additional members */
};

#endif // _SB_GSTREAMER_SIMPLE_H_
