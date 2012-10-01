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
// Songbird Windows device utilities services.
//
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsDeviceUtils.cpp
 * \brief Songbird Windows Device Utilities Source.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows device utilities imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbWindowsDeviceUtils.h"

// Songbird imports.
#include <sbWindowsUtils.h>

// Windows imports.
#include <dbt.h>
#include <winioctl.h>


//------------------------------------------------------------------------------
//
// Songbird Windows device utilities defs.
//
//------------------------------------------------------------------------------

//
// Win32 device GUID defs.
//
//   These are defined here to avoid DDK library dependencies.
//

static const GUID GUID_DEVINTERFACE_USB_HUB =
{
  0xF18A0E88,
  0xC30C,
  0x11D0,
  { 0x88, 0x15, 0x00, 0xA0, 0xC9, 0x06, 0xBE, 0xD8 }
};


//------------------------------------------------------------------------------
//
// Songbird Windows device file services.
//
//------------------------------------------------------------------------------

/**
 * Create a device file for the device instance specified by aDevInst and device
 * interface class specified by aGUID.  Return the created device file handle in
 * aDevFile
 *
 * \param aDevFile              Returned created device file handle.
 * \param aDevInst              Device instance for which to create file.
 * \param aGUID                 Device interface class for which to create file.
 * \param aDesiredAccess        File creation arguments.
 * \param aShareMode
 * \param aSecurityAttributes
 * \param aCreationDisposition
 * \param aFlagsAndAttributes
 * \param aTemplateFile
 *
 * \return NS_ERROR_NOT_AVAILABLE if the device interface is not available for
 *         the device.
 */

nsresult
sbWinCreateDeviceFile(HANDLE*               aDevFile,
                      DEVINST               aDevInst,
                      const GUID*           aGUID,
                      DWORD                 aDesiredAccess,
                      DWORD                 aShareMode,
                      LPSECURITY_ATTRIBUTES aSecurityAttributes,
                      DWORD                 aCreationDisposition,
                      DWORD                 aFlagsAndAttributes,
                      HANDLE                aTemplateFile)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevFile);
  NS_ENSURE_ARG_POINTER(aGUID);

  // Function variables.
  nsresult rv;

  // Get the device path.
  nsAutoString devicePath;
  rv = sbWinGetDevicePath(aDevInst, aGUID, devicePath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the device file.
  HANDLE devFile = CreateFileW(devicePath.get(),
                               aDesiredAccess,
                               aShareMode,
                               aSecurityAttributes,
                               aCreationDisposition,
                               aFlagsAndAttributes,
                               aTemplateFile);
  NS_ENSURE_TRUE(devFile != INVALID_HANDLE_VALUE, NS_ERROR_FAILURE);

  // Return results.
  *aDevFile = devFile;

  return NS_OK;
}


/**
 * Create a device file for an ancestor of the device instance specified by
 * aDevInst and device interface class specified by aGUID.  Create the device
 * file for the first ancestor implementing the specified device interface
 * class.  Return the created device file handle in aDevFile
 *
 * \param aDevFile              Returned created device file handle.
 * \param aDevInst              Device instance for which to create file.
 * \param aGUID                 Device interface class for which to create file.
 * \param aDesiredAccess        File creation arguments.
 * \param aShareMode
 * \param aSecurityAttributes
 * \param aCreationDisposition
 * \param aFlagsAndAttributes
 * \param aTemplateFile
 *
 * \return NS_ERROR_NOT_AVAILABLE if the device interface is not available for
 *         any of the device ancestors.
 */

