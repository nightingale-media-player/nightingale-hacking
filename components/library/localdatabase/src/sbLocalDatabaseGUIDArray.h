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

#ifndef __SBLOCALDATABASEGUIDARRAY_H__
#define __SBLOCALDATABASEGUIDARRAY_H__

#include "sbILocalDatabaseGUIDArray.h"
#include "sbILocalDatabasePropertyCache.h"

#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsCOMPtr.h>
#include <sbIDatabaseQuery.h>
#include <sbISQLBuilder.h>
#include <nsDataHashtable.h>
#include <nsISimpleEnumerator.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaItem.h>

struct FilterSpec {
  nsString property;
  nsTArray<nsString> values;
  PRBool isSearch;
};

class sbLocalDatabaseGUIDArray : public sbILocalDatabaseGUIDArray
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEGUIDARRAY

  sbLocalDatabaseGUIDArray();

private:

  struct ArrayItem {
    ArrayItem(PRUint32 aMediaItemId,
              const nsAString& aGuid,
              const nsAString& aValue) :
      mediaItemId(aMediaItemId),
      guid(aGuid),
      sortPropertyValue(aValue)
    {
    };

    ArrayItem(PRUint32 aMediaItemId,
              const PRUnichar* aGuid,
              const PRUnichar* aValue) :
      mediaItemId(aMediaItemId),
      guid(aGuid),
      sortPropertyValue(aValue)
    {
    };

    PRUint32 mediaItemId;
    nsString guid;
    nsString sortPropertyValue;
  };

  ~sbLocalDatabaseGUIDArray();

  nsresult Initalize();

  nsresult UpdateLength();

  nsresult RunLengthQuery(const nsAString& aSql,
                          PRUint32* _retval);

  nsresult UpdateQueries();

  PRBool IsTopLevelProperty(const nsAString& aProperty);

  nsresult GetPrimarySortKeyPosition(const nsAString& aValue,
                                     PRUint32 *_retval);

  nsresult GetTopLevelPropertyColumn(const nsAString& aProperty,
                                     nsAString& columnName);

  nsresult GetPropertyNullSort(const nsAString& aProperty,
                               PRUint32 *_retval);

  nsresult MakeQuery(const nsAString& aSql, sbIDatabaseQuery** _retval);

  nsresult AddFiltersToQuery(sbISQLSelectBuilder *aBuilder,
                             const nsAString& baseAlias);

  nsresult AddPrimarySortToQuery(sbISQLSelectBuilder *aBuilder,
                                 const nsAString& baseAlias);

  nsresult FetchRows(PRUint32 aRequestedIndex);

  nsresult SortRows(PRUint32 aStartIndex,
                    PRUint32 aEndIndex,
                    const nsAString& aKey,
                    PRBool aIsFirst,
                    PRBool aIsLast,
                    PRBool aIsOnly,
                    PRBool isNull);

  nsresult ReadRowRange(const nsAString& aSql,
                        PRUint32 aStartIndex,
                        PRUint32 aCount,
                        PRUint32 aDestIndexOffset,
                        PRBool isNull);

  nsresult GetByIndexInternal(PRUint32 aIndex, ArrayItem** _retval);

  PRInt32 GetPropertyId(const nsAString& aProperty);

  struct SortSpec {
    nsString property;
    PRBool ascending;
  };

  // Database GUID
  // XXX: This will probably change to a path?
  nsString mDatabaseGUID;

  // Query base table
  nsString mBaseTable;

  // Optional column constraint to use for base query
  nsString mBaseConstraintColumn;

  // Optional column constraint value to use for base query
  PRUint32 mBaseConstraintValue;

  // Number of rows to featch on cache miss
  PRUint32 mFetchSize;

  // Do things async
  PRBool mAsync;

  // Length of complete array
  PRUint32 mLength;

  // Length of non-null portion of the array
  PRUint32 mNonNullLength;

  // Is the cache valid
  PRBool mValid;

  // Current sort configuration
  nsTArray<SortSpec> mSorts;

  // Current filter configuration
  nsTArray<FilterSpec> mFilters;

  // Ordered array of GUIDs
  nsTArray<ArrayItem*> mCache;

  // Cache of primary sort key positions
  nsDataHashtable<nsStringHashKey, PRUint32> mPrimarySortKeyPositionCache;

  // Query used to count full length of the array
  nsString mFullCountQuery;

  // Query used to count full length of the array where the primary sort key
  // is null
  nsString mNonNullCountQuery;

  // Query used to return sorted GUIDs when the primary sort key is non-null
  nsString mFullGuidRangeQuery;

  // Query used to return sorted GUIDs when the primary sort key is null
  nsString mNullGuidRangeQuery;

  // Query used to resort chunks of results
  nsString mResortQuery;

  // Query used to resort a chunk of results with a null primary sort key
  nsString mNullResortQuery;

  // Query used to find the position of a primary sort key in the library
  nsString mPrimarySortKeyPositionQuery;

  // Cached versions of some of the above variables used my the fetch
  nsString mQueryX;
  nsString mQueryY;
  PRUint32 mLengthX;

  // How nulls are sorted
  PRBool mNullsFirst;

  // Paired property cache
  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

protected:
  /* additional members */
};

class sbGUIDArrayEnumerator : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  sbGUIDArrayEnumerator(sbILocalDatabaseLibrary* aLibrary,
                        sbILocalDatabaseGUIDArray* aArray);
private:
  ~sbGUIDArrayEnumerator();

  nsCOMPtr<sbILocalDatabaseLibrary> mLibrary;
  nsCOMPtr<sbILocalDatabaseGUIDArray> mArray;
  PRUint32 mNextIndex;
};

#endif /* __SBLOCALDATABASEGUIDARRAY_H__ */

