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
 * \file DatabaseEngine.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include "Common.h"
#include "DatabaseEngine.h"

#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsIFile.h>
#include <xpcom/nsILocalFile.h>
#include <string/nsString.h>
#include <string/nsStringAPI.h>
#include <xpcom/nsISimpleEnumerator.h>
#include <xpcom/nsDirectoryServiceDefs.h>
#include <unicharutil/nsUnicharUtils.h>
 
#include <vector>
#include <prmem.h>

#if defined(_WIN32)
  #include <direct.h>
#else
// on UNIX, strnicmp is called strncasecmp
#define strnicmp strncasecmp
#endif

#define USE_TIMING                    0
#define USE_SQLITE_FULL_DISK_CACHING  1
#define SQLITE_MAX_RETRIES            66
//#define HARD_SANITY_CHECK             0

const PRUnichar* ALL_TOKEN = NS_LITERAL_STRING("*").get();
const PRUnichar* DB_GUID_TOKEN = NS_LITERAL_STRING("%db_guid%").get();
const PRInt32 DB_GUID_TOKEN_LEN = 9;

// FUNCTIONS ==================================================================
int SQLiteAuthorizer(void *pData, int nOp, const char *pArgA, const char *pArgB, const char *pDBName, const char *pTrigger)
{
  int ret = SQLITE_OK;

  CDatabaseQuery *pQuery = reinterpret_cast<CDatabaseQuery *>(pData);

  if(pQuery)
  {
    switch(nOp)
    {
      case SQLITE_CREATE_INDEX:
      {
      }
      break;

      case SQLITE_CREATE_TABLE:
      {
      }
      break;

      case SQLITE_CREATE_TEMP_INDEX:
      {
      }
      break;

      case SQLITE_CREATE_TEMP_TABLE:
      {
      }
      break;

      case SQLITE_CREATE_TEMP_TRIGGER:
      {
      }
      break;

      case SQLITE_CREATE_TEMP_VIEW:
      {
      }
      break;

      case SQLITE_CREATE_TRIGGER:
      {
      }
      break;

      case SQLITE_CREATE_VIEW:
      {
      }
      break;

      case SQLITE_DELETE:
      {
        if(pArgA && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            sbCommon::CScopedLock lock(&pQuery->m_ModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_DROP_INDEX:
      {
      }
      break;

      case SQLITE_DROP_TABLE:
      {
        if(pArgA && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            sbCommon::CScopedLock lock(&pQuery->m_ModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_DROP_TEMP_INDEX:
      {
      }
      break;

      case SQLITE_DROP_TEMP_TABLE:
      {
        if(pArgA && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            sbCommon::CScopedLock lock(&pQuery->m_ModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_DROP_TEMP_TRIGGER:
      {
      }
      break;

      case SQLITE_DROP_TEMP_VIEW:
      {
      }
      break;

      case SQLITE_DROP_TRIGGER:
      {
      }
      break;

      case SQLITE_DROP_VIEW:
      {
      }
      break;

      case SQLITE_INSERT:
      {
        if(pArgA && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;

          {
            sbCommon::CScopedLock lock(&pQuery->m_ModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_PRAGMA:
      {
      }
      break;

      case SQLITE_READ:
      {
        //If there's a table name but no column name, it means we're doing the initial SELECT on the table.
        //This is a good time to check if the query is supposed to be persistent and insert it
        //into the list of persistent queries.
        if(pQuery->m_PersistentQuery && strnicmp("sqlite_", pArgA, 7))
        {
          pQuery->m_Engine->AddPersistentQuery(pQuery, pArgA);
        }
      }
      break;

      case SQLITE_SELECT:
      {
      }
      break;

      case SQLITE_TRANSACTION:
      {
      }
      break;

      case SQLITE_UPDATE:
      {
        if(pArgA && pArgB )
        {
          pQuery->m_HasChangedDataOfPersistQuery = PR_TRUE;
          
          {
            sbCommon::CScopedLock lock(&pQuery->m_ModifiedTablesLock);
            pQuery->m_ModifiedTables.insert( pArgA );
          }
        }
      }
      break;

      case SQLITE_ATTACH:
      {
      }
      break;

      case SQLITE_DETACH:
      {
      }
      break;

    }
  }

  return ret;
} //SQLiteAuthorizer

NS_IMPL_THREADSAFE_ISUPPORTS1(CDatabaseEngine, sbIDatabaseEngine)

// CLASSES ====================================================================
//-----------------------------------------------------------------------------
CDatabaseEngine::CDatabaseEngine()
{
  m_QueryProcessorThreadPool.Create(reinterpret_cast<sbCommon::threadfunction_t *>(CDatabaseEngine::QueryProcessor), this, 4);
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CDatabaseEngine::~CDatabaseEngine()
{
  m_QueryProcessorThreadPool.SetTerminateAll(PR_TRUE);
  m_QueryQueueHasItem.Set();
  m_QueryProcessorThreadPool.TerminateAll();

  ClearAllDBLocks();
  CloseAllDB();

  m_DatabaseLocks.clear();
  m_Databases.clear();
} //dtor

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::OpenDB(PRUnichar *dbGUID)
{
  sbCommon::CScopedLock lock(&m_DatabasesLock);

  sqlite3 *pDB = nsnull;
  std::prustring strDBGUID(dbGUID);
  std::prustring strFilename(dbGUID);

  strFilename += L".db";

  /*
  nsCOMPtr<nsIFile> cwd;
  NS_GetSpecialDirectory(NS_OS_CURRENT_WORKING_DIR, getter_AddRefs(cwd));
  
  nsCOMPtr<nsIFile> old_cwd;
  nsresult rv = cwd->Clone(getter_AddRefs(old_cwd));
  if (NS_FAILED(rv))
    return rv;
  
  // Create the db subfolder.  If it already exists, I don't care.
  cwd->Append( NS_LITERAL_STRING("db") );

  PRBool exists;
  cwd->Exists( &exists );
  if ( ! exists )
  {
    rv = cwd->Create( nsIFile::DIRECTORY_TYPE, 0777 );
  }

  // Set that to be our working directory (because sqlite is given a local file name)
  //rv = directory->Set(NS_OS_CURRENT_WORKING_DIR, cwd );

#if defined(_WIN32)
  _chdir("db/");
#else
  chdir("db/");
#endif
*/

  /*
  {
    // If there's a journal file, we suck.  Nuke it.
    nsString strDBJournal(dbGUID);
    strDBJournal += NS_LITERAL_STRING("-journal");

    rv = cwd->Append( strDBJournal );
    if (NS_FAILED(rv))
      return rv;

    rv = cwd->Remove( PR_FALSE );
  }
  */

  // Kick sqlite in the pants
  PRInt32 ret = sqlite3_open16(strFilename.c_str(), &pDB);

  // Restore the working directory to where it once were
  //rv = directory->Set(NS_OS_CURRENT_WORKING_DIR, old_cwd );

/*
#if defined(_WIN32)
  _chdir("..");
#else
  chdir("..");
#endif
*/

  // Remember what we just loaded
  if(ret == SQLITE_OK)
  {
    m_Databases.insert(std::make_pair(strDBGUID, pDB));

#if defined(USE_SQLITE_FULL_DISK_CACHING)
    sqlite3_busy_timeout(pDB, 5000);

    char *strErr = nsnull;
    sqlite3_exec(pDB, "PRAGMA synchronous = 0", nsnull, nsnull, &strErr);
    if(strErr) sqlite3_free(strErr);
#endif
  }

  return ret;
} //OpenDB

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::CloseDB(PRUnichar *dbGUID)
{
  PRInt32 ret = 0;
  sbCommon::CScopedLock lock(&m_DatabasesLock);

  databasemap_t::iterator itDatabases = m_Databases.find(dbGUID);

  if(itDatabases != m_Databases.end())
  {
    ret = sqlite3_close(itDatabases->second);
    m_Databases.erase(itDatabases);
  }
 
  return ret;
} //CloseDB

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::DropDB(PRUnichar *dbGUID)
{
  return 0;
} //DropDB

//-----------------------------------------------------------------------------
/* [noscript] PRInt32 SubmitQuery (in CDatabaseQueryPtr dbQuery); */
NS_IMETHODIMP CDatabaseEngine::SubmitQuery(CDatabaseQuery * dbQuery, PRInt32 *_retval)
{
  *_retval = SubmitQueryPrivate(dbQuery);
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::SubmitQueryPrivate(CDatabaseQuery *dbQuery)
{
  PRInt32 ret = 0;

  if(!dbQuery) return 1;

  {
    m_QueryQueueLock.Lock();

    dbQuery->AddRef();
    dbQuery->m_IsExecuting = PR_TRUE;

    m_QueryQueue.push_back(dbQuery);
    m_QueryQueueHasItem.Set();

    m_QueryQueueLock.Unlock();
  }

  PRBool bAsyncQuery = PR_FALSE;
  dbQuery->IsAyncQuery(&bAsyncQuery);

  if(!bAsyncQuery)
  {
    dbQuery->WaitForCompletion(&ret);

    if(ret == SB_WAIT_SUCCESS)
    {
      dbQuery->GetLastError(&ret);
    }
  }

  return ret;
} //SubmitQuery

//-----------------------------------------------------------------------------
/* [noscript] void AddPersistentQuery (in CDatabaseQueryPtr dbQuery, in stlCStringRef strTableName); */
NS_IMETHODIMP CDatabaseEngine::AddPersistentQuery(CDatabaseQuery * dbQuery, const std::string & strTableName)
{
  AddPersistentQueryPrivate(dbQuery, strTableName);
  return NS_OK;
}

//-----------------------------------------------------------------------------
void CDatabaseEngine::AddPersistentQueryPrivate(CDatabaseQuery *pQuery, const std::string &strTableName)
{
  sbCommon::CScopedLock lock(&m_PersistentQueriesLock);
    
  PRUnichar *strDBGUID = nsnull;
  pQuery->GetDatabaseGUID(&strDBGUID);

  querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.find(strDBGUID);
  if(itPersistentQueries != m_PersistentQueries.end())
  {
    tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(strTableName);
    if(itTableQuery != itPersistentQueries->second.end())
    {
      querylist_t::iterator itQueries = itTableQuery->second.begin();
      for( ; itQueries != itTableQuery->second.end(); itQueries++)
      {
        if((*itQueries) == pQuery)
          return;
      }

      NS_IF_ADDREF(pQuery);
      itTableQuery->second.insert(itTableQuery->second.end(), pQuery);
    }
    else
    {
      NS_IF_ADDREF(pQuery);

      querylist_t queryList;
      queryList.push_back(pQuery);

      itPersistentQueries->second.insert(std::make_pair<std::string, querylist_t>(strTableName, queryList));
    }
  }
  else
  {
    NS_IF_ADDREF(pQuery);

    querylist_t queryList;
    queryList.push_back(pQuery);

    tablepersistmap_t tableMap;
    tableMap.insert(std::make_pair<std::string, querylist_t>(strTableName, queryList));

    m_PersistentQueries.insert(std::make_pair<std::prustring, tablepersistmap_t>(strDBGUID, tableMap));
  }

  {
    sbCommon::CScopedLock lock(&pQuery->m_PersistentQueryTableLock);
    pQuery->m_PersistentQueryTable = strTableName;
    pQuery->m_IsPersistentQueryRegistered = PR_TRUE;
  }

  PR_Free(strDBGUID);

} //AddPersistentQuery

//-----------------------------------------------------------------------------
/* [noscript] void RemovePersistentQuery (in CDatabaseQueryPtr dbQuery); */
NS_IMETHODIMP CDatabaseEngine::RemovePersistentQuery(CDatabaseQuery * dbQuery)
{
  RemovePersistentQueryPrivate(dbQuery);
  return NS_OK;
}

//-----------------------------------------------------------------------------
void CDatabaseEngine::RemovePersistentQueryPrivate(CDatabaseQuery *pQuery)
{
  if(!pQuery->m_IsPersistentQueryRegistered)
    return;

  sbCommon::CScopedLock lock(&m_PersistentQueriesLock);

  std::string strTableName("");
  PRUnichar *strDBGUID = nsnull;
  pQuery->GetDatabaseGUID(&strDBGUID);

  {
    sbCommon::CScopedLock lock(&pQuery->m_PersistentQueryTableLock);
    strTableName = pQuery->m_PersistentQueryTable;
  }

  querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.find(strDBGUID);
  if(itPersistentQueries != m_PersistentQueries.end())
  {
    tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(strTableName);
    if(itTableQuery != itPersistentQueries->second.end())
    {
      querylist_t::iterator itQueries = itTableQuery->second.begin();
      for( ; itQueries != itTableQuery->second.end(); itQueries++)
      {
        if((*itQueries) == pQuery)
        {
          NS_RELEASE((*itQueries));
          itTableQuery->second.erase(itQueries);

          return;
        }
      }
    }
  }

  PR_Free(strDBGUID);

  return;
} //RemovePersistentQuery

//-----------------------------------------------------------------------------
void CDatabaseEngine::LockDatabase(sqlite3 *pDB)
{
  m_DatabaseLocksLock.Lock();
  databaselockmap_t::iterator itLock = m_DatabaseLocks.find(pDB);

  if(itLock != m_DatabaseLocks.end())
  {
    sbCommon::CLock *lock = itLock->second;
    m_DatabaseLocksLock.Unlock();
    
    lock->Lock();
  }
  else
  {
    sbCommon::CLock *lock = new sbCommon::CLock();
    m_DatabaseLocks.insert(std::make_pair<sqlite3 *, sbCommon::CLock *>(pDB, lock));
    m_DatabaseLocksLock.Unlock();

    lock->Lock();
  }

  return;
} //LockDatabase

//-----------------------------------------------------------------------------
void CDatabaseEngine::UnlockDatabase(sqlite3 *pDB)
{
  m_DatabaseLocksLock.Lock();
  databaselockmap_t::iterator itLock = m_DatabaseLocks.find(pDB);

  if(itLock != m_DatabaseLocks.end())
  {
    sbCommon::CLock *lock = itLock->second;
    m_DatabaseLocksLock.Unlock();

    lock->Unlock();
  }

  return;
} //UnlockDatabase

//-----------------------------------------------------------------------------
void CDatabaseEngine::ClearAllDBLocks()
{
  m_DatabaseLocksLock.Lock();
  databaselockmap_t::iterator itLock = m_DatabaseLocks.begin();
  databaselockmap_t::iterator itEnd = m_DatabaseLocks.end();

  for(; itLock != itEnd; itLock++)
  {
    if(itLock->second)
      delete itLock->second;
  }
  
  m_DatabaseLocks.clear();
  m_DatabaseLocksLock.Unlock();
} //ClearAllDBLocks

//-----------------------------------------------------------------------------
void CDatabaseEngine::CloseAllDB()
{
  PRInt32 ret = 0;
  sbCommon::CScopedLock lock(&m_DatabasesLock);

  databasemap_t::iterator itDatabases = m_Databases.begin();

  if(itDatabases != m_Databases.end())
  {
    ret = sqlite3_close(itDatabases->second);
    m_Databases.erase(itDatabases);
  }

  return;
} //CloseAllDB

//-----------------------------------------------------------------------------
sqlite3 *CDatabaseEngine::GetDBByGUID(PRUnichar *dbGUID, PRBool bCreateIfNotOpen /*= PR_FALSE*/)
{
  sqlite3 *pRet = nsnull;

  pRet = FindDBByGUID(dbGUID);

  if(pRet == nsnull)
  {
    int ret = OpenDB(dbGUID);
    if(ret == SQLITE_OK)
    {
      pRet = FindDBByGUID(dbGUID);
    }
  }

  return pRet;
} //GetDBByGUID

//-----------------------------------------------------------------------------
PRInt32 CDatabaseEngine::GetDBGUIDList(std::vector<std::prustring> &vGUIDList)
{
  PRInt32 nRet = 0;
  PRBool bFlag = PR_FALSE;
  nsresult rv;

  nsCOMPtr<nsIServiceManager> svcMgr;
  rv = NS_GetServiceManager(getter_AddRefs(svcMgr));

  nsCOMPtr<nsIProperties> directory;
  rv = svcMgr->GetServiceByContractID("@mozilla.org/file/directory_service;1",
    NS_GET_IID(nsIProperties),
    getter_AddRefs(directory));

  // Get it once so we can restore it
  nsCOMPtr<nsILocalFile> pDBDirectory = do_GetService("@mozilla.org/file/local;1");
  rv = directory->Get(NS_OS_CURRENT_WORKING_DIR, NS_GET_IID(nsIFile), getter_AddRefs(pDBDirectory));

  if(NS_FAILED(rv)) return 0;

  rv = pDBDirectory->IsDirectory(&bFlag);

  if(bFlag)
  {
    nsISimpleEnumerator *pDirEntries = nsnull;
    pDBDirectory->GetDirectoryEntries(&pDirEntries);

    if(pDirEntries)
    {
      PRBool bHasMore = PR_FALSE;

      for(;;)
      {
        if(pDirEntries)
          pDirEntries->HasMoreElements(&bHasMore);

        if(bHasMore)
        {

          nsCOMPtr<nsISupports> pDirEntry;
          pDirEntries->GetNext(getter_AddRefs(pDirEntry));

          if(pDirEntry)
          {
            nsIID nsIFileIID = NS_IFILE_IID;
            nsCOMPtr<nsIFile> pEntry;
            pDirEntry->QueryInterface(nsIFileIID, getter_AddRefs(pEntry));

            if(pEntry)
            {
              PRBool bIsFile = PR_FALSE;
              pEntry->IsFile(&bIsFile);

              if(bIsFile)
              {
                nsString strLeaf;
                pEntry->GetLeafName(strLeaf);

                nsString::const_iterator itStrStart, itStart, itEnd;
                strLeaf.BeginReading(itStrStart);
                strLeaf.BeginReading(itStart);
                strLeaf.EndReading(itEnd);

                PRBool bFound = FindInReadable(NS_LITERAL_STRING(".db"), itStart, itEnd, nsCaseInsensitiveStringComparator());
                
                if(bFound)
                {
                  size_t nCutPos = Distance(itStrStart, itStart);
                  size_t nCutLen = Distance(itStart, itEnd);

                  strLeaf.Cut(nCutPos, nCutLen);
                  vGUIDList.push_back(strLeaf.get());

                  ++nRet;
                }
              }
            }
          }
        }
        else
        {
          break;
        }
      }
    }
  }

  return nRet;
} //GetDBGUIDList

//-----------------------------------------------------------------------------
sqlite3 *CDatabaseEngine::FindDBByGUID(PRUnichar *dbGUID)
{
  sqlite3 *pRet = nsnull;

  {
    sbCommon::CScopedLock lock(&m_DatabasesLock);
    databasemap_t::const_iterator itDatabases = m_Databases.find(dbGUID);
    if(itDatabases != m_Databases.end())
    {
      pRet = itDatabases->second;
    }
  }

  return pRet;
} //FindDBByGUID

//-----------------------------------------------------------------------------
/*static*/ void PR_CALLBACK CDatabaseEngine::QueryProcessor(void *pData)
{
  sbCommon::threaddata_t *pThreadData = static_cast<sbCommon::threaddata_t *>(pData);
  sbCommon::CThread *t = pThreadData->pThread;
  CDatabaseEngine *p = static_cast<CDatabaseEngine *>(pThreadData->pData);

#if USE_TIMING
  FILETIME a_start, a_time;
  PRUnichar str[255];
#endif

  CDatabaseQuery *pQuery = nsnull;
  while(!t->IsTerminating())
  {
    pQuery = nsnull;

    PRInt32 ret = SB_WAIT_SUCCESS;
    ret = p->m_QueryQueueHasItem.Wait();

    switch(ret)
    {
      case SB_WAIT_SUCCESS:
      {
        sbCommon::CScopedLock lock(&p->m_QueryQueueLock);
        size_t nQueueSize = p->m_QueryQueue.size();

        if(t->IsTerminating())
        {
          sqlite3_thread_cleanup();
          return;
        }

        if(nQueueSize == 0)
        {
          if(nQueueSize == 0)
          {
            p->m_QueryQueueHasItem.Reset();
          }

          continue;
        }

        pQuery = p->m_QueryQueue.front();

        if(pQuery == nsnull)
        {
          if(nQueueSize == 0)
          {
            p->m_QueryQueueHasItem.Reset();
          }

          continue;
        }

        p->m_QueryQueue.pop_front();
        
        if(nQueueSize == 0)
        {
          p->m_QueryQueueHasItem.Reset();
        }
      }
      break;

      case SB_WAIT_FAILURE:
      case SB_WAIT_TIMEOUT:
      {
        if(t->IsTerminating())
        {
          sqlite3_thread_cleanup();
          return;
        }

        continue;
      }
      break;
      
      default:
        continue;
    }

#if USE_TIMING
    ::GetSystemTimeAsFileTime( &a_start );
#endif

    if(!pQuery)
    {
      continue;
    }

    PRBool bAllDB = PR_FALSE;
    sqlite3 *pDB = nsnull;
    sqlite3 *pSecondDB = nsnull;
    PRUnichar *pGUID = nsnull;
    pQuery->GetDatabaseGUID(&pGUID);
    
    // create |nsDependentString|s wrapping PRUnichar* buffers so we can
    // do fancy things in a platform independent way.
    nsDependentString ALL_TOKEN_nsds(ALL_TOKEN);
    nsDependentString pGUID_nsds(pGUID);
    
    if(ALL_TOKEN_nsds.Equals(pGUID_nsds, nsCaseInsensitiveStringComparator()))
    {
      bAllDB = PR_TRUE;
    }
    else
    {
      pDB = p->GetDBByGUID(pGUID, PR_TRUE);
    }

    PR_Free(pGUID);

    if(!pDB && !bAllDB)
    {
      //Some bad error we'll talk about later.
      pQuery->SetLastError(SQLITE_ERROR);
    }
    else
    {
      std::vector<std::prustring> vDBList;
      PRInt32 nNumDB = 1;
      PRInt32 nQueryCount = 0;
      PRBool bFirstRow = PR_TRUE;

      //Default return error.
      pQuery->SetLastError(SQLITE_ERROR);
      pQuery->GetQueryCount(&nQueryCount);

      if(bAllDB)
        nNumDB = p->GetDBGUIDList(vDBList);

      //pQuery->m_QueryResultLock.Lock();

      for(PRInt32 i = 0; i < nQueryCount && !pQuery->m_IsAborting; i++)
      {

        int retDB = 0;
        sqlite3_stmt *pStmt = nsnull;

        PRUnichar *pzQuery = nsnull;
        const void *pzTail = nsnull;
        std::prustring strQuery;

        pQuery->GetQuery(i, &pzQuery);

        strQuery = pzQuery;
        PR_Free(pzQuery);
        
        static nsString strAttachToken(L"ATTACH DATABASE \"");

        size_t nPos = strQuery.find(strAttachToken.get());
        if(nPos != strQuery.npos)
        {
          static nsString strStartToken(L"AS \"");
          static nsString strEndToken(L"\"");

          nPos += strAttachToken.Length();
          nPos = strQuery.find(strStartToken.get(), nPos);

          nPos += strStartToken.Length();
          size_t nEndPos = strQuery.find(strEndToken.get(), nPos);
          
          std::prustring strSecondDBGUID = strQuery.substr(nPos, nEndPos - nPos);
          pSecondDB = p->GetDBByGUID(const_cast<PRUnichar *>(strSecondDBGUID.c_str()), PR_TRUE);

          if(pSecondDB)
          {
            p->LockDatabase(pSecondDB);
          }
        }

        for(PRInt32 j = 0; j < nNumDB; j++)
        {
          if(!bAllDB)
          {
            p->LockDatabase(pDB);
            retDB = sqlite3_set_authorizer(pDB, SQLiteAuthorizer, pQuery);

#if defined(HARD_SANITY_CHECK)
            if(retDB != SQLITE_OK)
            {
              PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
              OutputDebugStringW(szErr);
              OutputDebugStringW(L"\n");
            }
#endif
          }
          else
          {
            pDB = p->GetDBByGUID(const_cast<PRUnichar *>(vDBList[j].c_str()), PR_TRUE);
            
            //Just in case there was some kind of bad error while attempting to get the database or create it.
            if ( !pDB ) 
              continue;
            
            p->LockDatabase(pDB);
            retDB = sqlite3_set_authorizer(pDB, SQLiteAuthorizer, pQuery);

#if defined(HARD_SANITY_CHECK)
            if(retDB != SQLITE_OK)
            {
              PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
              OutputDebugStringW(szErr);
              OutputDebugStringW(L"\n");
            }
#endif
          }

          retDB = sqlite3_prepare16(pDB, strQuery.c_str(), (int)strQuery.length() * sizeof(PRUnichar), &pStmt, &pzTail);
          pQuery->m_CurrentQuery = i;

          if(retDB != SQLITE_OK)
          {
            //Some bad error we'll talk about later.
            pQuery->SetLastError(SQLITE_ERROR);

#if defined(HARD_SANITY_CHECK)
            if(retDB != SQLITE_OK)
            {
              PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
              OutputDebugStringW(szErr);
              OutputDebugStringW(L"\n");
              //__asm int 3;
            }
#endif

          }
          else
          {
            PRInt32 nRetryCount = 0;

            do
            {
              retDB = sqlite3_step(pStmt);

              switch(retDB)
              {
                case SQLITE_ROW: 
                {
                  CDatabaseResult *pRes = pQuery->GetResultObject();
                  int nCount = sqlite3_column_count(pStmt);

                  if(bFirstRow)
                  {
                    bFirstRow = PR_FALSE;                                       
                    pRes->ClearResultSet();

                    std::vector<std::prustring> vColumnNames;
                    vColumnNames.reserve(nCount);

                    for(int i = 0; i < nCount; i++)
                    {
                      PRUnichar *p = (PRUnichar *)sqlite3_column_name16(pStmt, i);
                      std::prustring strColumnName;

                      if(p)
                      {
                        strColumnName = p;
                      }

                      vColumnNames.push_back(strColumnName);
                    }

                    pRes->SetColumnNames(vColumnNames);
                  }

                  std::vector<std::prustring> vCellValues;
                  vCellValues.reserve(nCount);

                  for(int i = 0; i < nCount; i++)
                  {
                    PRUnichar *p = (PRUnichar *)sqlite3_column_text16(pStmt, i);
                    std::prustring strCellValue;

                    if(p)
                    {
                      strCellValue = p;
                    }

                    vCellValues.push_back(strCellValue);
                  }

                  pRes->AddRow(vCellValues);
                  pRes->Release();
                }
                break;

                case SQLITE_DONE:
                {
                  pQuery->SetLastError(SQLITE_OK);
                }
                break;

                case SQLITE_MISUSE:
                case SQLITE_BUSY:
                {
                  if(nRetryCount++ > SQLITE_MAX_RETRIES)
                  {
                    PR_Sleep(33);
                    retDB = SQLITE_ROW;
                  }
                }
                break;

                default:
                {
                  pQuery->SetLastError(retDB);

#if defined(HARD_SANITY_CHECK)
                  PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
                  OutputDebugStringW(szErr);
                  OutputDebugStringW(L"\n");
#endif

                }
              }
            }
            while(retDB == SQLITE_ROW);
          }

          //Didn't get any rows
          if(bFirstRow)
          {
            PRBool isPersistent = PR_FALSE;
            pQuery->IsPersistentQuery(&isPersistent);

            if(isPersistent)
            {
              nsString strTableName(strQuery.c_str());
              ToLowerCase(strTableName);

              nsString::const_iterator itStrStart, itStart, itEnd;
              strTableName.BeginReading(itStrStart);
              strTableName.BeginReading(itStart);
              strTableName.EndReading(itEnd);

              PRBool bFound = FindInReadable(NS_LITERAL_STRING(" from "), itStart, itEnd, nsCaseInsensitiveStringComparator());

              if(bFound)
              {
                size_t nCutLen = Distance(itStrStart, itEnd);
                strTableName.Cut(0, nCutLen);

                strTableName.BeginReading(itStrStart);
                strTableName.BeginReading(itStart);
                while(*itStart == ' ')
                  itStart++;

                nCutLen = Distance(itStrStart, itStart);
                strTableName.Cut(0, nCutLen);

                strTableName.BeginReading(itStart);
                strTableName.EndReading(itEnd);
                bFound = FindInReadable(NS_LITERAL_STRING(" "), itStart, itEnd, nsCaseInsensitiveStringComparator());

                if(bFound)
                {
                  size_t nEndLen = Distance(itStrStart, itStart);
                  strTableName.SetLength(nEndLen);
                }

                nsCString theCTableName;
                NS_UTF16ToCString(strTableName, NS_CSTRING_ENCODING_ASCII, theCTableName);
                p->AddPersistentQuery(pQuery, const_cast<char *>(theCTableName.get()));

              }
            }
          }

          //Always release the statement.
          retDB = sqlite3_finalize(pStmt);
          p->UnlockDatabase(pDB);

#if defined(HARD_SANITY_CHECK)
          if(retDB != SQLITE_OK)
          {
            PRUnichar *szErr = (PRUnichar *)sqlite3_errmsg16(pDB);
            OutputDebugStringW(szErr);
            OutputDebugStringW(L"\n");
            //__asm int 3;
          }
#endif

        }
      }

      //pQuery->m_QueryResultLock.Unlock();
    }

    if(pSecondDB != nsnull)
    {
      p->UnlockDatabase(pSecondDB);
    }

    pQuery->m_IsExecuting = PR_FALSE;
    pQuery->m_IsAborting = PR_FALSE;

    //Whatever happened, the query is done running now.
    pQuery->m_DatabaseQueryHasCompleted.Set();

    //Check if this query changed any data 
    p->UpdatePersistentQueries(pQuery);

    //Check if this query is a persistent query so we can now fire off the callback.
    p->DoPersistentCallback(pQuery);

    //Close up the db.
    //p->CloseDB(pGUID);
    //PR_Free(pGUID);

    NS_IF_RELEASE(pQuery);
  }

  sqlite3_thread_cleanup();
  return;
} //QueryProcessor

//-----------------------------------------------------------------------------
void CDatabaseEngine::UpdatePersistentQueries(CDatabaseQuery *pQuery)
{
  if(pQuery->m_HasChangedDataOfPersistQuery && 
     !pQuery->m_PersistentQuery)
  {
    PRUnichar *pGUID = nsnull;
    pQuery->GetDatabaseGUID(&pGUID);

    if(pGUID)
    {
      sbCommon::CScopedLock lock(&m_PersistentQueriesLock);
      querypersistmap_t::iterator itPersistentQueries = m_PersistentQueries.find(pGUID);

      if(itPersistentQueries != m_PersistentQueries.end())
      {
        std::string strTableName;
        
        sbCommon::CScopedLock lock(&pQuery->m_ModifiedTablesLock);
        CDatabaseQuery::modifiedtables_t::iterator i;
        for ( i = pQuery->m_ModifiedTables.begin(); i != pQuery->m_ModifiedTables.end(); ++i )
        {
          strTableName = *i;
          tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(strTableName);
          if(itTableQuery != itPersistentQueries->second.end())
          {
            querylist_t::iterator itQueries = itTableQuery->second.begin();
            for(; itQueries != itTableQuery->second.end(); itQueries++)
            {
              SubmitQueryPrivate((*itQueries));
            }
          }
        }
      }

      itPersistentQueries = m_PersistentQueries.find(ALL_TOKEN);
      if(itPersistentQueries != m_PersistentQueries.end())
      {
        std::string strTableName;

        sbCommon::CScopedLock lock(&pQuery->m_ModifiedTablesLock);
        CDatabaseQuery::modifiedtables_t::iterator i;
        for ( i = pQuery->m_ModifiedTables.begin(); i != pQuery->m_ModifiedTables.end(); ++i )
        {
          strTableName = *i;
          tablepersistmap_t::iterator itTableQuery = itPersistentQueries->second.find(strTableName);
          if(itTableQuery != itPersistentQueries->second.end())
          {
            querylist_t::iterator itQueries = itTableQuery->second.begin();
            for(; itQueries != itTableQuery->second.end(); itQueries++)
            {
              (*itQueries)->SetDatabaseGUID(ALL_TOKEN);
              SubmitQueryPrivate((*itQueries));
            }
          }
        }
      }

      PR_Free(pGUID);
    }
  }

  {
    sbCommon::CScopedLock lock(&pQuery->m_ModifiedTablesLock);
    pQuery->m_HasChangedDataOfPersistQuery = PR_FALSE;
    pQuery->m_ModifiedTables.clear();
  }

} //UpdatePersistentQueries

//-----------------------------------------------------------------------------
void CDatabaseEngine::DoPersistentCallback(CDatabaseQuery *pQuery)
{
  if(pQuery->m_PersistentQuery)
  {
    //pQuery->m_QueryResultLock.Lock();

    nsCOMPtr<sbIDatabaseResult> pDBResult;
    PRUnichar *strGUID = nsnull, *strQuery = nsnull;

    pQuery->GetResultObject(getter_AddRefs(pDBResult));
    pQuery->GetDatabaseGUID(&strGUID);
    pQuery->GetQuery(0, &strQuery);

    sbCommon::CScopedLock lock(&pQuery->m_PersistentCallbackListLock);
    CDatabaseQuery::persistentcallbacklist_t::const_iterator itCallback = pQuery->m_PersistentCallbackList.begin();
    CDatabaseQuery::persistentcallbacklist_t::const_iterator itEnd = pQuery->m_PersistentCallbackList.end();

    for(; itCallback != itEnd; itCallback++)
    {
      if((*itCallback))
      {
        try
        {
          (*itCallback)->OnQueryEnd(pDBResult, strGUID, strQuery);
        }
        catch(...)
        {
          //bad monkey!
        }
      }
    }

    //pQuery->m_QueryResultLock.Unlock();

    PR_Free(strGUID);
    PR_Free(strQuery);
  }
} //DoPersistentCallback
