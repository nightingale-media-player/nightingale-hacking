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

#include "sbDefaultBaseDeviceInfoRegistrar.h"

// Mozilla includes
#include <nsArrayUtils.h>
#include <nsISupportsPrimitives.h>
#include <nsIVariant.h>
#include <nsIWritablePropertyBag2.h>
#include <nsMemory.h>
#include <prlog.h>

// Songbird includes
#include <sbDeviceXMLCapabilities.h>
#include <sbIDevice.h>
#include <sbIDeviceCapabilities.h>
#include <sbIDeviceEvent.h>
#include <sbIDeviceEventTarget.h>
#include <sbIDeviceManager.h>
#include <sbIMediaItem.h>
#include <sbITranscodeManager.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbDefaultBaseDeviceInfoRegistrar:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gDefaultBaseDeviceInfoRegistrarLog = nsnull;
#define TRACE(args) PR_LOG(gDefaultBaseDeviceInfoRegistrarLog , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gDefaultBaseDeviceInfoRegistrarLog , PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

sbDefaultBaseDeviceInfoRegistrar::sbDefaultBaseDeviceInfoRegistrar() :
  mDevice(nsnull),
  mDeviceXMLInfoPresent(PR_FALSE)
{
  #ifdef PR_LOGGING
    if (!gDefaultBaseDeviceInfoRegistrarLog)
      gDefaultBaseDeviceInfoRegistrarLog =
        PR_NewLogModule("sbDefaultBaseDeviceInfoRegistrar");
  #endif
}

sbDefaultBaseDeviceInfoRegistrar::~sbDefaultBaseDeviceInfoRegistrar()
{
}

