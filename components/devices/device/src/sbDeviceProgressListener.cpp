/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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
// Device progress listener.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDeviceProgressListener.cpp
 * \brief Songbird Device Progress Listener Source.
 */

//------------------------------------------------------------------------------
//
// Device progress listener imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbDeviceProgressListener.h"

// Local imports.
#include "sbDeviceStatusHelper.h"

// Mozilla imports.
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

  // Update the device status.
  if (mDeviceStatusHelper) {
    // Get the job progress.
    PRUint32 progress;
    PRUint32 total;
    rv = aJobProgress->GetProgress(&progress);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aJobProgress->GetTotal(&total);
    NS_ENSURE_SUCCESS(rv, rv);

    // Update the device status.
    if (total > 0) {
      mDeviceStatusHelper->ItemProgress(static_cast<double>(progress) /
                                        static_cast<double>(total));
    }
  }

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
                             PRMonitor*                 aCompleteNotifyMonitor,
                             sbDeviceStatusHelper*      aDeviceStatusHelper)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceProgressListener);

  // Create the device progress listener object.
  nsRefPtr<sbDeviceProgressListener>
    listener = new sbDeviceProgressListener(aCompleteNotifyMonitor,
                                            aDeviceStatusHelper);
  NS_ENSURE_TRUE(listener, NS_ERROR_OUT_OF_MEMORY);

  // Return results.
  listener.forget(aDeviceProgressListener);

  return NS_OK;
}


//-------------------------------------
//
// IsComplete
//

bool
sbDeviceProgressListener::IsComplete()
{
  return mIsComplete;
}


//-------------------------------------
//
// sbDeviceProgressListener
//

sbDeviceProgressListener::sbDeviceProgressListener
                            (PRMonitor*            aCompleteNotifyMonitor,
                             sbDeviceStatusHelper* aDeviceStatusHelper) :
  mCompleteNotifyMonitor(aCompleteNotifyMonitor),
  mDeviceStatusHelper(aDeviceStatusHelper),
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

