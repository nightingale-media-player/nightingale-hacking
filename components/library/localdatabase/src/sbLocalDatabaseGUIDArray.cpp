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

#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabaseQuery.h"
#include "sbLocalDatabaseMediaItem.h"
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseSchemaInfo.h"

#include <DatabaseQuery.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsCOMPtr.h>
#include <nsIStringEnumerator.h>
#include <nsIURI.h>
#include <nsStringGlue.h>
#include <prlog.h>
#include <prprf.h>
#include <sbSQLBuilderCID.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbILibrary.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbISQLBuilder.h>
#include <sbTArrayStringEnumerator.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>

#define DEFAULT_FETCH_SIZE 20

#define COUNT_COLUMN NS_LITERAL_STRING("count(1)")
#define GUID_COLUMN NS_LITERAL_STRING("guid")
#define OBJSORTABLE_COLUMN NS_LITERAL_STRING("obj_sortable")
#define MEDIAITEMID_COLUMN NS_LITERAL_STRING("media_item_id")
#define PROPERTIES_TABLE NS_LITERAL_STRING("resource_properties")
#define MEDIAITEMS_TABLE NS_LITERAL_STRING("media_items")

#if defined PR_LOGGING
static const PRLogModuleInfo *gLocalDatabaseGUIDArrayLog = nsnull;
#define TRACE(args) PR_LOG(gLocalDatabaseGUIDArrayLog, PR_LOG_DEBUG, args)
#define LOG(args) PR_LOG(gLocalDatabaseGUIDArrayLog, PR_LOG_WARN, args)
#else
#define TRACE(args)
#define LOG(args)
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbLocalDatabaseGUIDArray, sbILocalDatabaseGUIDArray)

sbLocalDatabaseGUIDArray::sbLocalDatabaseGUIDArray() :
  mBaseConstraintValue(0),
  mFetchSize(DEFAULT_FETCH_SIZE),
  mLength(0),
  mIsDistinct(PR_FALSE),
  mValid(PR_FALSE),
  mNullsFirst(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gLocalDatabaseGUIDArrayLog) {
    gLocalDatabaseGUIDArrayLog = PR_NewLogModule("sbLocalDatabaseGUIDArray");
  }
#endif
}

