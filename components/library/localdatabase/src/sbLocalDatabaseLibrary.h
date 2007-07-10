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

#ifndef __SBLOCALDATABASELIBRARY_H__
#define __SBLOCALDATABASELIBRARY_H__

#include <nsIObserver.h>
#include <sbIDatabaseQuery.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbILocalDatabaseSimpleMediaList.h>
#include "sbLocalDatabaseMediaListBase.h"

#include <nsClassHashtable.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIClassInfo.h>
#include <nsITimer.h>
#include <nsStringGlue.h>
#include <sbIMediaListFactory.h>

class nsIPropertyBag2;
class nsIURI;
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

typedef nsCOMArray<sbIMediaItem> sbMediaItemArray;
typedef nsCOMArray<sbIMediaList> sbMediaListArray;
typedef nsClassHashtable<nsISupportsHashKey, sbMediaItemArray>
        sbMediaItemToListsMap;

static PLDHashOperator PR_CALLBACK
  NotifyListsBeforeItemRemoved(nsISupportsHashKey::KeyType aKey,
                               sbMediaItemArray* aEntry,
                               void* aUserData);

static PLDHashOperator PR_CALLBACK
  NotifyListsAfterItemRemoved(nsISupportsHashKey::KeyType aKey,
                              sbMediaItemArray* aEntry,
                              void* aUserData);

static PLDHashOperator PR_CALLBACK
  EntriesToMediaListArray(nsISupportsHashKey* aEntry,
                          void* aUserData);

// These are the methods from sbLocalDatabaseMediaListBase that we're going to
// override in sbLocalDatabaseLibrary. Most of them are from sbIMediaList.
#define SB_DECL_MEDIALISTBASE_OVERRIDES                                         \
  NS_IMETHOD GetType(nsAString& aType);                                         \
  NS_IMETHOD GetItemByGuid(const nsAString& aGuid, sbIMediaItem** _retval);     \
  NS_IMETHOD Contains(sbIMediaItem* aMediaItem, PRBool* _retval);               \
  NS_IMETHOD Add(sbIMediaItem* aMediaItem);                                     \
  NS_IMETHOD AddAll(sbIMediaList* aMediaList);                                  \
  NS_IMETHOD AddSome(nsISimpleEnumerator* aMediaItems);                         \
  NS_IMETHOD Remove(sbIMediaItem* aMediaItem);                                  \
  NS_IMETHOD RemoveByIndex(PRUint32 aIndex);                                    \
  NS_IMETHOD RemoveSome(nsISimpleEnumerator* aMediaItems);                      \
  NS_IMETHOD Clear();                                                           \
  NS_IMETHOD CreateView(sbIMediaListView** _retval);                            \
  /* nothing */

class sbLocalDatabaseLibrary : public sbLocalDatabaseMediaListBase,
                               public sbIDatabaseSimpleQueryCallback,
                               public sbILibrary,
                               public sbILocalDatabaseLibrary,
                               public nsIObserver
{
  friend class sbLibraryInsertingEnumerationListener;
  friend class sbLibraryRemovingEnumerationListener;
  friend class sbLocalDatabasePropertyCache;
  friend class sbBatchCreateTimerCallback;

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
    sbMediaItemInfo()
    : itemID(0),
      hasItemID(PR_FALSE),
      hasListType(PR_FALSE)
    { }

    PRUint32 itemID;
    nsString listType;
    nsCOMPtr<nsIWeakReference> weakRef;
    PRPackedBool hasItemID;
    PRPackedBool hasListType;
  };

  typedef nsClassHashtable<nsStringHashKey, sbMediaListFactoryInfo>
          sbMediaListFactoryInfoTable;

  typedef nsClassHashtable<nsStringHashKey, sbMediaItemInfo>
          sbMediaItemInfoTable;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBIDATABASESIMPLEQUERYCALLBACK
  NS_DECL_SBILIBRARY
  NS_DECL_SBILOCALDATABASELIBRARY
  NS_DECL_NSICLASSINFO
  NS_DECL_NSIOBSERVER
  NS_FORWARD_SBILIBRARYRESOURCE(sbLocalDatabaseMediaListBase::)

  // Include our overrides.
  SB_DECL_MEDIALISTBASE_OVERRIDES

  sbLocalDatabaseLibrary();
  ~sbLocalDatabaseLibrary();

  NS_IMETHOD GetDefaultSortProperty(nsAString& aProperty);

  nsresult Init(const nsAString& aDatabaseGuid,
                nsIPropertyBag2* aCreationParameters,
                sbILibraryFactory* aFactory,
                nsIURI* aDatabaseLocation = nsnull);

  /**
   * \brief Bulk removes the selected items specified in aSelection from the
   *        view aView
   */
  nsresult RemoveSelected(nsISimpleEnumerator* aSelection,
                          sbLocalDatabaseMediaListView* aView);

