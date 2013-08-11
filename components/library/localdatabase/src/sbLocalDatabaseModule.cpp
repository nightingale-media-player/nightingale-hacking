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

NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseGUIDArray);
NS_DEFINE_NAMED_CID(SB_LOCALDATABASE_GUIDARRAY_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLocalDatabaseLibraryFactory, Init);
NS_DEFINE_NAMED_CID(SB_LOCALDATABASE_LIBRARYFACTORY_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseDiffingService);
NS_DEFINE_NAMED_CID(SB_LOCALDATABASE_DIFFINGSERVICE_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseLibraryLoader);
NS_DEFINE_NAMED_CID(SB_LOCALDATABASE_LIBRARYLOADER_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseSimpleMediaListFactory);
NS_DEFINE_NAMED_CID(SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLocalDatabaseAsyncGUIDArray, Init);
NS_DEFINE_NAMED_CID(SB_LOCALDATABASE_ASYNCGUIDARRAY_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseSmartMediaListFactory);
NS_DEFINE_NAMED_CID(SB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseDynamicMediaListFactory);
NS_DEFINE_NAMED_CID(SB_LOCALDATABASE_DYNAMICMEDIALISTFACTORY_CID);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseMediaListViewState);
NS_DEFINE_NAMED_CID(SB_LOCALDATABASE_MEDIALISTVIEWSTATE_CID);


static const mozilla::Module::CIDEntry kSongbirdLocalDatabaseLibraryCIDs[] = {
  { &kSB_LOCALDATABASE_GUIDARRAY_CID, false, NULL, sbLocalDatabaseGUIDArrayConstructor },
  { &kSB_LOCALDATABASE_LIBRARYFACTORY_CID, false, NULL, sbLocalDatabaseLibraryFactoryConstructor },
  { &kSB_LOCALDATABASE_DIFFINGSERVICE_CID, false, NULL, sbLocalDatabaseDiffingServiceConstructor },
  { &kSB_LOCALDATABASE_LIBRARYLOADER_CID, false, NULL, sbLocalDatabaseLibraryLoaderConstructor },
  { &kSB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CID, false, NULL, sbLocalDatabaseSimpleMediaListFactoryConstructor },
  { &kSB_LOCALDATABASE_ASYNCGUIDARRAY_CID, false, NULL, sbLocalDatabaseAsyncGUIDArrayConstructor },
  { &kSB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CID, false, NULL, sbLocalDatabaseSmartMediaListFactoryConstructor },
  { &kSB_LOCALDATABASE_DYNAMICMEDIALISTFACTORY_CID, false, NULL, sbLocalDatabaseDynamicMediaListFactoryConstructor },
  { &kSB_LOCALDATABASE_MEDIALISTVIEWSTATE_CID, false, NULL, sbLocalDatabaseMediaListViewStateConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSongbirdLocalDatabaseLibraryContracts[] = {
  { SB_LOCALDATABASE_GUIDARRAY_CONTRACTID, &kSB_LOCALDATABASE_GUIDARRAY_CID },
  { SB_LOCALDATABASE_ASYNCGUIDARRAY_CONTRACTID, &kSB_LOCALDATABASE_ASYNCGUIDARRAY_CID },
  { SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID, &kSB_LOCALDATABASE_LIBRARYFACTORY_CID },
  { SB_LOCALDATABASE_LIBRARYLOADER_CONTRACTID, &kSB_LOCALDATABASE_LIBRARYLOADER_CID },
  { SB_LOCALDATABASE_DIFFINGSERVICE_CONTRACTID, &kSB_LOCALDATABASE_DIFFINGSERVICE_CID },
  { SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CONTRACTID, &kSB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CID },
  { SB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CONTRACTID, &kSB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CID },
  { SB_LOCALDATABASE_DYNAMICMEDIALISTFACTORY_CONTRACTID, &kSB_LOCALDATABASE_DYNAMICMEDIALISTFACTORY_CID },
  { SB_LOCALDATABASE_MEDIALISTVIEWSTATE_CONTRACTID, &kSB_LOCALDATABASE_MEDIALISTVIEWSTATE_CID },
};

static const mozilla::Module::CategoryEntry kSongbirdLocalDatabaseLibraryCategories[] = {
  { "app-startup", SB_LOCALDATABASE_LIBRARYFACTORY_NAME, "service," SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID },
  { "songbird-library-loader", SB_LOCALDATABASE_LIBRARYLOADER_NAME, SB_LOCALDATABASE_LIBRARYLOADER_CONTRACTID },
  { NULL }
};

static const mozilla::Module kSongbirdLocalDatabaseLibraryModule = {
  mozilla::Module::kVersion,
  kSongbirdLocalDatabaseLibraryCIDs,
  kSongbirdLocalDatabaseLibraryContracts,
  kSongbirdLocalDatabaseLibraryCategories
};

NSMODULE_DEFN(sbLocalDatabaseLibrary) = &kSongbirdLocalDatabaseLibraryModule;
