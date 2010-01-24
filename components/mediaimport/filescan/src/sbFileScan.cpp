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
#include "sbFileScan.h"
#include "nspr.h"
#include "prmem.h"
#include "prlog.h"

#include <nsMemory.h>
#include <nsAutoLock.h>     // for nsAutoMonitor
#include <nsIIOService.h>
#include <nsIURI.h>
#include <nsUnicharUtils.h>
#include <nsISupportsPrimitives.h>
#include <nsArrayUtils.h>
#include <nsThreadUtils.h>
#include <nsStringGlue.h>
#include <nsIMutableArray.h>
#include <nsNetUtil.h>
#include <sbILibraryUtils.h>
#include <sbLockUtils.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediaImportFileScan:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediaImportFileScanLog = nsnull;
#define TRACE(args) \
  PR_BEGIN_MACRO \
  if (!gMediaImportFileScanLog) \
    gMediaImportFileScanLog = PR_NewLogModule("sbMediaImportFileScan"); \
  PR_LOG(gMediaImportFileScanLog, PR_LOG_DEBUG, args); \
  PR_END_MACRO
#define LOG(args) \
  PR_BEGIN_MACRO \
  if (!gMediaImportFileScanLog) \
    gMediaImportFileScanLog = PR_NewLogModule("sbMediaImportFileScan"); \
  PR_LOG(gMediaImportFileScanLog, PR_LOG_WARN, args); \
  PR_END_MACRO
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */


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
  , m_pExtensionsLock(PR_NewLock())
  , m_pFlaggedFileExtensionsLock(PR_NewLock())
  , m_pCancelLock(PR_NewLock())
  , m_bCancel(PR_FALSE)
{
  NS_ASSERTION(m_pDirectoryLock, "FileScanQuery.m_pDirectoryLock failed");
  NS_ASSERTION(m_pCurrentPathLock, "FileScanQuery.m_pCurrentPathLock failed");
  NS_ASSERTION(m_pCallbackLock, "FileScanQuery.m_pCallbackLock failed");
  NS_ASSERTION(m_pExtensionsLock, "FileScanQuery.m_pExtensionsLock failed");
  NS_ASSERTION(m_pFlaggedFileExtensionsLock,
      "FileScanQuery.m_pFlaggedFileExtensionsLock failed!");
  NS_ASSERTION(m_pScanningLock, "FileScanQuery.m_pScanningLock failed");
  NS_ASSERTION(m_pCancelLock, "FileScanQuery.m_pCancelLock failed");
  MOZ_COUNT_CTOR(sbFileScanQuery);
  init();
} //ctor

//-----------------------------------------------------------------------------
sbFileScanQuery::sbFileScanQuery(const nsString & strDirectory,
                                 const PRBool & bRecurse,
                                 sbIFileScanCallback *pCallback)
  : m_pDirectoryLock(PR_NewLock())
  , m_strDirectory(strDirectory)
  , m_pCurrentPathLock(PR_NewLock())
  , m_bSearchHidden(PR_FALSE)
  , m_bRecurse(bRecurse)
  , m_pScanningLock(PR_NewLock())
  , m_bIsScanning(PR_FALSE)
  , m_pCallbackLock(PR_NewLock())
  , m_pCallback(pCallback)
  , m_pExtensionsLock(PR_NewLock())
  , m_pFlaggedFileExtensionsLock(PR_NewLock())
  , m_pCancelLock(PR_NewLock())
  , m_bCancel(PR_FALSE)
{
  NS_ASSERTION(m_pDirectoryLock, "FileScanQuery.m_pDirectoryLock failed");
  NS_ASSERTION(m_pCurrentPathLock, "FileScanQuery.m_pCurrentPathLock failed");
  NS_ASSERTION(m_pCallbackLock, "FileScanQuery.m_pCallbackLock failed");
  NS_ASSERTION(m_pExtensionsLock, "FileScanQuery.m_pExtensionsLock failed");
  NS_ASSERTION(m_pFlaggedFileExtensionsLock,
      "FileScanQuery.m_pFlaggedFileExtensionsLock failed!");
  NS_ASSERTION(m_pScanningLock, "FileScanQuery.m_pScanningLock failed");
  NS_ASSERTION(m_pCancelLock, "FileScanQuery.m_pCancelLock failed");
  MOZ_COUNT_CTOR(sbFileScanQuery);
  init();
} //ctor

