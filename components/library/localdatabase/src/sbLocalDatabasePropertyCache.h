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

#ifndef __SBLOCALDATABASEPROPERTYCACHE_H__
#define __SBLOCALDATABASEPROPERTYCACHE_H__

#include <nsIStringEnumerator.h>
#include <sbILocalDatabasePropertyCache.h>

#include <nsClassHashtable.h>
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsInterfaceHashtable.h>
#include <nsIObserver.h>
#include <nsIThread.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsTHashtable.h>

struct PRLock;
struct PRMonitor;

class nsIURI;
class sbIDatabaseQuery;
class sbLocalDatabaseLibrary;
class sbIPropertyManager;
class sbISQLBuilderCriterionIn;
class sbISQLInsertBuilder;
class sbISQLSelectBuilder;
class sbISQLUpdateBuilder;

class sbLocalDatabasePropertyCache : public sbILocalDatabasePropertyCache,
                                     public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEPROPERTYCACHE
  NS_DECL_NSIOBSERVER

  sbLocalDatabasePropertyCache();

  // This is public so that it can be used with nsAutoPtr and friends.
  ~sbLocalDatabasePropertyCache();

  nsresult Init(sbLocalDatabaseLibrary* aLibrary,
                const nsAString& aLibraryResourceGUID);
  nsresult Shutdown();

  nsresult MakeQuery(const nsAString& aSql, sbIDatabaseQuery** _retval);
  nsresult LoadProperties();

  nsresult AddDirtyGUID(const nsAString &aGuid);

  PRUint32 GetPropertyDBIDInternal(const nsAString& aPropertyID);
  PRBool GetPropertyID(PRUint32 aPropertyDBID, nsAString& aPropertyID);

  void GetColumnForPropertyID(PRUint32 aPropertyID, nsAString &aColumn);

  nsresult InsertPropertyIDInLibrary(const nsAString& aPropertyID,
                                     PRUint32 *aPropertyDBID);

private:
  // Pending write count.
  PRUint32 mWritePendingCount;

  // Database GUID
  nsString mDatabaseGUID;

  // Database Location
  nsCOMPtr<nsIURI> mDatabaseLocation;

  // Cache the property name list
  nsDataHashtableMT<nsUint32HashKey, nsString> mPropertyDBIDToID;
  nsDataHashtableMT<nsStringHashKey, PRUint32> mPropertyIDToDBID;

  // Used to template the properties select statement
  nsCOMPtr<sbISQLSelectBuilder> mPropertiesSelect;
  // mPropertiesSelect has an owning reference to this
  sbISQLBuilderCriterionIn* mPropertiesInCriterion;

  // Used to template the media items select statement
  nsCOMPtr<sbISQLSelectBuilder> mMediaItemsSelect;
  // mMediaItemsSelect has an owning reference to this
  sbISQLBuilderCriterionIn* mMediaItemsInCriterion;

  // Used to template the properties table insert that generates
  // the property ID for the property in the current library.
  nsCOMPtr<sbISQLInsertBuilder> mPropertiesTableInsert;

  // Property insert or replace statement
  nsString mPropertiesInsertOrReplace;

  // Property delete query
  nsString mPropertiesDelete;

  // Used to template the media item property update statement
  nsDataHashtable<nsUint32HashKey, nsString> mMediaItemsUpdateQueries;

  // Used to template the library media item property update statement
  nsDataHashtable<nsUint32HashKey, nsString> mLibraryMediaItemUpdateQueries;

  // Used to template the query used to verify if we need to insert or
  // update a peculiar property.
  nsCOMPtr<sbISQLSelectBuilder> mPropertyInsertSelect;

  // Cache for GUID -> property bag
  nsInterfaceHashtableMT<nsStringHashKey, sbILocalDatabaseResourcePropertyBag> mCache;

  // Dirty GUIDs
  PRLock* mDirtyLock;
  nsTHashtable<nsStringHashKey> mDirty;

  // flushing on a background thread
  struct FlushQueryData {
    nsCOMPtr<sbIDatabaseQuery> query;
    PRUint32 dirtyGuidCount;
  };
  void RunFlushThread();

  // The GUID of the library resource
  nsString mLibraryResourceGUID;

  PRBool mIsShuttingDown;
  nsTArray<FlushQueryData> mUnflushedQueries;
  nsCOMPtr<nsIThread> mFlushThread;
  PRMonitor* mFlushThreadMonitor;

  // Backstage pass to our parent library. Can't use an nsRefPtr because the
  // library owns us and that would create a cycle.
  sbLocalDatabaseLibrary* mLibrary;

  nsCOMPtr<sbIPropertyManager> mPropertyManager;
};

class sbLocalDatabaseResourcePropertyBag : public sbILocalDatabaseResourcePropertyBag
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASERESOURCEPROPERTYBAG

  sbLocalDatabaseResourcePropertyBag(sbLocalDatabasePropertyCache* aCache,
                                     const nsAString& aGuid);

  ~sbLocalDatabaseResourcePropertyBag();

  nsresult Init();
  nsresult PutValue(PRUint32 aPropertyID, const nsAString& aValue);

  nsresult EnumerateDirty(nsTHashtable<nsUint32HashKey>::Enumerator aEnumFunc, void *aClosure, PRUint32 *aDirtyCount);
  nsresult SetDirty(PRBool aDirty);

private:

  sbLocalDatabasePropertyCache* mCache;
  nsClassHashtableMT<nsUint32HashKey, nsString> mValueMap;

  nsCOMPtr<sbIPropertyManager> mPropertyManager;

  PRUint32  mWritePendingCount;
  nsString  mGuid;

  // Dirty Property ID's
  PRLock* mDirtyLock;
  nsTHashtable<nsUint32HashKey> mDirty;
};

#endif /* __SBLOCALDATABASEPROPERTYCACHE_H__ */
