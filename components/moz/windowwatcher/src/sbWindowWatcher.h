/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

#ifndef __SB_WINDOWWATCHER_H__
#define __SB_WINDOWWATCHER_H__

/**
 * \file  sbWindowWatcher.h
 * \brief Songbird Window Watcher Definitions.
 */

// Songbird imports.
#include <sbIWindowWatcher.h>

// Mozilla imports.
#include <nsClassHashtable.h>
#include <nsCOMArray.h>
#include <nsIDOMEventListener.h>
#include <nsIDOMEventTarget.h>
#include <nsIDOMWindow.h>
#include <nsIObserver.h>
#include <nsIObserverService.h>
#include <nsIThreadManager.h>
#include <nsIWindowMediator.h>
#include <nsIWindowWatcher.h>
#include <nsTArray.h>
#include <prmon.h>
#include <mozilla/ReentrantMonitor.h>

#include <sbWeakReference.h>

/**
 * This class implements the Songbird window watcher interface.  This class
 * watches for windows to become available and ready.  A window is considered
 * read after all of its overlays have loaded.
 */

class sbWindowWatcherEventListener;

class sbWindowWatcher : public sbIWindowWatcher,
                        public nsIObserver,
                        public sbSupportsWeakReference
{
  //----------------------------------------------------------------------------
  //
  // Class friends.
  //
  //----------------------------------------------------------------------------

  friend class sbWindowWatcherEventListener;


  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIWINDOWWATCHER
  NS_DECL_NSIOBSERVER


  //
  // Songbird window watcher services.
  //

  sbWindowWatcher();

  virtual ~sbWindowWatcher();

  nsresult Init();

  void Finalize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mWindowWatcher             Window watcher service.
  // mWindowMediator            Window mediator service.
  // mObserverService           Observer service.
  // mThreadManager             Thread manager.
  // mSentMainWinPresentedNotification
  //                            True if the main window presented notification
  //                            has been sent.
  // mMonitor                   Monitor for the window watcher.
  // mIsShuttingDown            True if the window watcher service is shutting
  //                            down.
  // mWindowList                List of windows.
  // mWindowInfoTable           Table of window information.
  // mCallWithWindowList        List of call with window callbacks.
  // mServicingCallWithWindowList
  //                            True if the call with window list is being
  //                            serviced.
  //
  // The following fields must only be accessed under the window watcher
  // monitor:
  //
  //   mIsShuttingDown
  //   mWindowList
  //   mWindowInfoTable
  //   mCallWithWindowList
  //   mServicingCallWithWindowList
  //

  nsCOMPtr<nsIWindowWatcher>    mWindowWatcher;
  nsCOMPtr<nsIWindowMediator>   mWindowMediator;
  nsCOMPtr<nsIObserverService>  mObserverService;
  nsCOMPtr<nsIThreadManager>    mThreadManager;
  PRBool                        mSentMainWinPresentedNotification;
  mozilla::ReentrantMonitor     mMonitor;
  PRBool                        mIsShuttingDown;
  nsCOMArray<nsIDOMWindow>      mWindowList;

  class WindowInfo
  {
  public:
    WindowInfo() : isReady(PR_FALSE)
    {
      MOZ_COUNT_CTOR(WindowInfo);
    }
    ~WindowInfo()
    {
      MOZ_COUNT_DTOR(WindowInfo);
    }

    nsCOMPtr<nsIDOMWindow>      window;
    nsCOMPtr<nsIDOMEventTarget> eventTarget;
    nsRefPtr<sbWindowWatcherEventListener>
                                eventListener;
    PRBool                      isReady;
  };
  nsClassHashtable<nsISupportsHashKey, WindowInfo>
                                mWindowInfoTable;

  typedef struct
  {
    nsString                            windowType;
    nsCOMPtr<sbICallWithWindowCallback> callback;
  } CallWithWindowInfo;
  nsTArray<CallWithWindowInfo>  mCallWithWindowList;
  PRBool                        mServicingCallWithWindowList;


  //
  // Internal Songbird window watcher nsIObserver services.
  //

  nsresult OnDOMWindowOpened(nsISupports*     aSubject,
                             const PRUnichar* aData);

  nsresult OnDOMWindowClosed(nsISupports*     aSubject,
                             const PRUnichar* aData);

  nsresult OnQuitApplicationGranted();


  //
  // Internal Songbird window watcher services.
  //

  void Shutdown();

  nsresult AddWindow(nsIDOMWindow* aWindow);

  nsresult RemoveWindow(nsIDOMWindow* aWindow);

  void RemoveAllWindows();

  void OnWindowReady(nsIDOMWindow* aWindow);

  nsresult GetWindowType(nsIDOMWindow* aWindow,
                         nsAString&    aWindowType);

  nsresult InvokeCallWithWindowCallbacks(nsIDOMWindow* aWindow);

  nsresult
  CallWithWindow_Proxy(const nsAString&           aWindowType,
                             sbICallWithWindowCallback* aCallback,
                             PRBool                   aWait);
};


