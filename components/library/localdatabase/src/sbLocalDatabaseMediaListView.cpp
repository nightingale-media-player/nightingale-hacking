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

#include "sbLocalDatabaseMediaListView.h"

#include <DatabaseQuery.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIProgrammingLanguage.h>
#include <nsIProperty.h>
#include <nsITreeView.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <nsMemory.h>
#include <sbLocalDatabaseTreeView.h>
#include <sbICascadeFilterSet.h>
#include <sbIDatabaseQuery.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseAsyncGUIDArray.h>
#include <sbILocalDatabaseSimpleMediaList.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbISQLBuilder.h>
#include <sbIMediaList.h>
#include <sbPropertiesCID.h>
#include <sbSQLBuilderCID.h>
#include <sbTArrayStringEnumerator.h>
#include <prlog.h>

#include "sbDatabaseResultStringEnumerator.h"
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseCascadeFilterSet.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabasePropertyCache.h"

#define DEFAULT_FETCH_SIZE 1000

NS_IMPL_ISUPPORTS6(sbLocalDatabaseMediaListView,
                   sbIMediaListView,
                   sbIMediaListListener,
                   sbIFilterableMediaList,
                   sbISearchableMediaList,
                   sbISortableMediaList,
                   nsIClassInfo)

NS_IMPL_CI_INTERFACE_GETTER6(sbLocalDatabaseMediaListView,
                             sbIMediaListView,
                             sbIMediaListListener,
                             sbIFilterableMediaList,
                             sbISearchableMediaList,
                             sbISortableMediaList,
                             nsIClassInfo)

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseMediaListView:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* sMediaListViewLog = nsnull;
#define TRACE(args) if (sMediaListViewLog) PR_LOG(sMediaListViewLog, PR_LOG_DEBUG, args)
#define LOG(args)   if (sMediaListViewLog) PR_LOG(sMediaListViewLog, PR_LOG_WARN, args)
#else /* PR_LOGGING */
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

