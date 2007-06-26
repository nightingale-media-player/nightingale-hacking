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

#ifndef __SBSQLBUILDERCRITERION_H__
#define __SBSQLBUILDERCRITERION_H__

#include <sbISQLBuilder.h>

#include <nsAutoPtr.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>

class sbSQLBuilderCriterionBase : public sbISQLBuilderCriterion
{
public:
  NS_DECL_ISUPPORTS

  sbSQLBuilderCriterionBase(const nsAString& aTableName,
                            const nsAString& aColumnName,
                            PRUint32 aMatchType,
                            sbISQLBuilderCriterion* aLeft,
                            sbISQLBuilderCriterion* aRight);

  virtual ~sbSQLBuilderCriterionBase() {};
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

  virtual ~sbSQLBuilderCriterionString() {};
private:
  nsString mValue;
};

class sbSQLBuilderCriterionBetweenString : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionBetweenString(const nsAString& aTableName,
                                     const nsAString& aColumnName,
                                     const nsAString& aLeftValue,
                                     const nsAString& aRightValue,
                                     PRBool aNegate);

  virtual ~sbSQLBuilderCriterionBetweenString() {};
private:
  nsString mLeftValue;
  nsString mRightValue;
  PRBool mNegate;
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

  virtual ~sbSQLBuilderCriterionLong() {};
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
  virtual ~sbSQLBuilderCriterionNull() {};
};

class sbSQLBuilderCriterionParameter : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionParameter(const nsAString& aTableName,
                                 const nsAString& aColumnName,
                                 PRUint32 aMatchType);
  virtual ~sbSQLBuilderCriterionParameter() {};
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
  virtual ~sbSQLBuilderCriterionTable() {};
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
  virtual ~sbSQLBuilderCriterionAnd() {};
};

class sbSQLBuilderCriterionOr : public sbSQLBuilderCriterionBase
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBISQLBUILDERCRITERION

  sbSQLBuilderCriterionOr(sbISQLBuilderCriterion* aLeft,
                          sbISQLBuilderCriterion* aRight);
  virtual ~sbSQLBuilderCriterionOr() {};
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
  virtual ~sbSQLBuilderCriterionIn() {};
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

#endif /* __SBSQLBUILDERCRITERION_H__ */

