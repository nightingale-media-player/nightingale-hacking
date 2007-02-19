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

#include "sbLocalDatabasePropertyCache.h"
#include <prlog.h>
#include <DatabaseQuery.h>
#include <sbSQLBuilderCID.h>
#include <nsMemory.h>
#include <nsCOMArray.h>

#define INIT if (!mInitialized) { rv = Init(); NS_ENSURE_SUCCESS(rv, rv); }

#define MAX_IN_LENGTH 10

#if defined PR_LOGGING
static const PRLogModuleInfo *gLocalDatabasePropertyCacheLog = nsnull;
#define LOG(args) PR_LOG(gLocalDatabasePropertyCacheLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

NS_IMPL_ISUPPORTS1(sbLocalDatabasePropertyCache, sbILocalDatabasePropertyCache)

sbLocalDatabasePropertyCache::sbLocalDatabasePropertyCache() :
  mInitialized(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gLocalDatabasePropertyCacheLog) {
    gLocalDatabasePropertyCacheLog = PR_NewLogModule("LocalDatabasePropertyCache");
  }
#endif
}

sbLocalDatabasePropertyCache::~sbLocalDatabasePropertyCache()
{
  /* destructor code */
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::GetDatabaseGUID(nsAString& aDatabaseGUID)
{
  aDatabaseGUID = mDatabaseGUID;

  return NS_OK;
}
NS_IMETHODIMP
sbLocalDatabasePropertyCache::SetDatabaseGUID(const nsAString& aDatabaseGUID)
{
  mDatabaseGUID = aDatabaseGUID;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::GetProperties(const PRUnichar **aGUIDArray,
                                            PRUint32 aGUIDArrayCount,
                                            PRUint32 *aPropertyArrayCount,
                                            sbILocalDatabaseResourcePropertyBag ***aPropertyArray)
{
  nsresult rv;

  INIT;
  NS_ENSURE_ARG_POINTER(aPropertyArray);

  nsCOMArray<sbILocalDatabaseResourcePropertyBag> propertyBags;

  /*
   * Grab the ones that are cached, collect the rest into the missed array
   */
  nsTArray<nsString> misses;
  for (PRUint32 i = 0; i < aGUIDArrayCount; i++) {
    nsAutoString guid(aGUIDArray[i]);
    sbCacheEntry cacheEntry;
    if (mCache.Get(guid, &cacheEntry)) {
      NS_ENSURE_TRUE(propertyBags.AppendObject(cacheEntry.propertyBag),
                    NS_ERROR_OUT_OF_MEMORY);
    }
    else {
      NS_ENSURE_TRUE(misses.AppendElement(guid),
                     NS_ERROR_OUT_OF_MEMORY);
    }
  }

  /*
   * Look up and cache each of the misses
   */
  PRUint32 numMisses = misses.Length();
  if (numMisses > 0) {
    PRUint32 inNum = 0;
    for (PRUint32 j = 0; j < numMisses; j++) {

      /*
       * Add each guid to the query and execute the query when we've added
       * MAX_IN_LENGTH of them (or when we are on the last one)
       */
      nsAutoString& guid = misses[j];
      rv = mInCriterion->AddString(guid);
      NS_ENSURE_SUCCESS(rv, rv);

      if (inNum > MAX_IN_LENGTH || j + 1 == numMisses) {
        PRInt32 dbOk;

        nsAutoString sql;
        rv = mSelect->ToString(sql);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIDatabaseQuery> query;
        rv = MakeQuery(sql, getter_AddRefs(query));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = query->Execute(&dbOk);
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_SUCCESS(dbOk, dbOk);

        rv = query->WaitForCompletion(&dbOk);
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_SUCCESS(dbOk, dbOk);

        nsCOMPtr<sbIDatabaseResult> result;
        rv = query->GetResultObject(getter_AddRefs(result));
        NS_ENSURE_SUCCESS(rv, rv);

        PRInt32 rowCount;
        rv = result->GetRowCount(&rowCount);
        NS_ENSURE_SUCCESS(rv, rv);

        nsAutoString lastGUID;
        nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
        for (PRInt32 row = 0; row < rowCount; row++) {
          PRUnichar* guid;
          rv = result->GetRowCellPtr(row, 0, &guid);
          NS_ENSURE_SUCCESS(rv, rv);

          /*
           * If this is the first row result or we've encountered a new
           * guid, create a new property bag and add it to both the cache and
           * the output array.
           */
          if (row == 0 || !lastGUID.Equals(guid)) {
            lastGUID = guid;
            bag = new sbLocalDatabaseResourcePropertyBag(this);
            rv = NS_STATIC_CAST(sbLocalDatabaseResourcePropertyBag*, bag.get())
                                  ->Init();
            NS_ENSURE_SUCCESS(rv, rv);
            NS_ENSURE_TRUE(propertyBags.AppendObject(bag),
                           NS_ERROR_OUT_OF_MEMORY);
            NS_ENSURE_TRUE(mCache.Put(lastGUID, sbCacheEntry(bag)),
                           NS_ERROR_OUT_OF_MEMORY);
          }

          /*
           * Add each property / object pair to the current bag
           */
          nsAutoString propertyIDStr;
          rv = result->GetRowCell(row, 1, propertyIDStr);
          NS_ENSURE_SUCCESS(rv, rv);

          PRUint32 propertyID = propertyIDStr.ToInteger(&rv);
          NS_ENSURE_SUCCESS(rv, rv);

          nsAutoString obj;
          rv = result->GetRowCell(row, 2, obj);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = NS_STATIC_CAST(sbLocalDatabaseResourcePropertyBag*, bag.get())
                                ->PutValue(propertyID, obj);
          NS_ENSURE_SUCCESS(rv, rv);
        }

        mInCriterion->Clear();
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  /*
   * Allocate and copy the values into the out array
   */
  sbILocalDatabaseResourcePropertyBag **propertyBagArray;

  *aPropertyArrayCount = propertyBags.Count();
  if (*aPropertyArrayCount > 0) {
    propertyBagArray = (sbILocalDatabaseResourcePropertyBag **)
      nsMemory::Alloc((sizeof (sbILocalDatabaseResourcePropertyBag *)) * *aPropertyArrayCount);

    for (PRUint32 i = 0; i < *aPropertyArrayCount; i++) {
      propertyBagArray[i] = propertyBags[i];
      NS_ADDREF(propertyBagArray[i]);
    }
  }
  else {
    propertyBagArray = nsnull;
  }

  *aPropertyArray = propertyBagArray;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::Init()
{
  nsresult rv;

  mSelect = do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSelect->SetBaseTableName(NS_LITERAL_STRING("resource_properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSelect->AddColumn(EmptyString(), NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSelect->AddColumn(EmptyString(), NS_LITERAL_STRING("property_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSelect->AddColumn(EmptyString(), NS_LITERAL_STRING("obj"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSelect->CreateMatchCriterionIn(EmptyString(),
                                       NS_LITERAL_STRING("guid"),
                                       getter_AddRefs(mInCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSelect->AddCriterion(mInCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSelect->AddOrder(EmptyString(),
                         NS_LITERAL_STRING("guid"),
                         PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mSelect->AddOrder(EmptyString(),
                         NS_LITERAL_STRING("property_id"),
                         PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = LoadProperties();
  NS_ENSURE_SUCCESS(rv, rv);

  mInitialized = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::MakeQuery(const nsAString& aSql,
                                        sbIDatabaseQuery** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  LOG(("MakeQuery: %s", NS_ConvertUTF16toUTF8(aSql).get()));

  nsresult rv;

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(mDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(aSql);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = query);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::LoadProperties()
{
  nsresult rv;
  PRInt32 dbOk;

  if (!mPropertyNameToID.IsInitialized()) {
    NS_ENSURE_TRUE(mPropertyNameToID.Init(100), NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    mPropertyNameToID.Clear();
  }

  if (!mPropertyIDToName.IsInitialized()) {
    NS_ENSURE_TRUE(mPropertyIDToName.Init(100), NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    mPropertyIDToName.Clear();
  }

  nsCOMPtr<sbISQLSelectBuilder> builder =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->SetBaseTableName(NS_LITERAL_STRING("properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING("property_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = builder->AddColumn(EmptyString(), NS_LITERAL_STRING("property_name"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString sql;
  rv = builder->ToString(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(sql, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  rv = query->WaitForCompletion(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRInt32 i = 0; i < rowCount; i++) {
    nsAutoString propertyIDStr;
    rv = result->GetRowCell(i, 0, propertyIDStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyID = propertyIDStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString propertyName;
    rv = result->GetRowCell(i, 1, propertyName);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(mPropertyIDToName.Put(propertyID, propertyName),
                   NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(mPropertyNameToID.Put(propertyName, propertyID),
                   NS_ERROR_OUT_OF_MEMORY);

  }

  return NS_OK;
}

PRUint32
sbLocalDatabasePropertyCache::GetPropertyID(const nsAString& aPropertyName)
{
  PRUint32 retval;
  if (!mPropertyNameToID.Get(aPropertyName, &retval)) {
    retval = 0;
  }
  return retval;
}

// sbILocalDatabaseResourcePropertyBag
sbLocalDatabaseResourcePropertyBag::sbLocalDatabaseResourcePropertyBag(sbLocalDatabasePropertyCache* aCache) :
  mCache(aCache)
{
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::Init()
{
  NS_ENSURE_TRUE(mValueMap.Init(), NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetNames(nsIStringEnumerator **aNames)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetProperty(const nsAString& aName,
                                                nsAString& _retval)
{
  PRUint32 propertyID = mCache->GetPropertyID(aName);
  if(propertyID > 0) {
    nsAutoString value;
    if (mValueMap.Get(propertyID, &value)) {
      _retval = value;
      return NS_OK;
    }
  }

  return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::PutValue(PRUint32 aPropertyID,
                                             const nsAString& aValue)
{
  nsAutoString value(aValue);
  NS_ENSURE_TRUE(mValueMap.Put(aPropertyID, value),
                 NS_ERROR_OUT_OF_MEMORY);
  return NS_OK;
}