void sbFileScanQuery::init()
{
  m_pFileStack = nsnull;
  m_pFlaggedFileStack = nsnull;
  m_lastSeenExtension = EmptyString();

  {
    nsAutoLock lock(m_pExtensionsLock);
    PRBool success = m_Extensions.Init();
    NS_ASSERTION(success, "FileScanQuery.m_Extensions failed to be initialized");
  }

  {
    nsAutoLock lock(m_pFlaggedFileExtensionsLock);

    PRBool success = m_FlaggedExtensions.Init();
    NS_ASSERTION(success,
        "FileScanQuery.m_FlaggedExtensions failed to be initialized!");
  }
}

//-----------------------------------------------------------------------------
/*virtual*/ sbFileScanQuery::~sbFileScanQuery()
{
  MOZ_COUNT_DTOR(sbFileScanQuery);
  if (m_pDirectoryLock)
    PR_DestroyLock(m_pDirectoryLock);
  if (m_pCurrentPathLock)
    PR_DestroyLock(m_pCurrentPathLock);
  if (m_pCallbackLock)
    PR_DestroyLock(m_pCallbackLock);
  if (m_pExtensionsLock)
    PR_DestroyLock(m_pExtensionsLock);
  if (m_pFlaggedFileExtensionsLock)
    PR_DestroyLock(m_pFlaggedFileExtensionsLock);
  if (m_pScanningLock)
    PR_DestroyLock(m_pScanningLock);
  if (m_pCancelLock)
    PR_DestroyLock(m_pCancelLock);
} //dtor

