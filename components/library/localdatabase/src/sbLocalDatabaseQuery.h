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

#ifndef __SBLOCALDATABASEQUERY_H__
#define __SBLOCALDATABASEQUERY_H__

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <nsComponentManagerUtils.h>
#include <sbISQLBuilder.h>
#include "sbLocalDatabaseGUIDArray.h" // for FilterSpec

class sbLocalDatabaseQuery
{
public:

  sbLocalDatabaseQuery(const nsAString& aBaseTable,
                       const nsAString& aBaseConstraintColumn,
                       PRUint32 aBaseConstraintValue,
                       const nsAString& aBaseForeignKeyColumn,
                       const nsAString& aPrimarySortProperty,
                       PRBool aPrimarySortAscending,
                       nsTArray<FilterSpec>* aFilters);

  ~sbLocalDatabaseQuery();

  NS_IMETHODIMP GetFullCountQuery(nsAString& aQuery);
  NS_IMETHODIMP GetFullGuidRangeQuery(nsAString& aQuery);
  NS_IMETHODIMP GetNonNullCountQuery(nsAString& aQuery);
  NS_IMETHODIMP GetNullGuidRangeQuery(nsAString& aQuery);

private:

  NS_IMETHODIMP AddCountColumns();
  NS_IMETHODIMP AddGuidColumns(PRBool aIsNull);
  NS_IMETHODIMP AddBaseTable();
  NS_IMETHODIMP AddFilters();
  NS_IMETHODIMP AddRange();
  NS_IMETHODIMP AddPrimarySort();
  NS_IMETHODIMP AddNonNullPrimarySortConstraint();
  NS_IMETHODIMP AddJoinToGetNulls();

  NS_IMETHODIMP GetTopLevelPropertyColumn(const nsAString& aProperty,
                                          nsAString& columnName);
  PRInt32 GetPropertyId(const nsAString& aProperty);

  PRBool IsTopLevelProperty(const nsAString& aProperty);

  nsString mBaseTable;
  nsString mBaseConstraintColumn;
  PRUint32 mBaseConstraintValue;
  nsString mBaseForeignKeyColumn;
  nsString mPrimarySortProperty;
  PRBool mPrimarySortAscending;
  nsTArray<FilterSpec>* mFilters;

  nsCOMPtr<sbISQLSelectBuilder> mBuilder;
  PRBool mIsFullLibrary;
};

#endif /* __SBLOCALDATABASEQUERY_H__ */

