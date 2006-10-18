/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
* \file DatabaseQuery.cpp
*
*/

// INCLUDES ===================================================================
#include "DatabaseQuery.h"
#include "DatabaseEngine.h"

#include <prmem.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <string/nsString.h>
#include <nsAutoLock.h>

#ifdef DEBUG_locks
#include <nsPrintfCString.h>
#endif

// CLASSES ====================================================================
//=============================================================================
// CDatabaseQuery Class
//=============================================================================
//-----------------------------------------------------------------------------
NS_IMPL_THREADSAFE_ISUPPORTS1(CDatabaseQuery, sbIDatabaseQuery)

//-----------------------------------------------------------------------------
CDatabaseQuery::CDatabaseQuery()
: m_IsPersistentQueryRegistered(PR_FALSE)
, m_HasChangedDataOfPersistQuery(PR_FALSE)
, m_IsAborting(PR_FALSE)
, m_IsExecuting(PR_FALSE)
, m_AsyncQuery(PR_FALSE)
, m_PersistentQuery(PR_FALSE)
, m_LastError(0)
, m_DatabaseGUID(NS_LITERAL_STRING("").get())
, m_pQueryRunningMonitor(nsAutoMonitor::NewMonitor("CDatabaseQuery.m_pdbQueryRunningMonitor"))
, m_QueryHasCompleted(PR_FALSE)
{
  m_pPersistentQueryTableLock = PR_NewLock();
  m_pQueryResultLock = PR_NewLock();
  m_pDatabaseGUIDLock = PR_NewLock();
  m_pDatabaseQueryListLock = PR_NewLock();
  m_pCallbackListLock = PR_NewLock();
  m_pPersistentCallbackListLock = PR_NewLock();
  m_pModifiedDataLock = PR_NewLock();

  NS_ASSERTION(m_pPersistentQueryTableLock, "CDatabaseQuery.m_pPersistentQueryTableLock failed");
  NS_ASSERTION(m_pQueryResultLock, "CDatabaseQuery.m_pQueryResultLock failed");
  NS_ASSERTION(m_pDatabaseGUIDLock, "CDatabaseQuery.m_pDatabaseGUIDLock failed");
  NS_ASSERTION(m_pDatabaseQueryListLock, "CDatabaseQuery.m_pDatabaseQueryListLock failed");
  NS_ASSERTION(m_pCallbackListLock, "CDatabaseQuery.m_pCallbackListLock failed");
  NS_ASSERTION(m_pPersistentCallbackListLock, "CDatabaseQuery.m_pPersistentCallbackListLock failed");
  NS_ASSERTION(m_pModifiedDataLock, "CDatabaseQuery.m_pModifiedDataLock failed");
  NS_ASSERTION(m_pQueryRunningMonitor, "CDatabaseQuery.m_pQueryRunningMonitor failed");

#ifdef DEBUG_locks
  nsCAutoString log;
  log += NS_LITERAL_CSTRING("\n\nCDatabaseQuery (") + nsPrintfCString("%x", this) + NS_LITERAL_CSTRING(") lock addresses:\n");
  log += NS_LITERAL_CSTRING("m_pPersistentQueryTableLock   = ") + nsPrintfCString("%x\n", m_pPersistentQueryTableLock);
  log += NS_LITERAL_CSTRING("m_pQueryResultLock            = ") + nsPrintfCString("%x\n", m_pQueryResultLock);
  log += NS_LITERAL_CSTRING("m_pDatabaseGUIDLock           = ") + nsPrintfCString("%x\n", m_pDatabaseGUIDLock);
  log += NS_LITERAL_CSTRING("m_pDatabaseQueryListLock      = ") + nsPrintfCString("%x\n", m_pDatabaseQueryListLock);
  log += NS_LITERAL_CSTRING("m_pCallbackListLock           = ") + nsPrintfCString("%x\n", m_pCallbackListLock);
  log += NS_LITERAL_CSTRING("m_pPersistentCallbackListLock = ") + nsPrintfCString("%x\n", m_pPersistentCallbackListLock);
  log += NS_LITERAL_CSTRING("m_pModifiedTablesLock         = ") + nsPrintfCString("%x\n", m_pModifiedTablesLock);
  log += NS_LITERAL_CSTRING("m_pQueryRunningMonitor        = ") + nsPrintfCString("%x\n", m_pQueryRunningMonitor);
  log += NS_LITERAL_CSTRING("\n");
  NS_WARNING(log.get());
#endif

  NS_NEWXPCOM(m_QueryResult, CDatabaseResult);
  NS_ASSERTION(m_QueryResult, "Failed to create DatabaseQuery.m_QueryResult");
  m_QueryResult->AddRef();
} //ctor

