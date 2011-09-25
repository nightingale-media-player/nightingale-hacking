/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */

#include "sbGtkWindowMoveService.h"
#include "../NativeWindowFromNode.h"

#include <nsIWidget.h>
#include <nsComponentManagerUtils.h>

#define SB_WIN_MOVE_TIMEOUT 100 


//==============================================================================
// GTK signal callbacks. 
//==============================================================================

void frame_callback(GtkWindow *aWindow, GdkEvent *aEvent, gpointer data)
{
  sbGtkWindowMoveService *winService =
    (sbGtkWindowMoveService *)data;
  NS_ENSURE_TRUE(winService, /* void */);

  winService->OnWindowFrameCallback(aWindow, aEvent);
}

void window_destroyed(GtkWindow *aWindow, gpointer data)
{
  sbGtkWindowMoveService *winService =
    (sbGtkWindowMoveService *)data;
  NS_ENSURE_TRUE(winService, /* void */);

  winService->OnWindowDestroyed(aWindow);
}


//==============================================================================
// sbGtkWindowContext
//==============================================================================

NS_IMPL_ISUPPORTS1(sbGtkWindowContext, nsITimerCallback)

sbGtkWindowContext::sbGtkWindowContext(sbIWindowMoveListener *aListener,
                                       PRInt32 aPreviousX,
                                       PRInt32 aPreviousY,
                                       PRInt32 aConfigureHandlerID,
                                       PRInt32 aDestroyHandlerID)
  : mListener(aListener)
  , mIsTimerSet(PR_FALSE)
  , mShouldReArmTimer(PR_FALSE)
  , mPreviousX(aPreviousX)
  , mPreviousY(aPreviousY)
  , mConfigureHandlerID(aConfigureHandlerID)
  , mDestroyHandlerID(aDestroyHandlerID)
{
  nsresult rv;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
}

sbGtkWindowContext::~sbGtkWindowContext()
{
}

nsresult
sbGtkWindowContext::OnFrameEvent(GdkEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  nsresult rv;
  
  // Only process this event if the (x,y) coords have moved.
  if (aEvent->configure.x != mPreviousX ||
      aEvent->configure.y != mPreviousY)
  {
    // Stash the new (x,y) coords.
    mPreviousX = aEvent->configure.x;
    mPreviousY = aEvent->configure.y;
    
    if (mIsTimerSet) {
      // Since the window has moved, and the move stop timer is still set
      // the flag to re-arm the timer when the timer eventually fires.
      mShouldReArmTimer = PR_TRUE;
    }
    else {
      rv = mListener->OnMoveStarted();
      NS_ENSURE_SUCCESS(rv, rv);

      // Notify the listener that a move occured and then arm the timer.
      rv = mTimer->InitWithCallback(this,
          SB_WIN_MOVE_TIMEOUT,
          nsITimer::TYPE_ONE_SHOT);
      NS_ENSURE_SUCCESS(rv, rv);

      mIsTimerSet = PR_TRUE;
      mShouldReArmTimer = PR_FALSE;
    }
  }

  return NS_OK;
}

nsresult
sbGtkWindowContext::OnWindowDestroyed()
{
  // The window is closing, kill the move timer.
  if (mIsTimerSet) {
    nsresult rv = mTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);

    mIsTimerSet = PR_FALSE;
    mShouldReArmTimer = PR_FALSE;
  }
  return NS_OK;
}

nsresult
sbGtkWindowContext::GetSignalHandlerIDs(PRInt32 *aOutConfigureHandlerID,
                                        PRInt32 *aOutDestroyHandlerID)
{
  NS_ENSURE_ARG_POINTER(aOutConfigureHandlerID);
  NS_ENSURE_ARG_POINTER(aOutDestroyHandlerID);

  *aOutConfigureHandlerID = mConfigureHandlerID;
  *aOutDestroyHandlerID = mDestroyHandlerID;
  
  return NS_OK;
}

//------------------------------------------------------------------------------
// nsITimerCallback

