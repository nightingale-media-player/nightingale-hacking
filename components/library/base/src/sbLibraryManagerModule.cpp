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

/** 
 * \file  LibraryManagerComponent.cpp
 * \brief Songbird LibraryManager Component Factory and Main Entry Point.
 */

#include <nsIGenericFactory.h>
#include "sbLibraryCID.h"
#include "sbLibraryConstraints.h"
#include "sbLibraryManager.h"
#include "sbMediaListViewMap.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLibraryManager, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMediaListViewMap, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLibrarySort);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLibraryConstraintBuilder);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbLibraryConstraint);

static nsModuleComponentInfo components[] =
{
  {
    SONGBIRD_LIBRARYMANAGER_CLASSNAME,
    SONGBIRD_LIBRARYMANAGER_CID,
    SONGBIRD_LIBRARYMANAGER_CONTRACTID,
    sbLibraryManagerConstructor,
    sbLibraryManager::RegisterSelf
  },
  {
    SONGBIRD_MEDIALISTVIEWMAP_CLASSNAME,
    SONGBIRD_MEDIALISTVIEWMAP_CID,
    SONGBIRD_MEDIALISTVIEWMAP_CONTRACTID,
    sbMediaListViewMapConstructor,
    sbMediaListViewMap::RegisterSelf
  },
  {
    SONGBIRD_LIBRARYSORT_CLASSNAME,
    SONGBIRD_LIBRARYSORT_CID,
    SONGBIRD_LIBRARYSORT_CONTRACTID,
    sbLibrarySortConstructor
  },
  {
    SONGBIRD_LIBRARY_CONSTRAINTBUILDER_CLASSNAME,
    SONGBIRD_LIBRARY_CONSTRAINTBUILDER_CID,
    SONGBIRD_LIBRARY_CONSTRAINTBUILDER_CONTRACTID,
    sbLibraryConstraintBuilderConstructor
  },
  {
    SONGBIRD_LIBRARY_CONSTRAINT_CLASSNAME,
    SONGBIRD_LIBRARY_CONSTRAINT_CID,
    SONGBIRD_LIBRARY_CONSTRAINT_CONTRACTID,
    sbLibraryConstraintConstructor
  }
};

NS_IMPL_NSGETMODULE(Songbird Library Manager, components)