//-----------------------------------------------------------------------------
CDatabaseQuery::~CDatabaseQuery()
{
  nsCOMPtr<sbIDatabaseEngine> p = do_GetService(SONGBIRD_DATABASEENGINE_CONTRACTID);
  if(p) p->RemovePersistentQuery(this);
  
  RemoveAllCallbacks();

  NS_IF_RELEASE(m_QueryResult);

  if (m_pPersistentQueryTableLock)
    PR_DestroyLock(m_pPersistentQueryTableLock);
  if (m_pQueryResultLock)
    PR_DestroyLock(m_pQueryResultLock);
  if (m_pDatabaseGUIDLock)
    PR_DestroyLock(m_pDatabaseGUIDLock);
  if (m_pDatabaseQueryListLock)
    PR_DestroyLock(m_pDatabaseQueryListLock);
  if (m_pCallbackListLock)
    PR_DestroyLock(m_pCallbackListLock);
  if (m_pPersistentCallbackListLock)
    PR_DestroyLock(m_pPersistentCallbackListLock);
  if (m_pModifiedDataLock)
    PR_DestroyLock(m_pModifiedDataLock);
  if (m_pQueryRunningMonitor)
    nsAutoMonitor::DestroyMonitor(m_pQueryRunningMonitor);
} //dtor

//-----------------------------------------------------------------------------
/* void SetAsyncQuery (in PRBool bAsyncQuery); */
NS_IMETHODIMP CDatabaseQuery::SetAsyncQuery(PRBool bAsyncQuery)
{
  m_AsyncQuery = bAsyncQuery;
  return NS_OK;
} //SetAsyncQuery

//-----------------------------------------------------------------------------
/* PRBool IsAyncQuery (); */
NS_IMETHODIMP CDatabaseQuery::IsAyncQuery(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = m_AsyncQuery;
  return NS_OK;
} //IsAyncQuery

//-----------------------------------------------------------------------------
/* void SetPersistentQuery (in PRBool bPersistentQuery); */
NS_IMETHODIMP CDatabaseQuery::SetPersistentQuery(PRBool bPersistentQuery)
{
  m_PersistentQuery = bPersistentQuery;
  return NS_OK;
} //SetPersistentQuery

//-----------------------------------------------------------------------------
/* PRBool IsPersistentQuery (); */
NS_IMETHODIMP CDatabaseQuery::IsPersistentQuery(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = m_PersistentQuery;
  return NS_OK;
} //IsPersistentQuery

//-----------------------------------------------------------------------------
/* void AddPersistentQueryCallback (in sbIDatabasePersistentQueryCallback dbPersistCB); */
NS_IMETHODIMP CDatabaseQuery::AddSimpleQueryCallback(sbIDatabaseSimpleQueryCallback *dbPersistCB)
{
  NS_ENSURE_ARG_POINTER(dbPersistCB);

  nsAutoLock lock(m_pPersistentCallbackListLock);

  size_t nSize = m_PersistentCallbackList.size();
  for(size_t nCurrent = 0; nCurrent < nSize; nCurrent++)
  {
    if(m_PersistentCallbackList[nCurrent] == dbPersistCB)
      return NS_OK;
  }

  dbPersistCB->AddRef();
  m_PersistentCallbackList.push_back(dbPersistCB);

  return NS_OK;
} //AddPersistentQueryCallback

//-----------------------------------------------------------------------------
/* void RemovePersistentQueryCallback (in sbIDatabasePersistentQueryCallback dbPersistCB); */
NS_IMETHODIMP CDatabaseQuery::RemoveSimpleQueryCallback(sbIDatabaseSimpleQueryCallback *dbPersistCB)
{
  NS_ENSURE_ARG_POINTER(dbPersistCB);

  PR_Lock(m_pPersistentCallbackListLock);

  persistentcallbacklist_t::iterator itCallback = m_PersistentCallbackList.begin();
  for(; itCallback != m_PersistentCallbackList.end(); itCallback++)
  {
    if((*itCallback) == dbPersistCB)
    {
      (*itCallback)->Release();
      m_PersistentCallbackList.erase(itCallback);
    }
  }

  PR_Unlock(m_pPersistentCallbackListLock);

  return NS_OK;
} //RemovePersistentQueryCallback

