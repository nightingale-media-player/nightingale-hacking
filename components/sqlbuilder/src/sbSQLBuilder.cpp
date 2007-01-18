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

NS_IMPL_ISUPPORTS1(sbSQLBuilder, sbISQLBuilder)

sbSQLBuilder::sbSQLBuilder()
{
  /* member initializers and constructor code */
}

sbSQLBuilder::~sbSQLBuilder()
{
  /* destructor code */
}

/* attribute ACString baseTableName; */
NS_IMETHODIMP sbSQLBuilder::GetBaseTableName(nsACString & aBaseTableName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbSQLBuilder::SetBaseTableName(const nsACString & aBaseTableName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void addColumn (in ACString aTableName, in ACString aColumnName); */
NS_IMETHODIMP sbSQLBuilder::AddColumn(const nsACString & aTableName, const nsACString & aColumnName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void addJoin (in ACString aSrcTableName, in ACString aSrcColumnName, in ACString aDestTable, in ACString aDestColumnName); */
NS_IMETHODIMP sbSQLBuilder::AddJoin(const nsACString & aSrcTableName, const nsACString & aSrcColumnName, const nsACString & aDestTable, const nsACString & aDestColumnName)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* void addCriterion (in sbISQLBuilderCriterion aCriterion); */
NS_IMETHODIMP sbSQLBuilder::AddCriterion(sbISQLBuilderCriterion *aCriterion)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* AString toString (); */
NS_IMETHODIMP sbSQLBuilder::ToString(nsAString & _retval)
{
  _retval.Assign(NS_LITERAL_STRING("Not yet!!!"));
  return NS_OK;
}

/* sbISQLBuilderCriterion createMatchCriterionString (in ACString aTableName, in ACString aSrcColumnName, in short aMatchType, in AString aValue); */
NS_IMETHODIMP sbSQLBuilder::CreateMatchCriterionString(const nsACString & aTableName, const nsACString & aSrcColumnName, PRInt16 aMatchType, const nsAString & aValue, sbISQLBuilderCriterion **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* sbISQLBuilderCriterion createMatchCriterionInt32 (in ACString aTableName, in ACString aSrcColumnName, in short aMatchType, in long aValue); */
NS_IMETHODIMP sbSQLBuilder::CreateMatchCriterionInt32(const nsACString & aTableName, const nsACString & aSrcColumnName, PRInt16 aMatchType, PRInt32 aValue, sbISQLBuilderCriterion **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* sbISQLBuilderCriterion createMatchCriterionNull (in ACString aTableName, in ACString aSrcColumnName, in short aMatchType); */
NS_IMETHODIMP sbSQLBuilder::CreateMatchCriterionNull(const nsACString & aTableName, const nsACString & aSrcColumnName, PRInt16 aMatchType, sbISQLBuilderCriterion **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* sbISQLBuilderCriterion createAndCriterion (in sbISQLBuilderCriterion aLeft, in sbISQLBuilderCriterion aRight); */
NS_IMETHODIMP sbSQLBuilder::CreateAndCriterion(sbISQLBuilderCriterion *aLeft, sbISQLBuilderCriterion *aRight, sbISQLBuilderCriterion **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

/* sbISQLBuilderCriterion createOrCriterion (in sbISQLBuilderCriterion aLeft, in sbISQLBuilderCriterion aRight); */
NS_IMETHODIMP sbSQLBuilder::CreateOrCriterion(sbISQLBuilderCriterion *aLeft, sbISQLBuilderCriterion *aRight, sbISQLBuilderCriterion **_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

