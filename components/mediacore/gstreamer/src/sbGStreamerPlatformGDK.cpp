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

#include "sbGStreamerPlatformGDK.h"
#include "sbGStreamerMediacore.h"

#include <gdk/gdkx.h>
#include <X11/Xlib.h>

#include <prlog.h>
#include <nsDebug.h>

/**
 * To log this class, set the following environment variable in a debug build:
 *
 *  NSPR_LOG_MODULES=sbGStreamerPlatformGDK:5 (or :3 for LOG messages only)
 *
 */
#ifdef PR_LOGGING

static PRLogModuleInfo* gGStreamerPlatformGDK =
  PR_NewLogModule("sbGStreamerPlatformGDK");

#define LOG(args)                                         \
  if (gGStreamerPlatformGDK)                             \
    PR_LOG(gGStreamerPlatformGDK, PR_LOG_WARNING, args)

#define TRACE(args)                                      \
  if (gGStreamerPlatformGDK)                            \
    PR_LOG(gGStreamerPlatformGDK, PR_LOG_DEBUG, args)

#else /* PR_LOGGING */

#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */

#endif /* PR_LOGGING */

// TODO: This is a temporary bit of "UI" to get out of fullscreen mode.
// We'll do this properly at some point in the future.
/* static */ GdkFilterReturn
GDKPlatformInterface::gdk_event_filter(GdkXEvent *gdk_xevent, 
        GdkEvent *event, gpointer data)
{
  GDKPlatformInterface *platform = (GDKPlatformInterface *)data;
  XEvent *xevent = (XEvent *)gdk_xevent;

  switch (xevent->type) {
    case ButtonPress:
    case ButtonRelease:
      {
        platform->SetFullscreen(false);
        platform->ResizeToWindow();
      }
      break;
    default:
      break;
  }

  return GDK_FILTER_CONTINUE;
}

GDKPlatformInterface::GDKPlatformInterface(sbGStreamerMediacore *aCore) :
    BasePlatformInterface(aCore),
    mWindow(NULL),
    mParentWindow(NULL),
    mFullscreenWindow(NULL)
{
}

void
GDKPlatformInterface::FullScreen()
{
  NS_ASSERTION (mFullscreenWindow == NULL, "Fullscreen window is non-null");

  GdkScreen *screen = NULL;
  gint screenWidth, screenHeight;
  GdkWindowAttr attributes;
  XWindowAttributes xattrs;

  attributes.window_type = GDK_WINDOW_TOPLEVEL;
  attributes.x = 0;
  attributes.y = 0;
  attributes.width = 0;
  attributes.height = 0;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.event_mask = 0;

  // Create a new, full-screen, window.
  // Reparent our video window to this full-screen window
  mFullscreenWindow = gdk_window_new(NULL, &attributes, GDK_WA_X | GDK_WA_Y);
  gdk_window_show(mFullscreenWindow);
  gdk_window_reparent(mWindow, mFullscreenWindow, 0, 0);
  gdk_window_fullscreen(mFullscreenWindow);

  // Start listening to button press events on this window and the fullscreen 
  // window and register a gdk event filter to process them.
  XGetWindowAttributes(GDK_DISPLAY (), GDK_WINDOW_XWINDOW (mWindow), &xattrs);
  XSelectInput(GDK_DISPLAY (), GDK_WINDOW_XWINDOW (mWindow), 
          xattrs.your_event_mask | ButtonPressMask);
  gdk_window_add_filter(mWindow, gdk_event_filter, this);

  XGetWindowAttributes(GDK_DISPLAY (), GDK_WINDOW_XWINDOW (mFullscreenWindow),
          &xattrs);
  XSelectInput(GDK_DISPLAY (), GDK_WINDOW_XWINDOW (mFullscreenWindow), 
          xattrs.your_event_mask | ButtonPressMask);
  gdk_window_add_filter(mFullscreenWindow, gdk_event_filter, this);

  // Get the default screen. This can be wrong, but GDK doesn't seem to properly
  // support multiple screens (?)
  screen = gdk_screen_get_default();

  screenWidth = gdk_screen_get_width(screen);
  screenHeight = gdk_screen_get_height(screen);

  SetDisplayArea(0, 0, screenWidth, screenHeight);
  ResizeVideo();

  /* Set the cursor invisible in full screen mode initially. */
  SetInvisibleCursor();

  // TODO: if the user moves the cursor, it should become visible.
}

void 
GDKPlatformInterface::UnFullScreen()
{
  NS_ASSERTION (mFullscreenWindow, "Fullscreen window is null");

  gdk_window_remove_filter(mWindow, gdk_event_filter, this);
  gdk_window_remove_filter(mFullscreenWindow, gdk_event_filter, this);

  gdk_window_unfullscreen(mWindow);
  gdk_window_reparent(mWindow, mParentWindow, 0, 0);
  gdk_window_destroy(mFullscreenWindow);
  mFullscreenWindow = NULL;

  SetDefaultCursor();
}

