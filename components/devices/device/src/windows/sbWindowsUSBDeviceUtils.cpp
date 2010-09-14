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
// Songbird Windows USB device utilities services.
//
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsUSBDeviceUtils.cpp
 * \brief Songbird Windows USB Device Utilities Source.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows USB device utilities imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbWindowsUSBDeviceUtils.h"

// Local imports.
#include "sbWindowsDeviceUtils.h"

// Songbird imports.
#include <sbWindowsUtils.h>

// Windows imports.
#include <devioctl.h>
#include <usbioctl.h>


//------------------------------------------------------------------------------
//
// Songbird Windows USB device utilities defs.
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
// Songbird USB device system services.
//
//------------------------------------------------------------------------------

/**
 * Return in aDescriptor the USB descriptor for the device specified by
 * aDeviceRef of descriptor type specified by aDescriptorType and descriptor
 * index specified by aDescriptorIndex and aDescriptorIndex2.  Read up to
 * aDescriptorLength bytes of descriptor data.
 *
 * \param aDeviceRef            Device reference for which to get descriptor.
 * \param aDescriptorType       Type of descriptor to get.
 * \param aDescriptorIndex      Index of descriptor to get.
 * \param aDescriptorIndex2     Second index of descriptor to get (only used for
 *                              string descriptors).
 * \param aDescriptorLength     Number of bytes of descriptor data to get.
 * \param aDescriptor           Returned descriptor.
 */

nsresult
sbUSBDeviceGetDescriptor(sbUSBDeviceRef*   aDeviceRef,
                         PRUint8           aDescriptorType,
                         PRUint8           aDescriptorIndex,
                         PRUint16          aDescriptorIndex2,
                         PRUint16          aDescriptorLength,
                         sbUSBDescriptor** aDescriptor)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceRef);
  NS_ENSURE_ARG_POINTER(aDescriptor);

  // Function variables.
  sbWinUSBDeviceRef* deviceRef = static_cast<sbWinUSBDeviceRef*>(aDeviceRef);
  DEVINST            devInst = deviceRef->devInst;
  HANDLE             hubFile = deviceRef->hubFile;
  sbAutoHANDLE       autoHubFile;
  ULONG              portIndex = deviceRef->portIndex;
  BOOL               success;
  nsresult           rv;

  // Ensure a hub file and port index are available.
  if (hubFile == INVALID_HANDLE_VALUE) {
    // Get the hub file and port index and set up for auto-disposal.
    rv = sbWinUSBDeviceGetHubAndPort(devInst, &hubFile, &portIndex);
    NS_ENSURE_SUCCESS(rv, rv);
    autoHubFile = hubFile;
  }

  // Allocate a descriptor request to get the descriptor and set it up for
  // auto-disposal.
  DWORD requestSize = sizeof(USB_DESCRIPTOR_REQUEST) + aDescriptorLength;
  PUSB_DESCRIPTOR_REQUEST descriptorRequest =
    static_cast<PUSB_DESCRIPTOR_REQUEST>(NS_Alloc(requestSize));
  NS_ENSURE_TRUE(descriptorRequest, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoDescriptorRequest(descriptorRequest);

  // Set up the descriptor request.
  descriptorRequest->ConnectionIndex = portIndex;
  descriptorRequest->SetupPacket.wValue =
                                   (aDescriptorType << 8) | aDescriptorIndex;
  descriptorRequest->SetupPacket.wIndex = aDescriptorIndex2;
  descriptorRequest->SetupPacket.wLength = aDescriptorLength;

  // Issue a get descriptor request.
  DWORD byteCount;
  success = DeviceIoControl(hubFile,
                            IOCTL_USB_GET_DESCRIPTOR_FROM_NODE_CONNECTION,
                            descriptorRequest,
                            requestSize,
                            descriptorRequest,
                            requestSize,
                            &byteCount,
                            NULL);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Get the descriptor.
  nsRefPtr<sbUSBDescriptor> descriptor =
                              new sbUSBDescriptor(descriptorRequest->Data,
                                                  aDescriptorLength);
  NS_ENSURE_TRUE(descriptor, NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(descriptor->Get(), NS_ERROR_OUT_OF_MEMORY);

  // Return results.
  descriptor.forget(aDescriptor);

  // No need to hold onto the hub file, we're done with it now.
  rv = sbWinUSBDeviceCloseRef(deviceRef);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird Windows USB device services.
//
//------------------------------------------------------------------------------

/**
 * Open a USB device reference to the device specified by aDevInst and return
 * the reference in aDeviceRef.
 *
 * \param aDevInst              USB device instance to open.
 * \param aDeviceRef            Returned device reference.
 */

nsresult
sbWinUSBDeviceOpenRef(DEVINST            aDevInst,
                      sbWinUSBDeviceRef* aDeviceRef)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceRef);

  // Function variables.
  nsresult rv;

  // Get the device hub and port.
  HANDLE hubFile;
  ULONG  portIndex;
  rv = sbWinUSBDeviceGetHubAndPort(aDevInst, &hubFile, &portIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  aDeviceRef->devInst = aDevInst;
  aDeviceRef->hubFile = hubFile;
  aDeviceRef->portIndex = portIndex;

  return NS_OK;
}


/**
 * Close the USB device reference specified by aDeviceRef.
 *
 * \param aDeviceRef            USB device reference to close.
 */

nsresult
sbWinUSBDeviceCloseRef(sbWinUSBDeviceRef* aDeviceRef)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aDeviceRef);

  // Close the hub file.
  if (aDeviceRef->hubFile != INVALID_HANDLE_VALUE) {
    CloseHandle(aDeviceRef->hubFile);
    aDeviceRef->hubFile = INVALID_HANDLE_VALUE;
  }

  return NS_OK;
}


