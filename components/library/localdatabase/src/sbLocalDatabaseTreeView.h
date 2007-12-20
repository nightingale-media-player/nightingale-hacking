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

#ifndef __SBLOCALDATABASETREEVIEW_H__
#define __SBLOCALDATABASETREEVIEW_H__

#include <nsIClassInfo.h>
#include <nsITreeView.h>
#include <nsITreeSelection.h>
#include <sbILocalDatabaseAsyncGUIDArray.h>
#include <sbILocalDatabaseGUIDArray.h>
#include <sbILocalDatabaseTreeView.h>
#include <sbIMediaListViewTreeView.h>
#include <sbIPlaylistPlayback.h>

#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsInterfaceHashtable.h>
#include <nsISerializable.h>
#include <nsClassHashtable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsWeakReference.h>

class nsIObjectInputStream;
class nsIObjectOutputStream;
class nsISupportsArray;
class nsITreeBoxObject;
class nsITreeColumn;
class nsITreeSelection;
class sbILocalDatabasePropertyCache;
class sbILocalDatabaseResourcePropertyBag;
class sbILibrary;
class sbILibrarySort;
class sbIMediaList;
class sbIMediaListView;
class sbIPropertyArray;
class sbIPropertyInfo;
class sbIPropertyManager;
class sbITreeViewPropertyInfo;
class sbLocalDatabaseMediaListView;

class sbLocalDatabaseTreeViewState;

typedef nsDataHashtable<nsStringHashKey, nsString> sbSelectionList;

