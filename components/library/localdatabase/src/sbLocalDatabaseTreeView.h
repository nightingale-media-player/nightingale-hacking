/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#ifndef __SBLOCALDATABASETREEVIEW_H__
#define __SBLOCALDATABASETREEVIEW_H__

#include "sbSelectionListUtils.h"

#include <nsIClassInfo.h>
#include <nsITreeView.h>
#include <nsITreeSelection.h>
#include <nsIWeakReference.h>
#include <nsIWeakReferenceUtils.h>

#include <sbILocalDatabaseGUIDArray.h>
#include <sbILocalDatabaseTreeView.h>
#include <sbIMediacoreEventListener.h>
#include <sbIMediaListViewTreeView.h>
#include <sbIMediaListViewSelection.h>

#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsInterfaceHashtable.h>
#include <nsISerializable.h>
#include <nsClassHashtable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsWeakReference.h>
#include <nsTObserverArray.h>

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
class sbIMediacoreEvent;
class sbIMediaList;
class sbIMediaListView;
class sbIPropertyArray;
class sbIPropertyInfo;
class sbIPropertyManager;
class sbITreeViewPropertyInfo;
class sbLocalDatabaseMediaListView;
class sbFilterTreeSelection;
class sbPlaylistTreeSelection;
class sbLocalDatabaseTreeViewState;

class sbLocalDatabaseTreeView : public nsSupportsWeakReference,
                                public nsIClassInfo,
                                public sbILocalDatabaseGUIDArrayListener,
                                public sbIMediaListViewTreeView,
                                public sbILocalDatabaseTreeView,
                                public sbIMediacoreEventListener,
                                public sbIMediaListViewSelectionListener

