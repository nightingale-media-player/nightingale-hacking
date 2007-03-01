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

#include "nsIGenericFactory.h"
#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseTreeView.h"
#include "sbLocalDatabaseCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseGUIDArray)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabasePropertyCache)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLocalDatabaseTreeView)

static const nsModuleComponentInfo components[] =
{
	{
    "Local Database GUID Array",
    SB_LOCALDATABASE_GUIDARRAY_CID,
    SB_LOCALDATABASE_GUIDARRAY_CONTRACTID,
    sbLocalDatabaseGUIDArrayConstructor
	},
	{
    "Local Database Property Cache",
    SB_LOCALDATABASE_PROPERTYCACHE_CID,
    SB_LOCALDATABASE_PROPERTYCACHE_CONTRACTID,
    sbLocalDatabasePropertyCacheConstructor
	},
	{
    "Local Database TreeView",
    SB_LOCALDATABASE_TREEVIEW_CID,
    SB_LOCALDATABASE_TREEVIEW_CONTRACTID,
    sbLocalDatabaseTreeViewConstructor
	}
};

NS_IMPL_NSGETMODULE("Songbird Local Database Module", components)

