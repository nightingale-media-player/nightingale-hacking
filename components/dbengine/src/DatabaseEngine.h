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

/**
 * \file DatabaseEngine.h
 * \brief 
 */

#ifndef __DATABASE_ENGINE_H__
#define __DATABASE_ENGINE_H__

// INCLUDES ===================================================================
#include <nscore.h>
#include "sqlite3.h"

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
#include <nsServiceManagerUtils.h>
#include <nsIIdleService.h>
#include <nsIThread.h>
#include <nsIThreadPool.h>
#include <nsThreadUtils.h>
#include <nsIRunnable.h>
#include <nsIObserver.h>
#include <nsStringGlue.h>
#include <nsTArray.h>
#include <nsITimer.h>

#include "sbLeadingNumbers.h"

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

// CLASSES ====================================================================
class QueryProcessorQueue;
class CDatabaseDumpProcessor;

class collationBuffers;

class CDatabaseEngine : public sbIDatabaseEngine,
                        public nsIObserver
{
public:
  friend class QueryProcessorQueue;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIDATABASEENGINE

  CDatabaseEngine();
  virtual ~CDatabaseEngine();

  static CDatabaseEngine* GetSingleton();
  
  PRInt32 Collate(collationBuffers *aCollationBuffers, 
                  const NATIVE_CHAR_TYPE *aStr1, 
                  const NATIVE_CHAR_TYPE *aStr2);

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

  already_AddRefed<QueryProcessorQueue> GetQueueByQuery(CDatabaseQuery *pQuery, PRBool bCreate = PR_FALSE);
  already_AddRefed<QueryProcessorQueue> CreateQueueFromQuery(CDatabaseQuery *pQuery);

  PRInt32 SubmitQueryPrivate(CDatabaseQuery *pQuery);

  static void PR_CALLBACK QueryProcessor(CDatabaseEngine* pEngine,
                                         QueryProcessorQueue * pQueue);

  already_AddRefed<nsIEventTarget> GetEventTarget();
  
private:
  //[query list]
  typedef std::list<CDatabaseQuery *> querylist_t;
  
  void DoSimpleCallback(CDatabaseQuery *pQuery);
  void ReportError(sqlite3*, sqlite3_stmt*);
  
  nsresult InitMemoryConstraints();
  
  nsresult GetDBPrefs(const nsAString &dbGUID,
                      PRInt32 *cacheSize, 
                      PRInt32 *pageSize);
                      
  nsresult CreateDBStorePath();
  nsresult GetDBStorePath(const nsAString &dbGUID, CDatabaseQuery *pQuery, nsAString &strPath);

  nsresult GetCurrentCollationLocale(nsCString &aCollationLocale);
  PRInt32 CollateWithLeadingNumbers(collationBuffers *aCollationBuffers,
                                    const NATIVE_CHAR_TYPE *aStr1, 
                                    PRInt32 *number1Length,
                                    const NATIVE_CHAR_TYPE *aStr2,
                                    PRInt32 *number2Length);
  PRInt32 CollateForCurrentLocale(collationBuffers *aCollationBuffers,
                                  const NATIVE_CHAR_TYPE *aStr1,
                                  const NATIVE_CHAR_TYPE *aStr2);

  nsresult MarkDatabaseForPotentialDeletion(const nsAString &aDatabaseGUID, 
                                            CDatabaseQuery *pQuery);
  nsresult PromptToDeleteDatabases();
  nsresult DeleteMarkedDatabases();

  nsresult RunAnalyze();

private:
  typedef std::map<sqlite3 *, collationBuffers *> collationMap_t;
  collationMap_t m_CollationBuffersMap;

  PRLock * m_pDBStorePathLock;
  nsString m_DBStorePath;

  //[database guid / thread]
  nsRefPtrHashtableMT<nsStringHashKey, QueryProcessorQueue> m_QueuePool;
  // enum function for queue processor hashtable.
  template<class T>
    static NS_HIDDEN_(PLDHashOperator)
      EnumerateIntoArrayStringKey(const nsAString& aKey,
                                  T* aData,
                                  void* aArray);

  PRMonitor* m_pThreadMonitor;
  PRMonitor* m_CollationBuffersMapMonitor;

  PRBool m_AttemptShutdownOnDestruction;
  PRBool m_IsShutDown;

  PRBool m_MemoryConstraintsSet;

  PRBool m_PromptForDelete;
  PRBool m_DeleteDatabases;

  typedef std::map<nsString, nsRefPtr<CDatabaseQuery> > deleteDatabaseMap_t;
  deleteDatabaseMap_t m_DatabasesToDelete;

  nsCOMPtr<nsITimer> m_PromptForDeleteTimer;

  nsCOMPtr<nsIThreadPool> m_pThreadPool;

  // Tracks if we've successfully added an idle observer.
  PRPackedBool m_AddedIdleObserver;

  // Pre-allocated memory for sqlite page cache and scratch.
  // Created in InitMemoryConstraints and destroyed in Shutdown.
  // Used to avoid fragmentation.
  void* m_pPageSpace;
  void* m_pScratchSpace;

  nsCString mCollationLocale;
#ifdef XP_MACOSX
  CollatorRef m_Collator;
#endif
};

