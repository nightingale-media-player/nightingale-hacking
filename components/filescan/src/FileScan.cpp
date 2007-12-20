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
 * \file FileScan.cpp
 * \brief
 */

// INCLUDES ===================================================================
#include "FileScan.h"
#include "nspr.h"

#include <nspr/prmem.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsAutoLock.h>     // for nsAutoMonitor
#include <necko/nsIIOService.h>
#include <necko/nsIURI.h>
#include <unicharutil/nsUnicharUtils.h>
#include <nsThreadUtils.h>
#include <nsStringGlue.h>
#include <nsIObserverService.h>
#include <nsIMutableArray.h>
#include <nsNetUtil.h>
#include <sbLockUtils.h>

// CLASSES ====================================================================
//*****************************************************************************
//  sbFileScanQuery Class
//*****************************************************************************
/* Implementation file */
NS_IMPL_THREADSAFE_ISUPPORTS1(sbFileScanQuery, sbIFileScanQuery)

//-----------------------------------------------------------------------------
sbFileScanQuery::sbFileScanQuery()
: m_pDirectoryLock(PR_NewLock())
, m_pCurrentPathLock(PR_NewLock())
, m_bSearchHidden(PR_FALSE)
, m_bRecurse(PR_FALSE)
, m_pScanningLock(PR_NewLock())
, m_bIsScanning(PR_FALSE)
, m_pCallbackLock(PR_NewLock())
, m_pFileStackLock(PR_NewLock())
, m_pExtensionsLock(PR_NewLock())
, m_pCancelLock(PR_NewLock())
, m_bCancel(PR_FALSE)
{
  NS_ASSERTION(m_pDirectoryLock, "FileScanQuery.m_pDirectoryLock failed");
  NS_ASSERTION(m_pCurrentPathLock, "FileScanQuery.m_pCurrentPathLock failed");
  NS_ASSERTION(m_pCallbackLock, "FileScanQuery.m_pCallbackLock failed");
  NS_ASSERTION(m_pFileStackLock, "FileScanQuery.m_pFileStackLock failed");
  NS_ASSERTION(m_pExtensionsLock, "FileScanQuery.m_pExtensionsLock failed");
  NS_ASSERTION(m_pScanningLock, "FileScanQuery.m_pScanningLock failed");
  NS_ASSERTION(m_pCancelLock, "FileScanQuery.m_pCancelLock failed");
} //ctor

