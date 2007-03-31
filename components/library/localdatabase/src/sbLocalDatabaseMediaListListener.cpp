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

#include "sbLocalDatabaseMediaListListener.h"

#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>

#include <nsAutoLock.h>
#include <nsHashKeys.h>
#include <sbProxyUtils.h>

sbLocalDatabaseMediaListListener::sbLocalDatabaseMediaListListener()
: mListenerProxyTableLock(nsnull)
{
}

sbLocalDatabaseMediaListListener::~sbLocalDatabaseMediaListListener()
{
  if (mListenerProxyTableLock) {
    nsAutoLock::DestroyLock(mListenerProxyTableLock);
  }
}

nsresult
sbLocalDatabaseMediaListListener::Init()
{
  NS_WARN_IF_FALSE(!mListenerProxyTableLock &&
                   !mListenerProxyTable.IsInitialized(),
                   "You're calling Init more than once!");

  // Initialize our lock.
  if (!mListenerProxyTableLock) {
    mListenerProxyTableLock =
      nsAutoLock::NewLock("sbLocalDatabaseMediaListListener::"
                          "mListenerProxyTableLock");
    NS_ENSURE_TRUE(mListenerProxyTableLock, NS_ERROR_OUT_OF_MEMORY);
  }

  // Initialize our hash table.
  if (!mListenerProxyTable.IsInitialized()) {
    PRBool success = mListenerProxyTable.Init();
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  }

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListListener::AddListener(sbIMediaListListener* aListener)
{
  NS_ASSERTION(mListenerProxyTableLock && mListenerProxyTable.IsInitialized(),
               "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aListener);

  nsAutoLock lock(mListenerProxyTableLock);

  // See if we have already added this listener.
  sbIMediaListListener* previousProxy;
  PRBool success = mListenerProxyTable.Get(aListener, &previousProxy);
  if (success && previousProxy) {
    // The listener has already been added, so do nothing more. But warn in
    // debug builds.
    NS_WARNING("Attempted to add a listener twice!");
    return NS_OK;
  }

  // Make a proxy for the listener that will always send callbacks to the
  // current thread.
  nsCOMPtr<sbIMediaListListener> proxy;
  nsresult rv = SB_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                     NS_GET_IID(sbIMediaListListener),
                                     aListener,
                                     NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                                     getter_AddRefs(proxy));
  NS_ENSURE_SUCCESS(rv, rv);

  // Add the proxy to the hash table, using the listener as the key.
  success = mListenerProxyTable.Put(aListener, proxy);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult
sbLocalDatabaseMediaListListener::RemoveListener(sbIMediaListListener* aListener)
{
  NS_ASSERTION(mListenerProxyTableLock && mListenerProxyTable.IsInitialized(),
               "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aListener);


#ifdef DEBUG
  nsAutoLock lock(mListenerProxyTableLock);

  // Check to make sure that this listener has actually been added. Only do
  // this in debug builds because the Remove method doesn't return any success
  // information and always succeeds..
  sbIMediaListListener* previousProxy;
  PRBool success = mListenerProxyTable.Get(aListener, &previousProxy);
  if (success && !previousProxy) {
    // The listener was never added so there's nothing else to do here. But
    // warn in debug builds.
    NS_WARNING("Attempted to remove a listener that was never added!");
    return NS_OK;
  }
#endif

  mListenerProxyTable.Remove(aListener);

  return NS_OK;
}

/**
 * \brief Notifies all listeners that an item has been added to the list.
 */
nsresult
sbLocalDatabaseMediaListListener::NotifyListenersItemAdded(sbIMediaList* aList,
                                                           sbIMediaItem* aItem)
{
  NS_ASSERTION(mListenerProxyTableLock && mListenerProxyTable.IsInitialized(),
               "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aList);
  NS_ENSURE_ARG_POINTER(aItem);

  MediaListCallbackInfo info(aList, aItem);
  mListenerProxyTable.EnumerateRead(ItemAddedCallback, &info);

  return NS_OK;
}

/**
 * \brief Notifies all listeners that an item has been removed from the list.
 */
nsresult
sbLocalDatabaseMediaListListener::NotifyListenersItemRemoved(sbIMediaList* aList,
                                                             sbIMediaItem* aItem)
{
  NS_ASSERTION(mListenerProxyTableLock && mListenerProxyTable.IsInitialized(),
               "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aList);
  NS_ENSURE_ARG_POINTER(aItem);

  MediaListCallbackInfo info(aList, aItem);
  mListenerProxyTable.EnumerateRead(ItemRemovedCallback, &info);

  return NS_OK;
}

/**
 * \brief Notifies all listeners that an item has been updated.
 */
nsresult
sbLocalDatabaseMediaListListener::NotifyListenersItemUpdated(sbIMediaList* aList,
                                                             sbIMediaItem* aItem)
{
  NS_ASSERTION(mListenerProxyTableLock && mListenerProxyTable.IsInitialized(),
               "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aList);
  NS_ENSURE_ARG_POINTER(aItem);

  if (mListenerProxyTable.IsInitialized()) {
    MediaListCallbackInfo info(aList, aItem);
    mListenerProxyTable.EnumerateRead(ItemRemovedCallback, &info);
  }

  return NS_OK;
}

/**
 * \brief Notifies all listeners that the list has been cleared.
 */
nsresult
sbLocalDatabaseMediaListListener::NotifyListenersListCleared(sbIMediaList* aList)
{
  NS_ASSERTION(mListenerProxyTableLock && mListenerProxyTable.IsInitialized(),
               "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aList);

  mListenerProxyTable.EnumerateRead(ListClearedCallback, aList);

  return NS_OK;
}

/**
 * \brief Notifies all listeners that multiple items are about to be changed.
 */
nsresult
sbLocalDatabaseMediaListListener::NotifyListenersBatchBegin(sbIMediaList* aList)
{
  NS_ASSERTION(mListenerProxyTableLock && mListenerProxyTable.IsInitialized(),
               "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aList);

  mListenerProxyTable.EnumerateRead(BatchBeginCallback, aList);

  return NS_OK;
}

/**
 * \brief Notifies all listeners that multiple items have been changed.
 */
nsresult
sbLocalDatabaseMediaListListener::NotifyListenersBatchEnd(sbIMediaList* aList)
{
  NS_ASSERTION(mListenerProxyTableLock && mListenerProxyTable.IsInitialized(),
               "You haven't called Init yet!");
  NS_ENSURE_ARG_POINTER(aList);

  mListenerProxyTable.EnumerateRead(BatchEndCallback, aList);

  return NS_OK;
}

/**
 * \brief Notifies all registered listeners that an item has been added to the
 *        media list.
 *
 * \param aKey      - An sbIMediaListListener.
 * \param aEntry    - An sbIMediaListListener proxy.
 * \param aUserData - A MediaListCallbackInfo pointer.
 *
 * \return PL_DHASH_NEXT
 */
PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListListener::ItemAddedCallback(nsISupportsHashKey::KeyType aKey,
                                                    sbIMediaListListener* aEntry,
                                                    void* aUserData)
{
  NS_ASSERTION(aKey && aEntry, "Nulls in the hash table!");

  MediaListCallbackInfo* info =
    NS_STATIC_CAST(MediaListCallbackInfo*, aUserData);
  NS_ENSURE_TRUE(info, PL_DHASH_NEXT);

  NS_ASSERTION(info->list && info->item, "Bad MediaListCallbackInfo!");

  nsCOMPtr<sbIMediaListListener> listener = aEntry;
  nsresult rv = listener->OnItemAdded(info->list, info->item);

  // We don't really care if some listener impl returns failure, but warn for
  // good measure.
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnItemAdded returned a failure code");

  return PL_DHASH_NEXT;
}

/**
 * \brief Notifies all registered listeners that an item has been removed from
 *        the media list.
 *
 * \param aKey      - An sbIMediaListListener.
 * \param aEntry    - An sbIMediaListListener proxy.
 * \param aUserData - A MediaListCallbackInfo pointer.
 *
 * \return PL_DHASH_NEXT
 */
PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListListener::ItemRemovedCallback(nsISupportsHashKey::KeyType aKey,
                                                      sbIMediaListListener* aEntry,
                                                      void* aUserData)
{
  NS_ASSERTION(aKey && aEntry, "Nulls in the hash table!");

  MediaListCallbackInfo* info =
    NS_STATIC_CAST(MediaListCallbackInfo*, aUserData);
  NS_ENSURE_TRUE(info, PL_DHASH_NEXT);

  NS_ASSERTION(info->list && info->item, "Bad MediaListCallbackInfo!");

  nsCOMPtr<sbIMediaListListener> listener = aEntry;
  nsresult rv = listener->OnItemRemoved(info->list, info->item);

  // We don't really care if some listener impl returns failure, but warn for
  // good measure.
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnItemRemoved returned a failure code");

  return PL_DHASH_NEXT;
}

/**
 * \brief Notifies all registered listeners that an item has been updated.
 *
 * \param aKey      - An sbIMediaListListener.
 * \param aEntry    - An sbIMediaListListener proxy.
 * \param aUserData - A MediaListCallbackInfo pointer.
 *
 * \return PL_DHASH_NEXT
 */
PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListListener::ItemUpdatedCallback(nsISupportsHashKey::KeyType aKey,
                                                      sbIMediaListListener* aEntry,
                                                      void* aUserData)
{
  NS_ASSERTION(aKey && aEntry, "Nulls in the hash table!");

  MediaListCallbackInfo* info =
    NS_STATIC_CAST(MediaListCallbackInfo*, aUserData);
  NS_ENSURE_TRUE(info, PL_DHASH_NEXT);

  NS_ASSERTION(info->list && info->item, "Bad MediaListCallbackInfo!");

  nsCOMPtr<sbIMediaListListener> listener = aEntry;
  nsresult rv = listener->OnItemUpdated(info->list, info->item);

  // We don't really care if some listener impl returns failure, but warn for
  // good measure.
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnItemRemoved returned a failure code");

  return PL_DHASH_NEXT;
}

/**
 * \brief Notifies all registered listeners that an item has been removed from
 *        the media list.
 *
 * \param aKey      - An sbIMediaListListener.
 * \param aEntry    - An sbIMediaListListener proxy.
 * \param aUserData - An sbIMediaListPointer.
 *
 * \return PL_DHASH_NEXT
 */
PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListListener::ListClearedCallback(nsISupportsHashKey::KeyType aKey,
                                                      sbIMediaListListener* aEntry,
                                                      void* aUserData)
{
  NS_ASSERTION(aKey && aEntry, "Nulls in the hash table!");

  nsCOMPtr<sbIMediaList> list = NS_STATIC_CAST(sbIMediaList*, aUserData);
  NS_ENSURE_TRUE(list, PL_DHASH_NEXT);

  nsCOMPtr<sbIMediaListListener> listener = aEntry;
  nsresult rv = listener->OnListCleared(list);

  // We don't really care if some listener impl returns failure, but warn for
  // good measure.
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnListCleared returned a failure code");

  return PL_DHASH_NEXT;
}

/**
 * \brief Notifies all registered listeners that multiple items are about to be
 *        changed at once.
 *
 * \param aKey      - An sbIMediaListListener.
 * \param aEntry    - An sbIMediaListListener proxy.
 * \param aUserData - An sbIMediaListPointer.
 *
 * \return PL_DHASH_NEXT
 */
PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListListener::BatchBeginCallback(nsISupportsHashKey::KeyType aKey,
                                                     sbIMediaListListener* aEntry,
                                                     void* aUserData)
{
  NS_ASSERTION(aKey && aEntry, "Nulls in the hash table!");

  nsCOMPtr<sbIMediaList> list = NS_STATIC_CAST(sbIMediaList*, aUserData);
  NS_ENSURE_TRUE(list, PL_DHASH_NEXT);

  nsCOMPtr<sbIMediaListListener> listener = aEntry;
  nsresult rv = listener->OnBatchBegin(list);

  // We don't really care if some listener impl returns failure, but warn for
  // good measure.
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnBatchBegin returned a failure code");

  return PL_DHASH_NEXT;
}

/**
 * \brief Notifies all registered listeners that multiple items have been
 *        changed.
 *
 * \param aKey      - An sbIMediaListListener.
 * \param aEntry    - An sbIMediaListListener proxy.
 * \param aUserData - An sbIMediaListPointer.
 *
 * \return PL_DHASH_NEXT
 */
PLDHashOperator PR_CALLBACK
sbLocalDatabaseMediaListListener::BatchEndCallback(nsISupportsHashKey::KeyType aKey,
                                                   sbIMediaListListener* aEntry,
                                                   void* aUserData)
{
  NS_ASSERTION(aKey && aEntry, "Nulls in the hash table!");

  nsCOMPtr<sbIMediaList> list = NS_STATIC_CAST(sbIMediaList*, aUserData);
  NS_ENSURE_TRUE(list, PL_DHASH_NEXT);

  nsCOMPtr<sbIMediaListListener> listener = aEntry;
  nsresult rv = listener->OnBatchBegin(list);

  // We don't really care if some listener impl returns failure, but warn for
  // good measure.
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "OnBatchEnd returned a failure code");

  return PL_DHASH_NEXT;
}
