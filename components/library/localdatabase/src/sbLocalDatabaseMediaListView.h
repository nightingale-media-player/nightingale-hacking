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

#ifndef __SB_LOCALDATABASEMEDIALISTVIEW_H__
#define __SB_LOCALDATABASEMEDIALISTVIEW_H__

#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsClassHashtable.h>
#include <nsHashKeys.h>
#include <nsIClassInfo.h>
#include <nsIStringEnumerator.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsTHashtable.h>
#include <nsWeakReference.h>
#include <prlock.h>
#include <sbIFilterableMediaListView.h>
#include <sbIMediaListListener.h>
#include <sbIMediaListView.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbISearchableMediaListView.h>
#include <sbISortableMediaListView.h>
#include <sbLibraryUtils.h>
#include <sbPropertiesCID.h>

#include "sbLocalDatabaseMediaListBase.h"

class nsIURI;
class sbIDatabaseQuery;
class sbIDatabaseResult;
class sbILibraryConstraint;
class sbILocalDatabaseAsyncGUIDArray;
class sbLocalDatabaseLibrary;
class sbLocalDatabaseTreeView;
class sbLocalDatabaseCascadeFilterSet;

class sbLocalDatabaseMediaListView : public nsSupportsWeakReference,
                                     public sbIMediaListView,
                                     public sbIMediaListListener,
                                     public sbIFilterableMediaListView,
                                     public sbISearchableMediaListView,
                                     public sbISortableMediaListView,
                                     public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTVIEW
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBIFILTERABLEMEDIALISTVIEW
  NS_DECL_SBISEARCHABLEMEDIALISTVIEW
  NS_DECL_SBISORTABLEMEDIALISTVIEW
  NS_DECL_NSICLASSINFO

  sbLocalDatabaseMediaListView(sbLocalDatabaseLibrary* aLibrary,
                               sbLocalDatabaseMediaListBase* aMediaList,
                               nsAString& aDefaultSortProperty,
                               PRUint32 aMediaListId);

  ~sbLocalDatabaseMediaListView();

  nsresult Init(sbIMediaListViewState* aState);

  already_AddRefed<sbLocalDatabaseMediaListBase> GetNativeMediaList();

  sbILocalDatabaseGUIDArray* GetGUIDArray();

  nsresult UpdateViewArrayConfiguration(PRBool aClearTreeSelection);

  void NotifyListenersFilterChanged() {
    NotifyListenersInternal(&sbIMediaListViewListener::OnFilterChanged);
  }

  void NotifyListenersSearchChanged() {
    NotifyListenersInternal(&sbIMediaListViewListener::OnSearchChanged);
  }

  void NotifyListenersSortChanged() {
    NotifyListenersInternal(&sbIMediaListViewListener::OnSortChanged);
  }

private:
  typedef nsTArray<nsString> sbStringArray;
  typedef nsCOMArray<sbIPropertyArray> sbPropertyArrayList;
  typedef nsCOMArray<sbIMediaListViewListener> sbViewListenerArray;

  // This makes a typedef called ListenerFunc to any member function of
  // sbIMediaListViewListener that has this signature:
  //   nsresult sbIMediaListViewListener::func(sbIMediaListView*)
  typedef NS_STDCALL_FUNCPROTO(nsresult, ListenerFunc, sbIMediaListViewListener,
                               OnSortChanged, (sbIMediaListView*));

  static PLDHashOperator PR_CALLBACK
    AddValuesToArrayCallback(nsStringHashKey::KeyType aKey,
                             sbStringArray* aEntry,
                             void* aUserData);

  static PLDHashOperator PR_CALLBACK
    AddKeysToStringArrayCallback(nsStringHashKey::KeyType aKey,
                                 sbStringArray* aEntry,
                                 void* aUserData);

  static PLDHashOperator PR_CALLBACK
    AddListenersToCOMArray(nsISupportsHashKey* aEntry,
                           void* aUserData);

  nsresult MakeStandardQuery(sbIDatabaseQuery** _retval);

  nsresult CreateQueries();

  nsresult Invalidate();

  nsresult ClonePropertyArray(sbIPropertyArray* aSource,
                              sbIMutablePropertyArray** _retval);

  nsresult HasCommonProperty(sbIPropertyArray* aBag1,
                             sbIPropertyArray* aBag2,
                             PRBool* aHasCommonProperty);

  nsresult HasCommonProperty(sbIPropertyArray* aBag,
                             sbILibraryConstraint* aConstraint,
                             PRBool* aHasCommonProperty);

  nsresult ShouldCauseInvalidation(sbIPropertyArray* aProperties,
                                   PRBool* aShouldCauseInvalidation);

  nsresult UpdateListener(PRBool aRemoveListener = PR_TRUE);

  void NotifyListenersInternal(ListenerFunc aListenerFunc);

private:
  nsRefPtr<sbLocalDatabaseLibrary> mLibrary;

  // Property Manager
  nsCOMPtr<sbIPropertyManager> mPropMan;

  // The media list this view is of
  nsRefPtr<sbLocalDatabaseMediaListBase> mMediaList;

  // The default sort property of the guid array
  nsString mDefaultSortProperty;

  // Internal id of this media list, or 0 for the full library
  PRUint32 mMediaListId;

  // Database guid and location
  nsString mDatabaseGuid;
  nsCOMPtr<nsIURI> mDatabaseLocation;

  // GUID array for this view instance
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> mArray;

  // Filter set for this view, if any
  nsRefPtr<sbLocalDatabaseCascadeFilterSet> mCascadeFilterSet;

  // Tree view for this view, if any
  nsRefPtr<sbLocalDatabaseTreeView> mTreeView;

  // Curent filter configuration
  nsCOMPtr<sbILibraryConstraint> mViewFilter;

  // Current search configuration
  nsCOMPtr<sbILibraryConstraint> mViewSearch;

  // Current sort configuration
  nsCOMPtr<sbIMutablePropertyArray> mViewSort;

  // Query to return list of values for a given property
  nsString mDistinctPropertyValuesQuery;

  // Whether we're in batch mode.
  sbLibraryBatchHelper mBatchHelper;

  // The lock to protect our listener table.
  PRLock* mListenerTableLock;

  // Our listener table, stores nsISupports entries that are either strong
  // references to sbIMediaListViewListener instances or
  // nsISupportsWeakReference instances (weak references to
  // sbIMediaListViewListener instances).
  nsTHashtable<nsISupportsHashKey> mListenerTable;

  // True when we should invalidate when batching ends
  PRPackedBool mInvalidatePending;

  // If true, changing the sort/search/filter will not update the view array
  // configuration or listener settings
  PRPackedBool mInitializing;
};

class sbMakeSortableStringEnumerator : public nsIStringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  sbMakeSortableStringEnumerator(sbIPropertyInfo* aPropertyInfo,
                                 nsIStringEnumerator* aValues);

private:
  nsCOMPtr<sbIPropertyInfo> mPropertyInfo;
  nsCOMPtr<nsIStringEnumerator> mValues;
};

#endif /* __SB_LOCALDATABASEMEDIALISTVIEW_H__ */
