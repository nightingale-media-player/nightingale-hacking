/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device progress listener.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDeviceProgressListener.cpp
 * \brief Nightingale Device Progress Listener Source.
 */

//------------------------------------------------------------------------------
//
// Device progress listener imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbDeviceProgressListener.h"

// Mozilla imports.
#include <nsAutoLock.h>
#include <nsAutoPtr.h>


//------------------------------------------------------------------------------
//
// Device progress listener nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceProgressListener, sbIJobProgressListener)


//------------------------------------------------------------------------------
//
// Device progress listener sbIJobProgressListener implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// OnJobProgress
//

NS_IMETHODIMP
sbDeviceProgressListener::OnJobProgress(sbIJobProgress* aJobProgress)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aJobProgress);

  // Function variables.
  nsresult rv;

  // Get the job status.
  PRUint16 status;
  rv = aJobProgress->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check for job completion.
  if (status != sbIJobProgress::STATUS_RUNNING) {
    if (mCompleteNotifyMonitor) {
      nsAutoMonitor monitor(mCompleteNotifyMonitor);
      PR_AtomicSet(&mIsComplete, 1);
      monitor.Notify();
    }
    else {
      PR_AtomicSet(&mIsComplete, 1);
    }
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Device progress listener public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// New
//

/* static */ nsresult
sbDeviceProgressListener::New
                            (sbDeviceProgressListener** aDeviceProgressListener,
                             PRMonitor*                 aCompleteNotifyMonitor)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceProgressListener);

  // Create the device progress listener object.
  nsRefPtr<sbDeviceProgressListener>
    listener = new sbDeviceProgressListener(aCompleteNotifyMonitor);
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);

  // Return results.
  listener.forget(aDeviceProgressListener);

  return NS_OK;
}


//-------------------------------------
//
// IsComplete
//

PRBool
sbDeviceProgressListener::IsComplete()
{
  return mIsComplete;
}


//-------------------------------------
//
// sbDeviceProgressListener
//

sbDeviceProgressListener::sbDeviceProgressListener
                            (PRMonitor* aCompleteNotifyMonitor) :
  mCompleteNotifyMonitor(aCompleteNotifyMonitor),
  mIsComplete(PR_FALSE)
{
}


//-------------------------------------
//
// ~sbDeviceProgressListener
//

sbDeviceProgressListener::~sbDeviceProgressListener()
{
}

