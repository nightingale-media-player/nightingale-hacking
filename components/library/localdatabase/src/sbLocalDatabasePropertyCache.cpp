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
#include <sbILibraryManager.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>
#include <sbProxyUtils.h>

#include <DatabaseQuery.h>
#include <nsAutoLock.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsIObserverService.h>
#include <nsIProxyObjectManager.h>
#include <nsServiceManagerUtils.h>
#include <nsStringEnumerator.h>
#include <nsIThreadManager.h>
#include <nsThreadUtils.h>
#include <nsUnicharUtils.h>
#include <nsXPCOM.h>
#include <prlog.h>

#include "sbDatabaseResultStringEnumerator.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseSchemaInfo.h"
#include <sbTArrayStringEnumerator.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabasePropertyCache:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo *gLocalDatabasePropertyCacheLog = nsnull;
#endif

#define TRACE(args) PR_LOG(gLocalDatabasePropertyCacheLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLocalDatabasePropertyCacheLog, PR_LOG_WARN, args)

/**
 * \brief Max number of pending changes before automatic cache write.
 */
#define SB_LOCALDATABASE_MAX_PENDING_CHANGES (500)

/**
 * \brief Number of milliseconds after the last write to force a cache write
 */
#define SB_LOCALDATABASE_CACHE_FLUSH_DELAY (500)

#define CACHE_HASHTABLE_SIZE 1000
#define BAG_HASHTABLE_SIZE   50

#define NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID "xpcom-shutdown-threads"

NS_IMPL_THREADSAFE_ISUPPORTS2(sbLocalDatabasePropertyCache,
                              sbILocalDatabasePropertyCache,
                              nsIObserver)

