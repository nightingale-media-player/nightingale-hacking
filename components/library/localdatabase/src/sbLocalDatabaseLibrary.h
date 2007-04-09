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

#include "sbLocalDatabaseMediaListBase.h"
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbIMediaListListener.h>

#include <nsClassHashtable.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <nsIClassInfo.h>
#include <nsStringGlue.h>
#include <sbIMediaListFactory.h>

class nsIURI;
class nsIWeakReference;
class sbAutoBatchHelper;
class sbIDatabaseQuery;
class sbILocalDatabasePropertyCache;
class sbLibraryInsertingEnumerationListener;
class sbLibraryRemovingEnumerationListener;
class sbLocalDatabasePropertyCache;

// These are the methods from sbLocalDatabaseMediaListBase that we're going to
// override in sbLocalDatabaseLibrary. Most of them are from sbIMediaList.
#define SB_DECL_MEDIALISTBASE_OVERRIDES                                         \
  NS_IMETHOD GetItemByGuid(const nsAString& aGuid, sbIMediaItem** _retval);     \
  NS_IMETHOD Contains(sbIMediaItem* aMediaItem, PRBool* _retval);               \
  NS_IMETHOD Add(sbIMediaItem* aMediaItem);                                     \
  NS_IMETHOD AddAll(sbIMediaList* aMediaList);                                  \
  NS_IMETHOD AddSome(nsISimpleEnumerator* aMediaItems);                         \
  NS_IMETHOD InsertBefore(PRUint32 aIndex, sbIMediaItem* aMediaItem);           \
  NS_IMETHOD MoveBefore(PRUint32 aFromIndex, PRUint32 aToIndex);                \
  NS_IMETHOD MoveLast(PRUint32 aIndex);                                         \
  NS_IMETHOD Remove(sbIMediaItem* aMediaItem);                                  \
  NS_IMETHOD RemoveByIndex(PRUint32 aIndex);                                    \
  NS_IMETHOD RemoveSome(nsISimpleEnumerator* aMediaItems);                      \
  NS_IMETHOD Clear();                                                           \
  NS_IMETHOD CreateView(sbIMediaListView** _retval);                            \
  /* nothing */

class sbLocalDatabaseLibrary : public sbLocalDatabaseMediaListBase,
                               public sbILibrary,
                               public sbILocalDatabaseLibrary
{
  friend class sbAutoBatchHelper;
  friend class sbLibraryInsertingEnumerationListener;
  friend class sbLibraryRemovingEnumerationListener;
  friend class sbLocalDatabasePropertyCache;

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

  typedef nsClassHashtable<nsStringHashKey, sbMediaListFactoryInfo>
          sbMediaListFactoryInfoTable;

  typedef nsInterfaceHashtable<nsStringHashKey, nsIWeakReference>
          sbMediaItemTable;

  typedef nsDataHashtable<nsStringHashKey, nsString>
          sbGUIDToTypesMap;

  typedef nsDataHashtable<nsStringHashKey, PRUint32>
          sbGUIDToIDMap;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_SBILIBRARY
  NS_DECL_SBILOCALDATABASELIBRARY
  NS_DECL_NSICLASSINFO
  NS_FORWARD_SBILIBRARYRESOURCE(sbLocalDatabaseMediaListBase::)

  // Include our overrides.
  SB_DECL_MEDIALISTBASE_OVERRIDES

  sbLocalDatabaseLibrary();
  ~sbLocalDatabaseLibrary();

  NS_IMETHOD GetDefaultSortProperty(nsAString& aProperty);

  nsresult Init(const nsAString& aDatabaseGuid,
                nsIURI* aDatabaseLocation = nsnull);

private:
  nsresult CreateQueries();

  inline nsresult MakeStandardQuery(sbIDatabaseQuery** _retval);

  inline void GetNowString(nsAString& _retval);

  nsresult CreateNewItemInDatabase(const PRUint32 aMediaItemTypeID,
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

  nsresult RegisterDefaultMediaListFactories();

  nsresult DeleteDatabaseItem(const nsAString& aGuid);

  nsresult AddItemToLocalDatabase(sbIMediaItem* aMediaItem);

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

  sbMediaListFactoryInfoTable mMediaListFactoryTable;

  sbMediaItemTable mMediaItemTable;

  sbGUIDToTypesMap mCachedTypeTable;

  sbGUIDToIDMap mCachedIDTable;
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
  PRPackedBool mItemEnumerated;
};

/**
 * \class sbAutoBatchHelper
 *
 * \brief Simple class to make sure we notify listeners that a batch operation
 *        has completed every time they are notified that a batch operation
 *        has begun.
 */
class sbAutoBatchHelper
{
public:
  sbAutoBatchHelper(sbLocalDatabaseLibrary* aLibrary)
  : mLibrary(aLibrary)
  {
    NS_ASSERTION(mLibrary, "Null pointer!");
    mLibrary->NotifyListenersBatchBegin(mLibrary);
  }

  ~sbAutoBatchHelper()
  {
    mLibrary->NotifyListenersBatchEnd(mLibrary);
  }

private:
  sbLocalDatabaseLibrary* mLibrary;
};

#endif /* __SBLOCALDATABASELIBRARY_H__ */