class QueryProcessorQueue : public nsIRunnable
{
   friend class CDatabaseEngine;
   friend class CDatabaseDumpProcessor;

public:
  //[thread queue]
  typedef nsTArray<CDatabaseQuery *> queryqueue_t;

public:
  NS_DECL_ISUPPORTS

  QueryProcessorQueue()
  : m_Shutdown(PR_FALSE)
  , m_Running(PR_FALSE)
  , m_pQueueMonitor(nsnull)
  , m_pHandleLock(nsnull)
  , m_pHandle(nsnull)
  , m_pEngine(nsnull)
  , m_AnalyzeCount(0) {
    MOZ_COUNT_CTOR(QueryProcessorQueue);
  }

  ~QueryProcessorQueue() {
    if(m_pHandleLock) {
      PR_DestroyLock(m_pHandleLock);
    }

    if(m_pQueueMonitor) {
      nsAutoMonitor::DestroyMonitor(m_pQueueMonitor);
    }

    MOZ_COUNT_DTOR(QueryProcessorQueue);
  }

  nsresult Init(CDatabaseEngine *pEngine,
                const nsAString &aGUID,
                sqlite3 *pHandle) {
    NS_ENSURE_ARG_POINTER(pEngine);
    NS_ENSURE_ARG_POINTER(pHandle);

    m_pHandleLock = PR_NewLock();
    NS_ENSURE_TRUE(m_pHandleLock, NS_ERROR_OUT_OF_MEMORY);

    m_pQueueMonitor = nsAutoMonitor::NewMonitor("QueryProcessorQueue.m_pQueueMonitor");
    NS_ENSURE_TRUE(m_pQueueMonitor, NS_ERROR_OUT_OF_MEMORY);

    m_pEngine = pEngine;
    m_pHandle = pHandle;

    m_GUID = aGUID;

    m_pEventTarget = m_pEngine->GetEventTarget();
    NS_ENSURE_TRUE(m_pEventTarget, NS_ERROR_UNEXPECTED);

    return NS_OK;
  }

  NS_IMETHOD Run()
  {
    NS_ENSURE_TRUE(m_pEngine, NS_ERROR_NOT_INITIALIZED);

    CDatabaseEngine::QueryProcessor(m_pEngine, 
                                    this);

    return NS_OK;
  }

  nsresult PushQueryToQueue(CDatabaseQuery *pQuery, 
                            PRBool bPushToFront = PR_FALSE) {
    NS_ENSURE_ARG_POINTER(pQuery);

    nsAutoMonitor mon(m_pQueueMonitor);

    CDatabaseQuery **p = nsnull;

    if(bPushToFront) {
      p = m_Queue.InsertElementAt(0, pQuery);
    } else {
      p = m_Queue.AppendElement(pQuery);
    }

    NS_ENSURE_TRUE(p, NS_ERROR_OUT_OF_MEMORY);

    return NS_OK;
  }

