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
 * \file MediaScan.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include "MediaScan.h"
#include "nspr.h"

#include <nspr/prmem.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsAutoLock.h>     // for nsAutoMonitor
#include <necko/nsIIOService.h>
#include <necko/nsIURI.h>
#include <unicharutil/nsUnicharUtils.h>
#include <nsThreadUtils.h>
#include <nsStringGlue.h>

// CLASSES ====================================================================
//*****************************************************************************
//  CMediaScanQuery Class
//*****************************************************************************
/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS1(CMediaScanQuery, sbIMediaScanQuery)

//-----------------------------------------------------------------------------
CMediaScanQuery::CMediaScanQuery()
: m_strDirectory( EmptyString() )
, m_bSearchHidden(PR_FALSE)
, m_bRecurse(PR_FALSE)
, m_bIsScanning(PR_FALSE)
, m_pCallback(nsnull)
, m_bCancel(PR_FALSE)
, m_pDirectoryLock(PR_NewLock())
, m_pCurrentPathLock(PR_NewLock())
, m_pCallbackLock(PR_NewLock())
, m_pFileStackLock(PR_NewLock())
, m_pExtensionsLock(PR_NewLock())
, m_pScanningLock(PR_NewLock())
, m_pCancelLock(PR_NewLock())
{
  NS_ASSERTION(m_pDirectoryLock, "MediaScanQuery.m_pDirectoryLock failed");
  NS_ASSERTION(m_pCurrentPathLock, "MediaScanQuery.m_pCurrentPathLock failed");
  NS_ASSERTION(m_pCallbackLock, "MediaScanQuery.m_pCallbackLock failed");
  NS_ASSERTION(m_pFileStackLock, "MediaScanQuery.m_pFileStackLock failed");
  NS_ASSERTION(m_pExtensionsLock, "MediaScanQuery.m_pExtensionsLock failed");
  NS_ASSERTION(m_pScanningLock, "MediaScanQuery.m_pScanningLock failed");
  NS_ASSERTION(m_pCancelLock, "MediaScanQuery.m_pCancelLock failed");
} //ctor

//-----------------------------------------------------------------------------
CMediaScanQuery::CMediaScanQuery(const nsString &strDirectory, const PRBool &bRecurse, sbIMediaScanCallback *pCallback)
: m_strDirectory(strDirectory)
, m_bSearchHidden(PR_FALSE)
, m_bRecurse(bRecurse)
, m_bIsScanning(PR_FALSE)
, m_pCallback(pCallback)
, m_bCancel(PR_FALSE)
, m_pDirectoryLock(PR_NewLock())
, m_pCurrentPathLock(PR_NewLock())
, m_pCallbackLock(PR_NewLock())
, m_pFileStackLock(PR_NewLock())
, m_pExtensionsLock(PR_NewLock())
, m_pScanningLock(PR_NewLock())
, m_pCancelLock(PR_NewLock())
{
  NS_ASSERTION(m_pDirectoryLock, "MediaScanQuery.m_pDirectoryLock failed");
  NS_ASSERTION(m_pCurrentPathLock, "MediaScanQuery.m_pCurrentPathLock failed");
  NS_ASSERTION(m_pCallbackLock, "MediaScanQuery.m_pCallbackLock failed");
  NS_ASSERTION(m_pFileStackLock, "MediaScanQuery.m_pFileStackLock failed");
  NS_ASSERTION(m_pExtensionsLock, "MediaScanQuery.m_pExtensionsLock failed");
  NS_ASSERTION(m_pScanningLock, "MediaScanQuery.m_pScanningLock failed");
  NS_ASSERTION(m_pCancelLock, "MediaScanQuery.m_pCancelLock failed");
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
  if (m_pExtensionsLock)
    PR_DestroyLock(m_pExtensionsLock);
  if (m_pScanningLock)
    PR_DestroyLock(m_pScanningLock);
  if (m_pCancelLock)
    PR_DestroyLock(m_pCancelLock);
} //dtor