/* readonly attribute PRUint32 type; */
NS_IMETHODIMP
sbDefaultBaseDeviceInfoRegistrar::GetType(PRUint32 *aType)
{
  TRACE(("%s: default", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aType);

  *aType = sbIDeviceInfoRegistrar::DEFAULT;
  return NS_OK;
}

NS_IMETHODIMP
sbDefaultBaseDeviceInfoRegistrar::
  AddCapabilities(sbIDevice *aDevice,
                  sbIDeviceCapabilities *aCapabilities) {
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aCapabilities);

  nsresult rv;

  // Look for capabilities settings in the device preferences
  nsCOMPtr<nsIVariant> capabilitiesVariant;
  rv = aDevice->GetPreference(NS_LITERAL_STRING("capabilities"),
                              getter_AddRefs(capabilitiesVariant));
  if (NS_SUCCEEDED(rv)) {
    PRUint16 dataType;
    rv = capabilitiesVariant->GetDataType(&dataType);
    NS_ENSURE_SUCCESS(rv, rv);

    if ((dataType == nsIDataType::VTYPE_INTERFACE) ||
        (dataType == nsIDataType::VTYPE_INTERFACE_IS)) {
      nsCOMPtr<nsISupports>           capabilitiesISupports;
      nsCOMPtr<sbIDeviceCapabilities> capabilities;
      rv = capabilitiesVariant->GetAsISupports
                                  (getter_AddRefs(capabilitiesISupports));
      NS_ENSURE_SUCCESS(rv, rv);
      capabilities = do_QueryInterface(capabilitiesISupports, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aCapabilities->AddCapabilities(capabilities);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
  }

  // Add capabilities from the default device info document.
  sbDeviceXMLInfo* deviceXMLInfo;
  rv = GetDeviceXMLInfo(aDevice, &deviceXMLInfo);
  NS_ENSURE_SUCCESS(rv, rv);
  if (deviceXMLInfo) {
    // Get the device info element.
    nsCOMPtr<nsIDOMElement> deviceInfoElement;
    rv = deviceXMLInfo->GetDeviceInfoElement(getter_AddRefs(deviceInfoElement));
    NS_ENSURE_SUCCESS(rv, rv);

    // Add capabilities from the device info element.
    if (deviceInfoElement) {
      PRBool addedCapabilities;
      rv = sbDeviceXMLCapabilities::AddCapabilities(aCapabilities,
                                                    deviceInfoElement,
                                                    &addedCapabilities,
                                                    aDevice);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDefaultBaseDeviceInfoRegistrar::GetDeviceFolder(sbIDevice* aDevice,
                                                  PRUint32   aContentType,
                                                  nsAString& retval)
{
  TRACE(("%s", __FUNCTION__));

  nsresult rv;

  // Default to no folder.
  retval.Truncate();

  // Get the device XML info.  Just return if none available.
  sbDeviceXMLInfo* deviceXMLInfo;
  rv = GetDeviceXMLInfo(aDevice, &deviceXMLInfo);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!deviceXMLInfo)
    return NS_OK;

  // Get the device folder.
  rv = deviceXMLInfo->GetDeviceFolder(aContentType, retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDefaultBaseDeviceInfoRegistrar::GetMountTimeout(sbIDevice* aDevice,
                                                  PRUint32*  retval)
{
  TRACE(("%s", __FUNCTION__));

  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(retval);

  nsresult rv;

  // Get the device XML info and check if it's available.
  sbDeviceXMLInfo* deviceXMLInfo;
  rv = GetDeviceXMLInfo(aDevice, &deviceXMLInfo);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!deviceXMLInfo)
    return NS_ERROR_NOT_AVAILABLE;

  // Get the mount timeout value.
  rv = deviceXMLInfo->GetMountTimeout(retval);
  if (rv == NS_ERROR_NOT_AVAILABLE)
    return NS_ERROR_NOT_AVAILABLE;
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDefaultBaseDeviceInfoRegistrar::GetExcludedFolders(sbIDevice * aDevice,
                                                     nsAString & aFolders)
{
  TRACE(("%s", __FUNCTION__));

  nsresult rv;

  aFolders.Truncate();
  // Get the device XML info and check if it's available.
  sbDeviceXMLInfo* deviceXMLInfo;
  rv = GetDeviceXMLInfo(aDevice, &deviceXMLInfo);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!deviceXMLInfo)
    return NS_OK;

  rv = deviceXMLInfo->GetExcludedFolders(aFolders);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbDefaultBaseDeviceInfoRegistrar::InterestedInDevice(sbIDevice *aDevice,
                                                     PRBool *retval)
{
  NS_ENSURE_ARG_POINTER(retval);
  *retval = PR_FALSE;
  return NS_OK;
}

nsresult
sbDefaultBaseDeviceInfoRegistrar::GetDeviceXMLInfoSpec
                                    (nsACString& aDeviceXMLInfoSpec)
{
  aDeviceXMLInfoSpec.Truncate();
  return NS_OK;
}

nsresult
sbDefaultBaseDeviceInfoRegistrar::GetDeviceXMLInfo
                                    (sbIDevice*        aDevice,
                                     sbDeviceXMLInfo** aDeviceXMLInfo)
{
  NS_ENSURE_ARG_POINTER(aDevice);
  NS_ENSURE_ARG_POINTER(aDeviceXMLInfo);

  nsresult rv;

  // Default to no device XML info.
  *aDeviceXMLInfo = nsnull;

  // Check if device info was already read.
  if (mDeviceXMLInfo && (aDevice == mDevice)) {
    if (mDeviceXMLInfoPresent)
      *aDeviceXMLInfo = mDeviceXMLInfo;
    return NS_OK;
  }

  // Remember the device.
  mDevice = aDevice;

  // Get the device XML info document spec.
  nsCAutoString deviceXMLInfoSpec;
  rv = GetDeviceXMLInfoSpec(deviceXMLInfoSpec);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the device XML info document.
  if (!deviceXMLInfoSpec.IsEmpty()) {
    rv = GetDeviceXMLInfo(deviceXMLInfoSpec, aDevice);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If no device XML info was present, read from the default device XML info
  // document.
  if (!mDeviceXMLInfoPresent) {
    deviceXMLInfoSpec = NS_LITERAL_CSTRING
      ("chrome://songbird/content/devices/sbDefaultDeviceInfo.xml");
    rv = GetDeviceXMLInfo(deviceXMLInfoSpec, aDevice);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return device XML info if it's present.
  if (mDeviceXMLInfoPresent)
    *aDeviceXMLInfo = mDeviceXMLInfo;

  return NS_OK;
}

nsresult
sbDefaultBaseDeviceInfoRegistrar::GetDeviceXMLInfo
                                    (const nsACString& aDeviceXMLInfoSpec,
                                     sbIDevice*        aDevice)
{
  NS_ENSURE_ARG_POINTER(aDevice);

  nsresult rv;

  // Read the device XML info.
  mDeviceXMLInfo = new sbDeviceXMLInfo(aDevice);
  NS_ENSURE_TRUE(mDeviceXMLInfo, NS_ERROR_OUT_OF_MEMORY);
  rv = mDeviceXMLInfo->Read(aDeviceXMLInfoSpec.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if device XML info is present.
  rv = mDeviceXMLInfo->GetDeviceInfoPresent(&mDeviceXMLInfoPresent);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

