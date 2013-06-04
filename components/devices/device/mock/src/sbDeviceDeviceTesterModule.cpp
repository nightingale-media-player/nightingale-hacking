/* vim: set sw=2 :miv */
/*
 * BEGIN NIGHTINGALE GPL
 * 
 * This file is part of the Nightingale Media Player.
 *
 * Copyright(c) 2013
 * http://getnightingale.com
 * 
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 * 
 * Software distributed under the License is distributed 
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
 * express or implied. See the GPL for the specific language 
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this 
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc., 
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 * 
 * END NIGHTINGALE GPL
 */

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "sbDeviceDeviceTesterUtils.h"
#include "sbMockDevice.h"


#define SB_DEVICEDEVICETESTERUTILS_CID                                        \
{ 0x524f2420, 0xf979, 0x44ee,                                                 \
  { 0xbc, 0x77, 0xba, 0x4e, 0x11, 0xae, 0xfc, 0x62 } }
#define SB_DEVICEDEVICETESTERUTILS_CLASSNAME                                  \
  "Songbird Device Device Tester - Utils"
#define SB_DEVICEDEVICETESTERUTILS_CONTRACTID                                 \
  "@songbirdnest.com/Songbird/Device/DeviceTester/Utils;1"


#define SB_DEVICEDEVICETESTERMOCK_CID                                         \
{ 0x3f023adc, 0x11ef, 0x4f63,                                                 \
  { 0xa9, 0x69, 0xed, 0x55, 0x4d, 0xc9, 0x73, 0x6a } }
#define SB_DEVICEDEVICETESTERMOCK_CLASSNAME                                   \
  "Songbird Device Device Tester - Mock Device"
#define SB_DEVICEDEVICETESTERMOCK_CONTRACTID                                  \
  "@songbirdnest.com/Songbird/Device/DeviceTester/MockDevice;1"


NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceDeviceTesterUtils);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMockDevice);

NS_DEFINE_NAMED_CID(SB_DEVICEDEVICETESTERUTILS_CID);
NS_DEFINE_NAMED_CID(SB_DEVICEDEVICETESTERMOCK_CID);

static const mozilla::Module::CIDEntry kDeviceDeviceTestsCIDs[] = {
  { &kSB_DEVICEDEVICETESTERUTILS_CID, false, NULL, sbDeviceDeviceTesterUtilsConstructor },
  { &kSB_DEVICEDEVICETESTERMOCK_CID, false, NULL, sbMockDeviceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDeviceDeviceTestsContracts[] = {
  { SB_DEVICEDEVICETESTERUTILS_CONTRACTID, &kSB_DEVICEDEVICETESTERUTILS_CID },
  { SB_DEVICEDEVICETESTERMOCK_CONTRACTID, &kSB_DEVICEDEVICETESTERMOCK_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kDeviceDeviceTestsCategories[] = {
  { NULL }
};

static const mozilla::Module kDeviceDeviceTestsModule = {
  mozilla::Module::kVersion,
  kDeviceDeviceTestsCIDs,
  kDeviceDeviceTestsContracts,
  kDeviceDeviceTestsCategories
};

NSMODULE_DEFN(sbDeviceDeviceTests) = &kDeviceDeviceTestsModule;
