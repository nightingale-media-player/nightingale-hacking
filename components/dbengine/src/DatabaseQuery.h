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

#include <prlock.h>
#include <prmon.h>

#include <nsCOMPtr.h>
#include <nsTArray.h>
#include <nsStringGlue.h>
#include <nsInterfaceHashtable.h>
#include <nsHashKeys.h>

#include "sbIDatabaseQuery.h"
#include "DatabaseResult.h"

// DEFINES ====================================================================
#define SONGBIRD_DATABASEQUERY_CONTRACTID                 \
  "@songbirdnest.com/Songbird/DatabaseQuery;1"
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
typedef enum {
  ISNULL,
  UTF8STRING,
  STRING,
  DOUBLE,
  INTEGER32,
  INTEGER64
  } ParameterType;

struct CQueryParameter
{
  ParameterType type;
  nsCString utf8StringValue;
  nsString stringValue;
  double doubleValue;
  PRInt32 int32Value;
  PRInt64 int64Value;
};

typedef nsTArray<CQueryParameter> bindParameterArray_t;

class CDatabaseEngine;
class nsIURI;

class CDatabaseQuery : public sbIDatabaseQuery
{
friend class CDatabaseEngine;
friend int SQLiteAuthorizer(void *pData, int nOp, const char *pArgA, const char *pArgB, const char *pDBName, const char *pTrigger);
friend void SQLiteUpdateHook(void *pData, int nOp, const char *pArgA, const char *pArgB, PRInt64 nRowID);

public:
  CDatabaseQuery();
  virtual ~CDatabaseQuery();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDATABASEQUERY

protected:
  CDatabaseResult* GetResultObject();
  bindParameterArray_t* GetQueryParameters(PRInt32 aQueryIndex);

  PRLock *m_pLocationURILock;
  nsCOMPtr<nsIURI> m_LocationURI;

  PRBool m_IsPersistentQueryRegistered;
  PRBool m_HasChangedDataOfPersistQuery;

  PRBool m_PersistExecSelectiveMode;
  PRBool m_PersistExecOnInsert;
  PRBool m_PersistExecOnUpdate;
  PRBool m_PersistExecOnDelete;

  PRLock* m_pPersistentQueryTableLock;
  nsCString m_PersistentQueryTable;

  PRBool m_IsAborting;
  PRBool m_IsExecuting;
  
  PRBool m_AsyncQuery;
  PRBool m_PersistentQuery;

  PRUint32 m_CurrentQuery;
  PRInt32 m_LastError;

  PRLock* m_pQueryResultLock;
  CDatabaseResult* m_QueryResult;

  PRLock* m_pDatabaseGUIDLock;
  nsString m_DatabaseGUID;

  typedef std::vector<nsString> dbquerylist_t;
  PRLock* m_pDatabaseQueryListLock;
  dbquerylist_t m_DatabaseQueryList;

  PRMonitor* m_pQueryRunningMonitor;
  PRBool m_QueryHasCompleted;

  PRLock* m_pCallbackListLock;
  nsInterfaceHashtable<nsISupportsHashKey, sbIDatabaseQueryCallback> m_CallbackList;

  PRLock* m_pPersistentCallbackListLock;
  nsInterfaceHashtable<nsISupportsHashKey, sbIDatabaseSimpleQueryCallback> m_PersistentCallbackList;

  typedef std::set<nsCString> modifiedtables_t;
  typedef std::map<nsCString, modifiedtables_t> modifieddata_t;
  typedef std::vector< PRInt64 > dbrowids_t;
  
  PRLock* m_pModifiedDataLock;
  modifieddata_t m_ModifiedData;

  PRLock *m_pSelectedRowIDsLock;
  dbrowids_t m_SelectedRowIDs;

  PRLock *m_pInsertedRowIDsLock;
  dbrowids_t m_InsertedRowIDs;

  PRLock *m_pUpdatedRowIDsLock;
  dbrowids_t m_UpdatedRowIDs;

  PRLock *m_pDeletedRowIDsLock;
  dbrowids_t m_DeletedRowIDs;

  PRLock* m_pBindParametersLock;
  nsTArray<bindParameterArray_t> m_BindParameters;
  bindParameterArray_t* m_LastBindParameters;

private:
  NS_IMETHOD EnsureLastQueryParameter(PRUint32 aParamIndex);
};

#endif // __DATABASE_QUERY_H__

