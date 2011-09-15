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
// Songbird Windows storage device utilities services.
//
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsStorageDeviceUtils.cpp
 * \brief Songbird Windows Storage Device Utilities Source.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows storage device utilities imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbWindowsStorageDeviceUtils.h"

// Local imports.
#include "sbWindowsDeviceUtils.h"

// Songbird imports.
#include <sbWindowsUtils.h>

// Windows imports.
#include <ntddscsi.h>


//------------------------------------------------------------------------------
//
// Songbird Windows device storage services.
//
//------------------------------------------------------------------------------

/**
 *   Search for devices with the storage device number specified by
 * aStorageDevNum and interface class specified by aGUID.  Return all matching
 * device instances in aDevInstList.
 *   If aMatchPartitionNumber is true, look for device instances with a
 * matching partition number.  This should be set to true when looking for
 * partition or volume interfaces and false when looking for disk interfaces.
 *
 * \param aStorageDevNum        Storage device number for which to find device.
 * \param aMatchPartitionNumber Match partition numbers when looking for a
 *                              device interface.
 * \param aGUID                 Device interface class for which to search.
 * \param aDevInstList          List of matching device instances.
 */

nsresult
sbWinFindDevicesByStorageDevNum(STORAGE_DEVICE_NUMBER* aStorageDevNum,
                                PRBool                 aMatchPartitionNumber,
                                const GUID*            aGUID,
                                nsTArray<DEVINST>&     aDevInstList)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aStorageDevNum);
  NS_ENSURE_ARG_POINTER(aGUID);

  // Function variables.
  nsresult rv;

  // Get the interface device class info and set up for auto-disposal.
  HDEVINFO devInfo =
             SetupDiGetClassDevsW(aGUID,
                                  NULL,
                                  NULL,
                                  DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
  NS_ENSURE_TRUE(devInfo != INVALID_HANDLE_VALUE, NS_ERROR_FAILURE);
  sbAutoHDEVINFO autoDevInfo(devInfo);

  // Search for device instances with a matching storage device number.
  aDevInstList.Clear();
  DWORD devIndex = 0;
  while (1) {
    // Get the next device detail data and set it up for auto-disposal.
    PSP_DEVICE_INTERFACE_DETAIL_DATA devIfDetailData;
    SP_DEVINFO_DATA                  devInfoData;
    rv = sbWinGetDevDetail(&devIfDetailData,
                           &devInfoData,
                           devInfo,
                           aGUID,
                           devIndex++);
    if (rv == NS_ERROR_NOT_AVAILABLE)
      break;
    NS_ENSURE_SUCCESS(rv, rv);
    sbAutoMemPtr<SP_DEVICE_INTERFACE_DETAIL_DATA>
      autoDevIfDetailData(devIfDetailData);

    // Get the next storage device number.
    STORAGE_DEVICE_NUMBER storageDevNum;
    rv = sbWinGetStorageDevNum(devIfDetailData->DevicePath, &storageDevNum);
    if (NS_FAILED(rv))
      continue;

    // Skip device instance if it doesn't match the target storage device
    // number.
    if (storageDevNum.DeviceType != aStorageDevNum->DeviceType)
      continue;
    if (storageDevNum.DeviceNumber != aStorageDevNum->DeviceNumber)
      continue;
    if (aMatchPartitionNumber &&
        (storageDevNum.PartitionNumber != aStorageDevNum->PartitionNumber)) {
      continue;
    }

    // Add device instance to list.
    NS_ENSURE_TRUE(aDevInstList.AppendElement(devInfoData.DevInst),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}


/**
 * Return in aStorageDevNum the storage device number for the device instance
 * specified by aDevInst using the device interface class specified by aGUID.
 *
 * \param aDevInst              Device instance for which to get device number.
 * \param aGUID                 Device interface class.
 * \param aDevNum               Device number.
 *
 * \return NS_ERROR_NOT_AVAILABLE if device does not have a storage device
 *         number.
 */

nsresult
sbWinGetStorageDevNum(DEVINST                aDevInst,
                      const GUID*            aGUID,
                      STORAGE_DEVICE_NUMBER* aStorageDevNum)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aStorageDevNum);
  NS_ENSURE_ARG_POINTER(aGUID);

  // Function variables.
  nsresult rv;

  // Get the device interface path.
  nsAutoString deviceInterfacePath;
  rv = sbWinGetDevicePath(aDevInst, aGUID, deviceInterfacePath);
  NS_ENSURE_SUCCESS(rv, rv);

  return sbWinGetStorageDevNum(deviceInterfacePath.get(), aStorageDevNum);
}


