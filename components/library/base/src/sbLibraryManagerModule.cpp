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
#include "sbLibraryManager.h"

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbLibraryManager, Init);

static nsModuleComponentInfo components[] =
{
  {
    SONGBIRD_LIBRARYMANAGER_CLASSNAME,
    SONGBIRD_LIBRARYMANAGER_CID,
    SONGBIRD_LIBRARYMANAGER_CONTRACTID,
    sbLibraryManagerConstructor
  }
};

NS_IMPL_NSGETMODULE(SONGBIRD_LIBRARYMANAGER_DESCRIPTION, components)
