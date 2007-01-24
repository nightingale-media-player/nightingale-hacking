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

#include "sbSQLBuilder.h"

#include <nsStringGlue.h>
#include <nsCOMPtr.h>

NS_IMPL_ISUPPORTS1(sbSQLBuilder, sbISQLBuilder)

NS_IMETHODIMP
sbSQLBuilder::GetBaseTableName(nsAString& aBaseTableName)
{
  aBaseTableName.Assign(mBaseTableName);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::SetBaseTableName(const nsAString& aBaseTableName)
{
  mBaseTableName.Assign(aBaseTableName);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::GetBaseTableAlias(nsAString& aBaseTableAlias)
{
  aBaseTableAlias.Assign(mBaseTableAlias);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::SetBaseTableAlias(const nsAString& aBaseTableAlias)
{
  mBaseTableAlias.Assign(aBaseTableAlias);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::AddColumn(const nsAString& aTableName,
                        const nsAString& aColumnName)
{
  sbColumnInfo* ci = mOutputColumns.AppendElement();
  NS_ENSURE_TRUE(ci, NS_ERROR_OUT_OF_MEMORY);

  ci->tableName  = aTableName;
  ci->columnName = aColumnName;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::AddJoin(const nsAString& aJoinedTableName,
                      const nsAString& aJoinedTableAlias,
                      const nsAString& aJoinedColumnName,
                      const nsAString& aJoinToTableName,
                      const nsAString& aJoinToColumnName)
{
  sbJoinInfo* ji = mJoins.AppendElement();
  NS_ENSURE_TRUE(ji, NS_ERROR_OUT_OF_MEMORY);

  ji->joinedTableName  = aJoinedTableName;
  ji->joinedTableAlias = aJoinedTableAlias;
  ji->joinedColumnName = aJoinedColumnName;
  ji->joinToTableName  = aJoinToTableName;
  ji->joinToColumnName = aJoinToColumnName;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::AddSubquery(sbISQLSelectBuilder *aSubquery,
                          const nsAString& aAlias)
{
  NS_ENSURE_ARG_POINTER(aSubquery);

  // Don't allow a query to use itself as a subquery
  // XXX: This is bad since it assumes the implementation class of the
  // aSubquery parameter.  What is the right way to do this?
  sbSQLSelectBuilder* sb = NS_STATIC_CAST(sbSQLSelectBuilder*, aSubquery);
  NS_ENSURE_TRUE(sb != this, NS_ERROR_INVALID_ARG);

  sbSubqueryInfo* sq = mSubqueries.AppendElement();
  NS_ENSURE_TRUE(sq, NS_ERROR_OUT_OF_MEMORY);

  sq->subquery = aSubquery;
  sq->alias    = aAlias;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::AddCriterion(sbISQLBuilderCriterion *aCriterion)
{
  NS_ENSURE_ARG_POINTER(aCriterion);

  mCritera.AppendObject(aCriterion);

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::CreateMatchCriterionString(const nsAString& aTableName,
                                         const nsAString& aSrcColumnName,
                                         PRUint32 aMatchType,
                                         const nsAString& aValue,
                                         sbISQLBuilderCriterion** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterion> criterion =
    new sbSQLBuilderCriterionString(aTableName, aSrcColumnName, aMatchType, aValue);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::CreateMatchCriterionLong(const nsAString& aTableName,
                                       const nsAString& aSrcColumnName,
                                       PRUint32 aMatchType,
                                       PRInt32 aValue,
                                       sbISQLBuilderCriterion **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterion> criterion =
    new sbSQLBuilderCriterionLong(aTableName, aSrcColumnName, aMatchType, aValue);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::CreateMatchCriterionNull(const nsAString& aTableName,
                                       const nsAString& aSrcColumnName,
                                       PRUint32 aMatchType,
                                       sbISQLBuilderCriterion **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterion> criterion =
    new sbSQLBuilderCriterionNull(aTableName, aSrcColumnName, aMatchType);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::CreateAndCriterion(sbISQLBuilderCriterion *aLeft,
                                 sbISQLBuilderCriterion *aRight,
                                 sbISQLBuilderCriterion **_retval)
{
  NS_ENSURE_ARG_POINTER(aLeft);
  NS_ENSURE_ARG_POINTER(aRight);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterion> criterion =
    new sbSQLBuilderCriterionAnd(aLeft, aRight);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::CreateOrCriterion(sbISQLBuilderCriterion *aLeft,
                                sbISQLBuilderCriterion *aRight,
                                sbISQLBuilderCriterion **_retval)
{
  NS_ENSURE_ARG_POINTER(aLeft);
  NS_ENSURE_ARG_POINTER(aRight);
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterion> criterion =
    new sbSQLBuilderCriterionOr(aLeft, aRight);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilder::ToString(nsAString& _retval)
{
  // Foward this method call to the derived class' ToStringInternal() method.
  // This allows derived classes to effectivly override the base class'
  // ToString() method and still use NS_FORWARD_SBISQLBUILDER in the header
  // definition
  ToStringInternal(_retval);

  return NS_OK;
}

// sbSQLSelectBuilder
NS_IMPL_ISUPPORTS_INHERITED1(sbSQLSelectBuilder,
                             sbSQLBuilder,
                             sbISQLSelectBuilder)

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
sbSQLSelectBuilder::ToStringInternal(nsAString& _retval)
{
  nsAutoString buff;

  // Start with a select...
  buff.AssignLiteral("select ");

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
    buff.AppendLiteral(" join ");
    buff.Append(ji.joinedTableName);
    if (!ji.joinedTableAlias.IsEmpty()) {
      buff.AppendLiteral(" as ");
      buff.Append(ji.joinedTableAlias);
    }
    buff.AppendLiteral(" on ");
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

  // Append the where clause if there are criterea
  len = mCritera.Count();
  if (len > 0) {
    buff.AppendLiteral(" where ");
    for (PRUint32 i = 0; i < len; i++) {
      nsCOMPtr<sbISQLBuilderCriterion> criterion = mCritera[i];
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

  _retval.Assign(buff);
  return NS_OK;
}

// sbSQLBuilderCriterionBase
NS_IMPL_ISUPPORTS1(sbSQLBuilderCriterionBase, sbISQLBuilderCriterion)

sbSQLBuilderCriterionBase::sbSQLBuilderCriterionBase(const nsAString& aTableName,
                                                     const nsAString& aColumnName,
                                                     PRUint32 aMatchType,
                                                     sbISQLBuilderCriterion* aLeft,
                                                     sbISQLBuilderCriterion* aRight) :
  mTableName(aTableName),
  mColumnName(aColumnName),
  mMatchType(aMatchType),
  mLeft(aLeft),
  mRight(aRight)
{
}

void
sbSQLBuilderCriterionBase::AppendMatchTo(nsAString& aStr)
{
  switch(mMatchType) {
    case sbISQLBuilder::MATCH_EQUALS:
      aStr.AppendLiteral(" = ");
      break;
    case sbISQLBuilder::MATCH_NOTEQUALS:
      aStr.AppendLiteral(" != ");
      break;
    case sbISQLBuilder::MATCH_GREATER:
      aStr.AppendLiteral(" > ");
      break;
    case sbISQLBuilder::MATCH_GREATEREQUAL:
      aStr.AppendLiteral(" >= ");
      break;
    case sbISQLBuilder::MATCH_LESS:
      aStr.AppendLiteral(" < ");
      break;
    case sbISQLBuilder::MATCH_LESSEQUAL:
      aStr.AppendLiteral(" <= ");
      break;
    case sbISQLBuilder::MATCH_LIKE:
      aStr.AppendLiteral(" like ");
      break;
    default:
      NS_NOTREACHED("Unknown Match Type");
  }
}

void
sbSQLBuilderCriterionBase::AssignTableColumnTo(nsAString& aStr)
{
  aStr.Truncate();
  if (!mTableName.IsEmpty()) {
    aStr.Assign(mTableName);
    aStr.AppendLiteral(".");
  }
  aStr.Append(mColumnName);
}

void
sbSQLBuilderCriterionBase::AppendLogicalTo(const nsAString& aOperator,
                                       nsAString& aStr)
{
  aStr.AppendLiteral("(");

  sbISQLBuilderCriterion* criterion = mLeft;
  nsAutoString str;
  NS_STATIC_CAST(sbSQLBuilderCriterionBase*, criterion)->ToString(str);
  aStr.Append(str);

  aStr.AppendLiteral(" ");
  aStr.Append(aOperator);
  aStr.AppendLiteral(" ");

  criterion = mRight;
  str.Truncate();
  NS_STATIC_CAST(sbSQLBuilderCriterionBase*, criterion)->ToString(str);
  aStr.Append(str);

  aStr.AppendLiteral(")");
}

// sbSQLBuilderCriterionString
NS_IMPL_ISUPPORTS_INHERITED0(sbSQLBuilderCriterionString,
                             sbSQLBuilderCriterionBase)

sbSQLBuilderCriterionString::sbSQLBuilderCriterionString(const nsAString& aTableName,
                                                         const nsAString& aColumnName,
                                                         PRUint32 aMatchType,
                                                         const nsAString& aValue) :
  sbSQLBuilderCriterionBase(aTableName, aColumnName, aMatchType, nsnull, nsnull),
  mValue(aValue)
{
}

NS_IMETHODIMP
sbSQLBuilderCriterionString::ToString(nsAString& _retval)
{
  AssignTableColumnTo(_retval);

  AppendMatchTo(_retval);

  nsAutoString escapedValue(mValue);
  SB_EscapeSQL(escapedValue);

  _retval.AppendLiteral("'");
  _retval.Append(escapedValue);
  _retval.AppendLiteral("'");
  return NS_OK;
}

// sbSQLBuilderCriterionLong
NS_IMPL_ISUPPORTS_INHERITED0(sbSQLBuilderCriterionLong,
                             sbSQLBuilderCriterionBase)

sbSQLBuilderCriterionLong::sbSQLBuilderCriterionLong(const nsAString& aTableName,
                                                     const nsAString& aColumnName,
                                                     PRUint32 aMatchType,
                                                     PRInt32 aValue) :
  sbSQLBuilderCriterionBase(aTableName, aColumnName, aMatchType, nsnull, nsnull),
  mValue(aValue)
{
}

NS_IMETHODIMP
sbSQLBuilderCriterionLong::ToString(nsAString& _retval)
{
  AssignTableColumnTo(_retval);

  AppendMatchTo(_retval);

  nsAutoString stringValue;
  stringValue.AppendInt(mValue);
  _retval.Append(stringValue);
  return NS_OK;
}

// sbSQLBuilderCriterionNull
NS_IMPL_ISUPPORTS_INHERITED0(sbSQLBuilderCriterionNull,
                             sbSQLBuilderCriterionBase)

sbSQLBuilderCriterionNull::sbSQLBuilderCriterionNull(const nsAString& aTableName,
                                                     const nsAString& aColumnName,
                                                     PRUint32 aMatchType) :
  sbSQLBuilderCriterionBase(aTableName, aColumnName, aMatchType, nsnull, nsnull)
{
}

NS_IMETHODIMP
sbSQLBuilderCriterionNull::ToString(nsAString& _retval)
{
  AssignTableColumnTo(_retval);

  if (mMatchType == sbISQLBuilder::MATCH_EQUALS) {
    _retval.AppendLiteral(" is null");
  }
  else {
    _retval.AppendLiteral(" is not null");
  }
  return NS_OK;
}

// sbSQLBuilderCriterionAnd
NS_IMPL_ISUPPORTS_INHERITED0(sbSQLBuilderCriterionAnd,
                             sbSQLBuilderCriterionBase)

sbSQLBuilderCriterionAnd::sbSQLBuilderCriterionAnd(sbISQLBuilderCriterion* aLeft,
                                                   sbISQLBuilderCriterion* aRight) :
  sbSQLBuilderCriterionBase(EmptyString(), EmptyString(), 0, aLeft, aRight)
{
}

NS_IMETHODIMP
sbSQLBuilderCriterionAnd::ToString(nsAString& _retval)
{
  AppendLogicalTo(NS_LITERAL_STRING("and"), _retval);
  return NS_OK;
}

// sbSQLBuilderCriterionOr
NS_IMPL_ISUPPORTS_INHERITED0(sbSQLBuilderCriterionOr,
                             sbSQLBuilderCriterionBase)

sbSQLBuilderCriterionOr::sbSQLBuilderCriterionOr(sbISQLBuilderCriterion* aLeft,
                                                 sbISQLBuilderCriterion* aRight) :
  sbSQLBuilderCriterionBase(EmptyString(), EmptyString(), 0, aLeft, aRight)
{
  mLeft  = aLeft;
  mRight = aRight;
}

NS_IMETHODIMP
sbSQLBuilderCriterionOr::ToString(nsAString& _retval)
{
  AppendLogicalTo(NS_LITERAL_STRING("or"), _retval);
  return NS_OK;
}

