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

#include "sbGStreamerPlatformWin32.h"
#include "sbGStreamerMediacore.h"

#include <prlog.h>
#include <nsDebug.h>

#include <nsIThread.h>
#include <nsThreadUtils.h>

/**
 * To log this class, set the following environment variable in a debug build:
 *  
 *  NSPR_LOG_MODULES=sbGStreamerPlatformWin32:5 (or :3 for LOG messages only)
 *  
 */ 
#ifdef PR_LOGGING
      
static PRLogModuleInfo* gGStreamerPlatformWin32 =
  PR_NewLogModule("sbGStreamerPlatformWin32");
    
#define LOG(args)                                         \
  if (gGStreamerPlatformWin32)                            \
    PR_LOG(gGStreamerPlatformWin32, PR_LOG_WARNING, args)

#define TRACE(args)                                      \
  if (gGStreamerPlatformWin32)                           \
    PR_LOG(gGStreamerPlatformWin32, PR_LOG_DEBUG, args)
  
#else /* PR_LOGGING */
  
#define LOG(args)   /* nothing */
#define TRACE(args) /* nothing */

#endif /* PR_LOGGING */


#define SB_VIDEOWINDOW_CLASSNAME L"SBGStreamerVideoWindow"

// TODO: This is a temporary bit of "UI" to get out of fullscreen mode.
// We'll do this properly at some point in the future.
/* static */ LRESULT APIENTRY
Win32PlatformInterface::VideoWindowProc(HWND hWnd, UINT message, 
        WPARAM wParam, LPARAM lParam)
{
  Win32PlatformInterface *platform = 
      (Win32PlatformInterface *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

  switch (message) {
    // If we're in full-screen mode, switch out on left-click
    case WM_LBUTTONDOWN:
      if (platform->mFullscreen) {
        platform->SetFullscreen(false);
        platform->ResizeToWindow();
      }
      break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

Win32PlatformInterface::Win32PlatformInterface(sbGStreamerMediacore *aCore)
: BasePlatformInterface(aCore)
, mWindow(NULL)
, mFullscreenWindow(NULL)
, mParentWindow(NULL)
{

}

nsresult
Win32PlatformInterface::SetVideoBox(nsIBoxObject *aBoxObject,
                                    nsIWidget *aWidget)
{
  // First let the superclass do its thing.
  nsresult rv = BasePlatformInterface::SetVideoBox (aBoxObject, aWidget);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aWidget) {
    mParentWindow = (HWND)aWidget->GetNativeData(NS_NATIVE_WIDGET);
    NS_ENSURE_TRUE(mParentWindow != NULL, NS_ERROR_FAILURE);

    // There always be at least one child. If there isn't, we can
    // parent ourselves directly to mParentWnd.
    HWND actualParent = SelectParentWindow(mParentWindow);

    WNDCLASS WndClass;

    ::ZeroMemory(&WndClass, sizeof (WNDCLASS));

    WndClass.style = CS_HREDRAW | CS_VREDRAW;
    WndClass.hInstance = GetModuleHandle(NULL);
    WndClass.lpszClassName = SB_VIDEOWINDOW_CLASSNAME;
    WndClass.hbrBackground = (HBRUSH) GetStockObject(BLACK_BRUSH);
    WndClass.cbClsExtra = 0;
    WndClass.cbWndExtra = 0;
    WndClass.lpfnWndProc = VideoWindowProc;
    WndClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
 
    ::RegisterClass(&WndClass);

    mWindow = ::CreateWindowEx(
            0,                                  // extended window style
            SB_VIDEOWINDOW_CLASSNAME,           // Class name
            L"Songbird GStreamer Video Window", // Window name
            WS_CHILD | WS_CLIPCHILDREN,         // window style
            0, 0,                               // X,Y offset
            0, 0,                               // Width, height
            actualParent,                       // Parent window
            NULL,                               // Menu, or child identifier
            WndClass.hInstance,                 // Module handle
            NULL);                              // Extra parameter

    ::SetWindowLongPtr(mWindow, GWLP_USERDATA, (LONG)this);

    // Display our normal window 
    ::ShowWindow(mWindow, SW_SHOWNORMAL);
  }
  else {
    // Hide, unparent, then destroy our video window
    ::ShowWindow(mWindow, SW_HIDE);
    ::SetParent(mWindow, NULL);
    ::DestroyWindow(mWindow);

    mWindow = NULL;
    mParentWindow = NULL;
  }
}

Win32PlatformInterface::~Win32PlatformInterface ()
{
  // Must free sink before destroying window.
  if (mVideoSink) {
    gst_object_unref(mVideoSink);
    mVideoSink = NULL;
  }

  if (mWindow) {
   ::DestroyWindow(mWindow);
  }
}

void
Win32PlatformInterface::FullScreen()
{
  NS_ASSERTION(mFullscreenWindow == NULL, "Fullscreen window is not null");

  HMONITOR monitor;
  MONITORINFO info;

  monitor = ::MonitorFromWindow(mWindow, MONITOR_DEFAULTTONEAREST);
  info.cbSize = sizeof (MONITORINFO);
  ::GetMonitorInfo(monitor, &info);

  mFullscreenWindow = ::CreateWindowEx(
    0,
    SB_VIDEOWINDOW_CLASSNAME,
    L"Songbird Fullscreen Video Window",
    WS_POPUP,
    info.rcMonitor.left, info.rcMonitor.top, 
    abs(info.rcMonitor.right - info.rcMonitor.left), 
    abs(info.rcMonitor.bottom - info.rcMonitor.top),
    NULL, NULL, NULL, NULL);

  ::SetWindowLongPtr(mFullscreenWindow, GWLP_USERDATA, (LONG)this);

  ::SetParent(mWindow, mFullscreenWindow);
  ::ShowWindow(mFullscreenWindow, SW_SHOWMAXIMIZED);

  //
  // When a window is MAXIMIZED on a monitor, it's coordinates are not
  // in virtual screen space anymore but in actual display coordinates.
  //
  // e.g. Top left corner of display becomes 0,0 even if it's at virtual
  // coordinate 1600,0. Because of this, we should always use 0,0 for x and y.
  //
  SetDisplayArea(0, 0, 
                 abs(info.rcMonitor.right - info.rcMonitor.left),
                 abs(info.rcMonitor.bottom - info.rcMonitor.top));
  ResizeVideo();

  ::ShowCursor(FALSE);
}

void 
Win32PlatformInterface::UnFullScreen()
{
  NS_ASSERTION(mFullscreenWindow, "Fullscreen window is null");

  // Hide it before we reparent.
  ::ShowWindow(mWindow, SW_HIDE);

  // There always be at least one child. If there isn't, we can
  // parent ourselves directly to mParentWnd.
  HWND actualParent = SelectParentWindow(mParentWindow);

  // Still no parent? Just use the video box window as the parent.
  if(!actualParent) {
    actualParent = mParentWindow;
  }

  // Reparent to video window box.
  ::SetParent(mWindow, actualParent);

  // Our caller should call Resize() after this to make sure we get moved to
  // the correct location
  ::ShowWindow(mWindow, SW_SHOWNORMAL);

  ::DestroyWindow(mFullscreenWindow);
  mFullscreenWindow = NULL;

  ::ShowCursor(TRUE);
}


void Win32PlatformInterface::MoveVideoWindow(int x, int y,
        int width, int height)
{
  if (mWindow) {
    ::SetWindowPos(mWindow, NULL, x, y, width, height, SWP_NOZORDER);
  }
}


GstElement *
Win32PlatformInterface::SetVideoSink(GstElement *aVideoSink)
{
  if (mVideoSink) {
    gst_object_unref(mVideoSink);
    mVideoSink = NULL;
  }

  mVideoSink = aVideoSink;

  if (!mVideoSink)
    mVideoSink = ::gst_element_factory_make("dshowvideosink", NULL);
  if (!mVideoSink)
    mVideoSink = ::gst_element_factory_make("autovideosink", NULL);

  // Keep a reference to it.
  if (mVideoSink) 
      gst_object_ref(mVideoSink);

  return mVideoSink;
}

GstElement *
Win32PlatformInterface::SetAudioSink(GstElement *aAudioSink)
{
  if (mAudioSink) {
    gst_object_unref(mAudioSink);
    mAudioSink = NULL;
  }

  mAudioSink = aAudioSink;

  if (!mAudioSink) {
    mAudioSink = gst_element_factory_make("directsoundsink", "audio-sink");
  }

  if (!mAudioSink) {
    // Hopefully autoaudiosink will pick something appropriate...
    mAudioSink = gst_element_factory_make("autoaudiosink", "audio-sink");
  }

  // Keep a reference to it.
  if (mAudioSink) 
      gst_object_ref(mAudioSink);

  return mAudioSink;
}

void
Win32PlatformInterface::SetXOverlayWindowID(GstXOverlay *aXOverlay)
{
  /* GstXOverlay is confusingly named - it's actually generic enough for windows
   * too, so the windows videosink implements it too.
   * So, we use the GstXOverlay interface to set the window handle here 
   */
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
    gst_x_overlay_set_xwindow_id(aXOverlay, (glong)mWindow);

    LOG(("Set xoverlay %p to HWND %x\n", aXOverlay, mWindow));
  }
}

HWND 
Win32PlatformInterface::SelectParentWindow(HWND hWnd)
{
  HWND retWnd = NULL;
  HWND firstChildWnd = ::GetWindow(hWnd, GW_CHILD);

  if(firstChildWnd != NULL) {
    retWnd = ::GetWindow(firstChildWnd, GW_HWNDLAST);
  }

  if(!retWnd) {
    retWnd = hWnd;
  }

  return retWnd;
}