NS_IMETHODIMP
sbGtkWindowContext::Notify(nsITimer *aTimer)
{
  NS_ENSURE_ARG_POINTER(aTimer);

  nsresult rv;
  if (mShouldReArmTimer) {
    // The window was moved during the timer wait, re-set the timer.
    rv = mTimer->InitWithCallback(this,
                                  SB_WIN_MOVE_TIMEOUT,
                                  nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    mShouldReArmTimer = PR_FALSE;
  }
  else {
    // The window has now stopped moving by our records, invoke the listener.
    mIsTimerSet = PR_FALSE;
    mShouldReArmTimer = PR_FALSE;

    rv = mListener->OnMoveStopped();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


//==============================================================================
// sbGtkWindowMoveService
//==============================================================================

NS_IMPL_ISUPPORTS1(sbGtkWindowMoveService, sbIWindowMoveService)

sbGtkWindowMoveService::sbGtkWindowMoveService()
{
}

sbGtkWindowMoveService::~sbGtkWindowMoveService()
{
}

nsresult
sbGtkWindowMoveService::OnWindowFrameCallback(GtkWindow *aWindow,
                                              GdkEvent *aEvent)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aEvent);

  // First, find the context reference for this window to see if the (x,y)
  // coords have been modified.
  sbGtkWindowMoveServiceIter foundIter =
    mWindowContextMap.find(aWindow);
  NS_ENSURE_TRUE(foundIter != mWindowContextMap.end(), NS_ERROR_FAILURE);
  
  nsRefPtr<sbGtkWindowContext> winContext((*foundIter).second);
  NS_ENSURE_TRUE(winContext, NS_ERROR_FAILURE);

  nsresult rv = winContext->OnFrameEvent(aEvent);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbGtkWindowMoveService::OnWindowDestroyed(GtkWindow *aWindow)
{
  NS_ENSURE_ARG_POINTER(aWindow);

  sbGtkWindowMoveServiceIter foundIter =
    mWindowContextMap.find(aWindow);
  NS_ENSURE_TRUE(foundIter != mWindowContextMap.end(), NS_ERROR_FAILURE);

  nsRefPtr<sbGtkWindowContext> winContext((*foundIter).second);
  NS_ENSURE_TRUE(winContext, NS_ERROR_FAILURE);

  nsresult rv = winContext->OnWindowDestroyed();
  NS_ENSURE_SUCCESS(rv, rv);

  // Now remove this item from the map.
  mWindowContextMap.erase(foundIter); 

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIWindowMoveService

NS_IMETHODIMP
sbGtkWindowMoveService::StartWatchingWindow(nsISupports *aWindow,
                                            sbIWindowMoveListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aListener);

  nsIWidget *widget = NativeWindowFromNode::getWidget(aWindow);
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  GtkWidget *window =
    reinterpret_cast<GtkWidget *>(widget->GetNativeData(NS_NATIVE_SHELLWIDGET));
  NS_ENSURE_TRUE(window, NS_ERROR_FAILURE);
 
  // Listen for frame events for move notifications.
  gtk_widget_add_events(GTK_WIDGET(window), GDK_CONFIGURE);
  PRInt32 configureHandlerID = g_signal_connect(G_OBJECT(window),
                                                "configure-event",
                                                G_CALLBACK(frame_callback),
                                                this);

  // Listen for window close events to cleanup the context that will be
  // created for this window.
  PRInt32 destroyHandlerID = g_signal_connect(G_OBJECT(window),
                                              "destroy",
                                              G_CALLBACK(window_destroyed),
                                              this);

  // Stash this widget as a |GtkWindow| object.
  GtkWindow *widgetWindow = GTK_WINDOW(window);

  // Get the current (x,y) coords.
  PRInt32 curX, curY;
  gtk_window_get_position(widgetWindow, &curX, &curY);
  
  // Stash the window with some context in the listener map.
  nsRefPtr<sbGtkWindowContext> context = new sbGtkWindowContext(
      aListener, curX, curY, configureHandlerID, destroyHandlerID);
  NS_ENSURE_TRUE(context, NS_ERROR_FAILURE);

  mWindowContextMap.insert(sbGtkWindowContextMapPair(widgetWindow, context));
  return NS_OK;
}

NS_IMETHODIMP
sbGtkWindowMoveService::StopWatchingWindow(nsISupports *aWindow,
                                           sbIWindowMoveListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aWindow);
  NS_ENSURE_ARG_POINTER(aListener);

  // Clear out the matching window context for the closing window.
  nsIWidget *widget = NativeWindowFromNode::getWidget(aWindow);
  NS_ENSURE_TRUE(widget, NS_ERROR_FAILURE);

  GtkWidget *window =
    reinterpret_cast<GtkWidget *>(widget->GetNativeData(NS_NATIVE_SHELLWIDGET));

  GtkWindow *widgetWindow = GTK_WINDOW(window); 

  sbGtkWindowMoveServiceIter foundIter =
    mWindowContextMap.find(widgetWindow);
  if (foundIter != mWindowContextMap.end()) {
    // Before removing the context from the map, remove the handler ID signal
    // handlers first.
    nsRefPtr<sbGtkWindowContext> winContext((*foundIter).second);
    NS_ENSURE_TRUE(winContext, NS_ERROR_FAILURE);

    PRInt32 configureHandlerID, destroyHandlerID;
    nsresult rv = winContext->GetSignalHandlerIDs(&configureHandlerID,
                                                  &destroyHandlerID);
    NS_ENSURE_SUCCESS(rv, rv);

    g_signal_handler_disconnect(window, configureHandlerID);
    g_signal_handler_disconnect(window, destroyHandlerID);
    
    mWindowContextMap.erase(foundIter);
  }

  return NS_OK;
}

