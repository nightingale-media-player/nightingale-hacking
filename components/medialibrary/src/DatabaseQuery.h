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
* \file  DatabaseQuery.h
* \brief Songbird Database Object Definition.
*/

#ifndef __DATABASE_QUERY_H__
#define __DATABASE_QUERY_H__

// FORWARD REFERENCES =========================================================
class CDatabaseQuery;

// INCLUDES ===================================================================
#include <string>
#include <vector>
#include <set>

#include "IDatabaseQuery.h"
#include "DatabaseResult.h"
#include "DatabaseEngine.h"

#include <prlock.h>
#include <prmon.h>

#include <xpcom/nsCOMPtr.h>

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
#define SONGBIRD_DATABASEQUERY_CONTRACTID                 \
  "@songbird.org/Songbird/DatabaseQuery;1"
#define SONGBIRD_DATABASEQUERY_CLASSNAME                  \
  "Songbird Database Query Interface"
#define SONGBIRD_DATABASEQUERY_CID                        \
{ /* 377a6592-64bd-4a6b-a941-d488abb5a8aa */              \
  0x377a6592,                                             \
  0x64bd,                                                 \
  0x4a6b,                                                 \
  {0xa9, 0x41, 0xd4, 0x88, 0xab, 0xb5, 0xa8, 0xaa}        \
}
// CLASSES ====================================================================
class sbIDatabaseEngine;

class CDatabaseQuery : public sbIDatabaseQuery
{
friend class CDatabaseEngine;
friend int SQLiteAuthorizer(void *pData, int nOp, const char *pArgA, const char *pArgB, const char *pDBName, const char *pTrigger);

public:
  CDatabaseQuery();
  virtual ~CDatabaseQuery();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDATABASEQUERY

protected:
  CDatabaseResult* GetResultObject();

  PRBool m_IsPersistentQueryRegistered;
  PRBool m_HasChangedDataOfPersistQuery;

  PRLock* m_pPersistentQueryTableLock;
  std::string m_PersistentQueryTable;

  PRBool m_IsAborting;
  PRBool m_IsExecuting;
  
  PRBool m_AsyncQuery;
  PRBool m_PersistentQuery;

  PRInt32 m_CurrentQuery;
  PRInt32 m_LastError;

  PRLock* m_pQueryResultLock;
  CDatabaseResult* m_QueryResult;

  PRLock* m_pDatabaseGUIDLock;
  std::prustring m_DatabaseGUID;

  typedef std::vector<std::prustring> dbquerylist_t;
  PRLock* m_pDatabaseQueryListLock;
  dbquerylist_t m_DatabaseQueryList;

  PRMonitor* m_pQueryRunningMonitor;
  PRBool m_QueryHasCompleted;

  typedef std::vector<sbIDatabaseQueryCallback *> callbacklist_t;
  PRLock* m_pCallbackListLock;
  callbacklist_t m_CallbackList;

  typedef std::vector<sbIDatabaseSimpleQueryCallback *> persistentcallbacklist_t;
  PRLock* m_pPersistentCallbackListLock;
  persistentcallbacklist_t m_PersistentCallbackList;

  typedef std::set<std::string> modifiedtables_t;
  PRLock* m_pModifiedTablesLock;
  modifiedtables_t m_ModifiedTables;

  nsCOMPtr<sbIDatabaseEngine> m_Engine;
};

#endif // __DATABASE_QUERY_H__

