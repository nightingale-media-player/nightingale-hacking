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

#ifndef __SBLOCALDATABASESIMPLEMEDIALIST_H__
#define __SBLOCALDATABASESIMPLEMEDIALIST_H__

#include "sbLocalDatabaseMediaListBase.h"
#include <sbILocalDatabaseSimpleMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIOrderableMediaList.h>
#include <nsIClassInfo.h>

#include <nsStringGlue.h>
#include <nsCOMArray.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <nsTHashtable.h>
#include <prlock.h>

class nsIMutableArray;
class nsISimpleEnumerator;
class sbIDatabaseQuery;
class sbILocalDatabaseLibrary;
class sbIMediaItem;
class sbIMediaList;
class sbIMediaListView;
class sbSimpleMediaListInsertingEnumerationListener;
class sbSimpleMediaListRemovingEnumerationListener;

class sbLocalDatabaseSimpleMediaList : public sbLocalDatabaseMediaListBase,
                                       public sbILocalDatabaseSimpleMediaList,
                                       public sbIOrderableMediaList
{
public:
  friend class sbSimpleMediaListInsertingEnumerationListener;
  friend class sbSimpleMediaListRemovingEnumerationListener;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICLASSINFO
  NS_DECL_SBILOCALDATABASESIMPLEMEDIALIST
  NS_DECL_SBIORDERABLEMEDIALIST

  sbLocalDatabaseSimpleMediaList();
  virtual ~sbLocalDatabaseSimpleMediaList();

  nsresult Init(sbLocalDatabaseLibrary* aLibrary,
                const nsAString& aGuid);

  // override base class
  NS_IMETHOD GetType(nsAString& aType);
  NS_IMETHOD GetItemByGuid(const nsAString& aGuid, sbIMediaItem** _retval);
  NS_IMETHOD Contains(sbIMediaItem* aMediaItem, bool* _retval);
  NS_IMETHOD Add(sbIMediaItem *aMediaItem);
  NS_IMETHOD AddItem(sbIMediaItem * aMediaItem, sbIMediaItem ** aNewMediaItem);
  NS_IMETHOD AddAll(sbIMediaList *aMediaList);
  NS_IMETHOD AddSome(nsISimpleEnumerator *aMediaItems);
  NS_IMETHOD AddMediaItems(nsISimpleEnumerator *aMediaItems, sbIAddMediaItemsListener *aListener, bool aAsync);
  NS_IMETHOD Remove(sbIMediaItem* aMediaItem);
  NS_IMETHOD RemoveByIndex(PRUint32 aIndex);
  NS_IMETHOD RemoveSome(nsISimpleEnumerator* aMediaItems);
  NS_IMETHOD Clear();
  NS_IMETHOD CreateView(sbIMediaListViewState* aState,
                        sbIMediaListView** _retval);

  NS_IMETHOD GetDefaultSortProperty(nsAString& aProperty);

  nsresult AddSomeAsyncInternal(nsISimpleEnumerator *aMediaItems,
                                nsISupports *aListener,
                                PRUint32 aStartingIndex,
                                nsAString& aStartingOrdinal);

private:
  nsresult UpdateLastModifiedTime();

  nsresult ExecuteAggregateQuery(const nsAString& aQuery, nsAString& aValue);

  nsresult UpdateOrdinalByIndex(PRUint32 aIndex, const nsAString& aOrdinal);

  nsresult MoveSomeInternal(PRUint32* aFromIndexArray,
                            PRUint32 aFromIndexArrayCount,
                            PRUint32 aToIndex,
                            const nsAString& aOrdinalRoot);

  nsresult GetNextOrdinal(nsAString& aValue);

  nsresult GetBeforeOrdinal(PRUint32 aIndex, nsAString& aValue);

  nsresult AddToLastPathSegment(nsAString& aPath, PRInt32 aToAdd);

  PRUint32 CountLevels(const nsAString& aPath);

  nsresult CreateQueries();

  nsresult NotifyCopyListener(sbIMediaItem *aSourceItem,
                              sbIMediaItem *aDestItem);

private:
  // Query to get the media item id for a given guid, constrained to the
  // items within this simple media list
  nsString mGetMediaItemIdForGuidQuery;

  // Query to insert a new item into this list
  nsString mInsertIntoListQuery;

  // Query to update the ordinal of a media item in this list
  nsString mUpdateListItemOrdinalQuery;

  // Query to delete an item from the list
  nsString mDeleteFirstListItemQuery;

  // Query to delete the entire contents of the list
  nsString mDeleteAllQuery;

  // Query to delete an item with a given ordinal
  nsString mDeleteListItemByOrdinalQuery;

  // Get last ordinal
  nsString mGetLastOrdinalQuery;

  // Get first ordinal
  nsString mGetFirstOrdinalQuery;

  // Copy Listener
  nsCOMPtr<sbILocalDatabaseMediaListCopyListener> mCopyListener;

  // Keep a list of guids that should be notified after removal
  nsTHashtable<nsStringHashKey> mShouldNotifyAfterRemove;
};

