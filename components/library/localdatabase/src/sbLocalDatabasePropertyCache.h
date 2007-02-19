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
#include <sbISQLBuilder.h>

struct sbCacheEntry
{
  sbCacheEntry() {}
  sbCacheEntry(sbILocalDatabaseResourcePropertyBag *aPropertyBag) :
    propertyBag(aPropertyBag) {}

  nsCOMPtr<sbILocalDatabaseResourcePropertyBag> propertyBag;
};

class sbLocalDatabasePropertyCache : public sbILocalDatabasePropertyCache
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEPROPERTYCACHE

  sbLocalDatabasePropertyCache();

  NS_IMETHODIMP Init();
  NS_IMETHODIMP MakeQuery(const nsAString& aSql, sbIDatabaseQuery** _retval);
  NS_IMETHODIMP LoadProperties();
  PRUint32 GetPropertyID(const nsAString& aPropertyName);

private:
  ~sbLocalDatabasePropertyCache();

  PRBool mInitalized;

  // Database GUID
  // XXX: This will probably change to a path?
  nsString mDatabaseGUID;

  // Cache the property name list
  nsDataHashtable<nsUint32HashKey, nsString> mPropertyIDToName;
  nsDataHashtable<nsStringHashKey, PRUint32> mPropertyNameToID;

  // Used to template the select statement
  nsCOMPtr<sbISQLSelectBuilder> mSelect;
  nsCOMPtr<sbISQLBuilderCriterionIn> mInCriterion;

  // Cache for GUID -> property bag
  nsDataHashtable<nsStringHashKey, sbCacheEntry> mCache;
};

class sbLocalDatabaseResourcePropertyBag : public sbILocalDatabaseResourcePropertyBag
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASERESOURCEPROPERTYBAG

  sbLocalDatabaseResourcePropertyBag(sbLocalDatabasePropertyCache* aCache);

  NS_IMETHODIMP Init();
  NS_IMETHODIMP PutValue(PRUint32 aPropertyID, const nsAString& aValue);

private:
  ~sbLocalDatabaseResourcePropertyBag();

  sbLocalDatabasePropertyCache* mCache;
  nsDataHashtable<nsUint32HashKey, nsString> mValueMap;

protected:
  /* additional members */
};

#endif /* __SBLOCALDATABASEPROPERTYCACHE_H__ */