//-----------------------------------------------------------------------------
/* attribute boolean searchHidden; */
NS_IMETHODIMP CMediaScanQuery::GetSearchHidden(PRBool *aSearchHidden)
{
  *aSearchHidden = m_bSearchHidden;
  return NS_OK;
} //GetSearchHidden

//-----------------------------------------------------------------------------
NS_IMETHODIMP CMediaScanQuery::SetSearchHidden(PRBool aSearchHidden)
{
  m_bSearchHidden = aSearchHidden;
  return NS_OK;
} //SetSearchHidden

//-----------------------------------------------------------------------------
/* void SetDirectory (in wstring strDirectory); */
NS_IMETHODIMP CMediaScanQuery::SetDirectory(const nsAString &strDirectory)
{
  PR_Lock(m_pDirectoryLock);
  {
    PR_Lock(m_pFileStackLock);
    m_FileStack.clear();

    m_strDirectory = strDirectory;
    PR_Unlock(m_pFileStackLock);
  }
  PR_Unlock(m_pDirectoryLock);
  return NS_OK;
} //SetDirectory

//-----------------------------------------------------------------------------
/* wstring GetDirectory (); */
NS_IMETHODIMP CMediaScanQuery::GetDirectory(nsAString &_retval)
{
  PR_Lock(m_pDirectoryLock);
  _retval = m_strDirectory;
  PR_Unlock(m_pDirectoryLock);
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
NS_IMETHODIMP CMediaScanQuery::AddFileExtension(const nsAString &strExtension)
{
  PR_Lock(m_pExtensionsLock);
  m_Extensions.push_back(PromiseFlatString(strExtension));
  PR_Unlock(m_pExtensionsLock);
  return NS_OK;
} //AddFileExtension

//-----------------------------------------------------------------------------
/* void SetCallback (in sbIMediaScanCallback pCallback); */
NS_IMETHODIMP CMediaScanQuery::SetCallback(sbIMediaScanCallback *pCallback)
{
  NS_ENSURE_ARG_POINTER(pCallback);

  PR_Lock(m_pCallbackLock);

  if(pCallback != m_pCallback)
    NS_IF_RELEASE(m_pCallback);  
  NS_ADDREF(pCallback);
  m_pCallback = pCallback;

  PR_Unlock(m_pCallbackLock);

  return NS_OK;
} //SetCallback

//-----------------------------------------------------------------------------
/* sbIMediaScanCallback GetCallback (); */
NS_IMETHODIMP CMediaScanQuery::GetCallback(sbIMediaScanCallback **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  PR_Lock(m_pCallbackLock);
  *_retval = m_pCallback;
  NS_IF_ADDREF(m_pCallback);
  PR_Unlock(m_pCallbackLock);
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
  nsString strExtension = GetExtensionFromFilename(strFilePath);
  if(VerifyFileExtension(strExtension)) {
    PR_Lock(m_pFileStackLock);
    m_FileStack.push_back(PromiseFlatString(strFilePath));
    PR_Unlock(m_pFileStackLock);
  }
  return NS_OK;
} //AddFilePath

//-----------------------------------------------------------------------------
/* wstring GetFilePath (in PRInt32 nIndex); */
NS_IMETHODIMP CMediaScanQuery::GetFilePath(PRUint32 nIndex, nsAString &_retval)
{
  _retval = EmptyString();
  NS_ENSURE_ARG_MIN(nIndex, 0);

  PR_Lock(m_pFileStackLock);

  if(nIndex < m_FileStack.size()) {
    _retval = m_FileStack[nIndex];
  }

  PR_Unlock(m_pFileStackLock);

  return NS_OK;
} //GetFilePath

//-----------------------------------------------------------------------------
/* PRBool IsScanning (); */
NS_IMETHODIMP CMediaScanQuery::IsScanning(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  PR_Lock(m_pScanningLock);
  *_retval = m_bIsScanning;
  PR_Unlock(m_pScanningLock);
  return NS_OK;
} //IsScanning

//-----------------------------------------------------------------------------
/* void SetIsScanning (in PRBool bIsScanning); */
NS_IMETHODIMP CMediaScanQuery::SetIsScanning(PRBool bIsScanning)
{
  PR_Lock(m_pScanningLock);
  m_bIsScanning = bIsScanning;
  PR_Unlock(m_pScanningLock);
  return NS_OK;
} //SetIsScanning

//-----------------------------------------------------------------------------
/* wstring GetLastFileFound (); */
NS_IMETHODIMP CMediaScanQuery::GetLastFileFound(nsAString &_retval)
{
  PR_Lock(m_pFileStackLock);
  PRInt32 nIndex = (PRInt32)m_FileStack.size() - 1;
  _retval = m_FileStack[nIndex];
  PR_Unlock(m_pFileStackLock);
  return NS_OK;
} //GetLastFileFound

//-----------------------------------------------------------------------------
/* wstring GetCurrentScanPath (); */
NS_IMETHODIMP CMediaScanQuery::GetCurrentScanPath(nsAString &_retval)
{
  PR_Lock(m_pCurrentPathLock);
  _retval = m_strCurrentPath;
  PR_Unlock(m_pCurrentPathLock);
  return NS_OK;
} //GetCurrentScanPath

//-----------------------------------------------------------------------------
/* void SetCurrentScanPath (in wstring strScanPath); */
NS_IMETHODIMP CMediaScanQuery::SetCurrentScanPath(const nsAString &strScanPath)
{
  PR_Lock(m_pCurrentPathLock);
  m_strCurrentPath = strScanPath;
  PR_Unlock(m_pCurrentPathLock);
  return NS_OK;
} //SetCurrentScanPath

//-----------------------------------------------------------------------------
/* void Cancel (); */
NS_IMETHODIMP CMediaScanQuery::Cancel()
{
  PR_Lock(m_pCancelLock);
  m_bCancel = PR_TRUE;
  PR_Unlock(m_pCancelLock);
  return NS_OK;
} //Cancel

//-----------------------------------------------------------------------------
/* PRBool IsCancelled (); */
NS_IMETHODIMP CMediaScanQuery::IsCancelled(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  PR_Lock(m_pCancelLock);
  *_retval = m_bCancel;
  PR_Unlock(m_pCancelLock);
  return NS_OK;
} //IsCancelled

//-----------------------------------------------------------------------------
nsString CMediaScanQuery::GetExtensionFromFilename(const nsAString &strFilename)
{
  nsAutoString str(strFilename);

  PRInt32 index = str.RFindChar(NS_L('.'));
  if (index > -1)
    return nsString(Substring(str, index + 1, str.Length() - index));

  return EmptyString();
} //GetExtensionFromFilename

//-----------------------------------------------------------------------------
PRBool CMediaScanQuery::VerifyFileExtension(const nsAString &strExtension)
{
  PRBool isValid = PR_FALSE;

  PR_Lock(m_pExtensionsLock);
  filestack_t::size_type extCount =  m_Extensions.size();
  for(filestack_t::size_type extCur = 0; extCur < extCount; ++extCur)
  {
    if(m_Extensions[extCur].Equals(strExtension))
    {
      isValid = PR_TRUE;
      break;
    }
  }
  
  if(extCount == 0)
    isValid = PR_TRUE;
  PR_Unlock(m_pExtensionsLock);

  return isValid;
} //VerifyFileExtension

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
                               pThreadRunner);
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
    m_pThread->Shutdown();
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
  NS_ENSURE_ARG_POINTER(_retval);

  dirstack_t dirStack;
  fileentrystack_t fileEntryStack;

  *_retval = 0;

  nsresult rv;
  nsCOMPtr<nsILocalFile> pFile =
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIIOService> pIOService =
    do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pFile->InitWithPath(strDirectory);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool bFlag;
  rv = pFile->IsDirectory(&bFlag);

  if(pCallback)
    pCallback->OnMediaScanStart();

  if(NS_SUCCEEDED(rv) && bFlag)
  {
    nsISimpleEnumerator* pDirEntries;
    rv = pFile->GetDirectoryEntries(&pDirEntries);

    if(NS_SUCCEEDED(rv))
    {
      PRBool bHasMore;

      for(;;)
      {
        // Must null-check pDirEntries here because we're inside a loop
        if(pDirEntries &&
           NS_SUCCEEDED(pDirEntries->HasMoreElements(&bHasMore)) && bHasMore)
        {

          nsCOMPtr<nsISupports> pDirEntry;
          rv = pDirEntries->GetNext(getter_AddRefs(pDirEntry));

          if(NS_SUCCEEDED(rv))
          {
            nsCOMPtr<nsIFile> pEntry = do_QueryInterface(pDirEntry, &rv);
            
            if(NS_SUCCEEDED(rv))
            {
              PRBool bIsFile, bIsDirectory, bIsHidden;

              // Don't scan hidden things.  
              // Otherwise we wind up in places that can crash the app?
              if (NS_SUCCEEDED(pEntry->IsHidden(&bIsHidden)) && bIsHidden)
              {
                if(NS_SUCCEEDED(pEntry->IsFile(&bIsFile)) && bIsFile)
                {
                  // Get the file:// uri from the file object.
                  nsCOMPtr<nsIURI> pURI;
                  rv = pIOService->NewFileURI(pEntry, getter_AddRefs(pURI));
                  if (NS_SUCCEEDED(rv))
                  {
                    nsCAutoString u8spec;
                    rv = pURI->GetSpec(u8spec);
                    if (NS_SUCCEEDED(rv))
                    {
                      *_retval += 1;

                      if(pCallback)
                        pCallback->OnMediaScanFile(NS_ConvertUTF8toUTF16(u8spec),
                                                   *_retval);
                    }
                  }
                }
                else if(bRecurse &&
                        NS_SUCCEEDED(pEntry->IsDirectory(&bIsDirectory)) &&
                        bIsDirectory)
                {
                  dirStack.push_back(pDirEntries);
                  fileEntryStack.push_back(pEntry);

                  rv = pEntry->GetDirectoryEntries(&pDirEntries);
                  if (NS_FAILED(rv))
                    pDirEntries = nsnull;
                }
              }
            }
          }
        }
        else
        {
          NS_IF_RELEASE(pDirEntries);
          if(dirStack.size())
          {
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
    if(NS_SUCCEEDED(pFile->IsFile(&bFlag)) && 
       bFlag && 
       pCallback)
    {
      *_retval = 1;
      pCallback->OnMediaScanFile(strDirectory, *_retval);
    }
  }

  if(pCallback)
    pCallback->OnMediaScanEnd();

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
      pMediaScan->ScanDirectory(pQuery);
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

  PRBool bSearchHidden = PR_FALSE;
  pQuery->GetSearchHidden(&bSearchHidden);
  
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
        // Allow us to get the hell out of here.
        PRBool cancel = PR_FALSE;
        pQuery->IsCancelled(&cancel);
        if (cancel)
          break;

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

              if(!bIsHidden || bSearchHidden)
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

#if defined(XP_WIN)
                  ToLowerCase(strPath);
#endif
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
            fileEntryStack.pop_back();
            entryStack.pop_back();
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
        
        PR_Sleep(PR_MillisecondsToInterval(1));
      }
    }
  }
  else if(NS_SUCCEEDED(pFile->IsFile(&bFlag)) && bFlag)
  {
    pQuery->AddFilePath(strTheDirectory);
  }

  if(pCallback)
  {
    pCallback->OnMediaScanEnd();
  }

  NS_IF_RELEASE(pCallback);
  
  dirStack.clear();
  fileEntryStack.clear();
  entryStack.clear();
  
  return NS_OK;
} //ScanDirectory
