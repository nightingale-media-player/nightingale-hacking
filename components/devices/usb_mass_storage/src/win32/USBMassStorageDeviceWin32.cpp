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
PRBool CUSBMassStorageDeviceHelperWin32::Initialize(const PRUnichar *deviceName, const PRUnichar *deviceIdentifier)
{
  PRBool bCanHandleDevice = PR_FALSE;

  nsString strDeviceName(deviceName);
  nsString strDeviceIdentifier(deviceIdentifier);

  if(FindInReadable(NS_LITERAL_STRING("USBSTOR"), strDeviceName) == PR_TRUE)
  {
    bCanHandleDevice = PR_TRUE;
    m_DeviceName = deviceName;
    m_DeviceIdentifier = deviceIdentifier;
  }

  return bCanHandleDevice;
} //Initialize

//-----------------------------------------------------------------------------
const PRUnichar * CUSBMassStorageDeviceHelperWin32::GetDeviceVendor()
{
  return NS_L("");
} //GetDeviceVendor

//-----------------------------------------------------------------------------
const PRUnichar * CUSBMassStorageDeviceHelperWin32::GetDeviceModel()
{
  return NS_L("");
} //GetDeviceModel

//-----------------------------------------------------------------------------
const PRUnichar * CUSBMassStorageDeviceHelperWin32::GetDeviceSerialNumber()
{
  return NS_L("");
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