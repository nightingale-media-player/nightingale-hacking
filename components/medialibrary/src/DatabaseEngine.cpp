/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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

/**
 * \file DatabaseEngine.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include "DatabaseEngine.h"

#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsIFile.h>
#include <xpcom/nsILocalFile.h>
#include <string/nsString.h>
#include <string/nsStringAPI.h>
#include <xpcom/nsISimpleEnumerator.h>
#include <xpcom/nsDirectoryServiceDefs.h>
#include <unicharutil/nsUnicharUtils.h>

#include <nsAutoLock.h>

#ifdef DEBUG_locks
#include <nsPrintfCString.h>
#endif

#include <vector>
#include <prmem.h>
#include <prtypes.h>

#if defined(_WIN32)
  #include <direct.h>
#else
// on UNIX, strnicmp is called strncasecmp
#define strnicmp strncasecmp
#endif

#define USE_SQLITE_FULL_DISK_CACHING  1
#define SQLITE_MAX_RETRIES            66
//#define HARD_SANITY_CHECK             0
#define QUERY_PROCESSOR_THREAD_COUNT  4

const PRUnichar* ALL_TOKEN = NS_LITERAL_STRING("*").get();
const PRUnichar* DB_GUID_TOKEN = NS_LITERAL_STRING("%db_guid%").get();
const PRInt32 DB_GUID_TOKEN_LEN = 9;

// FUNCTIONS ==================================================================
int SQLiteCaseInsensitiveCollate(void *pData, int nLenA, const void *pStrA, int nLenB, const void *pStrB)
{
  nsString strA( (PRUnichar *)pStrA, nLenA / sizeof(PRUnichar) );
  nsString strB( (PRUnichar *)pStrB, nLenB / sizeof(PRUnichar) );

  ToLowerCase(strA);
  ToLowerCase(strB);

  if(strA.Equals(strB))
    return 0;

  if(strA < strB)
    return -1;
  
  if(strA > strB)
    return 1;

  return -1;
} //SQLiteCaseInsensitiveCollate

int SQLiteAuthorizer(void *pData, int nOp, const char *pArgA, const char *pArgB, const char *pDBName, const char *pTrigger)
{
  int ret = SQLITE_OK;

  CDatabaseQuery *pQuery = reinterpret_cast<CDatabaseQuery *>(pData);

  if(pQuery)
  {
    switch(nOp)
    {
      case SQLITE_CREATE_INDEX:
      {
      }
      break;

      case SQLITE_CREATE_TABLE:
      {
      }
      break;

      case SQLITE_CREATE_TEMP_INDEX:
      {
      }
      break;

      case SQLITE_CREATE_TEMP_TABLE:
      {
      }
      break;

      case SQLITE_CREATE_TEMP_TRIGGER:
      {
      }
      break;

      case SQLITE_CREATE_TEMP_VIEW:
      {
      }
      break;

      case SQLITE_CREATE_TRIGGER:
      {
      }
      break;

      case SQLITE_CREATE_VIEW:
      {
      }
      break;

      case SQLITE_DELETE:
      {
        if(pArgA && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            nsAutoLock lock(pQuery->m_pModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_DROP_INDEX:
      {
      }
      break;

      case SQLITE_DROP_TABLE:
      {
        if(pArgA && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            nsAutoLock lock(pQuery->m_pModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_DROP_TEMP_INDEX:
      {
      }
      break;

      case SQLITE_DROP_TEMP_TABLE:
      {
        if(pArgA && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            nsAutoLock lock(pQuery->m_pModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_DROP_TEMP_TRIGGER:
      {
      }
      break;

      case SQLITE_DROP_TEMP_VIEW:
      {
      }
      break;

      case SQLITE_DROP_TRIGGER:
      {
      }
      break;

      case SQLITE_DROP_VIEW:
      {
      }
      break;

      case SQLITE_INSERT:
      {
        if(pArgA && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            nsAutoLock lock(pQuery->m_pModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_PRAGMA:
      {
      }
      break;

      case SQLITE_READ:
      {
        //If there's a table name but no column name, it means we're doing the initial SELECT on the table.
        //This is a good time to check if the query is supposed to be persistent and insert it
        //into the list of persistent queries.
        if(pQuery->m_PersistentQuery && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_Engine->AddPersistentQuery(pQuery, pArgA);
        }
      }
      break;

      case SQLITE_SELECT:
      {
      }
      break;

      case SQLITE_TRANSACTION:
      {
      }
      break;

      case SQLITE_UPDATE:
      {
        if(pArgA && pArgB )
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;
          
          {
            nsAutoLock lock(pQuery->m_pModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_ATTACH:
      {
      }
      break;

      case SQLITE_DETACH:
      {
      }
      break;

    }
  }

  return ret;
} //SQLiteAuthorizer

NS_IMPL_THREADSAFE_ISUPPORTS1(CDatabaseEngine, sbIDatabaseEngine)
NS_IMPL_THREADSAFE_ISUPPORTS1(QueryProcessorThread, nsIRunnable)


// CLASSES ====================================================================
//-----------------------------------------------------------------------------
CDatabaseEngine::CDatabaseEngine()
: m_QueryProcessorQueueHasItem(PR_FALSE)
, m_QueryProcessorShouldShutdown(PR_FALSE)
{
  m_pDatabasesLock = PR_NewLock();
  m_pDatabaseLocksLock = PR_NewLock();
  m_pQueryProcessorMonitor  = nsAutoMonitor::NewMonitor("CDatabaseEngine.m_pQueryProcessorMonitor");
  m_pPersistentQueriesLock = PR_NewLock();
  m_pCaseConversionLock = PR_NewLock();

  NS_ASSERTION(m_pDatabasesLock, "CDatabaseEngine.mpDatabaseLock failed");
  NS_ASSERTION(m_pDatabaseLocksLock, "CDatabaseEngine.m_pDatabasesLocksLock failed");
  NS_ASSERTION(m_pQueryProcessorMonitor, "CDatabaseEngine.m_pQueryProcessorMonitor failed");
  NS_ASSERTION(m_pPersistentQueriesLock, "CDatabaseEngine.m_pPersistentQueriesLock failed");
  NS_ASSERTION(m_pCaseConversionLock, "CDatabaseEngine.m_pCaseConversionLock failed");

#ifdef DEBUG_locks
  nsCAutoString log;
  log += NS_LITERAL_CSTRING("\n\nCDatabaseEngine (") + nsPrintfCString("%x", this) + NS_LITERAL_CSTRING(") lock addresses:\n");
  log += NS_LITERAL_CSTRING("m_pDatabasesLock         = ") + nsPrintfCString("%x\n", m_pDatabasesLock);
  log += NS_LITERAL_CSTRING("m_pDatabaseLocksLock     = ") + nsPrintfCString("%x\n", m_pDatabaseLocksLock);
  log += NS_LITERAL_CSTRING("m_pQueryProcessorMonitor = ") + nsPrintfCString("%x\n", m_pQueryProcessorMonitor);
  log += NS_LITERAL_CSTRING("m_pPersistentQueriesLock = ") + nsPrintfCString("%x\n", m_pPersistentQueriesLock);
  log += NS_LITERAL_CSTRING("m_pCaseConversionLock    = ") + nsPrintfCString("%x\n", m_pCaseConversionLock);
  log += NS_LITERAL_CSTRING("\n");
  NS_WARNING(log.get());
#endif

  for (PRInt32 i = 0; i < QUERY_PROCESSOR_THREAD_COUNT; i++) {
    nsCOMPtr<nsIThread> pThread;
    nsCOMPtr<nsIRunnable> pQueryProcessorRunner = new QueryProcessorThread(this);
    NS_ASSERTION(pQueryProcessorRunner, "Unable to create QueryProcessorRunner");
    if (!pQueryProcessorRunner)
      break;        
    nsresult rv = NS_NewThread(getter_AddRefs(pThread),
                               pQueryProcessorRunner,
                               0,
                               PR_JOINABLE_THREAD);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start thread");
    if (NS_SUCCEEDED(rv))
      m_QueryProcessorThreads.AppendObject(pThread);
  }
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CDatabaseEngine::~CDatabaseEngine()
{
  PRInt32 count = m_QueryProcessorThreads.Count();
  if (count) {

    {
      nsAutoMonitor mon(m_pQueryProcessorMonitor);
      m_QueryProcessorShouldShutdown = PR_TRUE;
      mon.NotifyAll();
    }

    for (PRInt32 i = 0; i < count; i++)
      m_QueryProcessorThreads[i]->Join();
  }

  ClearAllDBLocks();
  CloseAllDB();

  m_DatabaseLocks.clear();
  m_Databases.clear();

  if (m_pDatabasesLock)
    PR_DestroyLock(m_pDatabasesLock);
  if (m_pDatabaseLocksLock)
    PR_DestroyLock(m_pDatabaseLocksLock);
  if (m_pQueryProcessorMonitor)
    nsAutoMonitor::DestroyMonitor(m_pQueryProcessorMonitor);
  if (m_pPersistentQueriesLock)
    PR_DestroyLock(m_pPersistentQueriesLock);
  if (m_pCaseConversionLock)
    PR_DestroyLock(m_pCaseConversionLock);
} //dtor

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::OpenDB(PRUnichar *dbGUID)
{
  nsAutoLock lock(m_pDatabasesLock);

  sqlite3 *pDB = nsnull;
  std::prustring strDBGUID(dbGUID);
  std::prustring strFilename(dbGUID);

  strFilename += NS_LITERAL_STRING(".db").get();

  // Kick sqlite in the pants
  PRInt32 ret = sqlite3_open16(strFilename.c_str(), &pDB);

  // Remember what we just loaded
  if(ret == SQLITE_OK)
  {
    m_Databases.insert(std::make_pair(strDBGUID, pDB));

#if defined(USE_SQLITE_FULL_DISK_CACHING)
    sqlite3_busy_timeout(pDB, 5000);

    char *strErr = nsnull;
    sqlite3_exec(pDB, "PRAGMA synchronous = 0", nsnull, nsnull, &strErr);
    if(strErr) sqlite3_free(strErr);
#endif

    ret = sqlite3_create_collation16(pDB, (char *) NS_LITERAL_STRING("songbird_ci").get(), SQLITE_UTF16, nsnull, SQLiteCaseInsensitiveCollate);

    if(ret != SQLITE_OK)
    {
#if defined(XP_WIN)
      __asm int 3;
#endif
    }
  }

  return ret;
} //OpenDB

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::CloseDB(PRUnichar *dbGUID)
{
  PRInt32 ret = 0;
  nsAutoLock lock(m_pDatabasesLock);

  databasemap_t::iterator itDatabases = m_Databases.find(dbGUID);

  if(itDatabases != m_Databases.end())
  {
    ret = sqlite3_close(itDatabases->second);
    m_Databases.erase(itDatabases);
  }
 
  return ret;
} //CloseDB

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::DropDB(PRUnichar *dbGUID)
{
  return 0;
} //DropDB

//-----------------------------------------------------------------------------
/* [noscript] PRInt32 SubmitQuery (in CDatabaseQueryPtr dbQuery); */
NS_IMETHODIMP CDatabaseEngine::SubmitQuery(CDatabaseQuery * dbQuery, PRInt32 *_retval)
{
  *_retval = SubmitQueryPrivate(dbQuery);
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::SubmitQueryPrivate(CDatabaseQuery *dbQuery)
{
  PRInt32 ret = 0;

  if(!dbQuery) return 1;

  {
    nsAutoMonitor mon(m_pQueryProcessorMonitor);

    dbQuery->AddRef();
    dbQuery->m_IsExecuting = PR_TRUE;

    m_QueryQueue.push_back(dbQuery);
    m_QueryProcessorQueueHasItem = PR_TRUE;
    mon.Notify();
  }

  PRBool bAsyncQuery = PR_FALSE;
  dbQuery->IsAyncQuery(&bAsyncQuery);

  if(!bAsyncQuery)
  {
    dbQuery->WaitForCompletion(&ret);
    dbQuery->GetLastError(&ret);
  }

  return ret;
} //SubmitQuery

//-----------------------------------------------------------------------------
/* [noscript] void AddPersistentQuery (in CDatabaseQueryPtr dbQuery, in stlCStringRef strTableName); */
NS_IMETHODIMP CDatabaseEngine::AddPersistentQuery(CDatabaseQuery * dbQuery, const std::string & strTableName)
{
  AddPersistentQueryPrivate(dbQuery, strTableName);
  return NS_OK;
}

//-----------------------------------------------------------------------------
void CDatabaseEngine::AddPersistentQueryPrivate(CDatabaseQuery *pQuery, const std::string &strTableName)
{
  nsAutoLock lock(m_pPersistentQueriesLock);
    
  PRUnichar *strDBGUID = nsnull;
  pQuery->GetDatabaseGUID(&strDBGUID);

  querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.find(strDBGUID);
  if(itPersistentQueries != m_PersistentQueries.end())
  {
    tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(strTableName);
    if(itTableQuery != itPersistentQueries->second.end())
    {
      querylist_t::iterator itQueries = itTableQuery->second.begin();
      for( ; itQueries != itTableQuery->second.end(); itQueries++)
      {
        if((*itQueries) == pQuery)
          return;
      }

      NS_IF_ADDREF(pQuery);
      itTableQuery->second.insert(itTableQuery->second.end(), pQuery);
    }
    else
    {
      NS_IF_ADDREF(pQuery);

      querylist_t queryList;
      queryList.push_back(pQuery);

      itPersistentQueries->second.insert(std::make_pair<std::string, querylist_t>(strTableName, queryList));
    }
  }
  else
  {
    NS_IF_ADDREF(pQuery);

    querylist_t queryList;
    queryList.push_back(pQuery);

    tablepersistmap_t tableMap;
    tableMap.insert(std::make_pair<std::string, querylist_t>(strTableName, queryList));

    m_PersistentQueries.insert(std::make_pair<std::prustring, tablepersistmap_t>(strDBGUID, tableMap));
  }

  {
    nsAutoLock lock(pQuery->m_pPersistentQueryTableLock);
    pQuery->m_PersistentQueryTable = strTableName;
    pQuery->m_IsPersistentQueryRegistered = PR_TRUE;
  }

  PR_Free(strDBGUID);

} //AddPersistentQuery

//-----------------------------------------------------------------------------
/* [noscript] void RemovePersistentQuery (in CDatabaseQueryPtr dbQuery); */
NS_IMETHODIMP CDatabaseEngine::RemovePersistentQuery(CDatabaseQuery * dbQuery)
{
  RemovePersistentQueryPrivate(dbQuery);
  return NS_OK;
}

//-----------------------------------------------------------------------------
void CDatabaseEngine::RemovePersistentQueryPrivate(CDatabaseQuery *pQuery)
{
  if(!pQuery->m_IsPersistentQueryRegistered)
    return;

  nsAutoLock lock(m_pPersistentQueriesLock);

  std::string strTableName("");
  PRUnichar *strDBGUID = nsnull;
  pQuery->GetDatabaseGUID(&strDBGUID);

  {
    nsAutoLock lock(pQuery->m_pPersistentQueryTableLock);
    strTableName = pQuery->m_PersistentQueryTable;
  }

  querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.find(strDBGUID);
  if(itPersistentQueries != m_PersistentQueries.end())
  {
    tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(strTableName);
    if(itTableQuery != itPersistentQueries->second.end())
    {
      querylist_t::iterator itQueries = itTableQuery->second.begin();
      for( ; itQueries != itTableQuery->second.end(); itQueries++)
      {
        if((*itQueries) == pQuery)
        {
          NS_RELEASE((*itQueries));
          itTableQuery->second.erase(itQueries);

          return;
        }
      }
    }
  }

  PR_Free(strDBGUID);

  return;
} //RemovePersistentQuery

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::LockDatabase(sqlite3 *pDB)
{
  NS_ENSURE_ARG_POINTER(pDB);
  PRLock* lock = nsnull;
  {
    nsAutoLock dbLock(m_pDatabaseLocksLock);

    databaselockmap_t::iterator itLock = m_DatabaseLocks.find(pDB);
    if(itLock != m_DatabaseLocks.end()) {
      lock = itLock->second;
      NS_ENSURE_TRUE(lock, NS_ERROR_NULL_POINTER);
    }
    else {
      lock = PR_NewLock();
      NS_ENSURE_TRUE(lock, NS_ERROR_OUT_OF_MEMORY);
      m_DatabaseLocks.insert(std::make_pair<sqlite3*, PRLock*>(pDB, lock));
    }
  }
  PR_Lock(lock);
  return NS_OK;
} //LockDatabase

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::UnlockDatabase(sqlite3 *pDB)
{
  NS_ENSURE_ARG_POINTER(pDB);
  PRLock* lock = nsnull;
  {
    nsAutoLock dbLock(m_pDatabaseLocksLock);
    databaselockmap_t::iterator itLock = m_DatabaseLocks.find(pDB);
    NS_ASSERTION(itLock != m_DatabaseLocks.end(), "Called UnlockDatabase on a database that isn't in our map");
    lock = itLock->second;
  }
  NS_ASSERTION(lock, "Null lock stored in database map");
  PR_Unlock(lock);
  return NS_OK;
} //UnlockDatabase

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::ClearAllDBLocks()
{
  nsAutoLock dbLock(m_pDatabaseLocksLock);

  databaselockmap_t::iterator itLock = m_DatabaseLocks.begin();
  databaselockmap_t::iterator itEnd = m_DatabaseLocks.end();

  for(PRLock* lock = nsnull; itLock != itEnd; itLock++)
  {
    lock = itLock->second;
    if (lock) {
      PR_DestroyLock(lock);
      lock = nsnull;
    }
  }
  m_DatabaseLocks.clear();
  return NS_OK;
} //ClearAllDBLocks

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::CloseAllDB()
{
  PRInt32 ret = nsnull;
  nsAutoLock lock(m_pDatabasesLock);

  databasemap_t::iterator itDatabases = m_Databases.begin();

  if(itDatabases != m_Databases.end())
  {
    ret = sqlite3_close(itDatabases->second);
    m_Databases.erase(itDatabases);
  }

  return NS_OK;
} //CloseAllDB

//-----------------------------------------------------------------------------
sqlite3 *CDatabaseEngine::GetDBByGUID(PRUnichar *dbGUID, PRBool bCreateIfNotOpen /*= PR_FALSE*/)
{
  sqlite3 *pRet = nsnull;

  pRet = FindDBByGUID(dbGUID);

  if(pRet == nsnull)
  {
    int ret = OpenDB(dbGUID);
    if(ret == SQLITE_OK)
    {
      pRet = FindDBByGUID(dbGUID);
    }
  }

  return pRet;
} //GetDBByGUID

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::GetDBGUIDList(std::vector<std::prustring> &vGUIDList)
{
  PRInt32 nRet = 0;
  PRBool bFlag = PR_FALSE;
  nsresult rv;

  nsCOMPtr<nsIServiceManager> svcMgr;
  rv = NS_GetServiceManager(getter_AddRefs(svcMgr));

  nsCOMPtr<nsIProperties> directory;
  rv = svcMgr->GetServiceByContractID("@mozilla.org/file/directory_service;1",
    NS_GET_IID(nsIProperties),
    getter_AddRefs(directory));

  // Get it once so we can restore it
  nsCOMPtr<nsILocalFile> pDBDirectory = do_GetService("@mozilla.org/file/local;1");
  rv = directory->Get(NS_OS_CURRENT_WORKING_DIR, NS_GET_IID(nsIFile), getter_AddRefs(pDBDirectory));

  if(NS_FAILED(rv)) return 0;

  rv = pDBDirectory->IsDirectory(&bFlag);

  if(bFlag)
  {
    nsISimpleEnumerator *pDirEntries = nsnull;
    pDBDirectory->GetDirectoryEntries(&pDirEntries);

    if(pDirEntries)
    {
      PRBool bHasMore = PR_FALSE;

      for(;;)
      {
        if(pDirEntries)
          pDirEntries->HasMoreElements(&bHasMore);

        if(bHasMore)
        {

          nsCOMPtr<nsISupports> pDirEntry;
          pDirEntries->GetNext(getter_AddRefs(pDirEntry));

          if(pDirEntry)
          {
            nsIID nsIFileIID = NS_IFILE_IID;
            nsCOMPtr<nsIFile> pEntry;
            pDirEntry->QueryInterface(nsIFileIID, getter_AddRefs(pEntry));

            if(pEntry)
            {
              PRBool bIsFile = PR_FALSE;
              pEntry->IsFile(&bIsFile);

              if(bIsFile)
              {
                nsString strLeaf;
                pEntry->GetLeafName(strLeaf);

                nsString::const_iterator itStrStart, itStart, itEnd;
                strLeaf.BeginReading(itStrStart);
                strLeaf.BeginReading(itStart);
                strLeaf.EndReading(itEnd);

                PRBool bFound = FindInReadable(NS_LITERAL_STRING(".db"), itStart, itEnd);
                
                if(bFound)
                {
                  PRUint32 nCutPos = (PRUint32)Distance(itStrStart, itStart);
                  PRUint32 nCutLen = (PRUint32)Distance(itStart, itEnd);

                  strLeaf.Cut(nCutPos, nCutLen);
                  vGUIDList.push_back(strLeaf.get());

                  ++nRet;
                }
              }
            }
          }
        }
        else
        {
          break;
        }
      }
    }
  }

  return nRet;
} //GetDBGUIDList

//-----------------------------------------------------------------------------
sqlite3 *CDatabaseEngine::FindDBByGUID(PRUnichar *dbGUID)
{
  sqlite3 *pRet = nsnull;

  {
    nsAutoLock lock(m_pDatabasesLock);
    databasemap_t::const_iterator itDatabases = m_Databases.find(dbGUID);
    if(itDatabases != m_Databases.end())
    {
      pRet = itDatabases->second;
    }
  }

  return pRet;
} //FindDBByGUID

//-----------------------------------------------------------------------------
/*static*/ void PR_CALLBACK CDatabaseEngine::QueryProcessor(CDatabaseEngine* pEngine)
{
  CDatabaseQuery *pQuery = nsnull;

  while(PR_TRUE)
  {
    pQuery = nsnull;

    { // Enter Monitor
      nsAutoMonitor mon(pEngine->m_pQueryProcessorMonitor);
      while (!pEngine->m_QueryProcessorQueueHasItem && !pEngine->m_QueryProcessorShouldShutdown)
        mon.Wait();

      // Handle shutdown request
      if (pEngine->m_QueryProcessorShouldShutdown) {
#if defined(XP_WIN)
        sqlite3_thread_cleanup();
#endif
        return;
      }

      // We must have an item in the queue
      pQuery = pEngine->m_QueryQueue.front();
      pEngine->m_QueryQueue.pop_front();
      if (pEngine->m_QueryQueue.empty()) {
        pEngine->m_QueryProcessorQueueHasItem = PR_FALSE;
      }
    } // Exit Monitor

    NS_ASSERTION(pQuery, "How can we get here without a query?");

    PRBool bAllDB = PR_FALSE;
    sqlite3 *pDB = nsnull;
    sqlite3 *pSecondDB = nsnull;
    PRUnichar *pGUID = nsnull;
    pQuery->GetDatabaseGUID(&pGUID);
    
    // create |nsDependentString|s wrapping PRUnichar* buffers so we can
    // do fancy things in a platform independent way.
    nsString ALL_TOKEN_nsds(ALL_TOKEN);
    nsString pGUID_nsds(pGUID);
    PRBool bEquals = PR_FALSE;
    {
      // UnicharUtils is not threadsafe, so make sure we don't switch threads here.
      nsAutoLock ccLock(pEngine->m_pCaseConversionLock);

      ToLowerCase(ALL_TOKEN_nsds);
      ToLowerCase(pGUID_nsds);

      bEquals = ALL_TOKEN_nsds.Equals(pGUID_nsds);
    }

    if(bEquals)
    {
      bAllDB = PR_TRUE;
    }
    else
    {
      pDB = pEngine->GetDBByGUID(pGUID, PR_TRUE);
    }

    PR_Free(pGUID);

    if(!pDB && !bAllDB)
    {
      //Some bad error we'll talk about later.
      pQuery->SetLastError(SQLITE_ERROR);
    }
    else
    {
      std::vector<std::prustring> vDBList;
      PRInt32 nNumDB = 1;
      PRInt32 nQueryCount = 0;
      PRBool bFirstRow = PR_TRUE;

      //Default return error.
      pQuery->SetLastError(SQLITE_ERROR);
      pQuery->GetQueryCount(&nQueryCount);

      if(bAllDB)
        nNumDB = pEngine->GetDBGUIDList(vDBList);

      //pQuery->m_QueryResultLock.Lock();

      for(PRInt32 i = 0; i < nQueryCount && !pQuery->m_IsAborting; i++)
      {

        int retDB = 0;
        sqlite3_stmt *pStmt = nsnull;

        PRUnichar *pzQuery = nsnull;
        const void *pzTail = nsnull;
        std::prustring strQuery;

        pQuery->GetQuery(i, &pzQuery);

        strQuery = pzQuery;
        PR_Free(pzQuery);
        
        NS_NAMED_LITERAL_STRING(strAttachToken, "ATTACH DATABASE \"");

        size_t nPos = strQuery.find(strAttachToken.get());
        if(nPos != strQuery.npos)
        {
          NS_NAMED_LITERAL_STRING(strStartToken, "AS \"");
          NS_NAMED_LITERAL_STRING(strEndToken, "\"");

          nPos += strAttachToken.Length();
          nPos = strQuery.find(strStartToken.get(), nPos);

          nPos += strStartToken.Length();
          size_t nEndPos = strQuery.find(strEndToken.get(), nPos);
          
          std::prustring strSecondDBGUID = strQuery.substr(nPos, nEndPos - nPos);
          pSecondDB = pEngine->GetDBByGUID(const_cast<PRUnichar *>(strSecondDBGUID.c_str()), PR_TRUE);

          if(pSecondDB)
          {
            pEngine->LockDatabase(pSecondDB);
          }
        }

        for(PRInt32 j = 0; j < nNumDB; j++)
        {
          if(!bAllDB)
          {
            pEngine->LockDatabase(pDB);
            retDB = sqlite3_set_authorizer(pDB, SQLiteAuthorizer, pQuery);

#if defined(HARD_SANITY_CHECK)
            if(retDB != SQLITE_OK)
            {
              PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
              OutputDebugStringW(szErr);
              OutputDebugStringW(NS_L("\n"));
            }
#endif
          }
          else
          {
            pDB = pEngine->GetDBByGUID(const_cast<PRUnichar *>(vDBList[j].c_str()), PR_TRUE);
            
            //Just in case there was some kind of bad error while attempting to get the database or create it.
            if ( !pDB ) 
              continue;
            
            pEngine->LockDatabase(pDB);
            retDB = sqlite3_set_authorizer(pDB, SQLiteAuthorizer, pQuery);

#if defined(HARD_SANITY_CHECK)
            if(retDB != SQLITE_OK)
            {
              PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
              OutputDebugStringW(szErr);
              OutputDebugStringW(NS_L("\n"));
            }
#endif
          }

          retDB = sqlite3_prepare16(pDB, strQuery.c_str(), (int)strQuery.length() * sizeof(PRUnichar), &pStmt, &pzTail);
          pQuery->m_CurrentQuery = i;

          if(retDB != SQLITE_OK)
          {
            //Some bad error we'll talk about later.
            pQuery->SetLastError(SQLITE_ERROR);

#if defined(HARD_SANITY_CHECK)
            if(retDB != SQLITE_OK)
            {
              PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
              OutputDebugStringW(szErr);
              OutputDebugStringW(NS_L("\n"));
              //__asm int 3;
            }
#endif

          }
          else
          {
            PRInt32 nRetryCount = 0;

            do
            {
              retDB = sqlite3_step(pStmt);

              switch(retDB)
              {
                case SQLITE_ROW: 
                {
                  CDatabaseResult *pRes = pQuery->GetResultObject();
                  PR_Lock(pQuery->m_pQueryResultLock);

                  int nCount = sqlite3_column_count(pStmt);

                  if(bFirstRow)
                  {
                    bFirstRow = PR_FALSE;

                    PR_Unlock(pQuery->m_pQueryResultLock);
                    pRes->ClearResultSet();
                    PR_Lock(pQuery->m_pQueryResultLock);

                    std::vector<std::prustring> vColumnNames;
                    vColumnNames.reserve(nCount);

                    for(int i = 0; i < nCount; i++)
                    {
                      PRUnichar *p = (PRUnichar *)sqlite3_column_name16(pStmt, i);
                      std::prustring strColumnName;

                      if(p)
                      {
                        strColumnName = p;
                      }

                      vColumnNames.push_back(strColumnName);
                    }

                    pRes->SetColumnNames(vColumnNames);
                  }

                  std::vector<std::prustring> vCellValues;
                  vCellValues.reserve(nCount);

                  for(int i = 0; i < nCount; i++)
                  {
                    PRUnichar *p = (PRUnichar *)sqlite3_column_text16(pStmt, i);
                    std::prustring strCellValue;

                    if(p)
                    {
                      strCellValue = p;
                    }

                    vCellValues.push_back(strCellValue);
                  }

                  pRes->AddRow(vCellValues);
                  PR_Unlock(pQuery->m_pQueryResultLock);
                  //pRes->Release();
                }
                break;

                case SQLITE_DONE:
                {
                  pQuery->SetLastError(SQLITE_OK);
                }
                break;

                case SQLITE_MISUSE:
                case SQLITE_BUSY:
                {
                  if(nRetryCount++ > SQLITE_MAX_RETRIES)
                  {
                    PR_Sleep(33);
                    retDB = SQLITE_ROW;
                  }
                }
                break;

                default:
                {
                  pQuery->SetLastError(retDB);

#if defined(HARD_SANITY_CHECK)
                  PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
                  OutputDebugStringW(szErr);
                  OutputDebugStringW(NS_L("\n"));
#endif

                }
              }
            }
            while(retDB == SQLITE_ROW);
          }

          //Didn't get any rows
          if(bFirstRow)
          {
            PRBool isPersistent = PR_FALSE;
            pQuery->IsPersistentQuery(&isPersistent);

            if(isPersistent)
            {
              nsString strTableName(strQuery.c_str());
              PRBool bFound = PR_FALSE;
              nsString::const_iterator itStrStart, itStart, itEnd;
              {
                // UnicharUtils is not threadsafe, so make sure we don't switch threads here
                nsAutoLock ccLock(pEngine->m_pCaseConversionLock);
                ToLowerCase(strTableName);

                strTableName.BeginReading(itStrStart);
                strTableName.BeginReading(itStart);
                strTableName.EndReading(itEnd);

                bFound = FindInReadable(NS_LITERAL_STRING(" from "), itStart, itEnd);
              }
              if(bFound)
              {
                PRUint32 nCutLen = (PRUint32)Distance(itStrStart, itEnd);
                strTableName.Cut(0, nCutLen);

                strTableName.BeginReading(itStrStart);
                strTableName.BeginReading(itStart);
                while(*itStart == ' ')
                  itStart++;

                nCutLen = (PRUint32)Distance(itStrStart, itStart);
                strTableName.Cut(0, nCutLen);

                strTableName.BeginReading(itStart);
                strTableName.EndReading(itEnd);
                bFound = FindInReadable(NS_LITERAL_STRING(" "), itStart, itEnd);

                if(bFound)
                {
                  PRInt32 nEndLen = (PRInt32)Distance(itStrStart, itStart);
                  strTableName.SetLength(nEndLen);
                }

                nsCString theCTableName;
                NS_UTF16ToCString(strTableName, NS_CSTRING_ENCODING_ASCII, theCTableName);
                pEngine->AddPersistentQuery(pQuery, const_cast<char *>(theCTableName.get()));

              }
            }
          }

          //Always release the statement.
          retDB = sqlite3_finalize(pStmt);
          pEngine->UnlockDatabase(pDB);

#if defined(HARD_SANITY_CHECK)
          if(retDB != SQLITE_OK)
          {
            PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
            OutputDebugStringW(szErr);
            OutputDebugStringW(NS_L("\n"));
            //__asm int 3;
          }
#endif

        }
      }

      //pQuery->m_QueryResultLock.Unlock();
    }

    if(pSecondDB != nsnull)
    {
      pEngine->UnlockDatabase(pSecondDB);
    }

    pQuery->m_IsExecuting = PR_FALSE;
    pQuery->m_IsAborting = PR_FALSE;

    //Whatever happened, the query is done running now.
    {
      nsAutoMonitor mon(pQuery->m_pQueryRunningMonitor);
      pQuery->m_QueryHasCompleted = PR_TRUE;
      mon.Notify();
    }

    //Check if this query changed any data 
    pEngine->UpdatePersistentQueries(pQuery);

    //Check if this query is a persistent query so we can now fire off the callback.
    pEngine->DoPersistentCallback(pQuery);

    //Close up the db.
    //pEngine->CloseDB(pGUID);
    //PR_Free(pGUID);

    NS_IF_RELEASE(pQuery);
  } // while
} //QueryProcessor

//-----------------------------------------------------------------------------
void CDatabaseEngine::UpdatePersistentQueries(CDatabaseQuery *pQuery)
{
  if(pQuery->m_HasChangedDataOfPersistQuery && 
     !pQuery->m_PersistentQuery)
  {
    PRUnichar *pGUID = nsnull;
    pQuery->GetDatabaseGUID(&pGUID);

    if(pGUID)
    {
      nsAutoLock pqLock(m_pPersistentQueriesLock);
      querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.find(pGUID);

      if(itPersistentQueries != m_PersistentQueries.end())
      {
        std::string strTableName;
        
        nsAutoLock lock(pQuery->m_pModifiedTablesLock);
        CDatabaseQuery::modifiedtables_t::iterator i;
        for ( i = pQuery->m_ModifiedTables.begin(); i != pQuery->m_ModifiedTables.end(); ++i )
        {
          strTableName = *i;
          tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(strTableName);
          if(itTableQuery != itPersistentQueries->second.end())
          {
            querylist_t::iterator itQueries = itTableQuery->second.begin();
            for(; itQueries != itTableQuery->second.end(); itQueries++)
            {
              SubmitQueryPrivate((*itQueries));
            }
          }
        }
      }

      itPersistentQueries = m_PersistentQueries.find(ALL_TOKEN);
      if(itPersistentQueries != m_PersistentQueries.end())
      {
        std::string strTableName;

        nsAutoLock lock(pQuery->m_pModifiedTablesLock);
        CDatabaseQuery::modifiedtables_t::iterator i;
        for ( i = pQuery->m_ModifiedTables.begin(); i != pQuery->m_ModifiedTables.end(); ++i )
        {
          strTableName = *i;
          tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(strTableName);
          if(itTableQuery != itPersistentQueries->second.end())
          {
            querylist_t::iterator itQueries = itTableQuery->second.begin();
            for(; itQueries != itTableQuery->second.end(); itQueries++)
            {
              (*itQueries)->SetDatabaseGUID(ALL_TOKEN);
              SubmitQueryPrivate((*itQueries));
            }
          }
        }
      }

      PR_Free(pGUID);
    }
  }

  {
    nsAutoLock lock(pQuery->m_pModifiedTablesLock);
    pQuery->m_HasChangedDataOfPersistQuery = PR_FALSE;
    pQuery->m_ModifiedTables.clear();
  }

} //UpdatePersistentQueries

//-----------------------------------------------------------------------------
void CDatabaseEngine::DoPersistentCallback(CDatabaseQuery *pQuery)
{
  if(pQuery->m_PersistentQuery)
  {
    nsCOMPtr<sbIDatabaseResult> pDBResult;
    PRUnichar *strGUID = nsnull, *strQuery = nsnull;

    pQuery->GetResultObject(getter_AddRefs(pDBResult));
    pQuery->GetDatabaseGUID(&strGUID);
    pQuery->GetQuery(0, &strQuery);

    nsAutoLock lock(pQuery->m_pPersistentCallbackListLock);
    CDatabaseQuery::persistentcallbacklist_t::const_iterator itCallback = pQuery->m_PersistentCallbackList.begin();
    CDatabaseQuery::persistentcallbacklist_t::const_iterator itEnd = pQuery->m_PersistentCallbackList.end();

    for(; itCallback != itEnd; itCallback++)
    {
      if((*itCallback))
      {
        try
        {
          (*itCallback)->OnQueryEnd(pDBResult, strGUID, strQuery);
        }
        catch(...)
        {
          //bad monkey!
        }
      }
    }

    PR_Free(strGUID);
    PR_Free(strQuery);
  }
} //DoPersistentCallback
