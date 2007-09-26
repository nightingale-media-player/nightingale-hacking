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
 * \file DatabaseEngine.h
 * \brief 
 */

#ifndef __DATABASE_ENGINE_H__
#define __DATABASE_ENGINE_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include <sqlite3.h>

#include <deque>
#include <list>
#include <map>

#include "DatabaseQuery.h"
#include "sbIDatabaseEngine.h"

#include <prmon.h>
#include <prlog.h>

#include <nsAutoLock.h>
#include <nsAutoPtr.h>
#include <nsCOMArray.h>
#include <nsRefPtrHashtable.h>
#include <nsIThread.h>
#include <nsThreadUtils.h>
#include <nsIRunnable.h>
#include <nsIObserver.h>
#include <nsProxyRelease.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

// DEFINES ====================================================================
#define SONGBIRD_DATABASEENGINE_CONTRACTID                \
  "@songbirdnest.com/Songbird/DatabaseEngine;1"
#define SONGBIRD_DATABASEENGINE_CLASSNAME                 \
  "Songbird Database Engine Interface"
#define SONGBIRD_DATABASEENGINE_CID                       \
{ /* 67d9edfd-9a76-4d60-9d76-59181801e193 */              \
  0x67d9edfd,                                             \
  0x9a76,                                                 \
  0x4d60,                                                 \
  {0x9d, 0x76, 0x59, 0x18, 0x18, 0x1, 0xe1, 0x93}         \
}
// EXTERNS ====================================================================
extern CDatabaseEngine *gEngine;

// FUNCTIONS ==================================================================
void SQLiteUpdateHook(void *pData, int nOp, const char *pArgA, const char *pArgB, sqlite_int64 nRowID);
int SQLiteAuthorizer(void *pData, int nOp, const char *pArgA, const char *pArgB, const char *pDBName, const char *pTrigger);

// CLASSES ====================================================================
class QueryProcessorThread;

class CDatabaseEngine : public sbIDatabaseEngine,
                        public nsIObserver
{
public:
  friend class QueryProcessorThread;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIDATABASEENGINE

  CDatabaseEngine();
  virtual ~CDatabaseEngine();

  static CDatabaseEngine* GetSingleton();

  typedef enum {
    dbEnginePreShutdown = 0,
    dbEngineShutdown
  } threadpoolmsg_t;

protected:
  NS_IMETHOD Init();
  NS_IMETHOD Shutdown();

  nsresult OpenDB(const nsAString &dbGUID, 
                  CDatabaseQuery *pQuery,
                  sqlite3 ** ppHandle);

  nsresult CloseDB(sqlite3 *pHandle);

  PRBool HasThreadForGUID(const nsAString & aGUID);
  already_AddRefed<QueryProcessorThread> GetThreadByQuery(CDatabaseQuery *pQuery, PRBool bCreate = PR_FALSE);
  already_AddRefed<QueryProcessorThread> CreateThreadFromQuery(CDatabaseQuery *pQuery);

  PRInt32 SubmitQueryPrivate(CDatabaseQuery *pQuery);

  void AddPersistentQueryPrivate(CDatabaseQuery *pQuery, 
                                 const nsACString &strTableName);

  void RemovePersistentQueryPrivate(CDatabaseQuery *pQuery);

  nsresult ClearPersistentQueries();

  static void PR_CALLBACK QueryProcessor(CDatabaseEngine* pEngine,
                                         QueryProcessorThread * pThread);
  
private:
  //[query list]
  typedef std::list<CDatabaseQuery *> querylist_t;
  
  //[table guid/name]
  typedef std::map<nsCString, querylist_t> tablepersistmap_t;

  //[database guid/name]
  typedef std::map<nsCString, tablepersistmap_t > querypersistmap_t;

  void UpdatePersistentQueries(CDatabaseQuery *pQuery);
  void DoSimpleCallback(CDatabaseQuery *pQuery);

  nsresult CreateDBStorePath();
  nsresult GetDBStorePath(const nsAString &dbGUID, CDatabaseQuery *pQuery, nsAString &strPath);

private:
  PRLock * m_pDBStorePathLock;
  nsString m_DBStorePath;

  PRLock *m_pDatabasesGUIDListLock;
  std::vector<nsString> m_DatabasesGUIDList;

  //[database guid / thread]
  nsRefPtrHashtableMT<nsStringHashKey, QueryProcessorThread> m_ThreadPool;

  PRMonitor* m_pPersistentQueriesMonitor;
  querypersistmap_t m_PersistentQueries;

  PRMonitor* m_pThreadMonitor;

  PRBool m_AttemptShutdownOnDestruction;
  PRBool m_IsShutDown;
};

class QueryProcessorThread : public nsIRunnable
{
   friend class CDatabaseEngine;

public:
  //[thread queue]
  typedef nsTArray<CDatabaseQuery *> threadqueue_t;

public:
  NS_DECL_ISUPPORTS

  QueryProcessorThread()
  : m_Shutdown(PR_FALSE)
  , m_IdleTime(0)
  , m_pQueueLock(nsnull)
  , m_pHandleLock(nsnull)
  , m_pHandle(nsnull)
  , m_pEngine(nsnull) {
    MOZ_COUNT_CTOR(QueryProcessorThread);
  }

  ~QueryProcessorThread() {
    if(m_pQueueLock) {
      PR_DestroyLock(m_pQueueLock);
    }

    if(m_pHandleLock) {
      PR_DestroyLock(m_pHandleLock);
    }

    if(m_pQueueMonitor) {
      nsAutoMonitor::DestroyMonitor(m_pQueueMonitor);
    }
    MOZ_COUNT_DTOR(QueryProcessorThread);
  }

