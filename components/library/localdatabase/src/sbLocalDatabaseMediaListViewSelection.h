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

#ifndef __SB_LOCALDATABASEMEDIALISTVIEWSELECTION_H__
#define __SB_LOCALDATABASEMEDIALISTVIEWSELECTION_H__

#include "sbIMediaListViewSelection.h"
#include "sbSelectionListUtils.h"

#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsIClassInfo.h>
#include <nsISimpleEnumerator.h>
#include <nsISerializable.h>
#include <nsITimer.h>
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

  nsresult GetUniqueIdForIndex(PRInt32 aIndex, nsAString& aId);

  nsresult GetIndexForUniqueId(const nsAString& aId,
                               PRUint32*        aIndex);

  static void DelayedSelectNotification(nsITimer* aTimer, void* aClosure);

  nsresult AddToSelection(PRUint32 aIndex);
  nsresult RemoveFromSelection(PRUint32 aIndex);
  inline void CheckSelectAll() {
    if (mLength > 1)
      mSelectionIsAll = (mSelection.Count() == mLength);
    else
      mSelectionIsAll = PR_FALSE;

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
  nsString mCurrentUID;

  nsCOMPtr<sbILibrary> mLibrary;
  nsString mListGUID;

  nsCOMPtr<nsITimer> mSelectTimer;

  // A weak reference to the view's array.  We're owned by the view so this
  // will never go away
  sbILocalDatabaseGUIDArray* mArray;
  PRBool mIsLibrary;

  PRUint32 mLength;
  PRBool mSelectionNotificationsSuppressed;
};

/**
 * \brief Use scope to manage the notifications suppressed flag on the
 *        view selection so it does not accidentally remain set.
 */
class sbAutoSelectNotificationsSuppressed
{
public:
  sbAutoSelectNotificationsSuppressed(sbIMediaListViewSelection* aSelection) :
    mSelection(aSelection)
  {
    NS_ASSERTION(aSelection, "aSelection is null");
#ifdef DEBUG
    nsresult rv =
#endif
    mSelection->SetSelectionNotificationsSuppressed(PR_TRUE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to set");
  }

  ~sbAutoSelectNotificationsSuppressed()
  {
#ifdef DEBUG
    nsresult rv =
#endif
    mSelection->SetSelectionNotificationsSuppressed(PR_FALSE);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to unset");
  }

private:
  sbIMediaListViewSelection* mSelection;
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
  PRInt32 mCurrentIndex;
  sbSelectionList mSelectionList;
  PRBool mSelectionIsAll;
};

class sbGUIDArrayToIndexedMediaItemEnumerator : public nsISimpleEnumerator,
                                                public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR
  NS_DECL_NSICLASSINFO

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

class sbIndexedGUIDArrayEnumerator : public nsISimpleEnumerator,
                                     public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR
  NS_DECL_NSICLASSINFO

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

/**
 * Helper class to convert from a nsISimpleEnumerator<sbIIndexedMediaItem>
 * to a nsISimpleEnumerator<sbIMediaItem>
 */
class sbIndexedToUnindexedMediaItemEnumerator : public nsISimpleEnumerator,
                                                public nsIClassInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLEENUMERATOR
  NS_DECL_NSICLASSINFO
  
  sbIndexedToUnindexedMediaItemEnumerator(nsISimpleEnumerator* aEnumerator);

private:
  nsCOMPtr<nsISimpleEnumerator> mIndexedEnumerator;
};

#endif /* __SB_LOCALDATABASEMEDIALISTVIEWSELECTION_H__ */
