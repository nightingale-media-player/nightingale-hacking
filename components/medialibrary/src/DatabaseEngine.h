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

#pragma once

// INCLUDES ===================================================================
#include <nscore.h>
#include <sqlite3.h>

#include <deque>
#include <list>
#include <map>

#include "Lock.h"
#include "Thread.h"
#include "Wait.h"

#include "DatabaseQuery.h"
#include "IDatabaseEngine.h"

//#include "Singleton.h"

// DEFINES ====================================================================
#define SONGBIRD_DATABASEENGINE_CONTRACTID  "@songbird.org/Songbird/DatabaseEngine;1"
#define SONGBIRD_DATABASEENGINE_CLASSNAME   "Songbird Database Engine Interface"

// {3E3D92D6-EF65-4b76-8670-93A025C3D76B}
#define SONGBIRD_DATABASEENGINE_CID { 0x3e3d92d6, 0x3f65, 0x4b76, { 0x86, 0x70, 0x93, 0xa0, 0x25, 0xc3, 0xd7, 0x6b } }

// FUNCTIONS ==================================================================
int SQLiteAuthorizer(void *pData, int nOp, const char *pArgA, const char *pArgB, const char *pDBName, const char *pTrigger);

// CLASSES ====================================================================
class CDatabaseEngine : public sbIDatabaseEngine
{
public:
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

  void LockDatabase(sqlite3 *pDB);
  void UnlockDatabase(sqlite3 *pDB);

  void ClearAllDBLocks();
  void CloseAllDB();

  sqlite3 *GetDBByGUID(PRUnichar *dbGUID, PRBool bCreateIfNotOpen = PR_FALSE);
  sqlite3 *FindDBByGUID(PRUnichar *dbGUID);

  PRInt32 GetDBGUIDList(std::vector<std::prustring> &vGUIDList);

  static void PR_CALLBACK QueryProcessor(void *pData);

private:
  //[database guid/name]
  typedef std::map<std::prustring, sqlite3 *>  databasemap_t;
  typedef std::map<sqlite3 *, sbCommon::CLock *> databaselockmap_t;
  typedef std::list<CDatabaseQuery *> querylist_t;
  //[table guid/name]
  typedef std::map<std::string, querylist_t> tablepersistmap_t;
  //[database guid/name]
  typedef std::map<std::prustring, tablepersistmap_t > querypersistmap_t;
  typedef std::deque<CDatabaseQuery *> queryqueue_t;

  void UpdatePersistentQueries(CDatabaseQuery *pQuery);
  void DoPersistentCallback(CDatabaseQuery *pQuery);

private:
  //sbCommon::CThread m_QueryProcessorThread;
  sbCommon::CThreadPool m_QueryProcessorThreadPool;

  databasemap_t m_Databases;
  sbCommon::CLock m_DatabasesLock;

  databaselockmap_t m_DatabaseLocks;
  sbCommon::CLock m_DatabaseLocksLock;

  queryqueue_t m_QueryQueue;
  sbCommon::CLock m_QueryQueueLock;
  sbCommon::CWait m_QueryQueueHasItem;

  sbCommon::CLock m_PersistentQueriesLock;
  querypersistmap_t m_PersistentQueries;
};