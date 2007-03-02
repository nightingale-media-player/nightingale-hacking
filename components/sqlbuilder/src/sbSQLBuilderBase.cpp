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

#include "sbSQLBuilderBase.h"
#include "sbSQLSelectBuilder.h"
#include "sbSQLBuilderCriterion.h"

#include <nsStringGlue.h>
#include <nsCOMPtr.h>

NS_IMPL_ISUPPORTS1(sbSQLBuilderBase, sbISQLBuilder)

sbSQLBuilderBase::sbSQLBuilderBase() :
  mLimit(-1),
  mLimitIsParameter(PR_FALSE),
  mOffset(-1),
  mOffsetIsParameter(PR_FALSE)
{
}

NS_IMETHODIMP
sbSQLBuilderBase::GetLimit(PRInt32 *aLimit)
{
  *aLimit = mLimit;
  return NS_OK;
}
NS_IMETHODIMP
sbSQLBuilderBase::SetLimit(PRInt32 aLimit)
{
  mLimit = aLimit;
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::GetLimitIsParameter(PRBool *aLimitIsParameter)
{
  *aLimitIsParameter = mLimitIsParameter;
  return NS_OK;
}
NS_IMETHODIMP
sbSQLBuilderBase::SetLimitIsParameter(PRBool aLimitIsParameter)
{
  mLimitIsParameter = aLimitIsParameter;
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::GetOffset(PRInt32 *aOffset)
{
  *aOffset = mOffset;
  return NS_OK;
}
NS_IMETHODIMP
sbSQLBuilderBase::SetOffset(PRInt32 aOffset)
{
  mOffset = aOffset;
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::GetOffsetIsParameter(PRBool *aOffsetIsParameter)
{
  *aOffsetIsParameter = mOffsetIsParameter;
  return NS_OK;
}
NS_IMETHODIMP
sbSQLBuilderBase::SetOffsetIsParameter(PRBool aOffsetIsParameter)
{
  mOffsetIsParameter = aOffsetIsParameter;
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::AddJoin(PRUint32 aJoinType,
                          const nsAString& aJoinedTableName,
                          const nsAString& aJoinedTableAlias,
                          const nsAString& aJoinedColumnName,
                          const nsAString& aJoinToTableName,
                          const nsAString& aJoinToColumnName)
{
  sbJoinInfo* ji = mJoins.AppendElement();
  NS_ENSURE_TRUE(ji, NS_ERROR_OUT_OF_MEMORY);

  ji->type             = aJoinType;
  ji->joinedTableName  = aJoinedTableName;
  ji->joinedTableAlias = aJoinedTableAlias;
  ji->joinedColumnName = aJoinedColumnName;
  ji->joinToTableName  = aJoinToTableName;
  ji->joinToColumnName = aJoinToColumnName;
  ji->criterion        = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::AddJoinWithCriterion(PRUint32 aJoinType,
                                       const nsAString& aJoinedTableName,
                                       const nsAString& aJoinedTableAlias,
                                       sbISQLBuilderCriterion *aCriterion)
{
  sbJoinInfo* ji = mJoins.AppendElement();
  NS_ENSURE_TRUE(ji, NS_ERROR_OUT_OF_MEMORY);

  ji->type             = aJoinType;
  ji->joinedTableName  = aJoinedTableName;
  ji->joinedTableAlias = aJoinedTableAlias;
  ji->joinedColumnName = EmptyString();
  ji->joinToTableName  = EmptyString();
  ji->joinToColumnName = EmptyString();
  ji->criterion        = aCriterion;

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::AddSubquery(sbISQLSelectBuilder *aSubquery,
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
sbSQLBuilderBase::AddCriterion(sbISQLBuilderCriterion *aCriterion)
{
  NS_ENSURE_ARG_POINTER(aCriterion);

  mCritera.AppendObject(aCriterion);

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::CreateMatchCriterionString(const nsAString& aTableName,
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
sbSQLBuilderBase::CreateMatchCriterionLong(const nsAString& aTableName,
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
sbSQLBuilderBase::CreateMatchCriterionNull(const nsAString& aTableName,
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
sbSQLBuilderBase::CreateMatchCriterionParameter(const nsAString& aTableName,
                                                const nsAString& aSrcColumnName,
                                                PRUint32 aMatchType,
                                                sbISQLBuilderCriterion **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterion> criterion =
    new sbSQLBuilderCriterionParameter(aTableName, aSrcColumnName, aMatchType);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::CreateMatchCriterionTable(const nsAString& aLeftTableName,
                                            const nsAString& aLeftColumnName,
                                            PRUint32 aMatchType,
                                            const nsAString& aRightTableName,
                                            const nsAString& aRightColumnName,
                                            sbISQLBuilderCriterion **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterion> criterion =
    new sbSQLBuilderCriterionTable(aLeftTableName, aLeftColumnName, aMatchType,
                                   aRightTableName, aRightColumnName);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::CreateMatchCriterionIn(const nsAString& aTableName,
                                         const nsAString& aSrcColumnName,
                                         sbISQLBuilderCriterionIn **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterionIn> criterion =
    new sbSQLBuilderCriterionIn(aTableName, aSrcColumnName);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);

  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::CreateAndCriterion(sbISQLBuilderCriterion *aLeft,
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
sbSQLBuilderBase::CreateOrCriterion(sbISQLBuilderCriterion *aLeft,
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
sbSQLBuilderBase::Reset()
{
  return ResetInternal();
}

NS_IMETHODIMP
sbSQLBuilderBase::ToString(nsAString& _retval)
{
  // Foward this method call to the derived class' ToStringInternal() method.
  // This allows derived classes to effectivly override the base class'
  // ToString() method and still use NS_FORWARD_SBISQLBUILDER in the header
  // definition
  return ToStringInternal(_retval);
}

