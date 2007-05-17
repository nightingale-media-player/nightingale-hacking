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

/** 
* \file  DownloadDevice.h
* \brief Songbird DownloadDevice Component Definition.
*/

#ifndef __DOWNLOAD_DEVICE_H__
#define __DOWNLOAD_DEVICE_H__

#include "sbIDownloadDevice.h"
#include "DeviceBase.h"

// DEFINES ====================================================================
#define SONGBIRD_DownloadDevice_CONTRACTID                \
  "@songbirdnest.com/Songbird/Device/DownloadDevice;1"
#define SONGBIRD_DownloadDevice_CLASSNAME                 \
  "Songbird Download Device"
#define SONGBIRD_DownloadDevice_CID                       \
{ /* 961da3f4-5ef1-4ad0-818d-622c7bd17447 */              \
  0x961da3f4,                                             \
  0x5ef1,                                                 \
  0x4ad0,                                                 \
  {0x81, 0x8d, 0x62, 0x2c, 0x7b, 0xd1, 0x74, 0x47}        \
}

// CLASSES ====================================================================

class sbDownloadDevice;

// Since download device has only one instance, the "Device String" notion does not
// apply to this device and hence ignored in all the functions.
class sbDownloadDevice :  public sbIDownloadDevice, public sbDeviceBase
{

public:

  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEBASE
  NS_DECL_SBIDOWNLOADDEVICE

};

#endif // __DOWNLOAD_DEVICE_H__
