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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird thread utilities services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbThreadUtils.cpp
 * \brief Songbird Thread Utilities Services Source.
 */

//------------------------------------------------------------------------------
//
// Songbird thread utilities imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbThreadUtils.h"

// Mozilla imports.
#include <nsIThreadManager.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird thread utilities services.
//
//------------------------------------------------------------------------------

/**
 * Check if the current thread is the main thread and return true if so.  Use
 * the thread manager object specified by aThreadManager if provided.  This
 * function can be used during XPCOM shutdown if aThreadManager is provided.
 *
 * \param aThreadManager        Optional thread manager.  Defaults to null.
 *
 * \return PR_TRUE              Current thread is main thread.
 */

PRBool
SB_IsMainThread(nsIThreadManager* aThreadManager)
{
  nsresult rv;

  // Get the thread manager if not provided.
  nsCOMPtr<nsIThreadManager> threadManager = aThreadManager;
  if (!threadManager) {
    threadManager = do_GetService("@mozilla.org/thread-manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, PR_FALSE);
  }

  // Check if the current thread is the main thread.
  PRBool isMainThread;
  rv = threadManager->GetIsMainThread(&isMainThread);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  return isMainThread;
}



NS_IMETHODIMP
sbRunnable::Run()
{
  // Enter the monitor to set the done flag:
  mozilla::MonitorAutoEnter lock(mMonitor);

  // Set the done flag and notify all waiters:
  mDone = true;
  mMonitor.NotifyAll();

  return NS_OK;
}



PRBool
sbRunnable::Wait(PRIntervalTime aTimeout)
{
  // Compute a fixed expiration time that won't drift:
  const PRIntervalTime expiry = PR_IntervalNow() + aTimeout;

  // Enter the monitor to check the done flag:
  mozilla::MonitorAutoEnter lock(mMonitor);

  // Only wait for Run() to complete if asked:
  if (aTimeout != PR_INTERVAL_NO_WAIT) {
    // Loop every time the monitor is signaled, until done
    // or timed out:
    while (!mDone) {
      // Compute how long to wait: indefinitely if PR_INTERVAL_NO_TIMEOUT
      // was given, or the time remaining until the fixed expiration time,
      // otherwise.
      PRIntervalTime timeout;
      if (aTimeout == PR_INTERVAL_NO_TIMEOUT) {
        timeout = PR_INTERVAL_NO_TIMEOUT;
      }
      else {
        timeout = expiry - PR_IntervalNow();
        if (PRInt32(timeout) <= 0) {
          break;
        }
      }
      // Wait for a signal from Run(), or until timed out
      mMonitor.Wait(timeout);
    }
  }

  // Return the done flag:
  return mDone;
}