/**
 * Return in aStorageDevNum the storage device number for the device with the
 * device path specified by aDevPath.
 *
 * \param aDevPath              Device path.
 * \param aStorageDevNum        Returned storage device number.
 *
 * \return NS_ERROR_NOT_AVAILABLE if device does not have a storage device
 *         number.
 */

nsresult
sbWinGetStorageDevNum(LPCTSTR                aDevPath,
                      STORAGE_DEVICE_NUMBER* aStorageDevNum)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDevPath);
  NS_ENSURE_ARG_POINTER(aStorageDevNum);

  // Function variables.
  PRBool hasDevNum = PR_TRUE;
  BOOL   success;

  // Create a device file and set it up for auto-disposal.
  HANDLE devFile = CreateFileW(aDevPath,
                               0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
  if (devFile == INVALID_HANDLE_VALUE)
    return NS_ERROR_NOT_AVAILABLE;
  sbAutoHANDLE autoDevFile(devFile);

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
  if (!success)
    return NS_ERROR_NOT_AVAILABLE;
  NS_ENSURE_TRUE(getDevNumParams.DeviceNumber != 0xFFFFFFFF,
                 NS_ERROR_FAILURE);

  // Return results.
  CopyMemory(aStorageDevNum, &getDevNumParams, sizeof(STORAGE_DEVICE_NUMBER));

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird Windows volume device services.
//
//------------------------------------------------------------------------------

/**
 * Return true in aIsReady if the volume device specified by aDevInst is ready;
 * otherwise, return false in aIsReady.
 *
 * \param aDevInst              Volume device instance.
 * \param aIsReady              Returned is ready state.
 */

nsresult
sbWinVolumeIsReady(DEVINST aDevInst,
                   PRBool* aIsReady)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aIsReady);

  // Function variables.
  PRBool   success;
  nsresult rv;

  // Get the volume device interface path.
  GUID guid = GUID_DEVINTERFACE_VOLUME;
  nsAutoString volumeDeviceInterfacePath;
  rv = sbWinGetDevicePath(aDevInst, &guid, volumeDeviceInterfacePath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Try getting the volume GUID path.  If this fails, the volume is not ready.
  static const DWORD volumeGUIDPathLength = 51;
  WCHAR volumeGUIDPath[volumeGUIDPathLength];
  volumeDeviceInterfacePath.AppendLiteral("\\");
  success = GetVolumeNameForVolumeMountPointW(volumeDeviceInterfacePath.get(),
                                              volumeGUIDPath,
                                              volumeGUIDPathLength);
  if (!success) {
    *aIsReady = PR_FALSE;
    return NS_OK;
  }

  // Try getting the volume information.  If this fails, the volume is not
  // ready.
  DWORD fileSystemFlags;
  WCHAR  volumeLabel[MAX_PATH+1];
  WCHAR  fileSystemName[MAX_PATH+1];
  success = GetVolumeInformationW(volumeDeviceInterfacePath.BeginReading(),
                                  volumeLabel,
                                  NS_ARRAY_LENGTH(volumeLabel),
                                  NULL,
                                  NULL,
                                  &fileSystemFlags,
                                  fileSystemName,
                                  NS_ARRAY_LENGTH(fileSystemName));
  if (!success) {
    *aIsReady = PR_FALSE;
    return NS_OK;
  }

  // The volume is ready.
  *aIsReady = PR_TRUE;

  return NS_OK;
}


