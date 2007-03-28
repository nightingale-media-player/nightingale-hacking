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

#include <sbLocalDatabaseResourceProperty.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibrary.h>

#include <nsClassHashtable.h>
#include <nsCOMPtr.h>
#include <nsDataHashTable.h>
#include <nsHashKeys.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>
#include <sbIMediaListFactory.h>

class nsIURI;
class sbIDatabaseQuery;
class sbILocalDatabasePropertyCache;

class sbLocalDatabaseLibrary : public sbLocalDatabaseResourceProperty,
                               public sbILibrary,
                               public sbILocalDatabaseLibrary
{
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

  typedef nsInterfaceHashtable<nsStringHashKey, sbIMediaItem>
          sbMediaItemTable;

  typedef nsDataHashtable<nsStringHashKey, nsString>
          sbGUIDToTypesMap;

  typedef nsDataHashtable<nsStringHashKey, PRUint32>
          sbGUIDToIDMap;

public:
  NS_DECL_ISUPPORTS_INHERITED

  // When using inheritence, you must forward all interfaces implemented
  // by the base class, else you will get "pure virtual function was not
  // defined" style errors.
  NS_FORWARD_SBILOCALDATABASERESOURCEPROPERTY(sbLocalDatabaseResourceProperty::)
  NS_FORWARD_SBILIBRARYRESOURCE(sbLocalDatabaseResourceProperty::)

  NS_DECL_SBILIBRARY
  NS_DECL_SBILOCALDATABASELIBRARY

  // This constructor assumes the database file lives in the 'ProfD/db'
  // directory.
  sbLocalDatabaseLibrary(const nsAString& aDatabaseGuid);

  // Use this constructor to specify a location for the database file.
  sbLocalDatabaseLibrary(nsIURI* aDatabaseLocation,
                         const nsAString& aDatabaseGuid);

private:
  nsresult CreateQueries();

  inline NS_METHOD MakeStandardQuery(sbIDatabaseQuery** _retval);

  inline void GetNowString(nsAString& _retval);

  NS_METHOD CreateNewItemInDatabase(const PRUint32 aMediaItemTypeID,
                                    const nsAString& aURISpecOrPrefix,
                                    nsAString& _retval);

  //NS_METHOD LoadRegisteredMediaListFactories();

  NS_METHOD GetTypeForGUID(const nsAString& aGUID,
                           nsAString& _retval);

  // This callback is meant to be used with mMediaListFactoryTable.
  // aUserData should be a nsTArray<nsString> pointer.
  static PLDHashOperator PR_CALLBACK
    AddTypesToArrayCallback(nsStringHashKey::KeyType aKey,
                            sbMediaListFactoryInfo* aEntry,
                            void* aUserData);

  NS_METHOD RegisterDefaultMediaListFactories();

private:

  nsString mDatabaseGuid;
  nsCOMPtr<nsIURI> mDatabaseLocation;

  nsCOMPtr<sbILocalDatabasePropertyCache> mPropertyCache;

  nsString mGetTypeForGUIDQuery;
  nsString mGetMediaItemIdForGUIDQuery;
  nsString mInsertMediaItemQuery;
  nsString mMediaListFactoriesQuery;
  nsString mInsertMediaListFactoryQuery;

  // This library's resource guid
  nsString mGuid;

  sbMediaListFactoryInfoTable mMediaListFactoryTable;

  sbMediaItemTable mMediaItemTable;

  sbGUIDToTypesMap mCachedTypeTable;

  sbGUIDToIDMap mCachedIDTable;
};

#endif /* __SBLOCALDATABASELIBRARY_H__ */
