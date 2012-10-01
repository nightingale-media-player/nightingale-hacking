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

// System imports.
#include <ctype.h> // for isxdigit


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
  nsresult rv;

  // call the parent
  rv = sbIPDDevice::Eject();
  NS_ENSURE_SUCCESS(rv, rv);

  // Unmount and eject.
  rv = mSBLibHalCtx->DeviceVolumeUnmount(mMediaPartUDI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mSBLibHalCtx->DeviceVolumeEject(mMediaPartUDI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// iPod system dependent device services
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
  mProperties(aProperties),
  mSBLibHalCtx(nsnull)
{
  SB_PRLOG_SETUP(sbIPDSysDevice);

  // Log progress.
  LOG("Enter: sbIPDSysDevice::sbIPDSysDevice\n");

  // Validate parameters.
  NS_ASSERTION(aProperties, "aProperties is null");
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

  // Initialize the HAL library context.
  mSBLibHalCtx = new sbLibHalCtx();
  NS_ENSURE_TRUE(mSBLibHalCtx, NS_ERROR_OUT_OF_MEMORY);
  rv = mSBLibHalCtx->Initialize();
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device properties.
  nsCOMPtr<nsIPropertyBag2> properties = do_QueryInterface(mProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIWritablePropertyBag> writeProperties =
                                     do_QueryInterface(mProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device and media partition UDIs.
  rv = properties->GetPropertyAsACString(NS_LITERAL_STRING("DeviceUDI"),
                                         mDeviceUDI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = properties->GetPropertyAsACString(NS_LITERAL_STRING("MediaPartitionUDI"),
                                         mMediaPartUDI);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the device mount path property.
  nsAutoString mountPath;
  rv = GetMountPath(mountPath);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = writeProperties->SetProperty(NS_LITERAL_STRING("MountPath"),
                                    sbIPDVariant(mountPath).get());
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
  rv = GetFirewireGUID(firewireGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = writeProperties->SetProperty(NS_LITERAL_STRING("FirewireGUID"),
                                    sbIPDVariant(firewireGUID).get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the device serial number property.
  //XXXeps use Firewire GUID for now.
  rv = writeProperties->SetProperty
                           (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_SERIAL_NUMBER),
                            sbIPDVariant(firewireGUID).get());
  NS_ENSURE_SUCCESS(rv, rv);

    // Hey, if it's a read-only hfsplus filesystem then it's almost certainly
  // journalled
  nsCAutoString fstype;
  rv = mSBLibHalCtx->DeviceGetPropertyString(mMediaPartUDI,
                                             "volume.fstype",
                                             fstype);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool readonly;
  rv = mSBLibHalCtx->DeviceGetPropertyBool(mMediaPartUDI,
                                           "volume.is_mounted_read_only",
                                           &readonly);
  NS_ENSURE_SUCCESS(rv, rv);
  if (fstype.EqualsLiteral("hfsplus") && readonly) {
    rv = writeProperties->SetProperty(NS_LITERAL_STRING("HFSPlusReadOnly"),
                                      sbIPDVariant(PR_TRUE).get());
  }

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

  // Dispose of the HAL library context.
  if (mSBLibHalCtx)
    delete mSBLibHalCtx;
  mSBLibHalCtx = nsnull;
}


//------------------------------------------------------------------------------
//
// Internal iPod system dependent device services
//
//------------------------------------------------------------------------------

/**
 * Get the device mount path and return it in aMountPath.
 *
 * \param aMountPath            Device mount path.
 */

nsresult
sbIPDSysDevice::GetMountPath(nsAString& aMountPath)
{
  nsresult rv;

  // Get the media partition mount point.
  nsCAutoString mountPoint;
  rv = mSBLibHalCtx->DeviceGetPropertyString(mMediaPartUDI,
                                             "volume.mount_point",
                                             mountPoint);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  aMountPath.Assign(NS_ConvertUTF8toUTF16(mountPoint));

  return NS_OK;
}


/**
 * Get the device firewire GUID and return it in aFirewireGUID.
 *
 * \param aFirewireGUID         Device firewire GUID.
 */

nsresult
sbIPDSysDevice::GetFirewireGUID(nsAString& aFirewireGUID)
{
  nsresult rv;

  // Get the device storage serial number.
  nsCAutoString storageSerial;
  rv = mSBLibHalCtx->DeviceGetPropertyString(mDeviceUDI,
                                             "storage.serial",
                                             storageSerial);
  NS_ENSURE_SUCCESS(rv, rv);

  // Parse the Firewire GUID from the device storage serial number.
  nsCString firewireGUID;
  for (PRUint32 i = 0;
       (i < storageSerial.Length()) && (firewireGUID.Length() < 16);
       i++) {
    char guidChar = storageSerial.CharAt(i);
    if (isxdigit(guidChar))
      firewireGUID += guidChar;
    else
      firewireGUID.Truncate();
  }
  NS_ENSURE_TRUE(firewireGUID.Length() == 16, NS_ERROR_FAILURE);

  // Return results.
  aFirewireGUID.Assign(NS_ConvertUTF8toUTF16(firewireGUID));

  return NS_OK;
}