//-----------------------------------------------------------------------------
sbFileScanQuery::sbFileScanQuery(const nsString &strDirectory, const PRBool &bRecurse, sbIFileScanCallback *pCallback)
: m_pDirectoryLock(PR_NewLock())
, m_strDirectory(strDirectory)
, m_pCurrentPathLock(PR_NewLock())
, m_bSearchHidden(PR_FALSE)
, m_bRecurse(bRecurse)
, m_pScanningLock(PR_NewLock())
, m_bIsScanning(PR_FALSE)
, m_pCallbackLock(PR_NewLock())
, m_pCallback(pCallback)
, m_pFileStackLock(PR_NewLock())
, m_pExtensionsLock(PR_NewLock())
, m_pCancelLock(PR_NewLock())
, m_bCancel(PR_FALSE)
{
  NS_ASSERTION(m_pDirectoryLock, "FileScanQuery.m_pDirectoryLock failed");
  NS_ASSERTION(m_pCurrentPathLock, "FileScanQuery.m_pCurrentPathLock failed");
  NS_ASSERTION(m_pCallbackLock, "FileScanQuery.m_pCallbackLock failed");
  NS_ASSERTION(m_pFileStackLock, "FileScanQuery.m_pFileStackLock failed");
  NS_ASSERTION(m_pExtensionsLock, "FileScanQuery.m_pExtensionsLock failed");
  NS_ASSERTION(m_pScanningLock, "FileScanQuery.m_pScanningLock failed");
  NS_ASSERTION(m_pCancelLock, "FileScanQuery.m_pCancelLock failed");
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ sbFileScanQuery::~sbFileScanQuery()
{
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
NS_IMETHODIMP sbFileScanQuery::GetSearchHidden(PRBool *aSearchHidden)
{
  *aSearchHidden = m_bSearchHidden;
  return NS_OK;
} //GetSearchHidden

//-----------------------------------------------------------------------------
NS_IMETHODIMP sbFileScanQuery::SetSearchHidden(PRBool aSearchHidden)
{
  m_bSearchHidden = aSearchHidden;
  return NS_OK;
} //SetSearchHidden

//-----------------------------------------------------------------------------
/* void SetDirectory (in wstring strDirectory); */
NS_IMETHODIMP sbFileScanQuery::SetDirectory(const nsAString &strDirectory)
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
NS_IMETHODIMP sbFileScanQuery::GetDirectory(nsAString &_retval)
{
  PR_Lock(m_pDirectoryLock);
  _retval = m_strDirectory;
  PR_Unlock(m_pDirectoryLock);
  return NS_OK;
} //GetDirectory

//-----------------------------------------------------------------------------
/* void SetRecurse (in PRBool bRecurse); */
NS_IMETHODIMP sbFileScanQuery::SetRecurse(PRBool bRecurse)
{
  m_bRecurse = bRecurse;
  return NS_OK;
} //SetRecurse

//-----------------------------------------------------------------------------
/* PRBool GetRecurse (); */
NS_IMETHODIMP sbFileScanQuery::GetRecurse(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = m_bRecurse;
  return NS_OK;
} //GetRecurse

//-----------------------------------------------------------------------------
NS_IMETHODIMP sbFileScanQuery::AddFileExtension(const nsAString &strExtension)
{
  PR_Lock(m_pExtensionsLock);
  m_Extensions.push_back(PromiseFlatString(strExtension));
  PR_Unlock(m_pExtensionsLock);
  return NS_OK;
} //AddFileExtension

//-----------------------------------------------------------------------------
/* void SetCallback (in sbIFileScanCallback pCallback); */
NS_IMETHODIMP sbFileScanQuery::SetCallback(sbIFileScanCallback *pCallback)
{
  NS_ENSURE_ARG_POINTER(pCallback);

  PR_Lock(m_pCallbackLock);

  m_pCallback = pCallback;

  PR_Unlock(m_pCallbackLock);

  return NS_OK;
} //SetCallback

//-----------------------------------------------------------------------------
/* sbIFileScanCallback GetCallback (); */
NS_IMETHODIMP sbFileScanQuery::GetCallback(sbIFileScanCallback **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  PR_Lock(m_pCallbackLock);
  NS_IF_ADDREF(*_retval = m_pCallback);
  PR_Unlock(m_pCallbackLock);
  return NS_OK;
} //GetCallback

//-----------------------------------------------------------------------------
/* PRInt32 GetFileCount (); */
NS_IMETHODIMP sbFileScanQuery::GetFileCount(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  size_t nSize = m_FileStack.size();
  *_retval = (PRUint32) nSize;
  return NS_OK;
} //GetFileCount

//-----------------------------------------------------------------------------
/* void AddFilePath (in wstring strFilePath); */
NS_IMETHODIMP sbFileScanQuery::AddFilePath(const nsAString &strFilePath)
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
NS_IMETHODIMP sbFileScanQuery::GetFilePath(PRUint32 nIndex, nsAString &_retval)
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
NS_IMETHODIMP sbFileScanQuery::IsScanning(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  PR_Lock(m_pScanningLock);
  *_retval = m_bIsScanning;
  PR_Unlock(m_pScanningLock);
  return NS_OK;
} //IsScanning

//-----------------------------------------------------------------------------
/* void SetIsScanning (in PRBool bIsScanning); */
NS_IMETHODIMP sbFileScanQuery::SetIsScanning(PRBool bIsScanning)
{
  PR_Lock(m_pScanningLock);
  m_bIsScanning = bIsScanning;
  PR_Unlock(m_pScanningLock);
  return NS_OK;
} //SetIsScanning

//-----------------------------------------------------------------------------
/* wstring GetLastFileFound (); */
NS_IMETHODIMP sbFileScanQuery::GetLastFileFound(nsAString &_retval)
{
  PR_Lock(m_pFileStackLock);
  PRInt32 nIndex = (PRInt32)m_FileStack.size() - 1;
  if (nIndex >= 0) {
    _retval = m_FileStack[nIndex];
  }
  else {
    _retval.Truncate();
  }
  PR_Unlock(m_pFileStackLock);
  return NS_OK;
} //GetLastFileFound

//-----------------------------------------------------------------------------
/* wstring GetCurrentScanPath (); */
NS_IMETHODIMP sbFileScanQuery::GetCurrentScanPath(nsAString &_retval)
{
  PR_Lock(m_pCurrentPathLock);
  _retval = m_strCurrentPath;
  PR_Unlock(m_pCurrentPathLock);
  return NS_OK;
} //GetCurrentScanPath

//-----------------------------------------------------------------------------
/* void SetCurrentScanPath (in wstring strScanPath); */
NS_IMETHODIMP sbFileScanQuery::SetCurrentScanPath(const nsAString &strScanPath)
{
  PR_Lock(m_pCurrentPathLock);
  m_strCurrentPath = strScanPath;
  PR_Unlock(m_pCurrentPathLock);
  return NS_OK;
} //SetCurrentScanPath

//-----------------------------------------------------------------------------
/* void Cancel (); */
NS_IMETHODIMP sbFileScanQuery::Cancel()
{
  PR_Lock(m_pCancelLock);
  m_bCancel = PR_TRUE;
  PR_Unlock(m_pCancelLock);
  return NS_OK;
} //Cancel

NS_IMETHODIMP sbFileScanQuery::GetResultRangeAsURIs(PRUint32 aStartIndex,
                                                    PRUint32 aEndIndex,
                                                    nsIArray** _retval)
{
  sbSimpleAutoLock lock(m_pFileStackLock);

  PRUint32 length = m_FileStack.size();
  NS_ENSURE_TRUE(aStartIndex < length, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(aEndIndex < length, NS_ERROR_INVALID_ARG);

  nsresult rv;
  nsCOMPtr<nsIMutableArray> array =
    do_CreateInstance("@mozilla.org/array;1", &rv);

  for (PRUint32 i = aStartIndex; i <= aEndIndex; i++) {
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), m_FileStack[i]);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = array->AppendElement(uri, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ADDREF(*_retval = array);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRBool IsCancelled (); */
NS_IMETHODIMP sbFileScanQuery::IsCancelled(PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  PR_Lock(m_pCancelLock);
  *_retval = m_bCancel;
  PR_Unlock(m_pCancelLock);
  return NS_OK;
} //IsCancelled

//-----------------------------------------------------------------------------
nsString sbFileScanQuery::GetExtensionFromFilename(const nsAString &strFilename)
{
  nsAutoString str(strFilename);

  PRInt32 index = str.RFindChar(NS_L('.'));
  if (index > -1)
    return nsString(Substring(str, index + 1, str.Length() - index));

  return EmptyString();
} //GetExtensionFromFilename

//-----------------------------------------------------------------------------
PRBool sbFileScanQuery::VerifyFileExtension(const nsAString &strExtension)
{
  PRBool isValid = PR_FALSE;

  PR_Lock(m_pExtensionsLock);
  filestack_t::size_type extCount =  m_Extensions.size();
  for(filestack_t::size_type extCur = 0; extCur < extCount; ++extCur)
  {
    if(m_Extensions[extCur].Equals(strExtension, CaseInsensitiveCompare))
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
//  sbFileScan Class
//*****************************************************************************
NS_IMPL_ISUPPORTS2(sbFileScan, nsIObserver, sbIFileScan)
NS_IMPL_THREADSAFE_ISUPPORTS1(sbFileScanThread, nsIRunnable)
//-----------------------------------------------------------------------------
sbFileScan::sbFileScan()
: m_AttemptShutdownOnDestruction(PR_FALSE)
, m_pThreadMonitor(nsAutoMonitor::NewMonitor("sbFileScan.m_pThreadMonitor"))
, m_pThread(nsnull)
, m_ThreadShouldShutdown(PR_FALSE)
, m_ThreadQueueHasItem(PR_FALSE)
{
  NS_ASSERTION(m_pThreadMonitor, "FileScan.m_pThreadMonitor failed");

  nsresult rv;

  // Attempt to create the scan thread
  do {
    nsCOMPtr<nsIRunnable> pThreadRunner = new sbFileScanThread(this);
    NS_ASSERTION(pThreadRunner, "Unable to create sbFileScanThread");
    if (!pThreadRunner)
      break;
    nsresult rv = NS_NewThread(getter_AddRefs(m_pThread),
                               pThreadRunner);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start sbFileScanThread");
  } while (PR_FALSE); // Only do this once

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  if(NS_SUCCEEDED(rv)) {
    rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID,
      PR_FALSE);
  }

  // This shouldn't be an 'else' case because we want to set this flag if
  // either of the above calls failed
  if (NS_FAILED(rv)) {
    NS_ERROR("Unable to register xpcom-shutdown observer");
    m_AttemptShutdownOnDestruction = PR_TRUE;
  }

} //ctor

//-----------------------------------------------------------------------------
sbFileScan::~sbFileScan()
{
  if(m_AttemptShutdownOnDestruction) {
    Shutdown();
  }

  if (m_pThreadMonitor)
    nsAutoMonitor::DestroyMonitor(m_pThreadMonitor);
} //dtor

NS_IMETHODIMP
sbFileScan::Observe(nsISupports *aSubject,
                    const char *aTopic,
                    const PRUnichar *aData)
{
  nsresult rv = NS_OK;

  // Bail if we don't care about the message
  if (strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID))
    return rv;

  rv = Shutdown();
  NS_ENSURE_SUCCESS(rv, rv);

  // And remove ourselves from the observer service
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  return observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
}

nsresult
sbFileScan::Shutdown()
{
  nsresult rv = NS_OK;

  // Shutdown our thread
  if (m_pThread) {
    {
      nsAutoMonitor mon(m_pThreadMonitor);
      m_ThreadShouldShutdown = PR_TRUE;

      rv = mon.Notify();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = m_pThread->Shutdown();
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Shutdown Failed!");

    m_pThread = nsnull;
  }

  return rv;
}

//-----------------------------------------------------------------------------
/* void SubmitQuery (in sbIFileScanQuery pQuery); */
NS_IMETHODIMP sbFileScan::SubmitQuery(sbIFileScanQuery *pQuery)
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
NS_IMETHODIMP sbFileScan::ScanDirectory(const nsAString &strDirectory, PRBool bRecurse, sbIFileScanCallback *pCallback, PRInt32 *_retval)
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
    pCallback->OnFileScanStart();

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
                  #if XP_UNIX && !XP_MACOSX
                    // Note that NewFileURI is broken on Linux when dealing with
                    // file names not in the filesystem charset; see bug 6227
                    // Mac uses a different persistentDescriptor; see bug 6341
                    nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(pEntry));
                    if (localFile) {
                      nsCString spec;
                      nsresult rv2 = localFile->GetPersistentDescriptor(spec);
                      nsCOMPtr<nsIURI> pNewURI;
                      if (NS_SUCCEEDED(rv2)) {
                        spec.Insert("file://", 0);
                        rv2 = pIOService->NewURI(spec, nsnull, nsnull, getter_AddRefs(pNewURI));
                        if (NS_SUCCEEDED(rv2)) {
                          pURI = pNewURI;
                        }
                      }
                    }
                  #endif
                  if (NS_SUCCEEDED(rv))
                  {
                    nsCAutoString u8spec;
                    rv = pURI->GetSpec(u8spec);
                    if (NS_SUCCEEDED(rv))
                    {
                      *_retval += 1;

                      if(pCallback)
                        pCallback->OnFileScanFile(NS_ConvertUTF8toUTF16(u8spec),
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
              pCallback->OnFileScanEnd();
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
      pCallback->OnFileScanFile(strDirectory, *_retval);
    }
  }

  if(pCallback)
    pCallback->OnFileScanEnd();

  return NS_OK;
} //ScanDirectory

//-----------------------------------------------------------------------------
/*static*/ void PR_CALLBACK sbFileScan::QueryProcessor(sbFileScan* pFileScan)
{
  while(PR_TRUE)
  {
    nsCOMPtr<sbIFileScanQuery> pQuery;

    { // Enter Monitor
      nsAutoMonitor mon(pFileScan->m_pThreadMonitor);

      while (!pFileScan->m_ThreadQueueHasItem && !pFileScan->m_ThreadShouldShutdown)
        mon.Wait(PR_MillisecondsToInterval(66));

      if (pFileScan->m_ThreadShouldShutdown) {
        return;
      }

      if(pFileScan->m_QueryQueue.size())
      {
        // The quey was addref'd when it was put into m_QueryQueue so we do
        // not need to addref it again when we assign it to pQuery.  This will
        // be deleted when pQuery goes out of scope.
        pQuery = dont_AddRef(pFileScan->m_QueryQueue.front());
        pFileScan->m_QueryQueue.pop_front();
      }

      if (pFileScan->m_QueryQueue.empty())
        pFileScan->m_ThreadQueueHasItem = PR_FALSE;
    } // Exit Monitor

    if(pQuery) {
      pQuery->SetIsScanning(PR_TRUE);
      pFileScan->ScanDirectory(pQuery);
      pQuery->SetIsScanning(PR_FALSE);
    }
  }
} //QueryProcessor

//-----------------------------------------------------------------------------
PRInt32 sbFileScan::ScanDirectory(sbIFileScanQuery *pQuery)
{
  dirstack_t dirStack;
  fileentrystack_t fileEntryStack;
  entrystack_t entryStack;

  PRInt32 nFoundCount = 0;

  nsresult ret = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsILocalFile> pFile = do_GetService("@mozilla.org/file/local;1");
  nsCOMPtr<nsIIOService> pIOService = do_GetService("@mozilla.org/network/io-service;1");

  sbIFileScanCallback *pCallback = nsnull;
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
    pCallback->OnFileScanStart();
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

                  #if XP_UNIX && !XP_MACOSX
                    // Note that NewFileURI is broken on Linux when dealing with
                    // file names not in the filesystem charset; see bug 6227
                    // Mac uses a different persistentDescriptor; see bug 6341
                    nsCOMPtr<nsILocalFile> localFile(do_QueryInterface(pEntry));
                    if (localFile) {
                      nsCString spec;
                      nsresult rv2 = localFile->GetPersistentDescriptor(spec);
                      nsCOMPtr<nsIURI> pNewURI;
                      if (NS_SUCCEEDED(rv2)) {
                        spec.Insert("file://", 0);
                        rv2 = pIOService->NewURI(spec, nsnull, nsnull, getter_AddRefs(pNewURI));
                        if (NS_SUCCEEDED(rv2)) {
                          pURI = pNewURI;
                        }
                      }
                    }
                  #endif

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
                    pCallback->OnFileScanFile(strPath, nFoundCount);
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
              pCallback->OnFileScanEnd();
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
    pCallback->OnFileScanEnd();
  }

  NS_IF_RELEASE(pCallback);

  dirStack.clear();
  fileEntryStack.clear();
  entryStack.clear();

  return NS_OK;
} //ScanDirectory