class sbLocalDatabaseTreeView : public nsSupportsWeakReference,
                                public nsIClassInfo,
                                public nsITreeView,
                                public sbILocalDatabaseAsyncGUIDArrayListener,
                                public sbILocalDatabaseGUIDArrayListener,
                                public sbIMediaListViewTreeView,
                                public sbILocalDatabaseTreeView,
                                public sbIPlaylistPlaybackListener
{
  friend class sbLocalDatabaseTreeSelection;
  typedef nsresult (*PR_CALLBACK sbSelectionEnumeratorCallbackFunc)
    (PRUint32 aIndex, const nsAString& aId, const nsAString& aGuid, void* aUserData);

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSITREEVIEW
  NS_DECL_SBILOCALDATABASEASYNCGUIDARRAYLISTENER
  NS_DECL_SBILOCALDATABASEGUIDARRAYLISTENER
  NS_DECL_SBIMEDIALISTVIEWTREEVIEW
  NS_DECL_SBILOCALDATABASETREEVIEW
  NS_DECL_SBIPLAYLISTPLAYBACKLISTENER

  sbLocalDatabaseTreeView();

  ~sbLocalDatabaseTreeView();

  nsresult Init(sbLocalDatabaseMediaListView* aListView,
                sbILocalDatabaseAsyncGUIDArray* aArray,
                sbIPropertyArray* aCurrentSort,
                sbLocalDatabaseTreeViewState* aState);

  nsresult Rebuild();

  void ClearMediaListView();

  nsresult GetState(sbLocalDatabaseTreeViewState** aState);

protected:
  nsresult TokenizeProperties(const nsAString& aProperties,
                              nsISupportsArray* aAtomArray);

private:

  enum PageCacheStatus {
    eNotCached,
    ePending,
    eCached
  };

  enum MediaListType {
    eLibrary,
    eSimple,
    eDistinct
  };

  nsresult UpdateRowCount(PRUint32 aRowCount);

  nsresult GetPropertyForTreeColumn(nsITreeColumn* aTreeColumn,
                                    nsAString& aProperty);

  nsresult GetTreeColumnForProperty(const nsAString& aProperty,
                                    nsITreeColumn** aTreeColumn);

  nsresult GetCellPropertyValue(PRInt32 aIndex,
                                nsITreeColumn *aTreeColumn,
                                nsAString& _retval);

  nsresult GetPropertyBag(const nsAString& aGuid,
                          PRUint32 aIndex,
                          sbILocalDatabaseResourcePropertyBag** _retval);

  nsresult GetPageCachedStatus(PRUint32 aIndex, PageCacheStatus* aStatus);

  nsresult SetPageCachedStatus(PRUint32 aIndex, PageCacheStatus aStatus);

  nsresult InvalidateCache();

  nsresult SaveSelectionList();

  nsresult EnumerateSelection(sbSelectionEnumeratorCallbackFunc aFunc,
                              void* aUserData);

  nsresult GetUniqueIdForIndex(PRUint32 aIndex, nsAString& aId);

  void SetSelectionIsAll(PRBool aSelectionIsAll);

  void ClearSelectionList();

  nsresult UpdateColumnSortAttributes(const nsAString& aProperty,
                                      PRBool aDirection);

  static nsresult PR_CALLBACK
    SelectionListSavingEnumeratorCallback(PRUint32 aIndex,
                                          const nsAString& aId,
                                          const nsAString& aGuid,
                                          void* aUserData);

  static nsresult PR_CALLBACK
    SelectionToArrayEnumeratorCallback(PRUint32 aIndex,
                                       const nsAString& aId,
                                       const nsAString& aGuid,
                                       void* aUserData);

  static nsresult PR_CALLBACK
    SelectionIndexEnumeratorCallback(PRUint32 aIndex,
                                     const nsAString& aId,
                                     const nsAString& aGuid,
                                     void* aUserData);

  static PLDHashOperator PR_CALLBACK
    CopySelectionListCallback(nsStringHashKey::KeyType aKey,
                              nsString aEntry,
                              void* aUserData);

  inline PRUint32 TreeToArray(PRInt32 aRow) {
    return (PRUint32) (mFakeAllRow ? aRow - 1 : aRow);
  }

  inline PRUint32 ArrayToTree(PRUint32 aIndex) {
    return (PRInt32) (mFakeAllRow ? aIndex + 1 : aIndex);
  }

  inline PRBool IsAllRow(PRInt32 aRow) {
    return mFakeAllRow && aRow == 0;
  }

  inline nsresult GetColumnPropertyInfo(nsITreeColumn* aColumn,
                                        sbIPropertyInfo** aPropertyInfo);

  nsresult GetPropertyInfoAndCachedValue(PRInt32 aRow,
                                         nsITreeColumn* aColumn,
                                         nsAString& aValue,
                                         sbIPropertyInfo** aPropertyInfo);

  nsresult GetPlayingProperty(PRUint32 aIndex,
                              nsISupportsArray* properties);

  // Cached property manager
  nsCOMPtr<sbIPropertyManager> mPropMan;

  // Type of media list this tree view is of
  MediaListType mListType;

  // The media list view that this tree view is a view of.  This pointer is
  // set to null when the view is destroyed.
  sbLocalDatabaseMediaListView* mMediaListView;

  // The async guid array given to us by our view
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> mArray;

  // The cached row count of the async guid array
  PRUint32 mCachedRowCount;

  // The fetch size of the guid array.  This is used to compute the size of
  // our pages when tracking their cached status
  PRUint32 mFetchSize;

  // Cache that maps row index number to a property bag of properties
  nsInterfaceHashtable<nsUint32HashKey, sbILocalDatabaseResourcePropertyBag> mRowCache;

  // Tracks the cache status of each page as determined by the fetch size of
  // the guid array.  Note that the values in this hash table are actually
  // from the PageCacheStatus enum defined above.
  nsDataHashtable<nsUint32HashKey, PRUint32> mPageCacheStatus;

  // The property cache that is linked with the guid array
  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  // Current sort property
  nsString mCurrentSortProperty;

  // This variable along with  mGetByIndexAsyncPending manage which row index
  // is "on deck" to be requested.  If mNextGetByIndexAsync is not -1 when a
  // request completes, it is automatically requested next.  This makes sure
  // that we only have one pending request to the guid array at a time.
  PRInt32 mNextGetByIndexAsync;

  // Stuff the tree view needs to track
  nsCOMPtr<nsITreeSelection> mSelection;
  nsCOMPtr<nsITreeSelection> mRealSelection;
  nsCOMPtr<nsITreeBoxObject> mTreeBoxObject;

  // Weak listener
  nsCOMPtr<nsIWeakReference> mObserver;

  // Temporary cache of visible rows while refreshing
  nsInterfaceHashtable<nsUint32HashKey, sbILocalDatabaseResourcePropertyBag> mDirtyRowCache;

  // Saved list of selected rows and associated guids used to restore selection
  // between rebuilds
  sbSelectionList mSelectionList;

  // Mouse state
  PRUint32 mMouseState;
  PRInt32 mMouseStateRow;
  nsCOMPtr<nsITreeColumn> mMouseStateColumn;

  // Currently playing track UID and index
  nsString mPlayingItemUID;
  nsCOMPtr<sbIPlaylistPlayback> mPlaylistPlayback;

  // True when the everything is selected
  PRPackedBool mSelectionIsAll;

  // True if the cached row count is no longer valid
  PRPackedBool mCachedRowCountDirty;

  // True if a row count request from the async guid array is pending
  PRPackedBool mCachedRowCountPending;

  // Array busy flag that is managed by the onStateChange() callback on the
  // listener.  When this is true, you will block when you call a non-async
  // method on the guid array
  PRPackedBool mIsArrayBusy;

  // Current sort direction
  PRPackedBool mCurrentSortDirectionIsAscending;

  // See mNextGetByIndexAsync
  PRPackedBool mGetByIndexAsyncPending;

  // Used to cause a selection clear on the next async response.
  PRPackedBool mClearSelectionPending;

  // Should we include a fake "All" row in the tree
  PRPackedBool mFakeAllRow;

  // Flag to indicate that the tree is changing its selection
  PRPackedBool mSelectionChanging;

  // True when we have a listener added to the playback service
  PRPackedBool mIsListeningToPlayback;

  nsString mLocalizedAll;
};

