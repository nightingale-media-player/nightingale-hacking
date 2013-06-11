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
#include <nsMemory.h>
#include <nsXPCOM.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>

#include <mozilla/ReentrantMonitor.h>
#include <nsNetUtil.h>

#include <nsIServiceManager.h>
#include <nsThreadUtils.h>
#include <nsSupportsArray.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIProxyObjectManager.h>

#include <sbProxiedComponentManager.h>
#include <sbLockUtils.h>

#ifdef DEBUG_locks
#include <nsPrintfCString.h>
#endif

#define DATABASEQUERY_DEFAULT_CALLBACK_SLOTS  (2)

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

NS_IMPL_THREADSAFE_ADDREF(CDatabaseQuery);
NS_IMPL_THREADSAFE_RELEASE(CDatabaseQuery);

NS_IMPL_CLASSINFO(CDatabaseQuery, NULL, nsIClassInfo::THREADSAFE, SONGBIRD_DATABASEQUERY_CID);

NS_INTERFACE_MAP_BEGIN(CDatabaseQuery)
  NS_IMPL_QUERY_CLASSINFO(CDatabaseQuery)
  NS_INTERFACE_MAP_ENTRY(sbIDatabaseQuery)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIDatabaseQuery)
NS_INTERFACE_MAP_END

//NS_IMPL_ISUPPORTS1(CDatabaseQuery,
//                   sbIDatabaseQuery);

NS_IMPL_CI_INTERFACE_GETTER1(CDatabaseQuery,
                             sbIDatabaseQuery);

NS_IMPL_THREADSAFE_CI(CDatabaseQuery)

//-----------------------------------------------------------------------------
CDatabaseQuery::CDatabaseQuery()
: m_pLock(PR_NewLock())
, m_IsAborting(PR_FALSE)
, m_IsExecuting(PR_FALSE)
, m_AsyncQuery(PR_FALSE)
, m_CurrentQuery((PRUint32)-1)
, m_LastError(0)
, m_pQueryRunningMonitor(nsnull)
, m_QueryHasCompleted(PR_FALSE)
, m_RollingLimit(0)
, m_RollingLimitColumnIndex(0)
, m_RollingLimitResult(0)
{
#ifdef PR_LOGGING
  if (!sDatabaseQueryLog)
    sDatabaseQueryLog = PR_NewLogModule("sbDatabaseQuery");
#endif

  m_CallbackList.Init(DATABASEQUERY_DEFAULT_CALLBACK_SLOTS);
} //ctor

//-----------------------------------------------------------------------------
CDatabaseQuery::~CDatabaseQuery()
{
  if (m_pLock)
    PR_DestroyLock(m_pLock);

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
  if (!NS_IsMainThread()) {
    NS_WARNING("CDatabaseQuery::GetDatabaseLocation is main thread only, "
               "since it constructs an nsStandardURL object");
    return NS_ERROR_FAILURE;
  }

  nsresult rv = NS_OK;
  *aDatabaseLocation = nsnull;

  sbSimpleAutoLock lock(m_pLock);
  if(!m_LocationURIString.IsEmpty())
  {
    rv = NS_NewURI(aDatabaseLocation, m_LocationURIString);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
} //GetDatabaseLocation

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseQuery::SetDatabaseLocation(nsIURI * aDatabaseLocation)
{
  NS_ENSURE_ARG_POINTER(aDatabaseLocation);
  nsresult rv = NS_ERROR_UNEXPECTED;
  
  PRBool isFile = PR_FALSE;
  if(NS_SUCCEEDED(aDatabaseLocation->SchemeIs("file", &isFile)) &&
     isFile)
  {
    nsCString spec;
    rv = aDatabaseLocation->GetSpec(spec);
    NS_ENSURE_SUCCESS(rv, rv);

    sbSimpleAutoLock lock(m_pLock);
    m_LocationURIString = spec;
  }
  else
  {
    //We only support database files stored locally, for now.
    rv = NS_ERROR_NOT_IMPLEMENTED;
  }

  return rv;
} //SetDatabaseLocation

//-----------------------------------------------------------------------------
nsresult CDatabaseQuery::GetDatabaseLocation(nsACString& aURISpec)
{
  sbSimpleAutoLock lock(m_pLock);
  aURISpec.Assign(m_LocationURIString);
  return NS_OK;
} //GetDatabaseLocation

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

  nsresult rv = do_GetProxyForObject(NS_PROXY_TO_CURRENT_THREAD,
                                     NS_GET_IID(sbIDatabaseSimpleQueryCallback),
                                     dbPersistCB,
                                     NS_PROXY_ASYNC | NS_PROXY_ALWAYS,
                                     getter_AddRefs(proxiedCallback));
  NS_ENSURE_SUCCESS(rv, rv);

  m_CallbackList.Put(dbPersistCB, proxiedCallback);

  return NS_OK;
} //AddPersistentQueryCallback

