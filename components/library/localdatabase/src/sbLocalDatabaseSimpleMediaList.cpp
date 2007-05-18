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

#include "sbLocalDatabaseSimpleMediaList.h"

#include <nsIArray.h>
#include <nsIMutableArray.h>
#include <nsIProgrammingLanguage.h>
#include <nsISimpleEnumerator.h>
#include <nsIURI.h>
#include <sbIDatabaseQuery.h>
#include <sbIDatabaseResult.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbIMediaListView.h>
#include <sbISQLBuilder.h>

#include "sbLocalDatabaseCID.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseGUIDArray.h"

#include <DatabaseQuery.h>
#include <nsArrayUtils.h>
#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsComponentManagerUtils.h>
#include <nsMemory.h>
#include <nsXPCOMCID.h>
#include <pratom.h>
#include <sbLocalDatabaseMediaListView.h>
#include <sbStandardProperties.h>
#include <sbSQLBuilderCID.h>

#define DEFAULT_SORT_PROPERTY NS_LITERAL_STRING(SB_PROPERTY_ORDINAL)
#define DEFAULT_FETCH_SIZE 1000

#ifdef DEBUG
#define ASSERT_LIST_IS_LIBRARY(_mediaList)                                     \
  PR_BEGIN_MACRO                                                               \
    nsresult rv;                                                               \
    nsCOMPtr<sbILibrary> library = do_QueryInterface(mLibrary, &rv);           \
    NS_ASSERTION(NS_SUCCEEDED(rv), "Library won't QI to sbILibrary!");         \
                                                                               \
    if (NS_SUCCEEDED(rv)) {                                                    \
      PRBool listIsLibrary;                                                    \
      rv = library->Equals(_mediaList, &listIsLibrary);                        \
      NS_ASSERTION(NS_SUCCEEDED(rv), "Equals failed!");                        \
                                                                               \
      if (NS_SUCCEEDED(rv)) {                                                  \
        NS_ASSERTION(listIsLibrary, "Watching the wrong list!");               \
      }                                                                        \
    }                                                                          \
  PR_END_MACRO
#else /* DEBUG */
#define ASSERT_LIST_IS_LIBRARY(_mediaList)                                     \
  PR_BEGIN_MACRO /* nothing */ PR_END_MACRO
#endif /* DEBUG */

