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
* \file DatabaseQuery.cpp
*
*/

// INCLUDES ===================================================================
#include "DatabaseQuery.h"
#include "DatabaseEngine.h"
#include "DatabasePreparedStatement.h"

#include <prlog.h>
#include <prmem.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <nsStringGlue.h>

#include <nsAutoLock.h>
#include <nsNetUtil.h>

#include <nsIServiceManager.h>
#include <nsThreadUtils.h>
#include <nsSupportsArray.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>

#include <sbProxyUtils.h>
#include <sbLockUtils.h>

#ifdef DEBUG_locks
#include <nsPrintfCString.h>
#endif

#define DATABASEQUERY_MAX_WAIT_TIME (50)

#ifdef PR_LOGGING
static PRLogModuleInfo* sDatabaseQueryLog = nsnull;
#define LOG(args)   PR_LOG(sDatabaseQueryLog, PR_LOG_ALWAYS, args)
#else
#define LOG(args)   /* nothing */
#endif


// CLASSES ====================================================================
//=============================================================================
// CDatabaseQuery Class
//=============================================================================
//-----------------------------------------------------------------------------
NS_IMPL_THREADSAFE_ADDREF(CDatabaseQuery)
NS_IMPL_THREADSAFE_RELEASE(CDatabaseQuery)

NS_INTERFACE_MAP_BEGIN(CDatabaseQuery)
  NS_IMPL_QUERY_CLASSINFO(CDatabaseQuery)
  NS_INTERFACE_MAP_ENTRY(sbIDatabaseQuery)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIDatabaseQuery)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER1(CDatabaseQuery, 
                             sbIDatabaseQuery)

NS_DECL_CLASSINFO(CDatabaseQuery)
NS_IMPL_THREADSAFE_CI(CDatabaseQuery)

