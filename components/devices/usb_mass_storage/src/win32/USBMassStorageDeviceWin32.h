/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 POTI, Inc.
// http://songbirdnest.com
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
#include "nsString.h"

#include <windows.h>
#include <stdio.h>

#include <usb.h>
#include <devioctl.h>

#include "USBMassStorageDeviceHelper.h"

class CUSBMassStorageDeviceHelperWin32 : public IUSBMassStorageDeviceHelper
{
public:
  CUSBMassStorageDeviceHelperWin32();
  ~CUSBMassStorageDeviceHelperWin32();
  
  virtual PRBool Initialize(const nsAString &deviceName, const nsAString &deviceIdentifier);

  virtual const nsAString & GetDeviceVendor();
  virtual const nsAString & GetDeviceModel();
  virtual const nsAString & GetDeviceSerialNumber();

  virtual PRInt64 GetDeviceCapacity();

private:
  PRBool GetDeviceInformation();

  nsString m_DeviceName;
  nsString m_DeviceIdentifier;  
};

#endif // __USB_MASS_STORAGE_DEVICE_WIN32_H__

