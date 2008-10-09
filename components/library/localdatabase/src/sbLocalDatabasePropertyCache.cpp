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

#include "sbLocalDatabasePropertyCache.h"

#include <sbIDatabaseQuery.h>
#include <sbILibraryManager.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>

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
#include <nsXPCOMCIDInternal.h>
#include <prlog.h>

#include "sbDatabaseResultStringEnumerator.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseResourcePropertyBag.h"
#include "sbLocalDatabaseSchemaInfo.h"
#include "sbLocalDatabaseSQL.h"
#include <sbTArrayStringEnumerator.h>
#include <sbStringUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabasePropertyCache:5
 */
#ifdef PR_LOGGING
PRLogModuleInfo *gLocalDatabasePropertyCacheLog = nsnull;
#endif

#define TRACE(args) PR_LOG(gLocalDatabasePropertyCacheLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gLocalDatabasePropertyCacheLog, PR_LOG_WARN, args)

/**
 * \brief Number of milliseconds after the last write to force a cache write
 */
#define SB_LOCALDATABASE_CACHE_FLUSH_DELAY (1000)

#define CACHE_HASHTABLE_SIZE 1000

#define NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID "xpcom-shutdown-threads"

NS_IMPL_THREADSAFE_ISUPPORTS2(sbLocalDatabasePropertyCache,
                              sbILocalDatabasePropertyCache,
                              nsIObserver)

sbLocalDatabasePropertyCache::sbLocalDatabasePropertyCache()
: mWritePendingCount(0),
  mCache(sbLocalDatabasePropertyCache::CACHE_SIZE),
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

  mCacheLock = nsAutoLock::NewLock("sbLocalDatabasePropertyCache::mCacheLock");
  NS_ASSERTION(mCacheLock, "sbLocalDatabasePropertyCache::mCacheLock failed to create lock!");

  mDirtyLock = nsAutoLock::NewLock("sbLocalDatabasePropertyCache::mDirtyLock");
  NS_ASSERTION(mDirtyLock,
    "sbLocalDatabasePropertyCache::mDirtyLock failed to create lock!");
}

