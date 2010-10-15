/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#ifndef __SBLOCALDATABASELIBRARY_H__
#define __SBLOCALDATABASELIBRARY_H__

#include <nsIObserver.h>
#include <sbIDatabaseQuery.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbILocalDatabaseSimpleMediaList.h>
#include "sbLocalDatabaseMediaListBase.h"
#include <sbProxiedComponentManager.h>

#include <prmon.h>
#include <nsAutoLock.h>
#include <nsClassHashtable.h>
#include <nsDataHashtable.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIClassInfo.h>
#include <nsInterfaceHashtable.h>
#include <nsIRunnable.h>
#include <nsIStreamListener.h>
#include <nsITimer.h>
#include <nsIThread.h>
#include <nsIThreadPool.h>
#include <nsIURI.h>
#include <nsStringGlue.h>
#include <nsVoidArray.h>
#include <sbIMediaListFactory.h>
#include <sbILibraryStatistics.h>

class nsIPropertyBag2;
class nsIWeakReference;
class nsStringHashKey;
class sbAutoBatchHelper;
class sbIBatchCreateMediaItemsListener;
class sbILibraryFactory;
class sbILocalDatabasePropertyCache;
class sbLibraryInsertingEnumerationListener;
class sbLibraryRemovingEnumerationListener;
class sbLocalDatabaseMediaListView;
class sbLocalDatabasePropertyCache;
class nsIPrefBranch;
class nsIURI;

typedef nsCOMArray<sbIMediaItem> sbMediaItemArray;
typedef nsCOMArray<sbIMediaList> sbMediaListArray;
typedef nsClassHashtable<nsISupportsHashKey, sbMediaItemArray>
        sbMediaItemToListsMap;
typedef nsDataHashtable<nsStringHashKey, PRUint32> sbListItemIndexMap;
typedef nsInterfaceHashtableMT<nsStringHashKey, nsIWeakReference>
        sbGUIDToListMap;

// These are the methods from sbLocalDatabaseMediaListBase that we're going to
// override in sbLocalDatabaseLibrary. Most of them are from sbIMediaList.
#define SB_DECL_MEDIALISTBASE_OVERRIDES                                             \
  NS_IMETHOD GetType(nsAString& aType);                                             \
  NS_IMETHOD GetItemByGuid(const nsAString& aGuid, sbIMediaItem** _retval);         \
  NS_IMETHOD Contains(sbIMediaItem* aMediaItem, PRBool* _retval);                   \
  NS_IMETHOD Add(sbIMediaItem* aMediaItem);                                         \
  NS_IMETHOD AddItem(sbIMediaItem* aMediaItem, sbIMediaItem ** aNewMediaItem);      \
  NS_IMETHOD AddAll(sbIMediaList* aMediaList);                                      \
  NS_IMETHOD AddSome(nsISimpleEnumerator* aMediaItems);                             \
  NS_IMETHOD AddSomeAsync(nsISimpleEnumerator* aMediaItems, sbIMediaListAsyncListener* aListener);\
  NS_IMETHOD Remove(sbIMediaItem* aMediaItem);                                      \
  NS_IMETHOD RemoveByIndex(PRUint32 aIndex);                                        \
  NS_IMETHOD RemoveSome(nsISimpleEnumerator* aMediaItems);                          \
  NS_IMETHOD Clear();                                                               \
  NS_IMETHOD CreateView(sbIMediaListViewState* aState, sbIMediaListView** _retval); \
  /* nothing */

#define SB_DECL_SBIMEDIAITEM_OVERRIDES                                          \
  NS_IMETHOD SetContentSrc(nsIURI* aContentSrc);                                \
  /* nothing */

