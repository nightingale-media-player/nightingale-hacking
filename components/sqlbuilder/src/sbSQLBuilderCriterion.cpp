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

#include "sbSQLBuilderCriterion.h"
#include "sbSQLBuilderBase.h"

NS_IMPL_THREADSAFE_ISUPPORTS1(sbSQLBuilderCriterionBase,
                              sbISQLBuilderCriterion)

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
    case sbISQLWhereBuilder::MATCH_EQUALS:
      aStr.AppendLiteral(" = ");
      break;
    case sbISQLWhereBuilder::MATCH_NOTEQUALS:
      aStr.AppendLiteral(" != ");
      break;
    case sbISQLWhereBuilder::MATCH_GREATER:
      aStr.AppendLiteral(" > ");
      break;
    case sbISQLWhereBuilder::MATCH_GREATEREQUAL:
      aStr.AppendLiteral(" >= ");
      break;
    case sbISQLWhereBuilder::MATCH_LESS:
      aStr.AppendLiteral(" < ");
      break;
    case sbISQLWhereBuilder::MATCH_LESSEQUAL:
      aStr.AppendLiteral(" <= ");
      break;
    case sbISQLWhereBuilder::MATCH_LIKE:
      aStr.AppendLiteral(" like ");
      break;
    default:
      NS_NOTREACHED("Unknown Match Type");
  }
}

void
sbSQLBuilderCriterionBase::AppendTableColumnTo(nsAString& aStr)
{
  if (!mTableName.IsEmpty()) {
    aStr.Append(mTableName);
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
  AppendTableColumnTo(_retval);

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
  AppendTableColumnTo(_retval);

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
  AppendTableColumnTo(_retval);

  if (mMatchType == sbISQLWhereBuilder::MATCH_EQUALS) {
    _retval.AppendLiteral(" is null");
  }
  else {
    _retval.AppendLiteral(" is not null");
  }
  return NS_OK;
}

// sbSQLBuilderCriterionParameter
NS_IMPL_ISUPPORTS_INHERITED0(sbSQLBuilderCriterionParameter,
                             sbSQLBuilderCriterionBase)

sbSQLBuilderCriterionParameter::sbSQLBuilderCriterionParameter(const nsAString& aTableName,
                                                               const nsAString& aColumnName,
                                                               PRUint32 aMatchType) :
  sbSQLBuilderCriterionBase(aTableName, aColumnName, aMatchType, nsnull, nsnull)
{
}

NS_IMETHODIMP
sbSQLBuilderCriterionParameter::ToString(nsAString& _retval)
{
  AppendTableColumnTo(_retval);

  AppendMatchTo(_retval);

  _retval.AppendLiteral("?");
  return NS_OK;
}

// sbSQLBuilderCriterionTable
NS_IMPL_ISUPPORTS_INHERITED0(sbSQLBuilderCriterionTable,
                             sbSQLBuilderCriterionBase)

sbSQLBuilderCriterionTable::sbSQLBuilderCriterionTable(const nsAString& aLeftTableName,
                                                       const nsAString& aLeftColumnName,
                                                       PRUint32 aMatchType,
                                                       const nsAString& aRightTableName,
                                                       const nsAString& aRightColumnName) :
  sbSQLBuilderCriterionBase(aLeftTableName, aLeftColumnName, aMatchType, nsnull, nsnull),
  mRightTableName(aRightTableName),
  mRightColumnName(aRightColumnName)
{
}

NS_IMETHODIMP
sbSQLBuilderCriterionTable::ToString(nsAString& _retval)
{
  AppendTableColumnTo(_retval);

  AppendMatchTo(_retval);

  if (!mRightTableName.IsEmpty()) {
    _retval.Append(mRightTableName);
    _retval.AppendLiteral(".");
  }
  _retval.Append(mRightColumnName);

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

// sbSQLBuilderCriterionIn
NS_IMPL_ISUPPORTS_INHERITED1(sbSQLBuilderCriterionIn,
                             sbSQLBuilderCriterionBase,
                             sbISQLBuilderCriterionIn)

sbSQLBuilderCriterionIn::sbSQLBuilderCriterionIn(const nsAString& aTableName,
                                                 const nsAString& aColumnName) :
  sbSQLBuilderCriterionBase(aTableName, aColumnName, 0, nsnull, nsnull)
{
}

NS_IMETHODIMP
sbSQLBuilderCriterionIn::AddString(const nsAString& aValue)
{
  sbInItem* ii = mInItems.AppendElement();
  NS_ENSURE_TRUE(ii, NS_ERROR_OUT_OF_MEMORY);

  ii->type        = eString;
  ii->stringValue = aValue;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderCriterionIn::AddSubquery(sbISQLSelectBuilder* aSubquery)
{
  sbInItem* ii = mInItems.AppendElement();
  NS_ENSURE_TRUE(ii, NS_ERROR_OUT_OF_MEMORY);

  ii->type     = eSubquery;
  ii->subquery = aSubquery;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderCriterionIn::Clear()
{
  mInItems.Clear();

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderCriterionIn::ToString(nsAString& _retval)
{
  AppendTableColumnTo(_retval);

  _retval.AppendLiteral(" in (");

  PRUint32 len = mInItems.Length();
  for (PRUint32 i = 0; i < len; i++) {
    const sbInItem& ii = mInItems[i];

    switch(ii.type) {
      case eIsNull:
        /* not implemented */
        break;
      case eString:
      {
        nsAutoString escapedValue(ii.stringValue);
        SB_EscapeSQL(escapedValue);

        _retval.AppendLiteral("'");
        _retval.Append(escapedValue);
        _retval.AppendLiteral("'");
        break;
      }
      case eInteger32:
        /* not implemented */
        break;
      case eSubquery:
      {
        nsresult rv;
        nsAutoString sql;
        rv = ii.subquery->ToString(sql);
        NS_ENSURE_SUCCESS(rv, rv);
        _retval.Append(sql);
        break;
      }
    }

    if (i + 1 < len) {
      _retval.AppendLiteral(", ");
    }
  }

  _retval.AppendLiteral(")");

  return NS_OK;
}