private:
  nsresult CreateQueries();

  inline nsresult MakeStandardQuery(sbIDatabaseQuery** _retval,
                                    PRBool aRunAsync = PR_FALSE);

  inline void GetNowString(nsAString& _retval);

  nsresult AddNewItemQuery(sbIDatabaseQuery* aQuery,
                           const PRUint32 aMediaItemTypeID,
                           const nsAString& aURISpecOrPrefix,
                           nsAString& _retval);

  nsresult GetTypeForGUID(const nsAString& aGUID,
                          nsAString& _retval);

  // This callback is meant to be used with mMediaListFactoryTable.
  // aUserData should be a nsTArray<nsString> pointer.
  static PLDHashOperator PR_CALLBACK
    AddTypesToArrayCallback(nsStringHashKey::KeyType aKey,
                            sbMediaListFactoryInfo* aEntry,
                            void* aUserData);

  static PLDHashOperator PR_CALLBACK
    NotifyListsItemUpdated(nsISupportsHashKey::KeyType aKey,
                           sbMediaItemArray* aEntry,
                           void* aUserData);

  nsresult RegisterDefaultMediaListFactories();

  nsresult DeleteDatabaseItem(const nsAString& aGuid);

  nsresult AddItemToLocalDatabase(sbIMediaItem* aMediaItem,
                                  sbIMediaItem** _retval);

  void IncrementDatabaseDirtyItemCounter(PRUint32 aIncrement = 1);

  nsresult RunAnalyzeQuery(PRBool aRunAsync = PR_TRUE);

  nsresult GetContainingLists(sbMediaItemArray* aItems,
                              sbMediaListArray* aLists,
                              sbMediaItemToListsMap* aMap);

  nsresult GetAllListsByType(const nsAString& aType, sbMediaListArray* aArray);

  nsresult Shutdown();

private:
  // This is the GUID used by the DBEngine to uniquely identify the sqlite
  // database file we'll be using. Don't confuse it with mGuid (inherited from
  // sbLocalDatabaseMediaItem) - that one represents the GUID that uniquely
  // identifies this "media item" in the sqlite database.
  nsString mDatabaseGuid;

  // Location of the database file. This may be null to indicate that the file
  // lives in the default DBEngine database store.
  nsCOMPtr<nsIURI> mDatabaseLocation;

  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  nsString mGetTypeForGUIDQuery;
  nsString mGetMediaItemIdForGUIDQuery;
  nsString mInsertMediaItemQuery;
  nsString mMediaListFactoriesQuery;
  nsString mInsertMediaListFactoryQuery;

  // Query to delete a single item from the library.
  nsString mDeleteItemQuery;

  // Query to clear the entire library.
  nsString mDeleteAllQuery;

  // Query to grab the media list factory type ID based on its type string.
  nsString mGetFactoryIDForTypeQuery;

  // Get the guids of all lists by type
  nsString mGetAllListsByTypeId;

  sbMediaListFactoryInfoTable mMediaListFactoryTable;

  sbMediaItemInfoTable mMediaItemTable;

  nsCOMArray<nsITimer> mBatchCreateTimers;

  nsCOMPtr<nsIPropertyBag2> mCreationParameters;
  nsCOMPtr<sbILibraryFactory> mFactory;

  PRUint32 mDirtyItemCount;

  PRUint32 mAnalyzeCountLimit;

  PRBool mPreventAddedNotification;
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
    mShouldInvalidate(PR_FALSE)
  {
    NS_ASSERTION(mFriendLibrary, "Null pointer!");
  }

private:
  sbLocalDatabaseLibrary* mFriendLibrary;
  PRBool mShouldInvalidate;
  sbMediaItemArray mNotificationList;
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
  PRPackedBool mItemEnumerated;
};

class sbBatchCreateTimerCallback : public nsITimerCallback
{
friend class sbLocalDatabaseLibrary;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMERCALLBACK

  sbBatchCreateTimerCallback(sbLocalDatabaseLibrary* aLibrary,
                             sbIBatchCreateMediaItemsListener* aListener,
                             sbIDatabaseQuery* aQuery);
private:
  sbLocalDatabaseLibrary* mLibrary;
  nsCOMPtr<sbIBatchCreateMediaItemsListener> mListener;
  nsCOMPtr<sbIDatabaseQuery> mQuery;
  nsTArray<nsString> mGuids;

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

