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

#include <nsIURI.h>
#include <sbIDatabaseQuery.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>

#include <DatabaseQuery.h>
#include <nsAutoLock.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsStringEnumerator.h>
#include <nsUnicharUtils.h>
#include <nsXPCOM.h>
#include <prlog.h>

#include "sbDatabaseResultStringEnumerator.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseSchemaInfo.h"
#include <sbTArrayStringEnumerator.h>

#define MAX_IN_LENGTH 5000

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabasePropertyCache:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo *gLocalDatabasePropertyCacheLog = nsnull;
#define TRACE(args) if (gLocalDatabasePropertyCacheLog) PR_LOG(gLocalDatabasePropertyCacheLog, PR_LOG_DEBUG, args)
#define LOG(args)   if (gLocalDatabasePropertyCacheLog) PR_LOG(gLocalDatabasePropertyCacheLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_THREADSAFE_ISUPPORTS1(sbLocalDatabasePropertyCache, sbILocalDatabasePropertyCache)

sbLocalDatabasePropertyCache::sbLocalDatabasePropertyCache() 
: mWritePending(PR_FALSE),
  mLibrary(nsnull)
{
#ifdef PR_LOGGING
  if (!gLocalDatabasePropertyCacheLog) {
    gLocalDatabasePropertyCacheLog = PR_NewLogModule("sbLocalDatabasePropertyCache");
  }
#endif

  mDirtyLock = nsAutoLock::NewLock("sbLocalDatabasePropertyCache::mDirtyLock");
  NS_ASSERTION(mDirtyLock, 
    "sbLocalDatabasePropertyCache::mDirtyLock failed to create lock!");
}

