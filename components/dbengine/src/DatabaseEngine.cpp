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

/**
 * \file DatabaseEngine.cpp
 * \brief
 */

// INCLUDES ===================================================================
#include "DatabaseEngine.h"

#include <nsCOMPtr.h>
#include <nsIFile.h>
#include <nsILocalFile.h>
#include <nsStringGlue.h>
#include <nsIObserverService.h>
#include <nsISimpleEnumerator.h>
#include <nsDirectoryServiceDefs.h>
#include <nsAppDirectoryServiceDefs.h>
#include <nsDirectoryServiceUtils.h>
#include <nsUnicharUtils.h>
#include <nsIURI.h>
#include <nsNetUtil.h>
#include <sbLockUtils.h>

#include <vector>
#include <algorithm>
#include <prmem.h>
#include <prtypes.h>

#include <nsCOMArray.h>

// The maximum characters to output in a single PR_LOG call
#define MAX_PRLOG 500

#if defined(_WIN32)
  #include <direct.h>
#else
// on UNIX, strnicmp is called strncasecmp
#define strnicmp strncasecmp
#endif

//Sometimes min is not defined.
#if !defined(min)
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

#define USE_SQLITE_FULL_DISK_CACHING
#define USE_SQLITE_LARGE_CACHE_SIZE
#define USE_SQLITE_READ_UNCOMMITTED
#define USE_SQLITE_MEMORY_TEMP_STORE
#define USE_SQLITE_BUSY_TIMEOUT
#define USE_SQLITE_SHARED_CACHE

#define SQLITE_MAX_RETRIES            666
#define QUERY_PROCESSOR_THREAD_COUNT  2

#if defined(_DEBUG) || defined(DEBUG)
  #if defined(XP_WIN)
    #include <windows.h>
  #endif
  #define HARD_SANITY_CHECK             1
#endif

#ifdef PR_LOGGING
static PRLogModuleInfo* sDatabaseEngineLog = nsnull;
static PRLogModuleInfo* sDatabaseEnginePerformanceLog = nsnull;
#define TRACE(args) PR_LOG(sDatabaseEngineLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(sDatabaseEngineLog, PR_LOG_WARN, args)

#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

#ifdef PR_LOGGING
#define BEGIN_PERFORMANCE_LOG(_strQuery, _dbName) \
 sbDatabaseEnginePerformanceLogger _performanceLogger(_strQuery, _dbName)
#else
#define BEGIN_PERFORMANCE_LOG(_strQuery, _dbName) /* nothing */
#endif

