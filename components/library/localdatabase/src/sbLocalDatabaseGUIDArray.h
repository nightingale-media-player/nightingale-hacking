/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#ifndef __SBLOCALDATABASEGUIDARRAY_H__
#define __SBLOCALDATABASEGUIDARRAY_H__

#include "sbILocalDatabaseGUIDArray.h"
#include "sbILocalDatabasePropertyCache.h"
#include "sbLocalDatabaseLibrary.h"

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <sbIDatabaseQuery.h>
#include <sbISQLBuilder.h>
#include <nsISimpleEnumerator.h>
#include <nsIStringEnumerator.h>
#include <sbIDatabasePreparedStatement.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaItem.h>
#include <sbHashKeys.h>

class nsIURI;
class nsIWeakReference;
class sbILibrary;
class sbIPropertyManager;
class sbLocalDatabaseResourcePropertyBag;

class sbLocalDatabaseGUIDArray : public sbILocalDatabaseGUIDArray
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEGUIDARRAY

  sbLocalDatabaseGUIDArray();

  nsresult MayInvalidate(const nsAString &aGUID,
                         sbLocalDatabaseResourcePropertyBag *aBag);


  struct FilterSpec {
    nsString property;
    nsTArray<nsString> values;
    PRBool isSearch;
  };

  struct SortSpec {
    nsString property;
    PRUint32 propertyId;
    PRBool ascending;
    PRBool secondary;
  };

