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

#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include "sbLocalDatabaseGUIDArray.h" // for FilterSpec

class sbIDatabaseQuery;
class sbILocalDatabasePropertyCache;
class sbISQLBuilder;

class sbLocalDatabaseQuery
{
  typedef nsTArray<PRUint32> sbUint32Array;

public:

  sbLocalDatabaseQuery(const nsAString& aBaseTable,
                       const nsAString& aBaseConstraintColumn,
                       PRUint32 aBaseConstraintValue,
                       const nsAString& aBaseForeignKeyColumn,
                       nsTArray<sbLocalDatabaseGUIDArray::FilterSpec>* aFilters,
                       nsTArray<sbLocalDatabaseGUIDArray::SortSpec>* aSorts,
                       PRBool aIsDistinct,
                       sbILocalDatabasePropertyCache* aPropertyCache);

  nsresult GetFullCountQuery(nsAString& aQuery);
  nsresult GetFullGuidRangeQuery(nsAString& aQuery);
  nsresult GetNonNullCountQuery(nsAString& aQuery);
  nsresult GetNullGuidRangeQuery(nsAString& aQuery);
  nsresult GetPrefixSearchQuery(nsAString& aQuery);
  nsresult GetResortQuery(nsAString& aQuery);
  nsresult GetNullResortQuery(nsAString& aQuery);

private:
  struct sbAddJoinInfo {
    sbAddJoinInfo(sbISQLSelectBuilder* aBuilder) :
      joinCounter(0),
      builder(aBuilder)
    {
      NS_ASSERTION(aBuilder, "aBuilder is null");
    };

    PRUint32 joinCounter;
    nsCOMPtr<sbISQLSelectBuilder> builder;
    nsCOMPtr<sbISQLBuilderCriterion> criterion;
  };

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
  nsresult AddResortColumns();
  nsresult AddMultiSorts();

  PRInt32 GetPropertyId(const nsAString& aProperty);

  static void MaxExpr(const nsAString& aAlias,
                      const nsAString& aColumn,
                      nsAString& aExpr);

  static PLDHashOperator PR_CALLBACK
    AddJoinSubqueryForSearchCallback(nsStringHashKey::KeyType aKey,
                                     sbUint32Array* aEntry,
                                     void* aUserData);

  nsString mBaseTable;
  nsString mBaseConstraintColumn;
  PRUint32 mBaseConstraintValue;
  nsString mBaseForeignKeyColumn;
  nsTArray<sbLocalDatabaseGUIDArray::FilterSpec>* mFilters;
  nsTArray<sbLocalDatabaseGUIDArray::SortSpec>* mSorts;
  PRBool mIsDistinct;

  nsCOMPtr<sbISQLSelectBuilder> mBuilder;
  PRBool mIsFullLibrary;
  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;
  PRBool mHasSearch;
};

#endif /* __SBLOCALDATABASEQUERY_H__ */

