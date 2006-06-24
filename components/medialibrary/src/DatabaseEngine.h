/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
#include "IDatabaseEngine.h"

#include <prlock.h>
#include <prmon.h>
#include <nsCOMArray.h>
#include <nsIThread.h>
#include <nsIRunnable.h>

#ifndef PRUSTRING_DEFINED
#define PRUSTRING_DEFINED
#include <string>
#include "nscore.h"
namespace std
{
  typedef basic_string< PRUnichar > prustring;
};
#endif

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

protected:
  PRInt32 OpenDB(PRUnichar *dbGUID);
  PRInt32 CloseDB(PRUnichar *dbGUID);

  PRInt32 DropDB(PRUnichar *dbGUID);

  PRInt32 SubmitQueryPrivate(CDatabaseQuery *dbQuery);

  void AddPersistentQueryPrivate(CDatabaseQuery *pQuery, const std::string &strTableName);
  void RemovePersistentQueryPrivate(CDatabaseQuery *pQuery);

  nsresult LockDatabase(sqlite3 *pDB);
  nsresult UnlockDatabase(sqlite3 *pDB);

  nsresult ClearAllDBLocks();
  nsresult CloseAllDB();

  sqlite3 *GetDBByGUID(PRUnichar *dbGUID, PRBool bCreateIfNotOpen = PR_FALSE);
  sqlite3 *FindDBByGUID(PRUnichar *dbGUID);

  PRInt32 GetDBGUIDList(std::vector<std::prustring> &vGUIDList);

  static void PR_CALLBACK QueryProcessor(CDatabaseEngine* pEngine);

private:
  //[database guid/name]
  typedef std::map<std::prustring, sqlite3 *>  databasemap_t;
  typedef std::map<sqlite3 *, PRLock *> databaselockmap_t;
  typedef std::list<CDatabaseQuery *> querylist_t;
  //[table guid/name]
  typedef std::map<std::string, querylist_t> tablepersistmap_t;
  //[database guid/name]
  typedef std::map<std::prustring, tablepersistmap_t > querypersistmap_t;
  typedef std::deque<CDatabaseQuery *> queryqueue_t;

  void UpdatePersistentQueries(CDatabaseQuery *pQuery);
  void DoPersistentCallback(CDatabaseQuery *pQuery);

private:

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

  PRLock* m_pCaseConversionLock;
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

