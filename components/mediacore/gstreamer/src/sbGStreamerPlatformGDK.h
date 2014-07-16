/*
 * BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2005-2008 POTI, Inc.
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

#ifndef _SB_GSTREAMER_PLATFORM_GDK_H_
#define _SB_GSTREAMER_PLATFORM_GDK_H_

#include <gst/gst.h>
#include <gst/video/videooverlay.h>

#include <gtk/gtkwindow.h>
#include <gdk/gdkx.h>

#include "sbIGstPlatformInterface.h"
#include "sbGStreamerPlatformBase.h"
#include "sbGStreamerMediacore.h"

#include <nsIBoxObject.h>

class GDKPlatformInterface : public BasePlatformInterface
{
public:
  GDKPlatformInterface (sbGStreamerMediacore *aCore);

  // Implementation of the rest of sbIGstPlatformInterface interface
  GstElement * SetVideoSink (GstElement *aVideoSink);
  GstElement * SetAudioSink (GstElement *aAudioSink);

protected:
  // Implement virtual methods in BasePlatformInterface
  void MoveVideoWindow (int x, int y, int width, int height);
  void SetVideoOverlayWindowID(GstVideoOverlay *aVideoOverlay);
  void FullScreen();
  void UnFullScreen();
  nsresult SetVideoBox (nsIBoxObject *aBoxObject, nsIWidget *aWidget);

private:
  // Callback for GDK events.
  static GdkFilterReturn gdk_event_filter(GdkXEvent *gdk_xevent,
        GdkEvent *event, gpointer data);

  // Set the cursor for the video window to an invisible cursor
  void SetInvisibleCursor();
  // Set the cursor for the video window to the default cursor
  void SetDefaultCursor();

  GdkWindow*  mWindow;           // Our video window
  GdkWindow*  mParentWindow;     // Our parent (XUL) window
  GdkWindow*  mFullscreenWindow; // Fullscreen window (if we're fullscreen) that
                                 // we reparent our video window to, or NULL.
  XID         mWindowXID;        // XID for the actual X window underlying the
                                 // mWindow object.
};

#endif // _SB_GSTREAMER_PLATFORM_GDK_H_
