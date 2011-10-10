/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

#include "sbMockDeviceFirmwareHandler.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMockDeviceFirmwareHandler, Init);
SB_DEVICE_FIRMWARE_HANDLER_REGISTERSELF(sbMockDeviceFirmwareHandler);

static nsModuleComponentInfo sbDeviceFirmwareTesterComponents[] =
{
  {
    "Nightingale Device Firmware Tester - Mock Device Firmware Handler",
    { 0x5b100d2b, 0x486a, 0x4f3a, { 0x9b, 0x60, 0x14, 0x64, 0xb, 0xcb, 0xa6, 0x8e } },
    "@getnightingale.com/Nightingale/Device/Firmware/Handler/MockDevice;1",
    sbMockDeviceFirmwareHandlerConstructor,
    sbMockDeviceFirmwareHandlerRegisterSelf,
    sbMockDeviceFirmwareHandlerUnregisterSelf
  }
};

NS_IMPL_NSGETMODULE(NightingaleDeviceFirmwareTests, sbDeviceFirmwareTesterComponents)
