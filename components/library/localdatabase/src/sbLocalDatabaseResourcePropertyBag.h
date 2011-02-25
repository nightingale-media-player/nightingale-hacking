/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2011 POTI, Inc.
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

#ifndef SBLOCALDATABASERESOURCEPROPERTYBAG_H_
#define SBLOCALDATABASERESOURCEPROPERTYBAG_H_

#include <sbILocalDatabaseResourcePropertyBag.h>

#include <nsClassHashtable.h>
#include <nsDataHashtable.h>
#include <nsStringAPI.h>

#include <set>

#include <sbIIdentityService.h>

class sbLocalDatabasePropertyCache;
class sbIPropertyManager;
struct sbPropertyData;

/**
 * This holds the collection of properties usually related to a media item.
 * It is generally by sbLocalDatabasePropertyCache, sbLocalDatabaseMediaItem,
 * sbLocalDatabaseTreeView
 */
class sbLocalDatabaseResourcePropertyBag : public sbILocalDatabaseResourcePropertyBag
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASERESOURCEPROPERTYBAG

  sbLocalDatabaseResourcePropertyBag(sbLocalDatabasePropertyCache* aCache,
                                     PRUint32 mMediaItemId,
                                     const nsAString& aGuid);

  ~sbLocalDatabaseResourcePropertyBag();

  nsresult Init();
  nsresult PutValue(PRUint32 aPropertyID,
                    const nsAString& aValue);

  PRBool IsPropertyDirty(PRUint32 aPropertyDBID);
  nsresult EnumerateDirty(nsTHashtable<nsUint32HashKey>::Enumerator aEnumFunc, void *aClosure, PRUint32 *aDirtyCount);
  nsresult ClearDirty();

  // Getting the dirty properties used for invalidation of GUID arrays will 
  // also clear them from the property bag.
  nsresult GetDirtyForInvalidation(std::set<PRUint32> &aDirty);

private:

  static PLDHashOperator PR_CALLBACK
    PropertyBagKeysToArray(const PRUint32& aPropertyID,
                           sbPropertyData* aData,
                           void *aArg);

  sbLocalDatabasePropertyCache* mCache;
  nsClassHashtableMT<nsUint32HashKey, sbPropertyData> mValueMap;

  nsCOMPtr<sbIPropertyManager> mPropertyManager;
  nsCOMPtr<sbIIdentityService> mIdService;

  nsString  mGuid;
  PRUint32  mMediaItemId;
  
  // Dirty Property ID's
  nsTHashtable<nsUint32HashKey> mDirty;
  // Dirty Property ID's used for invalidating GUID arrays.
  std::set<PRUint32> mDirtyForInvalidation;
};

/**
 * This holds the data for a property. The data is composed of three
 * values: the actual property value, the sortable value (eg, collation data
 * for strings), and the searchable value (eg, lowercased strings)
 */
struct sbPropertyData
{
  sbPropertyData(const nsAString &aValue,
                 const nsAString &aSearchableValue,
                 const nsAString &aSortableValue) :
    value(aValue),
    searchableValue(aSearchableValue),
    sortableValue(aSortableValue)
  {};
  nsString value;
  nsString searchableValue;
  nsString sortableValue;
};


#endif /* SBLOCALDATABASERESOURCEPROPERTYBAG_H_ */
