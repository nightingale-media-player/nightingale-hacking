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
                       nsTArray<FilterSpec>* aFilters,
                       PRBool aIsDistinct);

  nsresult GetFullCountQuery(nsAString& aQuery);
  nsresult GetFullGuidRangeQuery(nsAString& aQuery);
  nsresult GetNonNullCountQuery(nsAString& aQuery);
  nsresult GetNullGuidRangeQuery(nsAString& aQuery);
  nsresult GetPrefixSearchQuery(nsAString& aQuery);

private:

  nsresult AddCountColumns();
  nsresult AddGuidColumns(PRBool aIsNull);
  nsresult AddBaseTable();
  nsresult AddFilters();
  nsresult AddRange();
  nsresult AddPrimarySort();
  nsresult AddNonNullPrimarySortConstraint();
  nsresult AddJoinToGetNulls();
  nsresult AddDistinctConstraint();
  nsresult AddDistinctGroupBy();

  nsresult GetTopLevelPropertyColumn(const nsAString& aProperty,
                                     nsAString& columnName);
  PRInt32 GetPropertyId(const nsAString& aProperty);

  PRBool IsTopLevelProperty(const nsAString& aProperty);

  static void MaxExpr(const nsAString& aAlias,
                      const nsAString& aColumn,
                      nsAString& aExpr);

  nsString mBaseTable;
  nsString mBaseConstraintColumn;
  PRUint32 mBaseConstraintValue;
  nsString mBaseForeignKeyColumn;
  nsString mPrimarySortProperty;
  PRBool mPrimarySortAscending;
  nsTArray<FilterSpec>* mFilters;
  PRBool mIsDistinct;

  nsCOMPtr<sbISQLSelectBuilder> mBuilder;
  PRBool mIsFullLibrary;
};

#endif /* __SBLOCALDATABASEQUERY_H__ */

