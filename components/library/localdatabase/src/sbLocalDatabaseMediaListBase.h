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

#include <sbLocalDatabaseMediaItem.h>
#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <prmon.h>
#include <prlock.h>
#include <nsIStringEnumerator.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIFilterableMediaList.h>
#include <sbISearchableMediaList.h>
#include <sbISortableMediaList.h>
#include <sbIDatabaseResult.h>
#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseResourceProperty.h"

// Macros to help early returns within loops.
#define SB_CONTINUE_IF_FALSE(_expr)                                            \
  PR_BEGIN_MACRO                                                               \
    if (!(_expr)) {                                                            \
      NS_WARNING("SB_CONTINUE_IF_FALSE triggered");                            \
      continue;                                                                \
    }                                                                          \
  PR_END_MACRO

#define SB_CONTINUE_IF_FAILED(_rv)                                             \
  SB_CONTINUE_IF_FALSE(NS_SUCCEEDED(_rv))

// This macro is for derived classes that want to ensure safe access to
// mFullArray.
#define SB_MEDIALIST_LOCK_FULLARRAY_AND_ENSURE_MUTABLE()                       \
  PR_BEGIN_MACRO                                                               \
    nsAutoMonitor mon(mFullArrayMonitor);                                      \
    if (mLockedEnumerationActive) {                                            \
      NS_ERROR("Operation not permitted during a locked enumeration");         \
      return NS_ERROR_FAILURE;                                                 \
    }                                                                          \
  PR_END_MACRO

class sbLocalDatabaseMediaListBase : public sbLocalDatabaseResourceProperty,
                                     public sbIMediaList,
                                     public sbILocalDatabaseMediaItem,
                                     public sbIFilterableMediaList,
                                     public sbISearchableMediaList,
                                     public sbISortableMediaList,
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
  NS_DECL_SBIFILTERABLEMEDIALIST
  NS_DECL_SBISEARCHABLEMEDIALIST
  NS_DECL_SBISORTABLEMEDIALIST
  NS_DECL_NSICLASSINFO

  sbLocalDatabaseMediaListBase(sbILocalDatabaseLibrary* aLibrary,
                               const nsAString& aGuid);
  ~sbLocalDatabaseMediaListBase();

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

  NS_IMETHOD GetDefaultSortProperty(nsAString& aProperty) = 0;
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

  nsresult EnumerateAllItemsInternal(sbIMediaListEnumerationListener* aEnumerationListener);

  nsresult EnumerateItemsByPropertyInternal(const nsAString& aName,
                                            nsIStringEnumerator* aValueEnum,
                                            sbIMediaListEnumerationListener* aEnumerationListener);

  nsresult EnumerateItemsByPropertiesInternal(sbStringArrayHash* aPropertiesHash,
                                              sbIMediaListEnumerationListener* aEnumerationListener);

  // Called for the enumeration methods.
  nsresult EnumerateItemsInternal(sbGUIDArrayEnumerator* aEnumerator,
                                  sbIMediaListEnumerationListener* aListener);

  nsresult UpdateFiltersInternal(sbIPropertyArray* aPropertyArray,
                                 PRBool aReplace);

  nsresult UpdateViewArrayConfiguration();


protected:

  // The library this media list instance belogs to
  nsCOMPtr<sbILocalDatabaseLibrary> mLibrary;

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

  // A monitor for changes to the media list.
  PRMonitor* mFullArrayMonitor;

  // A lock for changes in the view filter
  PRLock* mViewArrayLock;

  PRBool mLockedEnumerationActive;

  // Query to return list of values for a given property
  nsString mDistinctPropertyValuesQuery;

private:
  // A thread-safe hash table that holds a mapping of listeners to proxies.
  sbMediaListListenersTableMT mListenerProxyTable;

  // This lock protects the code that checks for existing entries in the proxy
  // table.
  PRLock* mListenerProxyTableLock;

  // Map of current view filter configuration
  sbStringArrayHash mViewFilters;

  // Current search filter configuration
  nsCOMPtr<sbIPropertyArray> mViewSearches;

  // Current sort filter configuration
  nsCOMPtr<sbIPropertyArray> mViewSort;

};

class sbDatabaseResultStringEnumerator : public nsIStringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  sbDatabaseResultStringEnumerator(sbIDatabaseResult* aDatabaseResult) :
    mDatabaseResult(aDatabaseResult),
    mNextIndex(0),
    mLength(0)
  {
  }

  nsresult Init();
private:
  nsCOMPtr<sbIDatabaseResult> mDatabaseResult;
  PRUint32 mNextIndex;
  PRUint32 mLength;
};

#endif /* __SBLOCALDATABASEMEDIALISTBASE_H__ */
