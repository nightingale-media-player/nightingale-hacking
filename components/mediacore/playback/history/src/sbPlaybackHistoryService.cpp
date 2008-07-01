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

#include "sbPlaybackHistoryService.h"

#include <nsIArray.h>
#include <nsICategoryManager.h>
#include <nsIConverterInputStream.h>
#include <nsIInputStream.h>
#include <nsILocalFile.h>
#include <nsIMutableArray.h>
#include <nsIObserverService.h>

#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCID.h>

#include <prprf.h>

#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbIPlaybackHistoryEntry.h>
#include <sbISQLBuilder.h>

#include <DatabaseQuery.h>
#include <sbArray.h>
#include <sbLibraryCID.h>
#include <sbSQLBuilderCID.h>
#include <sbStringUtils.h>

#define NS_APPSTARTUP_CATEGORY           "app-startup"
#define NS_APPSTARTUP_TOPIC              "app-startup"

#define CONVERTER_BUFFER_SIZE 8192

#define PLAYBACKHISTORY_DB_GUID "playbackhistory@songbirdnest.com"
#define SCHEMA_URL \
  "chrome://songbird/content/mediacore/playback/history/playbackhistoryservice.sql"

#define PLAYBACKHISTORY_ENTRIES_TABLE     "playback_history_entries"
#define PLAYBACKHISTORY_ANNOTATIONS_TABLE "playback_history_entry_annotations"
#define PLAYBACKHISTORY_PROPERTIES_TABLE  "properties"
#define PLAYBACKHISTORY_COUNT_ENTRIES     "count(entry_id)"

#define ENTRY_ID_COLUMN         "entry_id"
#define LIBRARY_GUID_COLUMN     "library_guid"
#define MEDIA_ITEM_GUID_COLUMN  "media_item_guid"
#define PLAY_TIME_COLUMN        "play_time"
#define PLAY_DURATION_COLUMN    "play_duration"

#define ENTRY_ID_COLUMN       "entry_id"
#define PROPERTY_ID_COLUMN    "property_id"
#define PROPERTY_NAME_COLUMN  "property_name"
#define OBJ_COLUMN            "obj"
#define OBJ_SORTABLE          "obj_sortable"

//------------------------------------------------------------------------------
// Support Functions
//------------------------------------------------------------------------------
static already_AddRefed<nsILocalFile>
GetDBFolder()
{
  nsresult rv;
  nsCOMPtr<nsIProperties> ds =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsILocalFile* file;
  rv = ds->Get("ProfD", NS_GET_IID(nsILocalFile), (void**)&file);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = file->AppendRelativePath(NS_LITERAL_STRING("db"));
  if (NS_FAILED(rv)) {
    NS_WARNING("AppendRelativePath failed!");

    NS_RELEASE(file);
    return nsnull;
  }

  return file;
}

//-----------------------------------------------------------------------------
// sbPlaybackHistoryService
//-----------------------------------------------------------------------------
NS_IMPL_THREADSAFE_ISUPPORTS2(sbPlaybackHistoryService, 
                              sbIPlaybackHistoryService,
                              nsIObserver)

sbPlaybackHistoryService::sbPlaybackHistoryService()
{
  MOZ_COUNT_CTOR(sbPlaybackHistoryService);
}

sbPlaybackHistoryService::~sbPlaybackHistoryService()
{
  MOZ_COUNT_DTOR(sbPlaybackHistoryService);
}