//
// Songbird window watcher component defs.
//

#define SB_WINDOWWATCHER_CLASSNAME "sbWindowWatcher"
#define SB_WINDOWWATCHER_CID                                                   \
  /* {4d83ab89-f909-4e71-8d5d-d0884d0cb788} */                                 \
  {                                                                            \
    0x4d83ab89,                                                                \
    0xf909,                                                                    \
    0x4e71,                                                                    \
    { 0x8d, 0x5d, 0xd0, 0x88, 0x4d, 0x0c, 0xb7, 0x88 }                         \
  }


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird window watcher event listener class.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * This class handles window events for the Songbird window watcher.
 */

class sbWindowWatcherEventListener : public nsIDOMEventListener
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMEVENTLISTENER


  //
  // Event listener services.
  //

  static nsresult New(sbWindowWatcherEventListener** aListener,
                      sbWindowWatcher*               aSBWindowWatcher,
                      nsIDOMWindow*                  aWindow);

  /**
   * Listen for the given event on the DOM window associated with this listener
   */
  nsresult AddEventListener(const char* aEventName);

  /**
   * Clear all event listeners
   */
  nsresult ClearEventListeners();

  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mWeakSBWindowWatcher       Weak Songbird window watcher reference.
  // mSBWindowWatcher           Songbird window watcher.
  // mWindow                    Window for which to handle events.
  // mEventTarget               The event target to listen to
  // mOutstandingEvents         The names of events that still have to occur
  //

  nsCOMPtr<nsIWeakReference>    mWeakSBWindowWatcher;
  sbWindowWatcher*              mSBWindowWatcher;
  nsCOMPtr<nsIDOMWindow>        mWindow;
  nsCOMPtr<nsIDOMEventTarget>   mEventTarget;
  nsTArray<nsString>            mOutstandingEvents;


  //
  // Internal event listener services.
  //

  nsresult Initialize();

  sbWindowWatcherEventListener(sbWindowWatcher* aSBWindowWatcher,
                               nsIDOMWindow*    aWindow) :
    mSBWindowWatcher(aSBWindowWatcher),
    mWindow(aWindow)
  {
  }
};


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird window watcher wait for window class.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * This class gets windows of a specified type and will wait until a window of
 * the specified type is available and ready.
 */

class sbWindowWatcherWaitForWindow : public sbICallWithWindowCallback
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBICALLWITHWINDOWCALLBACK


  //
  // Get window services.
  //

  static nsresult New(sbWindowWatcherWaitForWindow** aWaitForWindow);

  virtual ~sbWindowWatcherWaitForWindow();

  nsresult Wait(const nsAString& aWindowType);


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Internal wait for window fields.
  //
  //   mSBWindowWatcher         Songbird window watcher service.
  //   mReadyMonitor            Window ready monitor.
  //   mWindow                  Window to return.
  //   mReady                   True when window is ready.
  //
  // The following fields must only be accessed under the ready monitor:
  //   mWindow
  //   mReady
  //

  nsCOMPtr<sbIWindowWatcher>    mSBWindowWatcher;
  mozilla::ReentrantMonitor     mReadyMonitor;
  nsCOMPtr<nsIDOMWindow>        mWindow;
  PRBool                        mReady;

  //
  // Internal wait for window services.
  //

  nsresult Initialize();

  sbWindowWatcherWaitForWindow();
};


#endif // __SB_WINDOWWATCHER_H__