void 
GDKPlatformInterface::SetInvisibleCursor()
{
  guint32 data = 0;
  GdkPixmap* pixmap = gdk_bitmap_create_from_data(NULL, (gchar*)&data, 1, 1);

  GdkColor color = { 0, 0, 0, 0 };
  GdkCursor* cursor = gdk_cursor_new_from_pixmap(pixmap,
          pixmap, &color, &color, 0, 0);

  gdk_pixmap_unref(pixmap);

  gdk_window_set_cursor(mWindow, cursor);
  if (mFullscreenWindow)
    gdk_window_set_cursor(mFullscreenWindow, cursor);

  gdk_cursor_unref(cursor);

}

void
GDKPlatformInterface::SetDefaultCursor()
{
  gdk_window_set_cursor(mWindow, NULL);
  if (mFullscreenWindow)
    gdk_window_set_cursor(mFullscreenWindow, NULL);
}

void 
GDKPlatformInterface::MoveVideoWindow(int x, int y, int width, int height)
{
  if (mWindow) {
    LOG(("Moving video window to %d,%d, size %d,%d", x, y, width, height));
    gdk_window_move_resize(mWindow, x, y, width, height);
  }
} 

GstElement *
GDKPlatformInterface::SetVideoSink(GstElement *aVideoSink)
{
  if (mVideoSink) {
    gst_object_unref(mVideoSink);
    mVideoSink = NULL;
  }

  // Use the provided sink, if any.
  mVideoSink = aVideoSink;

  if (!mVideoSink)
    mVideoSink = gst_element_factory_make("gconfvideosink", "video-sink");
  if (!mVideoSink) {
    // Then hopefully autovideosink will pick something appropriate...
    mVideoSink = gst_element_factory_make("autovideosink", "video-sink");
  }

  // Keep a reference to it.
  if (mVideoSink) 
      gst_object_ref(mVideoSink);

  return mVideoSink;
}

GstElement *
GDKPlatformInterface::SetAudioSink(GstElement *aAudioSink)
{
  if (mAudioSink) {
    gst_object_unref(mAudioSink);
    mAudioSink = NULL;
  }

  // Use the audio sink provided, if any.
  mAudioSink = aAudioSink;

  if (!mAudioSink) {
    mAudioSink = gst_element_factory_make("gconfaudiosink", "audio-sink");
    if (mAudioSink) {
      // Set the profile for gconfaudiosink to use to 'Music and Movies',
      // this is profile 1 (we can't use the enum directly, as the header 
      // is not installed).
      g_object_set (G_OBJECT (mAudioSink), "profile", 1, NULL);
    }
  }

  if (!mAudioSink) {
    // Then hopefully autoaudiosink will pick something appropriate...
    mAudioSink = gst_element_factory_make("autoaudiosink", "audio-sink");
  }

  // Keep a reference to it.
  if (mAudioSink) 
      gst_object_ref(mAudioSink);

  return mAudioSink;
}

nsresult
GDKPlatformInterface::SetVideoBox (nsIBoxObject *aBoxObject, nsIWidget *aWidget)
{
  // First let the superclass do its thing.
  nsresult rv = BasePlatformInterface::SetVideoBox (aBoxObject, aWidget);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aWidget) {
    mParentWindow = GDK_WINDOW(aWidget->GetNativeData(NS_NATIVE_WIDGET));
    NS_ENSURE_TRUE(mParentWindow != NULL, NS_ERROR_FAILURE);

    // Create the video window, initially with zero size.
    GdkWindowAttr attributes;
 
    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.x = 0;
    attributes.y = 0;
    attributes.width = 0;
    attributes.height = 0;
    attributes.wclass = GDK_INPUT_OUTPUT;
    attributes.event_mask = GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | 
                            GDK_BUTTON_PRESS_MASK | GDK_KEY_PRESS_MASK;

    mWindow = gdk_window_new(mParentWindow, &attributes, GDK_WA_X | GDK_WA_Y);
    NS_ENSURE_TRUE(mParentWindow != NULL, NS_ERROR_FAILURE);

    gdk_window_show(mWindow);
  }
  else {
    // hide, unparent, and then destroy our private window
    gdk_window_hide(mWindow);
    gdk_window_reparent(mWindow, NULL, 0, 0);
    gdk_window_destroy(mWindow);

    mWindow = nsnull;
    mParentWindow = nsnull;
  }

  return NS_OK;
}

void GDKPlatformInterface::SetXOverlayWindowID(GstXOverlay *aXOverlay)
{
  nsresult rv;

  if (!mWindow) {
    // If we haven't already had a window explicitly set on us, then request
    // one from the mediacore manager. This needs to be main-thread, as it does
    // DOM stuff internally.
    nsCOMPtr<nsIThread> mainThread;
    rv = NS_GetMainThread(getter_AddRefs(mainThread));
    NS_ENSURE_SUCCESS(rv, /* void */);

    nsCOMPtr<nsIRunnable> runnable = 
        NS_NEW_RUNNABLE_METHOD (sbGStreamerMediacore,
                                mCore,
                                RequestVideoWindow);

    rv = mainThread->Dispatch(runnable, NS_DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, /* void */);
  }

  if (mWindow) {
    XID window = GDK_WINDOW_XWINDOW(mWindow);
    gst_x_overlay_set_xwindow_id(aXOverlay, window);

    LOG(("Set xoverlay %d to windowid %x\n", aXOverlay, window));
  }
}

