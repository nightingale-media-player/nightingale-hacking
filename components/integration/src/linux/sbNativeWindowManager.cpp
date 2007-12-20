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

/** 
 * \file sbINativeWindowManager
 * \brief Interface to native window managers (on X11).
 */
 

#include "sbNativeWindowManager.h"
#include "../NativeWindowFromNode.h"

#include <gdk/gdkx.h>
#include <gdk/gdk.h>
#include <gtk/gtkwindow.h>

/* define this to use the X & Y from Mozilla DOM event */
#undef MOUSE_POSITION_FROM_EVENT

NS_IMPL_ISUPPORTS1(sbNativeWindowManager, sbINativeWindowManager)

/* a helper function to return our toplevel GDK window given a Mozilla window. */
static nsresult
GetToplevelGdkWindow(nsISupports* aWindow, GdkWindow** aToplevelWindow) {
  GdkWindow* window;

  /* we need somewhere to put the result */
  if (!aToplevelWindow) {
    return NS_ERROR_FAILURE;
  }

  /* find the window that was passed in */
  window = NativeWindowFromNode::get(aWindow);
  if (!GDK_IS_WINDOW(window)) {
    return NS_ERROR_FAILURE;
  }

  /* no, actually we want the top-level window since we're doing a resize */
  window = gdk_window_get_toplevel(window);
  if (!GDK_IS_WINDOW(window)) {
    return NS_ERROR_FAILURE;
  }

  *aToplevelWindow = window;
  return NS_OK;
}

