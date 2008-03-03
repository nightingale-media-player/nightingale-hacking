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

#ifndef __SB_LOCALDATABASEMEDIALISTVIEWSELECTION_H__
#define __SB_LOCALDATABASEMEDIALISTVIEWSELECTION_H__

#include "sbIMediaListViewSelection.h"
#include "sbSelectionListUtils.h"

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsISimpleEnumerator.h>
#include <nsISerializable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsTObserverArray.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseGUIDArray.h>

class sbLocalDatabaseMediaListViewSelectionState;

class sbLocalDatabaseMediaListViewSelection : public sbIMediaListViewSelection
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTVIEWSELECTION

  sbLocalDatabaseMediaListViewSelection();

  nsresult Init(sbILibrary* aLibrary,
                const nsAString& aListGUID,
                sbILocalDatabaseGUIDArray* aArray,
                PRBool aIsLibrary,
                sbLocalDatabaseMediaListViewSelectionState* aState);

  nsresult ConfigurationChanged();

  nsresult GetState(sbLocalDatabaseMediaListViewSelectionState** aState);

private:
  typedef nsresult (*PR_CALLBACK sbSelectionEnumeratorCallbackFunc)
    (PRUint32 aIndex, const nsAString& aId, const nsAString& aGuid, void* aUserData);

  nsresult GetUniqueIdForIndex(PRUint32 aIndex, nsAString& aId);

  nsresult AddToSelection(PRUint32 aIndex);
  nsresult RemoveFromSelection(PRUint32 aIndex);
  inline void CheckSelectAll() {
    mSelectionIsAll = mSelection.Count() == mLength;
    if (mSelectionIsAll) {
      mSelection.Clear();
    }
  }

#ifdef DEBUG
  void LogSelection();
#endif

  typedef nsTObserverArray<nsCOMPtr<sbIMediaListViewSelectionListener> > sbObserverArray;
  sbObserverArray mObservers;

  sbSelectionList mSelection;
  PRBool mSelectionIsAll;
  PRInt32 mCurrentIndex;
  PRInt32 mShiftSelectPivot;

  nsCOMPtr<sbILibrary> mLibrary;
  nsString mListGUID;

  // A weak reference to the view's array.  We're owned by the view so this
  // will never go away
  sbILocalDatabaseGUIDArray* mArray;
  PRBool mIsLibrary;

  PRUint32 mLength;
  PRBool mSelectionNotificationsSuppressed;
};

class sbLocalDatabaseMediaListViewSelectionState : public nsISerializable
{
  friend class sbLocalDatabaseMediaListViewSelection;
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISERIALIZABLE

  nsresult Init();
  nsresult ToString(nsAString& aStr);

  sbLocalDatabaseMediaListViewSelectionState();

protected:
  PRInt32 mShiftSelectPivot;
  PRInt32 mCurrentIndex;
  sbSelectionList mSelectionList;
  PRBool mSelectionIsAll;
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

#endif /* __SB_LOCALDATABASEMEDIALISTVIEWSELECTION_H__ */
