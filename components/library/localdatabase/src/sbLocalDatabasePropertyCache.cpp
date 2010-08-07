/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbLocalDatabasePropertyCache.h"

#include <sbIDatabaseQuery.h>
#include <sbILibraryManager.h>
#include <sbIPropertyManager.h>
#include <sbPropertiesCID.h>

#include <DatabaseQuery.h>
#include <nsAutoLock.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsIObserverService.h>
#include <nsIProxyObjectManager.h>
#include <nsServiceManagerUtils.h>
#include <nsIThreadManager.h>
#include <nsThreadUtils.h>
#include <nsUnicharUtils.h>
#include <nsXPCOM.h>
#include <nsXPCOMCIDInternal.h>
#include <prlog.h>
#include <nsThreadUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsIStringBundle.h>
#include <nsIPrefBranch.h>
#include <nsIPrefService.h>

#include "sbDatabaseResultStringEnumerator.h"
#include "sbLocalDatabaseGUIDArray.h"
#include "sbLocalDatabaseLibrary.h"
#include "sbLocalDatabaseResourcePropertyBag.h"
#include "sbLocalDatabaseSchemaInfo.h"
#include <sbIJobProgressService.h>
#include <sbTArrayStringEnumerator.h>
#include <sbStringBundle.h>
#include <sbStringUtils.h>
#include <sbMediaListBatchCallback.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyInfo.h>
#include "sbLocalDatabaseSQL.h"
#include <sbIDatabaseQuery.h>
#include <sbThreadPoolService.h>
#include <sbDebugUtils.h>

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

#define CACHE_HASHTABLE_SIZE 500


/**
 * \brief Number of milliseconds between sbIJobProgress notifications
 * for sbLocalDatabaseSortInvalidateJob
 */
#define SORTINVALIDATE_TIMER_PERIOD  50

#define NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID "xpcom-shutdown-threads"
#define NS_FINAL_UI_STARTUP_OBSERVER_ID       "final-ui-startup"


NS_IMPL_THREADSAFE_ISUPPORTS2(sbLocalDatabasePropertyCache,
                              sbILocalDatabasePropertyCache,
                              nsIObserver)

sbLocalDatabasePropertyCache::sbLocalDatabasePropertyCache()
: mWritePendingCount(0),
  mCache(sbLocalDatabasePropertyCache::CACHE_SIZE),
  mDependentGUIDArrayMonitor(nsnull),
  mMonitor(nsnull),
  mLibrary(nsnull),
  mSortInvalidateJob(nsnull)
{
  MOZ_COUNT_CTOR(sbLocalDatabasePropertyCache);
#ifdef PR_LOGGING
  if (!gLocalDatabasePropertyCacheLog) {
    gLocalDatabasePropertyCacheLog = PR_NewLogModule("sbLocalDatabasePropertyCache");
  }
#endif
}

