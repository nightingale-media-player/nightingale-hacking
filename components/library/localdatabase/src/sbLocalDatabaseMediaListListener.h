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

#ifndef __SB_LOCALDATABASE_MEDIALISTLISTENER_H__
#define __SB_LOCALDATABASE_MEDIALISTLISTENER_H__

#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <prlock.h>

class sbIMediaItem;
class sbIMediaList;
class sbIMediaListListener;

class sbLocalDatabaseMediaListListener
{
  typedef nsInterfaceHashtableMT<nsISupportsHashKey, sbIMediaListListener>
          sbMediaListListenersTableMT;

  struct MediaListCallbackInfo {

    MediaListCallbackInfo(sbIMediaList* aList, sbIMediaItem* aItem)
    : list(aList),
      item(aItem) { }

    nsCOMPtr<sbIMediaList> list;
    nsCOMPtr<sbIMediaItem> item;
  };

public:
  // Default constructor. You should really use the next constructor to be
  // safe.
  sbLocalDatabaseMediaListListener();

  // Constructor that give a status result. Don't use if this returns failure.
  sbLocalDatabaseMediaListListener(nsresult* aInitStatus);

  ~sbLocalDatabaseMediaListListener();

protected:
  // Add a listener to the hash table.
  nsresult AddListener(sbIMediaListListener* aListener);

  // Remove a listener from the hash table.
  nsresult RemoveListener(sbIMediaListListener* aListener);

  // Enumerate listeners and call OnItemAdded
  nsresult NotifyListenersItemAdded(sbIMediaList* aList,
                                    sbIMediaItem* aItem);

  // Enumerate listeners and call OnItemRemoved
  nsresult NotifyListenersItemRemoved(sbIMediaList* aList,
                                      sbIMediaItem* aItem);

  // Enumerate listeners and call OnItemRemoved
  nsresult NotifyListenersItemUpdated(sbIMediaList* aList,
                                      sbIMediaItem* aItem);

  // Enumerate listeners and call OnListCleared
  nsresult NotifyListenersListCleared(sbIMediaList* aList);

  // Enumerate listeners and call OnBatchBegin
  nsresult NotifyListenersBatchBegin(sbIMediaList* aList);

  // Enumerate listeners and call OnBatchEnd
  nsresult NotifyListenersBatchEnd(sbIMediaList* aList);

private:
  // Initialize the instance.
  nsresult Init();

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be a MediaListCallbackInfo pointer.
  static PLDHashOperator PR_CALLBACK
    ItemAddedCallback(nsISupportsHashKey::KeyType aKey,
                      sbIMediaListListener* aEntry,
                      void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be a MediaListCallbackInfo pointer.
  static PLDHashOperator PR_CALLBACK
    ItemRemovedCallback(nsISupportsHashKey::KeyType aKey,
                        sbIMediaListListener* aEntry,
                        void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be a MediaListCallbackInfo pointer.
  static PLDHashOperator PR_CALLBACK
    ItemUpdatedCallback(nsISupportsHashKey::KeyType aKey,
                        sbIMediaListListener* aEntry,
                        void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be an sbIMediaList pointer.
  static PLDHashOperator PR_CALLBACK
    ListClearedCallback(nsISupportsHashKey::KeyType aKey,
                        sbIMediaListListener* aEntry,
                        void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be an sbIMediaList pointer.
  static PLDHashOperator PR_CALLBACK
    BatchBeginCallback(nsISupportsHashKey::KeyType aKey,
                       sbIMediaListListener* aEntry,
                       void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be an sbIMediaList pointer.
  static PLDHashOperator PR_CALLBACK
    BatchEndCallback(nsISupportsHashKey::KeyType aKey,
                     sbIMediaListListener* aEntry,
                     void* aUserData);

private:
  // A thread-safe hash table that holds a mapping of listeners to proxies.
  sbMediaListListenersTableMT mListenerProxyTable;

  // This lock protects the code that checks for existing entries in the proxy
  // table.
  PRLock* mListenerProxyTableLock;
};

#endif /* __SB_LOCALDATABASE_MEDIALISTLISTENER_H__ */