nsresult
sbWinCreateAncestorDeviceFile(HANDLE*               aDevFile,
                              DEVINST               aDevInst,
                              const GUID*           aGUID,
                              DWORD                 aDesiredAccess,
                              DWORD                 aShareMode,
                              LPSECURITY_ATTRIBUTES aSecurityAttributes,
                              DWORD                 aCreationDisposition,
                              DWORD                 aFlagsAndAttributes,
                              HANDLE                aTemplateFile)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevFile);

  // Function variables.
  CONFIGRET cfgRet;
  nsresult  rv;

  // Search for an ancestor device that has the specified interface.
  DEVINST ancestorDevInst;
  DEVINST devInst = aDevInst;
  while (1) {
    // Get the next ancestor.
    cfgRet = CM_Get_Parent(&ancestorDevInst, devInst, 0);
    if (cfgRet != CR_SUCCESS)
      return NS_ERROR_NOT_AVAILABLE;

    // Check if the ancestor has the interface.
    bool hasInterface;
    rv = sbWinDeviceHasInterface(ancestorDevInst, aGUID, &hasInterface);
    NS_ENSURE_SUCCESS(rv, rv);
    if (hasInterface)
      break;

    // Check the next ancestor.
    devInst = ancestorDevInst;
  }

  /* Create a device file for the ancestor. */
  return sbWinCreateDeviceFile(aDevFile,
                               ancestorDevInst,
                               aGUID,
                               aDesiredAccess,
                               aShareMode,
                               aSecurityAttributes,
                               aCreationDisposition,
                               aFlagsAndAttributes,
                               aTemplateFile);
}


/**
 * Check if the device instance specified by aDevInst provides the device
 * interface specified by aGUID.  If it does, return true in aHasInterface.
 *
 * \param aDevInst              Device instance to check.
 * \param aGUID                 Device interface class.
 * \param aHasInterface         Returned true if device instance provides the
 *                              specified interface.
 */

nsresult
sbWinDeviceHasInterface(DEVINST     aDevInst,
                        const GUID* aGUID,
                        bool*     aHasInterface)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aGUID);
  NS_ENSURE_ARG_POINTER(aHasInterface);

  // Function variables.
  nsresult rv;

  // Set default result.
  *aHasInterface = PR_FALSE;

  // Get the device info set and set it up for auto-disposal.
  nsAutoString deviceInstanceID;
  rv = sbWinGetDeviceInstanceID(aDevInst, deviceInstanceID);
  NS_ENSURE_SUCCESS(rv, rv);
  HDEVINFO devInfo = SetupDiGetClassDevsW(aGUID,
                                          deviceInstanceID.get(),
                                          NULL,
                                          DIGCF_DEVICEINTERFACE);
  NS_ENSURE_TRUE(devInfo != INVALID_HANDLE_VALUE, NS_ERROR_FAILURE);
  sbAutoHDEVINFO autoDevInfo(devInfo);

  // Get the device info for the device instance.  Device does not have the
  // interface if it does not have a device info data record.
  SP_DEVINFO_DATA devInfoData;
  rv = sbWinGetDevInfoData(aDevInst, devInfo, &devInfoData);
  if (NS_FAILED(rv))
    return NS_OK;

  // Get the device interface detail.  Device does not have the interface if it
  // does not have the interface detail.
  PSP_DEVICE_INTERFACE_DETAIL_DATA devIfDetailData;
  rv = sbWinGetDevInterfaceDetail(&devIfDetailData,
                                  devInfo,
                                  &devInfoData,
                                  aGUID);
  if (NS_FAILED(rv))
    return NS_OK;
  sbAutoNSMemPtr autoDevIfDetailData(devIfDetailData);

  // Device has the interface.
  *aHasInterface = PR_TRUE;

  return NS_OK;
}


/**
 * Return in aDevicePath the device file path for the device specified by
 * aDevInst and device interface class specified by aGUID.
 *
 * \param aDevInst              Device instance for which to get path.
 * \param aGUID                 Device interface class.
 * \param aDevicePath           Returned device path.
 */