#define SB_FORWARD_SBIMEDIAITEM(_to) \
  NS_IMETHOD GetLibrary(sbILibrary * *aLibrary) { return _to GetLibrary(aLibrary); } \
  NS_IMETHOD GetIsMutable(PRBool *aIsMutable) { return _to GetIsMutable(aIsMutable); } \
  NS_IMETHOD GetItemController(sbIMediaItemController **aMediaItemController) { return _to GetItemController(aMediaItemController); } \
  NS_IMETHOD GetMediaCreated(PRInt64 *aMediaCreated) { return _to GetMediaCreated(aMediaCreated); } \
  NS_IMETHOD SetMediaCreated(PRInt64 aMediaCreated) { return _to SetMediaCreated(aMediaCreated); } \
  NS_IMETHOD GetMediaUpdated(PRInt64 *aMediaUpdated) { return _to GetMediaUpdated(aMediaUpdated); } \
  NS_IMETHOD SetMediaUpdated(PRInt64 aMediaUpdated) { return _to SetMediaUpdated(aMediaUpdated); } \
  NS_IMETHOD GetContentSrc(nsIURI **aContentSrc) { return _to GetContentSrc(aContentSrc); } \
  NS_IMETHOD GetContentLength(PRInt64 *aContentLength) { return _to GetContentLength(aContentLength); } \
  NS_IMETHOD SetContentLength(PRInt64 aContentLength) { return _to SetContentLength(aContentLength); } \
  NS_IMETHOD GetContentType(nsAString & aContentType) { return _to GetContentType(aContentType); } \
  NS_IMETHOD SetContentType(const nsAString & aContentType) { return _to SetContentType(aContentType); } \
  NS_IMETHOD TestIsURIAvailable(nsIObserver *aObserver) { return _to TestIsURIAvailable(aObserver); } \
  NS_IMETHOD OpenInputStreamAsync(nsIStreamListener *aListener, nsISupports *aContext, nsIChannel **_retval) { return _to OpenInputStreamAsync(aListener, aContext, _retval); } \
  NS_IMETHOD OpenInputStream(nsIInputStream **_retval) { return _to OpenInputStream(_retval); } \
  NS_IMETHOD OpenOutputStream(nsIOutputStream **_retval) { return _to OpenOutputStream(_retval); } \
  NS_IMETHOD ToString(nsAString & _retval) { return _to ToString(_retval); }