sbLocalDatabasePropertyCache::sbLocalDatabasePropertyCache()
: mWritePendingCount(0),
  mPropertiesInCriterion(nsnull),
  mMediaItemsInCriterion(nsnull),
  mIsShuttingDown(PR_FALSE),
  mFlushThreadMonitor(nsnull),
  mLibrary(nsnull)
{
  MOZ_COUNT_CTOR(sbLocalDatabasePropertyCache);
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
  if(mDirtyLock) {
    nsAutoLock::DestroyLock(mDirtyLock);
  }
  if (mFlushThreadMonitor) {
    nsAutoMonitor::DestroyMonitor(mFlushThreadMonitor);
  }

  MOZ_COUNT_DTOR(sbLocalDatabasePropertyCache);
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::GetWritePending(PRBool *aWritePending)
{
  NS_ASSERTION(mLibrary, "You didn't initalize!");
  NS_ENSURE_ARG_POINTER(aWritePending);
  *aWritePending = (mWritePendingCount > 0);

  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::Init(sbLocalDatabaseLibrary* aLibrary,
                                   const nsAString& aLibraryResourceGUID)
{
  NS_ASSERTION(!mLibrary, "Already initalized!");
  NS_ENSURE_ARG_POINTER(aLibrary);

  mLibraryResourceGUID = aLibraryResourceGUID;

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

  nsCOMPtr<sbISQLBuilderCriterionIn> inCriterion;
  rv = mPropertiesSelect->CreateMatchCriterionIn(EmptyString(),
                                                 NS_LITERAL_STRING("guid"),
                                                 getter_AddRefs(inCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPropertiesSelect->AddCriterion(inCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  // Keep a non-owning reference to this
  mPropertiesInCriterion = inCriterion;

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

  rv = mMediaItemsSelect->AddColumn(EmptyString(), NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    rv = mMediaItemsSelect->AddColumn(EmptyString(),
                                      nsDependentString(sStaticProperties[i].mColumn));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mMediaItemsSelect->CreateMatchCriterionIn(EmptyString(),
                                                 NS_LITERAL_STRING("guid"),
                                                 getter_AddRefs(inCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsSelect->AddCriterion(inCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  // Keep a non-owning reference to this
  mMediaItemsInCriterion = inCriterion;

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
  //  INSERT OR REPLACE INTO resource_properties (guid, property_id, obj, obj_sortable) VALUES (?, ?, ?, ?)

  nsCOMPtr<sbISQLInsertBuilder> propertiesInsert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->SetIntoTableName(NS_LITERAL_STRING("resource_properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->AddColumn(NS_LITERAL_STRING("guid"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->AddColumn(NS_LITERAL_STRING("property_id"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->AddColumn(NS_LITERAL_STRING("obj"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->AddColumn(NS_LITERAL_STRING("obj_sortable"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propertiesInsert->ToString(mPropertiesInsertOrReplace);
  NS_ENSURE_SUCCESS(rv, rv);

  // HACK: The query builder does not support the INSERT OR REPLACE syntax
  // so we jam it in here
  mPropertiesInsertOrReplace.Replace(0,
                                     6,
                                     NS_LITERAL_STRING("insert or replace"));

  // Create media item and library media item property update queries, one for
  // each static property
  PRBool success = mMediaItemsUpdateQueries.Init(sStaticPropertyCount);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mLibraryMediaItemUpdateQueries.Init(sStaticPropertyCount);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {

    PRUint32 propertyId = sStaticProperties[i].mDBID;

    nsCOMPtr<sbISQLUpdateBuilder> update =
      do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = update->SetTableName(NS_LITERAL_STRING("media_items"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString column;
    SB_GetTopLevelPropertyColumn(propertyId, column);

    rv = update->AddAssignmentParameter(column);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbISQLBuilderCriterion> criterion;
    rv = update->CreateMatchCriterionParameter(EmptyString(),
                                               NS_LITERAL_STRING("guid"),
                                               sbISQLBuilder::MATCH_EQUALS,
                                               getter_AddRefs(criterion));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = update->AddCriterion(criterion);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString sql;
    rv = update->ToString(sql);
    NS_ENSURE_SUCCESS(rv, rv);

    success = mMediaItemsUpdateQueries.Put(propertyId, sql);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    // Modify the query to update the library_media_item table
    rv = update->SetTableName(NS_LITERAL_STRING("library_media_item"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = update->ToString(sql);
    NS_ENSURE_SUCCESS(rv, rv);

    success = mLibraryMediaItemUpdateQueries.Put(propertyId, sql);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  // Create media item property delete query
  nsCOMPtr<sbISQLDeleteBuilder> deleteb =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->SetTableName(NS_LITERAL_STRING("resource_properties"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  rv = deleteb->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("guid"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->CreateMatchCriterionParameter(EmptyString(),
                                              NS_LITERAL_STRING("property_id"),
                                              sbISQLSelectBuilder::MATCH_EQUALS,
                                              getter_AddRefs(criterion));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = deleteb->AddCriterion(criterion);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteb->ToString(mPropertiesDelete);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = LoadProperties();
  NS_ENSURE_SUCCESS(rv, rv);

  success = mCache.Init(CACHE_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mDirty.Init(CACHE_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIThreadManager> threadMan =
    do_GetService(NS_THREADMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mFlushThreadMonitor = nsAutoMonitor::NewMonitor("sbLocalDatabasePropertyCache::mFlushThreadMonitor");
  NS_ENSURE_TRUE(mFlushThreadMonitor, NS_ERROR_OUT_OF_MEMORY);
  rv = threadMan->NewThread(0, getter_AddRefs(mFlushThread) );
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbLocalDatabasePropertyCache, this, RunFlushThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);
  rv = mFlushThread->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);
  LOG(("started background thread"));

  mLibrary = aLibrary;

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::Shutdown()
{
  if (mFlushThread) {
    {
      // tell the flush thread to finish
      nsAutoMonitor mon(mFlushThreadMonitor);
      mIsShuttingDown = PR_TRUE;
      mon.Notify();
    }
    
    // let the thread join
    mFlushThread->Shutdown();
    mFlushThread = nsnull;
  }

  if(mWritePendingCount) {
    return Write();
  }

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
  PRInt32 dbOk;
  nsString sql;

  // First, collect all the guids that are not cached
  nsTArray<nsString> misses;
  PRBool cacheLibraryMediaItem = PR_FALSE;
  for (PRUint32 i = 0; i < aGUIDArrayCount; i++) {

    nsDependentString guid(aGUIDArray[i]);

    if (!mCache.Get(guid, nsnull)) {

      if (guid.Equals(mLibraryResourceGUID)) {
        cacheLibraryMediaItem = PR_TRUE;
      }

      nsString* newElement = misses.AppendElement(guid);
      NS_ENSURE_TRUE(newElement, NS_ERROR_OUT_OF_MEMORY);

      nsRefPtr<sbLocalDatabaseResourcePropertyBag> newBag
        (new sbLocalDatabaseResourcePropertyBag(this, guid));
      NS_ENSURE_TRUE(newBag, NS_ERROR_OUT_OF_MEMORY);

      rv = newBag->Init();
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool success = mCache.Put(guid, newBag);
      NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  // Look up and cache each of the misses
  PRUint32 numMisses = misses.Length();
  if (numMisses > 0) {
    PRUint32 inNum = 0;
    for (PRUint32 j = 0; j < numMisses; j++) {

      // Add each guid to the query and execute the query when we've added
      // MAX_IN_LENGTH of them (or when we are on the last one)
      rv = mPropertiesInCriterion->AddString(misses[j]);
      NS_ENSURE_SUCCESS(rv, rv);

      if (inNum > MAX_IN_LENGTH || j + 1 == numMisses) {
        rv = mPropertiesSelect->ToString(sql);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIDatabaseQuery> query;
        rv = MakeQuery(sql, getter_AddRefs(query));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = query->Execute(&dbOk);
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

        nsCOMPtr<sbIDatabaseResult> result;
        rv = query->GetResultObject(getter_AddRefs(result));
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 rowCount;
        rv = result->GetRowCount(&rowCount);
        NS_ENSURE_SUCCESS(rv, rv);

        for (PRUint32 row = 0; row < rowCount; row++) {
          PRUnichar* guid;
          rv = result->GetRowCellPtr(row, 0, &guid);
          NS_ENSURE_SUCCESS(rv, rv);

          nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
          PRBool success = mCache.Get(nsDependentString(guid), getter_AddRefs(bag));
          NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

          // Add each property / object pair to the current bag
          nsAutoString propertyIDStr;
          rv = result->GetRowCell(row, 1, propertyIDStr);
          NS_ENSURE_SUCCESS(rv, rv);

          PRUint32 propertyID = propertyIDStr.ToInteger(&rv);
          NS_ENSURE_SUCCESS(rv, rv);

          nsAutoString obj;
          rv = result->GetRowCell(row, 2, obj);
          NS_ENSURE_SUCCESS(rv, rv);

          // XXXben FIX ME
          sbLocalDatabaseResourcePropertyBag* bagClassPtr =
            static_cast<sbLocalDatabaseResourcePropertyBag*>(bag.get());
          rv = bagClassPtr->PutValue(propertyID, obj);

          NS_ENSURE_SUCCESS(rv, rv);
        }

        mPropertiesInCriterion->Clear();
        NS_ENSURE_SUCCESS(rv, rv);
      }

    }

    // Do the same thing for top level properties
    rv = mMediaItemsSelect->SetBaseTableName(NS_LITERAL_STRING("media_items"));
    NS_ENSURE_SUCCESS(rv, rv);

    inNum = 0;
    for (PRUint32 j = 0; j < numMisses; j++) {

      // Skip the library resource guid
      if (!misses[j].Equals(mLibraryResourceGUID)) {
        rv = mMediaItemsInCriterion->AddString(misses[j]);
        NS_ENSURE_SUCCESS(rv, rv);
      }

      if (inNum > MAX_IN_LENGTH || j + 1 == numMisses) {
        rv = mMediaItemsSelect->ToString(sql);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<sbIDatabaseQuery> query;
        rv = MakeQuery(sql, getter_AddRefs(query));
        NS_ENSURE_SUCCESS(rv, rv);

        rv = query->Execute(&dbOk);
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

        nsCOMPtr<sbIDatabaseResult> result;
        rv = query->GetResultObject(getter_AddRefs(result));
        NS_ENSURE_SUCCESS(rv, rv);

        PRUint32 rowCount;
        rv = result->GetRowCount(&rowCount);
        NS_ENSURE_SUCCESS(rv, rv);

        for (PRUint32 row = 0; row < rowCount; row++) {
          nsAutoString guid;
          rv = result->GetRowCell(row, 0, guid);
          NS_ENSURE_SUCCESS(rv, rv);

          nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
          PRBool success = mCache.Get(guid, getter_AddRefs(bag));
          NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

          for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
            nsAutoString value;
            rv = result->GetRowCell(row, i + 1, value);
            NS_ENSURE_SUCCESS(rv, rv);

            if (!value.IsVoid()) {

              // XXXben FIX ME
              sbLocalDatabaseResourcePropertyBag* bagClassPtr =
                static_cast<sbLocalDatabaseResourcePropertyBag*>(bag.get());
              rv = bagClassPtr->PutValue(sStaticProperties[i].mDBID, value);

              NS_ENSURE_SUCCESS(rv, rv);
            }
          }

        }

        mMediaItemsInCriterion->Clear();
        NS_ENSURE_SUCCESS(rv, rv);
      }

    }

  }

  // Cache the library's property data from library_media_item
  if (cacheLibraryMediaItem) {
    rv = mMediaItemsSelect->SetBaseTableName(NS_LITERAL_STRING("library_media_item"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMediaItemsInCriterion->AddString(mLibraryResourceGUID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMediaItemsSelect->ToString(sql);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDatabaseQuery> query;
    rv = MakeQuery(sql, getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->Execute(&dbOk);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

    nsCOMPtr<sbIDatabaseResult> result;
    rv = query->GetResultObject(getter_AddRefs(result));
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 rowCount;
    rv = result->GetRowCount(&rowCount);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(rowCount == 1, "Failed to get data from library_media_item");

    nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
    PRBool success = mCache.Get(mLibraryResourceGUID, getter_AddRefs(bag));
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

    for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
      nsString value;
      rv = result->GetRowCell(0, i + 1, value);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!value.IsVoid()) {

        // XXXben FIX ME
        sbLocalDatabaseResourcePropertyBag* bagClassPtr =
          static_cast<sbLocalDatabaseResourcePropertyBag*>(bag.get());
        rv = bagClassPtr->PutValue(sStaticProperties[i].mDBID, value);

        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    mMediaItemsInCriterion->Clear();
    NS_ENSURE_SUCCESS(rv, rv);
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

  if (!aGUIDArrayCount) {
    NS_WARNING("Asked for 0 properties in call to GetProperties!");
    *aPropertyArrayCount = 0;
    *aPropertyArray = nsnull;
    return NS_OK;
  }

  // Build the output array using cache lookups
  rv = CacheProperties(aGUIDArray, aGUIDArrayCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // A lot of letters just to allocate the correct number of pointers...
  nsAutoPtr<sbILocalDatabaseResourcePropertyBag*> propertyBagArray(
    static_cast<sbILocalDatabaseResourcePropertyBag**> (
      NS_Alloc(sizeof(sbILocalDatabaseResourcePropertyBag*) * aGUIDArrayCount)));
  NS_ENSURE_TRUE(propertyBagArray, NS_ERROR_OUT_OF_MEMORY);

  for (PRUint32 i = 0; i < aGUIDArrayCount; i++) {
    // This will either set and addref or set to null.
    mCache.Get(nsDependentString(aGUIDArray[i]), &propertyBagArray[i]);
  }

  *aPropertyArrayCount = aGUIDArrayCount;
  *aPropertyArray = propertyBagArray.forget();
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
    nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;

    if(mCache.Get(guid, getter_AddRefs(bag)) && bag) {
      nsCOMPtr<nsIStringEnumerator> ids;
      rv = aPropertyArray[i]->GetIds(getter_AddRefs(ids));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool hasMore = PR_FALSE;
      nsString id, value;

      while(NS_SUCCEEDED(ids->HasMore(&hasMore)) && hasMore) {
        rv = ids->GetNext(id);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = aPropertyArray[i]->GetProperty(id, value);
        NS_ENSURE_SUCCESS(rv, rv);

        // XXXben FIX ME
        rv = static_cast<sbLocalDatabaseResourcePropertyBag*>(bag.get())
          ->SetProperty(id, value);
      }

      PR_Lock(mDirtyLock);
      mDirty.PutEntry(guid);
      PR_Unlock(mDirtyLock);

      if(++mWritePendingCount > SB_LOCALDATABASE_MAX_PENDING_CHANGES) {
        rv = Write();
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  if(aWriteThroughNow) {
    rv = Write();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

PR_STATIC_CALLBACK(PLDHashOperator)
EnumDirtyGuids(nsStringHashKey *aKey, void *aClosure)
{
  nsTArray<nsString> *dirtyGuids = static_cast<nsTArray<nsString> *>(aClosure);
  dirtyGuids->AppendElement(aKey->GetKey());
  return PL_DHASH_NEXT;
}

PR_STATIC_CALLBACK(PLDHashOperator)
EnumDirtyProps(nsUint32HashKey *aKey, void *aClosure)
{
  nsTArray<PRUint32> *dirtyProps = static_cast<nsTArray<PRUint32> *>(aClosure);
  dirtyProps->AppendElement(aKey->GetKey());
  return PL_DHASH_NEXT;
}

// note: this might be called either from the main thread (for a forced write)
// or a background thread (from the flush thread)
NS_IMETHODIMP
sbLocalDatabasePropertyCache::Write()
{
  NS_ASSERTION(mLibrary, "You didn't initalize!");
  
  nsresult rv = NS_OK;

  { // deal with any queued up queries (to make sure we commit in order)
    nsCOMPtr<sbIDatabaseQuery> query;
    nsAutoMonitor mon(mFlushThreadMonitor);
    while (!mUnflushedQueries.IsEmpty()) {
      PRInt32 dbOk;
      query = mUnflushedQueries[0].query;
    
      rv = query->Execute(&dbOk);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);
      
      rv = query->WaitForCompletion(&dbOk);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);
      
      mLibrary->IncrementDatabaseDirtyItemCounter(mUnflushedQueries[0].dirtyGuidCount);
      
      mUnflushedQueries.RemoveElementAt(0);
    }
  }

  nsCOMPtr<sbIDatabaseQuery> query;
  PRUint32 dirtyGuidCount;
  { // find the new dirty properties
    nsTArray<nsString> dirtyGuids;
  
    //Lock it.
    nsAutoLock lock(mDirtyLock);
  
    //Enumerate dirty GUIDs
    dirtyGuidCount = mDirty.EnumerateEntries(EnumDirtyGuids, (void *) &dirtyGuids);
  
    if (!dirtyGuidCount)
      return NS_OK;
  
    rv = MakeQuery(NS_LITERAL_STRING("begin"), getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);

  
    //For each GUID, there's a property bag that needs to be processed as well.
    for(PRUint32 i = 0; i < dirtyGuidCount; i++) {
      nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
      if (mCache.Get(dirtyGuids[i], getter_AddRefs(bag))) {
        nsTArray<PRUint32> dirtyProps;

        // XXXben FIX ME
        sbLocalDatabaseResourcePropertyBag* bagLocal =
          static_cast<sbLocalDatabaseResourcePropertyBag *>(bag.get());

        PRUint32 dirtyPropsCount = 0;
        rv = bagLocal->EnumerateDirty(EnumDirtyProps, (void *) &dirtyProps, &dirtyPropsCount);
        NS_ENSURE_SUCCESS(rv, rv);

        //Enumerate dirty properties for this GUID.
        nsAutoString value;
        for(PRUint32 j = 0; j < dirtyPropsCount; j++) {
          nsString sql, propertyID;

          //If this isn't true, something is very wrong, so bail out.
          PRBool success = GetPropertyID(dirtyProps[j], propertyID);
          NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

          // Never change the guid
          if (propertyID.EqualsLiteral(SB_PROPERTY_GUID)) {
            continue;
          }

          rv = bagLocal->GetPropertyByID(dirtyProps[j], value);
          NS_ENSURE_SUCCESS(rv, rv);

          //Top level properties need to be treated differently, so check for them.
          if(SB_IsTopLevelProperty(dirtyProps[j])) {
  
            // Switch the query if we are updating the library resource
            nsString sql;
            if (dirtyGuids[i].Equals(mLibraryResourceGUID)) {
              PRBool found = mLibraryMediaItemUpdateQueries.Get(dirtyProps[j],
                                                                &sql);
              NS_ENSURE_TRUE(found, NS_ERROR_UNEXPECTED);
            }
            else {
              PRBool found = mMediaItemsUpdateQueries.Get(dirtyProps[j], &sql);
              NS_ENSURE_TRUE(found, NS_ERROR_UNEXPECTED);
            }

            rv = query->AddQuery(sql);
            NS_ENSURE_SUCCESS(rv, rv);
  
            rv = query->BindStringParameter(0, value);
            NS_ENSURE_SUCCESS(rv, rv);
  
            rv = query->BindStringParameter(1, dirtyGuids[i]);
            NS_ENSURE_SUCCESS(rv, rv);
          }
          else { //Regular properties all go in the same spot.
            if (value.IsVoid()) {
              rv = query->AddQuery(mPropertiesDelete);
              NS_ENSURE_SUCCESS(rv, rv);
  
              rv = query->BindStringParameter(0, dirtyGuids[i]);
              NS_ENSURE_SUCCESS(rv, rv);
  
              rv = query->BindInt32Parameter(1, dirtyProps[j]);
              NS_ENSURE_SUCCESS(rv, rv);
            }
            else {
              nsCOMPtr<sbIPropertyInfo> propertyInfo;
              rv = mPropertyManager->GetPropertyInfo(propertyID,
                                                     getter_AddRefs(propertyInfo));
              NS_ENSURE_SUCCESS(rv, rv);
  
              nsString sortable;
              rv = propertyInfo->MakeSortable(value, sortable);
              NS_ENSURE_SUCCESS(rv, rv);
    
              rv = query->AddQuery(mPropertiesInsertOrReplace);
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
          }
        }
      }
    }
    rv = query->AddQuery(NS_LITERAL_STRING("commit"));
    NS_ENSURE_SUCCESS(rv, rv);

    // and mark the items as not dirty
    for(PRUint32 i = 0; i < dirtyGuidCount; i++) {
      nsCOMPtr<sbILocalDatabaseResourcePropertyBag> bag;
      if (mCache.Get(dirtyGuids[i], getter_AddRefs(bag))) {
  
        // XXXben FIX ME
        sbLocalDatabaseResourcePropertyBag* bagLocal =
          static_cast<sbLocalDatabaseResourcePropertyBag *>(bag.get());
        bagLocal->SetDirty(PR_FALSE);
      }
    }

    //Clear dirty guid hastable.
    mDirty.Clear();
  }
  
  // push the new query to the queue (in case it fails, we can retry)
  // (all this is to make sure we avoid holding mDirtyLock while waiting
  //  for the query to complete)
  {
    nsAutoMonitor mon(mFlushThreadMonitor);
    FlushQueryData *newData = mUnflushedQueries.AppendElement();
    newData->query = query;
    newData->dirtyGuidCount = dirtyGuidCount;
  }
    

  //  now we can actually attempt to commit
  {
    nsAutoMonitor mon(mFlushThreadMonitor);

    while (!mUnflushedQueries.IsEmpty()) {
      PRInt32 dbOk;
      query = mUnflushedQueries[0].query;
    
      rv = query->Execute(&dbOk);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);
      
      rv = query->WaitForCompletion(&dbOk);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);
      
      mLibrary->IncrementDatabaseDirtyItemCounter(mUnflushedQueries[0].dirtyGuidCount);
      
      mUnflushedQueries.RemoveElementAt(0);
    }
  }

  return rv;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::GetPropertyDBID(const nsAString& aPropertyID,
                                              PRUint32* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = GetPropertyDBIDInternal(aPropertyID);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::Observe(nsISupports* aSubject,
                                      const char* aTopic,
                                      const PRUnichar* aData)
{
  if (strcmp(aTopic, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC) == 0 ||
      strcmp(aTopic, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID) == 0) {

    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);

    if (NS_SUCCEEDED(rv)) {
      observerService->RemoveObserver(this, aTopic);
    }

    Shutdown();
  }

  return NS_OK;
}

void
sbLocalDatabasePropertyCache::RunFlushThread()
{
  const PRIntervalTime timeout =
    PR_MillisecondsToInterval(SB_LOCALDATABASE_CACHE_FLUSH_DELAY);
  while (PR_TRUE) {
    nsAutoMonitor mon(mFlushThreadMonitor);
    nsresult rv = mon.Wait(timeout);
    if (NS_FAILED(rv)) {
      // some other thread has acquired the monitor while the timeout expired
      // don't write, go back and try to re-acquire the monitor
      continue;
    }
    if (mIsShuttingDown) {
      // shutting down, stop this thread
      // (flush will happen on main thread in Shutdown())
      break;
    }
    rv = Write();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to flush property cache; will retry");
  }
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

  if (!mPropertyIDToDBID.IsInitialized()) {
    PRBool success = mPropertyIDToDBID.Init(100);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    mPropertyIDToDBID.Clear();
  }

  if (!mPropertyDBIDToID.IsInitialized()) {
    PRBool success = mPropertyDBIDToID.Init(100);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    mPropertyDBIDToID.Clear();
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
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  rv = query->WaitForCompletion(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 i = 0; i < rowCount; i++) {
    nsAutoString propertyDBIDStr;
    rv = result->GetRowCell(i, 0, propertyDBIDStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyDBID = propertyDBIDStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString propertyID;
    rv = result->GetRowCell(i, 1, propertyID);
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = mPropertyDBIDToID.Put(propertyDBID, propertyID);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    TRACE(("Added %d => %s to property name cache", propertyDBID,
           NS_ConvertUTF16toUTF8(propertyID).get()));

    success = mPropertyIDToDBID.Put(propertyID, propertyDBID);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  }

  /*
   * Add top level properties
   */
  for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {

    nsString propertyID(sStaticProperties[i].mPropertyID);
    PRBool success = mPropertyDBIDToID.Put(sStaticProperties[i].mDBID, propertyID);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    success = mPropertyIDToDBID.Put(nsDependentString(sStaticProperties[i].mPropertyID),
                                    sStaticProperties[i].mDBID);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

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

  mWritePendingCount++;

  return NS_OK;
}

PRUint32
sbLocalDatabasePropertyCache::GetPropertyDBIDInternal(const nsAString& aPropertyID)
{
  PRUint32 retval;
  if (!mPropertyIDToDBID.Get(aPropertyID, &retval)) {
    nsresult rv = InsertPropertyIDInLibrary(aPropertyID, &retval);

    if(NS_FAILED(rv)) {
      retval = 0;
    }

  }
  return retval;
}

PRBool
sbLocalDatabasePropertyCache::GetPropertyID(PRUint32 aPropertyDBID,
                                            nsAString& aPropertyID)
{
  nsString propertyID;
  if (mPropertyDBIDToID.Get(aPropertyDBID, &propertyID)) {
    aPropertyID = propertyID;
    return PR_TRUE;
  }
  return PR_FALSE;
}

PR_STATIC_CALLBACK(PLDHashOperator)
PropertyIDToNameKeys(nsUint32HashKey::KeyType aPropertyID,
                     nsString& aValue,
                     void *aArg)
{
  nsTArray<PRUint32>* propertyIDs = static_cast<nsTArray<PRUint32>*>(aArg);
  if (propertyIDs->AppendElement(aPropertyID)) {
    return PL_DHASH_NEXT;
  }
  else {
    return PL_DHASH_STOP;
  }
}

nsresult
sbLocalDatabasePropertyCache::InsertPropertyIDInLibrary(const nsAString& aPropertyID,
                                                        PRUint32 *aPropertyDBID)
{
  NS_ENSURE_ARG_POINTER(aPropertyDBID);
  nsAutoString sql;

  nsresult rv = mPropertiesTableInsert->ToString(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(sql, getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindStringParameter(0, aPropertyID);
  NS_ENSURE_SUCCESS(rv, rv);

  sql.AssignLiteral("select last_insert_rowid()");
  rv = query->AddQuery(sql);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  rv = query->WaitForCompletion(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString propertyDBIDStr;
  rv = result->GetRowCell(0, 0, propertyDBIDStr);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 propertyDBID = propertyDBIDStr.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  *aPropertyDBID = propertyDBID;

  mPropertyDBIDToID.Put(propertyDBID, nsAutoString(aPropertyID));
  mPropertyIDToDBID.Put(nsAutoString(aPropertyID), propertyDBID);

  return NS_OK;
}

// sbILocalDatabaseResourcePropertyBag
NS_IMPL_THREADSAFE_ISUPPORTS1(sbLocalDatabaseResourcePropertyBag,
                              sbILocalDatabaseResourcePropertyBag)

sbLocalDatabaseResourcePropertyBag::sbLocalDatabaseResourcePropertyBag(sbLocalDatabasePropertyCache* aCache,
                                                                       const nsAString &aGuid)
: mCache(aCache)
, mWritePendingCount(0)
, mGuid(aGuid)
, mDirtyLock(nsnull)
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

  PRBool success = mValueMap.Init(BAG_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  success = mDirty.Init(BAG_HASHTABLE_SIZE);
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
  nsTArray<PRUint32>* propertyIDs = static_cast<nsTArray<PRUint32>*>(aArg);
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
  *aWritePending = (mWritePendingCount > 0);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetIds(nsIStringEnumerator **aIDs)
{
  NS_ENSURE_ARG_POINTER(aIDs);

  nsTArray<PRUint32> propertyDBIDs;
  mValueMap.EnumerateRead(PropertyBagKeysToArray, &propertyDBIDs);

  PRUint32 len = mValueMap.Count();
  if (propertyDBIDs.Length() < len) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  nsTArray<nsString> propertyIDs;
  for (PRUint32 i = 0; i < len; i++) {
    nsString propertyID;
    PRBool success = mCache->GetPropertyID(propertyDBIDs[i], propertyID);
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);
    propertyIDs.AppendElement(propertyID);
  }

  *aIDs = new sbTArrayStringEnumerator(&propertyIDs);
  NS_ENSURE_TRUE(*aIDs, NS_ERROR_OUT_OF_MEMORY);
  NS_ADDREF(*aIDs);

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetProperty(const nsAString& aPropertyID,
                                                nsAString& _retval)
{
  PRUint32 propertyDBID = mCache->GetPropertyDBIDInternal(aPropertyID);
  return GetPropertyByID(propertyDBID, _retval);
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::GetPropertyByID(PRUint32 aPropertyDBID,
                                                    nsAString& _retval)
{
  if(aPropertyDBID > 0) {
    nsAutoLock lock(mDirtyLock);
    nsString* value;

    if (mValueMap.Get(aPropertyDBID, &value)) {
      _retval = *value;
      return NS_OK;
    }
  }

  // The value hasn't been set, so return a void string.
  _retval.SetIsVoid(PR_TRUE);
  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::SetProperty(const nsAString & aPropertyID,
                                                const nsAString & aValue)
{
  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  PRUint32 propertyDBID = mCache->GetPropertyDBIDInternal(aPropertyID);

  nsresult rv;
  rv = mPropertyManager->GetPropertyInfo(aPropertyID,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool valid = PR_FALSE;
  rv = propertyInfo->Validate(aValue, &valid);
  NS_ENSURE_SUCCESS(rv, rv);

#if defined(PR_LOGGING)
  if(NS_UNLIKELY(!valid)) {
    LOG(("Failed to set property %s with value %s",
      NS_ConvertUTF16toUTF8(aPropertyID).get(),
      NS_ConvertUTF16toUTF8(aValue).get()));
  }
#endif

  NS_ENSURE_TRUE(valid, NS_ERROR_ILLEGAL_VALUE);

  if(propertyDBID == 0) {
    rv = mCache->InsertPropertyIDInLibrary(aPropertyID, &propertyDBID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return SetPropertyByDBID(propertyDBID, aValue);
}

NS_IMETHODIMP
sbLocalDatabaseResourcePropertyBag::SetPropertyByDBID(PRUint32 aPropertyDBID,
                                                      const nsAString & aValue)
{
  nsresult rv = NS_ERROR_INVALID_ARG;
  if(aPropertyDBID > 0) {
    rv = PutValue(aPropertyDBID, aValue);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mCache->AddDirtyGUID(mGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    PR_Lock(mDirtyLock);
    mDirty.PutEntry(aPropertyDBID);
    PR_Unlock(mDirtyLock);

    if(++mWritePendingCount > SB_LOCALDATABASE_MAX_PENDING_CHANGES) {
      rv = Write();
    }
  }

  return rv;
}

NS_IMETHODIMP sbLocalDatabaseResourcePropertyBag::Write()
{
  nsresult rv = NS_OK;

  if(mWritePendingCount) {
    rv = mCache->Write();
    NS_ENSURE_SUCCESS(rv, rv);

    mWritePendingCount = 0;
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
  nsAutoLock lockDirty(mDirtyLock);

  if(!aDirty) {
    mDirty.Clear();
    mWritePendingCount = 0;
  }
  else {
    //Looks like we're attempting to force a flush on the next property that gets set.
    //Ensure we trigger a write.
    mWritePendingCount = SB_LOCALDATABASE_MAX_PENDING_CHANGES + 1;
  }

  return NS_OK;
}