//-----------------------------------------------------------------------------
CDatabaseQuery::CDatabaseQuery()
: m_StateLock(nsnull)
, m_IsAborting(PR_FALSE)
, m_IsExecuting(PR_FALSE)
, m_AsyncQuery(PR_FALSE)
, m_CurrentQueryLock(nsnull)
, m_CurrentQuery(-1)
, m_LastError(0)
, m_pQueryRunningMonitor(nsAutoMonitor::NewMonitor("CDatabaseQuery.m_pdbQueryRunningMonitor"))
, m_QueryHasCompleted(PR_FALSE)
, m_LastBindParameters(nsnull)
, m_RollingLimit(0)
, m_RollingLimitColumnIndex(0)
, m_RollingLimitResult(0)
{
  m_pLocationURILock = PR_NewLock();
  m_StateLock = PR_NewLock();
  m_CurrentQueryLock = PR_NewLock();
  m_pQueryResultLock = PR_NewLock();
  m_pDatabaseGUIDLock = PR_NewLock();
  m_pDatabaseQueryListLock = PR_NewLock();
  m_pCallbackListLock = PR_NewLock();
  m_pModifiedDataLock = PR_NewLock();
  m_pSelectedRowIDsLock = PR_NewLock();
  m_pInsertedRowIDsLock = PR_NewLock();
  m_pUpdatedRowIDsLock = PR_NewLock();
  m_pDeletedRowIDsLock = PR_NewLock();
  m_pBindParametersLock = PR_NewLock();
  m_pRollingLimitLock = PR_NewLock();

  NS_ASSERTION(m_pLocationURILock, "CDatabaseQuery.m_pLocationURILock failed");
  NS_ASSERTION(m_StateLock, "CDatabaseQuery.m_StateLock failed");
  NS_ASSERTION(m_CurrentQueryLock, "CDatabaseQuery.m_CurrentQueryLock failed");
  NS_ASSERTION(m_pQueryResultLock, "CDatabaseQuery.m_pQueryResultLock failed");
  NS_ASSERTION(m_pDatabaseGUIDLock, "CDatabaseQuery.m_pDatabaseGUIDLock failed");
  NS_ASSERTION(m_pDatabaseQueryListLock, "CDatabaseQuery.m_pDatabaseQueryListLock failed");
  NS_ASSERTION(m_pCallbackListLock, "CDatabaseQuery.m_pCallbackListLock failed");
  NS_ASSERTION(m_pSelectedRowIDsLock, "CDatabaseQuery.m_pModifiedRowIDsLock failed");
  NS_ASSERTION(m_pModifiedDataLock, "CDatabaseQuery.m_pModifiedDataLock failed");
  NS_ASSERTION(m_pInsertedRowIDsLock, "CDatabaseQuery.m_pModifiedRowIDsLock failed");
  NS_ASSERTION(m_pUpdatedRowIDsLock, "CDatabaseQuery.m_pModifiedRowIDsLock failed");
  NS_ASSERTION(m_pDeletedRowIDsLock, "CDatabaseQuery.m_pModifiedRowIDsLock failed");
  NS_ASSERTION(m_pQueryRunningMonitor, "CDatabaseQuery.m_pQueryRunningMonitor failed");
  NS_ASSERTION(m_pBindParametersLock, "CDatabaseQuery.m_pBindParametersLock failed");
  NS_ASSERTION(m_pRollingLimitLock, "CDatabaseQuery.m_pRollingLimitLock failed");

#ifdef DEBUG_locks
  nsCAutoString log;
  log += NS_LITERAL_CSTRING("\n\nCDatabaseQuery (") + nsPrintfCString("%x", this) + NS_LITERAL_CSTRING(") lock addresses:\n");
  log += NS_LITERAL_CSTRING("m_pQueryResultLock            = ") + nsPrintfCString("%x\n", m_pQueryResultLock);
  log += NS_LITERAL_CSTRING("m_pDatabaseGUIDLock           = ") + nsPrintfCString("%x\n", m_pDatabaseGUIDLock);
  log += NS_LITERAL_CSTRING("m_pDatabaseQueryListLock      = ") + nsPrintfCString("%x\n", m_pDatabaseQueryListLock);
  log += NS_LITERAL_CSTRING("m_pModifiedTablesLock         = ") + nsPrintfCString("%x\n", m_pModifiedTablesLock);
  log += NS_LITERAL_CSTRING("m_pQueryRunningMonitor        = ") + nsPrintfCString("%x\n", m_pQueryRunningMonitor);
  log += NS_LITERAL_CSTRING("m_pBindParametersLock         = ") + nsPrintfCString("%x\n", m_pBindParametersLock);
  log += NS_LITERAL_CSTRING("m_pRollingLimitLock           = ") + nsPrintfCString("%x\n", m_pRollingLimitLock);
  log += NS_LITERAL_CSTRING("\n");
  NS_WARNING(log.get());
#endif

  NS_NEWXPCOM(m_QueryResult, CDatabaseResult);
  NS_ASSERTION(m_QueryResult, "Failed to create DatabaseQuery.m_QueryResult");
  m_QueryResult->AddRef();

  m_CallbackList.Init();

#ifdef PR_LOGGING
  if (!sDatabaseQueryLog)
    sDatabaseQueryLog = PR_NewLogModule("sbDatabaseQuery");
#endif

} //ctor

//-----------------------------------------------------------------------------
CDatabaseQuery::~CDatabaseQuery()
{
  NS_IF_RELEASE(m_QueryResult);

  if (m_pLocationURILock)
    PR_DestroyLock(m_pLocationURILock);

  if (m_StateLock)
    PR_DestroyLock(m_StateLock);

  if (m_CurrentQueryLock)
    PR_DestroyLock(m_CurrentQueryLock);

  if (m_pQueryResultLock)
    PR_DestroyLock(m_pQueryResultLock);

  if (m_pDatabaseGUIDLock)
    PR_DestroyLock(m_pDatabaseGUIDLock);

  if (m_pDatabaseQueryListLock)
    PR_DestroyLock(m_pDatabaseQueryListLock);

  if (m_pCallbackListLock)
    PR_DestroyLock(m_pCallbackListLock);

  if (m_pModifiedDataLock)
    PR_DestroyLock(m_pModifiedDataLock);

  if (m_pSelectedRowIDsLock)
    PR_DestroyLock(m_pSelectedRowIDsLock);

  if (m_pInsertedRowIDsLock)
    PR_DestroyLock(m_pInsertedRowIDsLock);

  if (m_pUpdatedRowIDsLock)
    PR_DestroyLock(m_pUpdatedRowIDsLock);

  if (m_pDeletedRowIDsLock)
    PR_DestroyLock(m_pDeletedRowIDsLock);

  if (m_pBindParametersLock)
    PR_DestroyLock(m_pBindParametersLock);

  if (m_pRollingLimitLock)
    PR_DestroyLock(m_pRollingLimitLock);

  if (m_pQueryRunningMonitor)
    nsAutoMonitor::DestroyMonitor(m_pQueryRunningMonitor);

  m_CallbackList.Clear();
} //dtor

