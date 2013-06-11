/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbDirectoryProvider.h"

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <mozilla/ModuleUtils.h>
#include <nsICategoryManager.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDirectoryProvider, Init);
NS_DEFINE_NAMED_CID(SONGBIRD_DIRECTORY_PROVIDER_CID);

static const mozilla::Module::CIDEntry kDirectoryProviderCIDs[] = {
  { &kSONGBIRD_DIRECTORY_PROVIDER_CID, false, NULL, sbDirectoryProviderConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDirectoryProviderContracts[] = {
  { SONGBIRD_DIRECTORY_PROVIDER_CONTRACTID, &kSONGBIRD_DIRECTORY_PROVIDER_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kDirectoryProviderCategories[] = {
  { "profile-after-change", SONGBIRD_DIRECTORY_PROVIDER_CLASSNAME, SONGBIRD_DIRECTORY_PROVIDER_CONTRACTID },
  { NULL }
};

static const mozilla::Module kDirectoryProviderModule = {
  mozilla::Module::kVersion,
  kDirectoryProviderCIDs,
  kDirectoryProviderContracts,
  kDirectoryProviderCategories
};

NSMODULE_DEFN(sbDirectoryProviderModule) = &kDirectoryProviderModule;
