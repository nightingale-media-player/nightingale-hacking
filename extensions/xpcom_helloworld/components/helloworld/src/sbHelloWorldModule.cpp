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

#include <mozilla/ModuleUtils.h>
#include "sbHelloWorld.h"
#include "sbHelloWorldCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbHelloWorld);
NS_DEFINE_NAMED_CID(SB_HELLOWORLD_CID);

static const mozilla::Module::CIDEntry kHelloWorldCIDs[] = {
  { &kSB_HELLOWORLD_CID, false, NULL, sbHelloWorldConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kHelloWorldContracts[] = {
  { SB_HELLOWORLD_CONTRACTID, &kSB_HELLOWORLD_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kHelloWorldCategories[] = {
  { NULL }
};

static const mozilla::Module kHelloWorldModule = {
  mozilla::Module::kVersion,
  kHelloWorldCIDs,
  kHelloWorldContracts,
  kHelloWorldCategories
};

NSMODULE_DEFN(sbHelloWorld) = &kHelloWorldModule;
