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

#ifndef __SBSQLSELECTBUILDER_H__
#define __SBSQLSELECTBUILDER_H__

#include <sbISQLBuilder.h>
#include "sbSQLWhereBuilder.h"
#include "sbSQLBuilderBase.h"

#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>

class sbSQLSelectBuilder : public sbSQLWhereBuilder,
                           public sbISQLSelectBuilder
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  // defined in sbSQLBuilderBase.h
  NS_FORWARD_SBISQLBUILDER_WITHOUT_TOSTRING_RESET(sbSQLBuilderBase::)
  NS_FORWARD_SBISQLWHEREBUILDER(sbSQLWhereBuilder::)
  NS_DECL_SBISQLSELECTBUILDER

  // override sbISQLBuilder::ToString and sbISQLBuilder::Reset
  NS_IMETHOD ToString(nsAString& _result);
  NS_IMETHOD Reset();

  sbSQLSelectBuilder();
  virtual ~sbSQLSelectBuilder();
private:
  struct sbOrderInfo
  {
    nsString tableName;
    nsString columnName;
    PRBool ascending;
  };

  struct sbGroupInfo
  {
    nsString tableName;
    nsString columnName;
  };

  nsTArray<sbGroupInfo> mGroups;
  nsTArray<sbOrderInfo> mOrders;
  nsTArray<sbColumnInfo> mOutputColumns;

  nsString mBaseTableName;
  nsString mBaseTableAlias;
  PRBool mIsDistinct;

};

#endif /* __SBSQLSELECTBUILDER_H__ */