sbLocalDatabaseGUIDArray::~sbLocalDatabaseGUIDArray()
{
  Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetDatabaseGUID(nsAString& aDatabaseGUID)
{
  aDatabaseGUID = mDatabaseGUID;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetDatabaseGUID(const nsAString& aDatabaseGUID)
{
  mDatabaseGUID = aDatabaseGUID;

  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetDatabaseLocation(nsIURI** aDatabaseLocation)
{
  NS_ENSURE_ARG_POINTER(aDatabaseLocation);

  // Okay if it's null.
  if (!mDatabaseLocation) {
    *aDatabaseLocation = nsnull;
    return NS_OK;
  }

  nsresult rv = mDatabaseLocation->Clone(aDatabaseLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetDatabaseLocation(nsIURI* aDatabaseLocation)
{
  mDatabaseLocation = aDatabaseLocation;

  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetBaseTable(nsAString& aBaseTable)
{
  aBaseTable = mBaseTable;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetBaseTable(const nsAString& aBaseTable)
{
  mBaseTable = aBaseTable;

  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetBaseConstraintColumn(nsAString& aBaseConstraintColumn)
{
  aBaseConstraintColumn = mBaseConstraintColumn;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetBaseConstraintColumn(const nsAString& aBaseConstraintColumn)
{
  mBaseConstraintColumn = aBaseConstraintColumn;

  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetBaseConstraintValue(PRUint32 *aBaseConstraintValue)
{
  NS_ENSURE_ARG_POINTER(aBaseConstraintValue);
  *aBaseConstraintValue = mBaseConstraintValue;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetBaseConstraintValue(PRUint32 aBaseConstraintValue)
{
  mBaseConstraintValue = aBaseConstraintValue;

  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetFetchSize(PRUint32 *aFetchSize)
{
  NS_ENSURE_ARG_POINTER(aFetchSize);
  *aFetchSize = mFetchSize;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetFetchSize(PRUint32 aFetchSize)
{
  NS_ENSURE_ARG_MIN(aFetchSize, 1);
  mFetchSize = aFetchSize;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetIsDistinct(PRBool *aIsDistinct)
{
  NS_ENSURE_ARG_POINTER(aIsDistinct);
  *aIsDistinct = mIsDistinct;
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetIsDistinct(PRBool aIsDistinct)
{
  mIsDistinct = aIsDistinct;
  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetListener(sbILocalDatabaseGUIDArrayListener** aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  NS_IF_ADDREF(*aListener = mListener);
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetListener(sbILocalDatabaseGUIDArrayListener* aListener)
{
  mListener = aListener;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetPropertyCache(sbILocalDatabasePropertyCache** aPropertyCache)
{
  NS_ENSURE_ARG_POINTER(aPropertyCache);
  *aPropertyCache = mPropertyCache;
  NS_ADDREF(*aPropertyCache);
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetPropertyCache(sbILocalDatabasePropertyCache* aPropertyCache)
{
  mPropertyCache = aPropertyCache;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetLength(PRUint32 *aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  nsresult rv;

  if (mValid == PR_FALSE) {
    rv = Initalize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aLength = mLength;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::AddSort(const nsAString& aProperty,
                                  PRBool aAscending)
{
  // TODO: Check for valid properties
  SortSpec* ss = mSorts.AppendElement();
  NS_ENSURE_TRUE(ss, NS_ERROR_OUT_OF_MEMORY);

  ss->property  = aProperty;
  ss->ascending = aAscending;

  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::ClearSorts()
{
  mSorts.Clear();

  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetCurrentSort(sbIPropertyArray** aCurrentSort)
{
  NS_ENSURE_ARG_POINTER(aCurrentSort);

  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> sort =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < mSorts.Length(); i++) {
    const SortSpec& ss = mSorts[i];
    rv = sort->AppendProperty(ss.property,
                              ss.ascending ? NS_LITERAL_STRING("a") :
                                NS_LITERAL_STRING("d"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aCurrentSort = sort);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::AddFilter(const nsAString& aProperty,
                                    nsIStringEnumerator *aValues,
                                    PRBool aIsSearch)
{
  // TODO: Check for valid properties
  NS_ENSURE_ARG_POINTER(aValues);

  nsresult rv;

  FilterSpec* fs = mFilters.AppendElement();
  NS_ENSURE_TRUE(fs, NS_ERROR_OUT_OF_MEMORY);

  fs->property = aProperty;
  fs->isSearch = aIsSearch;

  // Copy the values from the enumerator into an array
  PRBool hasMore;
  rv = aValues->HasMore(&hasMore);
  NS_ENSURE_SUCCESS(rv, rv);
  while (hasMore) {
    nsAutoString s;
    rv = aValues->GetNext(s);
    NS_ENSURE_SUCCESS(rv, rv);
    fs->values.AppendElement(s);
    rv = aValues->HasMore(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::ClearFilters()
{
  mFilters.Clear();

  return Invalidate();
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::IsIndexCached(PRUint32 aIndex,
                                        PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  if (aIndex < mCache.Length()) {
    ArrayItem* item = mCache[aIndex];
    if (item) {
      *_retval = PR_TRUE;
      return NS_OK;
    }
  }

  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetSortPropertyValueByIndex(PRUint32 aIndex,
                                                      nsAString& _retval)
{
  nsresult rv;

  ArrayItem* item;
  rv = GetByIndexInternal(aIndex, &item);
  if (rv == NS_ERROR_INVALID_ARG) {
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  _retval.Assign(item->sortPropertyValue);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetMediaItemIdByIndex(PRUint32 aIndex,
                                                PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  ArrayItem* item;
  rv = GetByIndexInternal(aIndex, &item);
  if (rv == NS_ERROR_INVALID_ARG) {
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = item->mediaItemId;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetOrdinalByIndex(PRUint32 aIndex,
                                            nsAString& _retval)
{
  nsresult rv;

  ArrayItem* item;
  rv = GetByIndexInternal(aIndex, &item);
  if (rv == NS_ERROR_INVALID_ARG) {
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  _retval.Assign(item->ordinal);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetGuidByIndex(PRUint32 aIndex,
                                         nsAString& _retval)
{
  nsresult rv;

  ArrayItem* item;
  rv = GetByIndexInternal(aIndex, &item);
  if (rv == NS_ERROR_INVALID_ARG) {
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  _retval.Assign(item->guid);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::Invalidate()
{
  if (mValid == PR_FALSE) {
    return NS_OK;
  }

  if (mListener) {
    mListener->OnBeforeInvalidate();
  }

  mCache.Clear();

  if(mPrimarySortKeyPositionCache.IsInitialized()) {
    mPrimarySortKeyPositionCache.Clear();
  }

  mValid = PR_FALSE;

  return NS_OK;
}

/**
 * Copy all the base properties of the guid array. This method will fail if any
 * of the clone operations fail.
 */
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::Clone(sbILocalDatabaseGUIDArray** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  sbLocalDatabaseGUIDArray* newArray;
  NS_NEWXPCOM(newArray, sbLocalDatabaseGUIDArray);
  NS_ENSURE_TRUE(newArray, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = CloneInto(newArray);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = newArray);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::CloneInto(sbILocalDatabaseGUIDArray* aDest)
{
  NS_ENSURE_ARG_POINTER(aDest);

  nsresult rv = aDest->SetDatabaseGUID(mDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDest->SetDatabaseLocation(mDatabaseLocation);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDest->SetBaseTable(mBaseTable);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDest->SetBaseConstraintColumn(mBaseConstraintColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDest->SetBaseConstraintValue(mBaseConstraintValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDest->SetFetchSize(mFetchSize);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDest->SetPropertyCache(mPropertyCache);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aDest->SetIsDistinct(mIsDistinct);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 sortCount = mSorts.Length();
  for (PRUint32 index = 0; index < sortCount; index++) {
    const SortSpec refSpec = mSorts.ElementAt(index);
    rv = aDest->AddSort(refSpec.property, refSpec.ascending);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 filterCount = mFilters.Length();
  for (PRUint32 index = 0; index < filterCount; index++) {
    const FilterSpec refSpec = mFilters.ElementAt(index);

    nsTArray<nsString>* stringArray =
      NS_CONST_CAST(nsTArray<nsString>*, &refSpec.values);
    NS_ENSURE_STATE(stringArray);

    nsCOMPtr<nsIStringEnumerator> enumerator =
      new sbTArrayStringEnumerator(stringArray);
    NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

    rv = aDest->AddFilter(refSpec.property, enumerator, refSpec.isSearch);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::RemoveByIndex(PRUint32 aIndex)
{
  nsresult rv;

  if (mValid == PR_FALSE) {
    rv = Initalize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ENSURE_TRUE(aIndex < mLength, NS_ERROR_INVALID_ARG);

  // Remove the specified element from the cache
  if (aIndex < mCache.Length()) {
    mCache.RemoveElementAt(aIndex);
  }

  // Adjust the null length of the array.  Made sure we decrement the non
  // null lengths only if the removed element lies within that area
  if (mNullsFirst) {
    if (aIndex < mNonNullLength) {
      mNonNullLength--;
    }
  }
  else {
    if (aIndex > mLength - mNonNullLength - 1) {
      mNonNullLength--;
    }
  }

  // Finally, adjust the size of the full array
  mLength--;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetFirstIndexByPrefix(const nsAString& aValue,
                                                PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  PRInt32 dbOk;

  if (mValid == PR_FALSE) {
    rv = Initalize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(mPrefixSearchQuery, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rowCount == 0) {
    *_retval = 0;
    return NS_OK;
  }

  nsAutoString indexStr;
  rv = result->GetRowCell(0, 0, indexStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 index;
  index = indexStr.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // If the result is equal to the non null length, we know it was not found
  if (index == mNonNullLength) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  // Check to see if the returned index actually starts with the requested
  // prefix
  nsAutoString value;
  rv = GetSortPropertyValueByIndex(index, value);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!StringBeginsWith(value, aValue)) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  *_retval = index;
  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::Initalize()
{
  nsresult rv;

  // Make sure we have a database and a base table
  if (mDatabaseGUID.IsEmpty() || mBaseTable.IsEmpty()) {
    return NS_ERROR_UNEXPECTED;
  }

  // Make sure we have at least one sort
  if (mSorts.Length() == 0) {
    return NS_ERROR_UNEXPECTED;
  }

  if (mValid == PR_TRUE) {
    rv = Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = UpdateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateLength();
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * Determine where to put null values based on the null sort policy of the
   * primary sort property and how it is being sorted
   */
  if (!mPropMan) {
    mPropMan = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIPropertyInfo> info;
  rv = mPropMan->GetPropertyInfo(mSorts[0].property, getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 nullSort;
  rv = info->GetNullSort(&nullSort);
  NS_ENSURE_SUCCESS(rv, rv);

  switch (nullSort) {
    case sbIPropertyInfo::SORT_NULL_SMALL:
      mNullsFirst = mSorts[0].ascending;
    break;
    case sbIPropertyInfo::SORT_NULL_BIG:
      mNullsFirst = !mSorts[0].ascending;
    break;
    case sbIPropertyInfo::SORT_NULL_FIRST:
      mNullsFirst = PR_TRUE;
    break;
    case sbIPropertyInfo::SORT_NULL_LAST:
      mNullsFirst = PR_FALSE;
    break;
  }

  if (mNullsFirst) {
    mQueryX  = mNullGuidRangeQuery;
    mQueryY  = mFullGuidRangeQuery;
    mLengthX = mLength - mNonNullLength;
  }
  else {
    mQueryX  = mFullGuidRangeQuery;
    mQueryY  = mNullGuidRangeQuery;
    mLengthX = mNonNullLength;
  }

  mValid = PR_TRUE;

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::UpdateLength()
{
  nsresult rv;

  rv = RunLengthQuery(mFullCountQuery, &mLength);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!mNonNullCountQuery.IsEmpty()) {
    rv = RunLengthQuery(mNonNullCountQuery, &mNonNullLength);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    mNonNullLength = mLength;
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::RunLengthQuery(const nsAString& aSql,
                                         PRUint32* _retval)
{
  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(aSql, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  // Execute the length query
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure we get one row back
  NS_ENSURE_TRUE(rowCount == 1, NS_ERROR_UNEXPECTED);

  nsAutoString countStr;
  rv = result->GetRowCell(0, 0, countStr);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = countStr.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::UpdateQueries()
{

  /*
   * Generate a SQL statement that applies the current filter, search, and
   * primary sort for the supplied base table and constraints.
   */
  nsresult rv;

  /*
   * We need four different queries to do the magic here:
   *
   * mFullCountQuery - A query that returns the count of the full dataset
   * with filters applied
   * mFullGuidRangeQuery - A query that returns a list of guids with filters
   * applied and sorted by the primary sort
   * mNonNullCountQuery - A query that returns the count of the rows in the
   * dataset whose primary sort key value is not null
   * mNullGuidRangeQuery - A query that returns a list of guids whose primary
   * sort key value is null with filters applied
   */
  nsAutoPtr<sbLocalDatabaseQuery> ldq;

  // If we have a property cache, pass it to the sbLocalDatabaseQuery.
  // Otherwise, create a query object for it to use
  if (mPropertyCache) {
    ldq = new sbLocalDatabaseQuery(mBaseTable,
                                   mBaseConstraintColumn,
                                   mBaseConstraintValue,
                                   NS_LITERAL_STRING("member_media_item_id"),
                                   mSorts[0].property,
                                   mSorts[0].ascending,
                                   &mFilters,
                                   mIsDistinct,
                                   mPropertyCache);
  }
  else {
    nsCOMPtr<sbIDatabaseQuery> query =
      do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->SetDatabaseGUID(mDatabaseGUID);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mDatabaseLocation) {
      rv = query->SetDatabaseLocation(mDatabaseLocation);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    ldq = new sbLocalDatabaseQuery(mBaseTable,
                                   mBaseConstraintColumn,
                                   mBaseConstraintValue,
                                   NS_LITERAL_STRING("member_media_item_id"),
                                   mSorts[0].property,
                                   mSorts[0].ascending,
                                   &mFilters,
                                   mIsDistinct,
                                   query);
  }

  rv = ldq->GetFullCountQuery(mFullCountQuery);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ldq->GetFullGuidRangeQuery(mFullGuidRangeQuery);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ldq->GetNonNullCountQuery(mNonNullCountQuery);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ldq->GetNullGuidRangeQuery(mNullGuidRangeQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // This query is used for the prefix search
  rv = ldq->GetPrefixSearchQuery(mPrefixSearchQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString sql;

  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * Generate the resort query, if needed
   */
  PRUint32 numSorts = mSorts.Length();
  if (numSorts > 1) {
    nsCOMPtr<sbISQLBuilderCriterion> criterion;

    rv = builder->AddColumn(NS_LITERAL_STRING("_base"), GUID_COLUMN);
    NS_ENSURE_SUCCESS(rv, rv);

    if (SB_IsTopLevelProperty(mSorts[0].property)) {
      rv = builder->SetBaseTableName(MEDIAITEMS_TABLE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = builder->SetBaseTableName(PROPERTIES_TABLE);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = builder->CreateMatchCriterionLong(NS_LITERAL_STRING("_base"),
                                             NS_LITERAL_STRING("property_id"),
                                             sbISQLSelectBuilder::MATCH_EQUALS,
                                             GetPropertyId(mSorts[0].property),
                                             getter_AddRefs(criterion));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->AddCriterion(criterion);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = builder->SetBaseTableAlias(NS_LITERAL_STRING("_base"));
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 1; i < numSorts; i++) {
      nsAutoString joinedAlias(NS_LITERAL_STRING("_p"));
      joinedAlias.AppendInt(i);

      nsCOMPtr<sbISQLBuilderCriterion> criterionGuid;
      rv = builder->CreateMatchCriterionTable(joinedAlias,
                                              GUID_COLUMN,
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              NS_LITERAL_STRING("_base"),
                                              GUID_COLUMN,
                                              getter_AddRefs(criterionGuid));
      NS_ENSURE_SUCCESS(rv, rv);

      if (SB_IsTopLevelProperty(mSorts[i].property)) {
        rv = builder->AddJoinWithCriterion(sbISQLSelectBuilder::JOIN_INNER,
                                           MEDIAITEMS_TABLE,
                                           joinedAlias,
                                           criterion);
        NS_ENSURE_SUCCESS(rv, rv);

        nsAutoString columnName;
        rv = SB_GetTopLevelPropertyColumn(mSorts[i].property, columnName);
        NS_ENSURE_SUCCESS(rv, rv);

        builder->AddOrder(joinedAlias,
                          columnName,
                          mSorts[i].ascending);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        nsCOMPtr<sbISQLBuilderCriterion> criterionProperty;
        rv = builder->CreateMatchCriterionLong(joinedAlias,
                                               NS_LITERAL_STRING("property_id"),
                                               sbISQLSelectBuilder::MATCH_EQUALS,
                                               GetPropertyId(mSorts[i].property),
                                               getter_AddRefs(criterionProperty));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->CreateAndCriterion(criterionGuid,
                                         criterionProperty,
                                         getter_AddRefs(criterion));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->AddJoinWithCriterion(sbISQLSelectBuilder::JOIN_LEFT,
                                           PROPERTIES_TABLE,
                                           joinedAlias,
                                           criterion);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->AddOrder(joinedAlias,
                               OBJSORTABLE_COLUMN,
                               mSorts[i].ascending);
        NS_ENSURE_SUCCESS(rv, rv);
      }

    }

    rv = builder->CreateMatchCriterionParameter(NS_LITERAL_STRING("_base"),
                                                OBJSORTABLE_COLUMN,
                                                sbISQLSelectBuilder::MATCH_EQUALS,
                                                getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    /*
     * Sort on the base table's guid column so make sure things are sorted the
     * same on all platforms
     */
    rv = builder->AddOrder(NS_LITERAL_STRING("_base"),
                           GUID_COLUMN,
                           PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->ToString(mResortQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->Reset();
    NS_ENSURE_SUCCESS(rv, rv);

    /*
     * Generate the null resort query
     */
    rv = builder->AddColumn(NS_LITERAL_STRING("_base"), GUID_COLUMN);
    NS_ENSURE_SUCCESS(rv, rv);

    if (SB_IsTopLevelProperty(mSorts[0].property)) {
      return NS_ERROR_NOT_IMPLEMENTED;
    }
    else {
      rv = builder->SetBaseTableName(MEDIAITEMS_TABLE);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->SetBaseTableAlias(NS_LITERAL_STRING("_base"));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbISQLBuilderCriterion> criterionGuid;
      rv = builder->CreateMatchCriterionTable(NS_LITERAL_STRING("_p0"),
                                              GUID_COLUMN,
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              NS_LITERAL_STRING("_base"),
                                              GUID_COLUMN,
                                              getter_AddRefs(criterionGuid));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbISQLBuilderCriterion> criterionProperty;
      rv = builder->CreateMatchCriterionLong(NS_LITERAL_STRING("_p0"),
                                             NS_LITERAL_STRING("property_id"),
                                             sbISQLSelectBuilder::MATCH_EQUALS,
                                             GetPropertyId(mSorts[0].property),
                                             getter_AddRefs(criterionProperty));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->CreateAndCriterion(criterionGuid,
                                       criterionProperty,
                                       getter_AddRefs(criterion));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->AddJoinWithCriterion(sbISQLSelectBuilder::JOIN_LEFT,
                                         PROPERTIES_TABLE,
                                         NS_LITERAL_STRING("_p0"),
                                         criterion);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->CreateMatchCriterionNull(NS_LITERAL_STRING("_p0"),
                                             OBJSORTABLE_COLUMN,
                                             sbISQLSelectBuilder::MATCH_EQUALS,
                                             getter_AddRefs(criterion));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->AddCriterion(criterion);
      NS_ENSURE_SUCCESS(rv, rv);

      for (PRUint32 i = 1; i < numSorts; i++) {
        nsAutoString joinedAlias(NS_LITERAL_STRING("_p"));
        joinedAlias.AppendInt(i);

        nsCOMPtr<sbISQLBuilderCriterion> criterionGuid;
        rv = builder->CreateMatchCriterionTable(joinedAlias,
                                                GUID_COLUMN,
                                                sbISQLSelectBuilder::MATCH_EQUALS,
                                                NS_LITERAL_STRING("_base"),
                                                GUID_COLUMN,
                                                getter_AddRefs(criterionGuid));
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbISQLBuilderCriterion> criterionProperty;
        rv = builder->CreateMatchCriterionLong(joinedAlias,
                                               NS_LITERAL_STRING("property_id"),
                                               sbISQLSelectBuilder::MATCH_EQUALS,
                                               GetPropertyId(mSorts[i].property),
                                               getter_AddRefs(criterionProperty));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->CreateAndCriterion(criterionGuid,
                                         criterionProperty,
                                         getter_AddRefs(criterion));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->AddJoinWithCriterion(sbISQLSelectBuilder::JOIN_LEFT,
                                           PROPERTIES_TABLE,
                                           joinedAlias,
                                           criterion);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = builder->AddOrder(joinedAlias,
                               OBJSORTABLE_COLUMN,
                               mSorts[i].ascending);
        NS_ENSURE_SUCCESS(rv, rv);

      }

    }

    /*
     * Sort on the base table's guid column so make sure things are sorted the
     * same on all platforms
     */
    rv = builder->AddOrder(NS_LITERAL_STRING("_base"),
                           GUID_COLUMN,
                           PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->ToString(mNullResortQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->Reset();
    NS_ENSURE_SUCCESS(rv, rv);

    /*
     * Generate the primary sort key position query
     */
    rv = builder->AddColumn(EmptyString(), COUNT_COLUMN);
    NS_ENSURE_SUCCESS(rv, rv);

    if (SB_IsTopLevelProperty(mSorts[0].property)) {
      rv = builder->SetBaseTableName(MEDIAITEMS_TABLE);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString columnName;
      rv = SB_GetTopLevelPropertyColumn(mSorts[0].property, columnName);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->CreateMatchCriterionParameter(EmptyString(),
                                                  columnName,
                                                  sbISQLSelectBuilder::MATCH_LESS,
                                                  getter_AddRefs(criterion));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->AddCriterion(criterion);
      NS_ENSURE_SUCCESS(rv, rv);

      builder->AddOrder(EmptyString(),
                        columnName,
                        mSorts[0].ascending);
      NS_ENSURE_SUCCESS(rv, rv);

    }
    else {
      rv = builder->SetBaseTableName(PROPERTIES_TABLE);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = builder->CreateMatchCriterionLong(EmptyString(),
                                             NS_LITERAL_STRING("property_id"),
                                             sbISQLSelectBuilder::MATCH_EQUALS,
                                             GetPropertyId(mSorts[0].property),
                                             getter_AddRefs(criterion));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->AddCriterion(criterion);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->CreateMatchCriterionParameter(EmptyString(),
                                                  OBJSORTABLE_COLUMN,
                                                  sbISQLSelectBuilder::MATCH_LESS,
                                                  getter_AddRefs(criterion));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = builder->AddCriterion(criterion);
      NS_ENSURE_SUCCESS(rv, rv);

      builder->AddOrder(EmptyString(),
                        OBJSORTABLE_COLUMN,
                        mSorts[0].ascending);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = builder->ToString(mPrimarySortKeyPositionQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->Reset();
    NS_ENSURE_SUCCESS(rv, rv);

  }

 return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::MakeQuery(const nsAString& aSql,
                                    sbIDatabaseQuery** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("sbLocalDatabaseGUIDArray[0x%.8x] -  MakeQuery: %s",
       this, NS_ConvertUTF16toUTF8(aSql).get()));

  nsresult rv;

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(mDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mDatabaseLocation) {
    rv = query->SetDatabaseLocation(mDatabaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->AddQuery(aSql);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = query);
  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::AddFiltersToQuery(sbISQLSelectBuilder *aBuilder,
                                            const nsAString& baseAlias)
{
  nsresult rv;

  nsAutoString baseTableAlias(baseAlias);

  PRUint32 len = mFilters.Length();
  for (PRUint32 i = 0; i < len; i++) {
    const FilterSpec& fs = mFilters[i];

    nsAutoString tableAlias;
    tableAlias.AppendLiteral("_p");
    tableAlias.AppendInt(i);

    PRBool isTopLevelProperty = SB_IsTopLevelProperty(fs.property);

    nsAutoString joinedTableName;
    // If the filtered property is a top level property, join the top level
    // table
    if (isTopLevelProperty) {
      joinedTableName = MEDIAITEMS_TABLE;
    }
    else {
      joinedTableName = PROPERTIES_TABLE;
    }

    if (i == 0) {

      // If the base table alias of the query is empty we should use the first
      // filter as the base table.  Otherwise join the first filter to the base
      // table
      if (baseTableAlias.IsEmpty()) {
        rv = aBuilder->SetBaseTableName(joinedTableName);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = aBuilder->SetBaseTableAlias(tableAlias);
        NS_ENSURE_SUCCESS(rv, rv);

        baseTableAlias = tableAlias;
      }
      else {
        rv = aBuilder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                               joinedTableName,
                               tableAlias,
                               GUID_COLUMN,
                               baseTableAlias,
                               GUID_COLUMN);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      rv = aBuilder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                             joinedTableName,
                             tableAlias,
                             GUID_COLUMN,
                             baseTableAlias,
                             GUID_COLUMN);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Add the critera
    nsCOMPtr<sbISQLBuilderCriterion> criterion;

    if (fs.isSearch) {

      // XXX: Lets not support search on top level properties
      if (isTopLevelProperty) {
        return NS_ERROR_INVALID_ARG;
      }

      // If a property is specified, add it here.  If this is empty, no
      // property contraint is added for this join which makes it search all
      // properties
      if(!fs.property.IsEmpty()) {
        rv = aBuilder->CreateMatchCriterionLong(tableAlias,
                                                NS_LITERAL_STRING("property_id"),
                                                sbISQLSelectBuilder::MATCH_EQUALS,
                                                GetPropertyId(fs.property),
                                                getter_AddRefs(criterion));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aBuilder->AddCriterion(criterion);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // Add the search term
      nsAutoString searchTerm;
      searchTerm.AppendLiteral("%");
      searchTerm.Append(fs.values[0]);
      searchTerm.AppendLiteral("%");

      rv = aBuilder->CreateMatchCriterionString(tableAlias,
                                                OBJSORTABLE_COLUMN,
                                                sbISQLSelectBuilder::MATCH_LIKE,
                                                searchTerm,
                                                getter_AddRefs(criterion));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = aBuilder->AddCriterion(criterion);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {

      nsCOMPtr<sbISQLBuilderCriterionIn> inCriterion;
      if (isTopLevelProperty) {

        // Add the constraint for the top level table.
        nsAutoString columnName;
        rv = SB_GetTopLevelPropertyColumn(fs.property, columnName);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aBuilder->CreateMatchCriterionIn(tableAlias,
                                              columnName,
                                              getter_AddRefs(inCriterion));
        NS_ENSURE_SUCCESS(rv, rv);
      }
      else {
        // Add the propery constraint for the filter
        rv = aBuilder->CreateMatchCriterionLong(tableAlias,
                                                NS_LITERAL_STRING("property_id"),
                                                sbISQLSelectBuilder::MATCH_EQUALS,
                                                GetPropertyId(fs.property),
                                                getter_AddRefs(criterion));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aBuilder->AddCriterion(criterion);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aBuilder->CreateMatchCriterionIn(tableAlias,
                                              OBJSORTABLE_COLUMN,
                                              getter_AddRefs(inCriterion));
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // For each filter value, add the term to an IN constraint
      PRUint32 numValues = fs.values.Length();
      for (PRUint32 j = 0; j < numValues; j++) {
        const nsAString& filterTerm = fs.values[j];
        rv = inCriterion->AddString(filterTerm);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      rv = aBuilder->AddCriterion(inCriterion);
      NS_ENSURE_SUCCESS(rv, rv);
    }

  }

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::AddPrimarySortToQuery(sbISQLSelectBuilder *aBuilder,
                                                const nsAString& baseAlias)
{
  nsresult rv;

  const SortSpec& ss = mSorts[0];

  /*
   * If this is a top level propery, simply add the sort
   */
  if (SB_IsTopLevelProperty(ss.property)) {
    nsAutoString columnName;
    rv = SB_GetTopLevelPropertyColumn(ss.property, columnName);
    NS_ENSURE_SUCCESS(rv, rv);

    /*
     * Add the output column for the sorted property so we can later use
     * this for additional sorting
     */
    rv = aBuilder->AddColumn(baseAlias,
                            columnName);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aBuilder->AddOrder(baseAlias,
                            columnName,
                            ss.ascending);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    /*
     * Add the output column for the sorted property so we can later use
     * this for additional sorting
     */
    rv = aBuilder->AddColumn(NS_LITERAL_STRING("_sort"),
                            OBJSORTABLE_COLUMN);
    NS_ENSURE_SUCCESS(rv, rv);

    /*
     * Join an instance of the properties table to the base table
     */
    rv = aBuilder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                           PROPERTIES_TABLE,
                           NS_LITERAL_STRING("_sort"),
                           GUID_COLUMN,
                           baseAlias,
                           GUID_COLUMN);
    NS_ENSURE_SUCCESS(rv, rv);

    /*
     * Restrict the sort table to the sort property
     */
    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    rv = aBuilder->CreateMatchCriterionLong(NS_LITERAL_STRING("_sort"),
                                            NS_LITERAL_STRING("property_id"),
                                            sbISQLSelectBuilder::MATCH_EQUALS,
                                            GetPropertyId(ss.property),
                                            getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aBuilder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    /*
     * Add a sort on the primary sort
     */
    rv = aBuilder->AddOrder(NS_LITERAL_STRING("_sort"),
                            OBJSORTABLE_COLUMN,
                            ss.ascending);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::FetchRows(PRUint32 aRequestedIndex)
{
  nsresult rv;

  /*
   * To read the full media library, two queries are used -- one for when the
   * primary sort key has values and one for when the primary sort key has
   * no values (null values).  When sorting, items where the primary sort key
   * has no values may be sorted to the beginning or to the end of the list.
   * The diagram below describes this situation:
   *
   * A                            B                                  C
   * +--------------X-------------+-----------------Y----------------+
   *                 +------------------------------+
   *                 D                              E
   *
   * Array AC (A is at index 0 and C is the maximum index of the array)
   * represents the entire media library.  Index B represents the first item
   * in the array that requires a different query as described above. Requests
   * for items in [A, B - 1] use query X, and requests for items in [B, C] use
   * query Y.
   *
   * Index range DE (inclusive) represents some sub-array within AC.  Depending
   * on the relationship between DE to index B, different strategies need to be
   * used to query the data:
   *
   * - If DE lies entirely within [A, B - 1], use query X to return the data
   * - If DE lies entirely within [B, C], use query Y to return the data
   * - If DE lies in both [A, B - 1] and [B, C], use query X to return the data
   *   in [A, B - 1] and Y to return data in[B, C]
   *
   * Note the start index of query Y must be relative to index B, not index A.
   */

  PRUint32 indexB = mLengthX;
  PRUint32 indexC = mLength - 1;

  /*
   * Divide the array up into cells and figure out what cell to fetch
   */
  PRUint32 cell = aRequestedIndex / mFetchSize;

  PRUint32 indexD = cell * mFetchSize;
  PRUint32 indexE = indexD + mFetchSize - 1;
  if (indexE > indexC) {
    indexE = indexC;
  }
  PRUint32 lengthDE = indexE - indexD + 1;

  /*
   * If DE lies entirely within [A, B - 1], use query X to return the data
   */
  if (indexE < indexB) {
    rv = ReadRowRange(mQueryX,
                      indexD,
                      lengthDE,
                      indexD,
                      mNullsFirst);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    /*
     * If DE lies entirely within [B, C], use query Y to return the data
     */
    if (indexD >= indexB) {
      rv = ReadRowRange(mQueryY,
                        indexD - indexB,
                        lengthDE,
                        indexD,
                        !mNullsFirst);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      /*
       * If DE lies in both [A, B - 1] and [B, C], use query X to return the
       * data in [A, B - 1] and Y to return data in[B, C]
       */
      rv = ReadRowRange(mQueryX,
                        indexD,
                        indexB - indexD,
                        indexD,
                        mNullsFirst);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = ReadRowRange(mQueryY,
                        0,
                        indexE - indexB + 1,
                        indexB,
                        !mNullsFirst);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  /*
   * If we're paired with a property cache, cache the proeprties for the
   * range we just fetched
   */
/*
   if (mPropertyCache) {
    const PRUnichar** guids = new const PRUnichar*[lengthDE];
    for (PRUint32 i = 0; i < lengthDE; i++) {
      guids[i] = mCache[i + indexD]->guid.get();
    }
    rv = mPropertyCache->CacheProperties(guids, lengthDE);
    NS_ENSURE_SUCCESS(rv, rv);
    delete[] guids;
  }
*/
return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::ReadRowRange(const nsAString& aSql,
                                       PRUint32 aStartIndex,
                                       PRUint32 aCount,
                                       PRUint32 aDestIndexOffset,
                                       PRBool aIsNull)
{
  nsresult rv;
  PRInt32 dbOk;

  NS_ENSURE_ARG_MIN(aStartIndex, 0);
  NS_ENSURE_ARG_MIN(aCount, 1);
  NS_ENSURE_ARG_MIN(aDestIndexOffset, 0);

  LOG(("ReadRowRange start %d count %d dest offset %d isnull %d\n",
            aStartIndex,
            aCount,
            aDestIndexOffset,
            aIsNull));

  /*
   * Set up the query with limit and offset parameters and run it
   */
  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(aSql, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(0, aCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(1, aStartIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * If there is a multi-level sort in effect, we need to apply the additional
   * level of sorts to this result
   */
  PRBool needsSorting = mSorts.Length() > 1;

  /*
   * Resize the cache so we can fit the new data
   */
  if (mCache.Length() < aDestIndexOffset + aCount) {
    LOG(("SetLength %d to %d",
              mCache.Length(),
              aDestIndexOffset + aCount));
    NS_ENSURE_TRUE(mCache.SetLength(aDestIndexOffset + aCount),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  nsAutoString lastSortedValue;
  PRUint32 firstIndex = 0;
  PRBool isFirstValue = PR_TRUE;
  PRBool isFirstSort = PR_TRUE;
  for (PRUint32 i = 0; i < rowCount; i++) {
    nsAutoString mediaItemIdStr;
    rv = result->GetRowCell(i, 0, mediaItemIdStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 mediaItemId = mediaItemIdStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUnichar* guid;
    rv = result->GetRowCellPtr(i, 1, &guid);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUnichar* value;
    rv = result->GetRowCellPtr(i, 2, &value);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUnichar* ordinal;
    rv = result->GetRowCellPtr(i, 3, &ordinal);
    NS_ENSURE_SUCCESS(rv, rv);

    ArrayItem* item = new ArrayItem(mediaItemId, guid, value, ordinal);
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

    NS_ENSURE_TRUE(mCache.ReplaceElementsAt(i + aDestIndexOffset, 1, item),
                   NS_ERROR_OUT_OF_MEMORY);

    TRACE(("ReplaceElementsAt %d %s", i + aDestIndexOffset,
               NS_ConvertUTF16toUTF8(item->guid).get()));
    if (needsSorting) {
      if (isFirstValue || !lastSortedValue.Equals(item->sortPropertyValue)) {
        if (!isFirstValue) {
          rv = SortRows(aDestIndexOffset + firstIndex,
                        aDestIndexOffset + i - 1,
                        lastSortedValue,
                        isFirstSort,
                        PR_FALSE,
                        PR_FALSE,
                        aIsNull);
          NS_ENSURE_SUCCESS(rv, rv);
          isFirstSort = PR_FALSE;
        }
        lastSortedValue.Assign(item->sortPropertyValue);
        firstIndex = i;
        isFirstValue = PR_FALSE;
      }
    }
  }

  LOG(("Replaced Elements %d to %d",
            aDestIndexOffset,
            rowCount + aDestIndexOffset));

  if (needsSorting) {
    rv = SortRows(aDestIndexOffset + firstIndex,
                  aDestIndexOffset + rowCount - 1,
                  lastSortedValue,
                  isFirstSort,
                  PR_TRUE,
                  isFirstSort == PR_TRUE,
                  aIsNull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  /*
   * If the number of rows returned is less than what was requested, this is
   * really bad.  Fill the rest in so we don't crash
   */
  if (rowCount < aCount) {
    char* message = PR_smprintf("Did not get the requested number of rows, "
                                "requested %d got %d", aCount, rowCount);
    NS_WARNING(message);
    PR_smprintf_free(message);
    for (PRUint32 i = 0; i < aCount - rowCount; i++) {
      ArrayItem* item = new ArrayItem(0,
                                      NS_LITERAL_STRING("error"),
                                      NS_LITERAL_STRING("error"),
                                      EmptyString());
      NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

      NS_ENSURE_TRUE(mCache.ReplaceElementsAt(i + rowCount + aDestIndexOffset, 1, item),
                     NS_ERROR_OUT_OF_MEMORY);
    }
  }
  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::SortRows(PRUint32 aStartIndex,
                                   PRUint32 aEndIndex,
                                   const nsAString& aKey,
                                   PRBool aIsFirst,
                                   PRBool aIsLast,
                                   PRBool aIsOnly,
                                   PRBool aIsNull)
{
  nsresult rv;
  PRInt32 dbOk;

  LOG(("Sorting rows %d to %d on %s, isfirst %d islast %d isonly %d isnull %d\n",
            aStartIndex,
            aEndIndex,
            NS_ConvertUTF16toUTF8(aKey).get(),
            aIsFirst,
            aIsLast,
            aIsOnly,
            aIsNull));

  /*
   * If this is only one row and it is not the first, last, and only row in the
   * window, we don't need to sort it
   */
  if (!aIsFirst && !aIsLast && !aIsOnly && aStartIndex == aEndIndex) {
    return NS_OK;
  }

  nsCOMPtr<sbIDatabaseQuery> query;
  if(aIsNull) {
    rv = MakeQuery(mNullResortQuery, getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = MakeQuery(mResortQuery, getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(0, aKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rangeLength = aEndIndex - aStartIndex + 1;

  /*
   * Make sure we get at least the number of rows back from the query that
   * we need to replace
   */
  if (rangeLength > rowCount) {
    return NS_ERROR_UNEXPECTED;
  }

  /*
   * Figure out the offset into the result set we should use to copy the query
   * results into the cache.  If the range we are sorting is equal to the
   * length of the entire array, we know we don't need an offset.
   */
  PRUint32 offset = 0;
  if(rangeLength != mLength) {
    /* If this is the only sort being done for a window (indicated when aIsOnly
     * is true), we have no reference point to determine the offset, so we must
     * query for it.
     */
    if (aIsOnly) {
      /*
       * If we are resorting a null range, we can use the cached non null length
       * to calculate the offset
       */
      if (aIsNull) {
        if (mNullsFirst) {
          offset = 0;
        }
        else {
          offset = aStartIndex - mNonNullLength;
        }
      }
      else {
        PRUint32 position;
        rv = GetPrimarySortKeyPosition(aKey, &position);
        NS_ENSURE_SUCCESS(rv, rv);

        offset = aStartIndex - position;
      }
    }
    else {
      /*
       * If this range is at the top of the window, set the offset such that
       * we will copy the tail end of the result set
       */
      if (aIsFirst) {
        offset = rowCount - rangeLength;
      }
      else {
        /*
         * Otherwise just copy the entire result set into the cache
         */
        offset = 0;
      }
    }
  }

  /*
   * Copy the rows from the query result into the cache starting at the
   * calculated offset
   */
  for (PRUint32 i = 0; i < rangeLength; i++) {
    PRUnichar* guid;
    rv = result->GetRowCellPtr(offset + i, 0, &guid);
    NS_ENSURE_SUCCESS(rv, rv);

    mCache[i + aStartIndex]->guid = guid;
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::GetPrimarySortKeyPosition(const nsAString& aValue,
                                                    PRUint32 *_retval)
{
  nsresult rv;

  /*
   * Make sure the cache is initalized
   */
  if (!mPrimarySortKeyPositionCache.IsInitialized()) {
    mPrimarySortKeyPositionCache.Init(100);
  }

  PRUint32 position;
  if (!mPrimarySortKeyPositionCache.Get(aValue, &position)) {
    PRInt32 dbOk;

    nsCOMPtr<sbIDatabaseQuery> query;
    rv = MakeQuery(mPrimarySortKeyPositionQuery, getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(0, aValue);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->Execute(&dbOk);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_SUCCESS(dbOk, dbOk);

    nsCOMPtr<sbIDatabaseResult> result;
    rv = query->GetResultObject(getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 rowCount;
    rv = result->GetRowCount(&rowCount);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(rowCount == 1, NS_ERROR_UNEXPECTED);

    nsAutoString countStr;
    rv = result->GetRowCell(0, 0, countStr);
    NS_ENSURE_SUCCESS(rv, rv);

    position = countStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mPrimarySortKeyPositionCache.Put(aValue, position);
  }

  *_retval = position;
  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::GetByIndexInternal(PRUint32 aIndex,
                                             ArrayItem** _retval)
{
  nsresult rv;

  TRACE(("GetByIndexInternal %d %d", aIndex, mLength));

  if (mValid == PR_FALSE) {
    rv = Initalize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ENSURE_TRUE(aIndex < mLength, NS_ERROR_INVALID_ARG);

  /*
   * Check to see if we have this index in cache
   */
  if (aIndex < mCache.Length()) {
    ArrayItem* item = mCache[aIndex];
    if (item) {
      TRACE(("Cache hit, got %s", NS_ConvertUTF16toUTF8(item->guid).get()));
      *_retval = item;
      return NS_OK;
    }
  }

  TRACE(("MISS"));

  /*
   * Cache miss
   * TODO: WE can be a little smarter here my moving the fetch window around
   * to read as many missing guids as we can but still satisfy this request.
   * Also we should probably track movement to make backwards scrolling more
   * efficient
   */
  rv = FetchRows(aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

   *_retval = mCache[aIndex];

  return NS_OK;
}

PRInt32
sbLocalDatabaseGUIDArray::GetPropertyId(const nsAString& aProperty)
{
  if (mPropertyCache) {
    return SB_GetPropertyId(aProperty, mPropertyCache);
  }
  else {
    nsresult rv;

    nsCOMPtr<sbIDatabaseQuery> query =
      do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, -1);

    rv = query->SetDatabaseGUID(mDatabaseGUID);
    NS_ENSURE_SUCCESS(rv,  -1);

    if (mDatabaseLocation) {
      rv = query->SetDatabaseLocation(mDatabaseLocation);
      NS_ENSURE_SUCCESS(rv,  -1);
    }

    return SB_GetPropertyId(aProperty, query);
  }
}

NS_IMPL_ISUPPORTS1(sbGUIDArrayEnumerator, nsISimpleEnumerator)

sbGUIDArrayEnumerator::sbGUIDArrayEnumerator(sbLocalDatabaseLibrary* aLibrary,
                                             sbILocalDatabaseGUIDArray* aArray) :
  mLibrary(aLibrary),
  mArray(aArray),
  mNextIndex(0)
{
}

sbGUIDArrayEnumerator::~sbGUIDArrayEnumerator()
{
}

NS_IMETHODIMP
sbGUIDArrayEnumerator::HasMoreElements(PRBool *_retval)
{
  nsresult rv;

  PRUint32 length;
  rv = mArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mNextIndex < length;
  return NS_OK;
}

NS_IMETHODIMP
sbGUIDArrayEnumerator::GetNext(nsISupports **_retval)
{
  nsresult rv;

  nsAutoString guid;
  rv = mArray->GetGuidByIndex(mNextIndex, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = mLibrary->GetMediaItem(guid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> supports = do_QueryInterface(item, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = supports);

  mNextIndex++;
  return NS_OK;
}

NS_IMPL_ISUPPORTS1(sbGUIDArrayStringEnumerator, nsIStringEnumerator)

sbGUIDArrayStringEnumerator::sbGUIDArrayStringEnumerator(sbILocalDatabaseGUIDArray* aArray) :
  mArray(aArray),
  mNextIndex(0)
{
  NS_ASSERTION(aArray, "Null value passed to ctor");
}

sbGUIDArrayStringEnumerator::~sbGUIDArrayStringEnumerator()
{
}

NS_IMETHODIMP
sbGUIDArrayStringEnumerator::HasMore(PRBool *_retval)
{
  nsresult rv;

  PRUint32 length;
  rv = mArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = mNextIndex < length;
  return NS_OK;
}

NS_IMETHODIMP
sbGUIDArrayStringEnumerator::GetNext(nsAString& _retval)
{
  nsresult rv;

  rv = mArray->GetGuidByIndex(mNextIndex, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  mNextIndex++;

  return NS_OK;
}

