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

#include "sbSQLSelectBuilder.h"
#include "sbSQLBuilderCriterion.h"

NS_IMPL_ISUPPORTS_INHERITED1(sbSQLSelectBuilder,
                             sbSQLBuilderBase,
                             sbISQLSelectBuilder)

sbSQLSelectBuilder::sbSQLSelectBuilder() :
  mIsDistinct(PR_FALSE)
{
}

NS_IMETHODIMP
sbSQLSelectBuilder::GetBaseTableName(nsAString& aBaseTableName)
{
  aBaseTableName.Assign(mBaseTableName);
  return NS_OK;
}
NS_IMETHODIMP
sbSQLSelectBuilder::SetBaseTableName(const nsAString& aBaseTableName)
{
  mBaseTableName.Assign(aBaseTableName);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLSelectBuilder::GetBaseTableAlias(nsAString& aBaseTableAlias)
{
  aBaseTableAlias.Assign(mBaseTableAlias);
  return NS_OK;
}
NS_IMETHODIMP
sbSQLSelectBuilder::SetBaseTableAlias(const nsAString& aBaseTableAlias)
{
  mBaseTableAlias.Assign(aBaseTableAlias);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLSelectBuilder::GetDistinct(PRBool *aDistinct)
{
  *aDistinct = mIsDistinct;
  return NS_OK;
}
NS_IMETHODIMP
sbSQLSelectBuilder::SetDistinct(PRBool aDistinct)
{
  mIsDistinct = aDistinct;
  return NS_OK;
}

NS_IMETHODIMP
sbSQLSelectBuilder::AddColumn(const nsAString& aTableName,
                              const nsAString& aColumnName)
{
  sbColumnInfo* ci = mOutputColumns.AppendElement();
  NS_ENSURE_TRUE(ci, NS_ERROR_OUT_OF_MEMORY);

  ci->tableName  = aTableName;
  ci->columnName = aColumnName;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLSelectBuilder::ClearColumns()
{
  mOutputColumns.Clear();

  return NS_OK;
}

NS_IMETHODIMP
sbSQLSelectBuilder::AddOrder(const nsAString& aTableName,
                             const nsAString& aColumnName,
                             PRBool aAscending)
{
  sbOrderInfo* oi = mOrders.AppendElement();
  NS_ENSURE_TRUE(oi, NS_ERROR_OUT_OF_MEMORY);

  oi->tableName  = aTableName;
  oi->columnName = aColumnName;
  oi->ascending  = aAscending;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLSelectBuilder::ResetInternal()
{
  mBaseTableName.Truncate();
  mBaseTableAlias.Truncate();
  mLimit = -1;
  mLimitIsParameter = PR_FALSE;
  mOffset = -1;
  mOffsetIsParameter = PR_FALSE;
  mOutputColumns.Clear();
  mJoins.Clear();
  mSubqueries.Clear();
  mCritera.Clear();
  mOrders.Clear();

  return NS_OK;
}

NS_IMETHODIMP
sbSQLSelectBuilder::ToStringInternal(nsAString& _retval)
{
  nsresult rv;
  nsAutoString buff;

  // Start with a select...
  buff.AssignLiteral("select ");

  if (mIsDistinct) {
    buff.AppendLiteral("distinct ");
  }

  // Append output column names
  PRUint32 len = mOutputColumns.Length();
  for (PRUint32 i = 0; i < len; i++) {
    const sbColumnInfo& ci = mOutputColumns[i];
    if (!ci.tableName.IsEmpty()) {
      buff.Append(ci.tableName);
      buff.AppendLiteral(".");
    }
    buff.Append(ci.columnName);
    if (i + 1 < len) {
      buff.AppendLiteral(", ");
    }
  }

  // Append the from clause including the base table, subqueries, and joins
  buff.AppendLiteral(" from ");
  buff.Append(mBaseTableName);
  if (!mBaseTableAlias.IsEmpty()) {
      buff.AppendLiteral(" as ");
      buff.Append(mBaseTableAlias);
  }

  len = mSubqueries.Length();
  for (PRUint32 i = 0; i < len; i++) {
    const sbSubqueryInfo& sq = mSubqueries[i];
    buff.AppendLiteral(", ( ");
    nsAutoString str;
    sq.subquery->ToString(str);
    buff.Append(str);
    buff.AppendLiteral(" )");
    if (!sq.alias.IsEmpty()) {
      buff.AppendLiteral(" as ");
      buff.Append(sq.alias);
    }
  }

  len = mJoins.Length();
  for (PRUint32 i = 0; i < len; i++) {
    const sbJoinInfo& ji = mJoins[i];
    switch(ji.type) {
      case sbISQLBuilder::JOIN_INNER:
        /* default is inner */
        break;
      case sbISQLBuilder::JOIN_LEFT:
        buff.AppendLiteral(" left");
        break;
      default:
        NS_NOTREACHED("Unknown Join Type");
    }
    buff.AppendLiteral(" join ");
    buff.Append(ji.joinedTableName);
    if (!ji.joinedTableAlias.IsEmpty()) {
      buff.AppendLiteral(" as ");
      buff.Append(ji.joinedTableAlias);
    }
    buff.AppendLiteral(" on ");
    if (ji.criterion) {
      nsAutoString str;
      NS_STATIC_CAST(sbSQLBuilderCriterionBase*, ji.criterion.get())->ToString(str);
      buff.Append(str);
    }
    else {
      buff.Append(ji.joinToTableName);
      buff.AppendLiteral(".");
      buff.Append(ji.joinToColumnName);
      buff.AppendLiteral(" = ");
      if (!ji.joinedTableAlias.IsEmpty()) {
        buff.Append(ji.joinedTableAlias);
        buff.AppendLiteral(".");
      }
      else {
        if (!ji.joinedTableName.IsEmpty()) {
          buff.Append(ji.joinedTableName);
          buff.AppendLiteral(".");
        }
      }
      buff.Append(ji.joinedColumnName);
    }
  }

  // Append the where clause if there are criterea
  len = mCritera.Count();
  if (len > 0) {
    buff.AppendLiteral(" where ");
    for (PRUint32 i = 0; i < len; i++) {
      nsCOMPtr<sbISQLBuilderCriterion> criterion =
        do_QueryInterface(mCritera[i], &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsAutoString str;
      NS_STATIC_CAST(sbSQLBuilderCriterionBase*, criterion.get())->ToString(str);
      buff.Append(str);
      if (i + 1 < len) {
        buff.AppendLiteral(" and ");
      }
    }
  }

  // Append order by clause
  len = mOrders.Length();
  if (len > 0) {
    buff.AppendLiteral(" order by ");
    for (PRUint32 i = 0; i < len; i++) {
      const sbOrderInfo& oi = mOrders[i];
      if (!oi.tableName.IsEmpty()) {
        buff.Append(oi.tableName);
        buff.AppendLiteral(".");
      }
      buff.Append(oi.columnName);
      if (oi.ascending) {
        buff.AppendLiteral(" asc");
      }
      else {
        buff.AppendLiteral(" desc");
      }
      if (i + 1 < len) {
        buff.AppendLiteral(", ");
      }
    }
  }

  // Add limit and offset
  if(mLimit >= 0 || mLimitIsParameter) {
    buff.AppendLiteral(" limit ");
    if(mLimitIsParameter) {
      buff.AppendLiteral("?");
    }
    else {
      buff.AppendInt(mLimit);
    }
  }

  if(mOffset >= 0 || mOffsetIsParameter) {
    buff.AppendLiteral(" offset ");
    if(mOffsetIsParameter) {
      buff.AppendLiteral("?");
    }
    else {
      buff.AppendInt(mOffset);
    }
  }

  _retval.Assign(buff);
  return NS_OK;
}