nsresult
sbWinGetDevicePath(DEVINST     aDevInst,
                   const GUID* aGUID,
                   nsAString&  aDevicePath)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aGUID);

  // Function variables.
  nsresult rv;

  // Get the device info set and set it up for auto-disposal.
  nsAutoString deviceInstanceID;
  rv = sbWinGetDeviceInstanceID(aDevInst, deviceInstanceID);
  NS_ENSURE_SUCCESS(rv, rv);
  HDEVINFO devInfo = SetupDiGetClassDevsW(aGUID,
                                          deviceInstanceID.get(),
                                          NULL,
                                          DIGCF_DEVICEINTERFACE);
  NS_ENSURE_TRUE(devInfo != INVALID_HANDLE_VALUE, NS_ERROR_FAILURE);
  sbAutoHDEVINFO autoDevInfo(devInfo);

  // Get the device info for the device instance.
  SP_DEVINFO_DATA devInfoData;
  rv = sbWinGetDevInfoData(aDevInst, devInfo, &devInfoData);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device interface detail data for the device instance.
  PSP_DEVICE_INTERFACE_DETAIL_DATA devIfDetailData;
  rv = sbWinGetDevInterfaceDetail(&devIfDetailData,
                                  devInfo,
                                  &devInfoData,
                                  aGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoNSMemPtr autoDevIfDetailData(devIfDetailData);

  // Return results.
  aDevicePath.Assign(devIfDetailData->DevicePath);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird Windows device utilities services.
//
//------------------------------------------------------------------------------

/**
 *   Search the device specified by aRootDevInst and all of its descendents or
 * ancestors for device instances providing the device interface specified by
 * aGUID.  Return all found device instances in aDevInstList.  If
 * aSearchAncestors is true, search all ancestors; otherwise, search all
 * descendents.
 *   If searching ancestors, the returned list will be ordered by most immediate
 * ancestors first.
 *
 * \param aDevInstList          Returned list of found device instances.
 * \param aRootDevInst          Root device instance from which to begin search.
 * \param aGUID                 Device interface class for which to search.
 * \param aSearchAncestors      If true, search ancestors.
 */

static nsresult _sbWinFindDevicesByInterface
                  (nsTArray<DEVINST>& aDevInstList,
                   DEVINST            aRootDevInst,
                   const GUID*        aGUID,
                   bool             aSearchAncestors);

nsresult
sbWinFindDevicesByInterface(nsTArray<DEVINST>& aDevInstList,
                            DEVINST            aRootDevInst,
                            const GUID*        aGUID,
                            bool             aSearchAncestors)
{
  // Clear device instance list.
  aDevInstList.Clear();

  return _sbWinFindDevicesByInterface(aDevInstList,
                                      aRootDevInst,
                                      aGUID,
                                      aSearchAncestors);
}

nsresult
_sbWinFindDevicesByInterface(nsTArray<DEVINST>& aDevInstList,
                             DEVINST            aRootDevInst,
                             const GUID*        aGUID,
                             bool             aSearchAncestors)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aGUID);

  // Function variables.
  CONFIGRET cr;
  nsresult  rv;

  // Check if the root device has the specified interface.
  bool hasInterface;
  rv = sbWinDeviceHasInterface(aRootDevInst, aGUID, &hasInterface);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasInterface) {
    NS_ENSURE_TRUE(aDevInstList.AppendElement(aRootDevInst),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  // Search all descendents or ancestors of device.
  if (aSearchAncestors) {
    DEVINST parentDevInst;
    cr = CM_Get_Parent(&parentDevInst, aRootDevInst, 0);
    if (cr == CR_SUCCESS) {
      // Search parent.
      rv = _sbWinFindDevicesByInterface(aDevInstList,
                                        parentDevInst,
                                        aGUID,
                                        aSearchAncestors);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    DEVINST childDevInst;
    cr = CM_Get_Child(&childDevInst, aRootDevInst, 0);
    while (cr == CR_SUCCESS) {
      // Search child.
      rv = _sbWinFindDevicesByInterface(aDevInstList,
                                        childDevInst,
                                        aGUID,
                                        aSearchAncestors);
      NS_ENSURE_SUCCESS(rv, rv);

      // Get the next child.  An error indicates that no more children are
      // present.
      cr = CM_Get_Sibling(&childDevInst, childDevInst, 0);
    }
  }

  return NS_OK;
}


/**
 * Search the device specified by aRootDevInst and all of its descendents for a
 * device instance of the class specified by aClass.  If one is found, this
 * function returns the found device instance in aDevInst and true in aFound.
 *
 * \param aDevInst              Returned found device instance.
 * \param aFound                Returned true if device was found.
 * \param aRootDevInst          Root device instance.
 * \param aClass                Device class for which to search.
 */

nsresult
sbWinFindDeviceByClass(DEVINST*         aDevInst,
                       bool*          aFound,
                       DEVINST          aRootDevInst,
                       const nsAString& aClass)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevInst);
  NS_ENSURE_ARG_POINTER(aFound);

  // Function variables.
  bool    found = PR_FALSE;
  CONFIGRET cr;
  nsresult  rv;

  // Get the root device class.
  WCHAR propBuffer[256];
  ULONG length = sizeof(propBuffer);
  cr = CM_Get_DevNode_Registry_PropertyW(aRootDevInst,
                                         CM_DRP_CLASS,
                                         NULL,
                                         propBuffer,
                                         &length,
                                         0);
  NS_ENSURE_TRUE(cr == CR_SUCCESS, NS_ERROR_FAILURE);

  // Check if the root device class matches.  If it does, return.
  nsDependentString property(propBuffer);
  if (property.Equals(aClass)) {
    *aDevInst = aRootDevInst;
    *aFound = PR_TRUE;
    return NS_OK;
  }

  // Search all descendents of device.
  DEVINST childDevInst;
  cr = CM_Get_Child(&childDevInst, aRootDevInst, 0);
  while (cr == CR_SUCCESS) {
    // Search child.
    rv = sbWinFindDeviceByClass(aDevInst, &found, childDevInst, aClass);
    NS_ENSURE_SUCCESS(rv, rv);
    if (found) {
      found = PR_TRUE;
      break;
    }

    // Get the next child.  An error indicates that no more children are
    // present.
    cr = CM_Get_Sibling(&childDevInst, childDevInst, 0);
  }

  // Return results.
  *aFound = found;

  return NS_OK;
}


