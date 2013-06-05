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
#include "sbSongkickDBService.h"


NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbSongkickDBService, Init);

NS_DEFINE_NAMED_CID(SONGBIRD_SONGKICKDBSERVICE_CID);

static const mozilla::Module::CIDEntry kSongkickDBServiceCIDs[] = {
  { &kSONGBIRD_SONGKICKDBSERVICE_CID, false, NULL, sbSongkickDBServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSongkickDBServiceContracts[] = {
  { SONGBIRD_SONGKICKDBSERVICE_CONTRACTID, &kSONGBIRD_SONGKICKDBSERVICE_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kSongkickDBServiceCategories[] = {
  { "app-startup", SONGBIRD_SONGKICKDBSERVICE_CONTRACTID },
  { NULL }
};

static const mozilla::Module kSongkickDBServiceModule = {
  mozilla::Module::kVersion,
  kSongkickDBServiceCIDs,
  kSongkickDBServiceContracts,
  kSongkickDBServiceCategories
};

NSMODULE_DEFN(sbSongkickComponent) = &kSongkickDBServiceModule;
