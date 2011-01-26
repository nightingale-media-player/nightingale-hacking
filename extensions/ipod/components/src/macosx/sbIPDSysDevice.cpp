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

//------------------------------------------------------------------------------
//
// iPod system dependent device services.
//
//------------------------------------------------------------------------------

/**
 * \file  sbIPDSysDevice.cpp
 * \brief Songbird iPod System Dependent Device Source.
 */

//------------------------------------------------------------------------------
//
// iPod system dependent device imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbIPDSysDevice.h"

// Local imports.
#include "sbIPDLog.h"

// Mozilla imports.
#include <nsIPropertyBag.h>
#include <nsIPropertyBag2.h>
#include <nsIWritablePropertyBag.h>

// Songbird imports
#include <sbStandardDeviceProperties.h>
#include <sbDebugUtils.h>


//------------------------------------------------------------------------------
//
// iPod system dependent device sbIDevice services.
//
//------------------------------------------------------------------------------

/**
 * Eject device.
 */

NS_IMETHODIMP
sbIPDSysDevice::Eject()
{
  // call the parent
  nsresult rv = sbIPDDevice::Eject();
  NS_ENSURE_SUCCESS(rv, rv);

  OSErr osErr;

  // Eject the device volume.
  pid_t dissenterPID;
  osErr = FSEjectVolumeSync(mVolumeRefNum, 0, &dissenterPID);
  NS_ENSURE_TRUE(osErr == noErr, NS_ERROR_FAILURE);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod system dependent device services.
//
//------------------------------------------------------------------------------

/**
 * Construct an iPod system dependent device object.
 *
 * \param aControllerID         ID of device controller.
 * \param aProperties           Device properties.
 */

sbIPDSysDevice::sbIPDSysDevice(const nsID&     aControllerID,
                               nsIPropertyBag* aProperties) :
  sbIPDDevice(aControllerID, aProperties),
  mProperties(aProperties)
{
  // Validate parameters.
  NS_ASSERTION(aProperties, "aProperties is null");

  SB_PRLOG_SETUP(sbIPDSysDevice);

  // Log progress.
  LOG("Enter: sbIPDSysDevice::sbIPDSysDevice\n");
}


/**
 * Destroy an iPod system dependent device object.
 */

sbIPDSysDevice::~sbIPDSysDevice()
{
  // Log progress.
  LOG("Enter: sbIPDSysDevice::~sbIPDSysDevice\n");

  // Finalize the iPod system dependent device object.
  Finalize();
}


/**
 * Initialize the iPod system dependent device object.
 */

nsresult
sbIPDSysDevice::Initialize()
{
  nsresult rv;

  // Get the device properties.
  nsCOMPtr<nsIPropertyBag2> properties = do_QueryInterface(mProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIWritablePropertyBag> writeProperties =
                                     do_QueryInterface(mProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the device manufacturer and model number properties.
  rv = writeProperties->SetProperty
                           (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                            sbIPDVariant("Apple").get());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = writeProperties->SetProperty(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                                    sbIPDVariant("iPod").get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the Firewire GUID property.
  nsAutoString firewireGUID;
  NS_ENSURE_SUCCESS(rv, rv);
  rv = properties->GetPropertyAsAString(NS_LITERAL_STRING("FirewireGUID"),
                                        firewireGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device volume reference number.
  nsCOMPtr<nsIVariant> propVariant;
  rv = properties->Get(NS_LITERAL_STRING("VolumeRefNum"),
                       getter_AddRefs(propVariant));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = sbIPDVariant::GetValue(propVariant, &mVolumeRefNum);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the device serial number property.
  //XXXeps use Firewire GUID for now.
  rv = writeProperties->SetProperty
                           (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SERIAL_NUMBER),
                            sbIPDVariant(firewireGUID).get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the iPod device object.
  rv = sbIPDDevice::Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Finalize the iPod system dependent device object.
 */

void
sbIPDSysDevice::Finalize()
{
  // Finalize the iPod device object.
  sbIPDDevice::Finalize();
}


