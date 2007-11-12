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
#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIObjectInputStream.h>
#include <nsIObjectOutputStream.h>
#include <nsIProgrammingLanguage.h>
#include <nsIProperty.h>
#include <nsITreeView.h>
#include <nsIURI.h>
#include <nsIVariant.h>
#include <nsIWeakReferenceUtils.h>
#include <nsMemory.h>
#include <sbLocalDatabaseTreeView.h>
#include <sbICascadeFilterSet.h>
#include <sbIDatabaseQuery.h>
#include <sbILibrary.h>
#include <sbILibraryConstraints.h>
#include <sbILocalDatabaseAsyncGUIDArray.h>
#include <sbILocalDatabaseSimpleMediaList.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbISQLBuilder.h>
#include <sbIMediaList.h>
#include <sbPropertiesCID.h>
#include <sbSQLBuilderCID.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <prlog.h>

#include "sbDatabaseResultStringEnumerator.h"
#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseCascadeFilterSet.h"
#include "sbLocalDatabaseMediaListViewState.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabasePropertyCache.h"
#include "sbLocalDatabaseSchemaInfo.h"

#define DEFAULT_FETCH_SIZE 1000

NS_IMPL_ISUPPORTS7(sbLocalDatabaseMediaListView,
                   sbIMediaListView,
                   sbIMediaListListener,
                   sbIFilterableMediaListView,
                   sbISearchableMediaListView,
                   sbISortableMediaListView,
                   nsIClassInfo,
                   nsISupportsWeakReference)