/**
 * Return in aDevInfoData the device info for the device instance specified by
 * aDevInst using the device info set specified by aDevInfo.
 *
 * \param aDevInst              Device instance for which to get device info.
 * \param aDevInfo              Device info set handle.
 * \param aDevInfoData          Returned device info.
 *
 * \return NS_ERROR_NOT_AVAILABLE if the device instance does not have device
 *         info.
 */

nsresult
sbWinGetDevInfoData(DEVINST          aDevInst,
                    HDEVINFO         aDevInfo,
                    PSP_DEVINFO_DATA aDevInfoData)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevInfoData);

  // Function variables.
  BOOL success;

  // Find the device info data for the device instance.
  DWORD devIndex = 0;
  while (1) {
    // Get the next device info data.
    aDevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
    success = SetupDiEnumDeviceInfo(aDevInfo, devIndex, aDevInfoData);
    if (!success)
      return NS_ERROR_NOT_AVAILABLE;

    // The search is done if the device info data is for the device instance.
    if (aDevInfoData->DevInst == aDevInst)
      break;

    // Check the next device info data.
    devIndex++;
  }

  return NS_OK;
}


/**
 * Return in aDevIfDetailData and aDevInfoData the device details for a device.
 * The device is specified by the enumeration handle aDevInfo.  The device class
 * GUID is specified by aGUID, and the device enumeration index is specified by
 * aDevIndex.
 *
 * \param aDevIfDetailData      Returned device detail data.
 * \param aDevInfoData          Returned device info data.
 * \param aDevInfo              Device info enumeration handle.
 * \param aGUID                 Device class GUID.
 * \param aDevIndex             Device enumeration index.
 */

