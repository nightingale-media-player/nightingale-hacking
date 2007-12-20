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

#ifndef __SBLOCALDATABASECASCADEFILTERSET_H__
#define __SBLOCALDATABASECASCADEFILTERSET_H__

#include "sbLocalDatabaseTreeView.h"

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsCOMArray.h>
#include <nsHashKeys.h>
#include <nsISerializable.h>
#include <nsIStringEnumerator.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsTHashtable.h>
#include <nsWeakReference.h>

#include <sbICascadeFilterSet.h>
#include <sbILocalDatabaseAsyncGUIDArray.h>
#include <sbIMediaListListener.h>
#include <sbLibraryUtils.h>

class nsIArray;
class nsIMutableArray;
class nsIObjectInputStream;
class nsIObjectOutputStream;
class sbLocalDatabaseLibrary;
class sbLocalDatabaseMediaListView;
class sbLocalDatabaseTreeView;
class sbLocalDatabaseTreeViewState;
class sbILibraryConstraintBuilder;
class sbILocalDatabaseAsyncGUIDArray;
class sbILocalDatabaseGUIDArray;
class sbILocalDatabaseLibrary;
class sbIMediaListView;
class sbIMutablePropertyArray;
class sbLocalDatabaseCascadeFilterSetArrayListener;

class sbLocalDatabaseCascadeFilterSetState;

class sbLocalDatabaseCascadeFilterSet : public nsSupportsWeakReference,
                                        public sbICascadeFilterSet,
                                        public sbIMediaListListener
{
  typedef nsTArray<nsString> sbStringArray;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICASCADEFILTERSET
  NS_DECL_SBIMEDIALISTLISTENER

  sbLocalDatabaseCascadeFilterSet(sbLocalDatabaseMediaListView* aMediaListView);
  ~sbLocalDatabaseCascadeFilterSet();

  nsresult Init(sbLocalDatabaseLibrary* aLibrary,
                sbILocalDatabaseAsyncGUIDArray* aProtoArray,
                sbLocalDatabaseCascadeFilterSetState* aState);

  nsresult AddConfiguration(sbILocalDatabaseGUIDArray* aArray);

  nsresult AddFilters(sbILibraryConstraintBuilder* aBuilder,
                      PRBool* aChanged);

  nsresult AddSearches(sbILibraryConstraintBuilder* aBuilder,
                       PRBool* aChanged);

  nsresult ClearFilters();

  nsresult ClearSearches();

  void ClearMediaListView();

  nsresult OnGetLength(PRUint32 aIndex, PRUint32 aLength);

  nsresult GetState(sbLocalDatabaseCascadeFilterSetState** aState);

private:
  struct sbFilterSpec {
    sbFilterSpec() :
      isSearch(PR_FALSE),
      cachedValueCount(0),
      invalidationPending(PR_FALSE)
    {
    }

    PRBool isSearch;
    nsString property;
    sbStringArray propertyList;
    sbStringArray values;
    nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> array;
    nsRefPtr<sbLocalDatabaseTreeView> treeView;
    nsRefPtr<sbLocalDatabaseCascadeFilterSetArrayListener> arrayListener;
    PRUint32 cachedValueCount;
    PRBool invalidationPending;
  };

  nsresult ConfigureArray(PRUint32 aIndex);

  nsresult ConfigureFilterArray(sbFilterSpec* aSpec,
                                const nsAString& aSortProperty);

  nsresult InvalidateFilter(sbFilterSpec& aFilter);

  nsresult UpdateListener(PRBool aRemoveListener = PR_TRUE);

  // This callback is meant to be used with mListeners.
  // aUserData should be a sbICascadeFilterSetListener pointer.
  static PLDHashOperator PR_CALLBACK
    OnValuesChangedCallback(nsISupportsHashKey* aKey,
                            void* aUserData);

  // The library this filter set is associated with
  nsRefPtr<sbLocalDatabaseLibrary> mLibrary;

  // The media list view this filter set is associated with.  This pointer
  // will be set to null when the associated view gets deleted
  sbLocalDatabaseMediaListView* mMediaListView;

  // The media list we listen to for changes
  nsCOMPtr<sbIMediaList> mMediaList;

  // Prototypical array that is cloned to provide each filter's data
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> mProtoArray;

  // Current filter configuration
  nsTArray<sbFilterSpec> mFilters;

  // Change listeners
  nsTHashtable<nsISupportsHashKey> mListeners;

  // Is our media list in a batch
  sbLibraryBatchHelper mBatchHelper;
};

class sbLocalDatabaseCascadeFilterSetArrayListener :
                                  public nsSupportsWeakReference,
                                  public sbILocalDatabaseAsyncGUIDArrayListener
{
  friend class sbLocalDatabaseCascadeFilterSet;

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEASYNCGUIDARRAYLISTENER

  nsresult Init(sbLocalDatabaseCascadeFilterSet *aCascadeFilterSet);

private:
  nsCOMPtr<nsIWeakReference> mWeakCascadeFilterSet;
  sbLocalDatabaseCascadeFilterSet* mCascadeFilterSet;
  PRUint32 mIndex;
};

class sbGUIDArrayPrimarySortEnumerator : public nsIStringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  sbGUIDArrayPrimarySortEnumerator(sbILocalDatabaseAsyncGUIDArray* aArray);

private:
  nsCOMPtr<sbILocalDatabaseAsyncGUIDArray> mArray;
  PRUint32 mNextIndex;
};

class sbLocalDatabaseCascadeFilterSetState : public nsISerializable
{
friend class sbLocalDatabaseCascadeFilterSet;
  typedef nsTArray<nsString> sbStringArray;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISERIALIZABLE

  nsresult ToString(nsAString& aStr);

  struct Spec {
    PRBool isSearch;
    nsString property;
    sbStringArray propertyList;
    sbStringArray values;
    nsRefPtr<sbLocalDatabaseTreeViewState> treeViewState;
  };

protected:
  nsTArray<Spec> mFilters;
};

#endif /* __SBLOCALDATABASECASCADEFILTERSET_H__ */

