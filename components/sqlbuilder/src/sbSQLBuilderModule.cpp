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
#include "sbSQLBuilderBase.h"
#include "sbSQLSelectBuilder.h"
#include "sbSQLInsertBuilder.h"
#include "sbSQLUpdateBuilder.h"
#include "sbSQLBuilderCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbSQLSelectBuilder)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbSQLInsertBuilder)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbSQLUpdateBuilder)

static const nsModuleComponentInfo components[] =
{
  {
    "SQLBuilder for SELECT statements",
    SB_SQLBUILDER_SELECT_CID,
    SB_SQLBUILDER_SELECT_CONTRACTID,
    sbSQLSelectBuilderConstructor
  },
  {
    "SQLBuilder for INSERT statements",
    SB_SQLBUILDER_INSERT_CID,
    SB_SQLBUILDER_INSERT_CONTRACTID,
    sbSQLInsertBuilderConstructor
  },
  {
    "SQLBuilder for UPDATE statements",
    SB_SQLBUILDER_UPDATE_CID,
    SB_SQLBUILDER_UPDATE_CONTRACTID,
    sbSQLUpdateBuilderConstructor
  }
};

NS_IMPL_NSGETMODULE("Songbird SQL Statement Builder Module", components)

