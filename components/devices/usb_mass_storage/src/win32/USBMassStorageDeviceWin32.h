/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
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

#pragma once

/** 
 * \file  USBMassStorageDeviceWin32.h
 * \brief Songbird USBMassStorageDevice Win32 Definition.
 */

#include "nspr.h"

#include <string/nsStringAPI.h>
#include "nsString.h"

#include <windows.h>
///#include <ntddstor.h>

class IUSBMassStorageDeviceHelper
{
public:
  IUSBMassStorageDeviceHelper() {};
  ~IUSBMassStorageDeviceHelper() {};

  virtual PRBool Initialize(const PRUnichar *deviceName, const PRUnichar *deviceIdentifier) = 0;

  virtual PRUnichar * GetDeviceVendor() = 0;
  virtual PRUnichar * GetDeviceModel() = 0;
  virtual PRUnichar * GetDeviceSerialNumber() = 0;

  virtual PRInt64 GetDeviceCapacity() = 0;
};

class CUSBMassStorageDeviceHelperStub : public IUSBMassStorageDeviceHelper
{
public:
  CUSBMassStorageDeviceHelperStub() {};
  ~CUSBMassStorageDeviceHelperStub() {};

  virtual PRBool Initialize(const PRUnichar *deviceName, const PRUnichar *deviceIdentifier){ return PR_FALSE; };

  virtual PRUnichar * GetDeviceVendor() { return NS_L("__STUB__"); };
  virtual PRUnichar * GetDeviceModel() { return NS_L("__STUB__"); };
  virtual PRUnichar * GetDeviceSerialNumber() { return NS_L("__STUB__"); };

  virtual PRInt64 GetDeviceCapacity() { return -1; };
};

class CUSBMassStorageDeviceHelperWin32 : public IUSBMassStorageDeviceHelper
{
public:
  CUSBMassStorageDeviceHelperWin32();
  ~CUSBMassStorageDeviceHelperWin32();
  
  virtual PRBool Initialize(const PRUnichar *deviceName, const PRUnichar *deviceIdentifier);

  virtual PRUnichar * GetDeviceVendor();
  virtual PRUnichar * GetDeviceModel();
  virtual PRUnichar * GetDeviceSerialNumber();

  virtual PRInt64 GetDeviceCapacity();
};