//-----------------------------------------------------------------------------
/* void SetDatabaseGUID (in wstring dbGUID); */
NS_IMETHODIMP CDatabaseQuery::SetDatabaseGUID(const nsAString &dbGUID)
{
  PR_Lock(m_pDatabaseGUIDLock);
  m_DatabaseGUID = dbGUID;
  PR_Unlock(m_pDatabaseGUIDLock);
  return NS_OK;
} //SetDatabaseGUID

//-----------------------------------------------------------------------------
/* wstring GetDatabaseGUID (); */
NS_IMETHODIMP CDatabaseQuery::GetDatabaseGUID(nsAString &_retval)
{
  PR_Lock(m_pDatabaseGUIDLock);
  _retval = m_DatabaseGUID;
  PR_Unlock(m_pDatabaseGUIDLock);
  return NS_OK;
} //GetDatabaseGUID

//-----------------------------------------------------------------------------
/* void AddQuery (in wstring strQuery); */
NS_IMETHODIMP CDatabaseQuery::AddQuery(const nsAString &strQuery)
{
  PR_Lock(m_pDatabaseQueryListLock);
  m_DatabaseQueryList.push_back(PromiseFlatString(strQuery));
  PR_Unlock(m_pDatabaseQueryListLock);
  return NS_OK;
} //AddQuery

//-----------------------------------------------------------------------------
/* PRInt32 GetQueryCount (); */
NS_IMETHODIMP CDatabaseQuery::GetQueryCount(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  PR_Lock(m_pDatabaseQueryListLock);
  *_retval = (PRInt32)m_DatabaseQueryList.size();
  PR_Unlock(m_pDatabaseQueryListLock);
  return NS_OK;
} //GetQueryCount

//-----------------------------------------------------------------------------
/* wstring GetQuery (in PRInt32 nIndex); */
NS_IMETHODIMP CDatabaseQuery::GetQuery(PRInt32 nIndex, nsAString &_retval)
{
  NS_ENSURE_ARG_MIN(nIndex, 0);

  PR_Lock(m_pDatabaseQueryListLock);

  if((PRUint32)nIndex < m_DatabaseQueryList.size())
    _retval = m_DatabaseQueryList[nIndex];

  PR_Unlock(m_pDatabaseQueryListLock);

  return NS_OK;
} //GetQuery

//-----------------------------------------------------------------------------
/* void ResetQuery (); */
NS_IMETHODIMP CDatabaseQuery::ResetQuery()
{
  // XXXBen - Seems like we should have one lock that protects both of these.
  //          What happens when one thread resets the query and another tries
  //          to read the query results? Maybe we could just switch the order.
  // XXXAus - This is ok by design.
  PR_Lock(m_pDatabaseQueryListLock);
  m_DatabaseQueryList.clear();
  PR_Unlock(m_pDatabaseQueryListLock);

  PR_Lock(m_pQueryResultLock);
  m_QueryResult->ClearResultSet();
  PR_Unlock(m_pQueryResultLock);

  return NS_OK;
}

//-----------------------------------------------------------------------------
/* sbIDatabaseResult GetResultObject (); */
NS_IMETHODIMP CDatabaseQuery::GetResultObject(sbIDatabaseResult **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PR_Lock(m_pQueryResultLock);

  *_retval = m_QueryResult;
  m_QueryResult->AddRef();

  PR_Unlock(m_pQueryResultLock);

  return NS_OK;
} //GetResultObject

//-----------------------------------------------------------------------------
/* noscript sbIDatabaseResult GetResultObjectOrphan(); */
NS_IMETHODIMP CDatabaseQuery::GetResultObjectOrphan(sbIDatabaseResult **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsAutoLock lock(m_pQueryResultLock);

  CDatabaseResult *pRes = nsnull;
  NS_NEWXPCOM(pRes, CDatabaseResult);
  NS_ENSURE_TRUE(pRes, NS_ERROR_OUT_OF_MEMORY);

  int sc = m_QueryResult->m_RowCells.size();

  pRes->m_ColumnNames = m_QueryResult->m_ColumnNames;
  pRes->m_RowCells = m_QueryResult->m_RowCells;
  pRes->AddRef();

  NS_ASSERTION(sc == pRes->m_RowCells.size(), "Query Result Size Changed During Object Orphan Event");

  //Transfer references to the callee.
  *_retval = pRes;

  return NS_OK;
} //GetResultObjectOrphan

//-----------------------------------------------------------------------------
/* PRInt32 GetLastError (); */
NS_IMETHODIMP CDatabaseQuery::GetLastError(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = m_LastError;
  return NS_OK;
} //GetLastError

