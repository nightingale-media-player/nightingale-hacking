/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include <nsIGenericFactory.h>

#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabaseAsyncGUIDArray.h"
#include "sbLocalDatabaseLibraryFactory.h"
#include "sbLocalDatabaseLibraryLoader.h"
#include "sbLocalDatabaseSimpleMediaListFactory.h"
#include "sbLocalDatabaseSmartMediaListFactory.h"
#include "sbLocalDatabaseMediaListViewState.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseGUIDArray)
NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(sbLocalDatabaseLibraryFactory,
                                         sbLocalDatabaseLibraryFactory::GetInstance)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseLibraryLoader)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseSimpleMediaListFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLocalDatabaseAsyncGUIDArray, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseSmartMediaListFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseMediaListViewState)

SB_LIBRARY_LOADER_REGISTRATION(sbLocalDatabaseLibraryLoader,
                               SB_LOCALDATABASE_LIBRARYLOADER_DESCRIPTION)

static const nsModuleComponentInfo components[] =
{
  {
    SB_LOCALDATABASE_GUIDARRAY_DESCRIPTION,
    SB_LOCALDATABASE_GUIDARRAY_CID,
    SB_LOCALDATABASE_GUIDARRAY_CONTRACTID,
    sbLocalDatabaseGUIDArrayConstructor
  },
  {
    SB_LOCALDATABASE_ASYNCGUIDARRAY_DESCRIPTION,
    SB_LOCALDATABASE_ASYNCGUIDARRAY_CID,
    SB_LOCALDATABASE_ASYNCGUIDARRAY_CONTRACTID,
    sbLocalDatabaseAsyncGUIDArrayConstructor
  },
  {
    SB_LOCALDATABASE_LIBRARYFACTORY_DESCRIPTION,
    SB_LOCALDATABASE_LIBRARYFACTORY_CID,
    SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID,
    sbLocalDatabaseLibraryFactoryConstructor
  },
  {
    SB_LOCALDATABASE_LIBRARYLOADER_DESCRIPTION,
    SB_LOCALDATABASE_LIBRARYLOADER_CID,
    SB_LOCALDATABASE_LIBRARYLOADER_CONTRACTID,
    sbLocalDatabaseLibraryLoaderConstructor,
    sbLocalDatabaseLibraryLoaderRegisterSelf,
    sbLocalDatabaseLibraryLoaderUnregisterSelf
  },
  {
    SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_DESCRIPTION,
    SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CID,
    SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CONTRACTID,
    sbLocalDatabaseSimpleMediaListFactoryConstructor
  },
  {
    SB_LOCALDATABASE_SMARTMEDIALISTFACTORY_DESCRIPTION,
    SB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CID,
    SB_LOCALDATABASE_SMARTMEDIALISTFACTORY_CONTRACTID,
    sbLocalDatabaseSmartMediaListFactoryConstructor
  },
  {
    SB_LOCALDATABASE_MEDIALISTVIEWSTATE_DESCRIPTION,
    SB_LOCALDATABASE_MEDIALISTVIEWSTATE_CID,
    SB_LOCALDATABASE_MEDIALISTVIEWSTATE_CONTRACTID,
    sbLocalDatabaseMediaListViewStateConstructor
  }
};

NS_IMPL_NSGETMODULE(SongbirdLocalDatabaseLibraryModule, components)
