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

#ifndef __SBSQLBUILDER_H__
#define __SBSQLBUILDER_H__

#include <sbISQLBuilder.h>

#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>

#define QUOTE_CHAR '\''

class sbSQLBuilder : public sbISQLBuilder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBISQLBUILDER

  sbSQLBuilder();
protected:

  struct sbColumnInfo
  {
    nsString tableName;
    nsString columnName;
  };

  struct sbJoinInfo
  {
    PRUint32 type;
    nsString joinedTableName;
    nsString joinedTableAlias;
    nsString joinedColumnName;
    nsString joinToTableName;
    nsString joinToColumnName;
    nsCOMPtr<sbISQLBuilderCriterion> criterion;
  };

  struct sbOrderInfo
  {
    nsString tableName;
    nsString columnName;
    PRBool ascending;
  };

  struct sbSubqueryInfo
  {
    nsCOMPtr<sbISQLSelectBuilder> subquery;
    nsString alias;
  };

  NS_IMETHOD ToStringInternal(nsAString& _retval) = 0;
  NS_IMETHOD ResetInternal() = 0;

  nsString mBaseTableName;
  nsString mBaseTableAlias;
  PRBool mIsDistinct;
  PRInt32 mLimit;
  PRBool mLimitIsParameter;
  PRInt32 mOffset;
  PRBool mOffsetIsParameter;
  nsTArray<sbColumnInfo> mOutputColumns;
  nsTArray<sbJoinInfo> mJoins;
  nsTArray<sbSubqueryInfo> mSubqueries;
  nsCOMArray<sbISQLBuilderCriterion> mCritera;

};

class sbSQLSelectBuilder : public sbSQLBuilder,
                           public sbISQLSelectBuilder
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_SBISQLBUILDER(sbSQLBuilder::)
  NS_DECL_SBISQLSELECTBUILDER

  NS_IMETHOD ToStringInternal(nsAString& _retval);
  NS_IMETHOD ResetInternal();

private:
  nsTArray<sbOrderInfo> mOrders;
};

class sbSQLBuilderCriterionBase : public sbISQLBuilderCriterion
{
public:
  NS_DECL_ISUPPORTS

  sbSQLBuilderCriterionBase(const nsAString& aTableName,
                            const nsAString& aColumnName,
                            PRUint32 aMatchType,
                            sbISQLBuilderCriterion* aLeft,
                            sbISQLBuilderCriterion* aRight);

protected:
  void AppendMatchTo(nsAString& aStr);
  void AppendTableColumnTo(nsAString& aStr);
  void AppendLogicalTo(const nsAString& aOperator, nsAString& aStr);

  nsString mTableName;
  nsString mColumnName;
  PRUint32 mMatchType;

  nsCOMPtr<sbISQLBuilderCriterion> mLeft;
  nsCOMPtr<sbISQLBuilderCriterion> mRight;

};

class sbSQLBuilderCriterionString : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionString(const nsAString& aTableName,
                              const nsAString& aColumnName,
                              PRUint32 aMatchType,
                              const nsAString& aValue);

private:
  nsString mValue;
};

class sbSQLBuilderCriterionLong : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionLong(const nsAString& aTableName,
                            const nsAString& aColumnName,
                            PRUint32 aMatchType,
                            PRInt32 aValue);

private:
  PRInt32 mValue;
};

class sbSQLBuilderCriterionNull : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionNull(const nsAString& aTableName,
                            const nsAString& aColumnName,
                            PRUint32 aMatchType);
};

class sbSQLBuilderCriterionParameter : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionParameter(const nsAString& aTableName,
                                 const nsAString& aColumnName,
                                 PRUint32 aMatchType);
};

class sbSQLBuilderCriterionTable : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionTable(const nsAString& aLeftTableName,
                             const nsAString& aLeftColumnName,
                             PRUint32 aMatchType,
                             const nsAString& aRightTableName,
                             const nsAString& aRightColumnName);

private:
  nsString mRightTableName;
  nsString mRightColumnName;
};

class sbSQLBuilderCriterionAnd : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionAnd(sbISQLBuilderCriterion* aLeft,
                           sbISQLBuilderCriterion* aRight);
};

class sbSQLBuilderCriterionOr : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionOr(sbISQLBuilderCriterion* aLeft,
                          sbISQLBuilderCriterion* aRight);
};

class sbSQLBuilderCriterionIn : public sbSQLBuilderCriterionBase,
                                public sbISQLBuilderCriterionIn
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION
  NS_DECL_SBISQLBUILDERCRITERIONIN

  sbSQLBuilderCriterionIn(const nsAString& aTableName,
                          const nsAString& aColumnName);

private:
  enum ParameterType {
    eIsNull,
    eString,
    eInteger32,
    eSubquery
  };

  struct sbInItem
  {
    ParameterType type;
    nsString stringValue;
    PRInt32 int32Value;
    nsCOMPtr<sbISQLSelectBuilder> subquery;
  };

  nsTArray<sbInItem> mInItems;
};

static nsresult
SB_EscapeSQL(nsAString& str)
{
  nsAutoString dest;

  PRInt32 pos = str.FindChar(QUOTE_CHAR, 0);
  PRInt32 lastPos = 0;
  PRBool hasQuote = PR_FALSE;
  while(pos >= 0) {
    dest.Append(Substring(str, lastPos, pos - lastPos + 1));
    dest.Append(QUOTE_CHAR);
    lastPos = pos + 1;
    pos = str.FindChar(QUOTE_CHAR, lastPos);
    hasQuote = PR_TRUE;
  }

  if (hasQuote) {
    dest.Append(Substring(str, lastPos, str.Length() - lastPos));
    str.Assign(dest);
  }

  return NS_OK;
}

#endif /* __SBSQLBUILDER_H__ */