/**
 * This enumerator is used to add items to a media list. For each item it
 * determines if the item exists in the media list's library, a copy already
 * exists, or a new item must be created. After the enumeration is complete
 * it then creates any items that need to be created and updates the database
 * so the media list now contains the list of items enumerated.
 */
class sbSimpleMediaListInsertingEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbSimpleMediaListInsertingEnumerationListener(sbLocalDatabaseSimpleMediaList* aList,
                                                PRUint32 aStartingIndex,
                                                const nsAString& aStartingOrdinal,
                                                nsISupports * aListener = nsnull)
  : mFriendList(aList),
    mStartingIndex(aStartingIndex),
    mStartingOrdinal(aStartingOrdinal)
  {
    if (aListener) {
      mAsyncListener = do_QueryInterface(aListener);
      mAddListener = do_QueryInterface(aListener);
    }
  }

private:
  // Not meant to be implemented. This makes it a compiler error to
  // attempt to create an object on the heap.
  static void* operator new(size_t /*size*/) CPP_THROW_NEW { return 0; }
  static void operator delete(void* /*memory*/) { }

  sbLocalDatabaseSimpleMediaList* mFriendList;
  PRUint32 mStartingIndex;
  nsString mStartingOrdinal;
  /**
   * This is list of media items we'll be adding to media list in the order
   * desired. There may be duplicates, some of these items may exist in a
   * different database than the media list they're being added to.
   */
  nsCOMArray<sbIMediaItem> mItemList;
  /**
   * This holds items that do not exist in the library that the media list
   * belongs to. Some of these items may have corresponding items in the
   * media list's library and so may not need to be created. For those items
   * the hash entry will be the matching media item.
   */
  nsInterfaceHashtable<nsISupportsHashKey, sbIMediaItem> mItemsToCreateOrAdd;
  nsCOMPtr<sbILibrary> mListLibrary;
  nsCOMPtr<sbIAddMediaItemsListener> mAddListener;
  nsCOMPtr<sbIMediaListAsyncListener> mAsyncListener;

  /**
   * This function takes a list of existing items and updates the foreign
   * item list with the newly created item
   */
  nsresult UpdateItemsInForeignLib(nsIArray * aExistingItems,
                                   nsIArray * aNewItems);
};

class sbSimpleMediaListRemovingEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbSimpleMediaListRemovingEnumerationListener(sbLocalDatabaseSimpleMediaList* aList)
  : mFriendList(aList),
    mItemEnumerated(PR_FALSE)
  {
    NS_ASSERTION(mFriendList, "Null pointer!");
  }

private:
  // Not meant to be implemented. This makes it a compiler error to
  // attempt to create an object on the heap.
  static void* operator new(size_t /*size*/) CPP_THROW_NEW { return 0; }
  static void operator delete(void* /*memory*/) { }

  sbLocalDatabaseSimpleMediaList* mFriendList;
  nsCOMPtr<sbIDatabaseQuery> mDBQuery;
  nsCOMArray<sbIMediaItem> mNotificationList;
  nsTArray<PRUint32> mNotificationIndexes;
  bool mItemEnumerated;
};

#endif /* __SBLOCALDATABASESIMPLEMEDIALIST_H__ */
