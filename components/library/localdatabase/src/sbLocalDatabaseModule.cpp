/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
#include "sbLocalDatabaseLibraryFactory.h"
#include "sbLocalDatabaseLibraryLoader.h"
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseSimpleMediaListFactory.h"
#include "sbLocalDatabaseTreeView.h"
#include "sbLocalDatabaseViewMediaListFactory.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseGUIDArray)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseLibraryFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLocalDatabaseLibraryLoader, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabasePropertyCache)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseSimpleMediaListFactory)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseTreeView)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseViewMediaListFactory)

static const nsModuleComponentInfo components[] =
{
	{
    "Local Database GUID Array",
    SB_LOCALDATABASE_GUIDARRAY_CID,
    SB_LOCALDATABASE_GUIDARRAY_CONTRACTID,
    sbLocalDatabaseGUIDArrayConstructor
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
    sbLocalDatabaseLibraryLoader::RegisterSelf
	},
	{
    "Local Database Property Cache",
    SB_LOCALDATABASE_PROPERTYCACHE_CID,
    SB_LOCALDATABASE_PROPERTYCACHE_CONTRACTID,
    sbLocalDatabasePropertyCacheConstructor
	},
	{
    SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_DESCRIPTION,
    SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CID,
    SB_LOCALDATABASE_SIMPLEMEDIALISTFACTORY_CONTRACTID,
    sbLocalDatabaseSimpleMediaListFactoryConstructor
	},
	{
    "Local Database TreeView",
    SB_LOCALDATABASE_TREEVIEW_CID,
    SB_LOCALDATABASE_TREEVIEW_CONTRACTID,
    sbLocalDatabaseTreeViewConstructor
	},
	{
    SB_LOCALDATABASE_VIEWMEDIALISTFACTORY_DESCRIPTION,
    SB_LOCALDATABASE_VIEWMEDIALISTFACTORY_CID,
    SB_LOCALDATABASE_VIEWMEDIALISTFACTORY_CONTRACTID,
    sbLocalDatabaseViewMediaListFactoryConstructor
	}
};

NS_IMPL_NSGETMODULE(Songbird Local Database Module, components)
