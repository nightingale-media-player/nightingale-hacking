/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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
 * \file MediaScan.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include "MediaScan.h"
#include "nspr.h"

#include <nspr/prmem.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsAutoLock.h>
#include <necko/nsIIOService.h>
#include <necko/nsIURI.h>

// CLASSES ====================================================================
//*****************************************************************************
//  CMediaScanQuery Class
//*****************************************************************************
/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS1(CMediaScanQuery, sbIMediaScanQuery)

//-----------------------------------------------------------------------------
CMediaScanQuery::CMediaScanQuery()
: m_strDirectory( EmptyString() )
, m_bRecurse(PR_FALSE)
, m_bIsScanning(PR_FALSE)
, m_pCallback(nsnull)
, m_pDirectoryLock(PR_NewLock())
, m_pCurrentPathLock(PR_NewLock())
, m_pCallbackLock(PR_NewLock())
, m_pFileStackLock(PR_NewLock())
, m_pScanningLock(PR_NewLock())
{
  NS_ASSERTION(m_pDirectoryLock, "MediaScanQuery.m_pDirectoryLock failed");
  NS_ASSERTION(m_pCurrentPathLock, "MediaScanQuery.m_pCurrentPathLock failed");
  NS_ASSERTION(m_pCallbackLock, "MediaScanQuery.m_pCallbackLock failed");
  NS_ASSERTION(m_pFileStackLock, "MediaScanQuery.m_pFileStackLock failed");
  NS_ASSERTION(m_pScanningLock, "MediaScanQuery.m_pScanningLock failed");
} //ctor

