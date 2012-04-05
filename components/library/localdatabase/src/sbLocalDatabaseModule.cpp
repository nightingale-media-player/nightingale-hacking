/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://songbirdnest.com
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
// END SONGBIRD GPL
//
*/

#include <nsIClassInfoImpl.h>
#include <mozilla/ModuleUtils.h>

#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabaseAsyncGUIDArray.h"
#include "sbLocalDatabaseDiffingService.h"
#include "sbLocalDatabaseLibraryFactory.h"
#include "sbLocalDatabaseLibraryLoader.h"
#include "sbLocalDatabaseSimpleMediaListFactory.h"
#include "sbLocalDatabaseSmartMediaListFactory.h"
#include "sbLocalDatabaseDynamicMediaListFactory.h"
#include "sbLocalDatabaseMediaListViewState.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseGUIDArray)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLocalDatabaseLibraryFactory, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseDiffingService)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseLibraryLoader)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseSimpleMediaListFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLocalDatabaseAsyncGUIDArray, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseSmartMediaListFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseDynamicMediaListFactory);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseMediaListViewState)

static const mozilla::Module::CIDEntry kLocalDatabaseCIDs[] =
{
  { &kSB_LOCALDATABASE_GUIDARRAY_CID, false, NULL,
    sbLocalDatabaseGUIDArrayConstructor },
  { &kSB_LOCALDATABASE_ASYNCGUIDARRAY_CID, false, NULL,
    sbLocalDatabaseAsyncGUIDArray },
  { &kSB_LOCALDATABASE_LIBRARYFACTORY_CID, false, NULL,
    sbLocalDatabaseLibraryFactoryConstructor },
  { &kSB_LOCALDATABASE_LIBRARYLOADER_CID, false, NULL,
    sbLocalDatabaseLibraryLoaderConstructor },
  { &kSB_LOCALDATABASE_DIFFINGSERVICE_CID, false, NULL,
    sbLocalDatabaseDiffingServiceConstructor },
  { &kSB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CID, false, NULL,
    sbLocalDatabaseSimpleMediaListFactoryConstructor },
  { &kSB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CID, false, NULL,
    sbLocalDatabaseSmartMediaListFactoryConstructor },
  { &kSB_LOCALDATABASE_DYNAMICMEDIALISTFACTORY_CID, false, NULL,
    sbLocalDatabaseDynanmicMediaListFactoryConstructor },
  { &kSB_LOCALDATABASE_MEDIALISTVIEWSTATE_CID, false, NULL,
    sbLocalDatabaseMediaListViewStateConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kLocalDatabaseContracts[] =
{
  { SB_LOCALDATABASE_GUIDARRAY_CID,
    &kSB_LOCALDATABASE_GUIDARRAY_CONTRACTID },
  { SB_LOCALDATABASE_ASYNCGUIDARRAY_CID,
    &kSB_LOCALDATABASE_ASYNCGUIDARRAY_CONTRACTID },
  { SB_LOCALDATABASE_LIBRARYFACTORY_CID,
    &kSB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID },
  { SB_LOCALDATABASE_LIBRARYLOADER_CID,
    &kSB_LOCALDATABASE_LIBRARYLOADER_CONTRACTID },
  { SB_LOCALDATABASE_DIFFINGSERVICE_CID,
    &kSB_LOCALDATABASE_DIFFINGSERVICE_CONTRACTID },
  { SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CID,
    &kSB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CONTRACTID },
  { SB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CID,
    &kSB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CONTRACTID },
  { SB_LOCALDATABASE_DYNAMICMEDIALISTFACTORY_CID,
    &kSB_LOCALDATABASE_DYNAMICMEDIALISTFACTORY_CONTRACTID },
  { SB_LOCALDATABASE_MEDIALISTVIEWSTATE_CID,
    &kSB_LOCALDATABASE_MEDIALISTVIEWSTATE_CONTRACTID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kLocalDatabaseCategories[] =
{
  { "profile-after-change" SB_LOCALDATABASE_DIFFINGSERVICE_CONTRACTID },
  { "profile-after-change" SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID },
  { NULL }
};

static const mozilla::Module kLocalDatabaseLibraryModule =
{
  mozilla::Module::kVersion,
  kLocalDatabaseCIDs,
  kLocalDatabaseContracts,
  kLocalDatabaseCategories
};

NSMODULE_DEFN(SongbirdLocalDatabaseLibraryModule) =
  &kLocalDatabaseLibraryModule;
