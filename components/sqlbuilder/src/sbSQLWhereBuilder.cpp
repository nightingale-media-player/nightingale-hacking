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

#include "sbSQLWhereBuilder.h"
#include "sbSQLBuilderBase.h"
#include "sbSQLBuilderCriterion.h"

NS_IMPL_ISUPPORTS_INHERITED1(sbSQLWhereBuilder,
                             sbSQLBuilderBase,
                             sbISQLWhereBuilder)

sbSQLWhereBuilder::sbSQLWhereBuilder()
{
  MOZ_COUNT_CTOR(sbSQLWhereBuilder);
}

sbSQLWhereBuilder::~sbSQLWhereBuilder()
{
  MOZ_COUNT_DTOR(sbSQLWhereBuilder);
}

NS_IMETHODIMP
sbSQLWhereBuilder::AddCriterion(sbISQLBuilderCriterion *aCriterion)
{
  NS_ENSURE_ARG_POINTER(aCriterion);

  mCritera.AppendObject(aCriterion);

  return NS_OK;
}

NS_IMETHODIMP
sbSQLWhereBuilder::RemoveCriterion(sbISQLBuilderCriterion *aCriterion)
{
  NS_ENSURE_ARG_POINTER(aCriterion);

  PRBool success = mCritera.RemoveObject(aCriterion);
  if (!success) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbSQLWhereBuilder::CreateMatchCriterionString(const nsAString& aTableName,
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
sbSQLWhereBuilder::CreateMatchCriterionBetweenString(const nsAString& aTableName,
                                                     const nsAString& aSrcColumnName,
                                                     const nsAString& aLeftValue,
                                                     const nsAString& aRightValue,
                                                     sbISQLBuilderCriterion** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterion> criterion =
    new sbSQLBuilderCriterionBetweenString(aTableName, aSrcColumnName, aLeftValue, aRightValue, PR_FALSE);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLWhereBuilder::CreateMatchCriterionNotBetweenString(const nsAString& aTableName,
                                                        const nsAString& aSrcColumnName,
                                                        const nsAString& aLeftValue,
                                                        const nsAString& aRightValue,
                                                        sbISQLBuilderCriterion** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbISQLBuilderCriterion> criterion =
    new sbSQLBuilderCriterionBetweenString(aTableName, aSrcColumnName, aLeftValue, aRightValue, PR_TRUE);
  NS_ENSURE_TRUE(criterion, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = criterion);
  return NS_OK;
}

NS_IMETHODIMP
sbSQLWhereBuilder::CreateMatchCriterionLong(const nsAString& aTableName,
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
sbSQLWhereBuilder::CreateMatchCriterionNull(const nsAString& aTableName,
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
sbSQLWhereBuilder::CreateMatchCriterionParameter(const nsAString& aTableName,
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
sbSQLWhereBuilder::CreateMatchCriterionTable(const nsAString& aLeftTableName,
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
sbSQLWhereBuilder::CreateMatchCriterionIn(const nsAString& aTableName,
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
sbSQLWhereBuilder::CreateAndCriterion(sbISQLBuilderCriterion *aLeft,
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
sbSQLWhereBuilder::CreateOrCriterion(sbISQLBuilderCriterion *aLeft,
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
sbSQLWhereBuilder::Reset()
{
  sbSQLBuilderBase::Reset();
  mCritera.Clear();
  return NS_OK;
}

nsresult
sbSQLWhereBuilder::AppendWhere(nsAString& aBuffer)
{
  nsresult rv;

  // Append the where clause if there are criterea
  PRUint32 len = mCritera.Count();
  if (len > 0) {
    aBuffer.AppendLiteral(" where ");
    for (PRUint32 i = 0; i < len; i++) {
      nsCOMPtr<sbISQLBuilderCriterion> criterion =
        do_QueryInterface(mCritera[i], &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsAutoString str;
      rv = criterion->ToString(str);
      NS_ENSURE_SUCCESS(rv, rv);
      aBuffer.Append(str);
      if (i + 1 < len) {
        aBuffer.AppendLiteral(" and ");
      }
    }
  }

  return NS_OK;
}