//-----------------------------------------------------------------------------
/* attribute boolean searchHidden; */
NS_IMETHODIMP sbFileScanQuery::GetSearchHidden(PRBool *aSearchHidden)
{
  NS_ENSURE_ARG_POINTER(aSearchHidden);
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
  nsAutoLock lock(m_pDirectoryLock);

  nsresult rv;
  if (!m_pFileStack) {
    m_pFileStack =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (!m_pFlaggedFileStack) {
    m_pFlaggedFileStack =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  m_strDirectory = strDirectory;
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
  nsAutoLock lock(m_pExtensionsLock);

  nsAutoString extStr(strExtension);
  ToLowerCase(extStr);
  if (!m_Extensions.GetEntry(extStr)) {
    nsStringHashKey* hashKey = m_Extensions.PutEntry(extStr);
    NS_ENSURE_TRUE(hashKey, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
} //AddFileExtension

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbFileScanQuery::AddFlaggedFileExtension(const nsAString & strExtension)
{
  nsAutoLock lock(m_pFlaggedFileExtensionsLock);

  nsAutoString extStr(strExtension);
  ToLowerCase(extStr);
  if (!m_FlaggedExtensions.GetEntry(extStr)) {
    nsStringHashKey *hashKey = m_FlaggedExtensions.PutEntry(extStr);
    NS_ENSURE_TRUE(hashKey, NS_ERROR_OUT_OF_MEMORY);
  }

  return NS_OK;
} //AddFlaggedFileExtension

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbFileScanQuery::GetFlaggedExtensionsFound(PRBool *aOutIsFound)
{
  NS_ENSURE_ARG_POINTER(aOutIsFound);

  *aOutIsFound = PR_FALSE;

  if (m_pFlaggedFileStack) {
    PRUint32 length;
    nsresult rv = m_pFlaggedFileStack->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    *aOutIsFound = length > 0;
  }

  return NS_OK;
}

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
  if (m_pFileStack) {
    m_pFileStack->GetLength(_retval);
  } else {
    // no stack, scanning never started
    *_retval = 0;
  }
  LOG(("sbFileScanQuery: reporting %d files\n", *_retval));
  return NS_OK;
} //GetFileCount

//-----------------------------------------------------------------------------
/* PRInt32 GetFlaggedFileCount (); */
NS_IMETHODIMP sbFileScanQuery::GetFlaggedFileCount(PRUint32 *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if (m_pFlaggedFileStack) {
    m_pFlaggedFileStack->GetLength(_retval);
  }
  else {
    // no stack, scanning never started
    *_retval = 0;
  }

  LOG(("sbFileScanQuery: reporting %d flagged files\n", *_retval));
  return NS_OK;
} //GetFileCount

//-----------------------------------------------------------------------------
/* void AddFilePath (in wstring strFilePath); */
NS_IMETHODIMP sbFileScanQuery::AddFilePath(const nsAString &strFilePath)
{
  PRBool isFlagged = PR_FALSE;
  const nsAutoString strExtension = GetExtensionFromFilename(strFilePath);
  if (m_lastSeenExtension.IsEmpty() ||
      !m_lastSeenExtension.Equals(strExtension, CaseInsensitiveCompare)) {
    // m_lastSeenExtension could be set multiple times without lock guarded
    // in theory. However, the race is benign and in practice, it is rare.
    PRBool isValidExtension = VerifyFileExtension(strExtension, &isFlagged);
    if (isValidExtension) {
      m_lastSeenExtension = strExtension;
    } else if (!isValidExtension && !isFlagged) {
      LOG(("sbFileScanQuery::AddFilePath, unrecognized extension: (%s) is seen\n",
           NS_LossyConvertUTF16toASCII(strExtension).get()));
      return NS_OK;
    }
  }

  nsresult rv;
  nsCOMPtr<nsISupportsString> string =
    do_CreateInstance("@mozilla.org/supports-string;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = string->SetData(strFilePath);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isFlagged) {
    rv = m_pFlaggedFileStack->AppendElement(string, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = m_pFileStack->AppendElement(string, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  LOG(("sbFileScanQuery::AddFilePath(%s)\n",
       NS_LossyConvertUTF16toASCII(strFilePath).get()));
  return NS_OK;
} //AddFilePath

//-----------------------------------------------------------------------------
/* wstring GetFilePath (in PRInt32 nIndex); */
NS_IMETHODIMP sbFileScanQuery::GetFilePath(PRUint32 nIndex, nsAString &_retval)
{
  _retval = EmptyString();
  NS_ENSURE_ARG_MIN(nIndex, 0);

  PRUint32 length;
  m_pFileStack->GetLength(&length);
  if (nIndex < length) {
    nsresult rv;
    nsCOMPtr<nsISupportsString> path = do_QueryElementAt(m_pFileStack, nIndex, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    path->GetData(_retval);
  }

  return NS_OK;
} //GetFilePath

//------------------------------------------------------------------------------
/* AString getFlaggedFilePath(in PRUint32 nIndex); */
NS_IMETHODIMP
sbFileScanQuery::GetFlaggedFilePath(PRUint32 nIndex, nsAString &retVal)
{
  retVal = EmptyString();
  NS_ENSURE_ARG_MIN(nIndex, 0);

  PRUint32 length;
  m_pFlaggedFileStack->GetLength(&length);
  if (nIndex < length) {
    nsresult rv;
    nsCOMPtr<nsISupportsString> path =
      do_QueryElementAt(m_pFlaggedFileStack, nIndex, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    path->GetData(retVal);
  }

  return NS_OK;
} //getFlaggedFilePath

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
  PRUint32 length;
  m_pFileStack->GetLength(&length);
  if (length > 0) {
    nsresult rv;
    nsCOMPtr<nsISupportsString> path =
      do_QueryElementAt(m_pFileStack, length - 1, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    path->GetData(_retval);
  }
  else {
    _retval.Truncate();
  }
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

//-----------------------------------------------------------------------------
NS_IMETHODIMP sbFileScanQuery::GetResultRangeAsURIStrings(PRUint32 aStartIndex,
                                                          PRUint32 aEndIndex,
                                                          nsIArray** _retval)
{
  PRUint32 length;
  m_pFileStack->GetLength(&length);
  NS_ENSURE_TRUE(aStartIndex < length, NS_ERROR_INVALID_ARG);
  NS_ENSURE_TRUE(aEndIndex < length, NS_ERROR_INVALID_ARG);

  if (aStartIndex == 0 && aEndIndex == length - 1) {
    NS_ADDREF(*_retval = m_pFileStack);
  } else {
    nsresult rv;
    nsCOMPtr<nsIMutableArray> array =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);

    for (PRUint32 i = aStartIndex; i <= aEndIndex; i++) {
      nsCOMPtr<nsISupportsString> uriSpec = do_QueryElementAt(m_pFileStack, i);
      if (!uriSpec) {
        continue;
      }
      rv = array->AppendElement(uriSpec, PR_FALSE);
      NS_ENSURE_SUCCESS(rv, rv);
#if PR_LOGGING
      nsAutoString s;
      if (NS_SUCCEEDED(uriSpec->GetData(s)) && !s.IsEmpty()) {
        LOG(("sbFileScanQuery:: fetched URI %s\n", NS_LossyConvertUTF16toASCII(s).get()));
      }
#endif /* PR_LOGGING */
    }
    NS_ADDREF(*_retval = array);
  }
  LOG(("sbFileScanQuery:: fetched URIs %d through %d\n", aStartIndex, aEndIndex));

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
PRBool sbFileScanQuery::VerifyFileExtension(const nsAString &strExtension,
                                            PRBool *aOutIsFlaggedExtension)
{
  NS_ENSURE_ARG_POINTER(aOutIsFlaggedExtension);

  *aOutIsFlaggedExtension = PR_FALSE;
  PRBool isValid = PR_FALSE;
  nsAutoString extString;

  { // scoped lock
    nsAutoLock lock(m_pExtensionsLock);

    extString = PromiseFlatString(strExtension);
    ToLowerCase(extString);
    isValid = m_Extensions.GetEntry(extString) != nsnull;
  }

  // If the extension isn't valid, check to see if it is a flagged file
  // extension and set the out param.
  if (!isValid) {
    { // scoped lock
      nsAutoLock lock(m_pFlaggedFileExtensionsLock);

      *aOutIsFlaggedExtension =
        m_FlaggedExtensions.GetEntry(extString) != nsnull;
    }
  }

  return isValid;
} //VerifyFileExtension

//*****************************************************************************
//  sbFileScan Class
//*****************************************************************************
NS_IMPL_ISUPPORTS1(sbFileScan, sbIFileScan)
NS_IMPL_THREADSAFE_ISUPPORTS1(sbFileScanThread, nsIRunnable)
//-----------------------------------------------------------------------------
sbFileScan::sbFileScan()
: m_pThreadMonitor(nsAutoMonitor::NewMonitor("sbFileScan.m_pThreadMonitor"))
, m_pThread(nsnull)
, m_ThreadShouldShutdown(PR_FALSE)
, m_ThreadQueueHasItem(PR_FALSE)
{
  NS_ASSERTION(m_pThreadMonitor, "FileScan.m_pThreadMonitor failed");
  MOZ_COUNT_CTOR(sbFileScan);

  nsresult rv;

  // Attempt to create the scan thread

  nsCOMPtr<nsIRunnable> pThreadRunner = new sbFileScanThread(this);
  NS_ASSERTION(pThreadRunner, "Unable to create sbFileScanThread");
  if (pThreadRunner) {
    rv = NS_NewThread(getter_AddRefs(m_pThread),
                             pThreadRunner);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start sbFileScanThread");
  }
} //ctor

//-----------------------------------------------------------------------------
sbFileScan::~sbFileScan()
{
  MOZ_COUNT_DTOR(sbFileScan);
  nsresult rv = NS_OK;
  rv = Shutdown();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to shut down sbFileScanThread");

  if (m_pThreadMonitor)
    nsAutoMonitor::DestroyMonitor(m_pThreadMonitor);
} //dtor

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
    pQuery->SetIsScanning(PR_TRUE);
    m_QueryQueue.push_back(pQuery);
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

  nsCOMPtr<sbILibraryUtils> pLibraryUtils =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
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
                  // Get a library content URI for the file.
                  NS_ENSURE_SUCCESS(rv, rv);
                  nsCOMPtr<nsIURI> pURI;
                  rv = pLibraryUtils->GetFileContentURI(pEntry,
                                                        getter_AddRefs(pURI));

                  if (NS_SUCCEEDED(rv))
                  {
                    nsCAutoString u8spec;
                    rv = pURI->GetSpec(u8spec);
                    if (NS_SUCCEEDED(rv))
                    {
                      LOG(("sbFileScan::ScanDirectory (idl) found spec: %s\n", u8spec.get()));
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
        mon.Wait();

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
  nsresult rv;

  PRInt32 nFoundCount = 0;

  nsresult ret = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsILocalFile> pFile = do_CreateInstance("@mozilla.org/file/local;1");

  nsCOMPtr<sbILibraryUtils> pLibraryUtils =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

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
      PRBool keepRunning = PR_FALSE;

      {
        nsAutoMonitor mon(m_pThreadMonitor);
        keepRunning = !m_ThreadShouldShutdown;
      }

      while(keepRunning)
      {
        // Allow us to get the hell out of here.
        PRBool cancel = PR_FALSE;
        pQuery->IsCancelled(&cancel);
        
        if (cancel) {
          break;
        }

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
              PRBool bIsFile = PR_FALSE, bIsDirectory = PR_FALSE, bIsHidden = PR_FALSE;
              pEntry->IsFile(&bIsFile);
              pEntry->IsDirectory(&bIsDirectory);
              pEntry->IsHidden(&bIsHidden);

              if(!bIsHidden || bSearchHidden)
              {
                if(bIsFile)
                {
                  // Get a library content URI for the file.
                  nsCOMPtr<nsIURI> pURI;
                  rv = pLibraryUtils->GetFileContentURI(pEntry,
                                                        getter_AddRefs(pURI));

                  // Get the file URI spec.
                  nsCAutoString spec;
                  if (NS_SUCCEEDED(rv)) {
                    rv = pURI->GetSpec(spec);
                    LOG(("sbFileScan::ScanDirectory (C++) found spec: %s\n",
                         spec.get()));
                  }

                  // Add the file path to the query.
                  if (NS_SUCCEEDED(rv)) {
                    nsString strPath = NS_ConvertUTF8toUTF16(spec);
                    pQuery->AddFilePath(strPath);
                    nFoundCount += 1;

                    if(pCallback)
                    {
                      pCallback->OnFileScanFile(strPath, nFoundCount);
                    }
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

        // Yield.
        PR_Sleep(PR_MillisecondsToInterval(0));

        // Check thread shutdown flag since it's possible for our thread
        // to be in shutdown without the query being cancelled.
        {
          nsAutoMonitor mon(m_pThreadMonitor);
          keepRunning = !m_ThreadShouldShutdown;
        }
      }
      NS_IF_RELEASE(pDirEntries);
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

  dirstack_t::iterator it = dirStack.begin();
  for(; it != dirStack.end(); ++it) {
    NS_IF_RELEASE(*it);
  }

  dirStack.clear();
  fileEntryStack.clear();
  entryStack.clear();

  return NS_OK;
} //ScanDirectory
