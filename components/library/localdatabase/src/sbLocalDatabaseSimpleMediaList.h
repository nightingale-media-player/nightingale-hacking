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

#ifndef __SBLOCALDATABASESIMPLEMEDIALIST_H__
#define __SBLOCALDATABASESIMPLEMEDIALIST_H__

#include "sbLocalDatabaseMediaListBase.h"
#include <sbILocalDatabaseSimpleMediaList.h>
#include <sbIMediaListListener.h>

#include <nsStringGlue.h>
#include <prlock.h>

class nsISimpleEnumerator;
class sbILocalDatabaseLibrary;
class sbIMediaItem;
class sbIMediaList;
class sbIMediaListView;
class sbIDatabaseQuery;
class sbSimpleMediaListInsertingEnumerationListener;
class sbSimpleMediaListRemovingEnumerationListener;

class sbLocalDatabaseSimpleMediaList : public sbLocalDatabaseMediaListBase,
                                       public sbILocalDatabaseSimpleMediaList,
                                       public sbIMediaListListener
{
public:
  friend class sbSimpleMediaListInsertingEnumerationListener;
  friend class sbSimpleMediaListRemovingEnumerationListener;

  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBIMEDIALISTLISTENER
  NS_DECL_SBILOCALDATABASESIMPLEMEDIALIST

  sbLocalDatabaseSimpleMediaList()
  : mBatchRemoveListener(nsnull)
  { }

  nsresult Init(sbILocalDatabaseLibrary* aLibrary,
                const nsAString& aGuid);

  // override base class
  NS_IMETHOD GetItemByGuid(const nsAString& aGuid, sbIMediaItem** _retval);
  NS_IMETHOD Contains(sbIMediaItem* aMediaItem, PRBool* _retval);
  NS_IMETHOD Add(sbIMediaItem *aMediaItem);
  NS_IMETHOD AddAll(sbIMediaList *aMediaList);
  NS_IMETHOD AddSome(nsISimpleEnumerator *aMediaItems);
  NS_IMETHOD InsertBefore(PRUint32 aIndex, sbIMediaItem* aMediaItem);
  NS_IMETHOD MoveBefore(PRUint32 aFromIndex, PRUint32 aToIndex);
  NS_IMETHOD MoveLast(PRUint32 aIndex);
  NS_IMETHOD Remove(sbIMediaItem* aMediaItem);
  NS_IMETHOD RemoveByIndex(PRUint32 aIndex);
  NS_IMETHOD RemoveSome(nsISimpleEnumerator* aMediaItems);
  NS_IMETHOD Clear();
  NS_IMETHOD CreateView(sbIMediaListView** _retval);

  NS_IMETHOD GetDefaultSortProperty(nsAString& aProperty);

private:
  nsresult ExecuteAggregateQuery(const nsAString& aQuery, nsAString& aValue);

  nsresult UpdateOrdinalByIndex(PRUint32 aIndex, const nsAString& aOrdinal);

  nsresult DeleteItemByMediaItemId(PRUint32 aMediaItemId);

  nsresult GetNextOrdinal(nsAString& aValue);

  nsresult GetBeforeOrdinal(PRUint32 aIndex, nsAString& aValue);

  nsresult AddToLastPathSegment(nsAString& aPath, PRInt32 aToAdd);

  PRUint32 CountLevels(const nsAString& aPath);

  nsresult CreateQueries();

private:
  // Query to get the media item id for a given guid, constrained to the
  // items within this simple media list
  nsString mGetMediaItemIdForGuidQuery;

  // Query to insert a new item into this list
  nsString mInsertIntoListQuery;

  // Query to update the ordinal of a media item in this list
  nsString mUpdateListItemOrdinalQuery;

  // Query to get the guid of an item on this list by its ordinal
  nsString mGetMediaItemGuidForOrdinalQuery;

  // Query to delete an item from the list
  nsString mDeleteFirstListItemQuery;

  // Query to delete the entire contents of the list
  nsString mDeleteAllQuery;

  // Query to delete an item with a given ordinal
  nsString mDeleteListItemByOrdinalQuery;

  // Get last ordinal
  nsString mGetLastOrdinalQuery;

  // Get first ordinal
  nsString mGetFirstOrdinalQuery;

  sbSimpleMediaListRemovingEnumerationListener* mBatchRemoveListener;
};

class sbSimpleMediaListInsertingEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbSimpleMediaListInsertingEnumerationListener(sbLocalDatabaseSimpleMediaList* aList)
  : mFriendList(aList),
    mItemEnumerated(PR_FALSE)
  {
    NS_ASSERTION(mFriendList, "Null pointer!");
  }

private:
  sbLocalDatabaseSimpleMediaList* mFriendList;
  nsCOMPtr<sbIDatabaseQuery> mDBQuery;
  nsString mOrdinal;
  PRBool mItemEnumerated;
};

class sbSimpleMediaListRemovingEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbSimpleMediaListRemovingEnumerationListener(sbLocalDatabaseSimpleMediaList* aList)
  : mFriendList(aList),
    mItemEnumerated(PR_FALSE)
  {
    NS_ASSERTION(mFriendList, "Null pointer!");
  }

private:
  sbLocalDatabaseSimpleMediaList* mFriendList;
  nsCOMPtr<sbIDatabaseQuery> mDBQuery;
  PRBool mItemEnumerated;
};

#endif /* __SBLOCALDATABASESIMPLEMEDIALIST_H__ */
