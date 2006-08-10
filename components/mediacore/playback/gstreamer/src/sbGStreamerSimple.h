#ifndef _SB_GSTREAMER_SIMPLE_H_
#define _SB_GSTREAMER_SIMPLE_H_

#include "nsCOMPtr.h"

#include <gst/gst.h>
#include <gst/interfaces/xoverlay.h>

#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>

#include "sbIGStreamerSimple.h"

class sbGStreamerSimple : public sbIGStreamerSimple
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIGSTREAMERSIMPLE

  sbGStreamerSimple();

  GstBusSyncReply
  SyncHandler(GstBus* bus, GstMessage* message);

private:
  ~sbGStreamerSimple();

  PRBool mInitialized;

  GstElement* mPlay;
  GstBus*     mBus;

  GstElement* mVideoSink;
  GdkWindow*  mGdkWin;

  PRBool mIsAtEndOfStream;

  nsCOMPtr<nsIDOMXULElement> mVideoOutputElement;
protected:
  /* additional members */
};

#endif // _SB_GSTREAMER_SIMPLE_H_

