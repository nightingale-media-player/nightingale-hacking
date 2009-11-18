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

#include <sbVariantUtils.h>

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

// This is normally defined in a Windows header but this header is not
// part of the free toolset provided by MS so we define our own if it's
// not defined.
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lParam)  ((int)(short)LOWORD(lParam))
#endif
#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lParam)  ((int)(short)HIWORD(lParam))
#endif

/* static */ LRESULT APIENTRY
Win32PlatformInterface::VideoWindowProc(HWND hWnd, UINT message, 
        WPARAM wParam, LPARAM lParam)
{
  Win32PlatformInterface *platform = 
      (Win32PlatformInterface *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

  switch (message) {
    case WM_KEYDOWN: {
      nsCOMPtr<nsIDOMKeyEvent> keyEvent;
      nsresult rv = platform->CreateDOMKeyEvent(getter_AddRefs(keyEvent));
      if(NS_SUCCEEDED(rv)) {
        PRBool shiftKeyState = HIBYTE(GetKeyState(VK_SHIFT)) > 0;
        PRBool ctrlKeyState  = HIBYTE(GetKeyState(VK_CONTROL)) > 0;
        PRBool altKeyState   = HIBYTE(GetKeyState(VK_MENU)) > 0;
        PRBool winKeyStateL  = HIBYTE(GetKeyState(VK_LWIN)) > 0;
        PRBool winKeyStateR  = HIBYTE(GetKeyState(VK_RWIN)) > 0;

        PRInt32 keyCode = wParam;
        PRInt32 charCode = LOWORD(MapVirtualKey(wParam, MAPVK_VK_TO_CHAR));

        rv = keyEvent->InitKeyEvent(NS_LITERAL_STRING("keypress"),
                                    PR_TRUE,
                                    PR_TRUE,
                                    nsnull,
                                    ctrlKeyState,
                                    altKeyState,
                                    shiftKeyState,
                                    winKeyStateL || winKeyStateR,
                                    keyCode,
                                    charCode);
        if(NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIDOMEvent> event(do_QueryInterface(keyEvent));
          platform->DispatchDOMEvent(event);

          return 0;
        }
      }
    }
    break;

    case WM_CONTEXTMENU: {
      nsCOMPtr<nsIDOMMouseEvent> mouseEvent;
      nsresult rv = platform->CreateDOMMouseEvent(getter_AddRefs(mouseEvent));
      if(NS_SUCCEEDED(rv)) {
        PRBool shiftKeyState = HIBYTE(GetKeyState(VK_SHIFT)) > 0;
        PRBool ctrlKeyState  = HIBYTE(GetKeyState(VK_CONTROL)) > 0;
        PRBool altKeyState   = HIBYTE(GetKeyState(VK_MENU)) > 0;
        PRBool winKeyStateL  = HIBYTE(GetKeyState(VK_LWIN)) > 0;
        PRBool winKeyStateR  = HIBYTE(GetKeyState(VK_RWIN)) > 0;

        PRInt32 screenX = GET_X_LPARAM(lParam);
        PRInt32 screenY = GET_Y_LPARAM(lParam);

        POINT point = {0};
        point.x = screenX;
        point.y = screenY;

        BOOL success = ScreenToClient(hWnd, &point);
        NS_WARN_IF_FALSE(success, 
          "Failed to convert coordinates, popup menu will be positioned wrong");

        rv = mouseEvent->InitMouseEvent(NS_LITERAL_STRING("contextmenu"), 
                                        PR_TRUE,
                                        PR_TRUE,
                                        nsnull,
                                        0,
                                        screenX,
                                        screenY,
                                        point.x,
                                        point.y,
                                        ctrlKeyState,
                                        altKeyState,
                                        shiftKeyState,
                                        winKeyStateL || winKeyStateR,
                                        2,
                                        nsnull);
        if(NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIDOMEvent> event(do_QueryInterface(mouseEvent));
          platform->DispatchDOMEvent(event);

          return 0;
        } 
      }
    }
    break;

    case WM_MOUSEMOVE: {
      nsCOMPtr<nsIDOMMouseEvent> mouseEvent;
      nsresult rv = platform->CreateDOMMouseEvent(getter_AddRefs(mouseEvent));
      if(NS_SUCCEEDED(rv)) {
        PRBool shiftKeyState = HIBYTE(GetKeyState(VK_SHIFT)) > 0;
        PRBool ctrlKeyState  = HIBYTE(GetKeyState(VK_CONTROL)) > 0;
        PRBool altKeyState   = HIBYTE(GetKeyState(VK_MENU)) > 0;
        PRBool winKeyStateL  = HIBYTE(GetKeyState(VK_LWIN)) > 0;
        PRBool winKeyStateR  = HIBYTE(GetKeyState(VK_RWIN)) > 0;

        PRInt32 clientX = GET_X_LPARAM(lParam);
        PRInt32 clientY = GET_Y_LPARAM(lParam);

        POINT point = {0};
        point.x = clientX;
        point.y = clientY;

        BOOL success = ClientToScreen(hWnd, &point);
        NS_WARN_IF_FALSE(success, 
          "Failed to convert coords, mousemove will have wrong screen coords");

        rv = mouseEvent->InitMouseEvent(NS_LITERAL_STRING("mousemove"), 
                                        PR_TRUE,
                                        PR_TRUE,
                                        nsnull,
                                        0,
                                        point.x,
                                        point.y,
                                        clientX,
                                        clientY,
                                        ctrlKeyState,
                                        altKeyState,
                                        shiftKeyState,
                                        winKeyStateL || winKeyStateR,
                                        0,
                                        nsnull);
        if(NS_SUCCEEDED(rv)) {
          nsCOMPtr<nsIDOMEvent> event(do_QueryInterface(mouseEvent));
          platform->DispatchDOMEvent(event);

          return 0;
        } 
      }
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
    ::ShowWindow(mWindow, SW_SHOW);
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

  // XXXAus: For now don't hide the cursor. We'll hide it using a timer later.
  //         See bug 18834.
  //::ShowCursor(FALSE);
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

  // Reparent to video window box.
  ::SetParent(mWindow, actualParent);

  // Our caller should call Resize() after this to make sure we get moved to
  // the correct location
  ::ShowWindow(mWindow, SW_SHOW);

  ::DestroyWindow(mFullscreenWindow);
  mFullscreenWindow = NULL;

  // XXXAus: We'll use a timer for this later. For now we don't hide it so
  //         no need to show it. See bug 18834.
  //::ShowCursor(TRUE);
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
  // Select Parent Window attempts to select the best parent for the video
  // window so that all events get propagated through the various WindowProcs
  // as expected. 

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
