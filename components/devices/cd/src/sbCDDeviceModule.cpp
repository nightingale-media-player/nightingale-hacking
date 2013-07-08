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
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>

#include "sbCDDeviceMarshall.h"
#include "sbCDDeviceController.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbCDDeviceMarshall, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbCDDeviceController);

NS_DEFINE_NAMED_CID(SB_CDDEVICE_MARSHALL_CID);
NS_DEFINE_NAMED_CID(SB_CDDEVICE_CONTROLLER_CID);

static const mozilla::Module::CIDEntry kDeviceMarshallCIDs[] = {
  { &kSB_CDDEVICE_MARSHALL_CID, false, NULL, sbCDDeviceMarshallConstructor },
  { &kSB_CDDEVICE_CONTROLLER_CID, false, NULL, sbCDDeviceControllerConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDeviceMarshallContracts[] = {
  { SB_CDDEVICE_MARSHALL_CONTRACTID, &kSB_CDDEVICE_MARSHALL_CID },
  { SB_CDDEVICE_CONTROLLER_CONTRACTID, &kSB_CDDEVICE_CONTROLLER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kDeviceMarshallCategories[] = {
  // { "app-startup", SB_CDDEVICE_MARSHALL_CLASSNAME, SB_CDDEVICE_MARSHALL_CONTRACTID },
  { "app-startup", SB_CDDEVICE_CONTROLLER_CLASSNAME, SB_CDDEVICE_CONTROLLER_CONTRACTID },
  { NULL }
};

static const mozilla::Module kDeviceMarshallModule = {
  mozilla::Module::kVersion,
  kDeviceMarshallCIDs,
  kDeviceMarshallContracts,
  kDeviceMarshallCategories
};

NSMODULE_DEFN(sbDeviceMarshallComponents) = &kDeviceMarshallModule;
