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

#include <nsICategoryManager.h>
#include <nsIConverterInputStream.h>
#include <nsIInputStream.h>
#include <nsILocalFile.h>
#include <nsIObserverService.h>

#include <nsComponentManagerUtils.h>
#include <nsNetUtil.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOMCID.h>

#include <sbILibraryManager.h>
#include <sbIPlaybackHistoryEntry.h>

#include <DatabaseQuery.h>
#include <sbSQLBuilderCID.h>

#define NS_APPSTARTUP_CATEGORY           "app-startup"
#define NS_APPSTARTUP_TOPIC              "app-startup"

#define CONVERTER_BUFFER_SIZE 8192

#define PLAYBACKHISTORY_DB_GUID "playbackhistory@songbirdnest.com"
#define SCHEMA_URL \
  "chrome://songbird/content/mediacore/playback/history/playbackhistoryservice.sql"

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

  return NS_ERROR_NOT_IMPLEMENTED;
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

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::AddEntries(nsIArray *aEntries, 
                                     nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(aEntries);
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntryByIndex(PRInt64 aIndex, 
                                          sbIPlaybackHistoryEntry **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
sbPlaybackHistoryService::GetEntriesByIndex(PRInt64 aStartIndex, 
                                            PRUint64 aCount, 
                                            nsIArray **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  return NS_ERROR_NOT_IMPLEMENTED;
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
  return NS_ERROR_NOT_IMPLEMENTED;
}
