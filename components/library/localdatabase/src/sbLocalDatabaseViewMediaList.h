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

#ifndef __SBLOCALDATABASEVIEWMEDIALIST_H__
#define __SBLOCALDATABASEVIEWMEDIALIST_H__

#include "sbLocalDatabaseMediaListBase.h"
#include <sbIMediaListListener.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>
#include <prlock.h>

class sbIMediaItem;
class sbIMediaList;
class sbILibrary;
class sbILocalDatabaseLibrary;

class sbLocalDatabaseViewMediaList : public sbLocalDatabaseMediaListBase
{
public:
  friend class sbViewMediaListEnumerationListener;

  NS_DECL_ISUPPORTS_INHERITED

  sbLocalDatabaseViewMediaList() { };

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
  nsresult DeleteItemByMediaItemId(PRUint32 aMediaItemId);

  nsresult CreateQueries();

  nsresult AddItemToLocalDatabase(sbIMediaItem* aMediaItem);

private:
  // Query to delete a single item from the view
  nsString mDeleteItemQuery;

  // Query to clear the entire list
  nsString mDeleteAllQuery;
};

class sbViewMediaListEnumerationListener : public sbIMediaListEnumerationListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTENUMERATIONLISTENER

  sbViewMediaListEnumerationListener(sbLocalDatabaseViewMediaList* aList)
  : mFriendList(aList),
    mShouldInvalidate(PR_FALSE)
  {
    NS_ASSERTION(mFriendList, "Null pointer!");
  }

private:
  sbLocalDatabaseViewMediaList* mFriendList;
  PRBool mShouldInvalidate;
};

#endif /* __SBLOCALDATABASEVIEWMEDIALIST_H__ */