  nsresult Init(CDatabaseEngine *pEngine,
                const nsAString &aGUID,
                sqlite3 *pHandle) {
    NS_ENSURE_ARG_POINTER(pEngine);
    NS_ENSURE_ARG_POINTER(pHandle);

    m_pQueueLock = PR_NewLock();
    NS_ENSURE_TRUE(m_pQueueLock, NS_ERROR_OUT_OF_MEMORY);

    m_pHandleLock = PR_NewLock();
    NS_ENSURE_TRUE(m_pHandleLock, NS_ERROR_OUT_OF_MEMORY);

    m_pQueueMonitor = nsAutoMonitor::NewMonitor("QueryProcessorThread.m_pQueueMonitor");
    NS_ENSURE_TRUE(m_pQueueMonitor, NS_ERROR_OUT_OF_MEMORY);

    m_pEngine = pEngine;
    m_pHandle = pHandle;

    m_GUID = aGUID;

    nsCOMPtr<nsIThread> pThread;
    
    nsresult rv = NS_NewThread(getter_AddRefs(pThread), this);
    NS_ENSURE_SUCCESS(rv, rv);

    // Saves an AddRef.
    pThread.swap(m_pThread);

    return NS_OK;
  }

  NS_IMETHOD Run()
  {
    NS_ENSURE_TRUE(m_pEngine, NS_ERROR_NOT_INITIALIZED);

    CDatabaseEngine::QueryProcessor(m_pEngine, 
                                    this);

    nsresult rv = ClearQueue();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = m_pEngine->CloseDB(m_pHandle);
    NS_ENSURE_SUCCESS(rv, rv);

    m_pHandle = nsnull;

    return NS_OK;
  }

  nsresult LockQueue() {
    NS_ENSURE_TRUE(m_pQueueLock, NS_ERROR_NOT_INITIALIZED);
    PR_Lock(m_pQueueLock);
    return NS_OK;
  }

  nsresult UnlockQueue() {
    NS_ENSURE_TRUE(m_pQueueLock, NS_ERROR_NOT_INITIALIZED);
    PR_Unlock(m_pQueueLock);
    return NS_OK;
  }

  nsresult PushQueryToQueue(CDatabaseQuery *pQuery, PRBool bPushToFront = PR_FALSE) {
    NS_ENSURE_ARG_POINTER(pQuery);

    nsresult rv = LockQueue();
    NS_ENSURE_SUCCESS(rv, rv);

    CDatabaseQuery **p = nsnull;

    if(bPushToFront) {
      p = m_Queue.InsertElementAt(0, pQuery);
    } else {
      p = m_Queue.AppendElement(pQuery);
    }

    NS_ENSURE_TRUE(p, NS_ERROR_OUT_OF_MEMORY);

    return UnlockQueue();
  }

  nsresult PopQueryFromQueue(CDatabaseQuery ** ppQuery) {
    NS_ENSURE_ARG_POINTER(ppQuery);

    nsresult rv = LockQueue();
    NS_ENSURE_SUCCESS(rv, rv);

    if(m_Queue.Length()) {
      *ppQuery = m_Queue[0];
      m_Queue.RemoveElementAt(0);

      return UnlockQueue();
    }

    rv = UnlockQueue();
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult GetQueueSize(PRUint32 &aSize) {
    aSize = 0;
    
    nsresult rv = LockQueue();
    NS_ENSURE_SUCCESS(rv, rv);

    aSize = m_Queue.Length();

    return UnlockQueue();
  }

  threadqueue_t * GetQueue() {
    return &m_Queue;
  }

  nsresult NotifyQueue() {
    NS_ENSURE_TRUE(m_pQueueMonitor, NS_ERROR_NOT_INITIALIZED);

    nsAutoMonitor mon(m_pQueueMonitor);
    mon.NotifyAll();

    return NS_OK;
  }

  nsresult ClearQueue() {
    nsresult rv = LockQueue();
    NS_ENSURE_SUCCESS(rv, rv);

    threadqueue_t::size_type current = 0;
    threadqueue_t::size_type length = m_Queue.Length();
    
    for(; current < length; current++) {

      CDatabaseQuery *pQuery = m_Queue[current];
      nsresult rv = NS_ProxyRelease(pQuery->mLocationURIOwningThread, pQuery);

      if (NS_FAILED(rv)) {
        NS_WARNING("Could not proxy release pQuery");
        NS_RELEASE(pQuery);
      }
    }

    m_Queue.Clear();

    return UnlockQueue();
  }

  nsresult PrepareForShutdown() {
    NS_ENSURE_TRUE(m_pEngine, NS_ERROR_NOT_INITIALIZED);

    m_Shutdown = PR_TRUE;

    nsAutoMonitor mon(m_pQueueMonitor);
    return mon.NotifyAll();
  }

  nsIThread * GetThread() {
    return m_pThread;
  }

protected:
  CDatabaseEngine* m_pEngine;
  nsCOMPtr<nsIThread> m_pThread;

  nsString m_GUID;

  PRBool   m_Shutdown;
  PRInt64  m_IdleTime;

  PRLock *      m_pHandleLock;
  sqlite3*      m_pHandle;

  PRMonitor *   m_pQueueMonitor;

  PRLock *      m_pQueueLock;
  threadqueue_t m_Queue;
};

#ifdef PR_LOGGING
class sbDatabaseEnginePerformanceLogger
{
public:
  sbDatabaseEnginePerformanceLogger(const nsAString& aQuery,
                                    const nsAString& aGuid);
  ~sbDatabaseEnginePerformanceLogger();
private:
  nsString mQuery;
  nsString mGuid;
  PRTime mStart;
};
#endif

#endif // __DATABASE_ENGINE_H__