{
  friend class sbFilterTreeSelection;
  friend class sbPlaylistTreeSelection;

  typedef nsresult (*PR_CALLBACK sbSelectionEnumeratorCallbackFunc)
    (PRUint32 aIndex, const nsAString& aId, const nsAString& aGuid, void* aUserData);

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_NSITREEVIEW
  NS_DECL_SBILOCALDATABASEGUIDARRAYLISTENER
  NS_DECL_SBIMEDIALISTVIEWTREEVIEW
  NS_DECL_SBILOCALDATABASETREEVIEW
  NS_DECL_SBIMEDIACOREEVENTLISTENER
  NS_DECL_SBIMEDIALISTVIEWSELECTIONLISTENER

  sbLocalDatabaseTreeView();

  ~sbLocalDatabaseTreeView();

  nsresult Init(sbLocalDatabaseMediaListView* aListView,
                sbILocalDatabaseGUIDArray* aArray,
                sbIPropertyArray* aCurrentSort,
                sbLocalDatabaseTreeViewState* aState);

  nsresult Rebuild();

  // Getter and setter methods to toggle rebuild prevention.
  void SetShouldPreventRebuild(PRBool aShouldPreventRebuild);
  PRBool GetShouldPreventRebuild();

  void ClearMediaListView();

  nsresult GetState(sbLocalDatabaseTreeViewState** aState);

protected:
  nsresult TokenizeProperties(const nsAString& aProperties,
                              nsISupportsArray* aAtomArray);

  // Event Listener Handlers
  nsresult OnStop();

  nsresult OnTrackChange();
  nsresult OnTrackChange(sbIMediaListView *aView,
                         PRUint32 aIndex);

  nsresult OnTrackIndexChange(sbIMediacoreEvent *aEvent);

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

  nsresult GetPropertyForTreeColumn(nsITreeColumn* aTreeColumn,
                                    nsAString& aProperty);

  nsresult GetTreeColumnForProperty(const nsAString& aProperty,
                                    nsITreeColumn** aTreeColumn);

  nsresult GetCellPropertyValue(PRInt32 aIndex,
                                nsITreeColumn *aTreeColumn,
                                nsAString& _retval);

  nsresult SaveSelectionList();

  nsresult RestoreSelection();

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
    SelectionListGuidsEnumeratorCallback(PRUint32 aIndex,
                                         const nsAString& aId,
                                         const nsAString& aGuid,
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

  nsresult GetPropertyInfoAndValue(PRInt32 aRow,
                                   nsITreeColumn* aColumn,
                                   nsAString& aValue,
                                   sbIPropertyInfo** aPropertyInfo);

  nsresult GetPlayingProperty(PRUint32 aIndex,
                              nsISupportsArray* properties);

  nsresult GetIsListReadOnly(PRBool *aOutIsReadOnly);

  nsresult GetBag(PRUint32 aIndex,
                  sbILocalDatabaseResourcePropertyBag** aBag);

  nsresult GetBag(const nsAString& aGuid,
                  sbILocalDatabaseResourcePropertyBag** aBag);

  // Cached property manager
  nsCOMPtr<sbIPropertyManager> mPropMan;

  // Type of media list this tree view is of
  MediaListType mListType;

  // The media list view that this tree view is a view of.  This pointer is
  // set to null when the view is destroyed.
  sbLocalDatabaseMediaListView* mMediaListView;
  sbIMediaListViewSelection* mViewSelection;

  // The async guid array given to us by our view
  nsCOMPtr<sbILocalDatabaseGUIDArray> mArray;

  // The cached length of the guid array
  PRUint32 mArrayLength;

  // The fetch size of the guid array.  This is used to compute the size of
  // our pages when tracking their cached status
  PRUint32 mFetchSize;

  // The property cache that is linked with the guid array
  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  // Current sort property
  nsString mCurrentSortProperty;

  // Stuff the tree view needs to track
  nsCOMPtr<nsITreeSelection> mSelection;
  nsCOMPtr<nsITreeSelection> mRealSelection;
  nsCOMPtr<nsITreeBoxObject> mTreeBoxObject;

  // Weak listener
  nsCOMPtr<nsIWeakReference> mObserver;

  // Do we manage our selection?  Filters do, playlists don't
  PRBool mManageSelection;

  // Saved list of selected rows and associated guids used to restore selection
  // between rebuilds
  PRBool mHaveSavedSelection;
  sbSelectionList mSelectionList;

  // Mouse state
  PRUint32 mMouseState;
  PRInt32 mMouseStateRow;
  nsCOMPtr<nsITreeColumn> mMouseStateColumn;

  // Currently playing track UID and index
  nsString mPlayingItemUID;
  nsCOMPtr<nsIWeakReference> mMediacoreManager;

  // True when the everything is selected
  PRPackedBool mSelectionIsAll;

  // Current sort direction
  PRPackedBool mCurrentSortDirectionIsAscending;

  // Should we include a fake "All" row in the tree
  PRPackedBool mFakeAllRow;

  // True when we have a listener added to the playback service
  PRPackedBool mIsListeningToPlayback;

  // True when the tree should prevent rebuilding itself.
  PRPackedBool mShouldPreventRebuild;

  nsString mLocalizedAll;

  static PRInt32 const NOT_SET = -1;

  PRInt32 mFirstCachedRow;
  PRInt32 mLastCachedRow;
  /**
   * Nested class used to hold guid strings so that we can efficiently pass it
   * off to a function. This is used for the tree views and the number of items
   * should be relatively small < 100. These are frequently accessed for rendering
   * so this should be a win, reducing memory churn and fragmentation.
   */
  class GuidArray
  {
  public:
    /**
     * Initialize our item count to zero
     */
    GuidArray() : mItems(0) {}
    /**
     * Sets the capacity of our data arrays
     */
    void SetCapacity(PRUint32 aCapacity) {
      mTempGuids.SetCapacity(aCapacity);
      mTempGuidChars.SetCapacity(aCapacity);
    }
    /**
     * Appends a guid string to our list.
     * Append calls should never exceed the value passed to SetCapacity
     */
    nsresult Append(nsString const & aGuid) {
      // If the array is not big enough append
      if (mItems < mTempGuids.Length()) {
        mTempGuids[mItems] = aGuid;
        mTempGuidChars[mItems] = mTempGuids[mItems].get();
      }
      else {
        // When appending, mItems should always equal the length of mTempGuids
        NS_ASSERTION(mItems == mTempGuids.Length(),
                     "sbLocalDatabaseTreeView::GuidArray's array is out of sync");
        nsString * newGuid = mTempGuids.AppendElement(aGuid);
        NS_ENSURE_TRUE(newGuid, NS_ERROR_OUT_OF_MEMORY);

        mTempGuidChars.AppendElement(newGuid->get());
      }
      ++mItems;
      return NS_OK;
    }
    /**
     * Resets the number of items we hold without freeing anything
     */
    void Reset() {
      mItems = 0;
    }
    /**
     * Returns the actual number of items we hold
     */
    PRUint32 Length() const {
      return mItems;
    }
    /**
     * Return the guid strings as a PRUnichar * array
     */
    PRUnichar const * const * AsCharArray() const {
      return mTempGuidChars.Elements();
    }
  private:
    nsTArray<nsString> mTempGuids;
    nsTArray<PRUnichar const *> mTempGuidChars;
    PRUint32 mItems;
  };
  GuidArray mGuidWorkArray;
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

};

#endif /* __SBLOCALDATABASETREEVIEW_H__ */
