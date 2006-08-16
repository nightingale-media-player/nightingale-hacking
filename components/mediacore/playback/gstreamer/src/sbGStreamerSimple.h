#ifndef _SB_GSTREAMER_SIMPLE_H_
#define _SB_GSTREAMER_SIMPLE_H_

#include "nsCOMPtr.h"
#include "nsIDOMEventListener.h"
#include "nsIDOMWindow.h"

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>

#include "sbIGStreamerSimple.h"

class sbGStreamerSimple : public sbIGStreamerSimple,
                          public nsIDOMEventListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIGSTREAMERSIMPLE
  NS_DECL_NSIDOMEVENTLISTENER

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

  PRBool mInitialized;

  GstElement* mPlay;
  GstBus*     mBus;
  guint       mPixelAspectRatioN;
  guint       mPixelAspectRatioD;
  gint        mVideoWidth;
  gint        mVideoHeight;

  GstElement* mVideoSink;
  GdkWindow*  mGdkWin;

  PRBool mIsAtEndOfStream;

  nsCOMPtr<nsIDOMXULElement> mVideoOutputElement;
  nsCOMPtr<nsIDOMWindow> mDomWindow;

protected:
  /* additional members */
};

#endif // _SB_GSTREAMER_SIMPLE_H_