/**
 * Return true in aIsReadOnly if the volume with the mount path specified by
 * aVolumeMountPath is a read-only volue; otherwise, return false.
 *
 * \param aVolumeMountPath      Volume mount path.
 * \param aIsReadOnly           Returned read-only state.
 */

nsresult
sbWinVolumeGetIsReadOnly(const nsAString& aVolumeMountPath,
                         PRBool*          aIsReadOnly)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aIsReadOnly);

  // Function variables.
  BOOL success;

  // Get the file system flags.
  DWORD fileSystemFlags;
  WCHAR  volumeLabel[MAX_PATH+1];
  WCHAR  fileSystemName[MAX_PATH+1];
  success = GetVolumeInformationW(aVolumeMountPath.BeginReading(),
                                  volumeLabel,
                                  NS_ARRAY_LENGTH(volumeLabel),
                                  NULL,
                                  NULL,
                                  &fileSystemFlags,
                                  fileSystemName,
                                  NS_ARRAY_LENGTH(fileSystemName));
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Return results.
  if (fileSystemFlags & FILE_READ_ONLY_VOLUME)
    *aIsReadOnly = PR_TRUE;
  else
    *aIsReadOnly = PR_FALSE;

  return NS_OK;
}


/**
 * Return in aVolumeGUIDPath the volume GUID path for the volume device
 * specified by aDevInst.
 *
 * See http://msdn.microsoft.com/en-us/library/aa365248(VS.85).aspx
 *
 * \param aDevInst              Volume device instance.
 * \param aVolumeGUIDPath       Returned volume GUID path.
 */

