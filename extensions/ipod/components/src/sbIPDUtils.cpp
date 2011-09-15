/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
//
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=END SONGBIRD GPL
*/

/**
 * \file  sbIPDUtils.cpp
 * \brief Songbird iPod Device Utility Source.
 */

//------------------------------------------------------------------------------
//
// iPod device utility imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDUtils.h"

// Mozilla imports.
#include <nsServiceManagerUtils.h>

// Songbird imports.
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceManager.h>


//------------------------------------------------------------------------------
//
// iPod device utility services.
//
//------------------------------------------------------------------------------

/**
 * Create and dispatch an event through the device manager of the type specified
 * by aType with the data specified by aData, originating from the object
 * specified by aOrigin.  If aAsync is true, dispatch and return immediately;
 * otherwise, wait for the event handling to complete.
 * aData, aOrigin, and aAsync are all optional.  Default is to dispatch
 * synchronously.
 *
 * \param aType                 Type of event.
 * \param aData                 Event data.
 * \param aOrigin               Origin of event.
 * \param aDeviceState          State of the device
 * \param aAsync                If true, dispatch asynchronously.
 */

nsresult
CreateAndDispatchDeviceManagerEvent(PRUint32     aType,
                                    nsIVariant*  aData,
                                    nsISupports* aOrigin,
                                    PRUint32     aDeviceState,
                                    PRBool       aAsync)
{
  nsresult rv;

  // Get the device manager.
  nsCOMPtr<sbIDeviceManager2>
    manager = do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Use the device manager as the event target.
  nsCOMPtr<sbIDeviceEventTarget> eventTarget = do_QueryInterface(manager, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the event.
  nsCOMPtr<sbIDeviceEvent> event;
  rv = manager->CreateEvent(aType,
                            aData,
                            aOrigin,
                            aDeviceState,
                            sbIDevice::STATE_IDLE,
                            getter_AddRefs(event));
  NS_ENSURE_SUCCESS(rv, rv);

  // Dispatch the event.
  PRBool dispatched;
  rv = eventTarget->DispatchEvent(event, aAsync, &dispatched);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

