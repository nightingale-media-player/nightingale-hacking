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

/** 
* \file  DeviceManagerComponent.cpp
* \brief Songbird DeviceManager Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "DeviceManager.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDeviceManagerObsolete, Initialize);

NS_DEFINE_NAMED_CID(SONGBIRD_OBSOLETE_DEVICEMANAGER_CID);

static const mozilla::Module::CIDEntry kDeviceManagerObsoleteCIDs[] = {
  { &kSONGBIRD_OBSOLETE_DEVICEMANAGER_CID, false, NULL, sbDeviceManagerObsoleteConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDeviceManagerObsoleteContracts[] = {
  { SONGBIRD_OBSOLETE_DEVICEMANAGER_CONTRACTID, &kSONGBIRD_OBSOLETE_DEVICEMANAGER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kDeviceManagerObsoleteCategories[] = {
  { "app-startup", SONGBIRD_OBSOLETE_DEVICEMANAGER_CLASSNAME, SONGBIRD_OBSOLETE_DEVICEMANAGER_CONTRACTID },
  { NULL }
};

static const mozilla::Module kDeviceManagerObsoleteModule = {
  mozilla::Module::kVersion,
  kDeviceManagerObsoleteCIDs,
  kDeviceManagerObsoleteContracts,
  kDeviceManagerObsoleteCategories
};

NSMODULE_DEFN(sbDeviceManagerObsoleteComponents) = &kDeviceManagerObsoleteModule;