//-----------------------------------------------------------------------------
CMediaScanQuery::CMediaScanQuery(const nsString &strDirectory, const PRBool &bRecurse, sbIMediaScanCallback *pCallback)
: m_strDirectory(strDirectory)
, m_bRecurse(bRecurse)
, m_bIsScanning(PR_FALSE)
, m_pCallback(pCallback)
, m_pDirectoryLock(PR_NewLock())
, m_pCurrentPathLock(PR_NewLock())
, m_pCallbackLock(PR_NewLock())
, m_pFileStackLock(PR_NewLock())
, m_pScanningLock(PR_NewLock())
{
  NS_ASSERTION(m_pDirectoryLock, "MediaScanQuery.m_pDirectoryLock failed");
  NS_ASSERTION(m_pCurrentPathLock, "MediaScanQuery.m_pCurrentPathLock failed");
  NS_ASSERTION(m_pCallbackLock, "MediaScanQuery.m_pCallbackLock failed");
  NS_ASSERTION(m_pFileStackLock, "MediaScanQuery.m_pFileStackLock failed");
  NS_ASSERTION(m_pScanningLock, "MediaScanQuery.m_pScanningLock failed");
  NS_IF_ADDREF(m_pCallback);
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ CMediaScanQuery::~CMediaScanQuery()
{
  NS_IF_RELEASE(m_pCallback);
  if (m_pDirectoryLock)
    PR_DestroyLock(m_pDirectoryLock);
  if (m_pCurrentPathLock)
    PR_DestroyLock(m_pCurrentPathLock);
  if (m_pCallbackLock)
    PR_DestroyLock(m_pCallbackLock);
  if (m_pFileStackLock)
    PR_DestroyLock(m_pFileStackLock);
  if (m_pScanningLock)
    PR_DestroyLock(m_pScanningLock);
} //dtor

//--------------------------------------------------------------------- --------
/* void SetDirectory (in wstring strDirectory); */
NS_IMETHODIMP CMediaScanQuery::SetDirectory(const nsAString &strDirectory)
{
  nsAutoLock dirLock(m_pDirectoryLock);
  {
    nsAutoLock fileLock(m_pFileStackLock);
    m_FileStack.clear();

    m_strDirectory = strDirectory;
  }
  return NS_OK;
} //SetDirectory

//-----------------------------------------------------------------------------
/* wstring GetDirectory (); */
NS_IMETHODIMP CMediaScanQuery::GetDirectory(nsAString &_retval)
{
  nsAutoLock lock(m_pDirectoryLock);
  _retval = m_strDirectory;
  return NS_OK;
} //GetDirectory

//-----------------------------------------------------------------------------
/* void SetRecurse (in PRBool bRecurse); */
NS_IMETHODIMP CMediaScanQuery::SetRecurse(PRBool bRecurse)
{
  m_bRecurse = bRecurse;
  return NS_OK;
} //SetRecurse

//-----------------------------------------------------------------------------
/* PRBool GetRecurse (); */
NS_IMETHODIMP CMediaScanQuery::GetRecurse(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = m_bRecurse;
  return NS_OK;
} //GetRecurse

//-----------------------------------------------------------------------------
/* void SetCallback (in sbIMediaScanCallback pCallback); */
NS_IMETHODIMP CMediaScanQuery::SetCallback(sbIMediaScanCallback *pCallback)
{
  NS_ENSURE_ARG_POINTER(pCallback);
  nsAutoLock lock(m_pCallbackLock);
  if(pCallback != m_pCallback)
    NS_IF_RELEASE(m_pCallback);  
  NS_ADDREF(pCallback);
  m_pCallback = pCallback;
  return NS_OK;
} //SetCallback

//-----------------------------------------------------------------------------
/* sbIMediaScanCallback GetCallback (); */
NS_IMETHODIMP CMediaScanQuery::GetCallback(sbIMediaScanCallback **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsAutoLock lock(m_pCallbackLock);
  *_retval = m_pCallback;
  NS_IF_ADDREF(m_pCallback);
  return NS_OK;
} //GetCallback

//-----------------------------------------------------------------------------
/* PRInt32 GetFileCount (); */
NS_IMETHODIMP CMediaScanQuery::GetFileCount(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  size_t nSize = m_FileStack.size();
  *_retval = (PRUint32) nSize;
  return NS_OK;
} //GetFileCount

//-----------------------------------------------------------------------------
/* void AddFilePath (in wstring strFilePath); */
NS_IMETHODIMP CMediaScanQuery::AddFilePath(const nsAString &strFilePath)
{
  nsAutoLock lock(m_pFileStackLock);
  m_FileStack.push_back(PromiseFlatString(strFilePath));
  return NS_OK;
} //AddFilePath

//-----------------------------------------------------------------------------
/* wstring GetFilePath (in PRInt32 nIndex); */
NS_IMETHODIMP CMediaScanQuery::GetFilePath(PRUint32 nIndex, nsAString &_retval)
{
  _retval = EmptyString();
  NS_ENSURE_ARG_MIN(nIndex, 0);

  nsAutoLock lock(m_pFileStackLock);

  if(nIndex < m_FileStack.size()) {
    _retval = m_FileStack[nIndex];
  }

  return NS_OK;
} //GetFilePath

//-----------------------------------------------------------------------------
/* PRBool IsScanning (); */
NS_IMETHODIMP CMediaScanQuery::IsScanning(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsAutoLock lock(m_pScanningLock);
  *_retval = m_bIsScanning;
  return NS_OK;
} //IsScanning

//-----------------------------------------------------------------------------
/* void SetIsScanning (in PRBool bIsScanning); */
NS_IMETHODIMP CMediaScanQuery::SetIsScanning(PRBool bIsScanning)
{
  nsAutoLock lock(m_pScanningLock);
  m_bIsScanning = bIsScanning;
  return NS_OK;
} //SetIsScanning

//-----------------------------------------------------------------------------
/* wstring GetLastFileFound (); */
NS_IMETHODIMP CMediaScanQuery::GetLastFileFound(nsAString &_retval)
{
  nsAutoLock lock(m_pFileStackLock);
  PRInt32 nIndex = (PRInt32)m_FileStack.size() - 1;
  _retval = m_FileStack[nIndex];
  return NS_OK;
} //GetLastFileFound

//-----------------------------------------------------------------------------
/* wstring GetCurrentScanPath (); */
NS_IMETHODIMP CMediaScanQuery::GetCurrentScanPath(nsAString &_retval)
{
  nsAutoLock lock(m_pCurrentPathLock);
  _retval = m_strCurrentPath;
  return NS_OK;
} //GetCurrentScanPath

//-----------------------------------------------------------------------------
/* void SetCurrentScanPath (in wstring strScanPath); */
NS_IMETHODIMP CMediaScanQuery::SetCurrentScanPath(const nsAString &strScanPath)
{
  nsAutoLock lock(m_pCurrentPathLock);
  m_strCurrentPath = strScanPath;
  return NS_OK;
} //SetCurrentScanPath

//*****************************************************************************
//  CMediaScan Class
//*****************************************************************************
NS_IMPL_ISUPPORTS1(CMediaScan, sbIMediaScan)
NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediaScanThread, nsIRunnable)
//-----------------------------------------------------------------------------
CMediaScan::CMediaScan()
: m_pThreadMonitor(nsAutoMonitor::NewMonitor("CMediaScan.m_pThreadMonitor"))
, m_pThread(nsnull)
, m_ThreadShouldShutdown(PR_FALSE)
, m_ThreadQueueHasItem(PR_FALSE)
{
  NS_ASSERTION(m_pThreadMonitor, "MediaScan.m_pThreadMonitor failed");
  
  // Attempt to create the scan thread
  do {
    nsCOMPtr<nsIRunnable> pThreadRunner = new sbMediaScanThread(this);
    NS_ASSERTION(pThreadRunner, "Unable to create sbMediaScanThread");
    if (!pThreadRunner)
      break;
    nsresult rv = NS_NewThread(getter_AddRefs(m_pThread),
                               pThreadRunner,
                               0,
                               PR_JOINABLE_THREAD);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start sbMediaScanThread");
  } while (PR_FALSE); // Only do this once
} //ctor

