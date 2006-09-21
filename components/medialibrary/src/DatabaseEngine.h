/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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

#include <prlock.h>
#include <prmon.h>
#include <nsCOMArray.h>
#include <nsIThread.h>
#include <nsIRunnable.h>

#include <string/nsString.h>

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
int SQLiteAuthorizer(void *pData, int nOp, const char *pArgA, const char *pArgB, const char *pDBName, const char *pTrigger);

// CLASSES ====================================================================
class QueryProcessorThread;

class CDatabaseEngine : public sbIDatabaseEngine
{
public:
  friend class QueryProcessorThread;

  NS_DECL_ISUPPORTS
  NS_DECL_SBIDATABASEENGINE

  CDatabaseEngine();
  virtual ~CDatabaseEngine();

  static CDatabaseEngine* GetSingleton();

protected:
  PRInt32 OpenDB(const nsAString &dbGUID);
  PRInt32 CloseDB(const nsAString &dbGUID);

  PRInt32 DropDB(const nsAString &dbGUID);

  PRInt32 SubmitQueryPrivate(CDatabaseQuery *dbQuery);

  void AddPersistentQueryPrivate(CDatabaseQuery *pQuery, const nsACString &strTableName);
  void RemovePersistentQueryPrivate(CDatabaseQuery *pQuery);

  nsresult LockDatabase(sqlite3 *pDB);
  nsresult UnlockDatabase(sqlite3 *pDB);

  nsresult ClearAllDBLocks();
  nsresult CloseAllDB();

  nsresult ClearPersistentQueries();
  nsresult ClearQueryQueue();

  sqlite3 *GetDBByGUID(const nsAString &dbGUID, PRBool bCreateIfNotOpen = PR_FALSE);
  sqlite3 *FindDBByGUID(const nsAString &dbGUID);

  void GenerateDBGUIDList();
  PRInt32 GetDBGUIDList(std::vector<nsString> &vGUIDList);

  static void PR_CALLBACK QueryProcessor(CDatabaseEngine* pEngine);
  
private:
  //[database guid/name]
  typedef std::map<nsString, sqlite3 *>  databasemap_t;
  typedef std::map<sqlite3 *, PRMonitor *> databaselockmap_t;
  typedef std::list<CDatabaseQuery *> querylist_t;
  //[table guid/name]
  typedef std::map<nsCString, querylist_t> tablepersistmap_t;
  //[database guid/name]
  typedef std::map<nsCString, tablepersistmap_t > querypersistmap_t;
  typedef std::deque<CDatabaseQuery *> queryqueue_t;

  void UpdatePersistentQueries(CDatabaseQuery *pQuery);
  void DoPersistentCallback(CDatabaseQuery *pQuery);

  nsresult CreateDBStorePath();
  nsresult GetDBStorePath(const nsAString &dbGUID, nsAString &strPath);

private:
  PRLock * m_pDBStorePathLock;
  nsString m_DBStorePath;

  std::vector<nsString> m_DatabasesGUIDList;
  PRLock *m_pDatabasesGUIDListLock;

  databasemap_t m_Databases;
  PRLock* m_pDatabasesLock;

  databaselockmap_t m_DatabaseLocks;
  PRLock* m_pDatabaseLocksLock;
  
  PRMonitor* m_pQueryProcessorMonitor;
  nsCOMArray<nsIThread> m_QueryProcessorThreads;
  PRBool m_QueryProcessorShouldShutdown;
  queryqueue_t m_QueryQueue;
  PRBool m_QueryProcessorQueueHasItem;

  PRLock* m_pPersistentQueriesLock;
  querypersistmap_t m_PersistentQueries;
};

class QueryProcessorThread : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  QueryProcessorThread(CDatabaseEngine* pEngine) {
    NS_ASSERTION(pEngine, "Null pointer!");
    mpEngine = pEngine;
  }

  NS_IMETHOD Run()
  {
    CDatabaseEngine::QueryProcessor(mpEngine);
    return NS_OK;
  }
protected:
  CDatabaseEngine* mpEngine;
};

#endif // __DATABASE_ENGINE_H__