nsresult
sbWinGetVolumeGUIDPath(DEVINST    aDevInst,
                       nsAString& aVolumeGUIDPath)
{
  BOOL     success;
  nsresult rv;

  // Get the volume device interface path.
  GUID guid = GUID_DEVINTERFACE_VOLUME;
  nsAutoString volumeDeviceInterfacePath;
  rv = sbWinGetDevicePath(aDevInst, &guid, volumeDeviceInterfacePath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the volume GUID path.  The mount point must end with "\\".
  static const DWORD volumeGUIDPathLength = 51;
  WCHAR volumeGUIDPath[volumeGUIDPathLength];
  volumeDeviceInterfacePath.AppendLiteral("\\");
  success = GetVolumeNameForVolumeMountPointW(volumeDeviceInterfacePath.get(),
                                              volumeGUIDPath,
                                              volumeGUIDPathLength);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Return results.
  aVolumeGUIDPath.Assign(volumeGUIDPath);

  return NS_OK;
}


/**
 * Return in aVolumeGUID the volume GUID for the volume device specified by
 * aDevInst.  The volume GUID is extracted from the volume GUID path.
 *
 * See http://msdn.microsoft.com/en-us/library/aa365248(VS.85).aspx
 *
 * \param aDevInst              Volume device instance.
 * \param aVolumeGUID           Returned volume GUID.
 */

nsresult
sbWinGetVolumeGUID(DEVINST    aDevInst,
                   nsAString& aVolumeGUID)
{
  nsresult rv;

  // Start with the volume GUID path.
  rv = sbWinGetVolumeGUIDPath(aDevInst, aVolumeGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Strip everything but the volume GUID.  Transform
  // "\\?\Volume{26a21bda-a627-11d7-9931-806e6f6e6963}\" to
  // "{26a21bda-a627-11d7-9931-806e6f6e6963}".
  PRInt32 index;
  index = aVolumeGUID.Find("{");
  NS_ENSURE_TRUE(index >= 0, NS_ERROR_UNEXPECTED);
  aVolumeGUID.Cut(0, index);
  index = aVolumeGUID.Find("}");
  NS_ENSURE_TRUE(index >= 0, NS_ERROR_UNEXPECTED);
  aVolumeGUID.SetLength(index + 1);

  return NS_OK;
}


/**
 * Return in aPathNames the list of paths names for the volume device specified
 * by aDevInst.
 *
 * \param aDevInst              Volume device instance.
 * \param aPathNames            List of volume path names.
 */

nsresult
sbWinGetVolumePathNames(DEVINST             aDevInst,
                        nsTArray<nsString>& aPathNames)
{
  nsresult rv;

  // Get the volume GUID path.
  nsAutoString volumeGUIDPath;
  rv = sbWinGetVolumeGUIDPath(aDevInst, volumeGUIDPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the volume path names.
  rv = sbWinGetVolumePathNames(volumeGUIDPath, aPathNames);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Return in aPathNames the list of paths names for the volume device specified
 * by aVolumeGUIDPath.
 *
 * \param aVolumeGUIDPath       Volume GUID path.
 * \param aPathNames            List of volume path names.
 */

nsresult
sbWinGetVolumePathNames(nsAString&          aVolumeGUIDPath,
                        nsTArray<nsString>& aPathNames)
{
  BOOL success;

  // Clear the list of path names.
  aPathNames.Clear();

  // Get the total length of all of the path names.  Do nothing more if the
  // length is zero.
  DWORD pathNamesLength;
  success = GetVolumePathNamesForVolumeNameW(aVolumeGUIDPath.BeginReading(),
                                             NULL,
                                             0,
                                             &pathNamesLength);
  if (!success)
    NS_ENSURE_TRUE(GetLastError() == ERROR_MORE_DATA, NS_ERROR_FAILURE);
  if (pathNamesLength == 0)
    return NS_OK;

  // Allocate memory for the path names and set it up for auto-disposal.
  WCHAR* pathNames =
           static_cast<WCHAR*>(malloc(pathNamesLength * sizeof(WCHAR)));
  NS_ENSURE_TRUE(pathNames, NS_ERROR_OUT_OF_MEMORY);
  sbAutoMemPtr<WCHAR> autoPathNames(pathNames);

  // Get the volume path names.
  success = GetVolumePathNamesForVolumeNameW(aVolumeGUIDPath.BeginReading(),
                                             pathNames,
                                             pathNamesLength,
                                             &pathNamesLength);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Add the names to the volume path name list.
  DWORD pathNameIndex = 0;
  while (pathNameIndex < pathNamesLength) {
    // Get the next path name.  Exit loop if no more paths.
    WCHAR* pathName = pathNames + pathNameIndex;
    if (pathName[0] == L'\0')
      break;

    // Add the path name to the list.
    aPathNames.AppendElement(pathName);

    // Add the next path name.
    pathNameIndex += wcslen(pathName) + 1;
  }

  return NS_OK;
}


/**
 * Get the label for the volume mounted at the path specified by
 * aVolumeMountPath and return the label in aVolumeLabel.
 *
 * \param aVolumeMountPath      Volume mount path.
 * \param aVolumeLabel          Returned volume label in UTF-8.
 */

nsresult
sbWinGetVolumeLabel(const nsAString& aVolumeMountPath,
                    nsACString&      aVolumeLabel)
{
  BOOL success;

  // Get the volume label.
  WCHAR  volumeLabel[MAX_PATH+1];
  WCHAR  fileSystemName[MAX_PATH+1];
  success = GetVolumeInformationW(aVolumeMountPath.BeginReading(),
                                  volumeLabel,
                                  NS_ARRAY_LENGTH(volumeLabel),
                                  NULL,
                                  NULL,
                                  NULL,
                                  fileSystemName,
                                  NS_ARRAY_LENGTH(fileSystemName));
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Return results.
  aVolumeLabel.Assign(NS_ConvertUTF16toUTF8(volumeLabel));

  return NS_OK;
}


/**
 * Set the label for the volume mounted at the path specified by
 * aVolumeMountPath to the label specified by aVolumeLabel.
 *
 * \param aVolumeMountPath      Volume mount path.
 * \param aVolumeLabel          Volume label to set in UTF-8.
 */

nsresult
sbWinSetVolumeLabel(const nsAString&  aVolumeMountPath,
                    const nsACString& aVolumeLabel)
{
  BOOL success;

  // Set the volume label.
  success = SetVolumeLabelW(aVolumeMountPath.BeginReading(),
                            NS_ConvertUTF8toUTF16(aVolumeLabel).get());
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird Windows SCSI device utilities services.
//
//------------------------------------------------------------------------------

//
// Songbird Windows SCSI device utilities defs.
//

#define CDB6GENERIC_LENGTH         6

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
  SCSI_PASS_THROUGH Spt;
  ULONG             Filler;      // realign buffers to double word boundary
  UCHAR             SenseBuf[32];
  UCHAR             DataBuf[512];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

#define SCSIOP_INQUIRY             0x12


/**
 * Return in aVendorID and aProductID the SCSI vendor and product IDs for the
 * device specified by aDevInst.
 *
 * \param aDevInst              Device for which to get SCSI product info.
 * \param aVendorID             SCSI vendor ID.
 * \param aProductID            SCSI product ID.
 */

nsresult
sbWinGetSCSIProductInfo(DEVINST    aDevInst,
                        nsAString& aVendorID,
                        nsAString& aProductID)
{
  DWORD    byteCount;
  BOOL     success;
  errno_t  errno;
  nsresult rv;

  // Create a disk interface device file.
  sbAutoHANDLE diskHandle;
  GUID         guid = GUID_DEVINTERFACE_DISK;
  rv = sbWinCreateDeviceFile(diskHandle.StartAssignment(),
                             aDevInst,
                             &guid,
                             0,
                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up a storage property query to get the storage device descriptor.
  STORAGE_PROPERTY_QUERY storagePropertyQuery;
  memset(&storagePropertyQuery, 0, sizeof(storagePropertyQuery));
  storagePropertyQuery.PropertyId = StorageDeviceProperty;
  storagePropertyQuery.QueryType = PropertyStandardQuery;

  // Determine how many bytes are required to hold the full storage device
  // descriptor.
  STORAGE_DESCRIPTOR_HEADER storageDescriptorHeader;
  success = DeviceIoControl(diskHandle,
                            IOCTL_STORAGE_QUERY_PROPERTY,
                            &storagePropertyQuery,
                            sizeof(storagePropertyQuery),
                            &storageDescriptorHeader,
                            sizeof(storageDescriptorHeader),
                            &byteCount,
                            NULL);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(byteCount == sizeof(storageDescriptorHeader),
                 NS_ERROR_FAILURE);

  // Allocate the storage device descriptor.
  sbAutoMemPtr<STORAGE_DEVICE_DESCRIPTOR>
    storageDeviceDescriptor = static_cast<PSTORAGE_DEVICE_DESCRIPTOR>
                                (malloc(storageDescriptorHeader.Size));
  NS_ENSURE_TRUE(storageDeviceDescriptor, NS_ERROR_OUT_OF_MEMORY);

  // Get the storage device descriptor.
  success = DeviceIoControl(diskHandle,
                            IOCTL_STORAGE_QUERY_PROPERTY,
                            &storagePropertyQuery,
                            sizeof(storagePropertyQuery),
                            storageDeviceDescriptor,
                            storageDescriptorHeader.Size,
                            &byteCount,
                            NULL);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  NS_ENSURE_TRUE(byteCount == storageDescriptorHeader.Size, NS_ERROR_FAILURE);

  // Return results with trailing spaces trimmed.  SCSI inquiry vendor and
  // product IDs have trailing spaces as filler.
  aVendorID.AssignLiteral
              (reinterpret_cast<char*>(storageDeviceDescriptor.get()) +
               storageDeviceDescriptor->VendorIdOffset);
  aVendorID.Trim(" ", PR_FALSE, PR_TRUE);
  aProductID.AssignLiteral
               (reinterpret_cast<char*>(storageDeviceDescriptor.get()) +
                storageDeviceDescriptor->ProductIdOffset);
  aProductID.Trim(" ", PR_FALSE, PR_TRUE);

  return NS_OK;
}