class sbLocalDatabaseTreeSelection : public nsITreeSelection
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITREESELECTION

  sbLocalDatabaseTreeSelection(nsITreeSelection* aSelection,
                               sbLocalDatabaseTreeView* mTreeView,
                               PRBool mAllRow);

private:
  nsresult CheckIsSelectAll(PRBool* _retval = nsnull);
  PRBool RangeIncludesNotCachedPage(PRUint32 startIndex, PRUint32 endIndex);

  nsCOMPtr<nsITreeSelection> mSelection;

  // A pointer to the friend sbLocalDatabaseTreeView class.  This class keeps
  // a strong reference to this class so this pointer will never be invalid.
  sbLocalDatabaseTreeView* mTreeView;

  // Include an all row in this selection.  When this is true, the first row
  // (the "all" row) will remain selected when nothing else is selected.
  PRBool mAllRow;
};

class sbGUIDArrayToIndexedMediaItemEnumerator : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  sbGUIDArrayToIndexedMediaItemEnumerator(sbILibrary* aLibrary);

  nsresult AddGuid(const nsAString& aGuid, PRUint32 aIndex);

private:
  struct Item {
    PRUint32 index;
    nsString guid;
  };

  nsresult GetNextItem();

  PRBool mInitalized;
  nsCOMPtr<sbILibrary> mLibrary;
  nsTArray<Item> mItems;
  PRUint32 mNextIndex;
  nsCOMPtr<sbIMediaItem> mNextItem;
  PRUint32 mNextItemIndex;
};

class sbIndexedGUIDArrayEnumerator : public nsISimpleEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR

  sbIndexedGUIDArrayEnumerator(sbILibrary* aLibrary,
                               sbILocalDatabaseGUIDArray* aArray);

private:
  nsresult Init();

  nsTArray<nsString> mGUIDArray;
  nsCOMPtr<sbILibrary> mLibrary;
  nsCOMPtr<sbILocalDatabaseGUIDArray> mArray;
  PRUint32 mNextIndex;
  PRBool mInitalized;
};

class sbLocalDatabaseTreeViewState : public nsISerializable
{
friend class sbLocalDatabaseTreeView;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISERIALIZABLE

  nsresult Init();
  nsresult ToString(nsAString& aStr); 

  sbLocalDatabaseTreeViewState();

protected:
  nsCOMPtr<sbILibrarySort> mSort;

  sbSelectionList mSelectionList;

  PRPackedBool mSelectionIsAll;

private:
  static PLDHashOperator PR_CALLBACK
    SerializeSelectionListCallback(nsStringHashKey::KeyType aKey,
                                   nsString aEntry,
                                   void* aUserData);
};

#endif /* __SBLOCALDATABASETREEVIEW_H__ */

