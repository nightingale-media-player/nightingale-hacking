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

// Local imports.
#include "sbMediaFileManager.h"

// Mozilla imports.
#include <mozilla/ModuleUtils.h>

// Songbird imports.
#include <sbStandardProperties.h>

// Construct and initialize the sbMediaFileManager object.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMediaFileManager);
NS_DEFINE_NAMED_CID(SB_MEDIAFILEMANAGER_CID);


static const mozilla::Module::CIDEntry kMediaManagerCIDs[] = {
  { &kSB_MEDIAFILEMANAGER_CID, false, NULL, sbMediaFileManagerConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kMediaManagerContracts[] = {
  { SB_MEDIAFILEMANAGER_CONTRACTID, &kSB_MEDIAFILEMANAGER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kMediaManagerCategories[] = {
  { NULL }
};

static const mozilla::Module kMediaManagerModule = {
  mozilla::Module::kVersion,
  kMediaManagerCIDs,
  kMediaManagerContracts,
  kMediaManagerCategories
};

NSMODULE_DEFN(sbMediaManagerComponents) = &kMediaManagerModule;