//-----------------------------------------------------------------------------
CMediaScan::~CMediaScan()
{
  if (m_pThread) {
    {
      nsAutoMonitor mon(m_pThreadMonitor);
      m_ThreadShouldShutdown = PR_TRUE;
      mon.Notify();
    }
    m_pThread->Join();
    m_pThread = nsnull;
  }

  if (m_pThreadMonitor)
    nsAutoMonitor::DestroyMonitor(m_pThreadMonitor);
} //dtor

//-----------------------------------------------------------------------------
/* void SubmitQuery (in sbIMediaScanQuery pQuery); */
NS_IMETHODIMP CMediaScan::SubmitQuery(sbIMediaScanQuery *pQuery)
{
  NS_ENSURE_ARG_POINTER(pQuery);
  pQuery->AddRef();

  {
    nsAutoMonitor mon(m_pThreadMonitor);
    m_QueryQueue.push_back(pQuery);
    pQuery->SetIsScanning(PR_TRUE);
    m_ThreadQueueHasItem = PR_TRUE;
    mon.Notify();
  }

  return NS_OK;
} //SubmitQuery

//-----------------------------------------------------------------------------
/* PRInt32 ScanDirectory (in wstring strDirectory, in PRBool bRecurse); */
NS_IMETHODIMP CMediaScan::ScanDirectory(const nsAString &strDirectory, PRBool bRecurse, sbIMediaScanCallback *pCallback, PRInt32 *_retval)
{
  dirstack_t dirStack;
  fileentrystack_t fileEntryStack;

  *_retval = 0;

  nsresult ret = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsILocalFile> pFile = do_GetService("@mozilla.org/file/local;1");
  nsCOMPtr<nsIIOService> pIOService = do_GetService("@mozilla.org/network/io-service;1");
  if(!pFile || !pIOService) return ret;

  nsString strTheDirectory(strDirectory);

  ret = pFile->InitWithPath(strTheDirectory);
  if(NS_FAILED(ret)) return ret;

  PRBool bFlag = PR_FALSE;
  pFile->IsDirectory(&bFlag);

  if(pCallback)
  {
    pCallback->OnMediaScanStart();
  }

  if(bFlag)
  {
    nsISimpleEnumerator *pDirEntries = nsnull;
    pFile->GetDirectoryEntries(&pDirEntries);

    if(pDirEntries)
    {
      PRBool bHasMore = PR_FALSE;

      for(;;)
      {
        if(pDirEntries)
          pDirEntries->HasMoreElements(&bHasMore);
        else
          bHasMore = PR_FALSE;

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
              PRBool bIsFile = PR_FALSE, bIsDirectory = PR_FALSE, bIsHidden = PR_FALSE;
              pEntry->IsFile(&bIsFile);
              pEntry->IsDirectory(&bIsDirectory);
              pEntry->IsHidden(&bIsHidden);

              // Don't scan hidden things.  
              // Otherwise we wind up in places that can crash the app?
              if (!bIsHidden)
              {
                if(bIsFile)
                {
                  nsString strPath;

                  // Get the file:// uri from the file object.
                  nsCOMPtr<nsIURI> pURI;
                  pIOService->NewFileURI(pEntry, getter_AddRefs(pURI));
                  nsCString u8spec;
                  pURI->GetSpec(u8spec);
                  strPath = NS_ConvertUTF8toUTF16(u8spec);

                  *_retval += 1;

                  if(pCallback)
                  {
                    PRInt32 nCount = *_retval;
                    pCallback->OnMediaScanFile(strPath, nCount);
                  }
                }
                else if(bIsDirectory && bRecurse)
                {
                  dirStack.push_back(pDirEntries);
                  fileEntryStack.push_back(pEntry);

                  pEntry->GetDirectoryEntries(&pDirEntries);
                }
              }
            }
          }
        }
        else
        {
          if(dirStack.size())
          {
            NS_IF_RELEASE(pDirEntries);

            pDirEntries = dirStack.back();
            dirStack.pop_back();
          }
          else
          {
            if(pCallback)
            {
              pCallback->OnMediaScanEnd();
            }

            return NS_OK;
          }
        }
      }
    }
  }
  else
  {
    if(pCallback)
    {
      *_retval = 1;
      PRInt32 nCount = *_retval;
      pCallback->OnMediaScanFile(strTheDirectory, nCount);
    }
  }

  if(pCallback)
  {
    pCallback->OnMediaScanEnd();
  }

  return NS_OK;
} //ScanDirectory

