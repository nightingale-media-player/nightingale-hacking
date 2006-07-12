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

/** 
* \file  USBMassStorageDeviceWin32.cpp
* \brief Songbird USBMassStorageDevice Win32 Implementation.
*/

#include "USBMassStorageDeviceWin32.h"

//-----------------------------------------------------------------------------
CUSBMassStorageDeviceHelperWin32::CUSBMassStorageDeviceHelperWin32()
{

} //ctor

//-----------------------------------------------------------------------------
CUSBMassStorageDeviceHelperWin32::~CUSBMassStorageDeviceHelperWin32()
{

} //dtor

//-----------------------------------------------------------------------------
PRBool CUSBMassStorageDeviceHelperWin32::Initialize(const nsAString &deviceName, const nsAString &deviceIdentifier)
{
  PRBool bCanHandleDevice = PR_FALSE;

  if(FindInReadable(NS_LITERAL_STRING("USBSTOR"), deviceName) == PR_TRUE)
  {
    bCanHandleDevice = PR_TRUE;
    m_DeviceName = deviceName;
    m_DeviceIdentifier = deviceIdentifier;
  }

  return bCanHandleDevice;
} //Initialize

//-----------------------------------------------------------------------------
const nsAString & CUSBMassStorageDeviceHelperWin32::GetDeviceVendor()
{
  return EmptyString();
} //GetDeviceVendor

//-----------------------------------------------------------------------------
const nsAString & CUSBMassStorageDeviceHelperWin32::GetDeviceModel()
{
  return EmptyString();
} //GetDeviceModel

//-----------------------------------------------------------------------------
const nsAString & CUSBMassStorageDeviceHelperWin32::GetDeviceSerialNumber()
{
  return EmptyString();
} //GetDeviceSerialNumber

//-----------------------------------------------------------------------------
PRInt64 CUSBMassStorageDeviceHelperWin32::GetDeviceCapacity()
{
  return -1;
} //GetDeviceCapacity

//-----------------------------------------------------------------------------
PRBool CUSBMassStorageDeviceHelperWin32::GetDeviceInformation()
{
  PRBool bRet = PR_FALSE;

  //UsbBuildGetDescriptorRequest();

  return bRet;
} //GetDeviceInformation