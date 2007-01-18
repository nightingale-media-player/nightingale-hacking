/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END SONGBIRD GPL
//
*/

#ifndef __USB_MASS_STORAGE_DEVICE_WIN32_H__
#define __USB_MASS_STORAGE_DEVICE_WIN32_H__

/** 
 * \file  USBMassStorageDeviceWin32.h
 * \brief Songbird USBMassStorageDevice Win32 Definition.
 */

#include "nspr.h"
#include <nsStringGlue.h>

#include "usb.h"
#include "USBMassStorageDeviceHelper.h"

//#define USB_LANGUAGE_ID_EN_US (0x409)
#define USB_LANGUAGE_ID_EN_US (0x0)

class CUSBMassStorageDeviceHelperWin32 : public IUSBMassStorageDeviceHelper
{
public:
  CUSBMassStorageDeviceHelperWin32();
  virtual ~CUSBMassStorageDeviceHelperWin32();
  
  virtual PRBool Initialize(const nsAString &deviceName, const nsAString &deviceIdentifier);
  virtual PRBool Shutdown();

  virtual PRBool IsInitialized();

  virtual const nsAString & GetDeviceVendor();
  virtual const nsAString & GetDeviceModel();
  virtual const nsAString & GetDeviceSerialNumber();

  virtual const nsAString & GetDeviceMountPoint();
  virtual PRInt64 GetDeviceCapacity();

  virtual PRBool UpdateMountPoint(const nsAString &deviceMountPoint);

private:
  void GetDeviceInformation();
  
  int GetStringFromDescriptor(int index, nsAString &deviceString, usb_dev_handle *handle = nsnull);
  
  struct usb_device *GetDeviceByName(const nsAString &deviceName);

  nsString m_DeviceName;
  nsString m_DeviceIdentifier;
  
  nsString m_DeviceMountPoint;

  nsString m_DeviceVendorName;
  nsString m_DeviceModelName;
  nsString m_DeviceSerialNumber;

  struct usb_bus    *m_USBBusses;
  struct usb_device *m_USBDescriptor;
  usb_dev_handle    *m_USBDevice;

};

#endif // __USB_MASS_STORAGE_DEVICE_WIN32_H__