/**
 * Return in aHubFile and aPortIndex the USB device hub file and port index for
 * the USB device specified by aDevInst.
 *
 * \param aDevInst              Device instance for which to get hub and port.
 * \param aHubFile              Returned USB hub file for device.
 * \param aPortIndex            Returned USB hub port index for device.
 */

nsresult
sbWinUSBDeviceGetHubAndPort(DEVINST aDevInst,
                            HANDLE* aHubFile,
                            ULONG*  aPortIndex)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aHubFile);
  NS_ENSURE_ARG_POINTER(aPortIndex);

  // Function variables.
  nsresult rv;

  // Create a file for the first hub ancestor of device and set it up for
  // auto-disposal.
  HANDLE hubFile;
  GUID guid = GUID_DEVINTERFACE_USB_HUB;
  rv = sbWinCreateAncestorDeviceFile(&hubFile,
                                     aDevInst,
                                     &guid,
                                     GENERIC_READ,
                                     FILE_SHARE_READ | FILE_SHARE_WRITE,
                                     NULL,
                                     OPEN_EXISTING,
                                     0,
                                     NULL);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoHANDLE autoHubFile(hubFile);

  // Find the device hub port index.
  ULONG portIndex;
  rv = sbWinUSBHubFindDevicePort(hubFile, aDevInst, &portIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  *aHubFile = autoHubFile.forget();
  *aPortIndex = portIndex;

  return NS_OK;
}


/**
 * Return in aPortIndex the USB hub index of the USB hub specified by aHubFile
 * for the USB device specified by aDevInst.
 *
 * \param aHubFile              USB hub for which to find port.
 * \param aDevInst              USB device instance for which to find port.
 * \param aPortIndex            Returned USB hub port index for device.
 */

nsresult
sbWinUSBHubFindDevicePort(HANDLE  aHubFile,
                          DEVINST aDevInst,
                          ULONG*  aPortIndex)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aPortIndex);

  // Function variables.
  DWORD     byteCount;
  BOOL      success;
  CONFIGRET cr;
  nsresult  rv;

  // Get the device driver key.
  TCHAR winDriverKey[256];
  nsAutoString driverKey;
  ULONG len = sizeof(winDriverKey);
  cr = CM_Get_DevNode_Registry_Property(aDevInst,
                                        CM_DRP_DRIVER,
                                        NULL,
                                        winDriverKey,
                                        &len,
                                        0);
  NS_ENSURE_TRUE(cr == CR_SUCCESS, NS_ERROR_FAILURE);
  driverKey = winDriverKey;

  // Get the number of USB hub ports.
  USB_NODE_INFORMATION usbNodeInformation;
  success = DeviceIoControl(aHubFile,
                            IOCTL_USB_GET_NODE_INFORMATION,
                            NULL,
                            0,
                            &usbNodeInformation,
                            sizeof(usbNodeInformation),
                            &byteCount,
                            NULL);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  ULONG portCount =
          usbNodeInformation.u.HubInformation.HubDescriptor.bNumberOfPorts;

  // Check each USB hub port for a matching driver key.
  ULONG portIndex;
  for (portIndex = 1; portIndex <= portCount; portIndex++) {
    // Get the port driver key.
    nsAutoString portDriverKey;
    rv = sbWinUSBHubGetPortDriverKey(aHubFile, portIndex, portDriverKey);
    NS_ENSURE_SUCCESS(rv, rv);

    // Check for a match.
    if (driverKey.Equals(portDriverKey))
      break;
  }
  NS_ENSURE_TRUE(portIndex <= portCount, NS_ERROR_NOT_AVAILABLE);

  // Return results.
  *aPortIndex = portIndex;

  return NS_OK;
}


