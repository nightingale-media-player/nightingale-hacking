/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

/** 
 * \file WindowDragger.cpp
 * \brief Songbird Window Dragger Object Implementation.
 */
 
 // OMG OMG OMG THIS IS SUCH A HACK !!1!!!1!111! I SHALL ROT IN HELL AND AN 
 // EVIL FATE SHALL BEFALLEN UNTO MY DESCENDENTS FOR 18 GENERATIONS !!!!!!1!!
 //
 // - lone

 // Oh, yeah, and, this code is totally not portable. spank me.

#include "WindowDragger.h"

#ifdef XP_MACOSX
#include <Carbon/Carbon.h>
#include "nsIWidget.h"
#include "nsIToolkit.h"
#elif XP_UNIX
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#endif


#include "nscore.h"
#include "nspr.h"
#include "nsXPCOM.h"
#include "nsComponentManagerUtils.h"  // THIS for "do_CreateInstance" ??
#include <nsStringGlue.h>

#include <math.h>

#ifdef XP_WIN
#include "MultiMonitor.h"
#include "sbIDataRemote.h"
#endif


// CLASSES ====================================================================
//=============================================================================
// CWindowDragger Class
//=============================================================================

// so yeah ... because mozilla will not give us the mousemove events when they occur outside the 
// window rectangle (eventho it _has_ captured the window at the OS level!), we're implementing 
// our own hardcoded window dragger. 
//
// The idea is simple, whenever a mouse down event happens on a portion of the UI that should 
// trigger a window drag, we're given control: the position of the mouse in the currently active 
// window is recorded, a hidden window is created and captures the mouse, and any subsequent 
// mouse move event moves the window that was originally clicked accordingly. When the mouse 
// button is released, we destroy our hidden window, restore the capture to the moz window, 
// and send it a fake mouse up event, so as to get the exact same behavior internally as if 
// nothing happened (hopefully)

//-----------------------------------------------------------------------------
/* Implementation file */
NS_IMPL_ISUPPORTS1(CWindowDragger, sbIWindowDragger)

#define WINDOWDRAGGER_WNDCLASS NS_L("sbWindowDraggerWindow")

#ifdef XP_WIN
//-----------------------------------------------------------------------------
static LRESULT CALLBACK WindowDraggerProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  CWindowDragger *_this = reinterpret_cast<CWindowDragger *>(GetWindowLong(hWnd, GWL_USERDATA));
  return _this->WndProc(hWnd, uMsg, wParam, lParam);
} // WindowDraggerProc


//-----------------------------------------------------------------------------
CWindowDragger::CWindowDragger()
{
  m_captureWindow = NULL;
  m_oldCapture = NULL;
  m_draggedWindow = NULL;
  
  WNDCLASS wndClass;
  memset(&wndClass, 0, sizeof(wndClass));
  wndClass.hInstance = GetModuleHandle(NULL);
  wndClass.lpfnWndProc = WindowDraggerProc;
  wndClass.lpszClassName = WINDOWDRAGGER_WNDCLASS;
  RegisterClass(&wndClass);

  InitPause();
} // ctor

//-----------------------------------------------------------------------------
CWindowDragger::~CWindowDragger()
{
  UnregisterClass(WINDOWDRAGGER_WNDCLASS, GetModuleHandle(NULL));
} // dtor

//-----------------------------------------------------------------------------
LRESULT CWindowDragger::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_MOUSEMOVE:
      OnDrag();
      break;
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
      EndWindowDrag(uMsg, wParam);
      break;
    case WM_CAPTURECHANGED:
      OnCaptureLost();
      break;
  }
  return DefWindowProc(hWnd, uMsg, wParam, lParam);
} // WndProc

//-----------------------------------------------------------------------------
void CWindowDragger::OnDrag()
{
  POINT mousePos;
  GetCursorPos(&mousePos);
  int x = mousePos.x - m_relativeClickPos.x;
  int y = mousePos.y - m_relativeClickPos.y;
  DoDocking(m_draggedWindow, &x, &y, &mousePos);
  SetWindowPos(m_draggedWindow, NULL, x, y, 0, 0, SWP_NOSIZE|SWP_NOZORDER);
} // OnDrag

