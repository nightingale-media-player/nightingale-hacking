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

#ifndef SBLOCALDATABASERESOURCEPROPERTYBAG_H_
#define SBLOCALDATABASERESOURCEPROPERTYBAG_H_

#include <sbILocalDatabaseResourcePropertyBag.h>

#include <nsClassHashtable.h>
#include <nsDataHashtable.h>
#include <nsStringAPI.h>

class sbLocalDatabasePropertyCache;
class sbIPropertyManager;

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
                    const nsAString& aValue,
                    const nsAString& aSearchable,
                    const nsAString& aSortable);

  nsresult EnumerateDirty(nsTHashtable<nsUint32HashKey>::Enumerator aEnumFunc, void *aClosure, PRUint32 *aDirtyCount);
  nsresult ClearDirty();

private:

  struct sbPropertyData {
    sbPropertyData(const nsAString& aValue, 
                   const nsAString& aSearchable, 
                   const nsAString& aSortable) :
      value(aValue),
      searchable(aSearchable),
      sortable(aSortable)
    {};

    nsString value;
    nsString searchable;
    nsString sortable;
  };

  static PLDHashOperator PR_CALLBACK
    PropertyBagKeysToArray(const PRUint32& aPropertyID,
                           sbPropertyData* aData,
                           void *aArg);

  sbLocalDatabasePropertyCache* mCache;
  nsClassHashtableMT<nsUint32HashKey, sbPropertyData> mValueMap;

  nsCOMPtr<sbIPropertyManager> mPropertyManager;

  nsString  mGuid;
  PRUint32  mMediaItemId;
  // Dirty Property ID's
  PRLock* mDirtyLock;
  nsTHashtable<nsUint32HashKey> mDirty;
};

#endif /* SBLOCALDATABASERESOURCEPROPERTYBAG_H_ */