nsresult CDatabaseQuery::Init()
{
  nsresult rv;
  mDatabaseEngine = do_GetService(SONGBIRD_DATABASEENGINE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-----------------------------------------------------------------------------
/* attribute nsIURI databaseLocation; */
NS_IMETHODIMP CDatabaseQuery::GetDatabaseLocation(nsIURI * *aDatabaseLocation)
{
  NS_ENSURE_ARG_POINTER(aDatabaseLocation);

  nsresult rv = NS_OK;
  *aDatabaseLocation = nsnull;

  sbSimpleAutoLock lock(m_pLocationURILock);
  if(m_LocationURI)
  {
    rv = m_LocationURI->Clone(aDatabaseLocation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
} //GetDatabaseLocation

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseQuery::SetDatabaseLocation(nsIURI * aDatabaseLocation)
{
  NS_ENSURE_ARG_POINTER(aDatabaseLocation);

  PRBool isFile = PR_FALSE;
  nsresult rv = NS_ERROR_UNEXPECTED;

  if(NS_SUCCEEDED(aDatabaseLocation->SchemeIs("file", &isFile)) &&
     isFile)
  {
    nsCAutoString spec;
    rv = aDatabaseLocation->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIFile> location;
    rv = NS_GetFileFromURLSpec(spec, getter_AddRefs(location));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool isReadable, isWritable, isDirectory;
    rv = location->IsDirectory(&isDirectory);
    NS_ENSURE_SUCCESS(rv, rv);

    if(!isDirectory)
      return NS_ERROR_INVALID_ARG;

    rv = location->IsReadable(&isReadable);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = location->IsWritable(&isWritable);
    NS_ENSURE_SUCCESS(rv, rv);

    if(isReadable && isWritable)
    {
      sbSimpleAutoLock lock(m_pLocationURILock);
      rv = aDatabaseLocation->Clone(getter_AddRefs(m_LocationURI));
      NS_ENSURE_SUCCESS(rv, rv);

      // Remember the thread that this nsStandardURL was created on so we can
      // later release it on the same thread to prevent an assertion
      nsCOMPtr<nsIThread> thread;
      rv = NS_GetCurrentThread(getter_AddRefs(thread));
      NS_ENSURE_SUCCESS(rv, rv);
      mLocationURIOwningThread = do_QueryInterface(thread, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else
  {
    //We only support database files stored locally, for now.
    rv = NS_ERROR_NOT_IMPLEMENTED;
  }

  return rv;
} //SetDatabaseLocation

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
/* void AddSimpleQueryCallback (in sbIDatabaseSimpleQueryCallback dbPersistCB); */
NS_IMETHODIMP CDatabaseQuery::AddSimpleQueryCallback(sbIDatabaseSimpleQueryCallback *dbPersistCB)
{
  NS_ENSURE_ARG_POINTER(dbPersistCB);
  nsCOMPtr<sbIDatabaseSimpleQueryCallback> proxiedCallback;

  nsresult rv = SB_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                     NS_GET_IID(sbIDatabaseSimpleQueryCallback),
                                     dbPersistCB,
                                     NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                     getter_AddRefs(proxiedCallback));
  NS_ENSURE_SUCCESS(rv, rv);

  sbSimpleAutoLock lock(m_pCallbackListLock);
  m_CallbackList.Put(dbPersistCB, proxiedCallback);

  return NS_OK;
} //AddPersistentQueryCallback

//-----------------------------------------------------------------------------
/* void RemoveSimpleQueryCallback (in sbIDatabasePersistentQueryCallback dbPersistCB); */
NS_IMETHODIMP CDatabaseQuery::RemoveSimpleQueryCallback(sbIDatabaseSimpleQueryCallback *dbPersistCB)
{
  NS_ENSURE_ARG_POINTER(dbPersistCB);

  PR_Lock(m_pCallbackListLock);
  m_CallbackList.Remove(dbPersistCB);
  PR_Unlock(m_pCallbackListLock);

  return NS_OK;
} //RemoveSimpleQueryCallback

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
  // First, we need to create a prepared statement.
  nsCOMPtr<sbIDatabasePreparedStatement> preparedStatement;
  nsresult rv = PrepareQuery(strQuery, getter_AddRefs(preparedStatement));
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = AddPreparedStatement(preparedStatement);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
} //AddQuery

//-----------------------------------------------------------------------------
/* sbIPreparedStatement PrepareQuery (in wstring strQuery); */
NS_IMETHODIMP CDatabaseQuery::PrepareQuery(const nsAString &strQuery, sbIDatabasePreparedStatement **_retval)
{  
  nsCOMPtr<sbIDatabasePreparedStatement> preparedStatement = new CDatabasePreparedStatement(strQuery);
  NS_ENSURE_TRUE(preparedStatement, NS_ERROR_OUT_OF_MEMORY);
  preparedStatement.forget(_retval);

  return NS_OK;
} //AddQuery

NS_IMETHODIMP CDatabaseQuery::AddPreparedStatement(sbIDatabasePreparedStatement *preparedStatement) 
{
  NS_ENSURE_ARG_POINTER(preparedStatement);

  PR_Lock(m_pDatabaseQueryListLock);
  m_DatabaseQueryList.push_back(preparedStatement);
  PR_Unlock(m_pDatabaseQueryListLock);

  // Also add an element to the bind parameters array
  PR_Lock(m_pBindParametersLock);
  m_BindParameters.resize(m_BindParameters.size()+1);
  PR_Unlock(m_pBindParametersLock);
  
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRInt32 GetQueryCount (); */
NS_IMETHODIMP CDatabaseQuery::GetQueryCount(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  PR_Lock(m_pDatabaseQueryListLock);
  *_retval = (PRInt32)m_DatabaseQueryList.size();
  PR_Unlock(m_pDatabaseQueryListLock);
  return NS_OK;
} //GetQueryCount

//-----------------------------------------------------------------------------
/* sbIDatabasePreparedStatement GetQuery (in PRInt32 nIndex); */
NS_IMETHODIMP CDatabaseQuery::GetQuery(PRUint32 nIndex, sbIDatabasePreparedStatement **_retval)
{
  nsresult rv = NS_OK;
  
  NS_ENSURE_ARG_MIN(nIndex, 0);
  NS_ENSURE_ARG_POINTER(_retval);

  PR_Lock(m_pDatabaseQueryListLock);

  if((PRUint32)nIndex < m_DatabaseQueryList.size()) {
    NS_ADDREF(*_retval = m_DatabaseQueryList[nIndex]);
  }
  else {
    rv = NS_ERROR_ILLEGAL_VALUE;
  }
  PR_Unlock(m_pDatabaseQueryListLock);

  return rv;
} //GetQuery

//-----------------------------------------------------------------------------
/* sbIDatabasePreparedStatement PopQuery(); */
NS_IMETHODIMP CDatabaseQuery::PopQuery(sbIDatabasePreparedStatement **_retval)
{
  nsresult rv = NS_OK;
  
  NS_ENSURE_ARG_POINTER(_retval);

  PR_Lock(m_pDatabaseQueryListLock);

  if(m_DatabaseQueryList.size() > 0) {
    NS_ADDREF(*_retval = m_DatabaseQueryList[0]);
    m_DatabaseQueryList.pop_front();
  }
  else {
    rv = NS_ERROR_ILLEGAL_VALUE;
  }
  PR_Unlock(m_pDatabaseQueryListLock);

  return rv;
} //PopQuery

//-----------------------------------------------------------------------------
/* void ResetQuery (); */
NS_IMETHODIMP CDatabaseQuery::ResetQuery()
{
  // XXXBen - Seems like we should have one lock that protects both of these.
  //          What happens when one thread resets the query and another tries
  //          to read the query results? Maybe we could just switch the order.
  // XXXAus - This is ok by design.

  // Make sure we're not running
  nsAutoMonitor mon(m_pQueryRunningMonitor);

  // These should be in sync, but just in case
  NS_ASSERTION( !m_IsExecuting, "Resetting a query that is executing!!!!!");
  NS_ASSERTION( m_QueryHasCompleted || m_CurrentQuery == PR_UINT32_MAX, 
    "Resetting a query that is not complete!!!!!");

  // Make sure no-one is touching m_IsExecuting
  PR_Lock(m_StateLock);

  PR_Lock(m_pDatabaseQueryListLock);
  m_DatabaseQueryList.clear();
  PR_Unlock(m_pDatabaseQueryListLock);

  // Also clear parameters array
  PR_Lock(m_pBindParametersLock);
  m_BindParameters.clear();
  PR_Unlock(m_pBindParametersLock);

  PR_Lock(m_pQueryResultLock);
  m_QueryResult->ClearResultSet();
  PR_Unlock(m_pQueryResultLock);

  PR_Lock(m_pRollingLimitLock);
  m_RollingLimit = 0;
  m_RollingLimitColumnIndex = 0;
  m_RollingLimitResult = 0;
  PR_Unlock(m_pRollingLimitLock);

  PR_Unlock(m_StateLock);

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
  LOG(("DBQ:[0x%x] Execute - ENTRY POINT", this));
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = 1;

  {
    nsAutoMonitor mon(m_pQueryRunningMonitor);
    m_QueryHasCompleted = PR_FALSE;
  }

  // Will re-enter back to WaitForCompletion and wait if Sync Query
  //   returns failure only if shutting down
  //   retval is 1 if an error
  //             0 if successful OR already running
  nsresult rv = mDatabaseEngine->SubmitQuery(this, _retval);
  NS_ENSURE_SUCCESS(rv, rv);

  if(*_retval != SQLITE_OK)
  {
    nsAutoMonitor mon(m_pQueryRunningMonitor);
    m_QueryHasCompleted = PR_TRUE;
    mon.NotifyAll();
    return NS_ERROR_FAILURE;
  }

  LOG(("DBQ:[0x%x] Execute - EXIT POINT\n\n", this));

  return NS_OK;
} //Execute

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseQuery::WaitForCompletion(PRInt32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  {
    nsAutoMonitor mon(m_pQueryRunningMonitor);
    while (!m_QueryHasCompleted) {
      mon.Wait( PR_MillisecondsToInterval(DATABASEQUERY_MAX_WAIT_TIME) );
    }

    NS_ASSERTION( !m_IsExecuting, "Query marked completed but still executing.");
  }
  *_retval = NS_OK;
  return NS_OK;
} //WaitForCompletion

//-----------------------------------------------------------------------------
/* PRBool IsExecuting (); */
NS_IMETHODIMP CDatabaseQuery::IsExecuting(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  PR_Lock(m_StateLock);
  *_retval = m_IsExecuting;
  PR_Unlock(m_StateLock);

  return NS_OK;
} //IsExecuting

//-----------------------------------------------------------------------------
/* PRInt32 CurrentQuery (); */
NS_IMETHODIMP CDatabaseQuery::CurrentQuery(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  PR_Lock(m_CurrentQueryLock);
  *_retval = m_CurrentQuery;
  PR_Unlock(m_CurrentQueryLock);

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

    PR_Lock(m_StateLock);
    m_IsAborting = PR_TRUE;
    PR_Unlock(m_StateLock);

    WaitForCompletion(&nWaitResult);

    *_retval = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::GetRollingLimit(PRUint64 *aRollingLimit)
{
  NS_ENSURE_ARG_POINTER(aRollingLimit);

  sbSimpleAutoLock lock(m_pRollingLimitLock);

  *aRollingLimit = m_RollingLimit;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::SetRollingLimit(PRUint64 aRollingLimit)
{
  sbSimpleAutoLock lock(m_pRollingLimitLock);

  m_RollingLimit = aRollingLimit;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::GetRollingLimitColumnIndex(PRUint32 *aRollingLimitColumnIndex)
{
  NS_ENSURE_ARG_POINTER(aRollingLimitColumnIndex);

  sbSimpleAutoLock lock(m_pRollingLimitLock);

  *aRollingLimitColumnIndex = m_RollingLimitColumnIndex;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::SetRollingLimitColumnIndex(PRUint32 aRollingLimitColumnIndex)
{
  sbSimpleAutoLock lock(m_pRollingLimitLock);

  m_RollingLimitColumnIndex = aRollingLimitColumnIndex;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::GetRollingLimitResult(PRUint32 *aRollingLimitResult)
{
  NS_ENSURE_ARG_POINTER(aRollingLimitResult);

  sbSimpleAutoLock lock(m_pRollingLimitLock);

  *aRollingLimitResult = m_RollingLimitResult;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::SetRollingLimitResult(PRUint32 aRollingLimitResult)
{
  sbSimpleAutoLock lock(m_pRollingLimitLock);

  m_RollingLimitResult = aRollingLimitResult;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::BindUTF8StringParameter(PRUint32 aParamIndex,
                                                      const nsACString &aValue)
{
  NS_ENSURE_TRUE(m_BindParameters.size() > 0, NS_ERROR_FAILURE);
  sbSimpleAutoLock lock(m_pBindParametersLock);

  nsresult rv = EnsureLastQueryParameter(aParamIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  CQueryParameter& qp = m_BindParameters[m_BindParameters.size()-1][aParamIndex];
  qp.type = UTF8STRING;
  qp.utf8StringValue = aValue;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::BindStringParameter(PRUint32 aParamIndex,
                                                  const nsAString &aValue)
{
  NS_ENSURE_TRUE(m_BindParameters.size() > 0, NS_ERROR_FAILURE);
  sbSimpleAutoLock lock(m_pBindParametersLock);
  nsresult rv = EnsureLastQueryParameter(aParamIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  CQueryParameter& qp = m_BindParameters[m_BindParameters.size()-1][aParamIndex];
  qp.type = STRING;
  qp.stringValue = aValue;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::BindDoubleParameter(PRUint32 aParamIndex,
                                                  double aValue)
{
  NS_ENSURE_TRUE(m_BindParameters.size() > 0, NS_ERROR_FAILURE);
  sbSimpleAutoLock lock(m_pBindParametersLock);
  nsresult rv = EnsureLastQueryParameter(aParamIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  CQueryParameter& qp = m_BindParameters[m_BindParameters.size()-1][aParamIndex];
  qp.type = DOUBLE;
  qp.doubleValue = aValue;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::BindInt32Parameter(PRUint32 aParamIndex,
                                                 PRInt32 aValue)
{
  NS_ENSURE_TRUE(m_BindParameters.size() > 0, NS_ERROR_FAILURE);
  sbSimpleAutoLock lock(m_pBindParametersLock);
  nsresult rv = EnsureLastQueryParameter(aParamIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  CQueryParameter& qp = m_BindParameters[m_BindParameters.size()-1][aParamIndex];
  qp.type = INTEGER32;
  qp.int32Value = aValue;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::BindInt64Parameter(PRUint32 aParamIndex,
                                                  PRInt64 aValue)
{
  NS_ENSURE_TRUE(m_BindParameters.size() > 0, NS_ERROR_FAILURE);
  sbSimpleAutoLock lock(m_pBindParametersLock);
  nsresult rv = EnsureLastQueryParameter(aParamIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  CQueryParameter& qp = m_BindParameters[m_BindParameters.size()-1][aParamIndex];
  qp.type = INTEGER64;
  qp.int64Value = aValue;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::BindNullParameter(PRUint32 aParamIndex)
{
  NS_ENSURE_TRUE(m_BindParameters.size() > 0, NS_ERROR_FAILURE);
  sbSimpleAutoLock lock(m_pBindParametersLock);
  nsresult rv = EnsureLastQueryParameter(aParamIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  CQueryParameter& qp = m_BindParameters[m_BindParameters.size()-1][aParamIndex];
  qp.type = ISNULL;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::EnsureLastQueryParameter(PRUint32 aParamIndex)
{
  NS_ENSURE_TRUE(m_BindParameters.size() > 0, NS_ERROR_FAILURE);
  if (aParamIndex >= m_BindParameters[m_BindParameters.size()-1].size()) {
    m_BindParameters[m_BindParameters.size()-1].resize(aParamIndex+1);
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
CDatabaseResult *CDatabaseQuery::GetResultObject()
{
  sbSimpleAutoLock lock(m_pQueryResultLock);

  //m_QueryResult->AddRef();
  return m_QueryResult;
} //GetResultObject


bindParameterArray_t* CDatabaseQuery::GetQueryParameters(PRInt32 aQueryIndex)
{
  bindParameterArray_t* retval = nsnull;
  sbSimpleAutoLock lock(m_pBindParametersLock);

  if(aQueryIndex < m_BindParameters.size()) {
    retval = new bindParameterArray_t(m_BindParameters[aQueryIndex]);
  } else {
    retval = new bindParameterArray_t;
  }

  return retval;
}

bindParameterArray_t* CDatabaseQuery::PopQueryParameters()
{
  bindParameterArray_t* retval = nsnull;
  sbSimpleAutoLock lock(m_pBindParametersLock);

  if(m_BindParameters.size()) {
    retval = new bindParameterArray_t(m_BindParameters[0]);
    m_BindParameters.pop_front();
  } else {
    retval = new bindParameterArray_t;
  }

  return retval;
}
