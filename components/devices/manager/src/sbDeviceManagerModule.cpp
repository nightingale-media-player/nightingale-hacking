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

/** 
* \file  sbDeviceManagerModule.cpp
* \brief Songbird Device Manager Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "sbDeviceManager.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceManager);
NS_DEFINE_NAMED_CID(SONGBIRD_DEVICEMANAGER2_CID);

static const mozilla::Module::CIDEntry kDeviceManagerCIDs[] = {
  { &kSONGBIRD_DEVICEMANAGER2_CID, false, NULL, sbDeviceManagerConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDeviceManagerContracts[] = {
  { SONGBIRD_DEVICEMANAGER2_CONTRACTID, &kSONGBIRD_DEVICEMANAGER2_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kDeviceManagerCategories[] = {
  { "app-startup", SONGBIRD_DEVICEMANAGER2_CLASSNAME, SONGBIRD_DEVICEMANAGER2_CONTRACTID },
  { NULL }
};

static const mozilla::Module kDeviceManagerModule = {
  mozilla::Module::kVersion,
  kDeviceManagerCIDs,
  kDeviceManagerContracts,
  kDeviceManagerCategories
};

NSMODULE_DEFN(sbDeviceManagerComponents) = &kDeviceManagerModule;
