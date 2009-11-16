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
 * \file  sbIPDFairPlayEvent.cpp
 * \brief Songbird iPod Device FairPlay Event Source.
 */

//------------------------------------------------------------------------------
//
// iPod device FairPlay event imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbIPDFairPlayEvent.h"

// Songbird imports.
#include <sbIDeviceManager.h>

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// iPod device FairPlay event nsISupports implementation.
//
//------------------------------------------------------------------------------

//
// NS_IMPL_THREADSAFE_ISUPPORTS4(sbIPDFairPlayEvent,
//                               sbIIPDFairPlayEvent,
//                               sbIIPDDeviceEvent,
//                               sbIDeviceEvent,
//                               sbDeviceEvent)
//

NS_IMPL_THREADSAFE_ADDREF(sbIPDFairPlayEvent)
NS_IMPL_THREADSAFE_RELEASE(sbIPDFairPlayEvent)
NS_INTERFACE_MAP_BEGIN(sbIPDFairPlayEvent)
  NS_INTERFACE_MAP_ENTRY(sbIIPDFairPlayEvent)
  NS_INTERFACE_MAP_ENTRY(sbIIPDDeviceEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(sbIDeviceEvent, sbIIPDDeviceEvent)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIIPDDeviceEvent)
  NS_INTERFACE_MAP_ENTRY(sbDeviceEvent)
NS_INTERFACE_MAP_END


//------------------------------------------------------------------------------
//
// iPod device FairPlay event sbIIPDFairPlayEvent implementation.
//
//------------------------------------------------------------------------------

//
// Getters/setters.
//

/**
 * The FairPlay user ID.
 */

NS_IMETHODIMP
sbIPDFairPlayEvent::GetUserID(PRUint32* aUserID)
{
  NS_ENSURE_ARG_POINTER(aUserID);
  *aUserID = mUserID;
  return NS_OK;
}


/**
 * The FairPlay account name.
 */

NS_IMETHODIMP
sbIPDFairPlayEvent::GetAccountName(nsAString& aAccountName)
{
  aAccountName.Assign(mAccountName);
  return NS_OK;
}


/**
 * The FairPlay user name.
 */

NS_IMETHODIMP
sbIPDFairPlayEvent::GetUserName(nsAString& aUserName)
{
  aUserName.Assign(mUserName);
  return NS_OK;
}


/**
 * The FairPlay media item.
 */

NS_IMETHODIMP
sbIPDFairPlayEvent::GetMediaItem(sbIMediaItem** aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_IF_ADDREF(*aMediaItem = mMediaItem);
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod device FairPlay event services.
//
//------------------------------------------------------------------------------

/**
 * Create a FairPlay event of the type specified by aType for the device
 * specified by aDevice.  Set the event information as specified by aUserID,
 * aAccountName, aUserName, and aMediaItem.  Return event in aFairPlayEvent.
 *
 * \param aFairPlayEvent        Created event.
 * \param aDevice               Device target of event.
 * \param aType                 Event type.
 * \param aUserID               FairPlay userID.
 * \param aAccountName          FairPlay account name.
 * \param aUserName             FairPlay user name.
 * \param aMediaItem            FairPlay media item.
 */

/* static */
nsresult
sbIPDFairPlayEvent::CreateEvent(sbIPDFairPlayEvent** aFairPlayEvent,
                                sbIDevice*           aDevice,
                                PRUint32             aType,
                                PRUint32             aUserID,
                                nsAString&           aAccountName,
                                nsAString&           aUserName,
                                sbIMediaItem*        aMediaItem)
{
  // Validate parameters.
  NS_ENSURE_ARG_POINTER(aFairPlayEvent);
  NS_ENSURE_ARG_POINTER(aDevice);

  // Function variables.
  nsresult rv;

  // Create and initialize an event object.
  nsRefPtr<sbIPDFairPlayEvent> event = new sbIPDFairPlayEvent();
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);
  rv = event->InitEvent(aDevice,
                        aType,
                        aUserID,
                        aAccountName,
                        aUserName,
                        aMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  NS_ADDREF(*aFairPlayEvent = event);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal iPod device FairPlay event services.
//
//------------------------------------------------------------------------------

/**
 * Construct an iPod device FairPlay event object.
 */

sbIPDFairPlayEvent::sbIPDFairPlayEvent()
{
}


/**
 * Destroy an iPod device FairPlay event object.
 */

sbIPDFairPlayEvent::~sbIPDFairPlayEvent()
{
}


/**
 * Initialize a FairPlay event of the type specified by aType for the device
 * specified by aDevice.  Set the event information as specified by aUserID,
 * aAccountName, aUserName, and aMediaItem.
 *
 * \param aDevice               Device target of event.
 * \param aType                 Event type.
 * \param aUserID               FairPlay userID.
 * \param aAccountName          FairPlay account name.
 * \param aUserName             FairPlay user name.
 * \param aMediaItem            FairPlay media item.
 */

nsresult
sbIPDFairPlayEvent::InitEvent(sbIDevice*           aDevice,
                              PRUint32             aType,
                              PRUint32             aUserID,
                              nsAString&           aAccountName,
                              nsAString&           aUserName,
                              sbIMediaItem*        aMediaItem)
{
  // Validate parameters.
  NS_ASSERTION(aDevice, "aDevice is null");

  // Function variables.
  nsresult rv;

  // Get the device manager.
  nsCOMPtr<sbIDeviceManager2>
    manager = do_GetService("@songbirdnest.com/Songbird/DeviceManager;2", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 deviceState;
  rv = aDevice->GetState(&deviceState);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create a device event.
  rv = manager->CreateEvent(aType,
                            nsnull,
                            aDevice,
                            deviceState,
                            sbIDevice::STATE_IDLE,
                            getter_AddRefs(mDeviceEvent));
  NS_ENSURE_SUCCESS(rv, rv);
  mSBDeviceEvent = do_QueryInterface(mDeviceEvent, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the FairPlay event info.
  mUserID = aUserID;
  mAccountName.Assign(aAccountName);
  mUserName.Assign(aUserName);
  mMediaItem = aMediaItem;

  return NS_OK;
}

