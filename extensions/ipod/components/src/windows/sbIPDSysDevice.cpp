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
#include <nsIPropertyBag2.h>
#include <nsIWritablePropertyBag.h>
#include <prprf.h>

// Win32 imports.
#include <devioctl.h>

#include <ntddstor.h>

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
  CONFIGRET                   cfgRet;
  nsresult                    rv;

  // call the parent
  rv = sbIPDDevice::Eject();
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the parent device instance data record.
  DEVINST parentDevInst;
  cfgRet = CM_Get_Parent(&parentDevInst, mDevInst, 0);
  NS_ENSURE_TRUE(cfgRet == CR_SUCCESS, NS_ERROR_FAILURE);

  // Try ejecting the device three times.
  WCHAR         vetoName[MAX_PATH];
  PNP_VETO_TYPE vetoType;
  bool        ejected = PR_FALSE;
  for (int i = 0; i < 3; i++) {
    // Try ejecting using CM_Query_And_Remove_SubTree.
    cfgRet = CM_Query_And_Remove_SubTreeW(parentDevInst,
                                          &vetoType,
                                          vetoName,
                                          MAX_PATH,
                                          0);
    if (cfgRet == CR_SUCCESS) {
      ejected = PR_TRUE;
      break;
    }

    // Try ejecting using CM_Request_DeviceEject.
    cfgRet = CM_Request_Device_Eject(parentDevInst,
                                     &vetoType,
                                     vetoName,
                                     MAX_PATH,
                                     0);
    if (cfgRet == CR_SUCCESS) {
      ejected = PR_TRUE;
      break;
    }
  }

  // Try one last time and let the PnP manager notify the user of failure.
  if (!ejected) {
    cfgRet = CM_Request_Device_Eject(parentDevInst,
                                     NULL,
                                     NULL,
                                     0,
                                     0);
    NS_ENSURE_TRUE(cfgRet == CR_SUCCESS, NS_ERROR_FAILURE);
  }

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
  mProperties(aProperties)
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

  // Get the device properties.
  nsCOMPtr<nsIPropertyBag2> properties = do_QueryInterface(mProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIWritablePropertyBag> writeProperties =
                                     do_QueryInterface(mProperties, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the device drive letter.
  char          driveLetter;
  nsCAutoString nsDriveLetter;
  rv = properties->GetPropertyAsACString(NS_LITERAL_STRING("DriveLetter"),
                                         nsDriveLetter);
  NS_ENSURE_SUCCESS(rv, rv);
  driveLetter = nsDriveLetter.get()[0];

  // Get the device instance.
  rv = GetDevInst(driveLetter, &mDevInst);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the device mount path property.
  nsAutoString mountPath;
  mountPath.Assign(NS_ConvertUTF8toUTF16(nsDriveLetter));
  mountPath.AppendLiteral(":\\");
  rv = writeProperties->SetProperty(NS_LITERAL_STRING("MountPath"),
                                    sbIPDVariant(mountPath).get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the Firewire GUID property.
  nsAutoString firewireGUID;
  rv = GetFirewireGUID(firewireGUID);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = writeProperties->SetProperty(NS_LITERAL_STRING("FirewireGUID"),
                                    sbIPDVariant(firewireGUID).get());
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the device manufacturer and model number properties.
  rv = writeProperties->SetProperty
                           (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MANUFACTURER),
                            sbIPDVariant("Apple").get());
  NS_ENSURE_SUCCESS(rv, rv);
  rv = writeProperties->SetProperty(NS_LITERAL_STRING(SB_DEVICE_PROPERTY_MODEL),
                                    sbIPDVariant("iPod").get());
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


//------------------------------------------------------------------------------
//
// Internal iPod system dependent device services
//
//------------------------------------------------------------------------------

/**
 * Get the iPod device Firewire GUID and return it in aFirewireGUID.
 *
 * \param aFirewireGUID         Firewire GUID.
 */

nsresult
sbIPDSysDevice::GetFirewireGUID(nsAString& aFirewireGUID)
{
  CONFIGRET                   cfgRet;

  // Get the parent device instance data record.
  DEVINST devInst;
  cfgRet = CM_Get_Parent(&devInst, mDevInst, 0);
  NS_ENSURE_TRUE(cfgRet == CR_SUCCESS, NS_ERROR_FAILURE);

  // Get the device ID.
  WCHAR wDeviceID[256];
  nsAutoString deviceID;
  cfgRet = CM_Get_Device_IDW(devInst, wDeviceID, sizeof (wDeviceID), 0);
  NS_ENSURE_TRUE(cfgRet == CR_SUCCESS, NS_ERROR_FAILURE);
  deviceID.Assign(wDeviceID);

  // Get the Firewire GUID from the serial number.
  PRInt32 pos = deviceID.RFindChar('\\');
  NS_ENSURE_TRUE(pos != -1, NS_ERROR_FAILURE);
  aFirewireGUID = Substring(deviceID, pos + 1, 16);

  return NS_OK;
}


/**
 * Return in aDevInst the win32 iPod device instance data record for the drive
 * letter specified by aDriveLetter.
 *
 * \param aDriveLetter          Drive letter of device.
 * \param aDevInst              Win32 device instance.
 */

nsresult
sbIPDSysDevice::GetDevInst(char     aDriveLetter,
                           DEVINST* aDevInst)
{
  // Validate arguments.
  NS_ASSERTION(aDevInst, "aDevInst is null");

  // Function variables.
  GUID     guid = GUID_DEVINTERFACE_DISK;
  nsresult rv;

  // Produce the device path.
  char         cDevPath[8];
  nsAutoString devPath;
  PR_snprintf(cDevPath, sizeof(cDevPath), "\\\\.\\%c:", aDriveLetter);
  devPath.AssignLiteral(cDevPath);

  // Get the target device number.
  ULONG targetDevNum;
  rv = GetDevNum(devPath.get(), &targetDevNum);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the volume interface device class info and set up for auto-disposal.
  HDEVINFO devInfo = SetupDiGetClassDevsW
                       (&guid,
                        NULL,
                        NULL,
                        DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
  NS_ENSURE_TRUE(devInfo != INVALID_HANDLE_VALUE, NS_ERROR_FAILURE);
  sbIPDAutoDevInfo autoDevInfo(devInfo);

  // Search for the device instance with a matching device number.
  DEVINST devInst;
  for (DWORD devIndex = 0; ; devIndex++) {
    // Get the next device detail data and set it up for auto-disposal.
    PSP_DEVICE_INTERFACE_DETAIL_DATA devIfDetailData;
    SP_DEVINFO_DATA                  devInfoData;
    rv = GetDevDetail(&devIfDetailData,
                      &devInfoData,
                      devInfo,
                      &guid,
                      devIndex);
    NS_ENSURE_TRUE(rv != NS_ERROR_NOT_AVAILABLE, NS_ERROR_NOT_AVAILABLE);
    if (NS_FAILED(rv))
      continue;
    sbAutoMemPtr<SP_DEVICE_INTERFACE_DETAIL_DATA> autoDevIfDetailData
                                                    (devIfDetailData);

    // Get the next device number.
    ULONG devNum;
    rv = GetDevNum(devIfDetailData->DevicePath, &devNum);
    NS_ENSURE_SUCCESS(rv, rv);

    // Check if the device number matches the iPod device number.
    if (devNum == targetDevNum) {
      devInst = devInfoData.DevInst;
      break;
    }
  }

  // Return results.
  *aDevInst = devInst;

  return NS_OK;
}


/**
 * Return in aDevNum the device number for the device with the device path
 * specified by aDevPath.
 *
 * \param aDevPath              Device path.
 * \param aDevNum               Device number.
 */

nsresult
sbIPDSysDevice::GetDevNum(LPCTSTR aDevPath,
                          ULONG*  aDevNum)
{
  // Validate arguments.
  NS_ASSERTION(aDevPath, "aDevPath is null");
  NS_ASSERTION(aDevNum, "aDevNum is null");

  // Function variables.
  BOOL     success;

  // Create a device file and set it up for auto-disposal.
  HANDLE devFile = CreateFileW(aDevPath,
                               0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
  NS_ENSURE_TRUE(devFile != INVALID_HANDLE_VALUE, NS_ERROR_FAILURE);
  sbIPDAutoFile autoDevFile(devFile);

  // Get the device number.
  STORAGE_DEVICE_NUMBER getDevNumParams;
  DWORD                 byteCount;
  success = DeviceIoControl(devFile,
                            IOCTL_STORAGE_GET_DEVICE_NUMBER,
                            NULL,
                            0,
                            &getDevNumParams,
                            sizeof(getDevNumParams),
                            &byteCount,
                            NULL);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(getDevNumParams.DeviceNumber != 0xFFFFFFFF,
                 NS_ERROR_FAILURE);

  // Return results.
  *aDevNum = getDevNumParams.DeviceNumber;

  return NS_OK;
}


/**
 * Return in aDevIfDetailData and aDevInfoData the device details for a device.
 * The device is specified by the enumeration handle specified by aDevInfo.  The
 * device class GUID is specified by aGUID, and the device enumeration index is
 * specified by aDevIndex.
 *
 * \param aDevIfDetailData      Device detail data.
 * \param aDevInfoData          Device info data.
 * \param aDevInfo              Device info enumeration handle.
 * \param aGUID                 Device class GUID.
 * \param aDevIndex             Device enumeration index.
 */

nsresult
sbIPDSysDevice::GetDevDetail(PSP_DEVICE_INTERFACE_DETAIL_DATA* aDevIfDetailData,
                             SP_DEVINFO_DATA*                  aDevInfoData,
                             HDEVINFO                          aDevInfo,
                             GUID*                             aGUID,
                             DWORD                             aDevIndex)
{
  // Validate arguments.
  NS_ASSERTION(aDevIfDetailData, "aDevIfDetailData is null");
  NS_ASSERTION(aDevInfoData, "aDevInfoData is null");
  NS_ASSERTION(aGUID, "aGUID is null");

  // Function variables.
  BOOL     success;

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
  PSP_DEVICE_INTERFACE_DETAIL_DATA devIfDetailData;
  devIfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA) malloc(size);
  NS_ENSURE_TRUE(devIfDetailData, NS_ERROR_OUT_OF_MEMORY);
  sbAutoMemPtr<SP_DEVICE_INTERFACE_DETAIL_DATA> autoDevIfDetailData
                                                  (devIfDetailData);
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
  *aDevIfDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)
                        autoDevIfDetailData.forget();

  return NS_OK;
}