sbLocalDatabasePropertyCache::~sbLocalDatabasePropertyCache()
{
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::GetWritePending(PRBool *aWritePending)
{
  NS_ASSERTION(mLibrary, "You didn't initalize!");
  NS_ENSURE_ARG_POINTER(aWritePending);
  *aWritePending = mWritePending;

  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::Init(sbLocalDatabaseLibrary* aLibrary)
{
  NS_ASSERTION(!mLibrary, "Already initalized!");
  NS_ENSURE_ARG_POINTER(aLibrary);

  nsresult rv = aLibrary->GetDatabaseGuid(mDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aLibrary->GetDatabaseLocation(getter_AddRefs(mDatabaseLocation));
  NS_ENSURE_SUCCESS(rv, rv);

  mPropertyManager = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Simple select from properties table with an in list of guids

  mPropertiesSelect = do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesSelect->SetBaseTableName(NS_LITERAL_STRING("resource_properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesSelect->AddColumn(EmptyString(), NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesSelect->AddColumn(EmptyString(), NS_LITERAL_STRING("property_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesSelect->AddColumn(EmptyString(), NS_LITERAL_STRING("obj"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesSelect->CreateMatchCriterionIn(EmptyString(),
                                                 NS_LITERAL_STRING("guid"),
                                                 getter_AddRefs(mPropertiesInCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesSelect->AddCriterion(mPropertiesInCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesSelect->AddOrder(EmptyString(),
                                   NS_LITERAL_STRING("guid"),
                                   PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesSelect->AddOrder(EmptyString(),
                                   NS_LITERAL_STRING("property_id"),
                                   PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create simple media_items query with in list of guids

  mMediaItemsSelect = do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsSelect->SetBaseTableName(NS_LITERAL_STRING("media_items"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsSelect->AddColumn(EmptyString(), NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    rv = mMediaItemsSelect->AddColumn(EmptyString(),
                                      nsDependentString(sStaticProperties[i].mColumn));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mMediaItemsSelect->CreateMatchCriterionIn(EmptyString(),
                                                 NS_LITERAL_STRING("guid"),
                                                 getter_AddRefs(mMediaItemsInCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsSelect->AddCriterion(mMediaItemsInCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsSelect->AddOrder(EmptyString(),
                                   NS_LITERAL_STRING("guid"),
                                   PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // properties table insert, generates property id for property in this library.
  // INSERT INTO properties (property_name) VALUES (?)
  mPropertiesTableInsert = do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mPropertiesTableInsert->SetIntoTableName(NS_LITERAL_STRING("properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesTableInsert->AddColumn(NS_LITERAL_STRING("property_name"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesTableInsert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  // Create property insert query builder:
  //  INSERT INTO resource_properties (guid, property_id, obj, obj_sortable) VALUES (?, ?, ?, ?)

  mPropertiesInsert = do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesInsert->SetIntoTableName(NS_LITERAL_STRING("resource_properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesInsert->AddColumn(NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesInsert->AddColumn(NS_LITERAL_STRING("property_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesInsert->AddColumn(NS_LITERAL_STRING("obj"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesInsert->AddColumn(NS_LITERAL_STRING("obj_sortable"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesInsert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesInsert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesInsert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesInsert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  // Create property update query builder:
  //   UPDATE resource_properties SET obj = ?, obj_sortable = ? WHERE guid = ? AND property_id = ?

  mPropertiesUpdate = do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesUpdate->SetTableName(NS_LITERAL_STRING("resource_properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesUpdate->AddAssignmentParameter(NS_LITERAL_STRING("obj"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesUpdate->AddAssignmentParameter(NS_LITERAL_STRING("obj_sortable"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionLeft;
  rv = mPropertiesUpdate->CreateMatchCriterionParameter(EmptyString(), 
                                                        NS_LITERAL_STRING("guid"),
                                                        sbISQLBuilder::MATCH_EQUALS,
                                                        getter_AddRefs(criterionLeft));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterionRight;
  rv = mPropertiesUpdate->CreateMatchCriterionParameter(EmptyString(),
                                                        NS_LITERAL_STRING("property_id"),
                                                        sbISQLBuilder::MATCH_EQUALS,
                                                        getter_AddRefs(criterionRight));

  nsCOMPtr<sbISQLBuilderCriterion> criterionAnd;
  rv = mPropertiesUpdate->CreateAndCriterion(criterionLeft, criterionRight, getter_AddRefs(criterionAnd));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesUpdate->AddCriterion(criterionAnd);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create media item property update query builder.
  // This one can't be prepared in advance.

  mMediaItemsUpdate = do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create query used to verify if we need to insert or update a property

  mPropertyInsertSelect = do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertyInsertSelect->SetBaseTableName(NS_LITERAL_STRING("resource_properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertyInsertSelect->AddColumn(EmptyString(), NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertyInsertSelect->CreateMatchCriterionParameter(EmptyString(), 
                                                            NS_LITERAL_STRING("guid"),
                                                            sbISQLBuilder::MATCH_EQUALS,
                                                            getter_AddRefs(criterionLeft));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertyInsertSelect->CreateMatchCriterionParameter(EmptyString(),
                                                            NS_LITERAL_STRING("property_id"),
                                                            sbISQLBuilder::MATCH_EQUALS,
                                                            getter_AddRefs(criterionRight));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertyInsertSelect->CreateAndCriterion(criterionLeft, criterionRight, getter_AddRefs(criterionAnd));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertyInsertSelect->AddCriterion(criterionAnd);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = LoadProperties();
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mCache.Init(1000);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mDirty.Init(1000);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mLibrary = aLibrary;

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::CacheProperties(const PRUnichar **aGUIDArray,
                                              PRUint32 aGUIDArrayCount)
{
  TRACE(("sbLocalDatabasePropertyCache[0x%.8x] - CacheProperties(%d)", this,
         aGUIDArrayCount));

  NS_ASSERTION(mLibrary, "You didn't initalize!");
  nsresult rv;

  /*
   * First, collect all the guids that are not cached
   */
  nsTArray<nsString> misses;
  for (PRUint32 i = 0; i < aGUIDArrayCount; i++) {
    nsAutoString guid(aGUIDArray[i]);
    if (!mCache.Get(guid, nsnull)) {
      nsString* success = misses.AppendElement(guid);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
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
      rv = mPropertiesInCriterion->AddString(misses[j]);
      NS_ENSURE_SUCCESS(rv, rv);

      if (inNum > MAX_IN_LENGTH || j + 1 == numMisses) {
        PRInt32 dbOk;

        nsAutoString sql;
        rv = mPropertiesSelect->ToString(sql);
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

        PRUint32 rowCount;
        rv = result->GetRowCount(&rowCount);
        NS_ENSURE_SUCCESS(rv, rv);

        nsAutoString lastGUID;
        nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
        for (PRUint32 row = 0; row < rowCount; row++) {
          PRUnichar* guid;
          rv = result->GetRowCellPtr(row, 0, &guid);
          NS_ENSURE_SUCCESS(rv, rv);

          /*
           * If this is the first row result or we've encountered a new
           * guid, create a new property bag and add it to the cache
           */
          if (row == 0 || !lastGUID.Equals(guid)) {
            lastGUID = guid;
            nsAutoPtr<sbLocalDatabaseResourcePropertyBag> newBag
              (new sbLocalDatabaseResourcePropertyBag(this, lastGUID));
            NS_ENSURE_TRUE(newBag, NS_ERROR_OUT_OF_MEMORY);

            rv = newBag->Init();
            NS_ENSURE_SUCCESS(rv, rv);

            bag = newBag.forget();
            PRBool success = mCache.Put(lastGUID, bag);
            NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
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

        mPropertiesInCriterion->Clear();
        NS_ENSURE_SUCCESS(rv, rv);
      }

    }

    /*
     * Do the same thing for top level properties
     */
    inNum = 0;
    for (PRUint32 j = 0; j < numMisses; j++) {

      rv = mMediaItemsInCriterion->AddString(misses[j]);
      NS_ENSURE_SUCCESS(rv, rv);

      if (inNum > MAX_IN_LENGTH || j + 1 == numMisses) {
        PRInt32 dbOk;

        nsAutoString sql;
        rv = mMediaItemsSelect->ToString(sql);
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

        PRUint32 rowCount;
        rv = result->GetRowCount(&rowCount);
        NS_ENSURE_SUCCESS(rv, rv);

        sbILocalDatabaseResourcePropertyBag* bag;
        for (PRUint32 row = 0; row < rowCount; row++) {
          nsAutoString guid;
          rv = result->GetRowCell(row, 0, guid);
          NS_ENSURE_SUCCESS(rv, rv);

          if (!mCache.Get(guid, &bag)) {
            nsAutoPtr<sbLocalDatabaseResourcePropertyBag> newBag
              (new sbLocalDatabaseResourcePropertyBag(this, guid));
            NS_ENSURE_TRUE(newBag, NS_ERROR_OUT_OF_MEMORY);

            rv = newBag->Init();
            NS_ENSURE_SUCCESS(rv, rv);

            bag = newBag.forget();
            PRBool success = mCache.Put(guid, bag);
            NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
          }

          for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
            nsAutoString value;
            rv = result->GetRowCell(row, i + 1, value);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = NS_STATIC_CAST(sbLocalDatabaseResourcePropertyBag*, bag)
                                  ->PutValue(sStaticProperties[i].mID, value);
            NS_ENSURE_SUCCESS(rv, rv);
          }

        }

        mMediaItemsInCriterion->Clear();
        NS_ENSURE_SUCCESS(rv, rv);
      }

    }

  }

  TRACE(("sbLocalDatabasePropertyCache[0x%.8x] - CacheProperties() - Misses %d", this,
         numMisses));

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::GetProperties(const PRUnichar **aGUIDArray,
                                            PRUint32 aGUIDArrayCount,
                                            PRUint32 *aPropertyArrayCount,
                                            sbILocalDatabaseResourcePropertyBag ***aPropertyArray)
{
  NS_ASSERTION(mLibrary, "You didn't initalize!");
  nsresult rv;

  NS_ENSURE_ARG_POINTER(aPropertyArrayCount);
  NS_ENSURE_ARG_POINTER(aPropertyArray);

  /*
   * Build the output array using cache lookups
   */
  sbILocalDatabaseResourcePropertyBag** propertyBagArray = nsnull;

  *aPropertyArrayCount = aGUIDArrayCount;
  if (aGUIDArrayCount > 0) {

    rv = CacheProperties(aGUIDArray, aGUIDArrayCount);
    NS_ENSURE_SUCCESS(rv, rv);

    propertyBagArray = (sbILocalDatabaseResourcePropertyBag **)
      NS_Alloc((sizeof (sbILocalDatabaseResourcePropertyBag *)) * *aPropertyArrayCount);

    for (PRUint32 i = 0; i < aGUIDArrayCount; i++) {
      nsAutoString guid(aGUIDArray[i]);
      sbILocalDatabaseResourcePropertyBag* bag;
      if (mCache.Get(guid, &bag)) {
        propertyBagArray[i] = bag;
        NS_ADDREF(propertyBagArray[i]);
      }
      else {
        propertyBagArray[i] = nsnull;
      }
    }
  }
  else {
    propertyBagArray = nsnull;
  }

  *aPropertyArray = propertyBagArray;

  return NS_OK;
}


NS_IMETHODIMP 
sbLocalDatabasePropertyCache::SetProperties(const PRUnichar **aGUIDArray, 
                                            PRUint32 aGUIDArrayCount, 
                                            sbILocalDatabaseResourcePropertyBag **aPropertyArray, 
                                            PRUint32 aPropertyArrayCount, 
                                            PRBool aWriteThroughNow)
{
  NS_ASSERTION(mLibrary, "You didn't initalize!");
  NS_ENSURE_ARG_POINTER(aGUIDArray);
  NS_ENSURE_ARG_POINTER(aPropertyArray);
  NS_ENSURE_TRUE(aGUIDArrayCount == aPropertyArrayCount, NS_ERROR_INVALID_ARG);

  nsresult rv = NS_OK;

  sbAutoBatchHelper batchHelper(mLibrary);

  for(PRUint32 i = 0; i < aGUIDArrayCount; i++) {
    nsAutoString guid(aGUIDArray[i]);
    sbILocalDatabaseResourcePropertyBag* bag = nsnull;

    if(mCache.Get(guid, &bag) && bag) {
      nsCOMPtr<nsIStringEnumerator> names;
      rv = aPropertyArray[i]->GetNames(getter_AddRefs(names));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasMore = PR_FALSE;
      nsAutoString name, value;

      while(NS_SUCCEEDED(names->HasMore(&hasMore)) && hasMore) {
        rv = names->GetNext(name);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aPropertyArray[i]->GetProperty(name, value);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = NS_STATIC_CAST(sbLocalDatabaseResourcePropertyBag*, bag)
          ->SetProperty(name, value);
      }

      PR_Lock(mDirtyLock);
      mDirty.PutEntry(guid);
      PR_Unlock(mDirtyLock);

      mWritePending = PR_TRUE;
    }
  }

  if(aWriteThroughNow) {
    rv = Write();
  }

  return rv;
}

PR_STATIC_CALLBACK(PLDHashOperator)
EnumDirtyGuids(nsStringHashKey *aKey, void *aClosure)
{
  nsTArray<nsString> *dirtyGuids = NS_STATIC_CAST(nsTArray<nsString> *, aClosure);
  dirtyGuids->AppendElement(aKey->GetKey());
  return PL_DHASH_NEXT;
}

PR_STATIC_CALLBACK(PLDHashOperator)
EnumDirtyProps(nsUint32HashKey *aKey, void *aClosure)
{
  nsTArray<PRUint32> *dirtyProps = NS_STATIC_CAST(nsTArray<PRUint32> *, aClosure);
  dirtyProps->AppendElement(aKey->GetKey());
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP 
sbLocalDatabasePropertyCache::Write()
{
  NS_ASSERTION(mLibrary, "You didn't initalize!");

  nsresult rv = NS_OK;
  nsTArray<nsString> dirtyGuids;

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(NS_LITERAL_STRING("begin"), getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);
  
  //Lock it.
  nsAutoLock lock(mDirtyLock);

  //Enumerate dirty GUIDs
  PRUint32 dirtyGuidCount = mDirty.EnumerateEntries(EnumDirtyGuids, (void *) &dirtyGuids);

  //For each GUID, there's a property bag that needs to be processed as well.
  for(PRUint32 i = 0; i < dirtyGuidCount; i++) {
    sbILocalDatabaseResourcePropertyBag* bag;
    if (mCache.Get(dirtyGuids[i], &bag)) {
      nsTArray<PRUint32> dirtyProps;
      sbLocalDatabaseResourcePropertyBag* bagLocal = 
        NS_STATIC_CAST(sbLocalDatabaseResourcePropertyBag *, bag);

      PRUint32 dirtyPropsCount = 0;
      rv = bagLocal->EnumerateDirty(EnumDirtyProps, (void *) &dirtyProps, &dirtyPropsCount);
      NS_ENSURE_SUCCESS(rv, rv);

      //Enumerate dirty properties for this GUID.
      nsAutoString value;
      for(PRUint32 j = 0; j < dirtyPropsCount; j++) {
        nsAutoString sql, propertyName;

        //If this isn't true, something is very wrong, so bail out.
        PRBool success = GetPropertyName(dirtyProps[j], propertyName);
        NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);
        bagLocal->GetPropertyByID(dirtyProps[j], value);

        rv = InsertPropertyNameInLibrary(propertyName);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIPropertyInfo> propertyInfo;
        rv = mPropertyManager->GetPropertyInfo(propertyName, 
          getter_AddRefs(propertyInfo));
        NS_ENSURE_SUCCESS(rv, rv);
        
        //Top level properties need to be treated differently, so check for them.
        if(SB_IsTopLevelPropertyID(dirtyProps[j])) {

          rv = mMediaItemsUpdate->SetTableName(NS_LITERAL_STRING("media_items"));
          NS_ENSURE_SUCCESS(rv, rv);

          nsAutoString column;
          GetColumnForPropertyID(dirtyProps[j], column);

          rv = mMediaItemsUpdate->AddAssignmentString(column, value);
          NS_ENSURE_SUCCESS(rv, rv);

          nsCOMPtr<sbISQLBuilderCriterion> criterion;
          rv = mMediaItemsUpdate->CreateMatchCriterionString(EmptyString(), 
                                                             NS_LITERAL_STRING("guid"),
                                                             sbISQLBuilder::MATCH_EQUALS,
                                                             dirtyGuids[i],
                                                             getter_AddRefs(criterion));
          NS_ENSURE_SUCCESS(rv, rv);

          rv = mMediaItemsUpdate->AddCriterion(criterion);
          NS_ENSURE_SUCCESS(rv, rv);
          
          rv = mMediaItemsUpdate->ToString(sql);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = query->AddQuery(sql);
          NS_ENSURE_SUCCESS(rv, rv);

          rv = mMediaItemsUpdate->Reset();
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else { //Regular properties all go in the same spot.
          nsAutoString sortable;
          rv = propertyInfo->MakeSortable(value, sortable);
          NS_ENSURE_SUCCESS(rv, rv);

          //Check if we need to insert or update the property.
          PRBool bNeedInsert = PR_FALSE;
          PropertyRequiresInsert(dirtyGuids[i], dirtyProps[j], &bNeedInsert);
          if(bNeedInsert) {
            rv = mPropertiesInsert->ToString(sql);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->AddQuery(sql);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->BindStringParameter(0, dirtyGuids[i]);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->BindInt32Parameter(1, dirtyProps[j]);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->BindStringParameter(2, value);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->BindStringParameter(3, sortable);
            NS_ENSURE_SUCCESS(rv, rv);
          } 
          else {
            rv = mPropertiesUpdate->ToString(sql);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->AddQuery(sql);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->BindStringParameter(0, value);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->BindStringParameter(1, sortable);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->BindStringParameter(2, dirtyGuids[i]);
            NS_ENSURE_SUCCESS(rv, rv);

            rv = query->BindInt32Parameter(3, dirtyProps[j]);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
      }
    }
  }

  if(dirtyGuidCount)
  {
    PRInt32 dbOk;

    rv = query->AddQuery(NS_LITERAL_STRING("commit"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->Execute(&dbOk);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_SUCCESS(dbOk, dbOk);

    rv = query->WaitForCompletion(&dbOk);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_SUCCESS(dbOk, dbOk);

    for(PRUint32 i = 0; i < dirtyGuidCount; i++) {
      sbILocalDatabaseResourcePropertyBag* bag;
      if (mCache.Get(dirtyGuids[i], &bag)) {
        sbLocalDatabaseResourcePropertyBag* bagLocal = 
          NS_STATIC_CAST(sbLocalDatabaseResourcePropertyBag *, bag);
        bagLocal->SetDirty(PR_FALSE);
      }
    }
    
    //Clear dirty guid hastable.
    mDirty.Clear();
  }

  return rv;
}

NS_IMETHODIMP 
sbLocalDatabasePropertyCache::GetPropertyID(const nsAString& aProperty,
                                            PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = GetPropertyIDInternal(aProperty);
  return NS_OK;
}

nsresult
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

  if (mDatabaseLocation) {
    rv = query->SetDatabaseLocation(mDatabaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(aSql);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = query);
  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::LoadProperties()
{
  nsresult rv;
  PRInt32 dbOk;

  if (!mPropertyNameToID.IsInitialized()) {
    PRBool success = mPropertyNameToID.Init(100);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    mPropertyNameToID.Clear();
  }

  if (!mPropertyIDToName.IsInitialized()) {
    PRBool success = mPropertyIDToName.Init(100);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
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

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < rowCount; i++) {
    nsAutoString propertyIDStr;
    rv = result->GetRowCell(i, 0, propertyIDStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyID = propertyIDStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString propertyName;
    rv = result->GetRowCell(i, 1, propertyName);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = mPropertyIDToName.Put(propertyID, propertyName);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    TRACE(("Added %d => %s to property name cache", propertyID,
           NS_ConvertUTF16toUTF8(propertyName).get()));
 
    success = mPropertyNameToID.Put(propertyName, propertyID);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  }

  /*
   * Add top level properties
   */
  for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {

    nsAutoString propertyName(sStaticProperties[i].mName);
    PRBool success = mPropertyIDToName.Put(sStaticProperties[i].mID, propertyName);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    NS_ENSURE_TRUE(mPropertyNameToID.Put(
      nsDependentString(sStaticProperties[i].mName), sStaticProperties[i].mID),
      NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::AddDirtyGUID(const nsAString &aGuid)
{
  nsAutoString guid(aGuid);

  {
    nsAutoLock lock(mDirtyLock);
    mDirty.PutEntry(guid);
  }

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = mLibrary->GetMediaItem(guid, getter_AddRefs(item));
  if (NS_SUCCEEDED(rv)) {
    mLibrary->NotifyListenersItemUpdated(mLibrary, item);
  }

  return NS_OK;
}

PRUint32
sbLocalDatabasePropertyCache::GetPropertyIDInternal(const nsAString& aPropertyName)
{
  PRUint32 retval;
  if (!mPropertyNameToID.Get(aPropertyName, &retval)) {
    InsertPropertyNameInLibrary(aPropertyName);
  }
  return retval;
}

PRBool
sbLocalDatabasePropertyCache::GetPropertyName(PRUint32 aPropertyID,
                                              nsAString& aPropertyName)
{
  nsAutoString propertyName;
  if (mPropertyIDToName.Get(aPropertyID, &propertyName)) {
    aPropertyName = propertyName;
    return PR_TRUE;
  }
  return PR_FALSE;
}

nsresult
sbLocalDatabasePropertyCache::PropertyRequiresInsert(const nsAString &aGuid, PRUint32 aPropertyID, PRBool *aInsert)
{
  NS_ENSURE_ARG_POINTER(aInsert);
  *aInsert = PR_TRUE;

  nsAutoString sql;
  nsresult rv = mPropertyInsertSelect->ToString(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(sql, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt32Parameter(1, aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  rv = query->WaitForCompletion(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if(rowCount > 0) {
    *aInsert = PR_FALSE;
  }

  return NS_OK;
}

void
sbLocalDatabasePropertyCache::GetColumnForPropertyID(PRUint32 aPropertyID, 
                                                     nsAString &aColumn)
{
  //XXX: This needs to use the property manager when it becomes available.
  PRUint32 numTopLevelProps = sizeof(sStaticProperties) / sizeof(sbStaticProperty); 
  for(PRUint32 i = 0; i < numTopLevelProps; i++) {
    if(sStaticProperties[i].mID == aPropertyID) {
      aColumn = sStaticProperties[i].mColumn;
      return;
    }
  }
  return;
}

nsresult sbLocalDatabasePropertyCache::InsertPropertyNameInLibrary(const nsAString& aPropertyName)
{
  nsAutoString sql;

  nsresult rv = mPropertiesTableInsert->ToString(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  //Hack. Should fix SQLBuilder to support conflict clauses.
  sql.Replace(0, 6, NS_LITERAL_STRING("INSERT OR IGNORE"));

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(sql, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aPropertyName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  rv = query->WaitForCompletion(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbOk, dbOk);

  return NS_OK;
}

// sbILocalDatabaseResourcePropertyBag
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLocalDatabaseResourcePropertyBag,
                              sbILocalDatabaseResourcePropertyBag)

sbLocalDatabaseResourcePropertyBag::sbLocalDatabaseResourcePropertyBag(sbLocalDatabasePropertyCache* aCache,
                                                                       const nsAString &aGuid)
: mCache(aCache)
, mWritePending(PR_FALSE)
, mGuid(aGuid)
{
}

sbLocalDatabaseResourcePropertyBag::~sbLocalDatabaseResourcePropertyBag()
{
  if(mDirtyLock) {
    nsAutoLock::DestroyLock(mDirtyLock);
  }
}

nsresult
sbLocalDatabaseResourcePropertyBag::Init()
{
  nsresult rv;

  PRBool success = mValueMap.Init(1000);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mDirty.Init(1000);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mPropertyManager = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mDirtyLock = nsAutoLock::NewLock("sbLocalDatabaseResourcePropertyBag::mDirtyLock");
  NS_ENSURE_TRUE(mDirtyLock, NS_ERROR_OUT_OF_MEMORY);
  
  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
PropertyBagKeysToArray(const PRUint32& aPropertyID,
                       nsString* aValue,
                       void *aArg)
{
  nsTArray<PRUint32>* propertyIDs = (nsTArray<PRUint32>*) aArg;
  if (propertyIDs->AppendElement(aPropertyID)) {
    return PL_DHASH_NEXT;
  }
  else {
    return PL_DHASH_STOP;
  }
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetGuid(nsAString &aGuid)
{
  aGuid = mGuid;
  return NS_OK;
}

NS_IMETHODIMP 
sbLocalDatabaseResourcePropertyBag::GetWritePending(PRBool *aWritePending)
{
  NS_ENSURE_ARG_POINTER(aWritePending);
  *aWritePending = mWritePending;
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetNames(nsIStringEnumerator **aNames)
{
  NS_ENSURE_ARG_POINTER(aNames);

  nsTArray<PRUint32> propertyIDs;
  mValueMap.EnumerateRead(PropertyBagKeysToArray, &propertyIDs);

  PRUint32 len = mValueMap.Count();
  if (propertyIDs.Length() < len) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsTArray<nsString> propertyNames;
  for (PRUint32 i = 0; i < len; i++) {
    nsAutoString propertyName;
    PRBool success = mCache->GetPropertyName(propertyIDs[i], propertyName);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);
    propertyNames.AppendElement(propertyName);
  }

  *aNames = new sbTArrayStringEnumerator(&propertyNames);
  NS_ENSURE_TRUE(*aNames, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aNames);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetProperty(const nsAString& aName,
                                                nsAString& _retval)
{
  PRUint32 propertyID = mCache->GetPropertyIDInternal(aName);

  nsresult rv = GetPropertyByID(propertyID, _retval);
  if (NS_SUCCEEDED(rv)) {
    return NS_OK;
  }
  else {
    return NS_ERROR_ILLEGAL_VALUE;
  }
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetPropertyByID(PRUint32 aPropertyID,
                                                    nsAString& _retval)
{
  if(aPropertyID > 0) {
    nsAutoLock lock(mDirtyLock);
    nsString* value;

    if (mValueMap.Get(aPropertyID, &value)) {
      _retval = *value;
      return NS_OK;
    }
  }

  return NS_ERROR_ILLEGAL_VALUE;
}

NS_IMETHODIMP 
sbLocalDatabaseResourcePropertyBag::SetProperty(const nsAString & aName, 
                                                const nsAString & aValue)
{
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  PRUint32 propertyID = mCache->GetPropertyIDInternal(aName);

  nsresult rv = mPropertyManager->GetPropertyInfo(aName, 
    getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool valid = PR_FALSE;
  rv = propertyInfo->Validate(aValue, &valid);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(PR_LOGGING)
  if(NS_UNLIKELY(!valid)) {
    LOG(("Failed to set property %s with value %s", 
      NS_ConvertUTF16toUTF8(aName).get(),
      NS_ConvertUTF16toUTF8(aValue).get()));
  }
#endif

  NS_ENSURE_TRUE(valid, NS_ERROR_ILLEGAL_VALUE);

  if(propertyID == 0) {
    mCache->InsertPropertyNameInLibrary(aName);
  }

  return SetPropertyByID(propertyID, aValue);
}

NS_IMETHODIMP 
sbLocalDatabaseResourcePropertyBag::SetPropertyByID(PRUint32 aPropertyID, 
                                                    const nsAString & aValue)
{
  nsresult rv = NS_ERROR_INVALID_ARG;
  if(aPropertyID > 0) {
     rv = PutValue(aPropertyID, aValue);
     NS_ENSURE_SUCCESS(rv, rv);
     
     rv = mCache->AddDirtyGUID(mGuid);
     NS_ENSURE_SUCCESS(rv, rv);

     mWritePending = PR_TRUE;

     PR_Lock(mDirtyLock);
     mDirty.PutEntry(aPropertyID);
     PR_Unlock(mDirtyLock);
  }

  return rv;
}

NS_IMETHODIMP sbLocalDatabaseResourcePropertyBag::Write()
{
  nsresult rv = NS_OK;

  if(mWritePending) {
    rv = mCache->Write();
    NS_ENSURE_SUCCESS(rv, rv);

    mWritePending = PR_FALSE;
  }

  return rv;
}

nsresult
sbLocalDatabaseResourcePropertyBag::PutValue(PRUint32 aPropertyID,
                                             const nsAString& aValue)
{
  nsAutoPtr<nsString> value(new nsString(aValue));
  PRBool success = mValueMap.Put(aPropertyID, value);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  value.forget();

  return NS_OK;
}

nsresult
sbLocalDatabaseResourcePropertyBag::EnumerateDirty(nsTHashtable<nsUint32HashKey>::Enumerator aEnumFunc, 
                                                   void *aClosure, 
                                                   PRUint32 *aDirtyCount)
{
  NS_ENSURE_ARG_POINTER(aClosure);
  NS_ENSURE_ARG_POINTER(aDirtyCount);

  *aDirtyCount = mDirty.EnumerateEntries(aEnumFunc, aClosure);
  return NS_OK;
}

nsresult
sbLocalDatabaseResourcePropertyBag::SetDirty(PRBool aDirty)
{
  nsAutoLock lock(mDirtyLock);

  if(mWritePending && !aDirty) {
    mDirty.Clear();
  }

  mWritePending = aDirty;

  return NS_OK;
}