/* a helper function to return our toplevel Gtk+ window given a Mozilla window. */
static nsresult
GetToplevelGtkWindow(nsISupports* aWindow, GtkWindow** aToplevelWindow) {
  GdkWindow* gdkwindow = NULL;
  GtkWindow* gtkwindow = NULL;
  gpointer user_data = NULL;
  nsresult rv;

  // get the gdk window from the mozilla window
  rv = GetToplevelGdkWindow(aWindow, &gdkwindow);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the gtk window from the gdk window
  gdk_window_get_user_data(gdkwindow, &user_data);
  if (!user_data) {
    return NS_ERROR_FAILURE;
  }

  // make sure it's really a gtk window
  gtkwindow = GTK_WINDOW(user_data);

  // return it
  *aToplevelWindow = gtkwindow;
  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::BeginResizeDrag(nsISupports *aWindow, nsIDOMMouseEvent* aEvent, PRInt32 aDirection)
{
  nsresult rv;
  PRInt32 screenX;
  PRInt32 screenY;
  PRUint16 button;
  PRUint32 server_time;
  GdkWindow* window = NULL;
  GdkWindowEdge window_edge;

  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aEvent);

  /* find the window that was passed in */
  rv = GetToplevelGdkWindow(aWindow, &window);
  NS_ENSURE_SUCCESS(rv, rv);

  /* get the currently pressed mouse button from the event */
  rv = aEvent->GetButton(&button);
  NS_ENSURE_SUCCESS(rv, rv);
  /* mozilla and gtk disagree about button numbering... */
  button++;

  /* to work around an old, buggy metacity we need to move ourselves */
  /* where are we? */
  gdk_window_get_position(window, &screenX, &screenY);
  /* let's move to there */
  gdk_window_move(window, screenX, screenY);


#ifdef MOUSE_POSITION_FROM_EVENT
  /* pull the mouse position out of the mozilla event */
  rv = aEvent->GetScreenX(&screenX);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aEvent->GetScreenY(&screenY);
  NS_ENSURE_SUCCESS(rv, rv);
#else
  {
    /* ask X where the mouse is */
    GdkDisplay* display = NULL;
    GdkScreen* screen = NULL;
    GdkModifierType mask;
    /* get the current (default) display */
    display = gdk_display_get_default();
    if (display == NULL) {
      return NS_ERROR_FAILURE;
    }
    /* get the current pointer position */
    gdk_display_get_pointer(display, &screen, &screenX, &screenY, &mask);
  }
#endif

  /* convert interface directions to GDK_WINDOW_EDGE* */
  switch(aDirection) {
    case sbINativeWindowManager::DIRECTION_NORTH_WEST:
      window_edge = GDK_WINDOW_EDGE_NORTH_WEST;
      break;
    case sbINativeWindowManager::DIRECTION_NORTH:
      window_edge = GDK_WINDOW_EDGE_NORTH;
      break;
    case sbINativeWindowManager::DIRECTION_NORTH_EAST:
      window_edge = GDK_WINDOW_EDGE_NORTH_EAST;
      break;
    case sbINativeWindowManager::DIRECTION_WEST:
      window_edge = GDK_WINDOW_EDGE_WEST;
      break;
    case sbINativeWindowManager::DIRECTION_EAST:
      window_edge = GDK_WINDOW_EDGE_EAST;
      break;
    case sbINativeWindowManager::DIRECTION_SOUTH_WEST:
      window_edge = GDK_WINDOW_EDGE_SOUTH_WEST;
      break;
    case sbINativeWindowManager::DIRECTION_SOUTH:
      window_edge = GDK_WINDOW_EDGE_SOUTH;
      break;
    case sbINativeWindowManager::DIRECTION_SOUTH_EAST:
      window_edge = GDK_WINDOW_EDGE_SOUTH_EAST;
      break;
    default:
      return NS_ERROR_FAILURE;
  }

  /* if I were an event, when would I happen? */
  server_time = gdk_x11_get_server_time(window);

  /* tell the window manager to start the resize */
  gdk_window_begin_resize_drag(window, window_edge, button, 
      screenX, screenY, server_time);

  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::GetSupportsResizeDrag(PRBool *aSupportsResizeDrag)
{
  NS_ENSURE_ARG_POINTER(aSupportsResizeDrag);
  
  *aSupportsResizeDrag = PR_TRUE;

  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::SetMinimumWindowSize(nsISupports *aWindow, PRInt32 aMinimumWidth, PRInt32 aMinimumHeight)
{
  nsresult rv;
  GdkWindow* window = NULL;
  GdkGeometry hints;

  NS_ENSURE_ARG_POINTER(aWindow);

  /* find the window that was passed in */
  rv = GetToplevelGdkWindow(aWindow, &window);
  NS_ENSURE_SUCCESS(rv, rv);

  /* set up the GdkGeometry with the info we have */
  hints.min_width = aMinimumWidth;
  hints.min_height = aMinimumHeight;
  gdk_window_set_geometry_hints(window, &hints, GDK_HINT_MIN_SIZE);

  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::GetSupportsMinimumWindowSize(PRBool *aSupportsMinimumWindowSize)
{
  NS_ENSURE_ARG_POINTER(aSupportsMinimumWindowSize);
  *aSupportsMinimumWindowSize = PR_TRUE;
  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::SetOnTop(nsISupports *aWindow, PRBool aOnTop)
{
  nsresult rv;
  GtkWindow* window = NULL;

  NS_ENSURE_ARG_POINTER(aWindow);

  /* if we're not mapped yet, we can't just set keep_above on our gdk window.
   * if we do the gtk-window will override it when it maps. so we need to find
   * the gtk window instead of the gdk window
   */

  /* find the gtk window that was passed in */
  rv = GetToplevelGtkWindow(aWindow, &window);
  NS_ENSURE_SUCCESS(rv, rv);

  /* tell the window manager to put this window on top */
  gtk_window_set_keep_above(window, aOnTop);

  return NS_OK;
}


NS_IMETHODIMP sbNativeWindowManager::GetSupportsOnTop(PRBool *aSupportsOnTop)
{
  NS_ENSURE_ARG_POINTER(aSupportsOnTop);

  *aSupportsOnTop = PR_TRUE;

  return NS_OK;
}


