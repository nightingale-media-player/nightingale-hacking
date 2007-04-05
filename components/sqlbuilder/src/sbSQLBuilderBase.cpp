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

NS_IMPL_THREADSAFE_ISUPPORTS1(sbSQLBuilderBase, sbISQLBuilder)

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
sbSQLBuilderBase::Reset()
{
  mLimit = -1;
  mLimitIsParameter = PR_FALSE;
  mOffset = -1;
  mOffsetIsParameter = PR_FALSE;
  mJoins.Clear();
  mSubqueries.Clear();
  return NS_OK;
}

NS_IMETHODIMP
sbSQLBuilderBase::ToString(nsAString& _retval)
{
  // not meant to be implemented by base class
  return NS_ERROR_NOT_IMPLEMENTED;
}