//-----------------------------------------------------------------------------
/* void SetLastError (in PRInt32 dbError); */
NS_IMETHODIMP CDatabaseQuery::SetLastError(PRInt32 dbError)
{
  NS_ENSURE_ARG_MIN(dbError, 0);
  m_LastError = dbError;
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseQuery::Execute(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 1;

  {
    nsAutoMonitor mon(m_pQueryRunningMonitor);
    m_QueryHasCompleted = PR_FALSE;
  }

  nsCOMPtr<sbIDatabaseEngine> p = do_GetService(SONGBIRD_DATABASEENGINE_CONTRACTID);
  if(p) p->SubmitQuery(this, _retval);
  
  if(*_retval != 0)
  {
    nsAutoMonitor mon(m_pQueryRunningMonitor);

    m_QueryHasCompleted = PR_TRUE;
    mon.NotifyAll();
  }

  return NS_OK;
} //Execute

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseQuery::WaitForCompletion(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  {
    nsAutoMonitor mon(m_pQueryRunningMonitor);
    while (!m_QueryHasCompleted)
      mon.Wait();
  }
  *_retval = NS_OK;
  return NS_OK;
} //WaitForCompletion

//-----------------------------------------------------------------------------
/* void AddCallback (in IDatabaseQueryCallback dbCallback); */
NS_IMETHODIMP CDatabaseQuery::AddCallback(sbIDatabaseQueryCallback *dbCallback)
{
  NS_ENSURE_ARG_POINTER(dbCallback);

  nsAutoLock lock(m_pCallbackListLock);

  size_t nSize = m_CallbackList.size();
  for(size_t nCurrent = 0; nCurrent < nSize; nCurrent++)
  {
    if(m_CallbackList[nCurrent] == dbCallback)
      return NS_OK;
  }

  dbCallback->AddRef();
  m_CallbackList.push_back(dbCallback);

  return NS_OK;
} //AddCallback

//-----------------------------------------------------------------------------
/* void RemoveCallback (in IDatabaseQueryCallback dbCallback); */
NS_IMETHODIMP CDatabaseQuery::RemoveCallback(sbIDatabaseQueryCallback *dbCallback)
{
  NS_ENSURE_ARG_POINTER(dbCallback);

  nsAutoLock lock(m_pCallbackListLock);

  callbacklist_t::iterator itCallback = m_CallbackList.begin();
  for(; itCallback != m_CallbackList.end(); itCallback++)
  {
    if((*itCallback) == dbCallback)
    {
      (*itCallback)->Release();
      m_CallbackList.erase(itCallback);
    }
  }

  return NS_OK;
} //RemoveCallback

//-----------------------------------------------------------------------------
/* PRBool IsExecuting (); */
NS_IMETHODIMP CDatabaseQuery::IsExecuting(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = m_IsExecuting;
  return NS_OK;
} //IsExecuting

//-----------------------------------------------------------------------------
/* PRInt32 CurrentQuery (); */
NS_IMETHODIMP CDatabaseQuery::CurrentQuery(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = m_CurrentQuery;
  return NS_OK;
} //CurrentQuery

//-----------------------------------------------------------------------------
/* void Abort (); */
NS_IMETHODIMP CDatabaseQuery::Abort(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE;

  if(m_IsExecuting)
  {
    PRInt32 nWaitResult = 0;

    m_IsAborting = PR_TRUE;
    WaitForCompletion(&nWaitResult);

    *_retval = PR_TRUE;
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
CDatabaseResult *CDatabaseQuery::GetResultObject()
{
  nsAutoLock lock(m_pQueryResultLock);

  //m_QueryResult->AddRef();
  return m_QueryResult;
} //GetResultObject

//-----------------------------------------------------------------------------
void CDatabaseQuery::RemoveAllCallbacks()
{
  {
    nsAutoLock lock(m_pCallbackListLock);
    callbacklist_t::iterator itCallback = m_CallbackList.begin();
    for(; itCallback != m_CallbackList.end(); itCallback++)
      (*itCallback)->Release();
    m_CallbackList.clear();
  }

  {
    nsAutoLock lock(m_pPersistentCallbackListLock);
    persistentcallbacklist_t::iterator itCallback = m_PersistentCallbackList.begin();
    for(; itCallback != m_PersistentCallbackList.end(); itCallback++)
      (*itCallback)->Release();
    m_PersistentCallbackList.clear();
  }

  return;
}
