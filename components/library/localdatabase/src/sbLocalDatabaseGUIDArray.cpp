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

#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabaseQuery.h"
#include "sbLocalDatabaseMediaItem.h"
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseResourcePropertyBag.h"
#include "sbLocalDatabaseSchemaInfo.h"
#include "sbLocalDatabaseLibrary.h"

#include <algorithm>

#include <DatabaseQuery.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsCOMPtr.h>
#include <nsIStringEnumerator.h>
#include <nsIURI.h>
#include <nsIWeakReference.h>
#include <nsMemory.h>
#include <nsStringGlue.h>
#include <prlog.h>
#include <prprf.h>
#include <sbSQLBuilderCID.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbILibrary.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyInfo.h>
#include <sbIPropertyManager.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbISQLBuilder.h>
#include <sbTArrayStringEnumerator.h>
#include <sbPropertiesCID.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbMemoryUtils.h>
#include <sbThreadUtils.h>
#include <mozilla/Monitor.h>

#define DEFAULT_FETCH_SIZE 20

// Fetch all guids asynchronously, disabled by default.
//#define FORCE_FETCH_ALL_GUIDS_ASYNC

#define COUNT_COLUMN NS_LITERAL_STRING("count(1)")
#define GUID_COLUMN NS_LITERAL_STRING("guid")
#define OBJSORTABLE_COLUMN NS_LITERAL_STRING("obj_sortable")
#define MEDIAITEMID_COLUMN NS_LITERAL_STRING("media_item_id")
#define PROPERTIES_TABLE NS_LITERAL_STRING("resource_properties")
#define MEDIAITEMS_TABLE NS_LITERAL_STRING("media_items")

#ifdef PR_LOGGING
static const PRLogModuleInfo *gLocalDatabaseGUIDArrayLog = nsnull;
#endif /* PR_LOGGING */

#define TRACE(args) PR_LOG(gLocalDatabaseGUIDArrayLog, PR_LOG_DEBUG, args)
#define LOG(args) PR_LOG(gLocalDatabaseGUIDArrayLog, PR_LOG_WARN, args)

NS_IMPL_THREADSAFE_ISUPPORTS2(sbLocalDatabaseGUIDArray,
                              sbILocalDatabaseGUIDArray,
                              nsISupportsWeakReference)

sbLocalDatabaseGUIDArray::sbLocalDatabaseGUIDArray() :
  mNeedNewKey(PR_FALSE),
  mPropIdsLock("sbLocalDatabaseGUIDArray::mPropIdsLock"),
  mBaseConstraintValue(0),
  mFetchSize(DEFAULT_FETCH_SIZE),
  mLength(0),
  mPrimarySortsCount(0),
  mCacheMonitor("sbLocalDatabaseGUIDArray::mCacheMonitor"),
  mIsDistinct(PR_FALSE),
  mDistinctWithSortableValues(PR_FALSE),
  mValid(PR_FALSE),
  mQueriesValid(PR_FALSE),
  mNullsFirst(PR_FALSE),
  mPrefetchedRows(PR_FALSE),
  mIsFullLibrary(PR_FALSE),
  mSuppress(0)
{
#ifdef PR_LOGGING
  if (!gLocalDatabaseGUIDArrayLog) {
    gLocalDatabaseGUIDArrayLog = PR_NewLogModule("sbLocalDatabaseGUIDArray");
  }
#endif
}