nsresult
sbWinGetDevDetail(PSP_DEVICE_INTERFACE_DETAIL_DATA* aDevIfDetailData,
                  SP_DEVINFO_DATA*                  aDevInfoData,
                  HDEVINFO                          aDevInfo,
                  const GUID*                       aGUID,
                  DWORD                             aDevIndex)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevIfDetailData);
  NS_ENSURE_ARG_POINTER(aDevInfoData);
  NS_ENSURE_ARG_POINTER(aGUID);

  // Function variables.
  BOOL success;

  // Set up to get the device interface detail data.  If not successful, there's
  // no more device interfaces to enumerate.
  SP_DEVICE_INTERFACE_DATA devIfData;
  ZeroMemory(&devIfData, sizeof(SP_DEVICE_INTERFACE_DATA));
  devIfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
  success = SetupDiEnumDeviceInterfaces(aDevInfo,
                                        NULL,
                                        aGUID,
                                        aDevIndex,
                                        &devIfData);
  if (!success)
    return NS_ERROR_NOT_AVAILABLE;

  // Get the required size of the device interface detail data.
  DWORD size = 0;
  success = SetupDiGetDeviceInterfaceDetailW(aDevInfo,
                                             &devIfData,
                                             NULL,
                                             0,
                                             &size,
                                             NULL);
  NS_ENSURE_TRUE(size > 0, NS_ERROR_FAILURE);

  // Allocate the device interface detail data record and set it up for
  // auto-disposal.
  PSP_DEVICE_INTERFACE_DETAIL_DATA
    devIfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(size);
  NS_ENSURE_TRUE(devIfDetailData, NS_ERROR_OUT_OF_MEMORY);
  sbAutoMemPtr<SP_DEVICE_INTERFACE_DETAIL_DATA>
    autoDevIfDetailData(devIfDetailData);
  devIfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

  // Get the device interface details.
  ZeroMemory(aDevInfoData, sizeof(SP_DEVINFO_DATA));
  aDevInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
  success = SetupDiGetDeviceInterfaceDetailW(aDevInfo,
                                             &devIfData,
                                             devIfDetailData,
                                             size,
                                             NULL,
                                             aDevInfoData);
  NS_ENSURE_SUCCESS(success, NS_ERROR_FAILURE);

  // Return results.
  *aDevIfDetailData =
    (PSP_DEVICE_INTERFACE_DETAIL_DATA) autoDevIfDetailData.forget();

  return NS_OK;
}


/**
 *   Return in aDevIfDetailData the device interface details for the device
 * specified by aDevInfoData and the device interface class specified by aGUID
 * using the device info set specified by aDevInfo.
 *   If the specified device does not provide the specified interface, return
 * NS_ERROR_NOT_AVAILABLE.
 *   The caller is responsible for calling NS_Free for the returned device
 * interface details data record.
 *
 * \param aDevIfDetailData      Returned device detail data.
 * \param aDevInfo              Device info set handle.
 * \param aDevInfoData          Device info data for which to get device
 *                              interface details.
 * \param aGUID                 Device interface class GUID.
 *
 * \return NS_ERROR_NOT_AVAILABLE if the device interface is not available for
 *         the device.
 */

