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

#ifndef __SBLOCALDATABASETREEVIEW_H__
#define __SBLOCALDATABASETREEVIEW_H__

#include <nsITreeView.h>
#include <sbILocalDatabaseAsyncGUIDArray.h>
#include <sbIMediaListViewTreeView.h>

#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

class nsITreeBoxObject;
class nsITreeSelection;
class sbILocalDatabasePropertyCache;
class sbILocalDatabaseResourcePropertyBag;
class sbIMediaListView;
class sbIPropertyArray;

class sbLocalDatabaseTreeView : public nsITreeView,
                                public sbILocalDatabaseAsyncGUIDArrayListener,
                                public sbIMediaListViewTreeView
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITREEVIEW
  NS_DECL_SBILOCALDATABASEASYNCGUIDARRAYLISTENER
  NS_DECL_SBIMEDIALISTVIEWTREEVIEW

  sbLocalDatabaseTreeView();

  ~sbLocalDatabaseTreeView();

  nsresult Init(sbIMediaListView* aListView,
                sbILocalDatabaseAsyncGUIDArray* aArray,
                sbIPropertyArray* aCurrentSort);

  nsresult Rebuild();

private:
  enum PageCacheStatus {
    eNotCached,
    ePending,
    eCached
  };

  nsresult UpdateRowCount(PRUint32 aRowCount);

  nsresult GetPropertyForTreeColumn(nsITreeColumn* aTreeColumn,
                                    nsAString& aProperty);

  nsresult GetPropertyBag(const nsAString& aGuid,
                          PRUint32 aIndex,
                          sbILocalDatabaseResourcePropertyBag** _retval);

  nsresult GetPageCachedStatus(PRUint32 aIndex, PageCacheStatus* aStatus);

  nsresult SetPageCachedStatus(PRUint32 aIndex, PageCacheStatus aStatus);

  nsresult SetSort(const nsAString& aProperty, PRBool aDirection);

  void InvalidateCache();

  // The media list view that this tree view is a view of
  nsCOMPtr<sbIMediaListView> mMediaListView;

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
  nsCOMPtr<nsITreeBoxObject> mTreeBoxObject;

  // Listener
  nsCOMPtr<sbIMediaListViewTreeViewObserver> mObserver;

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
};

#endif /* __SBLOCALDATABASETREEVIEW_H__ */

