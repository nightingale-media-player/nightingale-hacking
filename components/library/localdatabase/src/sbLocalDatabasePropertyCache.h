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

#include "sbILocalDatabasePropertyCache.h"

#include <nsStringGlue.h>
#include <nsCOMPtr.h>
#include <nsTArray.h>
#include <nsComponentManagerUtils.h>
#include <sbIDatabaseQuery.h>
#include <nsDataHashtable.h>
#include <nsClassHashtable.h>
#include <nsInterfaceHashtable.h>
#include <nsTHashtable.h>
#include <sbISQLBuilder.h>
#include <nsIStringEnumerator.h>

class nsIURI;
class PRLock;

class sbLocalDatabasePropertyCache : public sbILocalDatabasePropertyCache
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEPROPERTYCACHE

  sbLocalDatabasePropertyCache();

  NS_IMETHOD Init();
  NS_IMETHOD MakeQuery(const nsAString& aSql, sbIDatabaseQuery** _retval);
  NS_IMETHOD LoadProperties();

  NS_IMETHOD AddDirtyGUID(const nsAString &aGuid);
  
  PRUint32 GetPropertyID(const nsAString& aPropertyName);
  PRBool GetPropertyName(PRUint32 aPropertyID, nsAString& aPropertyName);

  PRBool IsTopLevelProperty(PRUint32 aPropertyID);
  NS_IMETHOD PropertyRequiresInsert(const nsAString &aGuid, PRUint32 aPropertyID, PRBool *aInsert);
  void   GetColumnForPropertyID(PRUint32 aPropertyID, nsAString &aColumn); 

private:
  ~sbLocalDatabasePropertyCache();

  PRBool mInitialized;
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
  nsTHashtable<nsStringHashKey> mDirty;
};

class sbLocalDatabaseResourcePropertyBag : public sbILocalDatabaseResourcePropertyBag
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASERESOURCEPROPERTYBAG

  sbLocalDatabaseResourcePropertyBag(sbLocalDatabasePropertyCache* aCache,
                                     const nsAString& aGuid);

  NS_IMETHOD Init();
  NS_IMETHOD PutValue(PRUint32 aPropertyID, const nsAString& aValue);

  NS_IMETHOD EnumerateDirty(nsTHashtable<nsUint32HashKey>::Enumerator aEnumFunc, void *aClosure, PRUint32 *aDirtyCount);
  NS_IMETHOD SetDirty(PRBool aDirty);

private:
  ~sbLocalDatabaseResourcePropertyBag();

  sbLocalDatabasePropertyCache* mCache;
  nsClassHashtableMT<nsUint32HashKey, nsString> mValueMap;

  PRBool    mWritePending;
  nsString  mGuid;

  // Dirty Property ID's
  nsTHashtable<nsUint32HashKey> mDirty;

  PRLock* mLock;
};

class sbTArrayStringEnumerator : public nsIStringEnumerator
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISTRINGENUMERATOR

  sbTArrayStringEnumerator(nsTArray<nsString>* aStringArray);

private:

  nsTArray<nsString> mStringArray;
  PRUint32 mNextIndex;
};

#endif /* __SBLOCALDATABASEPROPERTYCACHE_H__ */