#define SB_FORWARD_SBIMEDIALIST(_to) \
  NS_IMETHOD GetName(nsAString & aName) { return _to GetName(aName); } \
  NS_IMETHOD SetName(const nsAString & aName) { return _to SetName(aName); } \
  NS_IMETHOD GetLength(PRUint32 *aLength) { return _to GetLength(aLength); } \
  NS_IMETHOD GetIsEmpty(PRBool *aIsEmpty) { return _to GetIsEmpty(aIsEmpty); } \
  NS_IMETHOD GetUserEditableContent(PRBool *aUserEditableContent) { return _to GetUserEditableContent(aUserEditableContent); } \
  NS_IMETHOD GetItemByIndex(PRUint32 aIndex, sbIMediaItem **_retval) { return _to GetItemByIndex(aIndex, _retval); } \
  NS_IMETHOD GetListContentType(PRUint16 *_retval) { return _to GetListContentType(_retval); } \
  NS_IMETHOD EnumerateAllItems(sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return _to EnumerateAllItems(aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD EnumerateItemsByProperty(const nsAString & aPropertyID, const nsAString & aPropertyValue, sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return _to EnumerateItemsByProperty(aPropertyID, aPropertyValue, aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD EnumerateItemsByProperties(sbIPropertyArray *aProperties, sbIMediaListEnumerationListener *aEnumerationListener, PRUint16 aEnumerationType) { return _to EnumerateItemsByProperties(aProperties, aEnumerationListener, aEnumerationType); } \
  NS_IMETHOD GetItemsByProperty(const nsAString & aPropertyID, const nsAString & aPropertyValue, nsIArray **_retval) { return _to GetItemsByProperty(aPropertyID, aPropertyValue, _retval); } \
  NS_IMETHOD GetItemCountByProperty(const nsAString & aPropertyID, const nsAString & aPropertyValue, PRUint32 *_retval) { return _to GetItemCountByProperty(aPropertyID, aPropertyValue, _retval); } \
  NS_IMETHOD GetItemsByProperties(sbIPropertyArray *aProperties, nsIArray **_retval) { return _to GetItemsByProperties(aProperties, _retval); } \
  NS_IMETHOD IndexOf(sbIMediaItem *aMediaItem, PRUint32 aStartFrom, PRUint32 *_retval) { return _to IndexOf(aMediaItem, aStartFrom, _retval); } \
  NS_IMETHOD LastIndexOf(sbIMediaItem *aMediaItem, PRUint32 aStartFrom, PRUint32 *_retval) { return _to LastIndexOf(aMediaItem, aStartFrom, _retval); } \
  NS_IMETHOD AddListener(sbIMediaListListener *aListener, PRBool aOwnsWeak, PRUint32 aFlags, sbIPropertyArray *aPropertyFilter) { return _to AddListener(aListener, aOwnsWeak, aFlags, aPropertyFilter); } \
  NS_IMETHOD RemoveListener(sbIMediaListListener *aListener) { return _to RemoveListener(aListener); } \
  NS_IMETHOD RunInBatchMode(sbIMediaListBatchCallback *aCallback, nsISupports *aUserData) { return _to RunInBatchMode(aCallback, aUserData); } \
  NS_IMETHOD GetDistinctValuesForProperty(const nsAString & aPropertyID, nsIStringEnumerator **_retval) { return _to GetDistinctValuesForProperty(aPropertyID, _retval); }

class sbLocalDatabaseLibrary : public sbLocalDatabaseMediaListBase,
                               public sbILibrary,
                               public sbILocalDatabaseLibrary,
                               public nsIObserver,
                               public sbILibraryStatistics
{
  friend class sbLibraryInsertingEnumerationListener;
  friend class sbLibraryRemovingEnumerationListener;
  friend class sbLocalDatabasePropertyCache;
  friend class sbBatchCreateTimerCallback;
  friend class sbBatchCreateHelper;

  struct sbMediaListFactoryInfo {
    sbMediaListFactoryInfo()
    : typeID(0)
    { }

    sbMediaListFactoryInfo(PRUint32 aTypeID, sbIMediaListFactory* aFactory)
    : typeID(aTypeID),
      factory(aFactory)
    { }

    PRUint32 typeID;
    nsCOMPtr<sbIMediaListFactory> factory;
  };

  struct sbMediaItemInfo {
    sbMediaItemInfo(PRPackedBool aHasListType = PR_FALSE,
                    PRPackedBool aHasAudioType = PR_FALSE,
                    PRPackedBool aHasVideoType = PR_FALSE)
    : itemID(0),
      hasItemID(PR_FALSE),
      hasListType(aHasListType),
      hasAudioType(aHasAudioType),
      hasVideoType(aHasVideoType) {
    }

    PRUint32 itemID;
    nsString listType;
    nsCOMPtr<nsIWeakReference> weakRef;
    PRPackedBool hasItemID;
    PRPackedBool hasListType;
    PRPackedBool hasAudioType;
    PRPackedBool hasVideoType;
  };

  struct sbMediaItemPair {
    sbMediaItemPair(sbIMediaItem *aSource,
                    sbIMediaItem *aDestination)
    : sourceItem(aSource)
    , destinationItem(aDestination)
    { }

    nsCOMPtr<sbIMediaItem> sourceItem;
    nsCOMPtr<sbIMediaItem> destinationItem;
  };

  struct sbMediaItemUpdatedInfo {
    sbMediaItemUpdatedInfo(sbIMediaItem           *aItem,
                           sbIPropertyArray       *aProperties,
                           sbGUIDToListMap        *aMediaListTable)
    : item(aItem)
    , newProperties(aProperties)
    , mediaListTable(aMediaListTable)
    { }

    nsCOMPtr<sbIMediaItem> item;
    nsCOMPtr<sbIPropertyArray> newProperties;
    sbGUIDToListMap *mediaListTable;
  };

  typedef nsClassHashtable<nsStringHashKey, sbMediaListFactoryInfo>
          sbMediaListFactoryInfoTable;

  typedef nsClassHashtable<nsStringHashKey, sbMediaItemInfo>
          sbMediaItemInfoTable;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBILIBRARY
  NS_DECL_SBILOCALDATABASELIBRARY
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIOBSERVER
  NS_DECL_SBILIBRARYSTATISTICS

  NS_FORWARD_SBILIBRARYRESOURCE(sbLocalDatabaseMediaListBase::)
  SB_FORWARD_SBIMEDIAITEM(sbLocalDatabaseMediaListBase::)
  SB_FORWARD_SBIMEDIALIST(sbLocalDatabaseMediaListBase::)

  // Include our overrides.
  SB_DECL_MEDIALISTBASE_OVERRIDES
  SB_DECL_SBIMEDIAITEM_OVERRIDES

  sbLocalDatabaseLibrary();
  ~sbLocalDatabaseLibrary();

  NS_IMETHOD GetDefaultSortProperty(nsAString& aProperty);

  nsresult Init(const nsAString& aDatabaseGuid,
                nsIPropertyBag2* aCreationParameters,
                sbILibraryFactory* aFactory,
                nsIURI* aDatabaseLocation = nsnull);

  static void GetNowString(nsAString& _retval);

  /**
   * \brief Bulk removes the selected items specified in aSelection from the
   *        view aView
   */
  nsresult RemoveSelected(nsISimpleEnumerator* aSelection,
                          sbLocalDatabaseMediaListView* aView);

  /*
   * Internal methods for async operations on sbIMediaList
   */
  nsresult AddSomeAsyncInternal(nsISimpleEnumerator *aMediaItems,
                                sbIMediaListAsyncListener *aListener);

private:
  nsresult CreateQueries();

  inline nsresult MakeStandardQuery(sbIDatabaseQuery** _retval,
                                    PRBool aRunAsync = PR_FALSE);

  nsresult AddNewItemQuery(sbIDatabaseQuery* aQuery,
                           const PRUint32 aMediaItemTypeID,
                           const nsAString& aURISpecOrPrefix,
                           nsAString& _retval);

  /**
   * Sets properties of a newly created media item without
   * sending notifications.
   */
  nsresult SetDefaultItemProperties(sbIMediaItem* aItem,
                                    sbIPropertyArray* aProperties,
                                    sbMediaItemInfo* aItemInfo);

  nsresult GetTypeForGUID(const nsAString& aGUID,
                          nsAString& _retval);

  // This callback is meant to be used with mMediaListFactoryTable.
  // aUserData should be a nsTArray<nsString> pointer.
  static PLDHashOperator PR_CALLBACK
    AddTypesToArrayCallback(nsStringHashKey::KeyType aKey,
                            sbMediaListFactoryInfo* aEntry,
                            void* aUserData);

  static PLDHashOperator PR_CALLBACK
    NotifyCopyListeners(nsISupportsHashKey::KeyType aKey,
                        sbILocalDatabaseLibraryCopyListener *aCopyListener,
                        void* aUserData);

  static PLDHashOperator PR_CALLBACK
    NotifyListItemUpdated(nsStringHashKey::KeyType aKey,
                          nsCOMPtr<nsIWeakReference>& aEntry,
                          void* aUserData);

  static PLDHashOperator PR_CALLBACK
    NotifyListsBeforeItemRemoved(nsISupportsHashKey::KeyType aKey,
                                 sbMediaItemArray* aEntry,
                                 void* aUserData);

  static PLDHashOperator PR_CALLBACK
    NotifyListsAfterItemRemoved(nsISupportsHashKey::KeyType aKey,
                                sbMediaItemArray* aEntry,
                                void* aUserData);

  /**
   * \brief Notify lists containing the items that the items are
   *        about to be removed.
   * \note This method also removes the items from the cache.
   */
  static PLDHashOperator PR_CALLBACK
    NotifyListsBeforeAfterItemRemoved(nsISupportsHashKey::KeyType aKey,
                                      sbMediaItemArray* aEntry,
                                      void* aUserData);

  static PLDHashOperator PR_CALLBACK
    EntriesToMediaListArray(nsISupportsHashKey* aEntry,
                            void* aUserData);

  static PLDHashOperator PR_CALLBACK
    RemoveIfNotList(nsStringHashKey::KeyType aKey,
                    nsAutoPtr<sbMediaItemInfo> &aEntry,
                    void *aUserData);

  nsresult RegisterDefaultMediaListFactories();

  nsresult DeleteDatabaseItem(const nsAString& aGuid);

  nsresult AddItemToLocalDatabase(sbIMediaItem* aMediaItem,
                                  sbIMediaItem** _retval);

  nsresult GetSimpleMediaListCopyProperties
                          (sbIMediaList*      aMediaList,
                           sbIPropertyArray** aSimpleProperties);

  nsresult GetContainingLists(sbMediaItemArray* aItems,
                              sbMediaListArray* aLists,
                              sbMediaItemToListsMap* aMap);

  nsresult GetAllListsByType(const nsAString& aType, sbMediaListArray* aArray);

  nsresult ConvertURIsToStrings(nsIArray* aURIs, nsStringArray** aStringArray);

  nsresult ContainsCopy(sbIMediaItem* aMediaItem,
                        PRBool*       aContainsCopy);

  nsresult FilterExistingItems(nsStringArray* aURIs,
                               nsIArray* aPropertyArrayArray,
                               nsTArray<PRUint32>* aFilteredIndexArray,
                               nsStringArray** aFilteredURIs,
                               nsIArray** aFilteredPropertyArrayArray);

  nsresult GetGuidFromContentURI(nsIURI* aURI, nsAString& aGUID);

  nsresult Shutdown();

  /* possibly create an item, and report if it was.
     See sbILibrary::CreateMediaItem.*/
  nsresult CreateMediaItemInternal(nsIURI* aUri,
                                   sbIPropertyArray* aProperties,
                                   PRBool aAllowDuplicates,
                                   PRBool* aWasCreated,
                                   sbIMediaItem** _retval);

  nsresult BatchCreateMediaItemsInternal(nsIArray* aURIArray,
                                         nsIArray* aPropertyArrayArray,
                                         PRBool aAllowDuplicates,
                                         nsIArray** aMediaItemCreatedArray,
                                         sbIBatchCreateMediaItemsListener* aListener,
                                         nsIArray** _retval);

  nsresult ClearInternal(PRBool aExcludeLists = PR_FALSE, 
                         const nsAString &aContentType = EmptyString());

  /* Migration related methods */
  nsresult NeedsMigration(PRBool *aNeedsMigration,
                          PRUint32 *aFromVersion,
                          PRUint32 *aToVersion);

  nsresult MigrateLibrary(PRUint32 aFromVersion, PRUint32 aToVersion);

  nsresult NeedsReindexCollations(PRBool *aNeedsReindexCollations);

  nsresult ReindexCollations();

private:
  // This is the GUID used by the DBEngine to uniquely identify the sqlite
  // database file we'll be using. Don't confuse it with mGuid (inherited from
  // sbLocalDatabaseMediaItem) - that one represents the GUID that uniquely
  // identifies this "media item" in the sqlite database.
  nsString mDatabaseGuid;

  // Location of the database file. This may be null to indicate that the file
  // lives in the default DBEngine database store.
  nsCOMPtr<nsIURI> mDatabaseLocation;

  // A dummy uri that holds a special 'songbird-library://' url.
  nsCOMPtr<nsIURI> mContentSrc;

  nsCOMPtr<sbIDatabasePreparedStatement> mCreateMediaItemPreparedStatement;
  nsCOMPtr<sbIDatabasePreparedStatement> mGetTypeForGUID;

  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  sbMediaListFactoryInfoTable mMediaListFactoryTable;
  sbMediaItemInfoTable mMediaItemTable;

  // Weak references to media lists that have been instantiated
  // (via CreateMediaList and GetMediaItem)
  // Used for fast list update notifications.
  sbGUIDToListMap mMediaListTable;

  nsCOMArray<nsITimer> mBatchCreateTimers;

  nsCOMPtr<nsIPropertyBag2> mCreationParameters;
  nsCOMPtr<sbILibraryFactory> mFactory;

  PRUint32 mAnalyzeCountLimit;

  PRBool mPreventAddedNotification;

  // This monitor protects calls to GetMediaItem.
  PRMonitor *mMonitor;

  // Hashtable that holds all the copy listeners.
  nsInterfaceHashtableMT<nsISupportsHashKey,
                         sbILocalDatabaseLibraryCopyListener> mCopyListeners;

  // initialize the library statistics stuff
  nsresult InitializeLibraryStatistics();
  // precompiled queries for library stats
  nsCOMPtr<sbIDatabaseQuery> mStatisticsSumQuery;
  nsCOMPtr<sbIDatabasePreparedStatement> mStatisticsSumPreparedStatement;

  nsresult FindMusicFolderURI(nsIURI ** aMusicFolderURI);

  nsresult SubmitCopyRequest(sbIMediaItem * aSourceItem,
                             sbIMediaItem * aDestinationItem);
};

/**
 * class sbLibraryInsertingEnumerationListener
 */
class sbLibraryInsertingEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbLibraryInsertingEnumerationListener(sbLocalDatabaseLibrary* aLibrary)
  : mFriendLibrary(aLibrary),
    mShouldInvalidate(PR_FALSE),
    mLength(0)
  {
    NS_ASSERTION(mFriendLibrary, "Null pointer!");
  }

private:
  sbLocalDatabaseLibrary* mFriendLibrary;
  PRBool mShouldInvalidate;

  sbMediaItemArray mNotificationList;

  // This list is to enable copyListener notifications
  // which must have the original item and the newly
  // created item available for the listener.
  sbMediaItemArray mOriginalItemList;

  PRUint32 mLength;
};

/**
 * class sbLibraryRemovingEnumerationListener
 */
class sbLibraryRemovingEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbLibraryRemovingEnumerationListener(sbLocalDatabaseLibrary* aLibrary)
  : mFriendLibrary(aLibrary),
    mItemEnumerated(PR_FALSE)
  {
    NS_ASSERTION(mFriendLibrary, "Null pointer!");
  }

private:
  sbLocalDatabaseLibrary* mFriendLibrary;
  nsCOMPtr<sbIDatabaseQuery> mDBQuery;
  nsCOMArray<sbIMediaItem> mNotificationList;
  nsTArray<PRUint32> mNotificationIndexes;
  PRPackedBool mItemEnumerated;
};

// Forward declare
class sbBatchCreateHelper;

class sbBatchCreateTimerCallback : public nsITimerCallback
{
friend class sbLocalDatabaseLibrary;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  sbBatchCreateTimerCallback(sbLocalDatabaseLibrary* aLibrary,
                             sbIBatchCreateMediaItemsListener* aListener,
                             sbIDatabaseQuery* aQuery);

  nsresult Init();

  nsresult SetQueryCount(PRUint32 aQueryCount);

  nsresult NotifyInternal(PRBool* _retval);

  sbBatchCreateHelper* BatchHelper();

private:
  sbLocalDatabaseLibrary* mLibrary;
  nsCOMPtr<sbIBatchCreateMediaItemsListener> mListener;
  nsRefPtr<sbBatchCreateHelper> mBatchHelper;
  nsCOMPtr<sbIDatabaseQuery> mQuery;
  nsITimer* mTimer;
  PRUint32 mQueryCount;

};

class sbBatchCreateHelper
{
public:
  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  sbBatchCreateHelper(sbLocalDatabaseLibrary* aLibrary,
                      sbBatchCreateTimerCallback* aCallback = nsnull);

  nsresult InitQuery(sbIDatabaseQuery* aQuery,
                     nsStringArray* aURIArray,
                     nsIArray* aPropertyArrayArray);

  nsresult NotifyAndGetItems(nsIArray** _retval);

protected:
  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

private:
  // When this is used for an async batch, the helper will have an owning
  // reference to the callback
  sbLocalDatabaseLibrary*     mLibrary;
  sbBatchCreateTimerCallback*      mCallback;
  nsAutoPtr<nsStringArray>  mURIArray;
  nsCOMPtr<nsIArray>  mPropertiesArray;
  nsTArray<nsString>  mGuids;
  PRUint32 mLength;
};

class sbAutoSimpleMediaListBatchHelper
{
public:
  sbAutoSimpleMediaListBatchHelper(sbMediaListArray* aLists)
  : mLists(aLists)
  {
    NS_ASSERTION(aLists, "Null pointer!");
    for (PRInt32 i = 0; i < mLists->Count(); i++) {
      nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
        do_QueryInterface(mLists->ObjectAt(i));
      if (simple)
        simple->NotifyListenersBatchBegin(mLists->ObjectAt(i));
    }
  }

  ~sbAutoSimpleMediaListBatchHelper()
  {
    for (PRInt32 i = 0; i < mLists->Count(); i++) {
      nsCOMPtr<sbILocalDatabaseSimpleMediaList> simple =
        do_QueryInterface(mLists->ObjectAt(i));
      if (simple)
        simple->NotifyListenersBatchEnd(mLists->ObjectAt(i));
    }
  }

private:
  // Not meant to be implemented. This makes it a compiler error to
  // attempt to create an object on the heap.
  static void* operator new(size_t /*size*/) CPP_THROW_NEW {return 0;}
  static void operator delete(void* /*memory*/) { }

  sbMediaListArray* mLists;
};

#endif /* __SBLOCALDATABASELIBRARY_H__ */