//-----------------------------------------------------------------------------
/*static*/ void PR_CALLBACK CMediaScan::QueryProcessor(CMediaScan* pMediaScan)
{
  while(PR_TRUE)
  {
    nsCOMPtr<sbIMediaScanQuery> pQuery;

    { // Enter Monitor
      nsAutoMonitor mon(pMediaScan->m_pThreadMonitor);

      while (!pMediaScan->m_ThreadQueueHasItem && !pMediaScan->m_ThreadShouldShutdown)
        mon.Wait();

      if (pMediaScan->m_ThreadShouldShutdown) {
        return;
      }

      if(pMediaScan->m_QueryQueue.size())
      {
        pQuery = pMediaScan->m_QueryQueue.front();
        pMediaScan->m_QueryQueue.pop_front();
      }

      if (pMediaScan->m_QueryQueue.empty())
        pMediaScan->m_ThreadQueueHasItem = PR_FALSE;
    } // Exit Monitor

    if(pQuery) {
      pQuery->SetIsScanning(PR_TRUE);
      PRInt32 nCount = pMediaScan->ScanDirectory(pQuery);
      pQuery->SetIsScanning(PR_FALSE);
    }
  }
} //QueryProcessor

//-----------------------------------------------------------------------------
PRInt32 CMediaScan::ScanDirectory(sbIMediaScanQuery *pQuery)
{
  dirstack_t dirStack;
  fileentrystack_t fileEntryStack;
  entrystack_t entryStack;

  PRInt32 nFoundCount = 0;

  nsresult ret = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsILocalFile> pFile = do_GetService("@mozilla.org/file/local;1");
  nsCOMPtr<nsIIOService> pIOService = do_GetService("@mozilla.org/network/io-service;1");

  sbIMediaScanCallback *pCallback = nsnull;
  pQuery->GetCallback(&pCallback);

  PRBool bRecurse = PR_FALSE;
  pQuery->GetRecurse(&bRecurse);
  
  nsString strTheDirectory;
  pQuery->GetDirectory(strTheDirectory);

  ret = pFile->InitWithPath(strTheDirectory);
  if(NS_FAILED(ret)) return ret;

  PRBool bFlag = PR_FALSE;
  pFile->IsDirectory(&bFlag);

  if(pCallback)
  {
    pCallback->OnMediaScanStart();
  }

  if(bFlag)
  {
    nsISimpleEnumerator *pDirEntries = nsnull;
    pFile->GetDirectoryEntries(&pDirEntries);

    if(pDirEntries)
    {
      PRBool bHasMore = PR_FALSE;

      for(;;)
      {
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
              PRBool bIsFile = PR_FALSE, bIsDirectory = PR_FALSE, bIsHidden = PR_FALSE;;
              pEntry->IsFile(&bIsFile);
              pEntry->IsDirectory(&bIsDirectory);
              pEntry->IsHidden(&bIsHidden);

              if(!bIsHidden)
              {
                if(bIsFile)
                {
                  nsString strPath;
                  // Get the file:// uri from the file object.
                  nsCOMPtr<nsIURI> pURI;
                  pIOService->NewFileURI(pEntry, getter_AddRefs(pURI));
                  nsCString u8spec;
                  pURI->GetSpec(u8spec);
                  strPath = NS_ConvertUTF8toUTF16(u8spec);

                  pQuery->AddFilePath(strPath);
                  nFoundCount += 1;

                  if(pCallback)
                  {
                    pCallback->OnMediaScanFile(strPath, nFoundCount);
                  }
                }
                else if(bIsDirectory && bRecurse)
                {
                  nsISimpleEnumerator *pMoreEntries = nsnull;
                  pEntry->GetDirectoryEntries(&pMoreEntries);

                  if(pMoreEntries)
                  {
                    dirStack.push_back(pDirEntries);
                    fileEntryStack.push_back(pEntry);
                    entryStack.push_back(pDirEntry);
                    
                    pDirEntries = pMoreEntries;
                  }
                }
              }
            }
          }
        }
        else
        {
          if(dirStack.size())
          {
            NS_IF_RELEASE(pDirEntries);

            pDirEntries = dirStack.back();
            dirStack.pop_back();
          }
          else
          {
            if(pCallback)
            {
              pCallback->OnMediaScanEnd();
            }

            NS_IF_RELEASE(pCallback);
            NS_IF_RELEASE(pDirEntries);

            return NS_OK;
          }
        }
      }
    }
  }
  else
  {
    pQuery->AddFilePath(strTheDirectory);
  }

  if(pCallback)
  {
    pCallback->OnMediaScanEnd();
  }

  NS_IF_RELEASE(pCallback);
  
  return NS_OK;
} //ScanDirectory
