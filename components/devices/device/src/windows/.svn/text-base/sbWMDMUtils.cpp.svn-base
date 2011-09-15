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
//
// Songbird WMDM utilities services.
//
//------------------------------------------------------------------------------

/**
 * \file  sbWMDMUtils.cpp
 * \brief Songbird WMDM Utilities Source.
 */

//------------------------------------------------------------------------------
//
// Songbird WMDM utilities imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbWMDMUtils.h"

// Local imports.
#include "sbWindowsDeviceUtils.h"

// Songbird imports.
#include <WMDRMCertificate.h>

// Mozilla imports.
#include <nsAutoPtr.h>

// Windows imports.
#include <SCClient.h>


//------------------------------------------------------------------------------
//
// Songbird WMDM utilities services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbWMDMGetDeviceFromDeviceInstanceID
//

nsresult
sbWMDMGetDeviceFromDeviceInstanceID(nsAString&    aDeviceInstanceID,
                                    IWMDMDevice** aDevice)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevice);

  // Function variables.
  HRESULT  hr;
  nsresult rv;

  // Get the WMDM manager.
  nsRefPtr<IWMDeviceManager>  deviceManager;
  nsRefPtr<IWMDeviceManager2> deviceManager2;
  rv = sbWMDMGetDeviceManager(getter_AddRefs(deviceManager));
  NS_ENSURE_SUCCESS(rv, rv);
  hr = deviceManager->QueryInterface(__uuidof(IWMDeviceManager2),
                                     getter_AddRefs(deviceManager2));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Get an enumerator for all WMDM devices.
  nsRefPtr<IWMDMEnumDevice> deviceEnum;
  hr = deviceManager2->EnumDevices2(getter_AddRefs(deviceEnum));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Search for a WMDM device that matches the device instance ID.
  nsRefPtr<IWMDMDevice> matchingDevice;
  while (1) {
    // Get the next device.
    nsRefPtr<IWMDMDevice> device;
    ULONG                 deviceCount;
    hr = deviceEnum->Next(1, getter_AddRefs(device), &deviceCount);
    NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);
    if (!deviceCount)
      break;

    // Get the device canonical name.
    WCHAR canonicalName[MAX_PATH] = {0};
    nsRefPtr<IWMDMDevice2> device2;
    hr = device->QueryInterface(__uuidof(IWMDMDevice2),
                                getter_AddRefs(device2));
    if (!SUCCEEDED(hr))
      continue;
    hr = device2->GetCanonicalName(canonicalName, ARRAYSIZE(canonicalName));
    if (!SUCCEEDED(hr))
      continue;

    // Get the device interface name from the canonical name.  Cut off the
    // "$<index>" string from the end.
    // See http://msdn.microsoft.com/en-us/library/ff801405%28v=VS.85%29.aspx
    nsAutoString deviceInterfaceName(canonicalName);
    PRInt32 index = deviceInterfaceName.RFindChar('$');
    if (index >= 0)
      deviceInterfaceName.SetLength(index);

    // Get the device instance ID from the device interface name.
    nsAutoString deviceInstanceID;
    rv = sbWinGetDeviceInstanceIDFromDeviceInterfaceName(deviceInterfaceName,
                                                         deviceInstanceID);
    if (NS_FAILED(rv))
      continue;

    // Check for a match.
    if (deviceInstanceID.Equals(aDeviceInstanceID)) {
      matchingDevice = device;
      break;
    }
  }
  if (!matchingDevice)
    return NS_ERROR_NOT_AVAILABLE;

  // Return results.
  matchingDevice.forget(aDevice);

  return NS_OK;
}


//-------------------------------------
//
// sbWMDMGetDeviceManager
//

nsresult
sbWMDMGetDeviceManager(IWMDeviceManager** aDeviceManager)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceManager);

  // Function variables.
  HRESULT hr;

  // Get the WMDM device manager authentication interface.
  nsRefPtr<IComponentAuthenticate> wmdmDeviceManagerAuth;
  hr = ::CoCreateInstance(__uuidof(::MediaDevMgr),
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          __uuidof(IComponentAuthenticate),
                          getter_AddRefs(wmdmDeviceManagerAuth));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Create a secure channel with the WMDRM certificate.
  CSecureChannelClient* secureChannelClient = new ::CSecureChannelClient;
  NS_ENSURE_TRUE(secureChannelClient, NS_ERROR_OUT_OF_MEMORY);
  hr = secureChannelClient->SetCertificate(SAC_CERT_V1,
                                           wmDRMCertificate,
                                           sizeof(wmDRMCertificate),
                                           wmDRMPrivateKey,
                                           sizeof(wmDRMPrivateKey));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Authenticate with the WMDM device manager.
  secureChannelClient->SetInterface(wmdmDeviceManagerAuth);
  hr = secureChannelClient->Authenticate(SAC_PROTOCOL_V1);
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Get the WMDM device manager interface.
  nsRefPtr<IWMDeviceManager> deviceManager;
  hr = wmdmDeviceManagerAuth->QueryInterface(__uuidof(IWMDeviceManager),
                                             getter_AddRefs(deviceManager));
  NS_ENSURE_TRUE(SUCCEEDED(hr), NS_ERROR_FAILURE);

  // Return results.
  deviceManager.forget(aDeviceManager);

  return NS_OK;
}