//-----------------------------------------------------------------------------
/* void RemoveSimpleQueryCallback (in sbIDatabasePersistentQueryCallback dbPersistCB); */
NS_IMETHODIMP CDatabaseQuery::RemoveSimpleQueryCallback(sbIDatabaseSimpleQueryCallback *dbPersistCB)
{
  NS_ENSURE_ARG_POINTER(dbPersistCB);

  m_CallbackList.Remove(dbPersistCB);

  return NS_OK;
} //RemoveSimpleQueryCallback

//-----------------------------------------------------------------------------
/* void SetDatabaseGUID (in wstring dbGUID); */
NS_IMETHODIMP CDatabaseQuery::SetDatabaseGUID(const nsAString &dbGUID)
{
  sbSimpleAutoLock lock(m_pLock);
  m_DatabaseGUID = dbGUID;
  return NS_OK;
} //SetDatabaseGUID

//-----------------------------------------------------------------------------
/* wstring GetDatabaseGUID (); */
NS_IMETHODIMP CDatabaseQuery::GetDatabaseGUID(nsAString &_retval)
{
  sbSimpleAutoLock lock(m_pLock);
  _retval = m_DatabaseGUID;
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

  sbSimpleAutoLock lock(m_pLock);
  m_DatabaseQueryList.push_back(preparedStatement);

  // Also add an element to the bind parameters array
  m_BindParameters.resize(m_BindParameters.size()+1);
  
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRInt32 GetQueryCount (); */
NS_IMETHODIMP CDatabaseQuery::GetQueryCount(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  sbSimpleAutoLock lock(m_pLock);
  *_retval = (PRInt32)m_DatabaseQueryList.size();
  return NS_OK;
} //GetQueryCount

//-----------------------------------------------------------------------------
/* sbIDatabasePreparedStatement GetQuery (in PRInt32 nIndex); */
NS_IMETHODIMP CDatabaseQuery::GetQuery(PRUint32 nIndex, sbIDatabasePreparedStatement **_retval)
{
  nsresult rv = NS_OK;
  NS_ENSURE_ARG_POINTER(_retval);

  sbSimpleAutoLock lock(m_pLock);

  if((PRUint32)nIndex < m_DatabaseQueryList.size()) {
    NS_ADDREF(*_retval = m_DatabaseQueryList[nIndex]);
  }
  else {
    rv = NS_ERROR_ILLEGAL_VALUE;
  }

  return rv;
} //GetQuery

//-----------------------------------------------------------------------------
/* sbIDatabasePreparedStatement PopQuery(); */
nsresult CDatabaseQuery::PopQuery(sbIDatabasePreparedStatement **_retval)
{
  nsresult rv = NS_OK;
  
  NS_ENSURE_ARG_POINTER(_retval);

  sbSimpleAutoLock lock(m_pLock);

  if(m_DatabaseQueryList.size() > 0) {
    NS_ADDREF(*_retval = m_DatabaseQueryList[0]);
    m_DatabaseQueryList.pop_front();
  }
  else {
    rv = NS_ERROR_ILLEGAL_VALUE;
  }
  
  return rv;
} //PopQuery

//-----------------------------------------------------------------------------
/* void ResetQuery (); */
NS_IMETHODIMP CDatabaseQuery::ResetQuery()
{
  // Make sure we're not running
  mozilla::ReentrantMonitorAutoEnter monQueryRunning(m_pQueryRunningMonitor);

  // These should be in sync, but just in case
  NS_ASSERTION( !m_IsExecuting, "Resetting a query that is executing!!!!!");
  NS_ASSERTION( m_QueryHasCompleted || m_CurrentQuery == PR_UINT32_MAX, 
    "Resetting a query that is not complete!!!!!");

  // Make sure no-one is touching m_IsExecuting
  sbSimpleAutoLock lock(m_pLock);

  m_DatabaseQueryList.clear();

  // Also clear parameters array
  m_BindParameters.clear();

  // Done with result set
  m_QueryResult = nsnull;

  m_RollingLimit = 0;
  m_RollingLimitColumnIndex = 0;
  m_RollingLimitResult = 0;

  return NS_OK;
}

//-----------------------------------------------------------------------------
/* sbIDatabaseResult GetResultObject (); */
NS_IMETHODIMP CDatabaseQuery::GetResultObject(sbIDatabaseResult **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  sbSimpleAutoLock lock(m_pLock);
  NS_IF_ADDREF(*_retval = m_QueryResult);

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
    mozilla::ReentrantMonitorAutoEnter mon(m_pQueryRunningMonitor);
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
    mozilla::ReentrantMonitorAutoEnter mon(m_pQueryRunningMonitor);
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
    mozilla::ReentrantMonitorAutoEnter mon(m_pQueryRunningMonitor);
    while (!m_QueryHasCompleted) {
      mon.Wait();
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
  
  sbSimpleAutoLock lock(m_pLock);
  *_retval = m_IsExecuting;

  return NS_OK;
} //IsExecuting

//-----------------------------------------------------------------------------
/* PRInt32 CurrentQuery (); */
NS_IMETHODIMP CDatabaseQuery::CurrentQuery(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  sbSimpleAutoLock lock(m_pLock);
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

    {
      sbSimpleAutoLock lock(m_pLock);
      m_IsAborting = PR_TRUE;
    }

    WaitForCompletion(&nWaitResult);

    *_retval = PR_TRUE;
  }

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::GetRollingLimit(PRUint64 *aRollingLimit)
{
  NS_ENSURE_ARG_POINTER(aRollingLimit);

  sbSimpleAutoLock lock(m_pLock);
  *aRollingLimit = m_RollingLimit;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::SetRollingLimit(PRUint64 aRollingLimit)
{
  sbSimpleAutoLock lock(m_pLock);
  m_RollingLimit = aRollingLimit;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::GetRollingLimitColumnIndex(PRUint32 *aRollingLimitColumnIndex)
{
  NS_ENSURE_ARG_POINTER(aRollingLimitColumnIndex);

  sbSimpleAutoLock lock(m_pLock);
  *aRollingLimitColumnIndex = m_RollingLimitColumnIndex;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::SetRollingLimitColumnIndex(PRUint32 aRollingLimitColumnIndex)
{
  sbSimpleAutoLock lock(m_pLock);
  m_RollingLimitColumnIndex = aRollingLimitColumnIndex;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::GetRollingLimitResult(PRUint32 *aRollingLimitResult)
{
  NS_ENSURE_ARG_POINTER(aRollingLimitResult);

  sbSimpleAutoLock lock(m_pLock);
  *aRollingLimitResult = m_RollingLimitResult;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::SetRollingLimitResult(PRUint32 aRollingLimitResult)
{
  sbSimpleAutoLock lock(m_pLock);
  m_RollingLimitResult = aRollingLimitResult;

  return NS_OK;
}

NS_IMETHODIMP CDatabaseQuery::BindUTF8StringParameter(PRUint32 aParamIndex,
                                                      const nsACString &aValue)
{
  NS_ENSURE_TRUE(m_BindParameters.size() > 0, NS_ERROR_FAILURE);
  sbSimpleAutoLock lock(m_pLock);

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
  sbSimpleAutoLock lock(m_pLock);
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
  sbSimpleAutoLock lock(m_pLock);
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
  sbSimpleAutoLock lock(m_pLock);
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
  sbSimpleAutoLock lock(m_pLock);
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
  sbSimpleAutoLock lock(m_pLock);
  nsresult rv = EnsureLastQueryParameter(aParamIndex);
  NS_ENSURE_SUCCESS(rv, rv);
  CQueryParameter& qp = m_BindParameters[m_BindParameters.size()-1][aParamIndex];
  qp.type = ISNULL;

  return NS_OK;
}

nsresult CDatabaseQuery::EnsureLastQueryParameter(PRUint32 aParamIndex)
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
  sbSimpleAutoLock lock(m_pLock);
  return m_QueryResult;
} //GetResultObject

void
CDatabaseQuery::SetResultObject(CDatabaseResult *aResultObject)
{
  sbSimpleAutoLock lock(m_pLock);
  m_QueryResult = aResultObject;
}

bindParameterArray_t* CDatabaseQuery::GetQueryParameters(PRUint32 aQueryIndex)
{
  bindParameterArray_t* retval = nsnull;
  sbSimpleAutoLock lock(m_pLock);

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
  sbSimpleAutoLock lock(m_pLock);

  if(m_BindParameters.size()) {
    retval = new bindParameterArray_t(m_BindParameters[0]);
    m_BindParameters.pop_front();
  } else {
    retval = new bindParameterArray_t;
  }

  return retval;
}