NS_IMPL_ISUPPORTS1(sbSimpleMediaListInsertingEnumerationListener,
                   sbIMediaListEnumerationListener)

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbSimpleMediaListInsertingEnumerationListener::OnEnumerationBegin(sbIMediaList* aMediaList,
                                                                  PRBool* _retval)
{
  NS_ASSERTION(aMediaList != mFriendList,
               "Can't enumerate our friend media list!");

  PRBool success = mItemsToCreate.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = mFriendList->GetLibrary(getter_AddRefs(mListLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // All good for enumerating.
  if (_retval) {
    *_retval = PR_TRUE;
  }

  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbSimpleMediaListInsertingEnumerationListener::OnEnumeratedItem(sbIMediaList* aMediaList,
                                                                sbIMediaItem* aMediaItem,
                                                                PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  NS_ASSERTION(aMediaList != mFriendList,
               "Can't enumerate our friend media list!");

  nsCOMPtr<sbILibrary> itemLibrary;
  nsresult rv = aMediaItem->GetLibrary(getter_AddRefs(itemLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool sameLibrary;
  rv = itemLibrary->Equals(mListLibrary, &sameLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success;
  if (!sameLibrary && !mItemsToCreate.Get(aMediaItem, nsnull)) {
    // This item comes from another library so we'll need to add it to our
    // library before it can be used.
    success = mItemsToCreate.Put(aMediaItem, nsnull);
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  }

  // Remember this media item.
  success = mItemList.AppendObject(aMediaItem);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  if (_retval) {
    *_retval = PR_TRUE;
  }

  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbSimpleMediaListInsertingEnumerationListener::OnEnumerationEnd(sbIMediaList* aMediaList,
                                                                nsresult aStatusCode)
{
  NS_ASSERTION(aMediaList != mFriendList,
               "Can't enumerate our friend media list!");

  PRUint32 itemCount = mItemList.Count();
  if (!itemCount) {
    NS_WARNING("OnEnumerationEnd called with no items enumerated");
    return NS_OK;
  }

  nsresult rv;

  PRUint32 itemsToCreateCount = mItemsToCreate.Count();
  if (itemsToCreateCount) {
    nsCOMPtr<nsIMutableArray> oldItems =
      do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIMutableArray> oldURIs =
      do_CreateInstance(NS_ARRAY_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    ArrayPointers pointers = {oldItems, oldURIs};

    PRUint32 enumeratedCount =
      mItemsToCreate.EnumerateRead(AddURIsToArrayCallback, &pointers);
    NS_ENSURE_TRUE(enumeratedCount == itemsToCreateCount, NS_ERROR_FAILURE);

    nsCOMPtr<nsIArray> newItems;
    rv = mListLibrary->BatchCreateMediaItems(oldURIs,
                                             getter_AddRefs(newItems));
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
    PRUint32 newItemCount;
    rv = newItems->GetLength(&newItemCount);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(newItemCount == itemsToCreateCount,
                 "BatchCreateMediaItems didn't make the right number of items!");
#endif

    for (PRUint32 index = 0; index < itemsToCreateCount; index++) {
      nsCOMPtr<sbIMediaItem> oldItem = do_QueryElementAt(oldItems, index, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIMediaItem> newItem = do_QueryElementAt(newItems, index, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      NS_ASSERTION(mItemsToCreate.Get(oldItem, nsnull),
                   "The old item should be in the hashtable!");

      PRBool success = mItemsToCreate.Put(oldItem, newItem);
      NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
    }
  }

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = mFriendList->MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("begin"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString ordinal;
  rv = mFriendList->GetNextOrdinal(ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 index = 0; index < itemCount; index++) {

    nsCOMPtr<sbIMediaItem> mediaItem = mItemList[index];

    if (mItemsToCreate.Get(mediaItem, nsnull)) {
      nsCOMPtr<sbIMediaItem> newMediaItem;
      PRBool success = mItemsToCreate.Get(mediaItem, getter_AddRefs(newMediaItem));
      NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

      rv = mFriendList->CopyStandardProperties(mediaItem, newMediaItem);
      NS_ENSURE_SUCCESS(rv, rv);

      //Call the copy listener for this media list at this time.
      //XXXAus: This could benefit from batching in the future.
      rv = mFriendList->NotifyCopyListener(mediaItem, newMediaItem);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to notify copy listener!");

      success = mItemList.ReplaceObjectAt(newMediaItem, index);
      NS_ENSURE_SUCCESS(rv, rv);

      mediaItem = newMediaItem;
    }

    nsCOMPtr<sbILocalDatabaseMediaItem> ldbmi =
      do_QueryInterface(mediaItem, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 mediaItemId;
    rv = ldbmi->GetMediaItemId(&mediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(mFriendList->mInsertIntoListQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindInt32Parameter(0, mediaItemId);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->BindStringParameter(1, ordinal);
    NS_ENSURE_SUCCESS(rv, rv);

    // Increment the ordinal
    rv = mFriendList->AddToLastPathSegment(ordinal, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->AddQuery(NS_LITERAL_STRING("commit"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbSuccess;
  rv = query->Execute(&dbSuccess);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbSuccess, NS_ERROR_FAILURE);

  // Invalidate the cached list
  rv = mFriendList->mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  // Notify our listeners
  for (PRUint32 index = 0; index < itemCount; index++) {
    mFriendList->NotifyListenersItemAdded(mFriendList, mItemList[index]);
  }

  return NS_OK;
}

/* static */ PLDHashOperator PR_CALLBACK
sbSimpleMediaListInsertingEnumerationListener::AddURIsToArrayCallback(nsISupportsHashKey::KeyType aKey,
                                                                      sbIMediaItem* aEntry,
                                                                      void* aUserData)
{
  ArrayPointers* pointers = NS_STATIC_CAST(ArrayPointers*, aUserData);
  NS_ASSERTION(pointers, "This can't be null!");

  nsresult rv;
  nsCOMPtr<sbIMediaItem> mediaItem = do_QueryInterface(aKey, &rv);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  nsCOMPtr<nsIURI> uri;
  rv = mediaItem->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = pointers->items->AppendElement(mediaItem, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  rv = pointers->uris->AppendElement(uri, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

NS_IMPL_ISUPPORTS1(sbSimpleMediaListRemovingEnumerationListener,
                   sbIMediaListEnumerationListener)

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbSimpleMediaListRemovingEnumerationListener::OnEnumerationBegin(sbIMediaList* aMediaList,
                                                                 PRBool* _retval)
{
  // Prep the query
  nsresult rv = mFriendList->MakeStandardQuery(getter_AddRefs(mDBQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->AddQuery(NS_LITERAL_STRING("begin"));
  NS_ENSURE_SUCCESS(rv, rv);

  if (_retval) {
    *_retval = PR_TRUE;
  }
  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbSimpleMediaListRemovingEnumerationListener::OnEnumeratedItem(sbIMediaList* aMediaList,
                                                               sbIMediaItem* aMediaItem,
                                                               PRBool* _retval)
{
  NS_ASSERTION(aMediaItem, "Null pointer!");

  // Remember this media item for later so we can notify with it
  PRBool success = mNotificationList.AppendObject(aMediaItem);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;
  nsCOMPtr<sbILocalDatabaseMediaItem> ldbmi =
    do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->AddQuery(mFriendList->mDeleteFirstListItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = ldbmi->GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDBQuery->BindInt32Parameter(0, mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  if (_retval) {
    *_retval = PR_TRUE;
  }

  mItemEnumerated = PR_TRUE;
  return NS_OK;
}

/**
 * See sbIMediaListListener.idl
 */
NS_IMETHODIMP
sbSimpleMediaListRemovingEnumerationListener::OnEnumerationEnd(sbIMediaList* aMediaList,
                                                               nsresult aStatusCode)
{
  nsresult rv;

  // Notify our listeners before removal
  PRUint32 count = mNotificationList.Count();
  for (PRUint32 i = 0; i < count; i++) {
    mFriendList->NotifyListenersBeforeItemRemoved(mFriendList,
                                                  mNotificationList[i]);
  }

  if (mItemEnumerated) {
    rv = mDBQuery->AddQuery(NS_LITERAL_STRING("commit"));
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 dbSuccess;
    rv = mDBQuery->Execute(&dbSuccess);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_SUCCESS(dbSuccess, NS_ERROR_FAILURE);
  }

  // Invalidate the cached list
  rv = mFriendList->mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  // Notify our listeners after removal
  for (PRUint32 i = 0; i < count; i++) {
    mFriendList->NotifyListenersAfterItemRemoved(mFriendList,
                                            mNotificationList[i]);
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS_INHERITED4(sbLocalDatabaseSimpleMediaList,
                             sbLocalDatabaseMediaListBase,
                             nsIClassInfo,
                             sbIMediaListListener,
                             sbIOrderableMediaList,
                             sbILocalDatabaseSimpleMediaList)

NS_IMPL_CI_INTERFACE_GETTER8(sbLocalDatabaseSimpleMediaList,
                             nsIClassInfo,
                             nsISupportsWeakReference,
                             sbIMediaListListener,
                             sbILibraryResource,
                             sbIMediaItem,
                             sbIMediaList,
                             sbILocalDatabaseSimpleMediaList,
                             sbIOrderableMediaList);

nsresult
sbLocalDatabaseSimpleMediaList::Init(sbILocalDatabaseLibrary* aLibrary,
                                     const nsAString& aGuid)
{
  nsresult rv = sbLocalDatabaseMediaListBase::Init(aLibrary, aGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  mFullArray = new sbLocalDatabaseGUIDArray();
  NS_ENSURE_TRUE(mFullArray, NS_ERROR_OUT_OF_MEMORY);

  PRUint32 mediaItemId;
  rv = GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString databaseGuid;
  rv = mLibrary->GetDatabaseGuid(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetDatabaseGUID(databaseGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> databaseLocation;
  rv = mLibrary->GetDatabaseLocation(getter_AddRefs(databaseLocation));
  NS_ENSURE_SUCCESS(rv, rv);

  if (databaseLocation) {
    rv = mFullArray->SetDatabaseLocation(databaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mFullArray->SetBaseTable(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetBaseConstraintColumn(NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetBaseConstraintValue(mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->AddSort(DEFAULT_SORT_PROPERTY, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetFetchSize(DEFAULT_FETCH_SIZE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabasePropertyCache> propertyCache;
  rv = aLibrary->GetPropertyCache(getter_AddRefs(propertyCache));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->SetPropertyCache(propertyCache);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  // And make sure that we know when items get removed from the library.
  nsCOMPtr<sbIMediaList> libraryList = do_QueryInterface(aLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryList->AddListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mShouldNotifyAfterRemove.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetType(nsAString& aType)
{
  aType.Assign(NS_LITERAL_STRING("simple"));
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetItemByGuid(const nsAString& aGuid,
                                              sbIMediaItem** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  nsCOMPtr<sbIMediaItem> item;
  rv = sbLocalDatabaseMediaListBase::GetItemByGuid(aGuid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool contains;
  rv = Contains(item, &contains);
  NS_ENSURE_SUCCESS(rv, rv);

  if (contains) {
    NS_ADDREF(*_retval = item);
    return NS_OK;
  }
  else {
    return NS_ERROR_NOT_AVAILABLE;
  }

}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::Contains(sbIMediaItem* aMediaItem,
                                         PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetMediaItemIdForGuidQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString guid;
  rv = aMediaItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, guid);
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

  *_retval = rowCount > 0;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::Add(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbSimpleMediaListInsertingEnumerationListener listener(this);

  nsresult rv = listener.OnEnumerationBegin(nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listener.OnEnumeratedItem(nsnull, aMediaItem, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listener.OnEnumerationEnd(nsnull, NS_OK);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::AddAll(sbIMediaList* aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbAutoBatchHelper batchHelper(this);

  sbSimpleMediaListInsertingEnumerationListener listener(this);
  nsresult rv =
    aMediaList->EnumerateAllItems(&listener,
                                  sbIMediaList::ENUMERATIONTYPE_SNAPSHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::AddSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbSimpleMediaListInsertingEnumerationListener listener(this);

  nsresult rv = listener.OnEnumerationBegin(nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  sbAutoBatchHelper batchHelper(this);

  PRBool hasMore;
  while (NS_SUCCEEDED(aMediaItems->HasMoreElements(&hasMore)) && hasMore) {
    nsCOMPtr<nsISupports> supports;
    rv = aMediaItems->GetNext(getter_AddRefs(supports));
    SB_CONTINUE_IF_FAILED(rv);

    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(supports, &rv);
    SB_CONTINUE_IF_FAILED(rv);

    rv = listener.OnEnumeratedItem(nsnull, item, nsnull);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnEnumeratedItem failed!");    
  }

  rv = listener.OnEnumerationEnd(nsnull, NS_OK);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::InsertBefore(PRUint32 aIndex,
                                             sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  nsresult rv;
  PRInt32 dbOk;

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_MAX(aIndex, length - 1);

  nsAutoString ordinal;
  rv = GetBeforeOrdinal(aIndex, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  // Run the insert query
  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mInsertIntoListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILocalDatabaseMediaItem> ldbmi =
    do_QueryInterface(aMediaItem, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = ldbmi->GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(0, mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(1, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  NotifyListenersItemAdded(this, aMediaItem);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::MoveBefore(PRUint32 aFromIndex,
                                           PRUint32 aToIndex)
{
  // Do nothing if we don't have to move
  if (aFromIndex == aToIndex) {
    return NS_OK;
  }

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;

  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_MAX(aFromIndex, length - 1);
  NS_ENSURE_ARG_MAX(aToIndex,   length - 1);

  // Get the ordinal of the space before the to index
  nsAutoString ordinal;
  rv = GetBeforeOrdinal(aToIndex, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateOrdinalByIndex(aFromIndex, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::MoveLast(PRUint32 aIndex)
{
  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;

  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_ARG_MAX(aIndex, length - 1);

  // Get the ordinal for the space after the last item in the list
  nsAutoString ordinal;
  rv = GetNextOrdinal(ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = UpdateOrdinalByIndex(aIndex, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::Remove(sbIMediaItem* aMediaItem)
{
  NS_ENSURE_ARG_POINTER(aMediaItem);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  // Use our enumeration helper to make all these remove calls use the same
  // code.
  sbSimpleMediaListRemovingEnumerationListener listener(this);

  nsresult rv = listener.OnEnumerationBegin(nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listener.OnEnumeratedItem(nsnull, aMediaItem, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = listener.OnEnumerationEnd(nsnull, NS_OK);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::RemoveByIndex(PRUint32 aIndex)
{
  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;

  nsAutoString ordinal;
  rv = mFullArray->GetSortPropertyValueByIndex(aIndex, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = GetItemByIndex(aIndex, getter_AddRefs(item));
  NotifyListenersBeforeItemRemoved(this, item);

  nsCOMPtr<sbIDatabaseQuery> dbQuery;
  rv = MakeStandardQuery(getter_AddRefs(dbQuery));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbQuery->AddQuery(mDeleteListItemByOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = dbQuery->BindStringParameter(0, ordinal);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbSuccess;
  rv = dbQuery->Execute(&dbSuccess);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbSuccess, NS_ERROR_FAILURE);

  rv = mFullArray->RemoveByIndex(aIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  NotifyListenersAfterItemRemoved(this, item);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::RemoveSome(nsISimpleEnumerator* aMediaItems)
{
  NS_ENSURE_ARG_POINTER(aMediaItems);

  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  sbSimpleMediaListRemovingEnumerationListener listener(this);

  nsresult rv = listener.OnEnumerationBegin(nsnull, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasMore;
  while (NS_SUCCEEDED(aMediaItems->HasMoreElements(&hasMore)) && hasMore) {

    nsCOMPtr<nsISupports> supports;
    rv = aMediaItems->GetNext(getter_AddRefs(supports));
    SB_CONTINUE_IF_FAILED(rv);

    nsCOMPtr<sbIMediaItem> item = do_QueryInterface(supports, &rv);
    SB_CONTINUE_IF_FAILED(rv);

    rv = listener.OnEnumeratedItem(nsnull, item, nsnull);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnEnumeratedItem failed!");
  }

  rv = listener.OnEnumerationEnd(nsnull, NS_OK);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::Clear()
{
  SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE();

  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mDeleteAllQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  // Invalidate the cached list
  rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::CreateView(sbIMediaListView** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  nsAutoString defaultSortProperty;
  rv = GetDefaultSortProperty(defaultSortProperty);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 mediaItemId;
  rv = GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoPtr<sbLocalDatabaseMediaListView>
    view(new sbLocalDatabaseMediaListView(mLibrary,
                                          this,
                                          defaultSortProperty,
                                          mediaItemId));
  NS_ENSURE_TRUE(view, NS_ERROR_OUT_OF_MEMORY);
  rv = view->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = view.forget());
  return NS_OK;
}

// sbILocalDatabaseSimpleMediaList
NS_IMETHODIMP 
sbLocalDatabaseSimpleMediaList::GetCopyListener(
  sbILocalDatabaseMediaListCopyListener * *aCopyListener)
{
  NS_ENSURE_ARG_POINTER(aCopyListener);

  *aCopyListener = nsnull;
  
  if(mCopyListener) {
    NS_ADDREF(*aCopyListener = mCopyListener);
  }

  return NS_OK;
}

NS_IMETHODIMP sbLocalDatabaseSimpleMediaList::SetCopyListener(
  sbILocalDatabaseMediaListCopyListener * aCopyListener)
{
  NS_ENSURE_ARG_POINTER(aCopyListener);

  mCopyListener = aCopyListener;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetIndexByOrdinal(const nsAString& aOrdinal,
                                                  PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  // First, search the cache for this ordinal
  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < length; i++) {
    PRBool isCached;
    rv = mFullArray->IsIndexCached(i, &isCached);
    NS_ENSURE_SUCCESS(rv, rv);

    if (isCached) {
      nsAutoString ordinal;
      rv = mFullArray->GetOrdinalByIndex(i, ordinal);
      NS_ENSURE_SUCCESS(rv, rv);

      if (ordinal.Equals(aOrdinal)) {
        *_retval = i;
        return NS_OK;
      }
    }
  }

  // Not cached, search the database
  PRUint32 index;
  rv = mFullArray->GetFirstIndexByPrefix(aOrdinal, &index);
  if (NS_SUCCEEDED(rv)) {
    *_retval = index;
    return NS_OK;
  }

  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::Invalidate()
{
  nsresult rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaList> list(this);
  nsCOMPtr<sbIMediaItem> item = do_QueryInterface(list, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NotifyListenersItemUpdated(this, item);
  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::ExecuteAggregateQuery(const nsAString& aQuery,
                                                      nsAString& aValue)
{
  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(aQuery);
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
    return NS_ERROR_UNEXPECTED;
  }

  rv = result->GetRowCell(0, 0, aValue);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::UpdateOrdinalByIndex(PRUint32 aIndex,
                                                     const nsAString& aOrdinal)
{
  nsresult rv;
  PRInt32 dbOk;

  // Get the media item id of the item we are moving
  PRUint32 mediaItemId;
  rv = mFullArray->GetMediaItemIdByIndex(aIndex, &mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  // Update the item at the from index with the new ordinal
  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mUpdateListItemOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aOrdinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(1, mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::DeleteItemByMediaItemId(PRUint32 aMediaItemId)
{
  nsresult rv;
  PRInt32 dbOk;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeStandardQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mDeleteFirstListItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(0, aMediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::GetNextOrdinal(nsAString& aValue)
{
  nsresult rv;

  PRUint32 length;
  rv = mFullArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if (length == 0) {
    aValue.AssignLiteral("0");
    return NS_OK;
  }

  PRBool cached;
  rv = mFullArray->IsIndexCached(length - 1, &cached);
  NS_ENSURE_SUCCESS(rv, rv);

  if (cached) {
    rv = mFullArray->GetSortPropertyValueByIndex(length - 1, aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = ExecuteAggregateQuery(mGetLastOrdinalQuery, aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = AddToLastPathSegment(aValue, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::GetBeforeOrdinal(PRUint32 aIndex,
                                                 nsAString& aValue)
{
  nsresult rv;

  // If we want to insert before the first index, get the ordinal of the
  // first index and trim off everything but the first path and subtract 1
  if (aIndex == 0) {
    PRBool cached;
    rv = mFullArray->IsIndexCached(0, &cached);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString ordinal;
    if (cached) {
      rv = mFullArray->GetSortPropertyValueByIndex(0, ordinal);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      rv = ExecuteAggregateQuery(mGetFirstOrdinalQuery, ordinal);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    // Trim off additional path segments, if any
    PRUint32 pos = ordinal.FindChar('.');
    if (pos >= 0) {
      ordinal.SetLength(pos);
    }

    PRInt32 value = ordinal.ToInteger(&rv) - 1;
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString newOrdinal;
    newOrdinal.AppendInt(value);
    aValue = newOrdinal;

    return NS_OK;
  }

  // Find the ordinals before and after the place we want to insert
  nsAutoString aboveOrdinal;
  nsAutoString belowOrdinal;

  rv = mFullArray->GetSortPropertyValueByIndex(aIndex - 1, aboveOrdinal);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFullArray->GetSortPropertyValueByIndex(aIndex, belowOrdinal);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 aboveLevels = CountLevels(aboveOrdinal);
  PRUint32 belowLevels = CountLevels(belowOrdinal);

  // If the two paths are to the same level, create a new level on the
  // below path to make a path that sorts between
  if (aboveLevels == belowLevels) {
    belowOrdinal.AppendLiteral(".0");
    aValue = belowOrdinal;
    return NS_OK;
  }

  // Otherwise if the below path is longer than the above path, decrement the
  // last path segment of the below path
  if (belowLevels > aboveLevels) {
    rv = AddToLastPathSegment(belowOrdinal, -1);
    NS_ENSURE_SUCCESS(rv, rv);
    aValue = belowOrdinal;
    return NS_OK;
  }

  // If the above path is longer than the below, increment the last path
  // segment of the above path
  rv = AddToLastPathSegment(aboveOrdinal, 1);
  NS_ENSURE_SUCCESS(rv, rv);
  aValue = aboveOrdinal;
  return NS_OK;
}

nsresult
sbLocalDatabaseSimpleMediaList::AddToLastPathSegment(nsAString& aPath,
                                                     PRInt32 aToAdd)
{

  PRUint32 startPos = aPath.RFindChar('.') + 1;
  PRUint32 length = aPath.Length() - startPos;

  nsresult rv;
  PRInt32 value = Substring(aPath, startPos, length).ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  value += aToAdd;

  nsAutoString newValue;
  newValue.AppendInt(value);
  aPath.Replace(startPos, length, newValue);

  return NS_OK;
}

PRUint32
sbLocalDatabaseSimpleMediaList::CountLevels(const nsAString& aPath)
{
  PRUint32 count = 0;
  PRInt32 foundpos = aPath.FindChar('.');
  while(foundpos >= 0) {
    count++;
    foundpos = aPath.FindChar('.', foundpos + 1);
  }
  return count;
}

nsresult
sbLocalDatabaseSimpleMediaList::CreateQueries()
{
  nsresult rv;

  PRUint32 mediaItemId;
  rv = GetMediaItemId(&mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  // The following builder is shared between queries that operate on the
  // items in this list
  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;

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
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create get last ordinal query
  rv = builder->ClearColumns();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("max(ordinal)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetLastOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create get first ordinal query
  rv = builder->ClearColumns();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(),
                          NS_LITERAL_STRING("min(ordinal)"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetFirstOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the query used by contains to see if a guid is in this list
  rv = builder->ClearColumns();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(NS_LITERAL_STRING("_mi"),
                          NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionParameter(NS_LITERAL_STRING("_mi"),
                                              NS_LITERAL_STRING("guid"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->ToString(mGetMediaItemIdForGuidQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create insertion query
  nsCOMPtr<sbISQLInsertBuilder> insert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);

  rv = insert->SetIntoTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("member_media_item_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddColumn(NS_LITERAL_STRING("ordinal"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueLong(mediaItemId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insert->ToString(mInsertIntoListQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create ordinal update query
  nsCOMPtr<sbISQLUpdateBuilder> update =
    do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);

  rv = update->SetTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->AddAssignmentParameter(NS_LITERAL_STRING("ordinal"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->CreateMatchCriterionLong(EmptyString(),
                                        NS_LITERAL_STRING("media_item_id"),
                                        sbISQLSelectBuilder::MATCH_EQUALS,
                                        mediaItemId,
                                        getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->CreateMatchCriterionParameter(EmptyString(),
                                             NS_LITERAL_STRING("member_media_item_id"),
                                             sbISQLSelectBuilder::MATCH_EQUALS,
                                             getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = update->ToString(mUpdateListItemOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create first item delete query
  // delete from
  //   simple_media_lists
  // where
  //   media_item_id = x and
  //   ordinal in (
  //     select
  //       ordinal
  //     from
  //       simple_media_lists
  //     where
  //       member_media_item_id = ?
  //     order by ordinal limit 1)
  nsCOMPtr<sbISQLDeleteBuilder> deleteb = 
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->CreateMatchCriterionLong(EmptyString(),
                                         NS_LITERAL_STRING("media_item_id"),
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  // Build the subquery
  rv = builder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING("rowid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("member_media_item_id"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->CreateMatchCriterionLong(EmptyString(),
                                         NS_LITERAL_STRING("media_item_id"),
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddOrder(EmptyString(), NS_LITERAL_STRING("ordinal"), PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetLimit(1);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterionIn> inCriterion;
  rv = deleteb->CreateMatchCriterionIn(EmptyString(),
                                       NS_LITERAL_STRING("rowid"),
                                       getter_AddRefs(inCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = inCriterion->AddSubquery(builder);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(inCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mDeleteFirstListItemQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create item delete by ordinal query
  rv = deleteb->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->CreateMatchCriterionLong(EmptyString(),
                                         NS_LITERAL_STRING("media_item_id"),
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("ordinal"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mDeleteListItemByOrdinalQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create delete all query
  rv = deleteb->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("simple_media_lists"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->CreateMatchCriterionLong(EmptyString(),
                                         NS_LITERAL_STRING("media_item_id"),
                                         sbISQLSelectBuilder::MATCH_EQUALS,
                                         mediaItemId,
                                         getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mDeleteAllQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult 
sbLocalDatabaseSimpleMediaList::NotifyCopyListener(sbIMediaItem *aSourceItem,
                                                   sbIMediaItem *aDestItem)
{
  NS_ENSURE_ARG_POINTER(aSourceItem);
  NS_ENSURE_ARG_POINTER(aDestItem);

  if(mCopyListener) {
    return mCopyListener->OnItemCopied(aSourceItem, aDestItem);
  }

  return NS_OK;
}


NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetDefaultSortProperty(nsAString& aProperty)
{
  aProperty.Assign(DEFAULT_SORT_PROPERTY);
  return NS_OK;
}

/**
 * See sbIMediaListListener
 */
NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::OnItemAdded(sbIMediaList* aMediaList,
                                            sbIMediaItem* aMediaItem)
{
  ASSERT_LIST_IS_LIBRARY(aMediaList);

  // We don't care if an item was added to the library

  return NS_OK;
}

/**
 * See sbIMediaListListener
 */
NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::OnBeforeItemRemoved(sbIMediaList* aMediaList,
                                              sbIMediaItem* aMediaItem)
{
  ASSERT_LIST_IS_LIBRARY(aMediaList);

  PRBool contains;
  nsresult rv = Contains(aMediaItem, &contains);
  NS_ENSURE_SUCCESS(rv, rv);

  // We don't care about this notification if the item isn't in our list.
  if (!contains) {
    return NS_OK;
  }

  // NOTE: We don't actually have to remove items from this list when they are
  // removed from the library since this is taken care of by a database trigger

  // Notify our listeners that the item is about to be removed
  NotifyListenersBeforeItemRemoved(this, aMediaItem);

  // Remember the guid of this item so we can do the after notification as well
  nsAutoString guid;
  rv = aMediaItem->GetGuid(guid);
    NS_ENSURE_SUCCESS(rv, rv);

  nsStringHashKey* success = mShouldNotifyAfterRemove.PutEntry(guid);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::OnAfterItemRemoved(sbIMediaList* aMediaList,
                                                   sbIMediaItem* aMediaItem)
{
  nsAutoString guid;
  nsresult rv = aMediaItem->GetGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mShouldNotifyAfterRemove.GetEntry(guid)) {

    mShouldNotifyAfterRemove.RemoveEntry(guid);
    // Invalidate the cached list
    // XXX: Should this be batch aware?
    nsresult rv = mFullArray->Invalidate();
    NS_ENSURE_SUCCESS(rv, rv);

    // Notify our listeners that the item was removed
    NotifyListenersAfterItemRemoved(this, aMediaItem);
  }

  return NS_OK;
}

/**
 * See sbIMediaListListener
 */
NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::OnItemUpdated(sbIMediaList* aMediaList,
                                              sbIMediaItem* aMediaItem)
{
  ASSERT_LIST_IS_LIBRARY(aMediaList);

  PRBool contains;
  nsresult rv = Contains(aMediaItem, &contains);
  NS_ENSURE_SUCCESS(rv, rv);

  // We don't care about this notification if the item isn't in our list.
  if (!contains) {
    return NS_OK;
  }

  // Pass the info on to our listeners (views, etc.).
  NotifyListenersItemUpdated(this, aMediaItem);

  return NS_OK;
}

/**
 * See sbIMediaListListener
 */
NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::OnListCleared(sbIMediaList* aMediaList)
{
  ASSERT_LIST_IS_LIBRARY(aMediaList);

  // NOTE: We don't actually have to remove items from this list when the
  // library is cleared since this is taken care of by a database trigger

  // Invalidate the cached list
  nsresult rv = mFullArray->Invalidate();
  NS_ENSURE_SUCCESS(rv, rv);

  // Notify our listeners that the list was cleared
  NotifyListenersListCleared(this);

  return NS_OK;
}

/**
 * See sbIMediaListListener
 */
NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::OnBatchBegin(sbIMediaList* aMediaList)
{
  ASSERT_LIST_IS_LIBRARY(aMediaList);

  // Just pass through to our listeners
  NotifyListenersBatchBegin(this);

  return NS_OK;
}

/**
 * See sbIMediaListListener
 */
NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::OnBatchEnd(sbIMediaList* aMediaList)
{
  ASSERT_LIST_IS_LIBRARY(aMediaList);

  // Just pass through to our listeners
  NotifyListenersBatchEnd(this);

  return NS_OK;
}

// nsIClassInfo
NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetInterfaces(PRUint32* count, nsIID*** array)
{
  return NS_CI_INTERFACE_GETTER_NAME(sbLocalDatabaseSimpleMediaList)(count, array);
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetHelperForLanguage(PRUint32 language,
                                                     nsISupports** _retval)
{
  *_retval = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetContractID(char** aContractID)
{
  *aContractID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetClassDescription(char** aClassDescription)
{
  *aClassDescription = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetClassID(nsCID** aClassID)
{
  *aClassID = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetImplementationLanguage(PRUint32* aImplementationLanguage)
{
  *aImplementationLanguage = nsIProgrammingLanguage::CPLUSPLUS;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetFlags(PRUint32 *aFlags)
{
  *aFlags = nsIClassInfo::THREADSAFE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseSimpleMediaList::GetClassIDNoAlloc(nsCID* aClassIDNoAlloc)
{
  return NS_ERROR_NOT_AVAILABLE;
}

