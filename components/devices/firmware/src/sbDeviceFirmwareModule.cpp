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

/** 
* \file  sbDeviceFirmwareModule.cpp
* \brief Songbird Device Firmware Updater Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "sbDeviceFirmwareSupport.h"
#include "sbDeviceFirmwareUpdate.h"
#include "sbDeviceFirmwareUpdater.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceFirmwareSupport);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceFirmwareUpdate);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDeviceFirmwareUpdater, Init);

NS_DEFINE_NAMED_CID(SB_DEVICEFIRMWARESUPPORT_CID);
NS_DEFINE_NAMED_CID(SB_DEVICEFIRMWAREUPDATE_CID);
NS_DEFINE_NAMED_CID(SB_DEVICEFIRMWAREUPDATER_CID);


static const mozilla::Module::CIDEntry kDeviceFirmwareSupportCIDs[] = {
  { &kSB_DEVICEFIRMWARESUPPORT_CID, false, NULL, sbDeviceFirmwareSupportConstructor },
  { &kSB_DEVICEFIRMWAREUPDATE_CID, false, NULL, sbDeviceFirmwareUpdateConstructor },
  { &kSB_DEVICEFIRMWAREUPDATER_CID, false, NULL, sbDeviceFirmwareUpdaterConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDeviceFirmwareSupportContracts[] = {
  { SB_DEVICEFIRMWARESUPPORT_CONTRACTID, &kSB_DEVICEFIRMWARESUPPORT_CID },
  { SB_DEVICEFIRMWAREUPDATE_CONTRACTID, &kSB_DEVICEFIRMWAREUPDATE_CID },
  { SB_DEVICEFIRMWAREUPDATER_CONTRACTID, &kSB_DEVICEFIRMWAREUPDATER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kDeviceFirmwareSupportCategories[] = {
  { "app-startup", SB_DEVICEFIRMWAREUPDATER_CLASSNAME, SB_DEVICEFIRMWAREUPDATER_CONTRACTID },
  { NULL }
};

static const mozilla::Module kDeviceFirmwareSupportModule = {
  mozilla::Module::kVersion,
  kDeviceFirmwareSupportCIDs,
  kDeviceFirmwareSupportContracts,
  kDeviceFirmwareSupportCategories
};

NSMODULE_DEFN(sbDeviceManagerComponents) = &kDeviceFirmwareSupportModule;