sbLocalDatabasePropertyCache::~sbLocalDatabasePropertyCache()
{
  if (mCacheLock) {
    nsAutoLock::DestroyLock(mCacheLock);
  }
  if (mDirtyLock) {
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
  NS_ASSERTION(mLibrary, "You didn't initialize!");
  NS_ENSURE_ARG_POINTER(aWritePending);
  *aWritePending = (mWritePendingCount > 0);

  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::Init(sbLocalDatabaseLibrary* aLibrary,
                                   const nsAString& aLibraryResourceGUID)
{
  NS_ASSERTION(!mLibrary, "Already initialized!");
  NS_ENSURE_ARG_POINTER(aLibrary);

  mLibraryResourceGUID = aLibraryResourceGUID;

  nsresult rv = aLibrary->GetDatabaseGuid(mDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aLibrary->GetDatabaseLocation(getter_AddRefs(mDatabaseLocation));
  NS_ENSURE_SUCCESS(rv, rv);

  mPropertyManager = do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  /* XXXAus: resource_properties_fts is disabled. See bug 9488 and bug 9617
             for more information.

  NS_NAMED_LITERAL_STRING(kResourceProperties, "resource_properties");
  NS_NAMED_LITERAL_STRING(kResourcePropertiesAlias, "rp");
  NS_NAMED_LITERAL_STRING(kMediaItems, "media_items");
  NS_NAMED_LITERAL_STRING(kMediaItemsAlias, "mi");
  NS_NAMED_LITERAL_STRING(kLibraryMediaItem, "library_media_item");
  NS_NAMED_LITERAL_STRING(kResourcePropertiesFts, "resource_properties_fts");
  NS_NAMED_LITERAL_STRING(kResourcePropertiesFtsAll, "resource_properties_fts_all");
  NS_NAMED_LITERAL_STRING(kGUID, "guid");
  NS_NAMED_LITERAL_STRING(kRowid, "rowid");
  NS_NAMED_LITERAL_STRING(kAllData, "alldata");
  NS_NAMED_LITERAL_STRING(kPropertyId, "property_id");
  NS_NAMED_LITERAL_STRING(kFtsPropertyId, "propertyid");
  NS_NAMED_LITERAL_STRING(kObj, "obj");
  NS_NAMED_LITERAL_STRING(kObjSortable, "obj_sortable");
  NS_NAMED_LITERAL_STRING(kMediaItemId, "media_item_id");

  // Media items delete fts query.  This query deletes all of the fts data
  // for the media items specified in the in criterion
   mMediaItemsFtsDelete =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsFtsDelete->SetTableName(kResourcePropertiesFts);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLSelectBuilder> subselect =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->AddColumn(EmptyString(), kRowid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->SetBaseTableName(kResourceProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->CreateMatchCriterionIn(EmptyString(),
                                         kMediaItemId,
                                         getter_AddRefs(inCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->AddCriterion(inCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  mMediaItemsFtsDeleteInCriterion = inCriterion;

  rv = mMediaItemsFtsDelete->CreateMatchCriterionIn(EmptyString(),
                                                    kRowid,
                                                    getter_AddRefs(inCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = inCriterion->AddSubquery(subselect);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsFtsDelete->AddCriterion(inCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  // Media items insert fts query.  This query inserts all of the fts data
  // for the media items specified in the in criterion
  mMediaItemsFtsInsert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsFtsInsert->SetIntoTableName(kResourcePropertiesFts);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsFtsInsert->AddColumn(kRowid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsFtsInsert->AddColumn(kFtsPropertyId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMediaItemsFtsInsert->AddColumn(kObj);
  NS_ENSURE_SUCCESS(rv, rv);

  subselect = do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->AddColumn(EmptyString(), kRowid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->AddColumn(EmptyString(), kPropertyId);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->AddColumn(EmptyString(), kObj);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->SetBaseTableName(kResourceProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->CreateMatchCriterionIn(EmptyString(),
                                         kMediaItemId,
                                         getter_AddRefs(inCriterion));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = subselect->AddCriterion(inCriterion);
  NS_ENSURE_SUCCESS(rv, rv);

  mMediaItemsFtsInsertInCriterion = inCriterion;

  rv = mMediaItemsFtsInsert->SetSelect(subselect);
  NS_ENSURE_SUCCESS(rv, rv);

    */

  rv = LoadProperties();
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mDirty.Init(CACHE_HASHTABLE_SIZE);
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
                                    SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC,
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

template <class T>
nsresult
sbLocalDatabasePropertyCache::RetrievePrimaryProperties(nsAString const & aSQLStatement,
                                                        T const & aGuids,
                                                        IDToBagMap & aIDToBagMap,
                                                        nsCOMArray<sbLocalDatabaseResourcePropertyBag> & aBags,
                                                        nsTArray<PRUint32> & aMissesIDs)
{
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeQuery(aSQLStatement,
                          getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure we got the number of guid's we expected
  NS_ENSURE_TRUE(aGuids.Length() == rowCount, NS_ERROR_UNEXPECTED);

  // Need to fill out the bag array as we won't be straight appending
  // but filling in as we find the guids
  aMissesIDs.SetLength(rowCount);

  // Variables declared outside of loops to be a little more efficient
  nsString mediaItemIdStr;
  nsString guid;
  nsString value;
  for (PRUint32 row = 0; row < rowCount; row++) {

    rv = result->GetRowCell(row, 0, mediaItemIdStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 mediaItemId = mediaItemIdStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = result->GetRowCell(row, 1, guid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<sbLocalDatabaseResourcePropertyBag> bag =
       new sbLocalDatabaseResourcePropertyBag(this, mediaItemId, guid);
    NS_ENSURE_TRUE(bag, NS_ERROR_OUT_OF_MEMORY);

    nsresult rv = bag->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
      rv = result->GetRowCell(row, i + 1, value);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!value.IsVoid()) {

        rv = bag->PutValue(sStaticProperties[i].mDBID, value, value);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    // Lookup where we should put this bag and it's ID
    PRInt32 const index = aGuids.IndexOf(guid);
    NS_ENSURE_TRUE(index >= 0, NS_ERROR_UNEXPECTED);

    aMissesIDs[index] = mediaItemId;

    PRBool const success = aIDToBagMap.Put(mediaItemId, bag);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    aBags.ReplaceObjectAt(bag, index);
  }
  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::RetrieveSecondaryProperties(nsAString const & aSQLStatement,
                                                          IDToBagMap const & bags)
{
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeQuery(aSQLStatement,
                          getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Temporaries declared outside of loop for minor perf gain
  nsString objSortable;
  nsString obj;
  nsString propertyIDStr;
  nsRefPtr<sbLocalDatabaseResourcePropertyBag> bag;
  for (PRUint32 row = 0; row < rowCount; row++) {

    nsString mediaItemIdStr;
    rv = result->GetRowCell(row, 0, mediaItemIdStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 mediaItemId = mediaItemIdStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    bags.Get(mediaItemId, getter_AddRefs(bag));
    NS_ENSURE_TRUE(bag, NS_ERROR_FAILURE);

    // Add each property / object pair to the current bag
    rv = result->GetRowCell(row, 1, propertyIDStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyID = propertyIDStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = result->GetRowCell(row, 2, obj);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = result->GetRowCell(row, 3, objSortable);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = bag->PutValue(propertyID, obj, objSortable);

    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::RetrieveLibraryProperties(nsAString const & aSQLStatement,
                                                        sbLocalDatabaseResourcePropertyBag * aBag)
{
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeQuery(aSQLStatement,
                          getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
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

    nsString propertyIDStr;
    rv = result->GetRowCell(row, 0, propertyIDStr);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 propertyID = propertyIDStr.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString obj;
    rv = result->GetRowCell(row, 1, obj);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString objSortable;
    rv = result->GetRowCell(row, 2, objSortable);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = aBag->PutValue(propertyID, obj, objSortable);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = MakeQuery(mSQLStrings.LibraryMediaItemSelect(),
                 getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(rowCount == 1, "Failed to get data from library_media_item");

  for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    nsString value;
    rv = result->GetRowCell(0, i, value);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!value.IsVoid()) {

      rv = aBag->PutValue(sStaticProperties[i].mDBID, value, value);

      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

template <class T>
nsresult sbLocalDatabasePropertyCache::RetrieveProperties(
  T & aGUIDs,
  nsCOMArray<sbLocalDatabaseResourcePropertyBag> & aBags)
{
  nsresult rv;
  PRBool const cacheLibraryMediaItem = aGUIDs.RemoveElement(mLibraryResourceGUID);
  // Start with the top level properties.  This pulls the data from the
  // media_items table for each requested guid.  This also serves to
  // translate the guid to a media_item_id so we can use the id for
  // the next lookup on the resource_properties table

  if (aGUIDs.Length() > 0) {
    // As we read in the data from media_item_ids, keep a map of the id to
    // property bag so we don't need guids in the second pass
    nsTArray<PRUint32> itemIDs(aGUIDs.Length());
    IDToBagMap idToBagMap;
    PRBool const success = idToBagMap.Init(aGUIDs.Length());
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    rv = RetrievePrimaryProperties(mSQLStrings.MediaItemSelect(aGUIDs),
                                   aGUIDs, idToBagMap, aBags, itemIDs);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(idToBagMap.Count() == aBags.Count() &&
                   aBags.Count() == itemIDs.Length(),
                 "RetrieveMediaItems returned an inconsistent result set");
    if (itemIDs.Length() == 0)
      return NS_OK;

    // Now do the data from resource_properties
    rv = RetrieveSecondaryProperties(sbLocalDatabaseSQL::SecondaryPropertySelect(itemIDs),
                                     idToBagMap);
  }
  // Cache the library's property data from library_media_item and
  // resource_properties
  if (cacheLibraryMediaItem) {
    nsRefPtr<sbLocalDatabaseResourcePropertyBag> bag =
      new sbLocalDatabaseResourcePropertyBag(this, 0, mLibraryResourceGUID);
    NS_ENSURE_TRUE(bag, NS_ERROR_OUT_OF_MEMORY);

    rv = bag->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = RetrieveLibraryProperties(sbLocalDatabaseSQL::LibraryMediaItemsPropertiesSelect(), bag);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ENSURE_TRUE(aBags.AppendObject(bag),
                   NS_ERROR_OUT_OF_MEMORY);
    NS_ENSURE_TRUE(aGUIDs.AppendElement(mLibraryResourceGUID),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

class PRUnicharAdaptor
{
public:
  PRUnicharAdaptor(PRUnichar const * * aCharArray,
                   PRUint32 aLength) : mCharArray(aCharArray), mLength(aLength)
  {
    NS_ASSERTION(aCharArray, "PRUnicharAdaptor must be not passed a null pointer");
  }
  nsString operator[](PRUint32 aIndex) const {
    NS_ASSERTION(aIndex < mLength,
                 "PRunicharAdaptor::operator [] index exceeds array length");
    return nsDependentString(mCharArray[aIndex]);
  }
  PRUint32 Length() const {
    return mLength;
  }
  PRBool RemoveElement(nsAString const & aItemToRemove)
  {
    for (PRUint32 index = 0; index < mLength; ++index) {
      if (aItemToRemove.Equals(mCharArray[index])) {
        // Shift everything down, overwriting and removing the desired element
        // These are pointers from nsAString::BeginReading so no need to free
        memcpy(mCharArray + index,
               mCharArray + index + 1,
               (mLength - index - 1) * sizeof(PRUnichar *));
        --mLength;
        return PR_TRUE;
      }
    }
    return PR_FALSE;
  }
  PRInt32 IndexOf(nsAString const & aItemToFind) const
  {
    for (PRUint32 index = 0; index < mLength; ++index) {
      if (aItemToFind.Equals(mCharArray[index])) {
        return index;
      }
    }
    return -1;
  }
private:
  PRUnichar const * * mCharArray;
  PRUint32 mLength;
};

NS_IMETHODIMP
sbLocalDatabasePropertyCache::CacheProperties(const PRUnichar **aGUIDArray,
                                              PRUint32 aGUIDArrayCount)
{
  TRACE(("sbLocalDatabasePropertyCache[0x%.8x] - CacheProperties(%d)", this,
         aGUIDArrayCount));

  NS_ASSERTION(mLibrary, "You didn't initialize!");
  nsresult rv;

  // If we're asked to cache more than we can store, then just ignore since
  // it would just do a lot of reads that would be throw away.
  if (aGUIDArrayCount > CACHE_SIZE) {
    NS_WARNING("Requested to cache more items than the cache can hold, ignoring request");
    return NS_OK;
  }
  // First, collect all the guids that are not cached
  nsTArray<nsString> misses;
  PRBool cacheLibraryMediaItem = PR_FALSE;

  // Unfortunately need to lock for the duration of this call
  // till after we build the misses array, we don't want another
  // thread coming in behind us and thinking there's misses when they're
  // going to be loaded.
  {
    nsAutoLock lock(mCacheLock);

    for (PRUint32 i = 0; i < aGUIDArrayCount; i++) {

      nsDependentString guid(aGUIDArray[i]);

      if (mCache.Get(guid) == nsnull) {

        if (guid.Equals(mLibraryResourceGUID)) {
          cacheLibraryMediaItem = PR_TRUE;
        }
        else {
          nsString* newElement = misses.AppendElement(guid);
          NS_ENSURE_TRUE(newElement, NS_ERROR_OUT_OF_MEMORY);
        }
      }
    }

    PRUint32 const numMisses = misses.Length();

    // If there were no misses and we don't have to cache the library media
    // item, just return since there is nothing to do
    if (numMisses > 0) {
      nsCOMArray<sbLocalDatabaseResourcePropertyBag> bags(numMisses);
      rv = RetrieveProperties(misses, bags);
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_TRUE(bags.Count() == numMisses, NS_ERROR_UNEXPECTED);
      for (PRUint32 index = 0; index < numMisses; ++index) {
#ifdef DEBUG
        nsString temp;
        bags[index]->GetGuid(temp);
        NS_ASSERTION(misses[index].Equals(temp), "inserting an bag that doesn't match the guid");
#endif
        mCache.Put(misses[index], bags[index]);
      }
    }

    TRACE(("sbLocalDatabasePropertyCache[0x%.8x] - CacheProperties() - Misses %d", this,
           numMisses));
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::GetProperties(const PRUnichar **aGUIDArray,
                                            PRUint32 aGUIDArrayCount,
                                            PRUint32 *aPropertyArrayCount,
                                            sbILocalDatabaseResourcePropertyBag ***aPropertyArray)
{
  NS_ASSERTION(mLibrary, "You didn't initialize!");

  NS_ENSURE_ARG_POINTER(aPropertyArrayCount);
  NS_ENSURE_ARG_POINTER(aPropertyArray);

  if (!aGUIDArrayCount) {
    NS_WARNING("Asked for 0 properties in call to GetProperties!");
    *aPropertyArrayCount = 0;
    *aPropertyArray = nsnull;
    return NS_OK;
  }

  // A lot of letters just to allocate the correct number of pointers...
  nsAutoArrayPtr<sbILocalDatabaseResourcePropertyBag*> propertyBagArray(
    static_cast<sbILocalDatabaseResourcePropertyBag**> (
      NS_Alloc(sizeof(sbILocalDatabaseResourcePropertyBag*) * aGUIDArrayCount)));
  NS_ENSURE_TRUE(propertyBagArray, NS_ERROR_OUT_OF_MEMORY);
  memset(propertyBagArray.get(), 0, sizeof(sbILocalDatabaseResourcePropertyBag*) * aGUIDArrayCount);

  // must initialize because we look for errors coming out of the for loop
  nsresult rv = NS_OK;
  PRBool locked = PR_FALSE;
  nsTArray<PRUint32> missesIndex(CACHE_SIZE);
  nsTArray<nsString> misses(CACHE_SIZE);
  PRUint32 i;
  PRBool cacheUpdated = PR_FALSE;
  nsAutoLock lookupLock(mCacheLock);
  for (i = 0; i < aGUIDArrayCount; i++) {

    nsDependentString const guid(aGUIDArray[i]);
    sbLocalDatabaseResourcePropertyBag * bag = nsnull;

    // If the bag has a pending write waiting we need to get it into the
    // database so that what is returned is consistent. And we do this
    // inside the lock so that we don't get a Write call right after the
    // flush.
    if (mDirty.Get(guid, nsnull)) {
      rv = WriteDirtyProperties(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    bag = mCache.Get(guid);
    if (bag) {
      // Bag is not addref from mCache, and propertyBagArray is straight
      // pointers
      NS_ADDREF(propertyBagArray[i] = bag);
    }
    else {
      // Save the miss guid and index so we can retrieve it later
      PRUint32 * const newIndex = missesIndex.AppendElement(i);
      NS_ENSURE_TRUE(newIndex, NS_ERROR_OUT_OF_MEMORY);

      nsString * const newGuid =
        misses.AppendElement(guid);
      NS_ENSURE_TRUE(newGuid, NS_ERROR_OUT_OF_MEMORY);
    }

    // Batch up our reads. We want to read multiple bags at a time
    // and we'll use the BATCH_READ_SIZE as an arbitrary point to read.
    // And we want to be sure to read on the last item
    PRBool const timeToRead = misses.Length() == BATCH_READ_SIZE ||
                              i == aGUIDArrayCount - 1;
    if (timeToRead && misses.Length() > 0) {
      // Clear the temporary bag container and retrieve the misses
      nsCOMArray<sbLocalDatabaseResourcePropertyBag> bags(CACHE_SIZE);
      rv = RetrieveProperties(misses, bags);
      if (NS_FAILED(rv))
        break;
      if (bags.Count() > 0) {
        PRUint32 const missesLength = missesIndex.Length();
        NS_ENSURE_TRUE(missesLength == bags.Count(), NS_ERROR_UNEXPECTED);
        // Now store the bags we retrieved into the out parameter
        for (PRUint32 index = 0; index < missesLength; ++index) {
          PRUint32 const missIndex = missesIndex[index];
          sbLocalDatabaseResourcePropertyBag * const bag = bags[index];
          // If this is the first set of misses and within the cache size
          // update the cache
          if (!cacheUpdated && missIndex < CACHE_SIZE) {
#ifdef DEBUG
            nsString temp;
            bag->GetGuid(temp);
            NS_ASSERTION(temp.Equals(aGUIDArray[missIndex]), "inserting an bag that doesn't match the guid");
#endif
            mCache.Put(nsDependentString(aGUIDArray[missIndex]), bag);
          }
          NS_ADDREF(propertyBagArray[missIndex] = bag);
        }
        cacheUpdated = PR_TRUE;
      }
      else {
        NS_WARNING("RetrieveProperties didn't retrieve anything");
      }
      missesIndex.Clear();
      misses.Clear();
    }
  }

  // Check for error in loop and clean up
  if (NS_FAILED(rv)) {
    for (PRUint32 index = 0; index < i; ++index) {
      NS_IF_RELEASE(propertyBagArray[index]);
    }
    return rv;
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
  NS_ASSERTION(mLibrary, "You didn't initialize!");
  NS_ENSURE_ARG_POINTER(aGUIDArray);
  NS_ENSURE_ARG_POINTER(aPropertyArray);
  NS_ENSURE_TRUE(aGUIDArrayCount == aPropertyArrayCount, NS_ERROR_INVALID_ARG);

  nsresult rv = NS_OK;

  sbAutoBatchHelper batchHelper(*mLibrary);

  nsAutoLock lock(mCacheLock);
  for(PRUint32 i = 0; i < aGUIDArrayCount; i++) {
    nsDependentString const guid(aGUIDArray[i]);
    nsRefPtr<sbLocalDatabaseResourcePropertyBag> bag;

    bag = mCache.Get(guid);
    // If it's not cached we need to create a new bag
    if (!bag) {
      PRUint32 mediaItemId;
      rv = aPropertyArray[i]->GetMediaItemId(&mediaItemId);
      NS_ENSURE_SUCCESS(rv, rv);

      bag = new sbLocalDatabaseResourcePropertyBag(this, mediaItemId, guid);
    }
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

      bag->SetProperty(id, value);
    }

    PR_Lock(mDirtyLock);
    mDirty.Put(guid, bag);
    PRBool const writeNeeded = ++mWritePendingCount > SB_LOCALDATABASE_MAX_PENDING_CHANGES;
    PR_Unlock(mDirtyLock);

    if(writeNeeded) {
      rv = WriteDirtyProperties(PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if(aWriteThroughNow) {
    rv = WriteDirtyProperties(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return rv;
}

/**
 * Used to enumerate dirty properties and build the property queries
 */
class DirtyPropertyEnumerator
{
public:
  DirtyPropertyEnumerator(sbLocalDatabasePropertyCache * aCache,
                          sbLocalDatabaseResourcePropertyBag * aBag,
                          sbIDatabaseQuery * aQuery,
                          PRUint32 aMediaItemID,
                          PRBool aIsLibrary) :
                            mCache(aCache),
                            mBag(aBag),
                            mQuery(aQuery),
                            mMediaItemID(aMediaItemID),
                            mIsLibrary(aIsLibrary) {}
  nsresult Process(PRUint32 aDirtyPropertyKey);
  nsresult Finish();
private:
  // None-owning reference
  sbLocalDatabasePropertyCache * mCache;
  // non-owning reference
  sbLocalDatabaseResourcePropertyBag * mBag;
  // Non-owning reference
  sbIDatabaseQuery * mQuery;
  PRUint32 mMediaItemID;
  PRBool mIsLibrary;
  nsTArray<nsString> mTopLevelSets;
};

nsresult DirtyPropertyEnumerator::Process(PRUint32 aDirtyPropertyKey)
{
  nsString propertyID;

  //If this isn't true, something is very wrong, so bail out.
  PRBool success = mCache->GetPropertyID(aDirtyPropertyKey, propertyID);
  NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

  // Never change the guid
  if (propertyID.EqualsLiteral(SB_PROPERTY_GUID)) {
    NS_WARNING("Attempt to change the SB_PROPERTY_GUID property");
    return NS_OK;
  }

  nsString value;
  nsresult rv = mBag->GetPropertyByID(aDirtyPropertyKey, value);
  NS_ENSURE_SUCCESS(rv, rv);

  //Top level properties need to be treated differently, so check for them.
  if(SB_IsTopLevelProperty(aDirtyPropertyKey)) {

    // Switch the query if we are updating the library resource
    nsString set;
    SB_GetTopLevelPropertyColumn(aDirtyPropertyKey, set);
    set.AppendLiteral("=\"");
    set.Append(value);
    set.AppendLiteral("\"");
    mTopLevelSets.AppendElement(set);
  }
  else { //Regular properties all go in the same spot.
    if (value.IsVoid()) {
      rv = mQuery->AddQuery(sbLocalDatabaseSQL::PropertiesDelete(mMediaItemID, aDirtyPropertyKey));
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      nsString sortable;
      rv = mBag->GetSortablePropertyByID(aDirtyPropertyKey, sortable);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->AddQuery(sbLocalDatabaseSQL::PropertiesInsert());
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindInt32Parameter(0, mMediaItemID);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindInt32Parameter(1, aDirtyPropertyKey);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindStringParameter(2, value);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindStringParameter(3, sortable);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

nsresult DirtyPropertyEnumerator::Finish()
{
  PRUint32 const length = mTopLevelSets.Length();
  if (length > 0) {
    nsString sql;
    if (mIsLibrary) {
      sql.AppendLiteral("UPDATE library_media_item SET ");
    }
    else {
      sql.AppendLiteral("UPDATE media_items SET ");
    }
    for (PRUint32 index = 0; index < length; ++index) {
      if (index > 0) {
        sql.AppendLiteral(",");
      }
      sql.Append(mTopLevelSets[index]);
    }
    if (!mIsLibrary) {
      sql.AppendLiteral(" WHERE media_item_id = ");
      sql.AppendInt(mMediaItemID);
    }
    nsresult rv = mQuery->AddQuery(sql);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

PR_STATIC_CALLBACK(PLDHashOperator)
EnumDirtyProps(nsUint32HashKey *aKey, void *aClosure)
{
  DirtyPropertyEnumerator * const dirtyPropsEnum = static_cast<DirtyPropertyEnumerator *>(aClosure);
  dirtyPropsEnum->Process(aKey->GetKey());
  return PL_DHASH_NEXT;
}

/**
 * Pair of arrays to keep track of the media item guid and ID.
 * The guid and IDs
 */
struct DirtyItems
{
  nsTArray<nsString> mGUIDs;
  nsTArray<PRUint32> mIDs;
};

PR_STATIC_CALLBACK(PLDHashOperator)
EnumDirtyItems(nsAString const &aKey, sbLocalDatabaseResourcePropertyBag * aBag, void *aClosure)
{
  DirtyItems *dirtyItem = static_cast<DirtyItems *>(aClosure);
  NS_ENSURE_TRUE(dirtyItem->mGUIDs.AppendElement(aKey),
                 PL_DHASH_STOP);
  nsresult rv = aBag->GetMediaItemId(dirtyItem->mIDs.AppendElement());
  NS_ENSURE_SUCCESS(rv, PL_DHASH_STOP);

  return PL_DHASH_NEXT;
}

PR_STATIC_CALLBACK(PLDHashOperator)
EnumDirtyItemsSetDirty(nsAString const & aKey, sbLocalDatabaseResourcePropertyBag * aBag, void *aClosure)
{
  aBag->ClearDirty();
  return PL_DHASH_NEXT;
}

nsresult
sbLocalDatabasePropertyCache::WriteDirtyProperties(PRBool aLockTheCache)
{
  NS_ASSERTION(mLibrary, "You didn't initialize!");

  nsresult rv = NS_OK;

  nsCOMPtr<sbIDatabaseQuery> query;
  PRUint32 dirtyItemCount;
  { // find the new dirty properties
    DirtyItems dirtyItems;

    //Lock it.
    nsAutoLock lock(mDirtyLock);

   //Reset the dirty flag of the property bags help by mDirty
    dirtyItemCount = mDirty.EnumerateRead(EnumDirtyItems, (void *) &dirtyItems);
    mWritePendingCount = 0;

    if (!dirtyItemCount)
      return NS_OK;

    rv = MakeQuery(NS_LITERAL_STRING("begin"), getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);

    // Run through the list of dirty items and build the fts delete/insert
    // queries

    /* XXXAus: resource_properties_fts is disabled for now. See bug 9488
               and bug 9617 for more information.

    rv = mMediaItemsFtsDeleteInCriterion->Clear();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMediaItemsFtsInsertInCriterion->Clear();
    NS_ENSURE_SUCCESS(rv, rv);
    */

    /* XXXAus: resource_properties_fts is disabled for now. See bug 9488
               and bug 9617 for more information.

    nsString mediaItemsFtsDeleteSql;
    rv = mMediaItemsFtsDelete->ToString(mediaItemsFtsDeleteSql);
    NS_ENSURE_SUCCESS(rv, rv);

    nsString mediaItemsFtsInsertSql;
    rv = mMediaItemsFtsInsert->ToString(mediaItemsFtsInsertSql);
    NS_ENSURE_SUCCESS(rv, rv);
    */

    // The first queries are to delete the fts data of the updated items

    /* XXXAus: resource_properties_fts is disabled for now. See bug 9488
               and bug 9617 for more information.

    rv = query->AddQuery(mediaItemsFtsDeleteSql);
    NS_ENSURE_SUCCESS(rv, rv);
    */

    rv = query->AddQuery(sbLocalDatabaseSQL::MediaItemsFtsAllDelete(dirtyItems.mIDs));
    NS_ENSURE_SUCCESS(rv, rv);

    //For each GUID, there's a property bag that needs to be processed as well.
    for(PRUint32 i = 0; i < dirtyItemCount; ++i) {
      nsRefPtr<sbLocalDatabaseResourcePropertyBag> bag;
      nsString const guid(dirtyItems.mGUIDs[i]);
      if (mDirty.Get(guid, getter_AddRefs(bag))) {

        PRUint32 const mediaItemId = dirtyItems.mIDs[i];


        PRBool const isLibrary = guid.Equals(mLibraryResourceGUID);

        DirtyPropertyEnumerator dirtyPropertyEnumerator(this,
                                                        bag,
                                                        query,
                                                        mediaItemId,
                                                        isLibrary);
        PRUint32 dirtyPropsCount;
        rv = bag->EnumerateDirty(EnumDirtyProps, (void *) &dirtyPropertyEnumerator, &dirtyPropsCount);
        NS_ENSURE_SUCCESS(rv, rv);
        dirtyPropertyEnumerator.Finish();
      }
    }

    // Finally, insert the new fts data for the updated items

    /* XXXAus: resource_properties_fts is disabled for now. See bug 9488
               and bug 9617 for more information.

    rv = query->AddQuery(mediaItemsFtsInsertSql);
    NS_ENSURE_SUCCESS(rv, rv);
    */

    rv = query->AddQuery(sbLocalDatabaseSQL::MediaItemsFtsAllInsert(dirtyItems.mIDs));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(NS_LITERAL_STRING("commit"));
    NS_ENSURE_SUCCESS(rv, rv);

    {
      if (aLockTheCache) {
        PR_Lock(mCacheLock);
      }
      mDirty.EnumerateRead(EnumDirtyItemsSetDirty, nsnull);
      if (aLockTheCache) {
        PR_Unlock(mCacheLock);
      }
    }
    //Clear out dirty guid hashtable.
    mDirty.Clear();
  }

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);
  return NS_OK;
}
// note: this might be called either from the main thread (for a forced write)
// or a background thread (from the flush thread)
NS_IMETHODIMP
sbLocalDatabasePropertyCache::Write()
{
  return WriteDirtyProperties(PR_TRUE);
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
  if (strcmp(aTopic, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC) == 0 ||
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

  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(sbLocalDatabaseSQL::PropertiesSelect(), getter_AddRefs(query));
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

    nsString propertyID(NS_ConvertASCIItoUTF16(sStaticProperties[i].mPropertyID));

    PRBool success = mPropertyDBIDToID.Put(sStaticProperties[i].mDBID, propertyID);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    success = mPropertyIDToDBID.Put(propertyID, sStaticProperties[i].mDBID);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  }

  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::AddDirty(const nsAString &aGuid,
                                           sbLocalDatabaseResourcePropertyBag * aBag)
{
  nsAutoString guid(aGuid);

  {
    nsAutoLock lock(mDirtyLock);
    mDirty.Put(guid, aBag);
    ++mWritePendingCount;
  }


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

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeQuery(sbLocalDatabaseSQL::PropertiesTableInsert(),
                          getter_AddRefs(query));
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