private:

  struct ArrayItem {
    ArrayItem(const ArrayItem& aOther) :
      mediaItemId(aOther.mediaItemId),
      guid(aOther.guid),
      sortPropertyValue(aOther.sortPropertyValue),
      ordinal(aOther.ordinal),
      rowid(aOther.rowid)
    {
      MOZ_COUNT_CTOR(ArrayItem);
    }

    ArrayItem(PRUint32 aMediaItemId,
              const nsAString& aGuid,
              const nsAString& aValue,
              const nsAString& aOrdinal,
              PRUint64 aRowid) :
      mediaItemId(aMediaItemId),
      guid(aGuid),
      sortPropertyValue(aValue),
      ordinal(aOrdinal),
      rowid(aRowid)
    {
      MOZ_COUNT_CTOR(ArrayItem);
    };

    ArrayItem(PRUint32 aMediaItemId,
              const PRUnichar* aGuid,
              const PRUnichar* aValue,
              const PRUnichar* aOrdinal,
              PRUint64 aRowid) :
      mediaItemId(aMediaItemId),
      guid(aGuid),
      sortPropertyValue(aValue),
      ordinal(aOrdinal),
      rowid(aRowid)
    {
      MOZ_COUNT_CTOR(ArrayItem);
    };

    ~ArrayItem()
    {
      MOZ_COUNT_DTOR(ArrayItem);
    }

    PRUint32 mediaItemId;
    nsString guid;
    nsString sortPropertyValue;
    nsString ordinal;
    PRUint64 rowid;
  };

  ~sbLocalDatabaseGUIDArray();

  nsresult Initialize();

  nsresult UpdateLength();

  nsresult RunLengthQuery(sbIDatabasePreparedStatement *aStatement,
                          PRUint32* _retval);

  nsresult UpdateQueries();

  nsresult GetPrimarySortKeyPosition(const nsAString& aValue,
                                     PRUint32 *_retval);

  nsresult MakeQuery(sbIDatabasePreparedStatement *aStatement, 
                     sbIDatabaseQuery** _retval);

  nsresult FetchRows(PRUint32 aRequestedIndex, PRUint32 aFetchSize);

  nsresult SortRows(PRUint32 aStartIndex,
                    PRUint32 aEndIndex,
                    const nsAString& aKey,
                    PRBool aIsFirst,
                    PRBool aIsLast,
                    PRBool aIsOnly,
                    PRBool isNull);

  nsresult ReadRowRange(sbIDatabasePreparedStatement *aStatement,
                        PRUint32 aStartIndex,
                        PRUint32 aCount,
                        PRUint32 aDestIndexOffset,
                        PRBool isNull);

  nsresult GetByIndexInternal(PRUint32 aIndex, ArrayItem** _retval);

  PRInt32 GetPropertyId(const nsAString& aProperty);

  static int SortBags(const void* a, const void* b, void* closure);

  nsresult AddSortInternal(const nsAString& aProperty,
                           PRBool aAscending,
                           PRBool aSecondary);
  nsresult ClearSecondarySorts();

  nsresult GetMTListener(sbILocalDatabaseGUIDArrayListener ** aListener);

  void QueryInvalidate() {
    mQueriesValid = PR_FALSE;
  }

  // Cached property manager
  nsCOMPtr<sbIPropertyManager> mPropMan;

  // Database GUID
  nsString mDatabaseGUID;

  // Database Location
  nsCOMPtr<nsIURI> mDatabaseLocation;

  // Query base table
  nsString mBaseTable;

  // Optional column constraint to use for base query
  nsString mBaseConstraintColumn;

  // Optional column constraint value to use for base query
  PRUint32 mBaseConstraintValue;

  // Number of rows to featch on cache miss
  PRUint32 mFetchSize;

  // Length of complete array
  PRUint32 mLength;

  // Length of non-null portion of the array
  PRUint32 mNonNullLength;

  // Current sort configuration
  nsTArray<SortSpec> mSorts;

  // Number of primary sorts
  PRInt32 mPrimarySortsCount;

  // Current filter configuration
  nsTArray<FilterSpec> mFilters;

  // Ordered array of GUIDs
  nsTArray<nsAutoPtr<ArrayItem> > mCache;

  // Cache of primary sort key positions
  nsDataHashtable<nsStringHashKey, PRUint32> mPrimarySortKeyPositionCache;

  // Query used to count full length of the array
  nsString mFullCountQuery;
  nsCOMPtr<sbIDatabasePreparedStatement> mFullCountStatement;

  // Query used to count full length of the array where the primary sort key
  // is null
  nsString mNonNullCountQuery;
  nsCOMPtr<sbIDatabasePreparedStatement> mNonNullCountStatement;

  // Query used to return sorted GUIDs when the primary sort key is non-null
  nsString mFullGuidRangeQuery;
  nsCOMPtr<sbIDatabasePreparedStatement> mFullGuidRangeStatement;

  // Query used to return sorted GUIDs when the primary sort key is null
  nsString mNullGuidRangeQuery;
  nsCOMPtr<sbIDatabasePreparedStatement> mNullGuidRangeStatement;

  // Query used to resort chunks of results
  nsString mResortQuery;
  nsCOMPtr<sbIDatabasePreparedStatement> mResortStatement;

  // Query used to resort a chunk of results with a null primary sort key
  nsString mNullResortQuery;
  nsCOMPtr<sbIDatabasePreparedStatement> mNullResortStatement;

  // Query used to search for the position of a value in the primary sort
  nsString mPrefixSearchQuery;
  nsCOMPtr<sbIDatabasePreparedStatement> mPrefixSearchStatement;


  // Query used to find the position of a primary sort key in the library
  nsString mPrimarySortKeyPositionQuery;
  nsCOMPtr<sbIDatabasePreparedStatement> mPrimarySortKeyPositionStatement;

  // Cached versions of some of the above variables used my the fetch
  nsCOMPtr<sbIDatabasePreparedStatement> mStatementX;
  nsCOMPtr<sbIDatabasePreparedStatement> mStatementY;
  PRUint32 mLengthX;

  // Our listener
  nsCOMPtr<nsIWeakReference> mListener;

  // Paired property cache
  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  // Map of guid -> first array index
  nsDataHashtable<nsStringHashKey, PRUint32> mGuidToFirstIndexMap;

  // Map of rowid -> array index
  nsDataHashtable<sbUint64HashKey, PRUint32> mRowidToIndexMap;

  // Get distinct values?
  PRPackedBool mIsDistinct;
  // Distinct values are the sortable versions and not human readable?
  PRPackedBool mDistinctWithSortableValues;

  // Is the cache valid
  PRPackedBool mValid;

  // Are the queries valid
  PRPackedBool mQueriesValid;

  // Is there a search filter with at least one active search term
  PRPackedBool mHasActiveSearch;

  // How nulls are sorted
  PRPackedBool mNullsFirst;

  // True if rows have been prefetched
  PRPackedBool mPrefetchedRows;

  // Counter for suppression of invalidation.
  PRUint32 mSuppress;
};

class sbGUIDArrayEnumerator : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  sbGUIDArrayEnumerator(sbLocalDatabaseLibrary* aLibrary,
                        sbILocalDatabaseGUIDArray* aArray);

  ~sbGUIDArrayEnumerator();

private:
  nsRefPtr<sbLocalDatabaseLibrary> mLibrary;
  nsCOMPtr<sbILocalDatabaseGUIDArray> mArray;

  PRUint32 mNextIndex;
  nsString mNextGUID;

  PRUint32 mPreviousLength;
};

class sbGUIDArrayStringEnumerator : public nsIStringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  sbGUIDArrayStringEnumerator(sbILocalDatabaseGUIDArray* aArray);

  ~sbGUIDArrayStringEnumerator();

private:
  nsCOMPtr<sbILocalDatabaseGUIDArray> mArray;
  PRUint32 mNextIndex;
};

#endif /* __SBLOCALDATABASEGUIDARRAY_H__ */