void SQLiteUpdateHook(void *pData, int nOp, const char *pArgA, const char *pArgB, sqlite_int64 nRowID)
{
  CDatabaseQuery *pQuery = reinterpret_cast<CDatabaseQuery *>(pData);

  switch(nOp)
  {
    case SQLITE_INSERT:
    {
      sbSimpleAutoLock lock(pQuery->m_pInsertedRowIDsLock);
      pQuery->m_InsertedRowIDs.push_back(nRowID);
    }
    break;

    case SQLITE_UPDATE:
    {
      sbSimpleAutoLock lock(pQuery->m_pUpdatedRowIDsLock);
      pQuery->m_UpdatedRowIDs.push_back(nRowID);
    }
    break;

    case SQLITE_DELETE:
    {
      sbSimpleAutoLock lock(pQuery->m_pDeletedRowIDsLock);
      pQuery->m_DeletedRowIDs.push_back(nRowID);
    }
    break;
  }

  return;
}

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
        if(pArgA && pDBName && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            sbSimpleAutoLock lock(pQuery->m_pModifiedDataLock);
            nsCAutoString strDBName(pDBName);

            if(strDBName.EqualsLiteral("main"))
            {
              nsAutoString strDBGUID;
              pQuery->GetDatabaseGUID(strDBGUID);
              strDBName = NS_ConvertUTF16toUTF8(strDBGUID);
            }

            CDatabaseQuery::modifieddata_t::iterator itDB = pQuery->m_ModifiedData.find(strDBName);
            if(itDB != pQuery->m_ModifiedData.end()) {
              itDB->second.insert(nsCAutoString(pArgA));
            } else {
              CDatabaseQuery::modifiedtables_t modifiedTables;
              modifiedTables.insert(nsCAutoString(pArgA));
              pQuery->m_ModifiedData.insert(
                std::make_pair<nsCString, CDatabaseQuery::modifiedtables_t>(
                strDBName, modifiedTables));
            }
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
            sbSimpleAutoLock lock(pQuery->m_pModifiedDataLock);
            nsCAutoString strDBName(pDBName);

            if(strDBName.EqualsLiteral("main"))
            {
              nsAutoString strDBGUID;
              pQuery->GetDatabaseGUID(strDBGUID);
              strDBName = NS_ConvertUTF16toUTF8(strDBGUID);
            }

            CDatabaseQuery::modifieddata_t::iterator itDB = pQuery->m_ModifiedData.find(strDBName);
            if(itDB != pQuery->m_ModifiedData.end()) {
              itDB->second.insert(nsCAutoString(pArgA));
            } else {
              CDatabaseQuery::modifiedtables_t modifiedTables;
              modifiedTables.insert(nsCAutoString(pArgA));
              pQuery->m_ModifiedData.insert(
                std::make_pair<nsCString, CDatabaseQuery::modifiedtables_t>(
                strDBName, modifiedTables));
            }
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
            sbSimpleAutoLock lock(pQuery->m_pModifiedDataLock);
            nsCAutoString strDBName(pDBName);

            CDatabaseQuery::modifieddata_t::iterator itDB = pQuery->m_ModifiedData.find(strDBName);
            if(itDB != pQuery->m_ModifiedData.end()) {
              itDB->second.insert(nsCAutoString(pArgA));
            } else {
              CDatabaseQuery::modifiedtables_t modifiedTables;
              modifiedTables.insert(nsCAutoString(pArgA));
              pQuery->m_ModifiedData.insert(
                std::make_pair<nsCString, CDatabaseQuery::modifiedtables_t>(
                strDBName, modifiedTables));
            }
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
            sbSimpleAutoLock lock(pQuery->m_pModifiedDataLock);
            nsCAutoString strDBName(pDBName);

            if(strDBName.EqualsLiteral("main"))
            {
              nsAutoString strDBGUID;
              pQuery->GetDatabaseGUID(strDBGUID);
              strDBName = NS_ConvertUTF16toUTF8(strDBGUID);
            }

            CDatabaseQuery::modifieddata_t::iterator itDB = pQuery->m_ModifiedData.find(strDBName);
            if(itDB != pQuery->m_ModifiedData.end()) {
              itDB->second.insert(nsCAutoString(pArgA));
            } else {
              CDatabaseQuery::modifiedtables_t modifiedTables;
              modifiedTables.insert(nsCAutoString(pArgA));
              pQuery->m_ModifiedData.insert(
                std::make_pair<nsCString, CDatabaseQuery::modifiedtables_t>(
                strDBName, modifiedTables));
            }
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
        if(pQuery->m_PersistentQuery && strnicmp( "sqlite_", pArgA, 7 ) )
        {
          nsCOMPtr<sbIDatabaseEngine> p = do_GetService(SONGBIRD_DATABASEENGINE_CONTRACTID);
          if(p) p->AddPersistentQuery( pQuery, nsCAutoString(pArgA) );
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
        if(pArgA && pArgB && pDBName)
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            sbSimpleAutoLock lock(pQuery->m_pModifiedDataLock);
            nsCAutoString strDBName(pDBName);

            if(strDBName.EqualsLiteral("main"))
            {
              nsAutoString strDBGUID;
              pQuery->GetDatabaseGUID(strDBGUID);
              strDBName = NS_ConvertUTF16toUTF8(strDBGUID);
            }

            CDatabaseQuery::modifieddata_t::iterator itDB = pQuery->m_ModifiedData.find(strDBName);
            if(itDB != pQuery->m_ModifiedData.end()) {
              itDB->second.insert(nsCAutoString(pArgA));
            } else {
              CDatabaseQuery::modifiedtables_t modifiedTables;
              modifiedTables.insert(nsCAutoString(pArgA));
              pQuery->m_ModifiedData.insert(
                std::make_pair<nsCString, CDatabaseQuery::modifiedtables_t>(
                strDBName, modifiedTables));
            }
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

/*
 * Parse a path string in the form of "n1.n2.n3..." where n is an integer.
 * Returns the next integer in the list while advancing the pos pointer to
 * the start of the next integer.  If the end of the list is reached, pos
 * is set to null
 */
static int tree_collate_func_next_num(const char* start,
                                      char** pos,
                                      int length,
                                      int eTextRep,
                                      int width)
{
  // If we are at the end of the string, set pos to null
  const char* end = start + length;
  if (*pos == end || *pos == NULL) {
    *pos = NULL;
    return 0;
  }

  int num = 0;
  int sign = 1;

  while (*pos < end) {

    // Extract the ASCII value of the digit from the encoded byte(s)
    char c;
    switch(eTextRep) {
      case SQLITE_UTF16BE:
        c = *(*pos + 1);
        break;
      case SQLITE_UTF16LE:
      case SQLITE_UTF8:
        c = **pos;
        break;
      default:
        return 0;
    }

    // If we see a period, we're done.  Also advance the pointer so the next
    // call starts on the first digit
    if (c == '.') {
      *pos += width;
      break;
    }

    // If we encounter a hyphen, treat this as a negative number.  Otherwise,
    // include the digit in the current number
    if (c == '-') {
      sign = -1;
    }
    else {
      num = (num * 10) + (c - '0');
    }
    *pos += width;
  }

  return num * sign;
}

static int tree_collate_func(void *pCtx,
                             int nA,
                             const void *zA,
                             int nB,
                             const void *zB,
                             int eTextRep)
{
  const char* cA = (const char*) zA;
  const char* cB = (const char*) zB;
  char* pA = (char*) cA;
  char* pB = (char*) cB;

  int width = eTextRep == SQLITE_UTF8 ? 1 : 2;

  /*
   * Compare each number in each string until either the numbers are different
   * or we run out of numbers to compare
   */
  int a = tree_collate_func_next_num(cA, &pA, nA, eTextRep, width);
  int b = tree_collate_func_next_num(cB, &pB, nB, eTextRep, width);

  while (pA != NULL && pB != NULL) {
    if (a != b) {
      return a < b ? -1 : 1;
    }
    a = tree_collate_func_next_num(cA, &pA, nA, eTextRep, width);
    b = tree_collate_func_next_num(cB, &pB, nB, eTextRep, width);
  }

  /*
   * At this point, if the lengths are the same, the strings are the same
   */
  if (nA == nB) {
    return 0;
  }

  /*
   * Otherwise, the longer string is always smaller
   */
  return nA > nB ? -1 : 1;
}

static int tree_collate_func_utf16be(void *pCtx,
                                     int nA,
                                     const void *zA,
                                     int nB,
                                     const void *zB)
{
  return tree_collate_func(pCtx, nA, zA, nB, zB, SQLITE_UTF16BE);
}

static int tree_collate_func_utf16le(void *pCtx,
                                     int nA,
                                     const void *zA,
                                     int nB,
                                     const void *zB)
{
  return tree_collate_func(pCtx, nA, zA, nB, zB, SQLITE_UTF16LE);
}

static int tree_collate_func_utf8(void *pCtx,
                                  int nA,
                                  const void *zA,
                                  int nB,
                                  const void *zB)
{
  return tree_collate_func(pCtx, nA, zA, nB, zB, SQLITE_UTF8);
}

NS_IMPL_THREADSAFE_ISUPPORTS2(CDatabaseEngine, sbIDatabaseEngine, nsIObserver)
NS_IMPL_THREADSAFE_ISUPPORTS1(QueryProcessorThread, nsIRunnable)

CDatabaseEngine *gEngine = nsnull;

// CLASSES ====================================================================
//-----------------------------------------------------------------------------
CDatabaseEngine::CDatabaseEngine()
: m_pDBStorePathLock(nsnull)
, m_pDatabasesGUIDListLock(nsnull)
, m_pPersistentQueriesMonitor(nsnull)
, m_pThreadMonitor(nsnull)
, m_AttemptShutdownOnDestruction(PR_FALSE)
, m_IsShutDown(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!sDatabaseEngineLog)
    sDatabaseEngineLog = PR_NewLogModule("sbDatabaseEngine");
  if (!sDatabaseEnginePerformanceLog)
    sDatabaseEnginePerformanceLog = PR_NewLogModule("sbDatabaseEnginePerformance");
#endif
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CDatabaseEngine::~CDatabaseEngine()
{
  if (m_AttemptShutdownOnDestruction)
    Shutdown();

  if (m_pPersistentQueriesMonitor)
    nsAutoMonitor::DestroyMonitor(m_pPersistentQueriesMonitor);
  if (m_pDBStorePathLock)
    PR_DestroyLock(m_pDBStorePathLock);
  if (m_pThreadMonitor)
    nsAutoMonitor::DestroyMonitor(m_pThreadMonitor);
} //dtor

//-----------------------------------------------------------------------------
CDatabaseEngine* CDatabaseEngine::GetSingleton()
{
  if (gEngine) {
    NS_ADDREF(gEngine);
    return gEngine;
  }

  NS_NEWXPCOM(gEngine, CDatabaseEngine);
  if (!gEngine)
    return nsnull;

  // AddRef once for us (released in nsModule destructor)
  NS_ADDREF(gEngine);

  // Set ourselves up properly
  if (NS_FAILED(gEngine->Init())) {
    NS_ERROR("Failed to Init CDatabaseEngine!");
    NS_RELEASE(gEngine);
    return nsnull;
  }

  // And AddRef once for the caller
  NS_ADDREF(gEngine);
  return gEngine;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::Init()
{
  LOG(("CDatabaseEngine[0x%.8x] - Init() - sqlite version %s",
       this, sqlite3_libversion()));

  PRBool success = m_ThreadPool.Init();
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  m_pPersistentQueriesMonitor =
    nsAutoMonitor::NewMonitor("CDatabaseEngine.m_pPersistentQueriesMonitor");

  NS_ENSURE_TRUE(m_pPersistentQueriesMonitor, NS_ERROR_OUT_OF_MEMORY);

  m_pThreadMonitor =
    nsAutoMonitor::NewMonitor("CDatabaseEngine.m_pThreadMonitor");

  NS_ENSURE_TRUE(m_pThreadMonitor, NS_ERROR_OUT_OF_MEMORY);

  m_pDBStorePathLock = PR_NewLock();
  NS_ENSURE_TRUE(m_pDBStorePathLock, NS_ERROR_OUT_OF_MEMORY);

  m_pDatabasesGUIDListLock = PR_NewLock();
  NS_ENSURE_TRUE(m_pDatabasesGUIDListLock, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = CreateDBStorePath();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to create db store folder in profile!");

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  if(NS_SUCCEEDED(rv)) {
    rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
                                      PR_FALSE);
  }

  // This shouldn't be an 'else' case because we want to set this flag if
  // either of the above calls failed
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to register xpcom-shutdown observer");
    m_AttemptShutdownOnDestruction = PR_TRUE;
  }

  return NS_OK;
}


//-----------------------------------------------------------------------------
PR_STATIC_CALLBACK(PLDHashOperator)
EnumThreadsOperate(nsStringHashKey::KeyType aKey, QueryProcessorThread *aThread, void *aClosure)
{
  NS_ASSERTION(aThread, "aThread is null");
  NS_ASSERTION(aClosure, "aClosure is null");

  // Stop if thread is null.
  NS_ENSURE_TRUE(aThread, PL_DHASH_STOP);

  // Stop if closure is null because it
  // contains the operation we are going to perform.
  NS_ENSURE_TRUE(aClosure, PL_DHASH_STOP);

  nsresult rv;
  PRUint32 *op = static_cast<PRUint32 *>(aClosure);

  switch(*op) {
    case CDatabaseEngine::dbEnginePreShutdown:
      rv = aThread->PrepareForShutdown();
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to prepare worker thread for shutdown.");
    break;

    case CDatabaseEngine::dbEngineShutdown:
      rv = aThread->GetThread()->Shutdown();
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to shutdown worker thread.");
    break;

    default:
      ;
  }

  return PL_DHASH_NEXT;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::Shutdown()
{
  m_IsShutDown = PR_TRUE;

  PRUint32 op = dbEnginePreShutdown;
  m_ThreadPool.EnumerateRead(EnumThreadsOperate, &op);

  op = dbEngineShutdown;
  m_ThreadPool.EnumerateRead(EnumThreadsOperate, &op);

  NS_WARN_IF_FALSE(NS_SUCCEEDED(ClearPersistentQueries()),
                   "ClearPersistentQueries Failed!");

  m_ThreadPool.Clear();

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseEngine::Observe(nsISupports *aSubject,
                                       const char *aTopic,
                                       const PRUnichar *aData)
{
  // Bail if we don't care about the message
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
    return NS_OK;

  // Shutdown our threads
  nsresult rv = Shutdown();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Shutdown Failed!");

  // And remove ourselves from the observer service
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
}

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::OpenDB(const nsAString &dbGUID,
                                 CDatabaseQuery *pQuery,
                                 sqlite3 ** ppHandle)
{
  sqlite3 *pHandle = nsnull;

  nsAutoString strFilename;
  GetDBStorePath(dbGUID, pQuery, strFilename);

  PRInt32 ret = sqlite3_open16(PromiseFlatString(strFilename).get(), &pHandle);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to open database: sqlite_open16 failed!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  ret  = sqlite3_create_collation(pHandle,
                                  "tree",
                                  SQLITE_UTF16BE,
                                  NULL,
                                  tree_collate_func_utf16be);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set tree collate function: utf16-be!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  ret = sqlite3_create_collation(pHandle,
                                 "tree",
                                 SQLITE_UTF16LE,
                                 NULL,
                                 tree_collate_func_utf16le);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set tree collate function: utf16-le!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  ret = sqlite3_create_collation(pHandle,
                                 "tree",
                                 SQLITE_UTF8,
                                 NULL,
                                 tree_collate_func_utf8);
  NS_ASSERTION(ret == SQLITE_OK, "Failed to set tree collate function: utf8!");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

#if defined(USE_SQLITE_FULL_DISK_CACHING)
  {
    char *strErr = nsnull;
    sqlite3_exec(pHandle, "PRAGMA synchronous = 0", nsnull, nsnull, &strErr);
    if(strErr) {
      sqlite3_free(strErr);
    }
  }
#endif

#if defined(USE_SQLITE_LARGE_CACHE_SIZE)
  {
    char *strErr = nsnull;
    sqlite3_exec(pHandle, "PRAGMA cache_size = 6000", nsnull, nsnull, &strErr);
    if(strErr) {
      sqlite3_free(strErr);
    }
  }
#endif

#if defined(USE_SQLITE_READ_UNCOMMITTED)
  {
    char *strErr = nsnull;
    sqlite3_exec(pHandle, "PRAGMA read_uncommitted = 1", nsnull, nsnull, &strErr);
    if(strErr) {
      sqlite3_free(strErr);
    }
  }
#endif

#if defined(USE_SQLITE_MEMORY_TEMP_STORE)
  {
    char *strErr = nsnull;
    sqlite3_exec(pHandle, "PRAGMA temp_store = 2", nsnull, nsnull, &strErr);
    if(strErr) {
      sqlite3_free(strErr);
    }
  }
#endif

#if defined(USE_SQLITE_BUSY_TIMEOUT)
  sqlite3_busy_timeout(pHandle, 60000);
#endif

#if defined(USE_SQLITE_SHARED_CACHE)
  sqlite3_enable_shared_cache(1);
#endif

  *ppHandle = pHandle;

  return NS_OK;
} //OpenDB

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::CloseDB(sqlite3 *pHandle)
{
  sqlite3_interrupt(pHandle);

  PRInt32 ret = sqlite3_close(pHandle);
  NS_ASSERTION(ret == SQLITE_OK, "");
  NS_ENSURE_TRUE(ret == SQLITE_OK, NS_ERROR_UNEXPECTED);

  return NS_OK;
} //CloseDB

//-----------------------------------------------------------------------------
/* [noscript] PRInt32 SubmitQuery (in CDatabaseQueryPtr dbQuery); */
NS_IMETHODIMP CDatabaseEngine::SubmitQuery(CDatabaseQuery * dbQuery, PRInt32 *_retval)
{
  if (m_IsShutDown) {
    NS_WARNING("Don't submit queries after the DBEngine is shut down!");
    return NS_ERROR_FAILURE;
  }

  *_retval = SubmitQueryPrivate(dbQuery);
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::SubmitQueryPrivate(CDatabaseQuery *pQuery)
{
  //Query is null, bail.
  if(!pQuery) {
    return 1;
  }

  //Grip.
  NS_ADDREF(pQuery);

  // If the query is already executing, do not add it.  This is to prevent
  // the same query from getting executed simultaneously
  PRBool isExecuting = PR_FALSE;
  pQuery->IsExecuting(&isExecuting);
  if(isExecuting) {
    //Release grip.
    NS_RELEASE(pQuery);
    return 0;
  }

  nsRefPtr<QueryProcessorThread> pThread = GetThreadByQuery(pQuery, PR_TRUE);
  NS_ENSURE_TRUE(pThread, 1);

  nsresult rv = pThread->PushQueryToQueue(pQuery);
  NS_ENSURE_SUCCESS(rv, 1);

  pQuery->m_IsExecuting = PR_TRUE;
  pThread->NotifyQueue();

  PRBool bAsyncQuery = PR_FALSE;
  pQuery->IsAyncQuery(&bAsyncQuery);

  PRInt32 result = 0;
  if(!bAsyncQuery) {
    pQuery->WaitForCompletion(&result);
    pQuery->GetLastError(&result);
  }

  return result;
} //SubmitQueryPrivate

//-----------------------------------------------------------------------------
PRBool CDatabaseEngine::HasThreadForGUID(const nsAString & aGUID)
{
  return m_ThreadPool.Get(nsAutoString(aGUID), nsnull);
}

//-----------------------------------------------------------------------------
already_AddRefed<QueryProcessorThread> CDatabaseEngine::GetThreadByQuery(CDatabaseQuery *pQuery,
                                                         PRBool bCreate /*= PR_FALSE*/)
{
  NS_ENSURE_TRUE(pQuery, nsnull);

  nsAutoString strGUID;
  nsAutoMonitor mon(m_pThreadMonitor);

  nsRefPtr<QueryProcessorThread> pThread;

  nsresult rv = pQuery->GetDatabaseGUID(strGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  if(!m_ThreadPool.Get(strGUID, getter_AddRefs(pThread))) {
    pThread = CreateThreadFromQuery(pQuery);
  }

  QueryProcessorThread *p = pThread.get();
  NS_ADDREF(p);

  return p;
}

//-----------------------------------------------------------------------------
already_AddRefed<QueryProcessorThread> CDatabaseEngine::CreateThreadFromQuery(CDatabaseQuery *pQuery)
{
  nsAutoString strGUID;
  nsAutoMonitor mon(m_pThreadMonitor);

  nsresult rv = pQuery->GetDatabaseGUID(strGUID);
  NS_ENSURE_SUCCESS(rv, nsnull);

  nsRefPtr<QueryProcessorThread> pThread(new QueryProcessorThread());
  NS_ENSURE_TRUE(pThread, nsnull);

  sqlite3 *pHandle = nsnull;
  rv = OpenDB(strGUID, pQuery, &pHandle);
  NS_ENSURE_SUCCESS(rv, nsnull);

  rv = pThread->Init(this, strGUID, pHandle);
  NS_ENSURE_SUCCESS(rv, nsnull);

  PRBool success = m_ThreadPool.Put(strGUID, pThread);
  NS_ENSURE_TRUE(success, nsnull);

  QueryProcessorThread *p = pThread.get();
  NS_ADDREF(p);

  return p;
}

//-----------------------------------------------------------------------------
/* [noscript] void AddPersistentQuery (in CDatabaseQueryPtr dbQuery, in stlCStringRef strTableName); */
NS_IMETHODIMP CDatabaseEngine::AddPersistentQuery(CDatabaseQuery * dbQuery, const nsACString & strTableName)
{
  AddPersistentQueryPrivate(dbQuery, strTableName);
  return NS_OK;
}

//-----------------------------------------------------------------------------
void CDatabaseEngine::AddPersistentQueryPrivate(CDatabaseQuery *pQuery, const nsACString &strTableName)
{
  nsAutoMonitor mon(m_pPersistentQueriesMonitor);

  nsAutoString strDBGUID;
  pQuery->GetDatabaseGUID(strDBGUID);

  NS_ConvertUTF16toUTF8 strTheDBGUID(strDBGUID);

  NS_ASSERTION(strTheDBGUID.IsEmpty() != PR_TRUE, "No DB GUID present in Query that requested persistent execution!");

  querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.find(strTheDBGUID);
  if(itPersistentQueries != m_PersistentQueries.end())
  {
    tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(PromiseFlatCString(strTableName));
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

      itPersistentQueries->second.insert(std::make_pair<nsCString, querylist_t>(PromiseFlatCString(strTableName), queryList));
    }
  }
  else
  {
    NS_IF_ADDREF(pQuery);

    querylist_t queryList;
    queryList.push_back(pQuery);

    tablepersistmap_t tableMap;
    tableMap.insert(std::make_pair<nsCString, querylist_t>(PromiseFlatCString(strTableName), queryList));

    m_PersistentQueries.insert(std::make_pair<nsCString, tablepersistmap_t>(strTheDBGUID, tableMap));
  }

  {
    PR_Lock(pQuery->m_pPersistentQueryTableLock);
    pQuery->m_PersistentQueryTable = strTableName;
    pQuery->m_IsPersistentQueryRegistered = PR_TRUE;
    PR_Unlock(pQuery->m_pPersistentQueryTableLock);
  }

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

  pQuery->m_IsPersistentQueryRegistered = PR_FALSE;

  nsAutoMonitor mon(m_pPersistentQueriesMonitor);

  nsCAutoString tableName;
  nsAutoString strDBGUID;

  pQuery->GetDatabaseGUID(strDBGUID);
  NS_ConvertUTF16toUTF8 strTheDBGUID(strDBGUID);

  {
    PR_Lock(pQuery->m_pPersistentQueryTableLock);
    tableName = pQuery->m_PersistentQueryTable;
    PR_Unlock(pQuery->m_pPersistentQueryTableLock);
  }

  querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.find(strTheDBGUID);
  if(itPersistentQueries != m_PersistentQueries.end())
  {
    tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(tableName);
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

  return;
} //RemovePersistentQuery

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::ClearPersistentQueries()
{
  nsAutoMonitor mon(m_pPersistentQueriesMonitor);

  querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.begin();
  for(; itPersistentQueries != m_PersistentQueries.end(); itPersistentQueries++)
  {
    tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.begin();
    for(; itTableQuery != itPersistentQueries->second.end(); itTableQuery++)
    {
      querylist_t::iterator itQueries = itTableQuery->second.begin();
      for( ; itQueries != itTableQuery->second.end(); itQueries++)
      {
        (*itQueries)->m_IsPersistentQueryRegistered = PR_FALSE;
        NS_IF_RELEASE((*itQueries));
      }
    }
  }

  m_PersistentQueries.clear();

  return NS_OK;
}

//-----------------------------------------------------------------------------
/*static*/ void PR_CALLBACK CDatabaseEngine::QueryProcessor(CDatabaseEngine* pEngine,
                                                            QueryProcessorThread *pThread)
{
  NS_NAMED_LITERAL_STRING(strSelectToken, "SELECT");

  if(!pEngine ||
     !pThread ) {
    return;
  }

  CDatabaseQuery *pQuery = nsnull;

  while(PR_TRUE)
  {
    pQuery = nsnull;

    { // Enter Monitor
      PRUint32 queueSize = 0;

      nsresult rv = pThread->GetQueueSize(queueSize);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get queue size.");

      nsAutoMonitor mon(pThread->m_pQueueMonitor);

      while (!queueSize &&
             !pThread->m_Shutdown) {

        mon.Wait();

        rv = pThread->GetQueueSize(queueSize);
        NS_ASSERTION(NS_SUCCEEDED(rv), "Couldn't get queue size.");
      }

      // Handle shutdown request
      if (pThread->m_Shutdown) {

#if defined(XP_WIN)
        // Cleanup all thread resources.
        sqlite3_thread_cleanup();
#endif

        return;
      }

      // We must have an item in the queue
      rv = pThread->PopQueryFromQueue(&pQuery);
      NS_ASSERTION(NS_SUCCEEDED(rv),
        "Failed to pop query from queue. Thread will restart.");

      // Restart the thread.
      if(NS_FAILED(rv)) {

#if defined(XP_WIN)
        // Cleanup all thread resources.
        sqlite3_thread_cleanup();
#endif

        return;
      }

    } // Exit Monitor


    PRBool bPersistent = pQuery->m_PersistentQuery;
    sqlite3 *pDB = pThread->m_pHandle;

    LOG(("DBE: Process Start, thread 0x%x query 0x%x",
      PR_GetCurrentThread(), pQuery));

    PRUint32 nQueryCount = 0;
    PRBool bFirstRow = PR_TRUE;

    //Default return error.
    pQuery->SetLastError(SQLITE_ERROR);
    pQuery->GetQueryCount(&nQueryCount);

    for(PRUint32 currentQuery = 0; currentQuery < nQueryCount && !pQuery->m_IsAborting; currentQuery++)
    {
      int retDB = 0;
      sqlite3_stmt *pStmt = nsnull;

      nsAutoString strQuery;
      bindParameterArray_t* pParameters;
      const void *pzTail = nsnull;

      pQuery->GetQuery(currentQuery, strQuery);
      pQuery->m_CurrentQuery = currentQuery;

      pParameters = pQuery->GetQueryParameters(currentQuery);

      if(bPersistent)
      {
        //To enable per-row change notification persistent query callbacks we
        //must always select the rowid column and store it to compare with the
        //rowid's that do get changed. We are notified of rowid's that are modified
        //using the sqlite3_update_hook function that we call to register a callback
        //that enables tracking of all data update in the database.
        PRInt32 selectOffset = strQuery.Find(strSelectToken, 0, CaseInsensitiveCompare);
        if(selectOffset > -1)
        {
          nsAutoString str;
          str.AssignLiteral("rowid, ");

          //Make sure we get the row id column first.
          strQuery.Insert(str, selectOffset + strSelectToken.Length() + 1);
        }
      }

      nsAutoString dbName;
      pQuery->GetDatabaseGUID(dbName);

      retDB = sqlite3_set_authorizer(pDB, SQLiteAuthorizer, pQuery);
      if(retDB != SQLITE_OK) {
#if defined(HARD_SANITY_CHECK)
        const char *szErr = sqlite3_errmsg(pDB);
        nsCAutoString log;
        log.Append(szErr);
        log.AppendLiteral("\n");
        NS_WARNING(log.get());
#endif
        continue;
      }

      sqlite3_update_hook(pDB, SQLiteUpdateHook, pQuery);

      retDB = sqlite3_prepare16(pDB, PromiseFlatString(strQuery).get(), (int)strQuery.Length() * sizeof(PRUnichar), &pStmt, &pzTail);
      if(retDB != SQLITE_OK) {
#if defined(HARD_SANITY_CHECK)
        const char *szErr = sqlite3_errmsg(pDB);
        nsCAutoString log;
        log.Append(szErr);
        log.AppendLiteral("\n");
        NS_WARNING(log.get());
#endif
        continue;
      }

      BEGIN_PERFORMANCE_LOG(strQuery, dbName);

      LOG(("DBE: '%s' on '%s'\n",
        NS_ConvertUTF16toUTF8(dbName).get(),
        NS_ConvertUTF16toUTF8(strQuery).get()));

      // If we have parameters for this query, bind them
      PRUint32 len = pParameters->Length();
      for(PRUint32 i = 0; i < len; i++) {
        CQueryParameter& p = pParameters->ElementAt(i);

        switch(p.type) {
          case ISNULL:
            sqlite3_bind_null(pStmt, i + 1);
            LOG(("DBE: Parameter %d is 'NULL'", i));
            break;
          case UTF8STRING:
            sqlite3_bind_text(pStmt, i + 1,
              p.utf8StringValue.get(),
              p.utf8StringValue.Length(),
              SQLITE_TRANSIENT);
            LOG(("DBE: Parameter %d is '%s'", i, p.utf8StringValue.get()));
            break;
          case STRING:
            sqlite3_bind_text16(pStmt, i + 1,
              p.stringValue.get(),
              p.stringValue.Length() * 2,
              SQLITE_TRANSIENT);
            LOG(("DBE: Parameter %d is '%s'", i, NS_ConvertUTF16toUTF8(p.stringValue).get()));
            break;
          case DOUBLE:
            sqlite3_bind_double(pStmt, i + 1, p.doubleValue);
            LOG(("DBE: Parameter %d is '%f'", i, p.doubleValue));
            break;
          case INTEGER32:
            sqlite3_bind_int(pStmt, i + 1, p.int32Value);
            LOG(("DBE: Parameter %d is '%d'", i, p.int32Value));
            break;
          case INTEGER64:
            sqlite3_bind_int64(pStmt, i + 1, p.int64Value);
            LOG(("DBE: Parameter %d is '%ld'", i, p.int64Value));
            break;
        }
      }

      PRInt32 nRetryCount = 0;
      PRInt32 totalRows = 0;

      PRUint64 rollingSum = 0;
      PRUint64 rollingLimit = 0;
      PRUint32 rollingLimitColumnIndex = 0;
      PRUint32 rollingRowCount = 0;
      pQuery->GetRollingLimit(&rollingLimit);
      pQuery->GetRollingLimitColumnIndex(&rollingLimitColumnIndex);

      PRBool finishEarly = PR_FALSE;
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

              std::vector<nsString> vColumnNames;
              vColumnNames.reserve(nCount);

              int j = 0;
              if(bPersistent)
              {
                PR_Lock(pQuery->m_pSelectedRowIDsLock);
                pQuery->m_SelectedRowIDs.clear();
                PR_Unlock(pQuery->m_pSelectedRowIDsLock);

                j = 1;
              }

              for(; j < nCount; j++)
              {
                PRUnichar *p = (PRUnichar *)sqlite3_column_name16(pStmt, j);
                nsString strColumnName;

                if(p) {
                  strColumnName = p;
                }
                else {
                  strColumnName.SetIsVoid(PR_TRUE);
                }

                vColumnNames.push_back(strColumnName);
              }

              pRes->SetColumnNames(vColumnNames);
            }

            std::vector<nsString> vCellValues;
            vCellValues.reserve(nCount);

            TRACE(("DBE: Result row %d:", totalRows));

            int k = 0;
            if(bPersistent)
            {
              PRInt64 nRowID = sqlite3_column_int64(pStmt, 0);
              PR_Lock(pQuery->m_pSelectedRowIDsLock);
              pQuery->m_SelectedRowIDs.push_back(nRowID);
              PR_Unlock(pQuery->m_pSelectedRowIDsLock);

              k = 1;
            }

            // If this is a rolling limit query, increment the rolling
            // sum by the value of the  specified column index.
            if (rollingLimit > 0) {
              rollingSum += sqlite3_column_int64(pStmt, rollingLimitColumnIndex);
              rollingRowCount++;
            }

            // Add the row to the result only if this is not a rolling
            // limit query, or if this is a rolling limit query and the
            // rolling sum has met or exceeded the limit
            if (rollingLimit == 0 || rollingSum >= rollingLimit) {
              for(; k < nCount; k++)
              {
                PRUnichar *p = (PRUnichar *)sqlite3_column_text16(pStmt, k);
                nsString strCellValue;

                if(p) {
                  strCellValue = p;
                }
                else {
                  strCellValue.SetIsVoid(PR_TRUE);
                }

                vCellValues.push_back(strCellValue);
                TRACE(("Column %d: '%s' ", k,
                  NS_ConvertUTF16toUTF8(strCellValue).get()));
              }
              totalRows++;

              pRes->AddRow(vCellValues);

              // If this is a rolling limit query, we're done
              if (rollingLimit > 0) {
                pQuery->SetRollingLimitResult(rollingRowCount);
                finishEarly = PR_TRUE;
              }
            }

            PR_Unlock(pQuery->m_pQueryResultLock);
          }
          break;

        case SQLITE_DONE:
          {
            pQuery->SetLastError(SQLITE_OK);
            TRACE(("Query complete, %d rows", totalRows));
          }
          break;

        case SQLITE_MISUSE:
          {
#if defined(HARD_SANITY_CHECK)
            NS_WARNING("SQLITE: MISUSE\n");
            NS_LossyConvertUTF16toASCII str(PromiseFlatString(strQuery).get());
            nsCAutoString log;

            log.Append(str);
            log.AppendLiteral("\n");
            NS_WARNING(log.get());

            const char *szErr = sqlite3_errmsg(pDB);
            log.Assign(szErr);
            log.AppendLiteral("\n");
            NS_WARNING(log.get());
#endif
          }
          break;

        case SQLITE_BUSY:
          {
#if defined(HARD_SANITY_CHECK)
#if defined(XP_WIN)
            OutputDebugStringA("SQLITE: BUSY\n");
            OutputDebugStringW(PromiseFlatString(strQuery).get());
            OutputDebugStringW(L"\n");

            const char *szErr = sqlite3_errmsg(pDB);
            OutputDebugStringA(szErr);
            OutputDebugStringA("\n");
#endif
#endif
            sqlite3_reset(pStmt);

            if(nRetryCount++ > SQLITE_MAX_RETRIES)
            {
              PR_Sleep(PR_MillisecondsToInterval(250));
              retDB = SQLITE_ROW;
            }
          }
          break;

        default:
          {
            pQuery->SetLastError(retDB);

#if defined(HARD_SANITY_CHECK)
#if defined(XP_WIN)
            OutputDebugStringA("SQLITE: DEFAULT ERROR\n");
            OutputDebugStringA("Error Code: ");
            char szCode[64] = {0};
            OutputDebugStringA(itoa(retDB, szCode, 10));
            OutputDebugStringA("\n");

            OutputDebugStringW(PromiseFlatString(strQuery).get());
            OutputDebugStringW(L"\n");

            const char *szErr = sqlite3_errmsg(pDB);
            OutputDebugStringA(szErr);
            OutputDebugStringA("\n");
#endif

            {
              const char *szErr = sqlite3_errmsg(pDB);
              nsCAutoString log;
              log.Append(szErr);
              log.AppendLiteral("\n");
              NS_WARNING(log.get());
            }
#endif
          }
        }
      }
      while(retDB == SQLITE_ROW &&
            !pQuery->m_IsAborting &&
            !finishEarly);

      //Didn't get any rows
      if(bFirstRow)
      {
        PRBool isPersistent = PR_FALSE;
        pQuery->IsPersistentQuery(&isPersistent);

        if(isPersistent)
        {
          nsAutoString strTableName(strQuery);
          ToLowerCase(strTableName);

          NS_NAMED_LITERAL_STRING(searchStr, " from ");

          PRInt32 foundIndex = strTableName.Find(searchStr);
          if(foundIndex > -1)
          {
            PRUint32 nCutLen = foundIndex + searchStr.Length();
            strTableName.Cut(0, nCutLen);

            PRUint32 offset = 0;
            while(strTableName.CharAt(offset) == NS_L(' '))
              offset++;

            strTableName.Cut(0, offset);

            NS_NAMED_LITERAL_STRING(spaceStr, " ");
            foundIndex = strTableName.Find(spaceStr);

            if(foundIndex > -1)
              strTableName.SetLength((PRUint32)foundIndex);

            pEngine->AddPersistentQuery(pQuery, NS_ConvertUTF16toUTF8(strTableName));
          }

          PR_Lock(pQuery->m_pSelectedRowIDsLock);
          pQuery->m_SelectedRowIDs.clear();
          PR_Unlock(pQuery->m_pSelectedRowIDsLock);
        }

        CDatabaseResult *pRes = pQuery->GetResultObject();
        pRes->ClearResultSet();
      }

      //Always release the statement.
      retDB = sqlite3_finalize(pStmt);

#if defined(HARD_SANITY_CHECK)
      if(retDB != SQLITE_OK)
      {
        const char *szErr = sqlite3_errmsg(pDB);
        nsCAutoString log;
        log.Append(szErr);
        log.AppendLiteral("\n");
        NS_WARNING(log.get());
      }
#endif
      delete pParameters;
    }

    pQuery->m_IsExecuting = PR_FALSE;
    pQuery->m_IsAborting = PR_FALSE;

    //Whatever happened, the query is done running now.
    {
      nsAutoMonitor mon(pQuery->m_pQueryRunningMonitor);
      pQuery->m_QueryHasCompleted = PR_TRUE;
      
      mon.NotifyAll();

      LOG(("DBE: Notified query monitor."));
    }

    //Check if this query is a persistent query so we can now fire off the callback.
    pEngine->DoSimpleCallback(pQuery);
    LOG(("DBE: Simple query listeners have been processed."));

    //Check if this query changed any data
    pEngine->UpdatePersistentQueries(pQuery);
    LOG(("DBE: Persistent queries updated."));

    // Release the query on the same thread that the location URI class was
    // created on.  This prevents an assertion.
    nsresult rv = NS_ProxyRelease(pQuery->mLocationURIOwningThread, pQuery);
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not proxy release pQuery");
      NS_RELEASE(pQuery);
    }

    LOG(("DBE: Process End"));

  } // while

  return;
} //QueryProcessor

//-----------------------------------------------------------------------------
void CDatabaseEngine::UpdatePersistentQueries(CDatabaseQuery *pQuery)
{
  NS_NAMED_LITERAL_STRING(strAllToken, "*");
  NS_NAMED_LITERAL_CSTRING(cstrAllToken, "*");

  //Make sure the query has changed persistent query data.
  //Also make sure that the query is not a persistent on itself.
  //This is to avoid circular processing nightmares.
  if(pQuery->m_HasChangedDataOfPersistQuery &&
     !pQuery->m_PersistentQuery)
  {
    PRBool mustExec = PR_FALSE;

    //Stash the number of deleted, inserted and updated rows.
    PRUint32 deletedRowCount = 0;
    PRUint32 insertedRowCount = 0;
    PRUint32 updatedRowCount = 0;

    PR_Lock(pQuery->m_pInsertedRowIDsLock);
    insertedRowCount = pQuery->m_InsertedRowIDs.size();
    PR_Unlock(pQuery->m_pInsertedRowIDsLock);

    PR_Lock(pQuery->m_pUpdatedRowIDsLock);
    updatedRowCount = pQuery->m_UpdatedRowIDs.size();
    PR_Unlock(pQuery->m_pUpdatedRowIDsLock);

    PR_Lock(pQuery->m_pDeletedRowIDsLock);
    deletedRowCount = pQuery->m_DeletedRowIDs.size();
    PR_Unlock(pQuery->m_pDeletedRowIDsLock);

    sbSimpleAutoLock lock(pQuery->m_pModifiedDataLock);
    CDatabaseQuery::modifieddata_t::const_iterator itDB = pQuery->m_ModifiedData.begin();

    //For each database the query has modified
    for(; itDB != pQuery->m_ModifiedData.end(); itDB++)
    {
      //Is there a persistent query registered for this database guid?
      nsAutoMonitor mon(m_pPersistentQueriesMonitor);
      querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.find(itDB->first);

      if(itPersistentQueries != m_PersistentQueries.end())
      {
        nsCAutoString strTableName;
        CDatabaseQuery::modifiedtables_t::const_iterator i = itDB->second.begin();

        //For each table modified in said database by the query
        for (; i != itDB->second.end(); ++i )
        {
          //Is the persitent query registered for this database also looking
          //for changes in said table?
          tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find((*i));
          if(itTableQuery != itPersistentQueries->second.end())
          {
            //Get all queries that are registered for this database/table pair.
            querylist_t::iterator itQueries = itTableQuery->second.begin();
            for(; itQueries != itTableQuery->second.end(); itQueries++)
            {
              //Does the persistent query wish to use selective persistent updating?
              if((*itQueries)->m_PersistExecSelectiveMode)
              {
                //If any rows were inserted or deleted, we must execute the query
                //because there is no way to determine if a rowid selected was affected
                //or not. Or how it's insertion or removal affects what is already
                //present.
                if( ((*itQueries)->m_PersistExecOnDelete && deletedRowCount) ||
                  ((*itQueries)->m_PersistExecOnInsert && insertedRowCount) )
                {
                  SubmitQueryPrivate((*itQueries));
                }
                //If any rows are updated, create an intersection to find out
                //if the persistent query's result rowids were affected.
                else if((*itQueries)->m_PersistExecOnUpdate && updatedRowCount)
                {
                  CDatabaseQuery::dbrowids_t::iterator itS, itE, itSS, itEE;
                  CDatabaseQuery::dbrowids_t intersect;

                  PR_Lock(pQuery->m_pUpdatedRowIDsLock);
                  PR_Lock((*itQueries)->m_pSelectedRowIDsLock);

                  itS = pQuery->m_UpdatedRowIDs.begin();
                  itSS = (*itQueries)->m_SelectedRowIDs.begin();

                  itE = pQuery->m_UpdatedRowIDs.end();
                  itEE = (*itQueries)->m_SelectedRowIDs.end();

                  intersect.resize(min(pQuery->m_UpdatedRowIDs.size(), (*itQueries)->m_SelectedRowIDs.size()));
                  std::set_intersection(itS, itE, itSS, itEE, intersect.begin());

                  mustExec = intersect.size();

                  PR_Unlock((*itQueries)->m_pSelectedRowIDsLock);
                  PR_Unlock(pQuery->m_pUpdatedRowIDsLock);

                  //persistent query rowids were affected, submit
                  //the query for execution.
                  if(mustExec)
                    SubmitQueryPrivate((*itQueries));
                }
              }
              else
              {
                SubmitQueryPrivate((*itQueries));
              }
            }
          }
        }
      }

      itPersistentQueries = m_PersistentQueries.find(cstrAllToken);
      if(itPersistentQueries != m_PersistentQueries.end())
      {
        CDatabaseQuery::modifiedtables_t::const_iterator i = itDB->second.begin();
        for (; i != itDB->second.end(); ++i )
        {
          tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find((*i));
          if(itTableQuery != itPersistentQueries->second.end())
          {
            querylist_t::iterator itQueries = itTableQuery->second.begin();
            for(; itQueries != itTableQuery->second.end(); itQueries++)
            {
              if((*itQueries)->m_PersistExecSelectiveMode)
              {
                if( ((*itQueries)->m_PersistExecOnDelete && deletedRowCount) ||
                    ((*itQueries)->m_PersistExecOnInsert && insertedRowCount) )
                {
                  (*itQueries)->SetDatabaseGUID(strAllToken);
                  SubmitQueryPrivate((*itQueries));
                }
                else if((*itQueries)->m_PersistExecOnUpdate && updatedRowCount)
                {
                  CDatabaseQuery::dbrowids_t::iterator itS, itE, itSS, itEE;

                  CDatabaseQuery::dbrowids_t intersect;
                  CDatabaseQuery::dbrowids_t::iterator itR = intersect.begin();

                  PR_Lock(pQuery->m_pUpdatedRowIDsLock);
                  PR_Lock((*itQueries)->m_pSelectedRowIDsLock);

                  itS = pQuery->m_UpdatedRowIDs.begin();
                  itSS = (*itQueries)->m_SelectedRowIDs.begin();

                  itE = pQuery->m_UpdatedRowIDs.end();
                  itEE = (*itQueries)->m_SelectedRowIDs.end();

                  std::set_intersection(itS, itE, itSS, itEE, itR);

                  mustExec = intersect.size();

                  PR_Unlock((*itQueries)->m_pSelectedRowIDsLock);
                  PR_Unlock(pQuery->m_pUpdatedRowIDsLock);

                  if(mustExec)
                    SubmitQueryPrivate((*itQueries));
                }
              }
              else
              {
                (*itQueries)->SetDatabaseGUID(strAllToken);
                SubmitQueryPrivate((*itQueries));
              }
            }
          }
        }
      }
    }
  }

  PR_Lock(pQuery->m_pModifiedDataLock);
  pQuery->m_HasChangedDataOfPersistQuery = PR_FALSE;
  pQuery->m_ModifiedData.clear();
  PR_Unlock(pQuery->m_pModifiedDataLock);

  PR_Lock(pQuery->m_pInsertedRowIDsLock);
  pQuery->m_InsertedRowIDs.clear();
  PR_Unlock(pQuery->m_pInsertedRowIDsLock);

  PR_Lock(pQuery->m_pUpdatedRowIDsLock);
  pQuery->m_UpdatedRowIDs.clear();
  PR_Unlock(pQuery->m_pUpdatedRowIDsLock);

  PR_Lock(pQuery->m_pDeletedRowIDsLock);
  pQuery->m_DeletedRowIDs.clear();
  PR_Unlock(pQuery->m_pDeletedRowIDsLock);

} //UpdatePersistentQueries

//-----------------------------------------------------------------------------
PR_STATIC_CALLBACK(PLDHashOperator)
EnumSimpleCallback(nsISupports *key, sbIDatabaseSimpleQueryCallback *data, void *closure)
{
  nsCOMArray<sbIDatabaseSimpleQueryCallback> *array = static_cast<nsCOMArray<sbIDatabaseSimpleQueryCallback> *>(closure);
  array->AppendObject(data);
  return PL_DHASH_NEXT;
}

//-----------------------------------------------------------------------------
void CDatabaseEngine::DoSimpleCallback(CDatabaseQuery *pQuery)
{
  PRUint32 callbackCount = 0;
  nsCOMArray<sbIDatabaseSimpleQueryCallback> callbackSnapshot;

  nsCOMPtr<sbIDatabaseResult> pDBResult;
  nsAutoString strGUID, strQuery;

  pQuery->GetResultObject(getter_AddRefs(pDBResult));
  pQuery->GetDatabaseGUID(strGUID);
  pQuery->GetQuery(0, strQuery);

  PR_Lock(pQuery->m_pPersistentCallbackListLock);
  pQuery->m_PersistentCallbackList.EnumerateRead(EnumSimpleCallback, &callbackSnapshot);
  PR_Unlock(pQuery->m_pPersistentCallbackListLock);

  callbackCount = callbackSnapshot.Count();
  if(!callbackCount)
    return;

  for(PRUint32 i = 0; i < callbackCount; i++)
  {
    nsCOMPtr<sbIDatabaseSimpleQueryCallback> callback = callbackSnapshot.ObjectAt(i);
    if(callback)
    {
      try
      {
        callback->OnQueryEnd(pDBResult, strGUID, strQuery);
      }
      catch(...) { }
    }
  }

  return;
} //DoSimpleCallback

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::CreateDBStorePath()
{
  nsresult rv = NS_ERROR_FAILURE;
  sbSimpleAutoLock lock(m_pDBStorePathLock);

  nsCOMPtr<nsIFile> f;

  rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_50_DIR, getter_AddRefs(f));
  if(NS_FAILED(rv)) return rv;

  rv = f->Append(NS_LITERAL_STRING("db"));
  if(NS_FAILED(rv)) return rv;

  PRBool dirExists = PR_FALSE;
  rv = f->Exists(&dirExists);
  if(NS_FAILED(rv)) return rv;

  if(!dirExists)
  {
    rv = f->Create(nsIFile::DIRECTORY_TYPE, 0700);
    if(NS_FAILED(rv)) return rv;
  }

  rv = f->GetPath(m_DBStorePath);
  if(NS_FAILED(rv)) return rv;

  return NS_OK;
}

