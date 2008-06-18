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

#include "prlog.h"
#include "nsDebug.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gGStreamerLog;
#endif /* PR_LOGGING */

#define TRACE(args) PR_LOG(gGStreamerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gGStreamerLog, PR_LOG_WARN, args)

GDKPlatformInterface::GDKPlatformInterface (GdkWindow *parent) : 
    BasePlatformInterface(),
    mWindow(NULL),
    mParentWindow(parent),
    mFullscreenWindow(NULL)
{
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

  gdk_window_show(mWindow);
}

void
GDKPlatformInterface::FullScreen ()
{
  NS_ASSERTION (mFullscreenWindow == NULL, "Fullscreen window is non-null");

  GdkScreen *screen = NULL;
  gint screenWidth, screenHeight;
  GdkWindowAttr attributes;

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

  // Get the default screen. This can be wrong, but GDK doesn't seem to properly
  // support multiple screens (?)
  screen = gdk_screen_get_default();

  screenWidth = gdk_screen_get_width(screen);
  screenHeight = gdk_screen_get_height(screen);

  SetDisplayArea (0, 0, screenWidth, screenHeight);
  ResizeVideo ();

  /* Set the cursor invisible in full screen mode initially. */
  SetInvisibleCursor();

  // TODO: if the user moves the cursor, it should become visible.
  // So, we need to get events for this window...
  // Also, we should react to keyboard events in some appropriate way?

  // TODO: No, this is wrong; this signal is on GtkWidget, not GdkWindow :-(
  //mMotionHandlerId = g_signal_connect (G_OBJECT (mFullscreenWindow), 
  //        "motion-notify-event", G_CALLBACK (motion-notify_cb), this);
}

void 
GDKPlatformInterface::UnFullScreen ()
{
  NS_ASSERTION (mFullscreenWindow, "Fullscreen window is null");

  gdk_window_unfullscreen(mWindow);
  gdk_window_reparent(mWindow, mParentWindow, 0, 0);
  gdk_window_destroy(mFullscreenWindow);
  mFullscreenWindow = NULL;

  SetDefaultCursor();
}

void 
GDKPlatformInterface::SetInvisibleCursor ()
{
  guint32 data = 0;
  GdkPixmap* pixmap = gdk_bitmap_create_from_data(NULL, (gchar*)&data, 1, 1);

  GdkColor color = { 0, 0, 0, 0 };
  GdkCursor* cursor = gdk_cursor_new_from_pixmap(pixmap,
          pixmap, &color, &color, 0, 0);

  gdk_pixmap_unref(pixmap);

  gdk_window_set_cursor(mWindow, cursor);

  gdk_cursor_unref(cursor);

}

void
GDKPlatformInterface::SetDefaultCursor ()
{
  gdk_window_set_cursor(mWindow, NULL);
}

void 
GDKPlatformInterface::MoveVideoWindow (int x, int y, int width, int height)
{
  gdk_window_move_resize(mWindow, x, y, width, height);
}

GstElement *
GDKPlatformInterface::CreateVideoSink ()
{
  if (mVideoSink) {
    gst_object_unref (mVideoSink);
    mVideoSink = NULL;
  }

  mVideoSink = gst_element_factory_make("gconfvideosink", "video-sink");
  if (!mVideoSink) {
    // Then hopefully autovideosink will pick something appropriate...
    mVideoSink = gst_element_factory_make("autovideosink", "video-sink");
  }

  // Keep a reference to it.
  if (mVideoSink) 
      gst_object_ref (mVideoSink);

  return mVideoSink;
}

GstElement *
GDKPlatformInterface::CreateAudioSink ()
{
  if (mAudioSink) {
    gst_object_unref (mAudioSink);
    mAudioSink = NULL;
  }

  mAudioSink = gst_element_factory_make("gconfaudiosink", "audio-sink");
  if (!mAudioSink) {
    // Then hopefully autoaudiosink will pick something appropriate...
    mAudioSink = gst_element_factory_make("autoaudiosink", "audio-sink");
  }

  // Keep a reference to it.
  if (mAudioSink) 
      gst_object_ref (mAudioSink);

  return mAudioSink;
}

void GDKPlatformInterface::SetXOverlayWindowID (GstXOverlay *xoverlay)
{
  XID window = GDK_WINDOW_XWINDOW(mWindow);
  gst_x_overlay_set_xwindow_id(xoverlay, window);
  LOG(("Set xoverlay %d to windowid %d\n", xoverlay, window));
}

