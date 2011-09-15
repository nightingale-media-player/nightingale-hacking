/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

#include "sbDeviceDeviceTesterUtils.h"
#include "sbMockDevice.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceDeviceTesterUtils);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMockDevice);

static nsModuleComponentInfo sbDeviceDeviceTesterComponents[] =
{
  {
    "Songbird Device Device Tester - Utils",
      { 0x524f2420, 0xf979, 0x44ee,
        { 0xbc, 0x77, 0xba, 0x4e, 0x11, 0xae, 0xfc, 0x62 } },
    "@songbirdnest.com/Songbird/Device/DeviceTester/Utils;1",
    sbDeviceDeviceTesterUtilsConstructor
  },
  {
    "Songbird Device Device Tester - Mock Device",
      { 0x3f023adc, 0x11ef, 0x4f63,
        { 0xa9, 0x69, 0xed, 0x55, 0x4d, 0xc9, 0x73, 0x6a } },
    "@songbirdnest.com/Songbird/Device/DeviceTester/MockDevice;1",
    sbMockDeviceConstructor
  }
};

NS_IMPL_NSGETMODULE(SongbirdDeviceDeviceTests, sbDeviceDeviceTesterComponents)
