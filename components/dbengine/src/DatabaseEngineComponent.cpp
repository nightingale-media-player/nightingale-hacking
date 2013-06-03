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

/** 
* \file  DatabaseEngineComponent.cpp
* \brief Songbird Database Engine Component Factory and Main Entry Point.
*/

#include <mozilla/ModuleUtils.h>

#include "DatabaseQuery.h"
#include "DatabaseResult.h"
#include "DatabaseEngine.h"

NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(CDatabaseEngine, CDatabaseEngine::GetSingleton);

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(CDatabaseQuery, Init);
NS_GENERIC_FACTORY_CONSTRUCTOR(CDatabaseResult);

NS_DEFINE_NAMED_CID(SONGBIRD_DATABASEENGINE_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_DATABASEQUERY_CID);
NS_DEFINE_NAMED_CID(SONGBIRD_DATABASERESULT_CID);

static const mozilla::Module::CIDEntry kDatabaseEngineCIDs[] = {
  { &kSONGBIRD_DATABASEENGINE_CID, false, NULL, CDatabaseEngineConstructor },
  { &kSONGBIRD_DATABASEQUERY_CID, false, NULL, CDatabaseQueryConstructor },
  { &kSONGBIRD_DATABASERESULT_CID, false, NULL, CDatabaseResultConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kDatabaseEngineContracts[] = {
  { SONGBIRD_DATABASEENGINE_CONTRACTID, &kSONGBIRD_DATABASEENGINE_CID },
  { SONGBIRD_DATABASEQUERY_CONTRACTID, &kSONGBIRD_DATABASEQUERY_CID },
  { SONGBIRD_DATABASERESULT_CONTRACTID, &kSONGBIRD_DATABASERESULT_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kDatabaseEngineCategories[] = {
  { NULL }
};

static const mozilla::Module kDatabaseEngineModule = {
  mozilla::Module::kVersion,
  kDatabaseEngineCIDs,
  kDatabaseEngineContracts,
  kDatabaseEngineCategories
};

NSMODULE_DEFN(sbDatabaseEngine) = &kDatabaseEngineModule;