  nsresult PopQueryFromQueue(CDatabaseQuery ** ppQuery) {
    NS_ENSURE_ARG_POINTER(ppQuery);

    nsAutoMonitor mon(m_pQueueMonitor);

    if(m_Queue.Length()) {
      *ppQuery = m_Queue[0];
      m_Queue.RemoveElementAt(0);

      return NS_OK;
    }

    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult GetQueueSize(PRUint32 &aSize) {
    aSize = 0;
    
    nsAutoMonitor mon(m_pQueueMonitor);
    aSize = m_Queue.Length();

    return NS_OK;
  }

  nsresult ClearQueue() {
    nsAutoMonitor mon(m_pQueueMonitor);

    queryqueue_t::size_type current = 0;
    queryqueue_t::size_type length = m_Queue.Length();
    
    for(; current < length; current++) {

      CDatabaseQuery *pQuery = m_Queue[current];
      NS_RELEASE(pQuery);
    }

    m_Queue.Clear();

    return NS_OK;
  }

  nsresult RunQueue() {
    nsAutoMonitor mon(m_pQueueMonitor);

    // If the query processor for this queue isn't running right now
    // start running it on the threadpool.
    if(!m_Running) {
      m_Running = PR_TRUE;

      nsresult rv = 
        m_pEventTarget->Dispatch(this, nsIEventTarget::DISPATCH_NORMAL);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
  }

  nsresult PrepareForShutdown() {
    NS_ENSURE_TRUE(m_pEngine, NS_ERROR_NOT_INITIALIZED);
    m_Shutdown = PR_TRUE;
    
    nsAutoMonitor mon(m_pQueueMonitor);
    return mon.NotifyAll();
  }

  nsresult Shutdown() {
    nsresult rv = ClearQueue();
    NS_ENSURE_SUCCESS(rv, rv);

    rv = m_pEngine->CloseDB(m_pHandle);
    NS_ENSURE_SUCCESS(rv, rv);

    m_pHandle = nsnull;
    return NS_OK;
  }

protected:
  CDatabaseEngine* m_pEngine;
  nsCOMPtr<nsIEventTarget> m_pEventTarget;

  nsString m_GUID;

  PRPackedBool  m_Shutdown;
  PRPackedBool  m_Running;

  PRLock *      m_pHandleLock;
  sqlite3*      m_pHandle;

  PRMonitor *   m_pQueueMonitor;
  queryqueue_t  m_Queue;

  PRUint32      m_AnalyzeCount;
};

// These classes are used for time-critical string copy during the collation
// algorithm and replace usage of nsString/nsCString in order to eliminate
// repeated allocations. The idea is just to hold on to a buffer which
// can grow in size but never reduce: as more strings get collated, the buffers
// quickly reach the maximum size needed and the only subsequent operations
// are memcopys. This is fine because the strings that are collated are never
// megabytes or even kilobytes of data, the largest ones will rarely exceed
// a few hundreds of bytes.
class fastString {
public:
  fastString() : 
    mBuffer(nsnull),
    mBufferLen(0) {}
  virtual ~fastString() {
    if (mBuffer)
      free(mBuffer);
  }
  inline void grow_native(PRInt32 aLength) {
    grow(aLength, sizeof(NATIVE_CHAR_TYPE));
  }
  inline void grow_utf16(PRInt32 aLength) {
    grow(aLength, sizeof(UTF16_CHARTYPE));
  }
  inline void copy_native(const NATIVE_CHAR_TYPE *aFrom, PRInt32 aSize) {
    grow_native(aSize);
    copy(aFrom, aSize, sizeof(NATIVE_CHAR_TYPE));
    mBuffer[aSize] = 0;
  }
  inline void copy_utf16(const UTF16_CHARTYPE *aFrom, PRInt32 aSize) {
    grow_utf16(aSize);
    copy(aFrom, aSize, sizeof(UTF16_CHARTYPE));
    ((UTF16_CHARTYPE*)mBuffer)[aSize] = 0;
  }
  inline NATIVE_CHAR_TYPE *buffer(){
    return mBuffer;
  }
  inline PRInt32 bufferLength() {
    return mBufferLen;
  }
private:
  inline void grow(PRInt32 aLength, PRInt32 aCharSize) {
    int s = ((aLength)+1)*aCharSize;
    if (mBufferLen < s) {
      if (mBuffer)
        free(mBuffer);
      mBuffer = (NATIVE_CHAR_TYPE *)malloc(s);
      mBufferLen = s;
    }
  }
  inline void copy(const void *aFrom, PRInt32 aSize, PRInt32 aCharSize) {
    memcpy(mBuffer, aFrom, aSize * aCharSize);
  }
  NATIVE_CHAR_TYPE *mBuffer;
  PRInt32 mBufferLen;
};

class collationBuffers {
public:
  fastString encodingConversionBuffer1;
  fastString encodingConversionBuffer2;
  fastString substringExtractionBuffer1;
  fastString substringExtractionBuffer2;
};

#endif // __DATABASE_ENGINE_H__
