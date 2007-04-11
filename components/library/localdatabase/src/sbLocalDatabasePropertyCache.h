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
#include <nsCOMPtr.h>
#include <nsDataHashtable.h>
#include <nsInterfaceHashtable.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsTHashtable.h>

struct PRLock;

class nsIURI;
class sbIDatabaseQuery;
class sbLocalDatabaseLibrary;
class sbISQLBuilderCriterionIn;
class sbISQLInsertBuilder;
class sbISQLSelectBuilder;
class sbISQLUpdateBuilder;

class sbLocalDatabasePropertyCache : public sbILocalDatabasePropertyCache
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEPROPERTYCACHE

  sbLocalDatabasePropertyCache();

  // This is public so that it can be used with nsAutoPtr and friends.
  ~sbLocalDatabasePropertyCache();

  nsresult Init(sbLocalDatabaseLibrary* aLibrary);

  nsresult MakeQuery(const nsAString& aSql, sbIDatabaseQuery** _retval);
  nsresult LoadProperties();

  nsresult AddDirtyGUID(const nsAString &aGuid);

  PRUint32 GetPropertyID(const nsAString& aPropertyName);
  PRBool GetPropertyName(PRUint32 aPropertyID, nsAString& aPropertyName);

  PRBool IsTopLevelProperty(PRUint32 aPropertyID);
  nsresult PropertyRequiresInsert(const nsAString &aGuid, PRUint32 aPropertyID, PRBool *aInsert);
  void   GetColumnForPropertyID(PRUint32 aPropertyID, nsAString &aColumn); 

private:
  PRBool mWritePending;

  PRUint32 mNumStaticProperties;
 
  // Database GUID
  nsString mDatabaseGUID;

  // Database Location
  nsCOMPtr<nsIURI> mDatabaseLocation;

  // Cache the property name list
  nsClassHashtable<nsUint32HashKey, nsString> mPropertyIDToName;
  nsDataHashtable<nsStringHashKey, PRUint32> mPropertyNameToID;

  // Used to template the properties select statement
  nsCOMPtr<sbISQLSelectBuilder> mPropertiesSelect;
  nsCOMPtr<sbISQLBuilderCriterionIn> mPropertiesInCriterion;

  // Used to template the media items select statement
  nsCOMPtr<sbISQLSelectBuilder> mMediaItemsSelect;
  nsCOMPtr<sbISQLBuilderCriterionIn> mMediaItemsInCriterion;

  // Used to template the properties insert statement
  nsCOMPtr<sbISQLInsertBuilder> mPropertiesInsert;

  //Used to template the properties update statement
  nsCOMPtr<sbISQLUpdateBuilder> mPropertiesUpdate;

  // Used to template the media item property update statement
  nsCOMPtr<sbISQLUpdateBuilder> mMediaItemsUpdate;

  // Used to template the query used to verify if we need to insert or
  // update a perticular property.
  nsCOMPtr<sbISQLSelectBuilder> mPropertyInsertSelect;

  // Cache for GUID -> property bag
  nsInterfaceHashtableMT<nsStringHashKey, sbILocalDatabaseResourcePropertyBag> mCache;

  // Dirty GUID's
  PRLock* mDirtyLock;
  nsTHashtable<nsStringHashKey> mDirty;

  // Backstage pass to our parent library. Can't use an nsRefPtr because the
  // library owns us and that would create a cycle.
  sbLocalDatabaseLibrary* mLibrary;
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

  PRBool    mWritePending;
  nsString  mGuid;

  // Dirty Property ID's
  PRLock* mDirtyLock;
  nsTHashtable<nsUint32HashKey> mDirty;
};

#endif /* __SBLOCALDATABASEPROPERTYCACHE_H__ */
