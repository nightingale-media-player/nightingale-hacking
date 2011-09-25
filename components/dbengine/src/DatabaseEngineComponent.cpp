/*
 //
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

/** 
* \file  DatabaseEngineComponent.cpp
* \brief Nightingale Database Engine Component Factory and Main Entry Point.
*/

#include "nsIGenericFactory.h"

#include "DatabaseQuery.h"
#include "DatabaseResult.h"
#include "DatabaseEngine.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(CDatabaseEngine, CDatabaseEngine::GetSingleton)

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(CDatabaseQuery, Init)
NS_GENERIC_FACTORY_CONSTRUCTOR(CDatabaseResult)

static nsModuleComponentInfo sbDatabaseEngine[] =
{
  {
    NIGHTINGALE_DATABASEENGINE_CLASSNAME,
    NIGHTINGALE_DATABASEENGINE_CID,
    NIGHTINGALE_DATABASEENGINE_CONTRACTID,
    CDatabaseEngineConstructor
  },

  {
    NIGHTINGALE_DATABASEQUERY_CLASSNAME,
    NIGHTINGALE_DATABASEQUERY_CID,
    NIGHTINGALE_DATABASEQUERY_CONTRACTID,
    CDatabaseQueryConstructor
  },

  {
    NIGHTINGALE_DATABASERESULT_CLASSNAME,
    NIGHTINGALE_DATABASERESULT_CID,
    NIGHTINGALE_DATABASERESULT_CONTRACTID,
    CDatabaseResultConstructor
  }
};

// When everything else shuts down, delete it.
static void sbDatabaseEngineDestructor(nsIModule* me)
{
  NS_IF_RELEASE(gEngine);
  gEngine = nsnull;
}

NS_IMPL_NSGETMODULE_WITH_DTOR(NightingaleDatabaseEngineComponent, sbDatabaseEngine, sbDatabaseEngineDestructor)