sbLocalDatabasePropertyCache::~sbLocalDatabasePropertyCache()
{
  if (mDependentGUIDArrayMonitor) {
    nsAutoMonitor::DestroyMonitor(mDependentGUIDArrayMonitor);
  }

  if (mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
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

  mDependentGUIDArrayMonitor =
    nsAutoMonitor::NewMonitor("sbLocalDatabasePropertyCache::mDependentGUIDArrayMonitor");
  NS_ENSURE_TRUE(mDependentGUIDArrayMonitor, NS_ERROR_OUT_OF_MEMORY);

  mMonitor = nsAutoMonitor::NewMonitor("sbLocalDatabasePropertyCache::mMonitor");
  NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);

  rv = LoadProperties();
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mDirty.Init(CACHE_HASHTABLE_SIZE);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  mThreadPoolService = do_GetService(SB_THREADPOOLSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPoolService->SetIdleThreadTimeout(15000);
  NS_ENSURE_SUCCESS(rv, rv);

  mFlushTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFlushTimer->Init(this,
                         SB_LOCALDATABASE_CACHE_FLUSH_DELAY,
                         nsITimer::TYPE_REPEATING_SLACK);
  NS_ENSURE_SUCCESS(rv, rv);

  mLibrary = aLibrary;

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Shut down the prop cache when the app goes away
  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Shut down any library related tasks BEFORE the library goes away
  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check for background tasks to be run when the first window comes up
  rv = observerService->AddObserver(this,
                                    NS_FINAL_UI_STARTUP_OBSERVER_ID,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialise some prepared statements.
  nsCOMPtr<sbIDatabaseQuery> query;
  rv = MakeQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(mSQLStrings.MediaItemSelect(),
                           getter_AddRefs(mItemSelectPreparedStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(mSQLStrings.SecondaryPropertySelect(),
                           getter_AddRefs(mSecondaryPropertySelectPreparedStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(mSQLStrings.MediaItemsFtsAllDelete(),
                           getter_AddRefs(mMediaItemsFtsAllDeletePreparedStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(mSQLStrings.MediaItemsFtsAllInsert(),
                           getter_AddRefs(mMediaItemsFtsAllInsertPreparedStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(sbLocalDatabaseSQL::PropertiesDelete(),
                           getter_AddRefs(mPropertiesDeletePreparedStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->PrepareQuery(sbLocalDatabaseSQL::PropertiesInsert(),
                           getter_AddRefs(mPropertiesInsertPreparedStatement));
  NS_ENSURE_SUCCESS(rv, rv);

  // Put together the update queries for each property.
  // By preparing these queries in advance we avoid having to recompile them all the time.
  success = mMediaItemsUpdatePreparedStatements.Init(sStaticPropertyCount);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    nsString updateSQL = NS_LITERAL_STRING("UPDATE media_items SET ");
    updateSQL.AppendLiteral(sStaticProperties[i].mColumn);
    updateSQL.Append(NS_LITERAL_STRING(" = ? WHERE media_item_id = ?"));

    nsCOMPtr<sbIDatabasePreparedStatement> updateMediaItemPreparedStatement;
    rv = query->PrepareQuery(updateSQL, getter_AddRefs(updateMediaItemPreparedStatement));
    NS_ENSURE_SUCCESS(rv,rv);

    success = mMediaItemsUpdatePreparedStatements.Put(sStaticProperties[i].mDBID,
                                                      updateMediaItemPreparedStatement);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  // Create a second, identical set of queries for the library media item.
  // The library media item is stored in its own table.
  success = mLibraryMediaItemUpdatePreparedStatements.Init(sStaticPropertyCount);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    nsString updateSQL = NS_LITERAL_STRING("UPDATE library_media_item SET ");
    updateSQL.AppendLiteral(sStaticProperties[i].mColumn);
    updateSQL.Append(NS_LITERAL_STRING(" = ?"));

    nsCOMPtr<sbIDatabasePreparedStatement> updateMediaItemPreparedStatement;
    rv = query->PrepareQuery(updateSQL, getter_AddRefs(updateMediaItemPreparedStatement));
    NS_ENSURE_SUCCESS(rv,rv);

    success = mLibraryMediaItemUpdatePreparedStatements.Put(sStaticProperties[i].mDBID,
                                                            updateMediaItemPreparedStatement);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::Shutdown()
{
  TRACE(("sbLocalDatabasePropertyCache[0x%.8x] - Shutdown()", this));

  nsresult rv = NS_OK;

  if(mFlushTimer) {
    rv = mFlushTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // If we were in the middle of rebuilding all the sort info
  // then must abort.
  if (mSortInvalidateJob) {
    mSortInvalidateJob->Shutdown();
    mSortInvalidateJob = nsnull;
  }

  if(mWritePendingCount) {
    rv =  Write();
  }

  mItemSelectPreparedStatement = nsnull;
  mSecondaryPropertySelectPreparedStatement = nsnull;
  mMediaItemsFtsAllDeletePreparedStatement = nsnull;
  mMediaItemsFtsAllInsertPreparedStatement = nsnull;
  mPropertiesDeletePreparedStatement = nsnull;
  mPropertiesInsertPreparedStatement = nsnull;

  mMediaItemsUpdatePreparedStatements.Clear();
  mLibraryMediaItemUpdatePreparedStatements.Clear();

  return rv;
}

template <class T>
nsresult
sbLocalDatabasePropertyCache::RetrievePrimaryProperties(sbIDatabaseQuery* query,
                                                        T const & aGuids,
                                                        IDToBagMap & aIDToBagMap,
                                                        nsCOMArray<sbLocalDatabaseResourcePropertyBag> & aBags,
                                                        nsTArray<PRUint32> & aMissesIDs)
{
  nsresult rv;

  PRUint32 const length = aGuids.Length();
  PRUint32 itemsRequested = length;
  for (PRUint32 i = 0; i < length; ++i) {
    // Skip any blank guid's as that's probably the library item
    nsString const & aGuid = aGuids[i];
    if (aGuid.IsEmpty()) {
      --itemsRequested;
      continue;
    }
    if (i % mSQLStrings.MediaItemBindCount == 0) {
      rv = query->AddPreparedStatement(mItemSelectPreparedStatement);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = query->BindStringParameter(i % mSQLStrings.MediaItemBindCount, aGuids[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  // We might have been asked for only the library guid if so there's
  // nothing to do
  if (itemsRequested == 0)
    return NS_OK;
  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure we got the number of guid's we expected
  NS_ENSURE_TRUE(rowCount == itemsRequested, NS_ERROR_UNEXPECTED);

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

        rv = bag->PutValue(sStaticProperties[i].mDBID, value);
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

  query->ResetQuery();
  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::RetrieveSecondaryProperties(sbIDatabaseQuery* query,
                                                          nsTArray<PRUint32> itemIDs,
                                                          IDToBagMap const & bags)
{
  nsresult rv;

  PRUint32 const length = itemIDs.Length();
  // If we weren't passed any id's there's nothing to do and just return
  if (length == 0)
    return NS_OK;
  for (PRUint32 i = 0; i < length; ++i) {
    if ( i % mSQLStrings.SecondaryPropertyBindCount == 0 ) {
      rv = query->AddPreparedStatement(mSecondaryPropertySelectPreparedStatement);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = query->BindInt32Parameter(i % mSQLStrings.SecondaryPropertyBindCount, itemIDs[i]);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString objSearchable;
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

    rv = bag->PutValue(propertyID, obj);

    NS_ENSURE_SUCCESS(rv, rv);
  }
  query->ResetQuery();

  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::RetrieveLibraryProperties(sbLocalDatabaseResourcePropertyBag * aBag)
{
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = MakeQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  // no sense preparing this. it hardly ever happens.
  rv = query->AddQuery(sbLocalDatabaseSQL::LibraryMediaItemsPropertiesSelect());
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

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

    rv = aBag->PutValue(propertyID, obj);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = MakeQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  // no sense preparing this either.
  rv = query->AddQuery(mSQLStrings.LibraryMediaItemSelect());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(rowCount == 1, "Failed to get data from library_media_item");

  for (PRUint32 i = 0; i < sStaticPropertyCount; i++) {
    nsString value;
    rv = result->GetRowCell(0, i, value);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!value.IsVoid()) {

      rv = aBag->PutValue(sStaticProperties[i].mDBID, value);

      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

/**
 * This takes in to account the missing library guid when getting the guid
 * count. This represents the current logical guid count and is mainly for
 * debug purposes. Since it's inline no need of message #if's
 */
template <class T>
inline
PRInt32 GetGUIDCount(T const & aGUIDs,
            PRInt32 aLibraryItemPosition)
{
  return aLibraryItemPosition == -1 ? aGUIDs.Length() : (aGUIDs.Length() - 1);
}

template <class T>
nsresult sbLocalDatabasePropertyCache::RetrieveProperties(
  T & aGUIDs,
  nsCOMArray<sbLocalDatabaseResourcePropertyBag> & aBags)
{
  nsresult rv;
  PRInt32 const libraryItemPosition = aGUIDs.IndexOf(mLibraryResourceGUID);
  // blank out the library guid so we don't process it as a media item
  if (libraryItemPosition != -1) {
    aGUIDs[libraryItemPosition].Truncate();
  }
  // Start with the top level properties.  This pulls the data from the
  // media_items table for each requested guid.  This also serves to
  // translate the guid to a media_item_id so we can use the id for
  // the next lookup on the resource_properties table

  if (aGUIDs.Length() > 0) {
    nsCOMPtr<sbIDatabaseQuery> query;
    nsresult rv = MakeQuery(getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);

    // As we read in the data from media_item_ids, keep a map of the id to
    // property bag so we don't need guids in the second pass

    nsTArray<PRUint32> itemIDs(aGUIDs.Length());
    IDToBagMap idToBagMap;
    PRBool const success = idToBagMap.Init(aGUIDs.Length());
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

    rv = RetrievePrimaryProperties(query, aGUIDs, idToBagMap, aBags, itemIDs);
    NS_ENSURE_SUCCESS(rv, rv);

    NS_ASSERTION(idToBagMap.Count() == GetGUIDCount(aGUIDs, libraryItemPosition)   &&
                   GetGUIDCount(aGUIDs, libraryItemPosition) == itemIDs.Length() &&
                   GetGUIDCount(aGUIDs, libraryItemPosition) == aBags.Count(),
                 "RetrieveProperties returned an inconsistent result set");

    // Now do the data from resource_properties
    rv = RetrieveSecondaryProperties(query, itemIDs, idToBagMap);
  }
  // Cache the library's property data from library_media_item and
  // resource_properties if we were passed the library guid
  if (libraryItemPosition != -1) {
    nsRefPtr<sbLocalDatabaseResourcePropertyBag> bag =
      new sbLocalDatabaseResourcePropertyBag(this, 0, mLibraryResourceGUID);
    NS_ENSURE_TRUE(bag, NS_ERROR_OUT_OF_MEMORY);

    rv = bag->Init();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = RetrieveLibraryProperties(bag);
    NS_ENSURE_SUCCESS(rv, rv);

    // Restore the library GUID
    aGUIDs[libraryItemPosition] = mLibraryResourceGUID;
    aBags.ReplaceObjectAt(bag, libraryItemPosition);
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
    nsAutoMonitor mon(mMonitor);

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
        sbLocalDatabaseResourcePropertyBag * const bag = bags[index];
        // Skip nulls, these weren't found in the database
        if (bag == nsnull) {
          TRACE(("sbLocalDatabasePropertyCache[0x%.8x] - CacheProperties() - "
                 "media item with id %s not found in database",
                   this, NS_ConvertUTF16toUTF8(misses[index]).get()));
          continue;
        }
#ifdef DEBUG
        nsString temp;
        bag->GetGuid(temp);
        NS_ASSERTION(misses[index].Equals(temp), "inserting an bag that doesn't match the guid");
#endif
        mCache.Put(misses[index], bag);
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
  nsTArray<PRUint32> missesIndex(CACHE_SIZE);
  nsTArray<nsString> misses(CACHE_SIZE);
  PRUint32 i;
  PRBool cacheUpdated = PR_FALSE;

  nsAutoMonitor mon(mMonitor);

  for (i = 0; i < aGUIDArrayCount; i++) {

    nsDependentString const guid(aGUIDArray[i]);
    sbLocalDatabaseResourcePropertyBag * bag = nsnull;

    // If the bag has a pending write waiting we need to get it into the
    // database so that what is returned is consistent.
    if (mDirty.Get(guid, nsnull)) {
      rv = Write();
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
          // If we're missing an item error out
          if (bag == nsnull) {
            rv = NS_ERROR_NOT_AVAILABLE;
            break;
          }
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

  nsAutoMonitor mon(mMonitor);

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
    NS_ENSURE_TRUE(bag, NS_ERROR_UNEXPECTED);
    mDirty.Put(guid, bag);
  }

  if(aWriteThroughNow) {
    rv = Write();
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
    // First, account for Top Level Properties.
    nsCOMPtr<sbIDatabasePreparedStatement> topLevelPropertyUpdate;
    if (!mIsLibrary) {
      success = mCache->mMediaItemsUpdatePreparedStatements.Get(aDirtyPropertyKey,
                                                                getter_AddRefs(topLevelPropertyUpdate));
    }
    else {
      success = mCache->mLibraryMediaItemUpdatePreparedStatements.Get(aDirtyPropertyKey,
                                                                      getter_AddRefs(topLevelPropertyUpdate));
    }
    NS_ENSURE_TRUE(success, NS_ERROR_UNEXPECTED);

    rv = mQuery->AddPreparedStatement(topLevelPropertyUpdate);
    NS_ENSURE_SUCCESS(rv,rv);

    nsString columnName;
    rv = SB_GetTopLevelPropertyColumn(aDirtyPropertyKey, columnName);
    NS_ENSURE_SUCCESS(rv,rv);

    PRUint32 columnType = -1;
    rv = SB_GetTopLevelPropertyColumnType(aDirtyPropertyKey, columnType);
    NS_ENSURE_SUCCESS(rv, rv);

    if (columnType == SB_COLUMN_TYPE_TEXT) {
     rv = mQuery->BindStringParameter(0, value);
     NS_ENSURE_SUCCESS(rv,rv);
    }
    else if (columnType == SB_COLUMN_TYPE_INTEGER) {
     PRUint64 intVal = nsString_ToUint64(value, &rv);
     NS_ENSURE_SUCCESS(rv,rv);
     rv = mQuery->BindInt64Parameter(0, intVal);
     NS_ENSURE_SUCCESS(rv,rv);
    }
    else {
     NS_WARNING("Failed to determine the column type of a top level property.");
     return NS_ERROR_CANNOT_CONVERT_DATA;
    }
    if (!mIsLibrary) {
      rv = mQuery->BindInt32Parameter(1, mMediaItemID);
      NS_ENSURE_SUCCESS(rv,rv);
    }
  }
  else { //Regular properties all go in the same spot.
    if (value.IsVoid()) {
      rv = mQuery->AddPreparedStatement(mCache->mPropertiesDeletePreparedStatement);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindInt32Parameter(0, mMediaItemID);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindInt32Parameter(1, aDirtyPropertyKey);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else {
      nsString searchable;
      rv = mBag->GetSearchablePropertyByID(aDirtyPropertyKey, searchable);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString sortable;
      rv = mBag->GetSortablePropertyByID(aDirtyPropertyKey, sortable);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString secondarySortable;
      rv = mCache->CreateSecondarySortValue(mBag,
                     aDirtyPropertyKey, secondarySortable);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->AddPreparedStatement(mCache->mPropertiesInsertPreparedStatement);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindInt32Parameter(0, mMediaItemID);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindInt32Parameter(1, aDirtyPropertyKey);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindStringParameter(2, value);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindStringParameter(3, searchable);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindStringParameter(4, sortable);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mQuery->BindStringParameter(5, secondarySortable);
      NS_ENSURE_SUCCESS(rv, rv);
    }
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

// note: this might be called either from the main thread (for a forced write)
// or a background thread (from the flush thread)
nsresult
sbLocalDatabasePropertyCache::Write()
{
  NS_ASSERTION(mLibrary, "You didn't initialize!");
  nsresult rv = NS_OK;

  nsCOMPtr<sbIDatabaseQuery> query;
  PRUint32 dirtyItemCount;
  { // find the new dirty properties
    DirtyItems dirtyItems;

    //Lock it.
    nsAutoMonitor mon(mMonitor);

    if (!mDirty.Count()) {
      return NS_OK;
    }

   //Reset the dirty flag of the property bags help by mDirty
    dirtyItemCount = mDirty.EnumerateRead(EnumDirtyItems, (void *) &dirtyItems);
    mWritePendingCount = 0;

    if (!dirtyItemCount)
      return NS_OK;

    rv = MakeQuery(getter_AddRefs(query));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(NS_LITERAL_STRING("begin"));
    NS_ENSURE_SUCCESS(rv, rv);

    // Run through the list of dirty items and build the fts delete/insert
    // queries
    for (PRUint32 i = 0; i < dirtyItemCount; ++i) {
      rv = query->AddPreparedStatement(mMediaItemsFtsAllDeletePreparedStatement);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = query->BindInt32Parameter(0, dirtyItems.mIDs[i]);
      NS_ENSURE_SUCCESS(rv, rv);
    }

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

        // Build a new FTS data table entry by concatenating all the user-viewable properties.
        // NOTE: This includes both top-level and not-top-level properties!
        // TODO: Look at top level properties to see if you want them searchable!
        nsString newFTSData;
        nsCOMPtr<nsIStringEnumerator> bagProperties;
        rv = bag->GetIds(getter_AddRefs(bagProperties));
        NS_ENSURE_SUCCESS(rv, rv);
        PRBool hasMore;
        while (NS_SUCCEEDED(bagProperties->HasMore(&hasMore)) && hasMore) {
          nsAutoString propertyId;
          rv = bagProperties->GetNext(propertyId);
          NS_ENSURE_SUCCESS(rv, rv);

          PRBool hasProperty, isUserViewable;
          rv = mPropertyManager->HasProperty(propertyId, &hasProperty);
          NS_ENSURE_SUCCESS(rv, rv);
          if (!hasProperty) {
            continue;
          }

          nsCOMPtr<sbIPropertyInfo> propertyInfo;
          rv = mPropertyManager->GetPropertyInfo(propertyId,
                                                 getter_AddRefs(propertyInfo));
          NS_ENSURE_SUCCESS(rv,rv);
          rv = propertyInfo->GetUserViewable(&isUserViewable);
          NS_ENSURE_SUCCESS(rv,rv);

          if (isUserViewable) {
            PRUint32 propertyDBID;
            rv = GetPropertyDBID(propertyId, &propertyDBID);
            NS_ENSURE_SUCCESS(rv, rv);
            nsString propertySearchable;
            rv = bag->GetSearchablePropertyByID(propertyDBID, propertySearchable);
            NS_ENSURE_SUCCESS(rv, rv);
            newFTSData.Append(propertySearchable);
            newFTSData.AppendLiteral(" ");
          }
        }

        if (!newFTSData.IsEmpty()) {
          rv = query->AddPreparedStatement(mMediaItemsFtsAllInsertPreparedStatement);
          NS_ENSURE_SUCCESS(rv, rv);
          rv = query->BindInt32Parameter(0, dirtyItems.mIDs[i]);
          NS_ENSURE_SUCCESS(rv, rv);
          rv = query->BindStringParameter(1, newFTSData);
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }

    rv = query->AddQuery(NS_LITERAL_STRING("commit"));
    NS_ENSURE_SUCCESS(rv, rv);

    mDirty.EnumerateRead(EnumDirtyItemsSetDirty, nsnull);

    //Clear out dirty guid hashtable.
    mDirty.Clear();
  }

  PRInt32 dbOk;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  return NS_OK;
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
  nsresult rv = NS_OK;

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Prop cache failed to get observer service");
  if (NS_SUCCEEDED(rv)) {
    observerService->RemoveObserver(this, aTopic);
  }

  if (strcmp(aTopic, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC) == 0 ||
      strcmp(aTopic, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID) == 0) {
    Shutdown();

  } else if (strcmp(aTopic, NS_FINAL_UI_STARTUP_OBSERVER_ID) == 0) {

    // If InvalidateSortData was previously called for this library but did not
    // complete, then we need to start it up again.   Don't worry about UI this time,
    // just let it run in the background.
    PRBool hasInvalidData = PR_FALSE;
    GetSetInvalidSortDataPref(PR_FALSE, hasInvalidData);
    if (NS_SUCCEEDED(rv) && hasInvalidData) {
      nsCOMPtr<sbIJobProgress> job;
      InvalidateSortData(getter_AddRefs(job));

      // Launch modal UI.  This prevents all libraries from trying to
      // update at once, in the case were multiple libraries have
      // been flagged as invalid
      nsCOMPtr<sbIJobProgressService> progressService =
        do_GetService("@songbirdnest.com/Songbird/JobProgressService;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = progressService->ShowProgressDialog(job, nsnull, 0);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else if (strcmp(aTopic, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC) == 0) {

    // If we were in the middle of rebuilding all the sort info
    // then must abort.  Do this BEFORE library shutdown, as
    // when library shutdown actually happens the invalidate job
    // library enumeration will fail.
    if (mSortInvalidateJob) {
      mSortInvalidateJob->Shutdown();
      mSortInvalidateJob = nsnull;
    }
  } else if (strcmp(aTopic, NS_TIMER_CALLBACK_TOPIC) == 0) {
    rv = DispatchFlush();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbLocalDatabasePropertyCache::InvalidateSortData(sbIJobProgress** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(!mSortInvalidateJob, NS_ERROR_ALREADY_INITIALIZED);
  nsresult rv = NS_OK;

  // Flag this library as containing invalid sort data.  This way
  // if we crash or are shutdown before the invalidate job completes
  // we can try again on next launch
  PRBool hasInvalidData = PR_TRUE;
  GetSetInvalidSortDataPref(PR_TRUE, hasInvalidData);

  // Start a job to read, process, and set all the properties in the library
  NS_NEWXPCOM(mSortInvalidateJob, sbLocalDatabaseSortInvalidateJob);
  NS_ENSURE_TRUE(mSortInvalidateJob, NS_ERROR_OUT_OF_MEMORY);

  // Go!
  mSortInvalidateJob->Init(this, mLibrary);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ADDREF(*_retval = mSortInvalidateJob);
  return rv;
}

nsresult
sbLocalDatabasePropertyCache::InvalidateSortDataComplete() {

  // Clear the needs-rebuilding flag
  PRBool hasInvalidData = PR_FALSE;
  GetSetInvalidSortDataPref(PR_TRUE, hasInvalidData);

  mSortInvalidateJob = nsnull;
  return NS_OK;
}

nsresult
sbLocalDatabasePropertyCache::GetSetInvalidSortDataPref(
  PRBool aWrite, PRBool& aValue)
{
  // Figure out the pref key
  nsString guid;
  nsresult rv = mLibrary->GetDatabaseGuid(guid);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCString pref = NS_LITERAL_CSTRING("songbird.propertycache.");
  pref.Append(NS_LossyConvertUTF16toASCII(guid));
  pref.AppendLiteral(".invalidSortData");

  nsCOMPtr<nsIPrefBranch> prefBranch =
      do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrefService> prefRoot = do_QueryInterface(prefBranch, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // If writing, set value and flush to disk just in case
  if (aWrite) {
    rv = prefBranch->SetBoolPref(pref.get(), aValue);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = prefRoot->SavePrefFile(nsnull);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to save prefs.");
  // If reading, assume not set == false
  } else {
    rv = prefBranch->GetBoolPref(
      pref.get(), &aValue);
    if (NS_FAILED(rv)) {
      aValue = PR_FALSE;
    }
  }
  return rv;
}

void
sbLocalDatabasePropertyCache::AddDependentGUIDArray(
                                sbLocalDatabaseGUIDArray *aGUIDArray)
{
  NS_ENSURE_TRUE(aGUIDArray, /*void*/);

  nsAutoMonitor mon(mDependentGUIDArrayMonitor);
  mDependentGUIDArraySet.insert(aGUIDArray);

  return;
}

void
sbLocalDatabasePropertyCache::RemoveDependentGUIDArray(
                                sbLocalDatabaseGUIDArray *aGUIDArray)
{
  NS_ENSURE_TRUE(aGUIDArray, /*void*/);
  nsAutoMonitor mon(mDependentGUIDArrayMonitor);

  guidarrayptr_set_t::iterator it = mDependentGUIDArraySet.find(aGUIDArray);
  if(it != mDependentGUIDArraySet.end()) {
    mDependentGUIDArraySet.erase(it);
  }

  return;
}

nsresult
sbLocalDatabasePropertyCache::DispatchFlush()
{
  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbLocalDatabasePropertyCache, this, RunFlushThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_FAILURE);

  nsresult rv = mThreadPoolService->Dispatch(runnable, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("property cache flush operation dispatched"));

  return NS_OK;
}

void
sbLocalDatabasePropertyCache::RunFlushThread()
{
  nsresult SB_UNUSED_IN_RELEASE(rv) = Write();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to flush property cache; will retry");
}

nsresult
sbLocalDatabasePropertyCache::MakeQuery(sbIDatabaseQuery** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

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
  rv = MakeQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sbLocalDatabaseSQL::PropertiesSelect());
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

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
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aBag);
  nsAutoString guid(aGuid);

  {
    nsAutoMonitor mon(mMonitor);

    // If another bag for the same guid is already in the dirty list, then we
    // risk losing information if we don't write out immediately.
    if (mDirty.Get(guid, nsnull)) {
      NS_WARNING("Property cache forcing Write() due to duplicate "
                 "guids in the dirty bag list.  This should be a rare event.");
      rv = Write();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    mDirty.Put(guid, aBag);
    ++mWritePendingCount;
  }

  // Invalidate dependent guid arrays.
  nsCOMArray<sbLocalDatabaseGUIDArray> arrays;
  {
    nsAutoMonitor mon(mDependentGUIDArrayMonitor);
    guidarrayptr_set_t::const_iterator cit = mDependentGUIDArraySet.begin();
    guidarrayptr_set_t::const_iterator end = mDependentGUIDArraySet.end();
    for(; cit != end; ++cit) {
      NS_ENSURE_TRUE(arrays.AppendObject(*cit), NS_ERROR_OUT_OF_MEMORY);
    }
  }

  PRInt32 const count = arrays.Count();
  for (PRInt32 index = 0; index < count; ++index) {
    rv = arrays[index]->MayInvalidate(guid, aBag);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
      "Failed to invalidate GUID array, GUIDs may be stale.");
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
  nsresult rv = MakeQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(sbLocalDatabaseSQL::PropertiesTableInsert());
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
  NS_ENSURE_TRUE(result, NS_ERROR_FAILURE);

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

nsresult
sbLocalDatabasePropertyCache::CreateSecondarySortValue(
    sbILocalDatabaseResourcePropertyBag* aBag,
    PRUint32 aPropertyDBID,
    nsAString& _retval)
{
  NS_ENSURE_ARG_POINTER(aBag);
  nsresult rv = NS_OK;
  _retval.Truncate();

  // Get the secondary sort properties for the given property
  nsString id;
  rv = GetPropertyID(aPropertyDBID, id);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyInfo> propertyInfo;
  rv = mPropertyManager->GetPropertyInfo(id,
                                         getter_AddRefs(propertyInfo));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPropertyArray> secondaryProps;
  rv = propertyInfo->GetSecondarySort(getter_AddRefs(secondaryProps));
  NS_ENSURE_SUCCESS(rv, rv);

  // If we found secondary properties, get the sort value for
  // each and concat them together
  if (secondaryProps) {
    PRUint32 secondaryPropCount;
    rv = secondaryProps->GetLength(&secondaryPropCount);
    NS_ENSURE_SUCCESS(rv, rv);

    nsTArray<nsString> strings(secondaryPropCount);

    for (PRUint32 i = 0; i < secondaryPropCount; i++) {
      nsCOMPtr<sbIProperty> property;
      rv = secondaryProps->GetPropertyAt(i, getter_AddRefs(property));
      NS_ENSURE_SUCCESS(rv, rv);

      nsString propertyID;
      rv = property->GetId(propertyID);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString sortable;
      rv = aBag->GetSortablePropertyByID(GetPropertyDBIDInternal(propertyID),
                                   sortable);
      NS_ENSURE_SUCCESS(rv, rv);

      nsString* appended = strings.AppendElement(sortable);
      NS_ENSURE_TRUE(appended, NS_ERROR_OUT_OF_MEMORY);
    }

    // Use a small separator so that Foo|Bar comes before Foobar|Baz
    // \u001f is INFO SEPARATOR 1
    nsString result;
    sbAppendStringArray(result, NS_LITERAL_STRING("\x1f"), strings);
    _retval = result;
  }

  return NS_OK;
}



NS_IMPL_THREADSAFE_ISUPPORTS5(sbLocalDatabaseSortInvalidateJob,
                              nsIClassInfo,
                              sbIJobProgress,
                              nsIRunnable,
                              sbIMediaListEnumerationListener,
                              nsIObserver);

NS_IMPL_CI_INTERFACE_GETTER5(sbLocalDatabaseSortInvalidateJob,
                             nsIClassInfo,
                             sbIJobProgress,
                             nsIRunnable,
                             sbIMediaListEnumerationListener,
                             nsIObserver)

NS_DECL_CLASSINFO(sbLocalDatabaseSortInvalidateJob)
NS_IMPL_THREADSAFE_CI(sbLocalDatabaseSortInvalidateJob)

sbLocalDatabaseSortInvalidateJob::sbLocalDatabaseSortInvalidateJob() :
  mShouldShutdown(PR_FALSE),
  mThread(nsnull),
  mLibrary(nsnull),
  mPropCache(nsnull),
  mNotificationTimer(nsnull),
  mStatus(sbIJobProgress::STATUS_RUNNING),
  mCompletedItemCount(0),
  mTotalItemCount(0)
{
  MOZ_COUNT_CTOR(sbLocalDatabaseSortInvalidateJob);
}

sbLocalDatabaseSortInvalidateJob::~sbLocalDatabaseSortInvalidateJob()
{
  MOZ_COUNT_DTOR(sbLocalDatabaseSortInvalidateJob);
  TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Destroyed",
        this));
  Shutdown();
}

nsresult sbLocalDatabaseSortInvalidateJob::Init(
  sbLocalDatabasePropertyCache* aPropCache,
  sbLocalDatabaseLibrary* aLibrary)
{
  NS_ENSURE_ARG_POINTER(aPropCache);
  NS_ENSURE_ARG_POINTER(aLibrary);
  NS_ENSURE_TRUE(!mThread, NS_ERROR_ALREADY_INITIALIZED);
  NS_ASSERTION(NS_IsMainThread(),
    "sbLocalDatabaseSortInvalidateJob::Init called off the main thread!");
  TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Initialized",
        this));
  nsresult rv;

  mPropCache = aPropCache;
  mLibrary = aLibrary;

  // Set up job progress params
  mLibrary->GetLength(&mTotalItemCount);
  mCompletedItemCount = 0;

  // Localize some strings
  sbStringBundle strings;
  mTitleText = strings.Get("propertycache.invalidatesortjob.title",
                           "Updating Library");
  mStatusText = strings.Get("propertycache.invalidatesortjob.status",
                            "Rebuilding library sorting data");
  mFailedText = strings.Get("propertycache.invalidatesortjob.failed",
                            "Failed!");

  // Start a timer to send job progress notifications
  if (!mNotificationTimer) {
    mNotificationTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = mNotificationTimer->Init(this,
                                SORTINVALIDATE_TIMER_PERIOD,
                                nsITimer::TYPE_REPEATING_SLACK);
  NS_ENSURE_SUCCESS(rv, rv);

  // Start up processing thread
  rv = NS_NewThread(getter_AddRefs(mThread), this);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbLocalDatabaseSortInvalidateJob::Shutdown()
{
  NS_ASSERTION(NS_IsMainThread(),
    "sbLocalDatabaseSortInvalidateJob::Shutdown called off the main thread!");
  TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Shutdown requested",
        this));
  nsresult rv;

  mShouldShutdown = PR_TRUE;

  // We're done. No more notifications needed.
  mListeners.Clear();

  if (mNotificationTimer) {
    rv = mNotificationTimer->Cancel();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to cancel a notification timer");
    mNotificationTimer = nsnull;
  }

  // Wait for the thread to shutdown
  if (mThread) {
    mThread->Shutdown();
    mThread = nsnull;
  }

  return NS_OK;
}

/**
 * nsIRunnable implementation.  Called by mThread.
 */
NS_IMETHODIMP sbLocalDatabaseSortInvalidateJob::Run()
{
  TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Thread Starting",
        this));
  nsresult rv;

  nsCOMPtr<sbIMediaListBatchCallback> batchCallback =
    new sbMediaListBatchCallback(&sbLocalDatabaseSortInvalidateJob::RunLibraryBatch);
  NS_ENSURE_TRUE(batchCallback, NS_ERROR_OUT_OF_MEMORY);

  rv = mLibrary->RunInBatchMode(
          batchCallback, static_cast<sbIJobProgress*>(this));
  if (NS_FAILED(rv)) {
    mStatus = sbIJobProgress::STATUS_FAILED;
  }

  TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Thread Finished", this));
  return NS_OK;
}

/* static */
nsresult
sbLocalDatabaseSortInvalidateJob::RunLibraryBatch(nsISupports* aUserData)
{
  TRACE(("sbLocalDatabaseSortInvalidateJob::RunLibraryBatch[0x%.8x]"));
  NS_ENSURE_ARG_POINTER(aUserData);
  sbLocalDatabaseSortInvalidateJob* thisJob =
      static_cast<sbLocalDatabaseSortInvalidateJob*>(
        static_cast<sbIJobProgress*>(aUserData));
  NS_ENSURE_TRUE(thisJob->mPropCache, NS_ERROR_UNEXPECTED);

  // Begin processing every item in the library
  nsresult rv = thisJob->mLibrary->EnumerateAllItems(thisJob,
            sbIMediaList::ENUMERATIONTYPE_LOCKING);
  if (NS_FAILED(rv)) {
    thisJob->mStatus = sbIJobProgress::STATUS_FAILED;
  }

  return NS_OK;
}

// ======================================================
// ==  sbIMediaListEnumerationListener Implementation  ==
// ======================================================

/* unsigned short onEnumerationBegin (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbLocalDatabaseSortInvalidateJob::OnEnumerationBegin(sbIMediaList *aMediaList,
                                                     PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

/* unsigned short onEnumeratedItem (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP
sbLocalDatabaseSortInvalidateJob::OnEnumeratedItem(sbIMediaList *aMediaList,
                                                   sbIMediaItem *aMediaItem,
                                                   PRUint16 *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);
  nsresult rv = NS_OK;

  // On error, keep enumerating
  *_retval = sbIMediaListEnumerationListener::CONTINUE;


  // Loop through all the properties for the current item,
  // and set back all non top-level properties.
  // This will force the sortable and secondary sortable
  // info to be recomputed.

  nsCOMPtr<sbIPropertyArray> properties;
  rv = aMediaItem->GetProperties(nsnull, getter_AddRefs(properties));
  NS_ENSURE_SUCCESS(rv, NS_OK);

  nsCOMPtr<sbIMutablePropertyArray> newPropertyArray =
      do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = newPropertyArray->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  PRUint32 propCount;
  rv = properties->GetLength(&propCount);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  for (PRUint32 i = 0; i < propCount; i++) {
    nsCOMPtr<sbIProperty> property;
    rv = properties->GetPropertyAt(i, getter_AddRefs(property));
    NS_ENSURE_SUCCESS(rv, NS_OK);

    nsString propertyID;
    rv = property->GetId(propertyID);
    NS_ENSURE_SUCCESS(rv, NS_OK);

    if (!SB_IsTopLevelProperty(propertyID)) {
      nsString propertyValue;
      rv = property->GetValue(propertyValue);
      NS_ENSURE_SUCCESS(rv, NS_OK);

      rv = newPropertyArray->AppendProperty(propertyID, propertyValue);
      NS_ENSURE_SUCCESS(rv, NS_OK);
    }
  }

  rv = aMediaItem->SetProperties(newPropertyArray);
  NS_ENSURE_SUCCESS(rv, NS_OK);

  mCompletedItemCount++;

  if (mShouldShutdown) {
    *_retval = sbIMediaListEnumerationListener::CANCEL;
    mStatus = sbIJobProgress::STATUS_FAILED;
    TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Thread saw shutdown request",
          this));
  }

  return NS_OK;
}

/* void onEnumerationEnd (in sbIMediaList aMediaList, in nsresult aStatusCode); */
NS_IMETHODIMP
sbLocalDatabaseSortInvalidateJob::OnEnumerationEnd(sbIMediaList *aMediaList,
                                                   nsresult aStatusCode)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  if (mCompletedItemCount == mTotalItemCount) {
    mStatus = sbIJobProgress::STATUS_SUCCEEDED;
  } else {
    mStatus = sbIJobProgress::STATUS_FAILED;
  }

  mShouldShutdown = PR_TRUE;

  TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Finished enumerating",
        this));

  return NS_OK;
}

// =====================================
// ==  sbIJobProgress Implementation  ==
// =====================================

/* readonly attribute unsigned short status; */
NS_IMETHODIMP sbLocalDatabaseSortInvalidateJob::GetStatus(PRUint16* aStatus)
{
  NS_ENSURE_ARG_POINTER( aStatus );
  *aStatus = mStatus;
  return NS_OK;
}

/* readonly attribute boolean blocked; */
NS_IMETHODIMP sbLocalDatabaseSortInvalidateJob::GetBlocked(PRBool* aBlocked)
{
  NS_ENSURE_ARG_POINTER( aBlocked );
  *aBlocked = PR_FALSE;
  return NS_OK;
}

/* readonly attribute unsigned AString statusText; */
NS_IMETHODIMP sbLocalDatabaseSortInvalidateJob::GetStatusText(nsAString& aText)
{
  NS_ASSERTION(NS_IsMainThread(), \
    "sbLocalDatabaseSortInvalidateJob::GetStatusText is main thread only!");
  nsresult rv = NS_OK;

  if (mStatus == sbIJobProgress::STATUS_FAILED) {
    aText = mFailedText;
  } else {
    aText = mStatusText;
  }

  return rv;
}

/* readonly attribute AString titleText; */
NS_IMETHODIMP sbLocalDatabaseSortInvalidateJob::GetTitleText(nsAString& aText)
{
  aText = mTitleText;
  return NS_OK;
}

/* readonly attribute unsigned long progress; */
NS_IMETHODIMP sbLocalDatabaseSortInvalidateJob::GetProgress(PRUint32* aProgress)
{
  NS_ENSURE_ARG_POINTER( aProgress );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbLocalDatabaseSortInvalidateJob::GetProgress is main thread only!");

  *aProgress = mCompletedItemCount;
  return NS_OK;
}

/* readonly attribute unsigned long total; */
NS_IMETHODIMP sbLocalDatabaseSortInvalidateJob::GetTotal(PRUint32* aTotal)
{
  NS_ENSURE_ARG_POINTER( aTotal );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbLocalDatabaseSortInvalidateJob::GetTotal is main thread only!");

  *aTotal = mTotalItemCount;
  return NS_OK;
}

/* readonly attribute unsigned long errorCount; */
NS_IMETHODIMP sbLocalDatabaseSortInvalidateJob::GetErrorCount(PRUint32* aCount)
{
  NS_ENSURE_ARG_POINTER( aCount );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbLocalDatabaseSortInvalidateJob::GetErrorCount is main thread only!");
  // We don't provide additional info
  *aCount = 0;
  return NS_OK;
}

/* nsIStringEnumerator getErrorMessages(); */
NS_IMETHODIMP sbLocalDatabaseSortInvalidateJob::GetErrorMessages(nsIStringEnumerator** aMessages)
{
  NS_ENSURE_ARG_POINTER(aMessages);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbLocalDatabaseSortInvalidateJob::GetErrorMessages is main thread only!");

  // No error messages, so just give back an empty enumerator
  *aMessages = nsnull;
  nsTArray<nsString> empty;
  nsCOMPtr<nsIStringEnumerator> enumerator =
     new sbTArrayStringEnumerator(&empty);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  enumerator.forget(aMessages);

  return NS_OK;
}

/* void addJobProgressListener( in sbIJobProgressListener aListener ); */
NS_IMETHODIMP
sbLocalDatabaseSortInvalidateJob::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbLocalDatabaseSortInvalidateJob::AddJobProgressListener is main thread only!");
  TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Listener added",
        this));

  PRInt32 index = mListeners.IndexOf(aListener);
  if (index >= 0) {
    // the listener already exists, do not re-add
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }
  PRBool succeeded = mListeners.AppendObject(aListener);
  return succeeded ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

/* void removeJobProgressListener( in sbIJobProgressListener aListener ); */
NS_IMETHODIMP
sbLocalDatabaseSortInvalidateJob::RemoveJobProgressListener(sbIJobProgressListener* aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbLocalDatabaseSortInvalidateJob::RemoveJobProgressListener is main thread only!");
  TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Listener removed",
        this));

  PRInt32 indexToRemove = mListeners.IndexOf(aListener);
  if (indexToRemove < 0) {
    // Err, no such listener
    return NS_ERROR_UNEXPECTED;
  }

  // remove the listener
  PRBool succeeded = mListeners.RemoveObjectAt(indexToRemove);
  NS_ENSURE_TRUE(succeeded, NS_ERROR_FAILURE);

  return NS_OK;
}

// ==================================
// ==  nsIObserver Implementation  ==
// ==================================

// Called by mNotification Timer.
NS_IMETHODIMP
sbLocalDatabaseSortInvalidateJob::Observe(nsISupports *aSubject,
                                          const char *aTopic,
                                          const PRUnichar *aData)
{
  TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Notification Timer Callback", this));
  nsresult rv;

  // Then announce status to the world
  for (PRInt32 i = mListeners.Count() - 1; i >= 0; --i) {
     mListeners[i]->OnJobProgress(this);
  }

  if (mStatus != sbIJobProgress::STATUS_RUNNING) {
    TRACE(("sbLocalDatabaseSortInvalidateJob[0x%.8x] - Finishing job...", this));
    Shutdown();

    // Write everything to disk so that the new sort data takes effect
    rv = mLibrary->Flush();
    NS_ASSERTION(NS_SUCCEEDED(rv),
      "sbLocalDatabaseSortInvalidateJob failed to flush library!");

    // Vacuum, analyze, etc.
    rv = mLibrary->Optimize();
    NS_ASSERTION(NS_SUCCEEDED(rv),
      "sbLocalDatabaseSortInvalidateJob failed to optimize library!");


    // Let the property cache know that we're done.
    // --------------------------------- NOTE ---------------------------------
    // This call may release our instance, deleting us.
    // It is not safe to use any member variables after this point
    mPropCache->InvalidateSortDataComplete();
  }

  return NS_OK;
}