//-----------------------------------------------------------------------------
void CWindowDragger::EndWindowDrag(UINT msg, WPARAM flags)
{
  DestroyWindow(m_captureWindow);
  m_captureWindow = NULL;
  m_draggedWindow = NULL;
  if (m_oldCapture != NULL)
  {
    SetCapture(m_oldCapture);
    POINT mousePos;
    GetCursorPos(&mousePos);
    RECT windowPos;
    GetWindowRect(m_oldCapture, &windowPos);
    int x = mousePos.x - windowPos.left;
    int y = mousePos.y - windowPos.top;
    PostMessage(m_oldCapture, msg, flags, (((short)y)<<16)|((short)x));
    m_oldCapture = NULL;
  }
  FireCallback();
  DecPause();
} // EndWindowDrag

//-----------------------------------------------------------------------------
void CWindowDragger::OnCaptureLost() 
{
  DestroyWindow(m_captureWindow);
  m_captureWindow = NULL;
  m_draggedWindow = NULL;
  m_oldCapture = NULL;
  FireCallback();
  DecPause();
} // OnCaptureLost

//-----------------------------------------------------------------------------
NS_IMETHODIMP CWindowDragger::BeginWindowDrag(int dockdistance, sbIWindowDraggerCallback *cb)
{
  // If we're already in a dragging session, cancel it and start a new one
  // this is better than aborting, because if we ever end up not detecting
  // the end of a drag session, we do not want to break the window dragging
  // feature altogether
  if (m_captureWindow != NULL) {
    DestroyWindow(m_captureWindow);
    m_captureWindow = NULL;
    if (m_oldCapture != NULL) SetCapture(m_oldCapture);
    m_oldCapture = NULL;
    FireCallback();
    DecPause();
  }

  m_callback = cb;
  if (m_callback) NS_ADDREF(m_callback);

  m_oldCapture = GetCapture();
  
  POINT mousePos;
  GetCursorPos(&mousePos);
  m_draggedWindow = GetForegroundWindow();
  RECT windowPos;
  GetWindowRect(m_draggedWindow, &windowPos);
  m_relativeClickPos.x = mousePos.x - windowPos.left;
  m_relativeClickPos.y = mousePos.y - windowPos.top;
  m_dockDistance = dockdistance;
  
  m_captureWindow = CreateWindow(WINDOWDRAGGER_WNDCLASS, NULL, WS_POPUP, 0, 0, 1, 1, NULL, NULL, GetModuleHandle(NULL), NULL);
  if (m_captureWindow)
  {
    SetWindowLong(m_captureWindow, GWL_USERDATA, (LPARAM)this);
    SetCapture(m_captureWindow);

    IncPause();
  }
  else
  {
    m_draggedWindow = NULL;
  }

  

  return NS_OK;
} // BeginWindowDrag

#define shoulddock(wndcoord, dockcoord) (abs(wndcoord-dockcoord)<=m_dockDistance)

//-----------------------------------------------------------------------------
void CWindowDragger::DoDocking(HWND wnd, int *x, int *y, POINT *monitorPoint) 
{
  RECT monitor;
  CMultiMonitor::GetMonitorFromPoint(&monitor, monitorPoint, true);
  
  RECT r;
  GetWindowRect(wnd, &r);
  int w = r.right-r.left;
  int h = r.bottom-r.top;
  r.left = *x;
  r.top = *y;
  r.right = r.left + w;
  r.bottom = r.top + h;
  
  if (shoulddock(r.top, monitor.top))
  {
    *y = monitor.top;
  }
  else if (shoulddock(r.bottom, monitor.bottom))
  {
    *y = monitor.bottom-h;
  }

  if (shoulddock(r.left, monitor.left))
  {
    *x = monitor.left;
  }
  else if (shoulddock(r.right, monitor.right))
  {
    *x = monitor.right-w;
  }
} // DoDocking

//-----------------------------------------------------------------------------
void CWindowDragger::InitPause()
{
  //
  // ?? Kinda hacky.... pause the background scanner.
  //
  m_pauseScan = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1");
  m_pauseScan->Init( NS_LITERAL_STRING( "backscan.paused" ), NS_LITERAL_STRING( "" ) );
  m_backscanPaused = false;
}

