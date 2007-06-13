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
#include <nsCOMArray.h>
#include <nsInterfaceHashtable.h>
#include <prlock.h>
#include <sbIPropertyArray.h>

class nsISupportsHashKey;
class sbIMediaItem;
class sbIMediaList;
class sbIMediaListListener;

class sbLocalDatabaseMediaListListener
{
  friend class sbAutoBatchHelper;

  typedef nsInterfaceHashtableMT<nsISupportsHashKey, sbIMediaListListener>
          sbMediaListListenersTableMT;

  typedef nsCOMArray<sbIMediaListListener> sbMediaListListenersArray;

  struct MediaListCallbackInfo {

    MediaListCallbackInfo(sbIMediaList* aList, sbIMediaItem* aItem)
    : list(aList),
      item(aItem) { }

    nsCOMPtr<sbIMediaList> list;
    nsCOMPtr<sbIMediaItem> item;
  };

  struct ItemUpdatedCallbackInfo {

    ItemUpdatedCallbackInfo(sbIMediaList* aList,
                            sbIMediaItem* aItem,
                            sbIPropertyArray* aProperties)
    : list(aList),
      item(aItem),
      properties(aProperties) { }

    nsCOMPtr<sbIMediaList> list;
    nsCOMPtr<sbIMediaItem> item;
    nsCOMPtr<sbIPropertyArray> properties;
  };

public:
  sbLocalDatabaseMediaListListener();
  ~sbLocalDatabaseMediaListListener();

protected:
  // Initialize the instance.
  nsresult Init();

  // Add a listener to the hash table.
  nsresult AddListener(sbIMediaListListener* aListener);

  // Remove a listener from the hash table.
  nsresult RemoveListener(sbIMediaListListener* aListener);

  // Return the number of listeners
  PRUint32 ListenerCount();

  // Enumerate listeners and call OnItemAdded
  void NotifyListenersItemAdded(sbIMediaList* aList,
                                sbIMediaItem* aItem);

  // Enumerate listeners and call OnBeforeItemRemoved
  void NotifyListenersBeforeItemRemoved(sbIMediaList* aList,
                                        sbIMediaItem* aItem);

  // Enumerate listeners and call OnAfterItemRemoved
  void NotifyListenersAfterItemRemoved(sbIMediaList* aList,
                                       sbIMediaItem* aItem);

  // Enumerate listeners and call OnItemRemoved
  void NotifyListenersItemUpdated(sbIMediaList* aList,
                                  sbIMediaItem* aItem,
                                  sbIPropertyArray* aProperties);

  // Enumerate listeners and call OnListCleared
  void NotifyListenersListCleared(sbIMediaList* aList);

  // Enumerate listeners and call OnBatchBegin
  void NotifyListenersBatchBegin(sbIMediaList* aList);

  // Enumerate listeners and call OnBatchEnd
  void NotifyListenersBatchEnd(sbIMediaList* aList);

private:
  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be a MediaListCallbackInfo pointer.
  static PRBool PR_CALLBACK
    ItemAddedCallback(sbIMediaListListener* aEntry,
                      void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be a MediaListCallbackInfo pointer.
  static PRBool PR_CALLBACK
    BeforeItemRemovedCallback(sbIMediaListListener* aEntry,
                              void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be a MediaListCallbackInfo pointer.
  static PRBool PR_CALLBACK
    AfterItemRemovedCallback(sbIMediaListListener* aEntry,
                             void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be a MediaListCallbackInfo pointer.
  static PRBool PR_CALLBACK
    ItemUpdatedCallback(sbIMediaListListener* aEntry,
                        void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be an sbIMediaList pointer.
  static PRBool PR_CALLBACK
    ListClearedCallback(sbIMediaListListener* aEntry,
                        void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be an sbIMediaList pointer.
  static PRBool PR_CALLBACK
    BatchBeginCallback(sbIMediaListListener* aEntry,
                       void* aUserData);

  // This callback is meant to be used with mListenerProxyTable.
  // aUserData should be an sbIMediaList pointer.
  static PRBool PR_CALLBACK
    BatchEndCallback(sbIMediaListListener* aEntry,
                     void* aUserData);

  static PLDHashOperator PR_CALLBACK
    ToArrayCallback(nsISupportsHashKey::KeyType aKey,
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
