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

#ifndef __SBLOCALDATABASEMEDIALISTBASE_H__
#define __SBLOCALDATABASEMEDIALISTBASE_H__

#include <sbILocalDatabaseLibrary.h>
#include <sbLocalDatabaseLibrary.h>
#include <sbLocalDatabaseMediaItem.h>
#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>

#include "sbLocalDatabaseResourceProperty.h"

class sbLocalDatabaseMediaListBase : public sbLocalDatabaseResourceProperty,
                                     public sbIMediaList,
                                     public sbILocalDatabaseMediaItem,
                                     public nsIClassInfo
{
  typedef nsTArray<nsString> sbStringArray;
  typedef nsClassHashtable<nsStringHashKey, sbStringArray> sbStringArrayHash;
  typedef nsInterfaceHashtableMT<nsISupportsHashKey, sbIMediaListListener> sbMediaListListenersTableMT;

public:
  NS_DECL_ISUPPORTS_INHERITED

  //When using inheritence, you must forward all interfaces implemented
  //by the base class, else you will get "pure virtual function was not defined"
  //style errors.
  NS_FORWARD_SBILOCALDATABASERESOURCEPROPERTY(sbLocalDatabaseResourceProperty::)
  NS_FORWARD_SBILIBRARYRESOURCE(sbLocalDatabaseResourceProperty::)

  NS_DECL_SBIMEDIAITEM
  NS_DECL_SBIMEDIALIST
  NS_DECL_SBILOCALDATABASEMEDIAITEM
  NS_DECL_NSICLASSINFO

  sbLocalDatabaseMediaListBase(sbILocalDatabaseLibrary* aLibrary,
                               const nsAString& aGuid);

  NS_IMETHODIMP Init();

protected:
  // Enumerate listeners and call OnItemAdded
  nsresult NotifyListenersItemAdded(sbIMediaItem* aItem);

  // Enumerate listeners and call OnItemRemoved
  nsresult NotifyListenersItemRemoved(sbIMediaItem* aItem);

  // Enumerate listeners and call OnListCleared
  nsresult NotifyListenersListCleared();

  // Enumerate listeners and call OnBatchBegin
  nsresult NotifyListenersBatchBegin();

  // Enumerate listeners and call OnBatchEnd
  nsresult NotifyListenersBatchEnd();

private:

  struct MediaListCallbackInfo {

    MediaListCallbackInfo(sbIMediaList* aList, sbIMediaItem* aItem)
    : list(aList),
      item(aItem) { }

    nsCOMPtr<sbIMediaList> list;
    nsCOMPtr<sbIMediaItem> item;
  };

  // This callback is meant to be used with an sbStringArrayHash.
  // aUserData should be a sbILocalDatabaseGUIDArray pointer.
  static PLDHashOperator PR_CALLBACK
    AddFilterToGUIDArrayCallback(nsStringHashKey::KeyType aKey,
                                 sbStringArray* aEntry,
                                 void* aUserData);

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

  // Called to initialize the proxy table. Returns PR_TRUE on success, PR_FALSE
  // otherwise.
  inline PRBool InitializeListenerProxyTable();

protected:

  // The library this media list instance belogs to
  nsCOMPtr<sbILocalDatabaseLibrary> mLibrary;

  // The guid of this media list
  nsString mGuid;

  // The media item id of the media list
  PRUint32 mMediaItemId;

  // The mViewArray is the guid array that represents the contents of the
  // media list when viewed in the UI.  This is the array that gets updated
  // from calls through the sbI*ableMediaList interfaces.  The contents of this
  // array does not affect the API level list manupulation methods
  nsCOMPtr<sbILocalDatabaseGUIDArray> mViewArray;

  // The mFullArray is a cached version of the full contents of the media
  // list this instance represents.
  nsCOMPtr<sbILocalDatabaseGUIDArray> mFullArray;

private:
  // A thread-safe hash table that holds a mapping of listeners to proxies.
  sbMediaListListenersTableMT mListenerProxyTable;
};

#endif /* __SBLOCALDATABASEMEDIALISTBASE_H__ */