NS_IMPL_CI_INTERFACE_GETTER7(sbLocalDatabaseMediaListView,
                             sbIMediaListView,
                             sbIMediaListListener,
                             sbIFilterableMediaListView,
                             sbISearchableMediaListView,
                             sbISortableMediaListView,
                             nsISupportsWeakReference,
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

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListView::CloneStringArrayHashCallback(nsStringHashKey::KeyType aKey,
                                                           sbStringArray* aEntry,
                                                           void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

  sbStringArrayHash* stringArrayHash =
    static_cast<sbStringArrayHash*>(aUserData);
  NS_ASSERTION(stringArrayHash, "Could not cast user data");

  sbStringArray* newStringArray = new sbStringArray(*aEntry);
  NS_ENSURE_TRUE(newStringArray, PL_DHASH_STOP);

  PRBool success = stringArrayHash->Put(aKey, newStringArray);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListView::AddValuesToArrayCallback(nsStringHashKey::KeyType aKey,
                                                       sbStringArray* aEntry,
                                                       void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

  nsCOMPtr<sbIMutablePropertyArray> propertyArray =
    static_cast<sbIMutablePropertyArray*>(aUserData);
  NS_ASSERTION(propertyArray, "Could not cast user data");

  PRUint32 length = aEntry->Length();
  nsresult rv;
  for (PRUint32 i = 0; i < length; i++) {
    rv = propertyArray->AppendProperty(aKey, aEntry->ElementAt(i));
    NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);
  }

  return PL_DHASH_NEXT;
}

/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListView::AddKeysToStringArrayCallback(nsStringHashKey::KeyType aKey,
                                                           sbStringArray* aEntry,
                                                           void* aUserData)
{
  NS_ASSERTION(aEntry, "Null entry in the hash?!");
  NS_ASSERTION(aUserData, "Null userData!");

  sbStringArray* stringArray = static_cast<sbStringArray*>(aUserData);
  NS_ASSERTION(stringArray, "Could not cast user data");

  nsString* appended = stringArray->AppendElement(aKey);
  NS_ENSURE_TRUE(appended, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

/**
 * This method churns through our listener table and resolves all listeners to
 * strong references that are added to a COM Array. It also prunes invalid or
 * dead weak references.
 */
/* static */ PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListView::AddListenersToCOMArray(nsISupportsHashKey* aEntry,
                                                     void* aUserData)
{
  sbViewListenerArray* array = static_cast<sbViewListenerArray*>(aUserData);
  NS_ASSERTION(array, "Null aUserData!");

  nsISupports* entry = aEntry->GetKey();
  NS_ASSERTION(entry, "Null entry in hash!");

  nsresult rv;
  nsCOMPtr<sbIMediaListViewListener> listener = do_QueryInterface(entry, &rv);
  if (NS_FAILED(rv)) {
    nsWeakPtr maybeWeak = do_QueryInterface(entry, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Listener doesn't QI to anything useful!");

    listener = do_QueryReferent(maybeWeak);
    if (!listener) {
      // The listener died or was invalid. Remove it from our table so that we
      // don't check it again.
      return PL_DHASH_REMOVE;
    }
  }

  PRBool success = array->AppendObject(listener);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);

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
  mListenerTableLock(nsnull),
  mInvalidatePending(PR_FALSE),
  mInitializing(PR_FALSE)
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
}

sbLocalDatabaseMediaListView::~sbLocalDatabaseMediaListView()
{
  TRACE(("sbLocalDatabaseMediaListView[0x%.8x] - Destructed", this));
  MOZ_COUNT_DTOR(sbLocalDatabaseMediaListView);

  if (mMediaList) {
    nsCOMPtr<sbIMediaListListener> listener =
      do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediaListListener*, this));
    mMediaList->RemoveListener(listener);
  }

  if (mCascadeFilterSet) {
    mCascadeFilterSet->ClearMediaListView();
  }

  if (mTreeView) {
    mTreeView->ClearMediaListView();
  }

  if (mListenerTableLock) {
    nsAutoLock::DestroyLock(mListenerTableLock);
  }
}

nsresult
sbLocalDatabaseMediaListView::Init(sbIMediaListViewState* aState)
{
#ifdef DEBUG
  nsString buff;
  if (aState) {
    aState->ToString(buff);
  }
  TRACE(("sbLocalDatabaseMediaListView[0x%.8x] - Init %s",
         this, NS_LossyConvertUTF16toASCII(buff).get()));
#endif

  nsresult rv;

  nsCOMPtr<sbILocalDatabaseMediaListViewState> state;
  if (aState) {
    state = do_QueryInterface(aState, &rv);;
    NS_ENSURE_SUCCESS(rv, NS_ERROR_INVALID_ARG);
  }

  PRBool success = mViewFilters.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mListenerTable.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mListenerTableLock =
    nsAutoLock::NewLock("sbLocalDatabaseMediaListView::mListenerTableLock");
  NS_ENSURE_TRUE(mListenerTableLock, NS_ERROR_OUT_OF_MEMORY);

  mPropMan = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mArray = do_CreateInstance(SB_LOCALDATABASE_ASYNCGUIDARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabasePropertyCache> propertyCache;
  rv = mLibrary->GetPropertyCache(getter_AddRefs(propertyCache));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->SetPropertyCache(propertyCache);
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

  rv = mArray->SetFetchSize(1000);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  mInitializing = PR_TRUE;

  if (state) {
    nsCOMPtr<sbIMutablePropertyArray> sort;
    rv = state->GetSort(getter_AddRefs(sort));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetSort(sort);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMutablePropertyArray> search;
    rv = state->GetSearch(getter_AddRefs(search));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetSearch(search);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMutablePropertyArray> filter;
    rv = state->GetFilter(getter_AddRefs(filter));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetFilters(filter);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    nsCOMPtr<sbIMutablePropertyArray> sort;
    sort = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sort->SetStrict(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = sort->AppendProperty(mDefaultSortProperty, NS_LITERAL_STRING("a"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetSort(sort);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMutablePropertyArray> search;
    search = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetSearch(search);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mInitializing = PR_FALSE;

  // Restore cfs and tree state
  if (state) {
    nsRefPtr<sbLocalDatabaseCascadeFilterSetState> filterSetState;
    rv = state->GetFilterSet(getter_AddRefs(filterSetState));
    NS_ENSURE_SUCCESS(rv, rv);

    if (filterSetState) {
      nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> guidArray;
      rv = mArray->CloneAsyncArray(getter_AddRefs(guidArray));
      NS_ENSURE_SUCCESS(rv, rv);

      nsRefPtr<sbLocalDatabaseCascadeFilterSet> filterSet =
        new sbLocalDatabaseCascadeFilterSet(this);
      NS_ENSURE_TRUE(filterSet, NS_ERROR_OUT_OF_MEMORY);

      rv = filterSet->Init(mLibrary, guidArray, filterSetState);
      NS_ENSURE_SUCCESS(rv, rv);

      mCascadeFilterSet = filterSet;
    }

    nsRefPtr<sbLocalDatabaseTreeViewState> treeViewState;
    rv = state->GetTreeViewState(getter_AddRefs(treeViewState));
    if (treeViewState) {
      nsRefPtr<sbLocalDatabaseTreeView> tree = new sbLocalDatabaseTreeView();
      NS_ENSURE_TRUE(tree, NS_ERROR_OUT_OF_MEMORY);

      rv = tree->Init(this, mArray, nsnull, treeViewState);
      NS_ENSURE_SUCCESS(rv, rv);

      mTreeView = tree;
    }
  }

  rv = UpdateListener(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateViewArrayConfiguration(PR_FALSE);
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

    nsRefPtr<sbLocalDatabaseTreeView> tree = new sbLocalDatabaseTreeView();
    NS_ENSURE_TRUE(tree, NS_ERROR_OUT_OF_MEMORY);

    rv = tree->Init(this, mArray, mViewSort, nsnull);
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

  nsresult rv;

  if (!mCascadeFilterSet) {
    nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> guidArray;
    rv = mArray->CloneAsyncArray(getter_AddRefs(guidArray));
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbLocalDatabaseCascadeFilterSet> filterSet =
      new sbLocalDatabaseCascadeFilterSet(this);
    NS_ENSURE_TRUE(filterSet, NS_ERROR_OUT_OF_MEMORY);

    rv = filterSet->Init(mLibrary, guidArray, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);

    mCascadeFilterSet = filterSet;
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
  rv = mArray->GetGuidByIndex(aIndex, guid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = mLibrary->GetMediaItem(guid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = item);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetIndexForItem(sbIMediaItem* aMediaItem,
                                              PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsAutoString guid;
  nsresult rv = aMediaItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->GetFirstIndexByGuid(guid, _retval);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

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
sbLocalDatabaseMediaListView::GetViewItemUIDForIndex(PRUint32 aIndex,
                                                     nsAString& _retval)
{
  nsresult rv;

  PRUint64 rowid;
  rv = mArray->GetRowidByIndex(aIndex, &rowid);
  NS_ENSURE_SUCCESS(rv, rv);

  _retval.Truncate();
  AppendInt(_retval, rowid);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetIndexForViewItemUID(const nsAString& aViewItemUID,
                                                     PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  PRUint64 rowid = ToInteger64(aViewItemUID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mArray->GetIndexByRowid(rowid, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::Clone(sbIMediaListView** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<sbIMediaListViewState> state;
  rv = GetState(getter_AddRefs(state));
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<sbLocalDatabaseMediaListView>
    clone(new sbLocalDatabaseMediaListView(mLibrary,
                                           mMediaList,
                                           mDefaultSortProperty,
                                           mMediaListId));
  NS_ENSURE_TRUE(clone, NS_ERROR_OUT_OF_MEMORY);

  rv = clone->Init(state);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = clone);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetState(sbIMediaListViewState** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsCOMPtr<sbIMutablePropertyArray> sort;
  rv = ClonePropertyArray(mViewSort, getter_AddRefs(sort));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMutablePropertyArray> search;
  rv = ClonePropertyArray(mViewSearches, getter_AddRefs(search));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMutablePropertyArray> filter =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mViewFilters.EnumerateRead(AddValuesToArrayCallback,
                             filter);

  nsRefPtr<sbLocalDatabaseCascadeFilterSetState> filterSet;
  if (mCascadeFilterSet) {
    rv = mCascadeFilterSet->GetState(getter_AddRefs(filterSet));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsRefPtr<sbLocalDatabaseTreeViewState> treeViewState;
  if (mTreeView) {
    rv = mTreeView->GetState(getter_AddRefs(treeViewState));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsRefPtr<sbLocalDatabaseMediaListViewState> state =
    new sbLocalDatabaseMediaListViewState(sort,
                                          search,
                                          filter,
                                          filterSet,
                                          treeViewState);
  NS_ENSURE_TRUE(state, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(*_retval = state);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::AddListener(sbIMediaListViewListener* aListener,
                                          /* optional */ PRBool aOwnsWeak)
{
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aListener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aOwnsWeak) {
    nsWeakPtr weakRef = do_GetWeakReference(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    supports = do_QueryInterface(weakRef, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsAutoLock lock(mListenerTableLock);
  if (mListenerTable.GetEntry(supports)) {
    NS_WARNING("Attempted to add the same listener twice!");
    return NS_OK;
  }

  if (!mListenerTable.PutEntry(supports)) {
    NS_WARNING("Failed to add entry to listener table");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::RemoveListener(sbIMediaListViewListener* aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);

  nsresult rv;
  nsCOMPtr<nsISupports> supports = do_QueryInterface(aListener, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupports> weakSupports;

  // Test to see if the listener supports weak references *and* a weak reference
  // is in our table. If both conditions are met then that is the listener that
  // will be removed. Otherwise remove a strong listener.
  nsCOMPtr<nsISupportsWeakReference> maybeWeak = do_QueryInterface(supports, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsWeakPtr weakRef = do_GetWeakReference(supports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    weakSupports = do_QueryInterface(weakRef, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsAutoLock lock(mListenerTableLock);

  // Lame, but we have to check this inside the lock.
  if (weakSupports && mListenerTable.GetEntry(weakSupports)) {
    supports = weakSupports;
  }

  NS_WARN_IF_FALSE(mListenerTable.GetEntry(supports),
                   "Attempted to remove a listener that was never added!");

  mListenerTable.RemoveEntry(supports);

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

  PRBool strict;
  rv = aSource->GetValidated(&strict);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = clone->SetStrict(strict);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 propertyCount;
  rv = aSource->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < propertyCount; i++) {
    nsCOMPtr<sbIProperty> property;
    rv = aSource->GetPropertyAt(i, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString propertyID;
    rv = property->GetId(propertyID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString value;
    rv = property->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = clone->AppendProperty(propertyID, value);
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
    nsCOMPtr<sbIProperty> property;
    rv = aBag1->GetPropertyAt(i, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString propertyID;
    rv = property->GetId(propertyID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString junk;
    rv = aBag2->GetPropertyValue(propertyID, junk);
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

nsresult
sbLocalDatabaseMediaListView::UpdateListener(PRBool aRemoveListener)
{
  nsresult rv;

  // Do nothing if initializing
  if (mInitializing) {
    return NS_OK;
  }

  nsCOMPtr<sbIMediaListListener> listener =
    do_QueryInterface(NS_ISUPPORTS_CAST(sbIMediaListListener*, this));

  if (aRemoveListener) {
    rv = mMediaList->RemoveListener(listener);
    NS_ENSURE_SUCCESS(rv, rv);
  }

/*
  XXXsteve Once we fix bug 3875 we can use this code :)

  nsCOMPtr<sbIMutablePropertyArray> filter =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ClonePropertyArray(mViewSort, getter_AddRefs(filter));
  NS_ENSURE_SUCCESS(rv, rv);

  // Also add the ordinal to the filter so we get notified when the list
  // is reordered
  rv = filter->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ORDINAL),
                              EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);
*/
  rv = mMediaList->AddListener(listener,
                               PR_TRUE,
                               sbIMediaList::LISTENER_FLAGS_ALL,
                               nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbLocalDatabaseMediaListView::NotifyListenersInternal(ListenerFunc aListenerFunc)
{
  sbViewListenerArray listeners;
  {
    // Take a snapshot of the listener array. This will return only strong
    // references, so any weak refs that have died will not be included in this
    // list.
    nsAutoLock lock(mListenerTableLock);
    mListenerTable.EnumerateEntries(AddListenersToCOMArray, &listeners);
  }

  sbIMediaListView* thisPtr = static_cast<sbIMediaListView*>(this);

  PRInt32 count = listeners.Count();
  for (PRInt32 index = 0; index < count; index++) {
    sbIMediaListViewListener* listener = listeners.ObjectAt(index);
    (listener->*aListenerFunc)(thisPtr);
  }
}

// sbIFilterableMediaListView
NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetFilterableProperties(nsIStringEnumerator** aFilterableProperties)
{
  NS_ENSURE_ARG_POINTER(aFilterableProperties);

  // Get this from the property manager?
  return NS_ERROR_NOT_IMPLEMENTED;

}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::GetFilterValues(const nsAString& aPropertyID,
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

  rv = query->BindStringParameter(0, aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Flush the property cache
  nsCOMPtr<sbILocalDatabasePropertyCache> propCache;
  rv = mLibrary->GetPropertyCache(getter_AddRefs(propCache));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propCache->Write();
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

  mViewFilters.EnumerateRead(AddValuesToArrayCallback, propertyArray);

  // Add filters from the cascade filter list, if any
  if (mCascadeFilterSet) {
    rv = mCascadeFilterSet->AddFilters(propertyArray);
    NS_ENSURE_SUCCESS(rv, rv);
  }

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
    nsCOMPtr<sbIProperty> property;
    rv = aPropertyArray->GetPropertyAt(index, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the id of the property. This will be the key for the hash table.
    nsString propertyID;
    rv = property->GetId(propertyID);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the property value
    nsString value;
    rv = property->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    sbStringArray* stringArray;
    PRBool arrayExists = mViewFilters.Get(propertyID, &stringArray);
    // If there is an array for this property, search the array for the value
    // and remove it
    if (arrayExists) {

      PRUint32 length = stringArray->Length();
      // Do this backwards so we don't have to deal with the array shifting
      // on us.  Also, be sure to remove multiple copies of the same string.
      for (PRInt32 i = length - 1; i >= 0; i--) {
        if (stringArray->ElementAt(i).Equals(value)) {
          stringArray->RemoveElementAt(i);
          dirty = PR_TRUE;
        }
      }
    }
  }

  if (dirty) {
    rv = UpdateViewArrayConfiguration(PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);

    // And notify listeners
    NotifyListenersFilterChanged();
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::ClearFilters()
{
  mViewFilters.Clear();

  nsresult rv;

  // Clear filters from the cascade filter list, if any
  if (mCascadeFilterSet) {
    rv = mCascadeFilterSet->ClearFilters();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = UpdateViewArrayConfiguration(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // And notify listeners
  NotifyListenersFilterChanged();

  return NS_OK;
}

// sbISearchableMediaListView
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

  nsCOMPtr<sbIMutablePropertyArray> propertyArray;
  rv = ClonePropertyArray(mViewSearches, getter_AddRefs(propertyArray));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add filters from the cascade filter list, if any
  if (mCascadeFilterSet) {
    rv = mCascadeFilterSet->AddSearches(propertyArray);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*aCurrentSearch = propertyArray);
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

  rv = UpdateViewArrayConfiguration(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // And notify listeners
  NotifyListenersSearchChanged();

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::ClearSearch()
{
  nsresult rv;

  nsCOMPtr<nsIMutableArray> array = do_QueryInterface(mViewSearches, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = array->Clear();
  NS_ENSURE_SUCCESS(rv, rv);

  // Clear searches from the cascade filter list, if any
  if (mCascadeFilterSet) {
    rv = mCascadeFilterSet->ClearSearches();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = UpdateViewArrayConfiguration(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // And notify listeners
  NotifyListenersSearchChanged();

  return NS_OK;
}

// sbISortableMediaListView
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

  rv = UpdateViewArrayConfiguration(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateListener();
  NS_ENSURE_SUCCESS(rv, rv);

  // And notify listeners
  NotifyListenersSortChanged();

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

    nsCOMPtr<sbIMutablePropertyArray> propertyArray =
      do_QueryInterface(mViewSort, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = propertyArray->SetStrict(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = propertyArray->AppendProperty(mDefaultSortProperty,
                                       NS_LITERAL_STRING("a"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = UpdateViewArrayConfiguration(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateListener();
  NS_ENSURE_SUCCESS(rv, rv);

  // And notify listeners
  NotifyListenersSortChanged();

  return NS_OK;
}

// sbIMediaListListener
NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnItemAdded(sbIMediaList* aMediaList,
                                          sbIMediaItem* aMediaItem,
                                          PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  if (mBatchHelper.IsActive()) {
    mInvalidatePending = PR_TRUE;
    *aNoMoreForBatch = PR_TRUE;
    return NS_OK;
  }

  // Invalidate the view array
  nsresult rv = Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                                  sbIMediaItem* aMediaItem,
                                                  PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  // Don't care

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                                 sbIMediaItem* aMediaItem,
                                                 PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  if (mBatchHelper.IsActive()) {
    mInvalidatePending = PR_TRUE;
    *aNoMoreForBatch = PR_TRUE;
    return NS_OK;
  }

  // Invalidate the view array
  nsresult rv = Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnItemUpdated(sbIMediaList* aMediaList,
                                            sbIMediaItem* aMediaItem,
                                            sbIPropertyArray* aProperties,
                                            PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

#ifdef PR_LOGGING
  nsAutoString buff;
  aProperties->ToString(buff);
  TRACE(("sbLocalDatabaseMediaListView[0x%.8x] - OnItemUpdated %s",
         this, NS_ConvertUTF16toUTF8(buff).get()));
#endif

  nsresult rv;

  // If we are in a batch, we don't need any more notifications since we always
  // invalidate when a batch ends
  PRBool shouldInvalidate;
  if (mBatchHelper.IsActive()) {
    shouldInvalidate = PR_FALSE;
    mInvalidatePending = PR_TRUE;
    *aNoMoreForBatch = PR_TRUE;
  }
  else {
    // If we are not in a batch, check to see if this update should cause an
    // invalidation
    rv = ShouldCauseInvalidation(aProperties, &shouldInvalidate);
    NS_ENSURE_SUCCESS(rv, rv);
    *aNoMoreForBatch = PR_FALSE;
  }

  if (shouldInvalidate) {
    // Invalidate the view array
    nsresult rv = Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    // If the array has not already been invalidated, we should invalidate the
    // row of the tree view that contains this item.  This lets us see updates
    // that don't cause invalidations
    if (mTreeView) {
      nsAutoString guid;
      rv = aMediaItem->GetGuid(guid);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mTreeView->InvalidateRowsByGuid(guid);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnListCleared(sbIMediaList* aMediaList,
                                            PRBool* aNoMoreForBatch)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aNoMoreForBatch);

  if (mBatchHelper.IsActive()) {
    mInvalidatePending = PR_TRUE;
    *aNoMoreForBatch = PR_TRUE;
    return NS_OK;
  }

  // Invalidate the view array
  nsresult rv = Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  *aNoMoreForBatch = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnBatchBegin(sbIMediaList* aMediaList)
{
  mBatchHelper.Begin();
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseMediaListView::OnBatchEnd(sbIMediaList* aMediaList)
{
  mBatchHelper.End();

  if (mInvalidatePending) {
    // Invalidate the view array
    nsresult rv = Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);

    mInvalidatePending = PR_FALSE;
  }

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
    nsCOMPtr<sbIProperty> property;
    rv = aPropertyArray->GetPropertyAt(index, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the id of the property. This will be the key for the hash table.
    nsString propertyID;
    rv = property->GetId(propertyID);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the property value
    nsString value;
    rv = property->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    // If the string is null and we are replacing, we should delete the
    // property from the hash
    if (value.IsVoid()) {
      if (aReplace) {
        mViewFilters.Remove(propertyID);
      }
    }
    else {
      // Get the string array associated with the key. If it doesn't yet exist
      // then we need to create it.
      sbStringArray* stringArray;
      PRBool arrayExists = mViewFilters.Get(propertyID, &stringArray);

      if (!arrayExists) {
        NS_NEWXPCOM(stringArray, sbStringArray);
        NS_ENSURE_TRUE(stringArray, NS_ERROR_OUT_OF_MEMORY);

        // Try to add the array to the hash table.
        success = mViewFilters.Put(propertyID, stringArray);
        NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
      }

      if (aReplace) {
        // If this is the first time we've seen this property, clear the
        // string array
        if (!seenProperties.GetEntry(propertyID)) {
          stringArray->Clear();
          nsStringHashKey* successKey = seenProperties.PutEntry(propertyID);
          NS_ENSURE_TRUE(successKey, NS_ERROR_OUT_OF_MEMORY);
        }
      }
      // Now we need a slot for the property value.
      nsString* appended = stringArray->AppendElement(value);
      NS_ENSURE_TRUE(appended, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  rv = UpdateViewArrayConfiguration(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // And notify listeners
  NotifyListenersFilterChanged();

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListView::UpdateViewArrayConfiguration(PRBool aClearTreeSelection)
{
  nsresult rv;

  // Do nothing if initializing
  if (mInitializing) {
    return NS_OK;
  }

  // Update filters
  rv = mArray->ClearFilters();
  NS_ENSURE_SUCCESS(rv, rv);

  sbStringArray filterProperties;
  mViewFilters.EnumerateRead(AddKeysToStringArrayCallback, &filterProperties);
  PRUint32 length = filterProperties.Length();
  for (PRUint32 i = 0; i < length; i++) {

    sbStringArray* values;
    PRBool success = mViewFilters.Get(filterProperties[i], &values);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

    nsCOMPtr<sbIPropertyInfo> info;
    rv = mPropMan->GetPropertyInfo(filterProperties[i], getter_AddRefs(info));
    NS_ENSURE_SUCCESS(rv, rv);

    sbStringArray sortableValues;

    PRUint32 valueCount = values->Length();
    for (PRUint32 j = 0; j < valueCount; j++) {

      nsAutoString sortableValue;
      // Top level properties are not searched as sortable
      if (SB_IsTopLevelProperty(filterProperties[i])) {
        sortableValue = values->ElementAt(j);
      }
      else {
        rv = info->MakeSortable(values->ElementAt(j), sortableValue);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      nsString* appended = sortableValues.AppendElement(sortableValue);
      NS_ENSURE_TRUE(appended, NS_ERROR_OUT_OF_MEMORY);
    }

    // Make a string enumerator for the string array.
    nsCOMPtr<nsIStringEnumerator> valueEnum =
      new sbTArrayStringEnumerator(&sortableValues);
    NS_ENSURE_TRUE(valueEnum, NS_ERROR_OUT_OF_MEMORY);

    // Set the filter.
    rv = mArray->AddFilter(filterProperties[i], valueEnum, PR_FALSE);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "AddFilter failed!");
  }

  // Update searches
  PRUint32 propertyCount;
  rv = mViewSearches->GetLength(&propertyCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = 0; index < propertyCount; index++) {

    nsCOMPtr<sbIProperty> property;
    rv = mViewSearches->GetPropertyAt(index, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString propertyID;
    rv = property->GetId(propertyID);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString value;
    rv = property->GetValue(value);
    NS_ENSURE_SUCCESS(rv, rv);

    // Treat a search with an emtpy search as if it wasn't there
    if (value.IsEmpty()) {
      continue;
    }

    nsCOMPtr<sbIPropertyInfo> info;
    rv = mPropMan->GetPropertyInfo(propertyID, getter_AddRefs(info));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString sortableValue;
    rv = info->MakeSortable(value, sortableValue);
    NS_ENSURE_SUCCESS(rv, rv);

    sbStringArray valueArray(1);
    nsString* successString = valueArray.AppendElement(sortableValue);
    NS_ENSURE_TRUE(successString, NS_ERROR_OUT_OF_MEMORY);

    nsCOMPtr<nsIStringEnumerator> valueEnum =
      new sbTArrayStringEnumerator(&valueArray);
    NS_ENSURE_TRUE(valueEnum, NS_ERROR_OUT_OF_MEMORY);

    rv = mArray->AddFilter(propertyID, valueEnum, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Add configuration from the cascade filter list, if any
  if (mCascadeFilterSet) {
    rv = mCascadeFilterSet->AddConfiguration(mArray);
    NS_ENSURE_SUCCESS(rv, rv);
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

      nsCOMPtr<sbIProperty> property;
      rv = mViewSort->GetPropertyAt(index, getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString propertyID;
      rv = property->GetId(propertyID);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString value;
      rv = property->GetValue(value);
      NS_ENSURE_SUCCESS(rv, rv);

      mArray->AddSort(propertyID, value.EqualsLiteral("a"));
      NS_ENSURE_SUCCESS(rv, rv);

      hasSorts = PR_TRUE;
    }
  }

  // If no sort is specified, use the default sort
  if (!hasSorts) {
    mArray->AddSort(mDefaultSortProperty, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aClearTreeSelection && mTreeView) {
    nsCOMPtr<nsITreeSelection> selection;
    rv = mTreeView->GetSelection(getter_AddRefs(selection));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = selection->ClearSelection();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Invalidate the view array
  rv = Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

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