/**
 * Return in aDriverKey the driver key for the device attached to the port
 * specified by aPortIndex of the USB hub specified by aHubFile.  If no device
 * is attached to the port, aDriverKey is set to void.
 *
 * \param aHubFile              USB hub for which to get port driver key.
 * \param aPortIndex            Index of USB hub port for which to get driver
 *                              key.
 * \param aDriverKey            Returned USB hub port driver key.
 */

nsresult
sbWinUSBHubGetPortDriverKey(HANDLE     aHubFile,
                            ULONG      aPortIndex,
                            nsAString& aDriverKey)
{
  USB_NODE_CONNECTION_DRIVERKEY_NAME* driverKeyName;
  DWORD                               requestSize;
  DWORD                               byteCount;
  BOOL                                success;

  // Set default result.
  aDriverKey.SetIsVoid(PR_TRUE);

  // Issue a get driver key request to probe the full request size.  Assume port
  // is not connected on error.
  USB_NODE_CONNECTION_DRIVERKEY_NAME  probeDriverKeyName;
  driverKeyName = &probeDriverKeyName;
  driverKeyName->ConnectionIndex = aPortIndex;
  requestSize = sizeof(probeDriverKeyName);
  success = DeviceIoControl(aHubFile,
                            IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                            driverKeyName,
                            requestSize,
                            driverKeyName,
                            requestSize,
                            &byteCount,
                            NULL);
  if (!success)
    return NS_OK;

  // Allocate a get driver key request and set it up for auto-disposal.
  requestSize = sizeof(USB_NODE_CONNECTION_DRIVERKEY_NAME) +
                probeDriverKeyName.ActualLength;
  driverKeyName =
    static_cast<USB_NODE_CONNECTION_DRIVERKEY_NAME*>(NS_Alloc(requestSize));
  NS_ENSURE_TRUE(driverKeyName, NS_ERROR_OUT_OF_MEMORY);
  sbAutoNSMemPtr autoDriverKeyName(driverKeyName);

  // Issue a get driver key request to get the driver key.
  driverKeyName->ConnectionIndex = aPortIndex;
  success = DeviceIoControl(aHubFile,
                            IOCTL_USB_GET_NODE_CONNECTION_DRIVERKEY_NAME,
                            driverKeyName,
                            requestSize,
                            driverKeyName,
                            requestSize,
                            &byteCount,
                            NULL);
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // Return results.
  aDriverKey.Assign(driverKeyName->DriverKeyName);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird Windows USB MSC device services.
//
//------------------------------------------------------------------------------

/**
 * Return in aLUN the USB MSC LUN for the disk device specified by aDevInst.
 *
 * \param aDevInst              Disk for which to get LUN.
 * \param aLUN                  Returned LUN.
 */

nsresult
sbWinUSBMSCGetLUN(DEVINST   aDevInst,
                  PRUint32* aLUN)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aLUN);

  // Function variables.
  nsresult rv;

  // Get the device instance ID.
  nsAutoString deviceInstanceID;
  rv = sbWinGetDeviceInstanceID(aDevInst, deviceInstanceID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Extract the LUN from the last number in the device instance ID.
  // E.g., "USBSTOR\DISK&VEN_SANDISK&PROD_SANSA_FUZE_4GB&REV_V02.\"
  //       "E812EA114524B6A80000000000000000&1"
  //       for LUN = 1.
  //XXXeps It sure would be nice to have a better way of doing this.
  PRUint32 lun = 0;
  PRInt32  index = deviceInstanceID.RFind(NS_LITERAL_STRING("&"));
  if (index >= 0) {
    // Extract the LUN.  Assume a LUN of zero on any failure.
    lun = Substring(deviceInstanceID, index + 1).ToInteger(&rv);
    if (NS_FAILED(rv))
      lun = 0;
  }

  // Return results.
  *aLUN = lun;

  return NS_OK;
}


