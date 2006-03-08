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
, m_AsyncQuery(PR_FALSE)
, m_IsAborting(PR_FALSE)
, m_IsExecuting(PR_FALSE)
, m_PersistentQuery(PR_FALSE)
, m_LastError(0)
, m_DatabaseGUID( NS_LITERAL_STRING("").get() )
, m_Engine(nsnull)
{
  m_QueryResult = new CDatabaseResult;
  m_QueryResult->AddRef();

  m_Engine = do_GetService(SONGBIRD_DATABASEENGINE_CONTRACTID);
} //ctor

//-----------------------------------------------------------------------------
CDatabaseQuery::~CDatabaseQuery()
{
  m_Engine->RemovePersistentQuery(this);
  
  NS_IF_RELEASE(m_QueryResult);
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
  *_retval = m_PersistentQuery;
  return NS_OK;
} //IsPersistentQuery

//-----------------------------------------------------------------------------
/* void AddPersistentQueryCallback (in sbIDatabasePersistentQueryCallback dbPersistCB); */
NS_IMETHODIMP CDatabaseQuery::AddSimpleQueryCallback(sbIDatabaseSimpleQueryCallback *dbPersistCB)
{
  if(dbPersistCB == NULL) return NS_ERROR_NULL_POINTER;

  sbCommon::CScopedLock lock(&m_PersistentCallbackListLock);

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
  if(dbPersistCB == NULL) return NS_ERROR_NULL_POINTER;

  sbCommon::CScopedLock lock(&m_PersistentCallbackListLock);

  persistentcallbacklist_t::iterator itCallback = m_PersistentCallbackList.begin();
  for(; itCallback != m_PersistentCallbackList.end(); itCallback++)
  {
    if((*itCallback) == dbPersistCB)
    {
      (*itCallback)->Release();
      m_PersistentCallbackList.erase(itCallback);
    }
  }

  return NS_OK;
} //RemovePersistentQueryCallback

//-----------------------------------------------------------------------------
/* void SetDatabaseGUID (in wstring dbGUID); */
NS_IMETHODIMP CDatabaseQuery::SetDatabaseGUID(const PRUnichar *dbGUID)
{
  nsresult ret = NS_ERROR_NULL_POINTER;
  
  if(dbGUID)
  {
    sbCommon::CScopedLock lock(&m_DatabaseGUIDLock);
    m_DatabaseGUID = dbGUID;
    ret = NS_OK;
  }

  return ret;
} //SetDatabaseGUID

//-----------------------------------------------------------------------------
/* wstring GetDatabaseGUID (); */
NS_IMETHODIMP CDatabaseQuery::GetDatabaseGUID(PRUnichar **_retval)
{
  sbCommon::CScopedLock lock(&m_DatabaseGUIDLock);
  size_t nLen = m_DatabaseGUID.length() + 1;

  *_retval = (PRUnichar *)nsMemory::Clone(m_DatabaseGUID.c_str(), nLen * sizeof(PRUnichar));
  if(*_retval == NULL) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
} //GetDatabaseGUID

//-----------------------------------------------------------------------------
/* void AddQuery (in wstring strQuery); */
NS_IMETHODIMP CDatabaseQuery::AddQuery(const PRUnichar *strQuery)
{
  sbCommon::CScopedLock lock(&m_DatabaseQueryListLock);
  m_DatabaseQueryList.push_back(strQuery);

  return NS_OK;
} //AddQuery

//-----------------------------------------------------------------------------
/* PRInt32 GetQueryCount (); */
NS_IMETHODIMP CDatabaseQuery::GetQueryCount(PRInt32 *_retval)
{
  sbCommon::CScopedLock lock(&m_DatabaseQueryListLock);
  *_retval = m_DatabaseQueryList.size();
  return NS_OK;
} //GetQueryCount