//-----------------------------------------------------------------------------
nsresult CDatabaseEngine::GetDBStorePath(const nsAString &dbGUID, CDatabaseQuery *pQuery, nsAString &strPath)
{
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsILocalFile> f;
  nsCOMPtr<nsIURI> uri;
  nsAutoString strDBFile(dbGUID);

  rv = pQuery->GetDatabaseLocation(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  if(uri)
  {
    nsCAutoString spec;

    rv = uri->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> file;
    rv = NS_GetFileFromURLSpec(spec, getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString path;
    rv = file->GetPath(path);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = NS_NewLocalFile(path, PR_FALSE, getter_AddRefs(f));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else
  {
    PR_Lock(m_pDBStorePathLock);
    rv = NS_NewLocalFile(m_DBStorePath, PR_FALSE, getter_AddRefs(f));
    PR_Unlock(m_pDBStorePathLock);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  strDBFile.AppendLiteral(".db");
  rv = f->Append(strDBFile);
  if(NS_FAILED(rv)) return rv;
  rv = f->GetPath(strPath);
  if(NS_FAILED(rv)) return rv;

  return NS_OK;
} //GetDBStorePath

#ifdef PR_LOGGING
sbDatabaseEnginePerformanceLogger::sbDatabaseEnginePerformanceLogger(const nsAString& aQuery,
                                                                     const nsAString& aGuid) :
  mQuery(aQuery),
  mGuid(aGuid)
{
  mStart = PR_Now();
}

sbDatabaseEnginePerformanceLogger::~sbDatabaseEnginePerformanceLogger()
{
  PRUint32 total = PR_Now() - mStart;

  PRUint32 length = mQuery.Length();
  for (PRUint32 i = 0; i < (length / MAX_PRLOG) + 1; i++) {
    nsAutoString q(Substring(mQuery, MAX_PRLOG * i, MAX_PRLOG));
    if (i == 0) {
      PR_LOG(sDatabaseEnginePerformanceLog, PR_LOG_DEBUG,
             ("sbDatabaseEnginePerformanceLogger %s\t%u\t%s",
              NS_LossyConvertUTF16toASCII(mGuid).get(),
              total,
              NS_LossyConvertUTF16toASCII(q).get()));
    }
    else {
      PR_LOG(sDatabaseEnginePerformanceLog, PR_LOG_DEBUG,
             ("sbDatabaseEnginePerformanceLogger +%s",
              NS_LossyConvertUTF16toASCII(q).get()));
    }
  }
}
#endif