/*static*/ NS_METHOD 
sbPlaybackHistoryService::RegisterSelf(nsIComponentManager* aCompMgr,
                                       nsIFile* aPath,
                                       const char* aLoaderStr,
                                       const char* aType,
                                       const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->AddCategoryEntry(NS_APPSTARTUP_CATEGORY,
                                         SB_PLAYBACKHISTORYSERVICE_DESCRIPTION,
                                         "service,"
                                         SB_PLAYBACKHISTORYSERVICE_CONTRACTID,
                                         PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_METHOD 
sbPlaybackHistoryService::Init()
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, 
                                    SB_LIBRARY_MANAGER_READY_TOPIC, 
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);


  rv = observerService->AddObserver(this, 
                                    SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, 
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateQueries();
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool success = mLibraries.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::CreateQueries()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  NS_NAMED_LITERAL_STRING(playbackHistoryEntriesTableName, 
                          PLAYBACKHISTORY_ENTRIES_TABLE);

  NS_NAMED_LITERAL_STRING(playbackHistoryAnnotationsTableName,
                          PLAYBACKHISTORY_ANNOTATIONS_TABLE);

    NS_NAMED_LITERAL_STRING(playbackHistoryCountEntries, 
                            PLAYBACKHISTORY_COUNT_ENTRIES);

  NS_NAMED_LITERAL_STRING(entryIdColumn, ENTRY_ID_COLUMN);
  NS_NAMED_LITERAL_STRING(libraryGuidColumn, LIBRARY_GUID_COLUMN);
  NS_NAMED_LITERAL_STRING(mediaItemGuidColumn, MEDIA_ITEM_GUID_COLUMN);
  NS_NAMED_LITERAL_STRING(playTimeColumn, PLAY_TIME_COLUMN);
  NS_NAMED_LITERAL_STRING(playDurationColumn, PLAY_DURATION_COLUMN);

  nsCOMPtr<sbISQLInsertBuilder> insertBuilder = 
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Inserting new Playback History Entries.
  rv = insertBuilder->SetIntoTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(libraryGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(mediaItemGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(playTimeColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->AddColumn(playDurationColumn);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = insertBuilder->AddValueParameter();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->ToString(mAddEntryQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = insertBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Inserting Annotations
  // XXXAus: PLACEHOLDER

  // Query for Entry Count
  nsCOMPtr<sbISQLSelectBuilder> selectBuilder = 
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playbackHistoryCountEntries);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetEntryCountQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Getting Entries by Index
  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), entryIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), libraryGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), mediaItemGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playTimeColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playDurationColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddOrder(EmptyString(), playTimeColumn, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetLimitIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetOffsetIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetEntriesByIndexQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Same as above, but ascending instead of descending.
  rv = selectBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetBaseTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), entryIdColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), libraryGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), mediaItemGuidColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playTimeColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddColumn(EmptyString(), playDurationColumn);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->AddOrder(EmptyString(), playTimeColumn, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetLimitIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->SetOffsetIsParameter(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetEntriesByIndexQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = selectBuilder->ToString(mGetEntriesByIndexQueryAscending);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Getting Entries by Timestamp
  // XXXAus: PLACEHOLDER

  // Query for Deleting Entries by Index
  nsCOMPtr<sbISQLDeleteBuilder> deleteBuilder =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  //rv = deleteBuilder->SetTableName(playbackHistoryEntriesTableName);
  //NS_ENSURE_SUCCESS(rv, rv);

  //nsCOMPtr<sbISQLBuilderCriterion> criterion;

  // Query for Deleting All Entries
  rv = deleteBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->SetTableName(playbackHistoryEntriesTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->ToString(mRemoveAllEntriesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  // Query for Deleting All Annotations
  rv = deleteBuilder->Reset();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->SetTableName(playbackHistoryAnnotationsTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = deleteBuilder->ToString(mRemoveAllAnnotationsQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbPlaybackHistoryService::CreateDefaultQuery(sbIDatabaseQuery **aQuery)
{
  NS_ENSURE_ARG_POINTER(aQuery);

  NS_NAMED_LITERAL_STRING(playbackHistoryDatabaseGUID, 
                          PLAYBACKHISTORY_DB_GUID);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbIDatabaseQuery> query = 
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);

  rv = query->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(playbackHistoryDatabaseGUID);
  NS_ENSURE_SUCCESS(rv, rv);

  query.forget(aQuery);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::CreateEntryFromResultSet(sbIDatabaseResult *aResult,
                                                   PRUint32 aRow,
                                                   sbIPlaybackHistoryEntry **aEntry)
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_ARG_POINTER(aEntry);

  PRUint32 rowCount = 0;
  nsresult rv = aResult->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(aRow < rowCount, NS_ERROR_INVALID_ARG);

  nsString entryId;
  rv = aResult->GetRowCell(aRow, 0, entryId);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString libraryGuid;
  rv = aResult->GetRowCell(aRow, 1, libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString mediaItemGuid;
  rv = aResult->GetRowCell(aRow, 2, mediaItemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString playTime;
  rv = aResult->GetRowCell(aRow, 3, playTime);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString playDuration;
  rv = aResult->GetRowCell(aRow, 4, playDuration);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 timestamp = ToInteger64(playTime, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 duration = ToInteger64(playDuration, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaItem> item;
  rv = GetItem(libraryGuid, mediaItemGuid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPlaybackHistoryEntry> entry;
  rv = CreateEntry(item, timestamp, duration, nsnull, getter_AddRefs(entry));
  NS_ENSURE_SUCCESS(rv, rv);

  entry.forget(aEntry);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::CreateEntriesFromResultSet(sbIDatabaseResult *aResult,
                                                     nsIArray **aEntries)
{
  NS_ENSURE_ARG_POINTER(aResult);
  NS_ENSURE_ARG_POINTER(aEntries);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIMutableArray> mutableArray = 
    do_CreateInstance(SB_THREADSAFE_ARRAY_CONTRACTID, &rv);

  PRUint32 rowCount = 0;
  rv = aResult->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  for(PRUint32 currentRow = 0; currentRow < rowCount; ++currentRow) {
    nsCOMPtr<sbIPlaybackHistoryEntry> entry;
    
    rv = CreateEntryFromResultSet(aResult, currentRow, getter_AddRefs(entry));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mutableArray->AppendElement(entry, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIArray> array = do_QueryInterface(mutableArray);
  array.forget(aEntries);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::EnsureHistoryDatabaseAvailable()
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<nsILocalFile> file = GetDBFolder();
  NS_ENSURE_TRUE(file, NS_ERROR_FAILURE);

  nsString dbFileName = NS_LITERAL_STRING(PLAYBACKHISTORY_DB_GUID);
  dbFileName.AppendLiteral(".db");

  rv = file->AppendRelativePath(dbFileName);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool exists = PR_FALSE;
  rv = file->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);

  if(exists) {
    return NS_OK;
  }

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance(SONGBIRD_DATABASEQUERY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->SetDatabaseGUID(NS_LITERAL_STRING(PLAYBACKHISTORY_DB_GUID));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> schemaURI;
  rv = NS_NewURI(getter_AddRefs(schemaURI), NS_LITERAL_CSTRING(SCHEMA_URL));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIInputStream> input;
  rv = NS_OpenURI(getter_AddRefs(input), schemaURI);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIConverterInputStream> converterStream =
    do_CreateInstance("@mozilla.org/intl/converter-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = converterStream->Init(input,
                             "UTF-8",
                             CONVERTER_BUFFER_SIZE,
                             nsIConverterInputStream::DEFAULT_REPLACEMENT_CHARACTER);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUnicharInputStream> unichar =
    do_QueryInterface(converterStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 read = 0;
  nsString response;
  rv = unichar->ReadString(PR_UINT32_MAX, response, &read);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(read, "Schema file zero bytes?");

  rv = unichar->Close();
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING(colonNewline, ";\n");

  PRInt32 posStart = 0;
  PRInt32 posEnd = response.Find(colonNewline, posStart);
  while (posEnd >= 0) {
    rv = query->AddQuery(Substring(response, posStart, posEnd - posStart));
    NS_ENSURE_SUCCESS(rv, rv);
    posStart = posEnd + 2;
    posEnd = response.Find(colonNewline, posStart);
  }

  PRInt32 dbOk = 0;
  rv = query->Execute(&dbOk);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(dbOk == 0, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::FillAddQueryParameters(sbIDatabaseQuery *aQuery,
                                                 sbIPlaybackHistoryEntry *aEntry)
{
  NS_ENSURE_ARG_POINTER(aQuery);
  NS_ENSURE_ARG_POINTER(aEntry);

  nsCOMPtr<sbIMediaItem> item;
  nsresult rv = aEntry->GetItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = item->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  nsString libraryGuid;
  rv = library->GetGuid(libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->BindStringParameter(0, libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString itemGuid;
  rv = item->GetGuid(itemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->BindStringParameter(1, itemGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  PRTime timestamp = 0;
  rv = aEntry->GetTimestamp(&timestamp);
  NS_ENSURE_SUCCESS(rv, rv);

  char buf[30];
  PRUint32 len = PR_snprintf(buf, sizeof(buf), "%lld", timestamp);

  NS_ConvertASCIItoUTF16 timestampString(buf, len);
  rv = aQuery->BindStringParameter(2, timestampString);
  NS_ENSURE_SUCCESS(rv, rv);

  PRTime duration = 0;
  rv = aEntry->GetDuration(&duration);
  NS_ENSURE_SUCCESS(rv, rv);

  // duration is optional so if it's not present we bind to null instead of 0.
  if(duration) {
    len = PR_snprintf(buf, sizeof(buf), "%lld", duration);
    NS_ConvertASCIItoUTF16 durationString(buf, len);
    
    rv = aQuery->BindStringParameter(3, durationString);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = aQuery->BindNullParameter(3);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::FillAddAnnotationsQueryParameters(
                                           sbIDatabaseQuery *aQuery,
                                           sbIPlaybackHistoryEntry *aEntry)
{
  NS_ENSURE_ARG_POINTER(aQuery);
  NS_ENSURE_ARG_POINTER(aEntry);

  return NS_OK;
}

nsresult 
sbPlaybackHistoryService::GetItem(const nsAString &aLibraryGuid,
                                  const nsAString &aItemGuid,
                                  sbIMediaItem **aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<sbILibrary> library;
  if(!mLibraries.Get(aLibraryGuid, getter_AddRefs(library))) {
    nsCOMPtr<sbILibraryManager> libraryManager = 
      do_GetService(SONGBIRD_LIBRARYMANAGER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = libraryManager->GetLibrary(aLibraryGuid, getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success = mLibraries.Put(aLibraryGuid, library);
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }
  
  nsCOMPtr<sbIMediaItem> item;
  rv = library->GetMediaItem(aItemGuid, getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  item.forget(aItem);

  return NS_OK;
}


//-----------------------------------------------------------------------------
// nsIObserver
//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbPlaybackHistoryService::Observe(nsISupports* aSubject,
                                  const char* aTopic,
                                  const PRUnichar* aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if(strcmp(aTopic, SB_LIBRARY_MANAGER_READY_TOPIC) == 0) {
    rv = observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_READY_TOPIC);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = EnsureHistoryDatabaseAvailable();
    NS_ENSURE_SUCCESS(rv, rv);
  } 
  else if(strcmp(aTopic, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC) == 0) {
    rv = 
      observerService->RemoveObserver(this, 
                                      SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
// sbIPlaybackHistoryService
//-----------------------------------------------------------------------------
NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntries(nsISimpleEnumerator * *aEntries)
{
  NS_ENSURE_ARG_POINTER(aEntries);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntryCount(PRUint64 *aEntryCount)
{
  NS_ENSURE_ARG_POINTER(aEntryCount);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mGetEntryCountQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbError, NS_ERROR_UNEXPECTED);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Make sure we get one row back
  NS_ENSURE_TRUE(rowCount == 1, NS_ERROR_UNEXPECTED);

  nsAutoString countStr;
  rv = result->GetRowCell(0, 0, countStr);
  NS_ENSURE_SUCCESS(rv, rv);

  *aEntryCount = ToInteger64(countStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::CreateEntry(sbIMediaItem *aItem, 
                                      PRInt64 aTimestamp, 
                                      PRInt64 aDuration, 
                                      sbIPropertyArray *aAnnotations, 
                                      sbIPlaybackHistoryEntry **_retval)
{
  nsresult rv = NS_ERROR_UNEXPECTED;

  nsCOMPtr<sbIPlaybackHistoryEntry> entry = 
    do_CreateInstance(SB_PLAYBACKHISTORYENTRY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = entry->Init(aItem, aTimestamp, aDuration, aAnnotations);
  NS_ENSURE_SUCCESS(rv, rv);

  entry.forget(_retval);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::AddEntry(sbIPlaybackHistoryEntry *aEntry)
{
  NS_ENSURE_ARG_POINTER(aEntry);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mAddEntryQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = FillAddQueryParameters(query, aEntry);
  NS_ENSURE_SUCCESS(rv, rv);

  // XXXAus: PLACEHOLDER, doesn't do anything yet.
  rv = FillAddAnnotationsQueryParameters(query, aEntry);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbError, NS_ERROR_UNEXPECTED);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::AddEntries(nsIArray *aEntries)
{
  NS_ENSURE_ARG_POINTER(aEntries);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(NS_LITERAL_STRING("BEGIN"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 length = 0;
  rv = aEntries->GetLength(&length);

  for(PRUint32 current = 0; current < length; ++current) {
    nsCOMPtr<sbIPlaybackHistoryEntry> entry = 
      do_QueryElementAt(aEntries, current, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = query->AddQuery(mAddEntryQuery);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = FillAddQueryParameters(query, entry);
    NS_ENSURE_SUCCESS(rv, rv);

    // XXXAus: PLACEHOLDER, doesn't do anything yet.
    rv = FillAddAnnotationsQueryParameters(query, entry);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = query->AddQuery(NS_LITERAL_STRING("COMMIT"));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbError, NS_ERROR_UNEXPECTED);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntryByIndex(PRInt64 aIndex, 
                                          sbIPlaybackHistoryEntry **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 actualIndex = aIndex;
  if(actualIndex >= 0) {
    rv = query->AddQuery(mGetEntriesByIndexQuery);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = query->AddQuery(mGetEntriesByIndexQueryAscending);
    NS_ENSURE_SUCCESS(rv, rv);

    actualIndex = PR_ABS(actualIndex) - 1;
  }

  rv = query->BindInt64Parameter(0, 1);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt64Parameter(1, actualIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbError, NS_ERROR_UNEXPECTED);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = CreateEntryFromResultSet(result, 0, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntriesByIndex(PRInt64 aStartIndex, 
                                            PRUint64 aCount, 
                                            nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 actualIndex = aStartIndex;
  if( actualIndex >= 0) {
    rv = query->AddQuery(mGetEntriesByIndexQuery);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = query->AddQuery(mGetEntriesByIndexQueryAscending);
    NS_ENSURE_SUCCESS(rv, rv);

    actualIndex = PR_ABS(actualIndex) - 1;
  }

  rv = query->BindInt64Parameter(0, aCount);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->BindInt64Parameter(1, actualIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbError, NS_ERROR_UNEXPECTED);

  nsCOMPtr<sbIDatabaseResult> result;
  rv = query->GetResultObject(getter_AddRefs(result));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreateEntriesFromResultSet(result, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntriesByTimestamp(PRInt64 aStartTimestamp, 
                                                PRInt64 aEndTimestamp, 
                                                nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::RemoveEntry(sbIPlaybackHistoryEntry *aEntry)
{
  NS_ENSURE_ARG_POINTER(aEntry);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::RemoveEntryByIndex(PRInt64 aIndex)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::RemoveEntriesByIndex(PRInt64 aStartIndex, 
                                               PRUint64 aCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::RemoveEntries(nsIArray *aEntries)
{
  NS_ENSURE_ARG_POINTER(aEntries);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::Clear()
{
  nsCOMPtr<sbIDatabaseQuery> query;
  nsresult rv = CreateDefaultQuery(getter_AddRefs(query));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mRemoveAllEntriesQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = query->AddQuery(mRemoveAllAnnotationsQuery);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 dbError = 0;
  rv = query->Execute(&dbError);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_SUCCESS(dbError, NS_ERROR_UNEXPECTED);

  return NS_OK;
}