nsresult
sbWinGetDevInterfaceDetail(PSP_DEVICE_INTERFACE_DETAIL_DATA* aDevIfDetailData,
                           HDEVINFO                          aDevInfo,
                           SP_DEVINFO_DATA*                  aDevInfoData,
                           const GUID*                       aGUID)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevIfDetailData);
  NS_ENSURE_ARG_POINTER(aDevInfoData);
  NS_ENSURE_ARG_POINTER(aGUID);

  // Function variables.
  BOOL success;

  // Get the device interface data for the requested device and interface GUID.
  SP_DEVICE_INTERFACE_DATA devIfData;
  ZeroMemory(&devIfData, sizeof(SP_DEVICE_INTERFACE_DATA));
  devIfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
  success = SetupDiEnumDeviceInterfaces(aDevInfo,
                                        aDevInfoData,
                                        aGUID,
                                        0,
                                        &devIfData);
  if (!success)
    return NS_ERROR_NOT_AVAILABLE;

  // Get the required size of the device interface detail data.
  DWORD size = 0;
  success = SetupDiGetDeviceInterfaceDetailW(aDevInfo,
                                             &devIfData,
                                             NULL,
                                             0,
                                             &size,
                                             NULL);
  if (!success) {
    NS_ENSURE_TRUE(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                   NS_ERROR_FAILURE);
  }
  NS_ENSURE_TRUE(size > 0, NS_ERROR_FAILURE);

  // Allocate the device interface detail data record and set it up for
  // auto-disposal.
  PSP_DEVICE_INTERFACE_DETAIL_DATA devIfDetailData;
  devIfDetailData =
    static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(NS_Alloc(size));
  NS_ENSURE_TRUE(devIfDetailData, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoDevIfDetailData(devIfDetailData);

  // Get the device interface details.
  devIfDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
  success = SetupDiGetDeviceInterfaceDetailW(aDevInfo,
                                             &devIfData,
                                             devIfDetailData,
                                             size,
                                             NULL,
                                             NULL);
  NS_ENSURE_SUCCESS(success, NS_ERROR_FAILURE);

  // Return results.
  *aDevIfDetailData =
    static_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA>(autoDevIfDetailData.forget());

  return NS_OK;
}


/**
 * Return in aDeviceInstanceID the device instance ID for the device with the
 * device interface name specified by aDeviceInterfaceName.
 *
 * \param aDeviceInterfaceName  Device interface name.
 * \param aDeviceInstanceID     Returned device instance ID.
 */

