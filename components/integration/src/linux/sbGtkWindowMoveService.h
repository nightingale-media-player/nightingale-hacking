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

#ifndef sbGtkWindowMoveService_h_
#define sbGtkWindowMoveService_h_

#include <sbIWindowMoveService.h>
#include <nsITimer.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <gtk/gtk.h>
#include <map>


//==============================================================================
//
// @inteface sbGtkWindowContext
// @brief An interface for stashing (x,y) coordinates and a listener callback.
//
//==============================================================================

class sbGtkWindowContext : public nsITimerCallback 
{
public:
  sbGtkWindowContext(sbIWindowMoveListener *aListener,
                     PRInt32 aPreviousX,
                     PRInt32 aPreviousY,
                     PRInt32 aConfigureHandlerID,
                     PRInt32 aDestroyHandlerID);
  virtual ~sbGtkWindowContext();

  //
  // @brief Inform the window context that a frame event has occured. If a
  //        move has been detected, the listener will be notified and a timer
  //        will be set to notify the listener when the move has stopped.
  // @param aEvent The GDK event class from the move signal. 
  //
  nsresult OnFrameEvent(GdkEvent *aEvent);

  //
  // @brief Call this method when the listening window is destroyed so that
  //        any existing listeners and timers can be cleaned up.
  //
  nsresult OnWindowDestroyed();

  //
  // @brief Get the handler IDs used to connect a signal to the window that
  //        was used when this context was created.
  // @param aOutConfigureHandlerID The "configure-event" handler ID.
  // @parma aOutDestroyHandlerID The "destroy" handler ID.
  //
  nsresult GetSignalHandlerIDs(PRInt32 *aOutConfigureHandlerID,
                               PRInt32 *aOutDestroyHandlerID);

  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

private:
  nsCOMPtr<sbIWindowMoveListener> mListener;
  nsCOMPtr<nsITimer>              mTimer;
  PRBool                          mIsTimerSet;
  PRBool                          mShouldReArmTimer;
  PRInt32                         mPreviousX;
  PRInt32                         mPreviousY;
  PRInt32                         mConfigureHandlerID;
  PRInt32                         mDestroyHandlerID;
};

typedef std::map<GtkWindow *, nsRefPtr<sbGtkWindowContext> > sbGtkWindowContextMap;
typedef sbGtkWindowContextMap::value_type         sbGtkWindowContextMapPair;
typedef sbGtkWindowContextMap::iterator           sbGtkWindowMoveServiceIter;


//==============================================================================
//
// @interface sbGtkWindowMoveService
// @brief GTK window move service implementation.
//
//==============================================================================

class sbGtkWindowMoveService : public sbIWindowMoveService 
{
public:
  sbGtkWindowMoveService();
  virtual ~sbGtkWindowMoveService();

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWMOVESERVICE

  nsresult OnWindowFrameCallback(GtkWindow *aWindow, GdkEvent *aEvent);
  nsresult OnWindowDestroyed(GtkWindow *aWindow);

private:
  sbGtkWindowContextMap mWindowContextMap;
};

#endif  // sbGtkWindowMoveService_h_