//-----------------------------------------------------------------------------
void CWindowDragger::IncPause()
{
  // Increment the scan pause level.
  if ( ! m_backscanPaused )
  {
    m_backscanPaused = true;
    PRInt32 scan_pause_level = -1;
    m_pauseScan->GetIntValue( &scan_pause_level );
    m_pauseScan->SetIntValue( scan_pause_level + 1 );
  }
}

//-----------------------------------------------------------------------------
void CWindowDragger::DecPause()
{
  // Decrement the scan pause level.
  if ( m_backscanPaused )
  {
    PRInt32 scan_pause_level = -1;
    m_pauseScan->GetIntValue( &scan_pause_level );
    m_pauseScan->SetIntValue( max( 0, scan_pause_level - 1 ) + 1 );
  }
}

#endif  // END OF WIN32 VERSION




#ifdef XP_MACOSX

//-----------------------------------------------------------------------------
CWindowDragger::CWindowDragger()
{
} // ctor

//-----------------------------------------------------------------------------
CWindowDragger::~CWindowDragger()
{
} // dtor

//-----------------------------------------------------------------------------
// Start the window dragging.  
// Notes:
//      - Ends automatically on mouseup
//      - dockdistance is ignored for now
// Todo:
//      - detect end of drag operation and call FireCallback();
NS_IMETHODIMP CWindowDragger::BeginWindowDrag(int dockdistance, sbIWindowDraggerCallback *cb)
{
  Point click;
  WindowPtr     whichWindow;

  m_callback = cb;
  if (m_callback) NS_ADDREF(m_callback); // it should be safe to pass NULL for no callback

  // Find selected window using the current mouse location (better way?)
  GetMouse(&click);
  ::LocalToGlobal(&click);
  ::FindWindow(click, &whichWindow);
	
  // Tell the OS to start dragging the window
  Rect screenRect;
  ::GetRegionBounds(::GetGrayRgn(), &screenRect);
  ::DragWindow(whichWindow, click, &screenRect);

  return NS_OK;
} // BeginWindowDrag
// END OS X VERSION
#elif XP_UNIX

CWindowDragger::CWindowDragger()
{
    m_callback = NULL;
}

CWindowDragger::~CWindowDragger()
{
}

//-----------------------------------------------------------------------------
// Start the window dragging.  
// Notes:
//      - Ends automatically on mouseup
//      - dockdistance is ignored for now
//      - the mouse button that was pressed is just guessed right now.
//        right now it's always 1 so this isn't a problem
// Todo:
//      - detect end of drag operation and call FireCallback();
NS_IMETHODIMP CWindowDragger::BeginWindowDrag(int dockdistance, sbIWindowDraggerCallback *cb)
{
  m_callback = cb;
  if (m_callback) NS_ADDREF(m_callback); // it should be safe to pass NULL for no callback

  // Find the current mouse state
  GdkDisplay* display = gdk_display_get_default();
  gint x, y;
  GdkModifierType mask;
  gdk_display_get_pointer(display, NULL, &x, &y, &mask);

  // OK, this is hard - we need to guess what button was Just Dragged Now
  // we don't actually know but we can guess
  gint button = (mask&GDK_BUTTON1_MASK)?1:
               ((mask&GDK_BUTTON2_MASK)?2:
               ((mask&GDK_BUTTON3_MASK)?3:
               ((mask&GDK_BUTTON4_MASK)?4:
               ((mask&GDK_BUTTON5_MASK)?5:-1))));
  if (button < 0) {
    // if we can't work out what button it might be we can't start a drag
    return NS_ERROR_FAILURE;
  }

  // Find the currently hovered-over window
  GdkWindow* mouse_window;
  int win_x, win_y;
  mouse_window = gdk_display_get_window_at_pointer(display, &win_x, &win_y);
  GdkWindow* window;
  window = gdk_window_get_toplevel(mouse_window);

	
  // if I were an event, when would I happen?
  guint32 timestamp = gdk_x11_get_server_time(window);

  // make a drag happen
  gdk_window_begin_move_drag(window, button, x, y, timestamp);

  // we don't need to hold a reference to the window anymore
  //g_object_unref (window);

  return NS_OK;
} // BeginWindowDrag
// END GTK+ VERSION
#endif  


//-----------------------------------------------------------------------------
void CWindowDragger::FireCallback() 
{
  if (m_callback)
  {
    m_callback->OnWindowDragComplete();
    NS_RELEASE(m_callback);
    m_callback = NULL;
  }
}