//-----------------------------------------------------------------------------
/* wstring GetQuery (in PRInt32 nIndex); */
NS_IMETHODIMP CDatabaseQuery::GetQuery(PRInt32 nIndex, PRUnichar **_retval)
{
  sbCommon::CScopedLock lock(&m_DatabaseQueryListLock);

  if(nIndex < m_DatabaseQueryList.size())
  {
    size_t nLen = m_DatabaseQueryList[nIndex].length() + 1;
    *_retval = (PRUnichar *)nsMemory::Clone(m_DatabaseQueryList[nIndex].c_str(), nLen * sizeof(PRUnichar));
    if(*_retval == NULL) return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
} //GetQuery

//-----------------------------------------------------------------------------
/* void ResetQuery (); */
NS_IMETHODIMP CDatabaseQuery::ResetQuery()
{
  {
    sbCommon::CScopedLock lock(&m_DatabaseQueryListLock);
    m_DatabaseQueryList.clear();
  }

  {
    sbCommon::CScopedLock lock(&m_QueryResultLock);
    m_QueryResult->ClearResultSet();
  }

  return NS_OK;
}

//-----------------------------------------------------------------------------
/* sbIDatabaseResult GetResultObject (); */
NS_IMETHODIMP CDatabaseQuery::GetResultObject(sbIDatabaseResult **_retval)
{
  sbCommon::CScopedLock lock(&m_QueryResultLock);

  *_retval = m_QueryResult;
  m_QueryResult->AddRef();

  return NS_OK;
} //GetResultObject

//-----------------------------------------------------------------------------
/* noscript sbIDatabaseResult GetResultObjectOrphan(); */
NS_IMETHODIMP CDatabaseQuery::GetResultObjectOrphan(sbIDatabaseResult **_retval)
{
  sbCommon::CScopedLock lock(&m_QueryResultLock);

  CDatabaseResult *pRes = new CDatabaseResult;
  
  pRes->m_ColumnNames = m_QueryResult->m_ColumnNames;
  pRes->m_RowCells = m_QueryResult->m_RowCells;
  pRes->AddRef();

  //Transfer references to the callee.
  *_retval = pRes;

  return NS_OK;
} //GetResultObjectOrphan

//-----------------------------------------------------------------------------
/* PRInt32 GetLastError (); */
NS_IMETHODIMP CDatabaseQuery::GetLastError(PRInt32 *_retval)
{
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
  m_DatabaseQueryHasCompleted.Reset();
  m_Engine->SubmitQuery(this, _retval);
  return NS_OK;
} //Execute

//-----------------------------------------------------------------------------
NS_IMETHODIMP CDatabaseQuery::WaitForCompletion(PRInt32 *_retval)
{
  *_retval = m_DatabaseQueryHasCompleted.Wait();
  //m_DatabaseQueryHasCompleted.Reset();

  return NS_OK;
} //WaitForCompletion

//-----------------------------------------------------------------------------
/* void AddCallback (in IDatabaseQueryCallback dbCallback); */
NS_IMETHODIMP CDatabaseQuery::AddCallback(sbIDatabaseQueryCallback *dbCallback)
{
  if(dbCallback == NULL) return NS_ERROR_NULL_POINTER;

  sbCommon::CScopedLock lock(&m_CallbackListLock);

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
  if(dbCallback == NULL) return NS_ERROR_NULL_POINTER;

  sbCommon::CScopedLock lock(&m_CallbackListLock);

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
  *_retval = m_IsExecuting;
  return NS_OK;
} //IsExecuting

//-----------------------------------------------------------------------------
/* PRInt32 CurrentQuery (); */
NS_IMETHODIMP CDatabaseQuery::CurrentQuery(PRInt32 *_retval)
{
  *_retval = m_CurrentQuery;
  return NS_OK;
} //CurrentQuery

//-----------------------------------------------------------------------------
/* void Abort (); */
NS_IMETHODIMP CDatabaseQuery::Abort(PRBool *_retval)
{
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
  sbCommon::CScopedLock lock(&m_QueryResultLock);

  m_QueryResult->AddRef();
  return m_QueryResult;
} //GetResultObject