/**
 * \brief Adds multiple filters to a GUID array.
 *
 * This method enumerates a hash table and calls AddFilter on a GUIDArray once 
 * for each key. It constructs a string enumerator for the string array that
 * the hash table contains.
 *
 * This method expects to be handed an sbILocalDatabaseGUIDArray pointer as its
 * aUserData parameter.
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListView::AddFilterToGUIDArrayCallback(nsStringHashKey::KeyType aKey,
                                                           sbStringArray* aEntry,
                                                           void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

  // Make a string enumerator for the string array.
  nsCOMPtr<nsIStringEnumerator> valueEnum =
    new sbTArrayStringEnumerator(aEntry);

  // If we failed then we're probably out of memory. Hope we do better on the
  // next key?
  NS_ENSURE_TRUE(valueEnum, PL_DHASH_NEXT);

  // Unbox the guidArray.
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> guidArray =
    NS_STATIC_CAST(sbILocalDatabaseAsyncGUIDArray*, aUserData);

  // Set the filter.
  nsresult rv = guidArray->AddFilter(aKey, valueEnum, PR_FALSE);
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "AddFilter failed!");

  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListView::CloneStringArrayHashCallback(nsStringHashKey::KeyType aKey,
                                                           sbStringArray* aEntry,
                                                           void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

  sbStringArrayHash* stringArrayHash =
    NS_STATIC_CAST(sbStringArrayHash*, aUserData);
  NS_ASSERTION(stringArrayHash, "Could not cast user data");

  sbStringArray* newStringArray = new sbStringArray(*aEntry);
  NS_ENSURE_TRUE(newStringArray, PL_DHASH_STOP);

  PRBool success = stringArrayHash->Put(aKey, newStringArray);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListView::CopyStringArrayHashCallback(nsStringHashKey::KeyType aKey,
                                                          sbStringArray* aEntry,
                                                          void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

  nsCOMPtr<sbIMutablePropertyArray> propertyArray =
    NS_STATIC_CAST(sbIMutablePropertyArray*, aUserData);
  NS_ASSERTION(propertyArray, "Could not cast user data");

  PRUint32 length = aEntry->Length();
  nsresult rv;
  for (PRUint32 i = 0; i < length; i++) {
    rv = propertyArray->AppendProperty(aKey, aEntry->ElementAt(i));
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  }

  return PL_DHASH_NEXT;
}

sbLocalDatabaseMediaListView::sbLocalDatabaseMediaListView(sbLocalDatabaseLibrary* aLibrary,
                                                           sbLocalDatabaseMediaListBase* aMediaList,
                                                           nsAString& aDefaultSortProperty,
                                                           PRUint32 aMediaListId) :
  mLibrary(aLibrary),
  mMediaList(aMediaList),
  mDefaultSortProperty(aDefaultSortProperty),
  mMediaListId(aMediaListId),
  mInBatch(PR_FALSE),
  mInvalidatePending(PR_FALSE)
{
  NS_ASSERTION(aLibrary, "aLibrary is null");
  NS_ASSERTION(aMediaList, "aMediaList is null");

  MOZ_COUNT_CTOR(sbLocalDatabaseMediaListView);
#ifdef PR_LOGGING
  if (!sMediaListViewLog) {
    sMediaListViewLog = PR_NewLogModule("sbLocalDatabaseMediaListView");
  }
#endif
  TRACE(("sbLocalDatabaseMediaListView[0x%.8x] - Constructed", this));

  PRBool success = mViewFilters.Init();
  NS_ASSERTION(success, "Failed to init view filter table");
}

sbLocalDatabaseMediaListView::~sbLocalDatabaseMediaListView()
{
  TRACE(("sbLocalDatabaseMediaListView[0x%.8x] - Destructed", this));
  MOZ_COUNT_DTOR(sbLocalDatabaseMediaListView);

  if (mMediaList) {
    mMediaList->RemoveListener(this);
  }
}

nsresult
sbLocalDatabaseMediaListView::Init()
{
  nsresult rv;

  mPropMan = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mArray = do_CreateInstance(SB_LOCALDATABASE_ASYNCGUIDARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> databaseLocation;
  rv = mLibrary->GetDatabaseLocation(getter_AddRefs(databaseLocation));
  NS_ENSURE_SUCCESS(rv, rv);

  if (databaseLocation) {
    rv = mArray->SetDatabaseLocation(databaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mMediaListId == 0) {
    rv = mArray->SetBaseTable(NS_LITERAL_STRING("media_items"));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = mArray->SetBaseTable(NS_LITERAL_STRING("simple_media_lists"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mArray->SetBaseConstraintColumn(NS_LITERAL_STRING("media_item_id"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mArray->SetBaseConstraintValue(mMediaListId);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mArray->AddSort(mDefaultSortProperty, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetFetchSize(1000);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  // We listen to our media list
  rv = mMediaList->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the default sort as our view sort
  mViewSort = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mViewSort->AppendProperty(mDefaultSortProperty, NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

already_AddRefed<sbLocalDatabaseMediaListBase>
sbLocalDatabaseMediaListView::GetNativeMediaList()
{
  NS_ASSERTION(mMediaList, "mMediaList is null");
  sbLocalDatabaseMediaListBase* result = mMediaList;
  NS_ADDREF(result);
  return result;
}

sbILocalDatabaseGUIDArray*
sbLocalDatabaseMediaListView::GetGUIDArray()
{
  NS_ASSERTION(mArray, "mArray is null");
  return mArray;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetMediaList(sbIMediaList** aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  NS_IF_ADDREF(*aMediaList = mMediaList);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetLength(PRUint32* aFilteredLength)
{
  NS_ENSURE_ARG_POINTER(aFilteredLength);

  nsresult rv = mArray->GetLength(aFilteredLength);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetTreeView(nsITreeView** aTreeView)
{

  NS_ENSURE_ARG_POINTER(aTreeView);

  if (!mTreeView) {

    nsresult rv;

    nsCOMPtr<sbILocalDatabasePropertyCache> propertyCache;
    rv = mLibrary->GetPropertyCache(getter_AddRefs(propertyCache));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mArray->SetPropertyCache(propertyCache);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbLocalDatabaseTreeView> tree = new sbLocalDatabaseTreeView();
    NS_ENSURE_TRUE(tree, NS_ERROR_OUT_OF_MEMORY);

    rv = tree->Init(this, mArray, mViewSort);
    NS_ENSURE_SUCCESS(rv, rv);

    mTreeView = tree;
  }

  NS_ADDREF(*aTreeView = mTreeView);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetCascadeFilterSet(sbICascadeFilterSet** aCascadeFilterSet)
{
  NS_ENSURE_ARG_POINTER(aCascadeFilterSet);

  if (!mCascadeFilterSet) {
    nsresult rv;

    mCascadeFilterSet = new sbLocalDatabaseCascadeFilterSet();
    NS_ENSURE_TRUE(mCascadeFilterSet, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> guidArray;
    rv = mArray->CloneAsyncArray(getter_AddRefs(guidArray));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mCascadeFilterSet->Init(mLibrary, this, guidArray);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aCascadeFilterSet = mCascadeFilterSet);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetItemByIndex(PRUint32 aIndex,
                                             sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsAutoString guid;
  rv = mArray->GetByIndex(aIndex, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = mLibrary->GetMediaItem(guid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = item);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetUnfilteredIndex(PRUint32 aIndex,
                                                 PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // If this view is on the library, we know we only have unique items so
  // we can get the guid of the item at the given index and use that to find
  // the unfiltered index
  if (mMediaListId == 0) {
    nsAutoString guid;
    rv = mArray->GetGuidByIndex(aIndex, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMediaItem> item;
    rv = mMediaList->GetItemByGuid(guid, getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMediaList->IndexOf(item, 0, _retval);
    NS_ENSURE_SUCCESS(rv, rv);

  }
  else {
    // Otherwise, get the ordinal for this item and use it to get the item
    // from the full media list
    nsAutoString ordinal;
    rv = mArray->GetOrdinalByIndex(aIndex, ordinal);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILocalDatabaseSimpleMediaList> sml =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediaList*, mMediaList), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sml->GetIndexByOrdinal(ordinal, _retval);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::Clone(sbIMediaListView** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  // XXXsteve THIS IS INCOMPLETE.  Still needs to clone the cascade filter
  // set
  nsresult rv;

  nsRefPtr<sbLocalDatabaseMediaListView>
    clone(new sbLocalDatabaseMediaListView(mLibrary,
                                           mMediaList,
                                           mDefaultSortProperty,
                                           mMediaListId));
  NS_ENSURE_TRUE(clone, NS_ERROR_OUT_OF_MEMORY);

  rv = clone->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  // Copy filters
  mViewFilters.EnumerateRead(CloneStringArrayHashCallback,
                             &clone->mViewFilters);

  // Copy searches
  if (mViewSearches) {
    rv = ClonePropertyArray(mViewSearches,
                            getter_AddRefs(clone->mViewSearches));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Copy sorts
  if (mViewSort) {
    rv = ClonePropertyArray(mViewSort, getter_AddRefs(clone->mViewSort));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Call the update method to notify the clone that its internal data was
  // tinkered with
  rv = clone->UpdateViewArrayConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = clone);
  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListView::ClonePropertyArray(sbIPropertyArray* aSource,
                                                 sbIMutablePropertyArray** _retval)
{
  NS_ENSURE_ARG_POINTER(aSource);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> clone =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 propertyCount;
  rv = aSource->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < propertyCount; i++) {
    nsCOMPtr<nsIProperty> property;
    rv = aSource->GetPropertyAt(i, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIVariant> value;
    rv = property->GetValue(getter_AddRefs(value));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint16 dataType;
    rv = value->GetDataType(&dataType);
    NS_ENSURE_SUCCESS(rv, rv);

    // Skip non strings
    if (dataType != nsIDataType::VTYPE_ASTRING) {
      continue;
    }

    nsAutoString propertyName;
    rv = property->GetName(propertyName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString valueString;
    rv = value->GetAsAString(valueString);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = clone->AppendProperty(propertyName, valueString);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*_retval = clone);
  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListView::HasCommonProperty(sbIPropertyArray* aBag1,
                                                sbIPropertyArray* aBag2,
                                                PRBool* aHasCommonProperty)
{
  NS_ASSERTION(aBag1, "aBag1 is null");
  NS_ASSERTION(aBag2, "aBag2 is null");
  NS_ASSERTION(aHasCommonProperty, "aHasCommonProperty is null");

  PRUint32 length;
  nsresult rv = aBag1->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<nsIProperty> property;
    rv = aBag1->GetPropertyAt(i, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString propertyName;
    rv = property->GetName(propertyName);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString junk;
    rv = aBag2->GetPropertyValue(propertyName, junk);
    if (NS_SUCCEEDED(rv)) {
      *aHasCommonProperty = PR_TRUE;
      return NS_OK;
    }

  }

  *aHasCommonProperty = PR_FALSE;
  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListView::ShouldCauseInvalidation(sbIPropertyArray* aProperties,
                                                      PRBool* aShouldCauseInvalidation)
{
  NS_ASSERTION(aProperties, "aProperties is null");
  NS_ASSERTION(aShouldCauseInvalidation, "aShouldCauseInvalidation is null");
  
  // If one of the updated proprties is involved in the current sort, filter,
  // or search, we should invalidate
  nsCOMPtr<sbIPropertyArray> props;

  // Search sort
  nsresult rv = GetCurrentSort(getter_AddRefs(props));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = HasCommonProperty(aProperties, props, aShouldCauseInvalidation);
  NS_ENSURE_SUCCESS(rv, rv);
  if (*aShouldCauseInvalidation)
    return NS_OK;

  // Search filter
  rv = GetCurrentFilter(getter_AddRefs(props));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = HasCommonProperty(aProperties, props, aShouldCauseInvalidation);
  NS_ENSURE_SUCCESS(rv, rv);
  if (*aShouldCauseInvalidation)
    return NS_OK;

  // Search search
  rv = GetCurrentSearch(getter_AddRefs(props));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = HasCommonProperty(aProperties, props, aShouldCauseInvalidation);
  NS_ENSURE_SUCCESS(rv, rv);
  if (*aShouldCauseInvalidation)
    return NS_OK;

  return NS_OK;
}

// sbIFilterableMediaList
NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetFilterableProperties(nsIStringEnumerator** aFilterableProperties)
{
  NS_ENSURE_ARG_POINTER(aFilterableProperties);

  // Get this from the property manager?
  return NS_ERROR_NOT_IMPLEMENTED;

}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetFilterValues(const nsAString& aPropertyName,
                                              nsIStringEnumerator** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mDistinctPropertyValuesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aPropertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  sbDatabaseResultStringEnumerator* values =
    new sbDatabaseResultStringEnumerator(result);
  NS_ENSURE_TRUE(values, NS_ERROR_OUT_OF_MEMORY);

  rv = values->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = values);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetCurrentFilter(sbIPropertyArray** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> propertyArray =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mViewFilters.EnumerateRead(CopyStringArrayHashCallback, propertyArray);

  NS_ADDREF(*_retval = propertyArray);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::SetFilters(sbIPropertyArray* aPropertyArray)
{
  NS_ENSURE_ARG_POINTER(aPropertyArray);
#ifdef PR_LOGGING
  nsAutoString buff;
  aPropertyArray->ToString(buff);
  TRACE(("sbLocalDatabaseMediaListView[0x%.8x] - SetFilters(%s)",
         this, NS_LossyConvertUTF16toASCII(buff).get()));
#endif

  nsresult rv = UpdateFiltersInternal(aPropertyArray, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::UpdateFilters(sbIPropertyArray* aPropertyArray)
{
  NS_ENSURE_ARG_POINTER(aPropertyArray);

  nsresult rv = UpdateFiltersInternal(aPropertyArray, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::RemoveFilters(sbIPropertyArray* aPropertyArray)
{
  NS_ENSURE_ARG_POINTER(aPropertyArray);

  nsresult rv;

  PRUint32 propertyCount;
  rv = aPropertyArray->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // ensure we got something
  NS_ENSURE_STATE(propertyCount);

  PRBool dirty = PR_FALSE;
  for (PRUint32 index = 0; index < propertyCount; index++) {

    // Get the property.
    nsCOMPtr<nsIProperty> property;
    rv = aPropertyArray->GetPropertyAt(index, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the value.
    nsCOMPtr<nsIVariant> value;
    rv = property->GetValue(getter_AddRefs(value));
    NS_ENSURE_SUCCESS(rv, rv);

    // Make sure the value is a string type, otherwise bail.
    PRUint16 dataType;
    rv = value->GetDataType(&dataType);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(dataType == nsIDataType::VTYPE_ASTRING,
                   NS_ERROR_INVALID_ARG);

    // Get the name of the property. This will be the key for the hash table.
    nsAutoString propertyName;
    rv = property->GetName(propertyName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the property value
    nsAutoString valueString;
    rv = value->GetAsAString(valueString);
    NS_ENSURE_SUCCESS(rv, rv);

    sbStringArray* stringArray;
    PRBool arrayExists = mViewFilters.Get(propertyName, &stringArray);
    // If there is an array for this property, search the array for the value
    // and remove it
    if (arrayExists) {
      nsCOMPtr<sbIPropertyInfo> info;
      rv = mPropMan->GetPropertyInfo(propertyName, getter_AddRefs(info));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString sortableValue;
      rv = info->MakeSortable(valueString, sortableValue);
      NS_ENSURE_SUCCESS(rv, rv);

      PRUint32 length = stringArray->Length();
      // Do this backwards so we don't have to deal with the array shifting
      // on us.  Also, be sure to remove multiple copies of the same string.
      for (PRInt32 i = length - 1; i >= 0; i--) {
        if (stringArray->ElementAt(i).Equals(sortableValue)) {
          stringArray->RemoveElementAt(i);
          dirty = PR_TRUE;
        }
      }
    }
  }

  if (dirty) {
    rv = UpdateViewArrayConfiguration();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::ClearFilters()
{
  mViewFilters.Clear();

  nsresult rv = UpdateViewArrayConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbISearchableMediaList
NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetSearchableProperties(nsIStringEnumerator** aSearchableProperties)
{
  NS_ENSURE_ARG_POINTER(aSearchableProperties);
  // To be implemented by property manager?
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetCurrentSearch(sbIPropertyArray** aCurrentSearch)
{
  NS_ENSURE_ARG_POINTER(aCurrentSearch);

  nsresult rv;

  if (!mViewSearches) {
    mViewSearches = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aCurrentSearch = mViewSearches);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::SetSearch(sbIPropertyArray* aSearch)
{
  NS_ENSURE_ARG_POINTER(aSearch);
#ifdef PR_LOGGING
  nsAutoString buff;
  aSearch->ToString(buff);
  TRACE(("sbLocalDatabaseMediaListView[0x%.8x] - SetSearch(%s)",
         this, NS_LossyConvertUTF16toASCII(buff).get()));
#endif

  nsresult rv;

  rv = ClonePropertyArray(aSearch, getter_AddRefs(mViewSearches));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateViewArrayConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::ClearSearch()
{
  nsresult rv;

  if (mViewSearches) {
    nsCOMPtr<nsIMutableArray> array = do_QueryInterface(mViewSearches, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = array->Clear();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = UpdateViewArrayConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbISortableMediaList
NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetSortableProperties(nsIStringEnumerator** aSortableProperties)
{
  NS_ENSURE_ARG_POINTER(aSortableProperties);
  // To be implemented by property manager?
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetCurrentSort(sbIPropertyArray** aCurrentSort)
{
  NS_ENSURE_ARG_POINTER(aCurrentSort);
  NS_ENSURE_STATE(mViewSort);

  NS_ADDREF(*aCurrentSort = mViewSort);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::SetSort(sbIPropertyArray* aSort)
{
  NS_ENSURE_ARG_POINTER(aSort);

  nsresult rv;

  rv = ClonePropertyArray(aSort, getter_AddRefs(mViewSort));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateViewArrayConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::ClearSort()
{
  nsresult rv;

  if (mViewSort) {
    nsCOMPtr<nsIMutableArray> array = do_QueryInterface(mViewSort, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = array->Clear();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = UpdateViewArrayConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// sbIMediaListListener
NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnItemAdded(sbIMediaList* aMediaList,
                                          sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  if (mInBatch) {
    mInvalidatePending = PR_TRUE;
    return NS_OK;
  }

  // Invalidate the view array
  nsresult rv = Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                                  sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  // Don't care

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                                 sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);

  if (mInBatch) {
    mInvalidatePending = PR_TRUE;
    return NS_OK;
  }

  // Invalidate the view array
  nsresult rv = Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnItemUpdated(sbIMediaList* aMediaList,
                                            sbIMediaItem* aMediaItem,
                                            sbIPropertyArray* aProperties)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aProperties);

#ifdef PR_LOGGING
  nsAutoString buff;
  aProperties->ToString(buff);
  TRACE(("sbLocalDatabaseMediaListView[0x%.8x] - OnItemUpdated %s",
         this, NS_ConvertUTF16toUTF8(buff).get()));
#endif

  nsresult rv;

  // If the list itself is getting updated (rather than an item in the list),
  // we always invalidate.  This happens when items in a simple media list is
  // reordered
  PRBool isList;
  rv = mMediaList->Equals(aMediaItem, &isList);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isList) {
    if (mInBatch) {
      mInvalidatePending = PR_TRUE;
    }
    else {
      nsresult rv = Invalidate();
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }

  PRBool shouldInvalidate = PR_FALSE;

  if (mInBatch) {
    // If we are in a batch, remember the updated properties for later.  Once
    // the batch is complete, we can check it to see if it should cause an
    // invalidation
    PRBool success = mUpdatedPropertiesInBatch.AppendObject(aProperties);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    // Otherwise, check to see if this update should cause an invalidation
    rv = ShouldCauseInvalidation(aProperties, &shouldInvalidate);
    NS_ENSURE_SUCCESS(rv, rv);

    if (shouldInvalidate) {
      // Invalidate the view array
      nsresult rv = Invalidate();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // If the array has not already been invalidated, we should invalidate the
  // row of the tree view that contains this item.  This lets us see updates
  // that don't cause invalidations as well as see updates that happen in
  // batches sooner
  if (mTreeView && !shouldInvalidate) {
    nsAutoString guid;
    rv = aMediaItem->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mTreeView->InvalidateRowsByGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnListCleared(sbIMediaList* aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnBatchBegin(sbIMediaList* aMediaList)
{
  NS_ASSERTION(!mInBatch, "Shouldn't be notified of more than one batch!");
  mInBatch = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnBatchEnd(sbIMediaList* aMediaList)
{
  NS_ASSERTION(mInBatch, "Should have been notified when entering a batch!");
  mInBatch = PR_FALSE;

  if (mInvalidatePending) {
    // Invalidate the view array
    nsresult rv = Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);

    mInvalidatePending = PR_FALSE;
  }
  else {
    // If we haven't been told explicitly that there is an invalidation,
    // pending, we may still have to invalidate due to a series of item
    // updates
    PRUint32 length = mUpdatedPropertiesInBatch.Count();
    for (PRUint32 i = 0; i < length; i++) {
      PRBool shouldInvalidate;
      nsresult rv = ShouldCauseInvalidation(mUpdatedPropertiesInBatch[i],
                                            &shouldInvalidate);
      NS_ENSURE_SUCCESS(rv, rv);
      if (shouldInvalidate) {
        nsresult rv = Invalidate();
        NS_ENSURE_SUCCESS(rv, rv);
        break;
      }
    }
  }

  mUpdatedPropertiesInBatch.Clear();

  return NS_OK;
}

/**
 * \brief Updates the internal filter map with a contents of a property bag.
 *        In replace mode, the value list for each distinct property in the
 *        bag is first cleared before the values in the bag are added.  When
 *        not in replace mode, the new values in the bag are simply appended
 *        to each property's list of values.
 */