nsresult
sbWinGetDeviceInstanceIDFromDeviceInterfaceName(nsAString& aDeviceInterfaceName,
                                                nsAString& aDeviceInstanceID)
{
  BOOL     success;
  nsresult rv;

  // Create a device info set and set it up for auto-disposal.
  HDEVINFO devInfoSet = SetupDiCreateDeviceInfoList(NULL, NULL);
  NS_ENSURE_TRUE(devInfoSet != INVALID_HANDLE_VALUE, NS_ERROR_FAILURE);
  sbAutoHDEVINFO autoDevInfoSet(devInfoSet);

  // Add the device interface data, including the device info data, to the
  // device info set.
  SP_DEVICE_INTERFACE_DATA devIfData;
  ZeroMemory(&devIfData, sizeof(SP_DEVICE_INTERFACE_DATA));
  devIfData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
  success = SetupDiOpenDeviceInterfaceW(devInfoSet,
                                        aDeviceInterfaceName.BeginReading(),
                                        0,
                                        &devIfData);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Get the device info data.
  SP_DEVINFO_DATA devInfoData;
  devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
  success = SetupDiEnumDeviceInfo(devInfoSet, 0, &devInfoData);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Get the device instance ID.
  rv = sbWinGetDeviceInstanceID(devInfoData.DevInst, aDeviceInstanceID);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aDeviceInstanceID the device instance ID for the device specified
 * by aDevInst.
 *
 * \param aDevInst              Device instance for which to get ID.
 * \param aDeviceInstanceID     Returned device instance ID.
 */

nsresult
sbWinGetDeviceInstanceID(DEVINST    aDevInst,
                         nsAString& aDeviceInstanceID)
{
  CONFIGRET cr;

  // Get the device instance ID.
  TCHAR deviceInstanceID[MAX_DEVICE_ID_LEN];
  cr = CM_Get_Device_ID(aDevInst, deviceInstanceID, MAX_DEVICE_ID_LEN, 0);
  NS_ENSURE_TRUE(cr == CR_SUCCESS, NS_ERROR_FAILURE);
  aDeviceInstanceID.Assign(deviceInstanceID);

  return NS_OK;
}


/**
 * Eject the device specified by aDevInst.
 *
 * \param aDevInst              Device instance to eject.
 */

nsresult
sbWinDeviceEject(DEVINST aDevInst)
{
  CONFIGRET cfgRet;

  // Try ejecting the device three times.
  WCHAR         vetoName[MAX_PATH];
  PNP_VETO_TYPE vetoType;
  bool        ejected = PR_FALSE;
  for (int i = 0; i < 3; i++) {
    // Try ejecting using CM_Request_Device_Eject.
    cfgRet = CM_Request_Device_EjectW(aDevInst,
                                      &vetoType,
                                      vetoName,
                                      MAX_PATH,
                                      0);
    if (cfgRet == CR_SUCCESS) {
      ejected = PR_TRUE;
      break;
    }
    // Wait for 1/10 second to give the device time to handle the eject.
    // This probably isn't needed, but all the examples I saw that used
    // the functions always put in a delay between calls at least for retries
    Sleep(100);
    // Try ejecting using CM_Query_And_Remove_SubTree.
    cfgRet = CM_Query_And_Remove_SubTreeW(aDevInst,
                                          &vetoType,
                                          vetoName,
                                          MAX_PATH,
                                          CM_REMOVE_NO_RESTART);
    if (cfgRet == CR_SUCCESS) {
      ejected = PR_TRUE;
      break;
    }
    // Wait 1/2 before retrying so we don't just slam the device with a bunch
    // of eject/remove requests and fail out.
    Sleep(500);
  }

  // Try one last time and let the PnP manager notify the user of failure.
  if (!ejected) {
    cfgRet = CM_Request_Device_Eject(aDevInst, NULL, NULL, 0, 0);
    NS_ENSURE_TRUE(cfgRet == CR_SUCCESS, NS_ERROR_FAILURE);
  }

  return NS_OK;
}

/**
 * Ejects the USB drive using the supplied drive (X:) via the DeviceIoControl
 * functions. I used MSDN KB165721 to create this code.
 */
nsresult
sbWinDeviceEject(nsAString const & aMountPath)
{

  DWORD    byteCount;
  BOOL     success;

  nsString volumePath(NS_LITERAL_STRING("\\\\.\\"));
  volumePath.Append(aMountPath);
  volumePath.Trim("\\", PR_FALSE, PR_TRUE);

  // Create a disk interface device file.
  sbAutoHANDLE diskHandle = CreateFileW(volumePath.BeginReading(),
                                        GENERIC_READ | GENERIC_WRITE,
                                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                                        NULL,
                                        OPEN_EXISTING,
                                        0,
                                        NULL);
  NS_ENSURE_TRUE(diskHandle.get() != INVALID_HANDLE_VALUE, NS_ERROR_FAILURE);

  const PRUint32 LOCK_RETRY = 3;
  const PRUint32 LOCK_RETRY_WAIT= 500; // Wait between retry in milliseconds

  for (PRUint32 retry = 0; retry < LOCK_RETRY; ++retry) {
    success = DeviceIoControl(diskHandle,
                              FSCTL_LOCK_VOLUME,
                              NULL, 0,
                              NULL, 0,
                              &byteCount,
                              NULL);
    if (success) {
      break;
    }
#ifdef DEBUG
    // If this was the last retry then report the error
    if (!success && retry == LOCK_RETRY - 1) {
      const DWORD error = GetLastError();
      printf("FSTL_LOCK_VOLUME failed on %s with code %u\n",
             NS_LossyConvertUTF16toASCII(volumePath).get(),
             error);
    }
#endif
    Sleep(LOCK_RETRY_WAIT);
  }
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  success = DeviceIoControl(diskHandle,
                            FSCTL_DISMOUNT_VOLUME,
                            NULL, 0,
                            NULL, 0,
                            &byteCount,
                            NULL);
  if (!success) {
#ifdef DEBUG
    const DWORD error = GetLastError();
    printf("FSCTL_DISMOUNT_VOLUME failed on %s with code %u\n",
           NS_LossyConvertUTF16toASCII(volumePath).get(),
           error);
#endif
  }
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  PREVENT_MEDIA_REMOVAL preventMediaRemoval;

  preventMediaRemoval.PreventMediaRemoval = FALSE;

  success = DeviceIoControl(diskHandle,
                            IOCTL_STORAGE_MEDIA_REMOVAL,
                            &preventMediaRemoval, sizeof(preventMediaRemoval),
                            NULL, 0,
                            &byteCount,
                            NULL);
  if (!success) {
#ifdef DEBUG
    const DWORD error = GetLastError();
    printf("IOCTL_STORAGE_MEDIA_REMOVAL failed on %s with code %u\n",
           NS_LossyConvertUTF16toASCII(volumePath).get(),
           error);
#endif
  }
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  success = DeviceIoControl(diskHandle,
                            IOCTL_STORAGE_EJECT_MEDIA,
                            NULL,
                            0,
                            NULL,
                            0,
                            &byteCount,
                            NULL);
  if (!success) {
#ifdef DEBUG
    const DWORD error = GetLastError();
    printf("IOCTL_STORAGE_EJECT_MEDIA failed on %s with code %u\n",
           NS_LossyConvertUTF16toASCII(volumePath).get(),
           error);
#endif
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

/**
 * Determine whether the device specified by aDescendantDevInst is a descendant
 * of the device specified by aDevInst.  If it is, return true in aIsDescendant.
 *
 * \param aDevInst              Ancestor device.
 * \param aDescendantDevInst    Descendant device.
 * \param aIsDesendant          Returned true if descendant device is a
 *                              descendant of ancestor device.
 */

nsresult
sbWinDeviceIsDescendantOf(DEVINST aDevInst,
                          DEVINST aDescendantDevInst,
                          bool* aIsDescendant)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aIsDescendant);

  // Function variables.
  bool    isDescendant = PR_FALSE;
  CONFIGRET cfgRet;

  // Search ancestors for target device.
  DEVINST currentAncestor = aDescendantDevInst;
  while (1) {
    // Get the next ancestor.
    cfgRet = CM_Get_Parent(&currentAncestor, currentAncestor, 0);
    if (cfgRet != CR_SUCCESS)
      break;

    // Check for a match.
    if (currentAncestor == aDevInst) {
      isDescendant = PR_TRUE;
      break;
    }
  }

  // Return results.
  *aIsDescendant = isDescendant;

  return NS_OK;
}


/**
 * Register device handle notification for the device specified by aDevInst with
 * the device interface specified by aGUID.  Send the notification to the window
 * specified by aEventWindow.  Return the device notification handle in
 * aDeviceNotification.
 *
 * \param aDeviceNotification   Returned device notification handle.
 * \param aEventWindow          Window to which to send notification.
 * \param aDevInst              Device for which to register notification.
 * \param aGUID                 Device interface for which to register
 *                              notification.
 */

nsresult
sbWinRegisterDeviceHandleNotification(HDEVNOTIFY* aDeviceNotification,
                                      HWND        aEventWindow,
                                      DEVINST     aDevInst,
                                      const GUID& aGUID)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceNotification);

  // Function variables.
  nsresult rv;

  // Create a device file for notifications.
  sbAutoHANDLE deviceHandle;
  rv = sbWinCreateDeviceFile(deviceHandle.StartAssignment(),
                             aDevInst,
                             &aGUID,
                             0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  // Register for device handle notifications.  The handle may be closed after
  // registration without affecting the registration.  Doing so avoids having
  // extra device file handles open.
  HDEVNOTIFY           deviceNotification;
  DEV_BROADCAST_HANDLE devBroadcast = {0};
  devBroadcast.dbch_size = sizeof(devBroadcast);
  devBroadcast.dbch_devicetype = DBT_DEVTYP_HANDLE;
  devBroadcast.dbch_handle = deviceHandle;
  deviceNotification = RegisterDeviceNotification(aEventWindow,
                                                  &devBroadcast,
                                                  0);
  NS_ENSURE_TRUE(deviceNotification, NS_ERROR_FAILURE);

  // Return results.
  *aDeviceNotification = deviceNotification;

  return NS_OK;
}

