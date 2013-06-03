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

#include <mozilla/ModuleUtils.h>
#include "sbSQLBuilderBase.h"
#include "sbSQLSelectBuilder.h"
#include "sbSQLInsertBuilder.h"
#include "sbSQLUpdateBuilder.h"
#include "sbSQLDeleteBuilder.h"
#include "sbSQLBuilderCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbSQLSelectBuilder);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbSQLInsertBuilder);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbSQLUpdateBuilder);
NS_GENERIC_FACTORY_CONSTRUCTOR(sbSQLDeleteBuilder);

NS_DEFINE_NAMED_CID(SB_SQLBUILDER_SELECT_CID);
NS_DEFINE_NAMED_CID(SB_SQLBUILDER_INSERT_CID);
NS_DEFINE_NAMED_CID(SB_SQLBUILDER_DELETE_CID);
NS_DEFINE_NAMED_CID(SB_SQLBUILDER_UPDATE_CID);

static const mozilla::Module::CIDEntry kSQLStatementBuilderCIDs[] = {
  { &kSB_SQLBUILDER_SELECT_CID, false, NULL, sbSQLSelectBuilderConstructor }, 
  { &kSB_SQLBUILDER_INSERT_CID, false, NULL, sbSQLInsertBuilderConstructor }, 
  { &kSB_SQLBUILDER_UPDATE_CID, false, NULL, sbSQLUpdateBuilderConstructor }, 
  { &kSB_SQLBUILDER_DELETE_CID, false, NULL, sbSQLDeleteBuilderConstructor }, 
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSQLStatementBuilderContracts[] = {
  { SB_SQLBUILDER_SELECT_CONTRACTID, &kSB_SQLBUILDER_SELECT_CID },
  { SB_SQLBUILDER_INSERT_CONTRACTID, &kSB_SQLBUILDER_INSERT_CID },
  { SB_SQLBUILDER_UPDATE_CONTRACTID, &kSB_SQLBUILDER_UPDATE_CID },
  { SB_SQLBUILDER_DELETE_CONTRACTID, &kSB_SQLBUILDER_DELETE_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kSQLStatementBuilderCategories[] = {
  { NULL }
};

static const mozilla::Module kSQLStatementBuilderModule = {
  mozilla::Module::kVersion,
  kSQLStatementBuilderCIDs,
  kSQLStatementBuilderContracts,
  kSQLStatementBuilderCategories
};

NSMODULE_DEFN(sbSQLStatementBuilder) = &kSQLStatementBuilderModule;