nsresult
sbLocalDatabaseMediaListView::UpdateFiltersInternal(sbIPropertyArray* aPropertyArray,
                                                    PRBool aReplace)
{
  nsresult rv;

  PRUint32 propertyCount;
  rv = aPropertyArray->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // ensure we got something
  NS_ENSURE_STATE(propertyCount);

  nsTHashtable<nsStringHashKey> seenProperties;
  PRBool success = seenProperties.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 index = 0; index < propertyCount; index++) {

    // Get the property.
    nsCOMPtr<nsIProperty> property;
    rv = aPropertyArray->GetPropertyAt(index, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the value.
    nsCOMPtr<nsIVariant> value;
    rv = property->GetValue(getter_AddRefs(value));
    NS_ENSURE_SUCCESS(rv, rv);

    // Make sure the value is a string type, otherwise bail.
    PRUint16 dataType;
    rv = value->GetDataType(&dataType);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(dataType == nsIDataType::VTYPE_ASTRING,
                   NS_ERROR_INVALID_ARG);

    // Get the name of the property. This will be the key for the hash table.
    nsAutoString propertyName;
    rv = property->GetName(propertyName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the property value
    nsAutoString valueString;
    rv = value->GetAsAString(valueString);
    NS_ENSURE_SUCCESS(rv, rv);

    // If the string is empty and we are replacing, we should delete the
    // property from the hash
    if (valueString.IsEmpty()) {
      if (aReplace) {
        mViewFilters.Remove(propertyName);
      }
    }
    else {
      // Get the string array associated with the key. If it doesn't yet exist
      // then we need to create it.
      sbStringArray* stringArray;
      PRBool arrayExists = mViewFilters.Get(propertyName, &stringArray);

      if (!arrayExists) {
        NS_NEWXPCOM(stringArray, sbStringArray);
        NS_ENSURE_TRUE(stringArray, NS_ERROR_OUT_OF_MEMORY);

        // Try to add the array to the hash table.
        success = mViewFilters.Put(propertyName, stringArray);
        NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
      }

      if (aReplace) {
        // If this is the first time we've seen this property, clear the
        // string array
        if (!seenProperties.GetEntry(propertyName)) {
          stringArray->Clear();
          nsStringHashKey* successKey = seenProperties.PutEntry(propertyName);
          NS_ENSURE_TRUE(successKey, NS_ERROR_OUT_OF_MEMORY);
        }
      }

      nsCOMPtr<sbIPropertyInfo> info;
      rv = mPropMan->GetPropertyInfo(propertyName, getter_AddRefs(info));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString sortableValue;
      rv = info->MakeSortable(valueString, sortableValue);
      NS_ENSURE_SUCCESS(rv, rv);

      // Now we need a slot for the property value.
      nsString* valueStringPtr = stringArray->AppendElement();
      NS_ENSURE_TRUE(valueStringPtr, NS_ERROR_OUT_OF_MEMORY);

      valueStringPtr->Assign(sortableValue);
    }
  }

  rv = UpdateViewArrayConfiguration();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListView::UpdateViewArrayConfiguration()
{
  nsresult rv;

  // Update filters
  rv = mArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  mViewFilters.EnumerateRead(AddFilterToGUIDArrayCallback, mArray);

  // Update searches
  if (mViewSearches) {
    PRUint32 propertyCount;
    rv = mViewSearches->GetLength(&propertyCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 index = 0; index < propertyCount; index++) {

      nsCOMPtr<nsIProperty> property;
      rv = mViewSearches->GetPropertyAt(index, getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIVariant> value;
      rv = property->GetValue(getter_AddRefs(value));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString propertyName;
      rv = property->GetName(propertyName);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString stringValue;
      rv = value->GetAsAString(stringValue);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIPropertyInfo> info;
      rv = mPropMan->GetPropertyInfo(propertyName, getter_AddRefs(info));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString sortableValue;
      rv = info->MakeSortable(stringValue, sortableValue);
      NS_ENSURE_SUCCESS(rv, rv);

      sbStringArray valueArray(1);
      nsString* successString = valueArray.AppendElement(sortableValue);
      NS_ENSURE_TRUE(successString, NS_ERROR_OUT_OF_MEMORY);

      nsCOMPtr<nsIStringEnumerator> valueEnum =
        new sbTArrayStringEnumerator(&valueArray);
      NS_ENSURE_TRUE(valueEnum, NS_ERROR_OUT_OF_MEMORY);

      rv = mArray->AddFilter(propertyName, valueEnum, PR_TRUE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Update sort
  rv = mArray->ClearSorts();
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasSorts = PR_FALSE;
  if (mViewSort) {
    PRUint32 propertyCount;
    rv = mViewSort->GetLength(&propertyCount);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 index = 0; index < propertyCount; index++) {

      nsCOMPtr<nsIProperty> property;
      rv = mViewSort->GetPropertyAt(index, getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIVariant> value;
      rv = property->GetValue(getter_AddRefs(value));
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString propertyName;
      rv = property->GetName(propertyName);
      NS_ENSURE_SUCCESS(rv, rv);

      nsAutoString stringValue;
      rv = value->GetAsAString(stringValue);
      NS_ENSURE_SUCCESS(rv, rv);

      mArray->AddSort(propertyName, stringValue.EqualsLiteral("a"));
      NS_ENSURE_SUCCESS(rv, rv);

      hasSorts = PR_TRUE;
    }
  }

  // If no sort is specified, use the default sort
  if (!hasSorts) {
    mArray->AddSort(mDefaultSortProperty, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Invalidate the view array
  rv = Invalidate();

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListView::MakeStandardQuery(sbIDatabaseQuery** _retval)
{
  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> databaseLocation;
  rv = mLibrary->GetDatabaseLocation(getter_AddRefs(databaseLocation));
  NS_ENSURE_SUCCESS(rv, rv);

  if (databaseLocation) {
    rv = query->SetDatabaseLocation(databaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = query);
  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListView::CreateQueries()
{
  nsresult rv;

  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;

  // Create distinct property values query
  rv = builder->SetDistinct(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(NS_LITERAL_STRING("_rp"),
                          NS_LITERAL_STRING("obj"));
  NS_ENSURE_SUCCESS(rv, rv);

  if (mMediaListId == 0) {
    rv = builder->SetBaseTableName(NS_LITERAL_STRING("properties"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->SetBaseTableAlias(NS_LITERAL_STRING("_p"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->SetDistinct(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddColumn(NS_LITERAL_STRING("_rp"),
                            NS_LITERAL_STRING("obj"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                          NS_LITERAL_STRING("resource_properties"),
                          NS_LITERAL_STRING("_rp"),
                          NS_LITERAL_STRING("property_id"),
                          NS_LITERAL_STRING("_p"),
                          NS_LITERAL_STRING("property_id"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->CreateMatchCriterionParameter(NS_LITERAL_STRING("_p"),
                                                NS_LITERAL_STRING("property_name"),
                                                sbISQLSelectBuilder::MATCH_EQUALS,
                                                getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = builder->SetBaseTableName(NS_LITERAL_STRING("media_items"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->SetBaseTableAlias(NS_LITERAL_STRING("_mi"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                          NS_LITERAL_STRING("simple_media_lists"),
                          NS_LITERAL_STRING("_sml"),
                          NS_LITERAL_STRING("member_media_item_id"),
                          NS_LITERAL_STRING("_mi"),
                          NS_LITERAL_STRING("media_item_id"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->CreateMatchCriterionLong(NS_LITERAL_STRING("_sml"),
                                           NS_LITERAL_STRING("media_item_id"),
                                           sbISQLSelectBuilder::MATCH_EQUALS,
                                           mMediaListId,
                                           getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                          NS_LITERAL_STRING("resource_properties"),
                          NS_LITERAL_STRING("_rp"),
                          NS_LITERAL_STRING("guid"),
                          NS_LITERAL_STRING("_mi"),
                          NS_LITERAL_STRING("guid"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddJoin(sbISQLSelectBuilder::JOIN_INNER,
                          NS_LITERAL_STRING("properties"),
                          NS_LITERAL_STRING("_p"),
                          NS_LITERAL_STRING("property_id"),
                          NS_LITERAL_STRING("_rp"),
                          NS_LITERAL_STRING("property_id"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->CreateMatchCriterionParameter(NS_LITERAL_STRING("_p"),
                                                NS_LITERAL_STRING("property_name"),
                                                sbISQLSelectBuilder::MATCH_EQUALS,
                                                getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = builder->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = builder->AddOrder(NS_LITERAL_STRING("_rp"),
                         NS_LITERAL_STRING("obj_sortable"),
                         PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mDistinctPropertyValuesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListView::Invalidate()
{
  LOG(("sbLocalDatabaseMediaListView[0x%.8x] - Invalidate", this));
  nsresult rv;

  // Invalidate the view array
  rv = mArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  // If we have an active tree view, rebuild it
  if (mTreeView) {
    rv = mTreeView->Rebuild();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseMediaListView)(count, array);
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetHelperForLanguage(PRUint32 language,
                                                   nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetFlags(PRUint32 *aFlags)
{
  *aFlags = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