sbLocalDatabaseGUIDArray::~sbLocalDatabaseGUIDArray()
{
  // We need to remove the cached lengths when a guid array is destroyed
  // because it may have been used as a transient array for enumeration
  // of items in a medialist or library which will make the cached lengths
  // invalid.
  if(mLengthCache && !mCachedLengthKey.IsEmpty()) {
    mLengthCache->RemoveCachedLength(mCachedLengthKey);
    mLengthCache->RemoveCachedNonNullLength(mCachedLengthKey);
  }
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

  QueryInvalidate();

  return Invalidate(PR_FALSE);
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

  QueryInvalidate();

  return Invalidate(PR_FALSE);
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

  QueryInvalidate();

  return Invalidate(PR_FALSE);
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

  QueryInvalidate();

  return Invalidate(PR_FALSE);
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

  QueryInvalidate();

  return Invalidate(PR_FALSE);
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

  QueryInvalidate();

  return Invalidate(PR_FALSE);
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetIsValid(PRBool *aIsValid)
{
  NS_ENSURE_ARG_POINTER(aIsValid);
  *aIsValid = mValid;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetDistinctWithSortableValues(PRBool *aDistinctWithSortableValues)
{
  NS_ENSURE_ARG_POINTER(aDistinctWithSortableValues);
  *aDistinctWithSortableValues = mDistinctWithSortableValues;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetDistinctWithSortableValues(PRBool aDistinctWithSortableValues)
{
  mDistinctWithSortableValues = aDistinctWithSortableValues;

  QueryInvalidate();

  return Invalidate(PR_FALSE);
}


NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetListener(sbILocalDatabaseGUIDArrayListener** aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  if (mListener) {
    nsresult rv;
    nsCOMPtr<sbILocalDatabaseGUIDArrayListener> listener =
      do_QueryReferent(mListener, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (listener) {
      NS_ADDREF(*aListener = listener);
      return NS_OK;
    }
  }

  *aListener = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetListener(sbILocalDatabaseGUIDArrayListener* aListener)
{
  nsresult rv = NS_OK;

  if (aListener) {
    mListener = do_GetWeakReference(aListener, &rv);
  }
  else {
    mListener = nsnull;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetPropertyCache(sbILocalDatabasePropertyCache** aPropertyCache)
{
  NS_ENSURE_ARG_POINTER(aPropertyCache);
  NS_IF_ADDREF(*aPropertyCache = mPropertyCache);
  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetPropertyCache(sbILocalDatabasePropertyCache* aPropertyCache)
{
  // If we already had a property cache we need to remove the guid array from
  // the list of depedent guid arrays for the property cache.
  if(mPropertyCache) {
    mPropertyCache->RemoveDependentGUIDArray(this);
  }

  mPropertyCache = aPropertyCache;

  // Add dependent guid array
  if(mPropertyCache) {
    mPropertyCache->AddDependentGUIDArray(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetLength(PRUint32 *aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);
  nsresult rv;

  if (mValid == PR_FALSE) {
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aLength = mLength;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SetLengthCache(
        sbILocalDatabaseGUIDArrayLengthCache *aLengthCache)
{
  mLengthCache = aLengthCache;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetLengthCache(
        sbILocalDatabaseGUIDArrayLengthCache **aLengthCache)
{
  NS_ENSURE_ARG_POINTER(aLengthCache);

  NS_IF_ADDREF(*aLengthCache = mLengthCache);

  return NS_OK;
}

nsresult sbLocalDatabaseGUIDArray::AddSortInternal(const nsAString& aProperty,
                                                   PRBool aAscending,
                                                   PRBool aSecondary) {

  // TODO: Check for valid properties
  SortSpec* ss = mSorts.AppendElement();
  NS_ENSURE_TRUE(ss, NS_ERROR_OUT_OF_MEMORY);

  // Can't sort by ordinal for media_item table default to created
  if (aProperty.Equals(NS_LITERAL_STRING(SB_PROPERTY_ORDINAL)) &&
      !mBaseTable.Equals(NS_LITERAL_STRING("simple_media_lists"))) {
    NS_WARNING("Trying to sort by ordinal on a non-media list");
      ss->property = NS_LITERAL_STRING(SB_PROPERTY_CREATED);
  }
  else {
    ss->property   = aProperty;
  }
  ss->ascending  = aAscending;
  ss->secondary = aSecondary;

  if (mPropertyCache) {
    nsresult rv = mPropertyCache->GetPropertyDBID(aProperty, &ss->propertyId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult sbLocalDatabaseGUIDArray::ClearSecondarySorts() {
  for (PRUint32 i = 0; i < mSorts.Length(); i++) {
    const SortSpec& ss = mSorts[i];
    if (ss.secondary) {
      mSorts.RemoveElementAt(i);
      i--;
    }
  }
  return NS_OK;
}

namespace
{
  /**
   * Used to search the property ID array using a binary search algorithm
   */
  const PRUint32 * FindPropID(const PRUint32 * aPropIDs,
                              PRUint32 aCount,
                              PRUint32 aPropID)
  {
    const PRUint32 * result = std::lower_bound(aPropIDs, aPropIDs + aCount, aPropID);
    if (result != aPropIDs + aCount && *result == aPropID) {
      return result;
    }
    return aPropIDs + aCount;
  }
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::MayInvalidate(PRUint32 * aDirtyPropIDs,
                                        PRUint32 aCount)
{
  PRUint32 propertyDBID = 0;
  nsresult rv = NS_ERROR_UNEXPECTED;


  // First we check to see if we need to remove cached lengths.
  if (mLengthCache) {
    mozilla::MutexAutoLock mon(mPropIdsLock);
    for (PRUint32 index = 0; index < aCount; ++index) {
      std::set<PRUint32>::iterator itEntry =
          mPropIdsUsedInCacheKey.find(aDirtyPropIDs[index]);
      if(itEntry != mPropIdsUsedInCacheKey.end()) {
        mLengthCache->RemoveCachedLength(mCachedLengthKey);
        mLengthCache->RemoveCachedNonNullLength(mCachedLengthKey);
        break;
      }
    }
  }

  // Calc the end of the collection for iterator purposes
  const PRUint32 * const dirtyPropIDsEnd = aDirtyPropIDs + aCount;

  // Go through the filters and see if we should invalidate
  PRUint32 filterCount = mFilters.Length();
  for (PRUint32 index = 0; index < filterCount; index++) {
    const FilterSpec& refSpec = mFilters.ElementAt(index);

    rv = mPropertyCache->GetPropertyDBID(refSpec.property, &propertyDBID);
    if(NS_FAILED(rv)) {
      continue;
    }

    const PRUint32 * const found = FindPropID(aDirtyPropIDs,
                                              aCount,
                                              propertyDBID);

    if (found != dirtyPropIDsEnd) {
      // Return right away, no use in continuing if we're invalid.
      return Invalidate(PR_TRUE);
    }
  }

  // Go through the properties we use to sort to see if we should invalidate.
  PRUint32 sortCount = mSorts.Length();
  for (PRUint32 index = 0; index < sortCount; index++) {
    const SortSpec& sortSpec = mSorts.ElementAt(index);

    const PRUint32 * const found = FindPropID(aDirtyPropIDs,
                                              aCount,
                                              sortSpec.propertyId);
    if (found != dirtyPropIDsEnd) {
      // Return right away, no use in continuing if we're invalid.
      return Invalidate(PR_TRUE);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::AddSort(const nsAString& aProperty,
                                  PRBool aAscending)
{
  nsresult rv;

  // clear any existing secondary sort, appending this primary sort
  // will add its own secondary sorts.
  rv = ClearSecondarySorts();
  NS_ENSURE_SUCCESS(rv, rv);

  // add the primary sort
  rv = AddSortInternal(aProperty, aAscending, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  mPrimarySortsCount++;

  if (!mIsDistinct) {

    if (!mPropMan) {
      mPropMan = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // read the list of secondary sort properties for this primary sort
    nsCOMPtr<sbIPropertyInfo> propertyInfo;
    rv = mPropMan->GetPropertyInfo(aProperty,
                                   getter_AddRefs(propertyInfo));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIPropertyArray> secondarySortProperties;
    rv =
      propertyInfo->GetSecondarySort(getter_AddRefs(secondarySortProperties));
    NS_ENSURE_SUCCESS(rv, rv);

    if (secondarySortProperties) {
      PRUint32 secondarySortPropertyCount;
      rv = secondarySortProperties->GetLength(&secondarySortPropertyCount);
      NS_ENSURE_SUCCESS(rv, rv);

      // for all secondary sort properties, add the sort to mSorts
      for (PRUint32 i=0; i<secondarySortPropertyCount; i++) {
        nsCOMPtr<sbIProperty> property;
        rv = secondarySortProperties->GetPropertyAt(i, getter_AddRefs(property));
        // we cannot support secondary sort on toplevel properties, so skip them
        if (!SB_IsTopLevelProperty(aProperty)) {
          // don't completely fail AddSort when a dependent property is not found,
          // just don't add that secondary sort
          NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get dependent property!");
          if (NS_SUCCEEDED(rv)) {
            nsString propertyID;
            rv = property->GetId(propertyID);
            NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to get dependent property ID!");
            if (NS_SUCCEEDED(rv)) {
              nsString propertyValue;
              rv = property->GetValue(propertyValue);
              NS_ASSERTION(NS_SUCCEEDED(rv),
                           "Failed to get dependent property value!");
              if (NS_SUCCEEDED(rv)) {
                AddSortInternal(propertyID,
                                propertyValue.EqualsLiteral("a"),
                                PR_TRUE);
              }
            }
          }
        }
      }
    }
  }

  QueryInvalidate();

  return Invalidate(PR_FALSE);
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::ClearSorts()
{
  mSorts.Clear();

  mPrimarySortsCount = 0;

  QueryInvalidate();

  return Invalidate(PR_FALSE);
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetCurrentSort(sbIPropertyArray** aCurrentSort)
{
  NS_ENSURE_ARG_POINTER(aCurrentSort);

  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> sort =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sort->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < mSorts.Length(); i++) {
    const SortSpec& ss = mSorts[i];
    if (!ss.secondary) {
      rv = sort->AppendProperty(ss.property,
                                ss.ascending ? NS_LITERAL_STRING("a") :
                                NS_LITERAL_STRING("d"));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  NS_ADDREF(*aCurrentSort = sort);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::AddFilter(const nsAString& aProperty,
                                    nsIStringEnumerator *aValues,
                                    PRBool aIsSearch)
{
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
    nsString* success = fs->values.AppendElement(s);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    rv = aValues->HasMore(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  QueryInvalidate();

  return Invalidate(PR_FALSE);
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::ClearFilters()
{
  mFilters.Clear();

  QueryInvalidate();

  return Invalidate(PR_FALSE);
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::IsIndexCached(PRUint32 aIndex,
                                        PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  {
    mozilla::MonitorAutoLock mon(mCacheMonitor);
    if (aIndex < mCache.Length()) {
      ArrayItem* item = mCache[aIndex];
      if (item) {
        *_retval = PR_TRUE;
        return NS_OK;
      }
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
sbLocalDatabaseGUIDArray::GetRowidByIndex(PRUint32 aIndex,
                                          PRUint64* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  ArrayItem* item;
  rv = GetByIndexInternal(aIndex, &item);
  if (rv == NS_ERROR_INVALID_ARG) {
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  *_retval = item->rowid;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetViewItemUIDByIndex(PRUint32 aIndex,
                                                nsAString& _retval)
{
  nsresult rv;

  ArrayItem* item;
  rv = GetByIndexInternal(aIndex, &item);
  if (rv == NS_ERROR_INVALID_ARG) {
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  // the viewItemUID is just a concatenation of rowid and mediaitemid in the
  // form: "rowid-mediaitemid"
  _retval.Truncate();
  AppendInt(_retval, item->rowid);
  _retval.Append('-');
  _retval.AppendInt(item->mediaItemId);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::Invalidate(PRBool aInvalidateLength)
{
  TRACE(("sbLocalDatabaseGUIDArray[0x%.8x] - Invalidate", this));

  // Even if the GUID array is already invalid we'll invalidate the cached
  // length to ensure that everything is consistent. We need to do this because
  // certain operations don't invalidate the cached length which causes the array
  // to be in an invalid state. This same array is later invalidated again but
  // the length should also be invalidated.
  if (aInvalidateLength) {
    if (mLengthCache) {
      mLengthCache->RemoveCachedLength(mCachedLengthKey);
      mLengthCache->RemoveCachedNonNullLength(mCachedLengthKey);
    }

    // After we've invalidated we're likely to need a new key.
    mNeedNewKey = PR_TRUE;
  }

  if (mValid == PR_FALSE || mSuppress > 0) {
    return NS_OK;
  }
  nsresult rv;

  nsCOMPtr<sbILocalDatabaseGUIDArrayListener> listener;
  rv = GetMTListener(getter_AddRefs(listener));
  NS_ENSURE_SUCCESS(rv, rv);

  if (listener) {
    listener->OnBeforeInvalidate(aInvalidateLength);
  }

  // Scope for monitor.
  {
    mozilla::MonitorAutoLock mon(mCacheMonitor);

    mCache.Clear();
    mGuidToFirstIndexMap.Clear();
    mViewItemUIDToIndexMap.Clear();
    mPrefetchedRows = PR_FALSE;

    if (mPrimarySortKeyPositionCache.IsInitialized()) {
      mPrimarySortKeyPositionCache.Clear();
    }

    mValid = PR_FALSE;
  }

  rv = GetMTListener(getter_AddRefs(listener));
  NS_ENSURE_SUCCESS(rv, rv);
  if (listener) {
    listener->OnAfterInvalidate();
  }

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

  sbLocalDatabaseGUIDArray* newArray = new sbLocalDatabaseGUIDArray();
  NS_ENSURE_TRUE(newArray, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<sbILocalDatabaseGUIDArray> guidArray(newArray);
  nsresult rv = CloneInto(guidArray);
  NS_ENSURE_SUCCESS(rv, rv);

  guidArray.forget(_retval);

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

  rv = aDest->SetDistinctWithSortableValues(mDistinctWithSortableValues);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 sortCount = mSorts.Length();
  for (PRUint32 index = 0; index < sortCount; index++) {
    const SortSpec refSpec = mSorts.ElementAt(index);
    if (!refSpec.secondary) {
      rv = aDest->AddSort(refSpec.property, refSpec.ascending);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  PRUint32 filterCount = mFilters.Length();
  for (PRUint32 index = 0; index < filterCount; index++) {
    const FilterSpec refSpec = mFilters.ElementAt(index);

    nsTArray<nsString>* stringArray =
      const_cast<nsTArray<nsString>*>(&refSpec.values);
    NS_ENSURE_STATE(stringArray);

    nsCOMPtr<nsIStringEnumerator> enumerator =
      new sbTArrayStringEnumerator(stringArray);
    NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

    rv = aDest->AddFilter(refSpec.property, enumerator, refSpec.isSearch);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = aDest->SetLengthCache(mLengthCache);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::RemoveByIndex(PRUint32 aIndex)
{
  nsresult rv;

  mozilla::MonitorAutoLock mon(mCacheMonitor);

  if (mValid == PR_FALSE) {
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ENSURE_TRUE(aIndex < mLength, NS_ERROR_INVALID_ARG);

  // Remove the specified element from the cache
  {
    if (aIndex < mCache.Length()) {
      nsString guid;
      rv = GetGuidByIndex(aIndex, guid);
      NS_ENSURE_SUCCESS(rv, rv);
      mGuidToFirstIndexMap.Remove(guid);

      nsString viewItemUID;
      rv = GetViewItemUIDByIndex(aIndex, viewItemUID);
      NS_ENSURE_SUCCESS(rv, rv);

      mViewItemUIDToIndexMap.Remove(viewItemUID);

      mCache.RemoveElementAt(aIndex);
    }
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

  // Remove Cached Lengths
  if (mLengthCache) {
    mLengthCache->RemoveCachedLength(mCachedLengthKey);
    mLengthCache->RemoveCachedNonNullLength(mCachedLengthKey);
  }

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
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(mPrefixSearchStatement, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

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

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetFirstIndexByGuid(const nsAString& aGuid,
                                              PRUint32* _retval)
{
  TRACE(("sbLocalDatabaseGUIDArray[0x%.8x] - GetFirstIndexByGuid", this));

  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  mozilla::MonitorAutoLock mon(mCacheMonitor);

  if (mValid == PR_FALSE) {
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If this is a guid array on a simple media list we can't do any
  // optimizations that depend on returning the first matching guid we find
  PRBool uniqueGuids = PR_TRUE;
  if (mBaseTable.EqualsLiteral("simple_media_lists")) {
    uniqueGuids = PR_FALSE;
  }

  PRUint32 firstUncached = 0;

  // First check to see if the guid is cached.  If we have unique guids, we
  // can use the guid-to-index map as a shortcut
  if (uniqueGuids) {
    if (mGuidToFirstIndexMap.Get(aGuid, _retval)) {
      return NS_OK;
    }

    // If we are fully cached and the guid was not found, then we know that
    // it does not exist in this array
    if (mCache.Length() == mLength) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    // If it wasn't found, we need to find the first uncached row
    PRBool found = PR_FALSE;
    for (PRUint32 i = 0; !found && i < mCache.Length(); i++) {
      if (!mCache[i]) {
        firstUncached = i;
        found = PR_TRUE;
      }
    }

    // If we didn't find any uncached rows and the cache size is the same as
    // the array length, we know the guid isn't in this array
    if (!found && mCache.Length() == mLength) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }
  else {
    // Since we could have duplicate guids, just search the array for the guid
    // from the beginning to the first uncached item
    PRBool foundFirstUncached = PR_FALSE;
    for (PRUint32 i = 0; !foundFirstUncached && i < mCache.Length(); i++) {
      ArrayItem* item = mCache[i];
      if (item) {
        if (item->guid.Equals(aGuid)) {
          *_retval = i;
          return NS_OK;
        }
      }
      else {
        firstUncached = i;
        foundFirstUncached = PR_TRUE;
      }
    }

    // If we didn't find any uncached rows and the cache size is the same as
    // the array length, we know the guid isn't in this array
    if (!foundFirstUncached && mCache.Length() == mLength) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  // So the guid we are looking for is not cached.  Cache the rest of the
  // array and search it
  rv = FetchRows(firstUncached, mLength);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(mLength == mCache.Length(), "Full read didn't work");

  // Either the guid is in the map or it just not in our array
  if (mGuidToFirstIndexMap.Get(aGuid, _retval)) {
    return NS_OK;
  }

  return NS_ERROR_NOT_AVAILABLE;
}

/* aViewItemUID is a concatenation of a mediaitems rowid and mediaitemid from
 * it's entry in the library's database of the form "rowid-mediaitemid" */
NS_IMETHODIMP
sbLocalDatabaseGUIDArray::GetIndexByViewItemUID
                          (const nsAString& aViewItemUID,
                                          PRUint32* _retval)
{
  TRACE(("sbLocalDatabaseGUIDArray[0x%.8x] - GetIndexByRowid", this));
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  mozilla::MonitorAutoLock mon(mCacheMonitor);

  if (mValid == PR_FALSE) {
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // First check to see if we have this in cache
  if (mViewItemUIDToIndexMap.Get(aViewItemUID, _retval)) {
    return NS_OK;
  }

  PRUint32 firstUncached = 0;

  // If no, we need to cache the entire guid array.  Find the first uncached
  // row so we can trigger the load
  PRBool found = PR_FALSE;
  for (PRUint32 i = 0; !found && i < mCache.Length(); i++) {
    if (!mCache[i]) {
      firstUncached = i;
      found = PR_TRUE;
    }
  }

  // If all rows are cached, it was not found
  if (!found && mLength == mCache.Length()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  rv = FetchRows(firstUncached, mLength);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(mLength == mCache.Length(), "Full read didn't work");

  // Either the guid is in the map or it just not in our array
  if (mViewItemUIDToIndexMap.Get(aViewItemUID, _retval)) {
    return NS_OK;
  }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::ContainsGuid(const nsAString& aGuid,
                                       PRBool* _retval)
{
  TRACE(("sbLocalDatabaseGUIDArray[0x%.8x] - ContainsGuid", this));
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv;

  mozilla::MonitorAutoLock mon(mCacheMonitor);

  if (mValid == PR_FALSE) {
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Since we don't actually care where in the array the GUID appears,
  // we can take advantage of mGuidToFirstIndexMap even when this
  // is NOT a distinct array.

  // First check to see if the guid is cached.
  PRUint32 index;
  if (mGuidToFirstIndexMap.Get(aGuid, &index)) {
    *_retval = PR_TRUE;
    return NS_OK;
  }

  PRUint32 firstUncached = 0;
  // If we are fully cached and the guid was not found, then we know that
  // it does not exist in this array
  if (mCache.Length() == mLength) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  // If it wasn't found, we need to find the first uncached row
  PRBool found = PR_FALSE;
  for (PRUint32 i = 0; !found && i < mCache.Length(); i++) {
    if (!mCache[i]) {
      firstUncached = i;
      found = PR_TRUE;
    }
  }

  // If we didn't find any uncached rows and the cache size is the same as
  // the array length, we know the guid isn't in this array
  if (!found && mCache.Length() == mLength) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  // So the guid we are looking for is not cached.  Cache the rest of the
  // array and search it
  rv = FetchRows(firstUncached, mLength);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ASSERTION(mLength == mCache.Length(), "Full read didn't work");

  // Either the guid is in the map or it is just not in our array
  *_retval = mGuidToFirstIndexMap.Get(aGuid, &index);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseGUIDArray::SuppressInvalidation(PRBool aSuppress)
{
  if(aSuppress) {
    mSuppress++;
  }
  else if(--mSuppress <= 0) {
    mSuppress = 0;

    // We don't need to invalidate the lengths here because the only
    // time suppress invalidation is used is when the cascade filters
    // are being rebuilt. This means no new property values will show
    // up and no new items will be created in the library associated
    // with the GUID array.
    nsresult rv = Invalidate(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::Initialize()
{
  TRACE(("sbLocalDatabaseGUIDArray[0x%.8x] - Initialize", this));
  NS_ASSERTION(mPropertyCache, "No property cache!");

  nsresult rv = NS_ERROR_UNEXPECTED;

  // Make sure we have a database and a base table
  if (mDatabaseGUID.IsEmpty() || mBaseTable.IsEmpty()) {
    return NS_ERROR_UNEXPECTED;
  }

  // Make sure we have at least one sort
  if (mSorts.Length() == 0) {
    return NS_ERROR_UNEXPECTED;
  }

  if (!mGuidToFirstIndexMap.IsInitialized()) {
    PRBool success = mGuidToFirstIndexMap.Init();
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  if (!mViewItemUIDToIndexMap.IsInitialized()) {
    PRBool success = mViewItemUIDToIndexMap.Init();
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  if (mValid == PR_TRUE) {
    // There's no cached length information associated with the array
    // when it's being initialized (or re-initialized).
    rv = Invalidate(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mPropertyCache->Write();
  NS_ENSURE_SUCCESS(rv, rv);

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
    mStatementX  = mNullGuidRangeStatement;
    mStatementY  = mFullGuidRangeStatement;
    mLengthX = mLength - mNonNullLength;
  }
  else {
    mStatementX  = mFullGuidRangeStatement;
    mStatementY  = mNullGuidRangeStatement;
    mLengthX = mNonNullLength;
  }

  // Figure out if there is an active search filter,
  // as this impacts how we do secondary sorting at the moment.
  mHasActiveSearch = PR_FALSE;
  PRUint32 filterCount = mFilters.Length();
  for (PRUint32 index = 0; index < filterCount; index++) {
    const FilterSpec& refSpec = mFilters.ElementAt(index);

    nsTArray<nsString>* stringArray =
      const_cast<nsTArray<nsString>*>(&refSpec.values);
    NS_ENSURE_STATE(stringArray);

    if (refSpec.isSearch && stringArray->Length() > 0) {
      mHasActiveSearch = PR_TRUE;
      break;
    }
  }

  mValid = PR_TRUE;

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::UpdateLength()
{
  nsresult rv;

  mozilla::MonitorAutoLock mon(mCacheMonitor);

  // If we have a fetch size of 0 or PR_UINT32_MAX it means
  // we're supposed to fetch everything.  If this is
  // the case, and we don't have to worry about the
  // non null query, then we can skip the count query by
  // just fetching and using the resulting length.
  if ((mFetchSize == PR_UINT32_MAX || mFetchSize == 0) &&
      mNonNullCountQuery.IsEmpty() && mNullGuidRangeQuery.IsEmpty())
  {
    rv = ReadRowRange(mFullGuidRangeStatement,
                      0,
                      PR_UINT32_MAX,
                      0,
                      PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    mLength = mCache.Length();
    mNonNullLength = mLength;
  }
  else {
    // Try and get the cached length first.
    if (mCachedLengthKey.IsEmpty() || mNeedNewKey) {
      GenerateCachedLengthKey();
      mNeedNewKey = PR_FALSE;
    }
    if (mLengthCache) {
      rv = mLengthCache->GetCachedLength(mCachedLengthKey, &mLength);
      // Not in cache, run the query.
      if(NS_FAILED(rv)) {
        // Otherwise, use separate queries to establish the length.
        rv = RunLengthQuery(mFullCountStatement, &mLength);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mLengthCache->AddCachedLength(mCachedLengthKey, mLength);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      rv = RunLengthQuery(mFullCountStatement, &mLength);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // We need to get the non-null count separately.
    if (!mNonNullCountQuery.IsEmpty()) {
      if (mLengthCache) {
        rv = mLengthCache->GetCachedNonNullLength(mCachedLengthKey,
                                                  &mNonNullLength);
        // Not in cache, run the query.
        if(NS_FAILED(rv)) {
          rv = RunLengthQuery(mNonNullCountStatement, &mNonNullLength);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = mLengthCache->AddCachedNonNullLength(mCachedLengthKey, mLength);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
      else {
        rv = RunLengthQuery(mNonNullCountStatement, &mNonNullLength);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else {
      mNonNullLength = mLength;
    }
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::RunLengthQuery(sbIDatabasePreparedStatement *aStatement,
                                         PRUint32* _retval)
{
  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(aStatement, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  // Execute the length query
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

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
  // No need to update the queries, they're still valid
  if (mQueriesValid) {
    return NS_OK;
  }
  /*
   * Generate a SQL statement that applies the current filter, search, and
   * primary sort for the supplied base table and constraints.
   */
  nsresult rv;

  /*
   * We're going to use this query to prepare the sql statements.
   * This speeds things up _significantly_
   */
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(mDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mDatabaseLocation) {
    rv = query->SetDatabaseLocation(mDatabaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

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

  ldq = new sbLocalDatabaseQuery(mBaseTable,
                                 mBaseConstraintColumn,
                                 mBaseConstraintValue,
                                 NS_LITERAL_STRING("member_media_item_id"),
                                 &mFilters,
                                 &mSorts,
                                 mIsDistinct,
                                 mDistinctWithSortableValues,
                                 mPropertyCache);

  // Full Count Query
  rv = ldq->GetFullCountQuery(mFullCountQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(mFullCountQuery,
                           getter_AddRefs(mFullCountStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  // Full Guid Range Query
  rv = ldq->GetFullGuidRangeQuery(mFullGuidRangeQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(mFullGuidRangeQuery,
                           getter_AddRefs(mFullGuidRangeStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  // Non Null Count Query
  rv = ldq->GetNonNullCountQuery(mNonNullCountQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(mNonNullCountQuery,
                           getter_AddRefs(mNonNullCountStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  // Null Guid Range Query
  rv = ldq->GetNullGuidRangeQuery(mNullGuidRangeQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(mNullGuidRangeQuery,
                           getter_AddRefs(mNullGuidRangeStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  // Prefix Search Query
  rv = ldq->GetPrefixSearchQuery(mPrefixSearchQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(mPrefixSearchQuery,
                           getter_AddRefs(mPrefixSearchStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * Generate the resort query, if needed.
   * Only multiple primary sorts need a resort query, since any number of
   * secondary sorts are otherwise handled by the main query via
   * obj_secondary_sortable
   */
  PRUint32 numSorts = mSorts.Length();
  if (numSorts > 1 && !mIsDistinct) {
    rv = ldq->GetResortQuery(mResortQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->PrepareQuery(mResortQuery,
                             getter_AddRefs(mResortStatement));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ldq->GetNullResortQuery(mNullResortQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->PrepareQuery(mNullResortQuery,
                             getter_AddRefs(mNullResortStatement));
    NS_ENSURE_SUCCESS(rv, rv);

    /*
     * Generate the primary sort key position query
     */
    rv = ldq->GetPrefixSearchQuery(mPrimarySortKeyPositionQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->PrepareQuery(mPrimarySortKeyPositionQuery,
                             getter_AddRefs(mPrimarySortKeyPositionStatement));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mIsFullLibrary = ldq->GetIsFullLibrary();

  mQueriesValid = PR_TRUE;

  // If we're updating queries, it's likely we'll need to update the key
  // we're using so re-generate now.
  GenerateCachedLengthKey();

  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::MakeQuery(sbIDatabasePreparedStatement *aStatement,
                                    sbIDatabaseQuery** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

#ifdef PR_LOGGING
  nsString queryString;
  rv = aStatement->GetQueryString(queryString);
  if (NS_SUCCEEDED(rv)) {
    LOG(("sbLocalDatabaseGUIDArray[0x%.8x] -  MakeQuery: %s",
         this, NS_ConvertUTF16toUTF8(queryString).get()));
  }
#endif

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(mDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mDatabaseLocation) {
    rv = query->SetDatabaseLocation(mDatabaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->AddPreparedStatement(aStatement);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = query);
  return NS_OK;
}

nsresult
sbLocalDatabaseGUIDArray::FetchRows(PRUint32 aRequestedIndex,
                                    PRUint32 aFetchSize)
{
  nsresult rv;

  // FetchRows always gets called with mCacheMonitor already acquired!
  // No need to acquire the lock in this method.

  /*
   * Nothing to fetch if not valid
   */
  if (mValid == PR_FALSE)
    return NS_OK;

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

  // Edge cases mean fetch everything.  Just return if nothing to fetch.
  if (aFetchSize == PR_UINT32_MAX || aFetchSize == 0) {
    aFetchSize = mLength;
  }
  if (!aFetchSize)
    return NS_OK;

  /*
   * Divide the array up into cells and figure out what cell to fetch
   */
  PRUint32 cell = aRequestedIndex / aFetchSize;

  PRUint32 indexD = cell * aFetchSize;
  PRUint32 indexE = indexD + aFetchSize - 1;
  if (indexE > indexC) {
    indexE = indexC;
  }
  PRUint32 lengthDE = indexE - indexD + 1;

  /*
   * If DE lies entirely within [A, B - 1], use query X to return the data
   */
  if (indexE < indexB) {
    rv = ReadRowRange(mStatementX,
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
      rv = ReadRowRange(mStatementY,
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
      rv = ReadRowRange(mStatementX,
                        indexD,
                        indexB - indexD,
                        indexD,
                        mNullsFirst);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = ReadRowRange(mStatementY,
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
sbLocalDatabaseGUIDArray::ReadRowRange(sbIDatabasePreparedStatement *aStatement,
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

  // ReadRowRange always gets called with mCacheMonitor acquired!
  // No need to acquire this lock in this method.

  /*
   * Set up the query with limit and offset parameters and run it
   */
  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(aStatement, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt64Parameter(0, aCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(1, aStartIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  /*
   * If asked to fetch everything, assume we got
   * the right number of rows.
   */
  if (aCount == PR_UINT32_MAX) {
    aCount = rowCount;
  }

  /*
   * We need to apply additional levels of sorts if either of the following
   * conditions is true:
   * - Multiple sorts have been added via AddSort
   * - A single primary sort is active but it has secondary sorts and we are
   *   processing the null values: null values mean there was no row to hold
   *   the secondary sort data
   */
  PRBool needsSorting = (mPrimarySortsCount > 1) ||
                        (mSorts.Length() > 1 && aIsNull);

  /*
   * Resize the cache so we can fit the new data
   */
  if (mCache.Length() < aDestIndexOffset + aCount) {
    LOG(("SetLength %d to %d", mCache.Length(), aDestIndexOffset + aCount));
    PRBool success = mCache.SetLength(aDestIndexOffset + aCount);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  nsAutoString lastSortedValue;
  PRUint32 firstIndex = 0;
  PRBool isFirstValue = PR_TRUE;
  PRBool isFirstSort = PR_TRUE;
  for (PRUint32 i = 0; i < rowCount; i++) {
    PRUint32 index = i + aDestIndexOffset;

    nsString mediaItemIdStr;
    rv = result->GetRowCell(i, 0, mediaItemIdStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 mediaItemId = mediaItemIdStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString guid;
    rv = result->GetRowCell(i, 1, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString value;
    rv = result->GetRowCell(i, 2, value);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString ordinal;
    rv = result->GetRowCell(i, 3, ordinal);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString rowidStr;
    rv = result->GetRowCell(i, 4, rowidStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint64 rowid = nsString_ToUint64(rowidStr, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    ArrayItem* item = new ArrayItem(mediaItemId, guid, value, ordinal, rowid);
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

    nsAutoPtr<ArrayItem>* success =
      mCache.ReplaceElementsAt(index, 1, item);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    TRACE(("ReplaceElementsAt %d %s", index,
           NS_ConvertUTF16toUTF8(item->guid).get()));

    if (needsSorting) {
      if (isFirstValue || !lastSortedValue.Equals(item->sortPropertyValue)) {
        if (!isFirstValue) {
          rv = SortRows(aDestIndexOffset + firstIndex,
                        index - 1,
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

  // Record the indexes of the guids
  for (PRUint32 i = 0; i < rowCount; i++) {
    PRUint32 index = i + aDestIndexOffset;

    ArrayItem* item = mCache[index];
    NS_ASSERTION(item, "Null item in cache?");

    // Add the new guid to the guid to first index map.
    PRUint32 firstGuidIndex;
    PRBool found = mGuidToFirstIndexMap.Get(item->guid, &firstGuidIndex);
    if (!found || index < firstGuidIndex) {
      PRBool added = mGuidToFirstIndexMap.Put(item->guid, index);
      NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
    }

    // Add the concatenated rowid and mediaitemid (a viewItemUID)
    // as a key mapping to index so that we readily recover that index
    nsAutoString viewItemUID;
    AppendInt(viewItemUID, item->rowid);
    viewItemUID.Append('-');
    viewItemUID.AppendInt(item->mediaItemId);

    PRBool added = mViewItemUIDToIndexMap.Put(viewItemUID, index);
    NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);
  }

  /*
   * If the number of rows returned is less than what was requested, this is
   * really bad. Fill the rest in so we don't crash.
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
                                      EmptyString(),
                                      0);
      NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);

      nsAutoPtr<ArrayItem>* success =
        mCache.ReplaceElementsAt(i + rowCount + aDestIndexOffset, 1, item);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    }
  }
  return NS_OK;
}

/* static */ int
sbLocalDatabaseGUIDArray::SortBags(const void* a, const void* b, void* closure)
{
  sbILocalDatabaseResourcePropertyBag* bagA =
    *static_cast<sbILocalDatabaseResourcePropertyBag* const *>(a);

  sbILocalDatabaseResourcePropertyBag* bagB =
    *static_cast<sbILocalDatabaseResourcePropertyBag* const *>(b);

  nsTArray<SortSpec>* sorts = static_cast<nsTArray<SortSpec>*>(closure);
  NS_ASSERTION(sorts->Length() > 1, "Multisorting with single sort!");

  nsresult rv;
  for (PRUint32 i = 1; i < sorts->Length(); i++) {
    PRUint32 propertyId = sorts->ElementAt(i).propertyId;
    PRBool ascending = sorts->ElementAt(i).ascending;

    nsString valueA;
    rv = bagA->GetSortablePropertyByID(propertyId, valueA);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString valueB;
    rv = bagB->GetSortablePropertyByID(propertyId, valueB);
    NS_ENSURE_SUCCESS(rv, rv);

    if (valueA == valueB) {
      continue;
    }

    if (ascending) {
      return valueA > valueB ? 1 : -1;
    }
    else {
      return valueA < valueB ? 1 : -1;
    }

  }

  // If we reach here, the two bags are equal.  Use the mediaItemId of the bags
  // to break the tie
  PRUint32 mediaItemIdA;
  rv = bagA->GetMediaItemId(&mediaItemIdA);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemIdB;
  rv = bagB->GetMediaItemId(&mediaItemIdB);
  NS_ENSURE_SUCCESS(rv, rv);

  return mediaItemIdA > mediaItemIdB ? 1 : -1;
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

  // SortRows always gets called with mCacheMonitor already acquired!
  // No need to acquire this lock in this method.

  /*
   * If this is only one row and it is not the first, last, and only row in the
   * window, we don't need to sort it
   */
  if (!aIsFirst && !aIsLast && !aIsOnly && aStartIndex == aEndIndex) {
    return NS_OK;
  }

  PRUint32 rangeLength = aEndIndex - aStartIndex + 1;

  // XXX Disable memory sorting in the general case, since it appears to slow things
  // down with the index fix from bug 8612. Enable it however when an FTS search
  // is active, as joining the FTS table can severely slow the resort query.

  // We can sort these rows in memory in the case where the entire group of
  // rows lies within the fetched chunk, meaning for a distinct primary sort
  // value, the rows with this value is not the first or last row in the
  // chunk.  It also is not the only value in the chunk.
  if (mHasActiveSearch && !aIsFirst && !aIsLast && !aIsOnly && mPropertyCache) {
    nsTArray<const PRUnichar*> guids(rangeLength);
    for (PRUint32 i = aStartIndex; i <= aEndIndex; i++) {
      const PRUnichar** appended =
        guids.AppendElement(mCache[i]->guid.get());
      NS_ENSURE_TRUE(appended, NS_ERROR_OUT_OF_MEMORY);
    }

    // Grab the propety bags of all the items that are in the range that
    // we're sorting
    PRUint32 bagsCount = 0;
    sbILocalDatabaseResourcePropertyBag** bags = nsnull;
    rv = mPropertyCache->GetProperties(guids.Elements(),
                                       rangeLength,
                                       &bagsCount,
                                       &bags);
    NS_ENSURE_SUCCESS(rv, rv);
    sbAutoFreeXPCOMPointerArray<sbILocalDatabaseResourcePropertyBag> freeBags(bagsCount,
                                                                              bags);

    // Do the sort
    NS_QuickSort(bags,
                 bagsCount,
                 sizeof(sbILocalDatabaseResourcePropertyBag*),
                 SortBags,
                 &mSorts);

    // Update mCache with the results of the sort.  Create a copy of all the
    // items that we are going to reorder, then update the cache in the order
    // of the sorted bags
    nsClassHashtable<nsStringHashKey, ArrayItem> lookup;
    PRBool success = lookup.Init(bagsCount);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    for (PRUint32 i = aStartIndex; i <= aEndIndex; i++) {
      ArrayItem* item = mCache[i];
      nsAutoPtr<ArrayItem> copy(new ArrayItem(*item));
      NS_ENSURE_TRUE(copy, NS_ERROR_OUT_OF_MEMORY);
      success = lookup.Put(copy->guid, copy);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
      copy.forget();
    }

    for (PRUint32 i = 0; i < bagsCount; i++) {
      nsString guid;
      rv = bags[i]->GetGuid(guid);
      NS_ENSURE_SUCCESS(rv, rv);

      ArrayItem* item;
      PRBool found = lookup.Get(guid, &item);
      NS_ENSURE_TRUE(found, NS_ERROR_UNEXPECTED);
      nsAutoPtr<ArrayItem> copy(new ArrayItem(*item));
      NS_ENSURE_TRUE(copy, NS_ERROR_OUT_OF_MEMORY);
      nsAutoPtr<ArrayItem>* replaced =
        mCache.ReplaceElementsAt(i + aStartIndex,
                                 1,
                                 copy.get());
      NS_ENSURE_TRUE(replaced, NS_ERROR_OUT_OF_MEMORY);
      copy.forget();
    }

    return NS_OK;
  }

  nsCOMPtr<sbIDatabaseQuery> query;
  if(aIsNull) {
    rv = MakeQuery(mNullResortStatement, getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = MakeQuery(mResortStatement, getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(0, aKey);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

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

  // Copy the rows from the query result into the cache starting at the
  // calculated offset
  for (PRUint32 i = 0; i < rangeLength; i++) {

    PRUint32 row = offset + i;
    nsAutoPtr<ArrayItem>& item = mCache[i + aStartIndex];

    nsString mediaItemIdStr;
    rv = result->GetRowCell(row, 0, mediaItemIdStr);
    NS_ENSURE_SUCCESS(rv, rv);

    item->mediaItemId = mediaItemIdStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = result->GetRowCell(row, 1, item->guid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = result->GetRowCell(row, 2, item->ordinal);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString rowidStr;
    rv = result->GetRowCell(row, 3, rowidStr);
    NS_ENSURE_SUCCESS(rv, rv);

    item->rowid = nsString_ToUint64(rowidStr, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
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
    rv = MakeQuery(mPrimarySortKeyPositionStatement,
                   getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(0, aValue);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->Execute(&dbOk);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

    nsCOMPtr<sbIDatabaseResult> result;
    rv = query->GetResultObject(getter_AddRefs(result));
    NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

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

  mozilla::MonitorAutoLock mon(mCacheMonitor);

  if (mValid == PR_FALSE) {
    rv = Initialize();
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

#if defined(FORCE_FETCH_ALL_GUIDS_ASYNC)
  /*
   * Cache miss
   */
  rv = FetchRows(aIndex, mFetchSize);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aIndex < mCache.Length(), NS_ERROR_FAILURE);

  /*
   * Prefetch all rows.  Do this from an asynchronous event on the main thread
   * so that any pending UI events can complete.
   */
  if (!mPrefetchedRows) {
    mPrefetchedRows = PR_TRUE;
    sbInvokeOnMainThread2Async(*this,
                               &sbLocalDatabaseGUIDArray::FetchRows,
                               NS_ERROR_FAILURE,
                               static_cast<PRUint32>(0),
                               static_cast<PRUint32>(0));
  }
#else
  /*
   * Cache miss, cache all GUIDs. FetchRows takes care of it's own locking.
   */
  rv = FetchRows(0, 0);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aIndex < mCache.Length(), NS_ERROR_FAILURE);
#endif

  *_retval = mCache[aIndex];

  return NS_OK;
}

PRInt32
sbLocalDatabaseGUIDArray::GetPropertyId(const nsAString& aProperty)
{
  return SB_GetPropertyId(aProperty, mPropertyCache);
}

nsresult
sbLocalDatabaseGUIDArray::GetMTListener(
                                 sbILocalDatabaseGUIDArrayListener ** aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;

  // if the listener has gone away just return null
  if (!mListener) {
    *aListener = nsnull;
    return NS_OK;
  }
  nsCOMPtr<nsIWeakReference> weak;
  nsCOMPtr<sbILocalDatabaseGUIDArrayListener> listener;
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIThread> mainThread;
    rv = NS_GetMainThread(getter_AddRefs(mainThread));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = do_GetProxyForObject(mainThread,
                              mListener.get(),
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(weak));
    NS_ENSURE_SUCCESS(rv, rv);

    listener = do_QueryReferent(weak, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!listener) {
      *aListener = nsnull;
      return NS_OK;
    }
    nsCOMPtr<sbILocalDatabaseGUIDArrayListener> proxiedListener;
    rv = do_GetProxyForObject(mainThread,
                              listener.get(),
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(proxiedListener));
    NS_ENSURE_SUCCESS(rv, rv);
    proxiedListener.forget(aListener);
    return NS_OK;
  }

  listener = do_QueryReferent(mListener);

  listener.forget(aListener);
  return NS_OK;
}

void
sbLocalDatabaseGUIDArray::GenerateCachedLengthKey()
{
  mozilla::MutexAutoLock mon(mPropIdsLock);

  // Clear all the old property IDs from this set, and remove the cache
  // entries on the old key.
  mPropIdsUsedInCacheKey.clear();

  if(mLengthCache && !mCachedLengthKey.IsEmpty()) {
    mLengthCache->RemoveCachedLength(mCachedLengthKey);
    mLengthCache->RemoveCachedNonNullLength(mCachedLengthKey);
  }

  // Try and avoid resizing the string a bunch of times.
  mCachedLengthKey.SetLength(2048);

  // Clear the key.
  mCachedLengthKey.Truncate();

  // Add Database GUID
  mCachedLengthKey.Append(mDatabaseGUID);

  // Add Base Table
  mCachedLengthKey.Append(mBaseTable);

  // Add Base Constraint Column
  mCachedLengthKey.Append(mBaseConstraintColumn);

  // Add Base Constraint Value
  mCachedLengthKey.AppendInt(mBaseConstraintValue);

  // Add Is Distinct
  mCachedLengthKey.AppendInt(mIsDistinct);

  // Add Distinct with Sortable
  mCachedLengthKey.AppendInt(mDistinctWithSortableValues);

  // Add Is Full Library
  mCachedLengthKey.AppendInt(mIsFullLibrary);

  // We'll save off the property ids so we can associate them with the
  // keys generated so we can invalidate cached lengths later.

  // Go through the filters and add them to the key.
  PRUint32 filterCount = mFilters.Length();
  for (PRUint32 index = 0; index < filterCount; index++) {
    const FilterSpec& refSpec = mFilters.ElementAt(index);

    mCachedLengthKey.Append(refSpec.property);

    PRUint32 propId = 0;
    if(NS_SUCCEEDED(mPropertyCache->GetPropertyDBID(refSpec.property,
                                                    &propId))) {
      mPropIdsUsedInCacheKey.insert(propId);
    }

    mCachedLengthKey.AppendInt(refSpec.isSearch);

    PRUint32 valueCount = refSpec.values.Length();
    for(PRUint32 valueIndex = 0; valueIndex < valueCount; valueIndex++) {
      mCachedLengthKey.Append(refSpec.values.ElementAt(valueIndex));
    }
  }

  // Go through the properties we use to sort and add them to the key.
  PRUint32 sortCount = mSorts.Length();
  for (PRUint32 index = 0; index < sortCount; index++) {
    const SortSpec& sortSpec = mSorts.ElementAt(index);
    mCachedLengthKey.AppendInt(sortSpec.propertyId);
    mPropIdsUsedInCacheKey.insert(sortSpec.propertyId);

    mCachedLengthKey.AppendInt(sortSpec.ascending);
    mCachedLengthKey.AppendInt(sortSpec.secondary);
  }
}

NS_IMPL_ISUPPORTS1(sbGUIDArrayEnumerator, nsISimpleEnumerator)

sbGUIDArrayEnumerator::sbGUIDArrayEnumerator(sbLocalDatabaseLibrary* aLibrary,
                                             sbILocalDatabaseGUIDArray* aArray) :
  mLibrary(aLibrary),
  mArray(aArray),
  mNextIndex(0),
  mPreviousLength(0)
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

  // Length changed, reset next index, next guid.
  // This happens when the data inside the guid array changes
  // because guids were added or removed from the result set.
  if (length != mPreviousLength) {
    mPreviousLength = length;
    mNextIndex = 0;
    mNextGUID.Truncate();
  }

  *_retval = mNextIndex < length;

  return NS_OK;
}

NS_IMETHODIMP
sbGUIDArrayEnumerator::GetNext(nsISupports **_retval)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

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
