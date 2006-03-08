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
 * \file MediaScan.cpp
 * \brief 
 */

// INCLUDES ===================================================================
#include "MediaScan.h"
#include "nspr.h"

#include <nspr/prmem.h>
#include <xpcom/nsMemory.h>
#include <xpcom/nsAutoLock.h>

// CLASSES ====================================================================
//*****************************************************************************
//  CMediaScanQuery Class
//*****************************************************************************
/* Implementation file */
NS_IMPL_ISUPPORTS1(CMediaScanQuery, sbIMediaScanQuery)

//-----------------------------------------------------------------------------
CMediaScanQuery::CMediaScanQuery()
: m_strDirectory( NS_LITERAL_STRING("").get() )
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
  m_FileStack.reserve(0);
} //ctor

//-----------------------------------------------------------------------------
CMediaScanQuery::CMediaScanQuery(const std::prustring &strDirectory, const PRBool &bRecurse, sbIMediaScanCallback *pCallback)
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
NS_IMETHODIMP CMediaScanQuery::SetDirectory(const PRUnichar *strDirectory)
{ 
  {
    nsAutoLock fileLock(m_pFileStackLock);
    m_FileStack.clear();
  }

  if(strDirectory == nsnull)
    return NS_ERROR_NULL_POINTER;

  {
    nsAutoLock dirLock(m_pDirectoryLock);
    m_strDirectory = strDirectory;
  }

  return NS_OK;
} //SetDirectory

//-----------------------------------------------------------------------------
/* wstring GetDirectory (); */
NS_IMETHODIMP CMediaScanQuery::GetDirectory(PRUnichar **_retval)
{
  nsAutoLock lock(m_pDirectoryLock);

  size_t nLen = m_strDirectory.length() + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(m_strDirectory.c_str(), nLen * sizeof(PRUnichar));

  if(*_retval == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

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
  *_retval = m_bRecurse;
  return NS_OK;
} //GetRecurse

//-----------------------------------------------------------------------------
/* void SetCallback (in sbIMediaScanCallback pCallback); */
NS_IMETHODIMP CMediaScanQuery::SetCallback(sbIMediaScanCallback *pCallback)
{
  if(pCallback == nsnull) return NS_ERROR_NULL_POINTER;

  nsAutoLock lock(m_pCallbackLock);
  
  if(pCallback != m_pCallback)
  {
    NS_IF_RELEASE(m_pCallback);
  }
  
  NS_IF_ADDREF(pCallback);
  m_pCallback = pCallback;

  return NS_OK;
} //SetCallback

//-----------------------------------------------------------------------------
/* sbIMediaScanCallback GetCallback (); */
NS_IMETHODIMP CMediaScanQuery::GetCallback(sbIMediaScanCallback **_retval)
{
  nsAutoLock lock(m_pCallbackLock);
  
  *_retval = m_pCallback;
  NS_IF_ADDREF(m_pCallback);

  return NS_OK;
} //GetCallback

//-----------------------------------------------------------------------------
/* PRInt32 GetFileCount (); */
NS_IMETHODIMP CMediaScanQuery::GetFileCount(PRUint32 *_retval)
{
  nsAutoLock lock(m_pFileStackLock);
  size_t nSize = m_FileStack.size();
  *_retval = nSize;

  return NS_OK;
} //GetFileCount

//-----------------------------------------------------------------------------
/* void AddFilePath (in wstring strFilePath); */
NS_IMETHODIMP CMediaScanQuery::AddFilePath(const PRUnichar *strFilePath)
{
  if(strFilePath == nsnull)
    return NS_ERROR_NULL_POINTER;

  nsAutoLock lock(m_pFileStackLock);
  m_FileStack.push_back(strFilePath);

  return NS_OK;
} //AddFilePath

//-----------------------------------------------------------------------------
/* wstring GetFilePath (in PRInt32 nIndex); */
NS_IMETHODIMP CMediaScanQuery::GetFilePath(PRUint32 nIndex, PRUnichar **_retval)
{
  nsAutoLock lock(m_pFileStackLock);

  if(nIndex < m_FileStack.size())
  {
    size_t nLen = m_FileStack[nIndex].length() + 1;
    *_retval = (PRUnichar *) nsMemory::Clone(m_FileStack[nIndex].c_str(), nLen * sizeof(PRUnichar));
    if(*_retval == nsnull)
      return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
} //GetFilePath

//-----------------------------------------------------------------------------
/* PRBool IsScanning (); */
NS_IMETHODIMP CMediaScanQuery::IsScanning(PRBool *_retval)
{
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
NS_IMETHODIMP CMediaScanQuery::GetLastFileFound(PRUnichar **_retval)
{
  nsAutoLock lock(m_pFileStackLock);
  PRInt32 nIndex = m_FileStack.size() - 1;

  size_t nLen = m_FileStack[nIndex].length() + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(m_FileStack[nIndex].c_str(), nLen * sizeof(PRUnichar));

  if(*_retval == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
} //GetLastFileFound

//-----------------------------------------------------------------------------
/* wstring GetCurrentScanPath (); */
NS_IMETHODIMP CMediaScanQuery::GetCurrentScanPath(PRUnichar **_retval)
{
  nsAutoLock lock(m_pCurrentPathLock);

  size_t nLen = m_strCurrentPath.length() + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(m_strCurrentPath.c_str(), nLen * sizeof(PRUnichar));

  if(*_retval == nsnull)
    return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
} //GetCurrentScanPath

//-----------------------------------------------------------------------------
/* void SetCurrentScanPath (in wstring strScanPath); */
NS_IMETHODIMP CMediaScanQuery::SetCurrentScanPath(const PRUnichar *strScanPath)
{
  if(strScanPath == nsnull)
    return NS_ERROR_NULL_POINTER;

  nsAutoLock lock(m_pCurrentPathLock);
  m_strCurrentPath = strScanPath;

  return NS_OK;
} //SetCurrentScanPath

//*****************************************************************************
//  CMediaScan Class
//*****************************************************************************
NS_IMPL_ISUPPORTS1(CMediaScan, sbIMediaScan)

//-----------------------------------------------------------------------------
CMediaScan::CMediaScan()
: m_pQueryQueueLock(PR_NewLock())
{
  NS_ASSERTION(m_pQueryQueueLock, "MediaScan.m_pQueryQueueLock failed");
  m_QueryProcessorThread.Create(reinterpret_cast<sbCommon::threadfunction_t *>(CMediaScan::QueryProcessor), this);
} //ctor

//-----------------------------------------------------------------------------
CMediaScan::~CMediaScan()
{
  m_QueryProcessorThread.SetTerminate(PR_TRUE);
  m_QueryQueueHasItem.Set();
  m_QueryProcessorThread.Terminate();
  if (m_pQueryQueueLock)
    PR_DestroyLock(m_pQueryQueueLock);
} //dtor

//-----------------------------------------------------------------------------
/* void SubmitQuery (in sbIMediaScanQuery pQuery); */
NS_IMETHODIMP CMediaScan::SubmitQuery(sbIMediaScanQuery *pQuery)
{
  if(pQuery == nsnull)
    return NS_ERROR_NULL_POINTER;

  NS_IF_ADDREF(pQuery);

  {
    nsAutoLock lock(m_pQueryQueueLock);
    m_QueryQueue.push_back(pQuery);

    pQuery->SetIsScanning(PR_TRUE);
  }

  m_QueryQueueHasItem.Set();

  return NS_OK;
} //SubmitQuery

//-----------------------------------------------------------------------------
/* PRInt32 ScanDirectory (in wstring strDirectory, in PRBool bRecurse); */
NS_IMETHODIMP CMediaScan::ScanDirectory(const PRUnichar *strDirectory, PRBool bRecurse, sbIMediaScanCallback *pCallback, PRInt32 *_retval)
{
  if(strDirectory == nsnull)
    return NS_ERROR_NULL_POINTER;

  dirstack_t dirStack;
  //filestack_t fileStack;

  *_retval = 0;

  nsresult ret = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsILocalFile> pFile = do_GetService("@mozilla.org/file/local;1");

  nsString strTheDirectory(strDirectory);
  nsCString cstrTheDirectory;
  NS_UTF16ToCString(strTheDirectory, NS_CSTRING_ENCODING_ASCII, cstrTheDirectory);

  ret = pFile->InitWithNativePath(cstrTheDirectory);
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
          //pDirEntries->GetNext(pDirEntry.StartAssignment());
          pDirEntries->GetNext(getter_AddRefs(pDirEntry));

          if(pDirEntry)
          {
            nsIID nsIFileIID = NS_IFILE_IID;
            nsCOMPtr<nsIFile> pEntry;
            //pDirEntry->QueryInterface(nsIFileIID, (void **) pEntry.StartAssignment());
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
                  pEntry->GetPath(strPath);

                  *_retval += 1;

                  if(pCallback)
                  {
                    PRInt32 nCount = *_retval;
                    pCallback->OnMediaScanFile(strPath.get(), nCount);
                  }
                }
                else if(bIsDirectory && bRecurse)
                {
                  dirStack.push_back(pDirEntries);
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
      pCallback->OnMediaScanFile(strTheDirectory.get(), nCount);
    }
  }

  if(pCallback)
  {
    pCallback->OnMediaScanEnd();
  }

  return NS_OK;
} //ScanDirectory

//-----------------------------------------------------------------------------
/*static*/ void PR_CALLBACK CMediaScan::QueryProcessor(void *pData)
{
  sbCommon::threaddata_t *pThreadData = static_cast<sbCommon::threaddata_t *>(pData);
  sbCommon::CThread *t = pThreadData->pThread;
  CMediaScan *p = static_cast<CMediaScan *>(pThreadData->pData);

  while(!t->IsTerminating())
  {
    sbIMediaScanQuery *pQuery = nsnull;
    PRInt32 ret = p->m_QueryQueueHasItem.Wait();
    p->m_QueryQueueHasItem.Reset();

    if(ret == SB_WAIT_SUCCESS)
    {
      nsAutoLock lock(p->m_pQueryQueueLock);

      if(!t->IsTerminating())
      {
        pQuery = p->m_QueryQueue.front();
        p->m_QueryQueue.pop_front();
      }
    }

    if(pQuery)
    {
      PRInt32 nCount = 0;

      pQuery->SetIsScanning(PR_TRUE);
      nCount = p->ScanDirectory(pQuery);
      pQuery->SetIsScanning(PR_FALSE);

      NS_IF_RELEASE(pQuery);
    }
  }

  return;
} //QueryProcessor

//-----------------------------------------------------------------------------
PRInt32 CMediaScan::ScanDirectory(sbIMediaScanQuery *pQuery)
{
  dirstack_t dirStack;
  PRInt32 nFoundCount = 0;

  nsresult ret = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsILocalFile> pFile = do_GetService("@mozilla.org/file/local;1");

  sbIMediaScanCallback *pCallback = nsnull;
  pQuery->GetCallback(&pCallback);

  PRBool bRecurse = PR_FALSE;
  pQuery->GetRecurse(&bRecurse);
  
  PRUnichar *strDirectory = nsnull;
  pQuery->GetDirectory(&strDirectory);

  nsString strTheDirectory(strDirectory);

  {
    nsCString cstrTheDirectory;
    NS_UTF16ToCString(strTheDirectory, NS_CSTRING_ENCODING_ASCII, cstrTheDirectory);
    ret = pFile->InitWithNativePath(cstrTheDirectory);
  }
  
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
          //pDirEntries->GetNext(pDirEntry.StartAssignment());
          pDirEntries->GetNext(getter_AddRefs(pDirEntry));

          if(pDirEntry)
          {
            nsIID nsIFileIID = NS_IFILE_IID;
            nsCOMPtr<nsIFile> pEntry;
            //pDirEntry->QueryInterface(nsIFileIID, (void **) pEntry.StartAssignment());
            pDirEntry->QueryInterface(nsIFileIID, getter_AddRefs(pEntry));

            if(pEntry)
            {
              PRBool bIsFile = PR_FALSE, bIsDirectory = PR_FALSE;
              pEntry->IsFile(&bIsFile);
              pEntry->IsDirectory(&bIsDirectory);

              if(bIsFile)
              {
                nsString strPath;
                pEntry->GetPath(strPath);

                pQuery->AddFilePath(strPath.get());
                nFoundCount += 1;

                if(pCallback)
                {
                  pCallback->OnMediaScanFile(strPath.get(), nFoundCount);
                }
              }
              else if(bIsDirectory && bRecurse)
              {
                dirStack.push_back(pDirEntries);
                pEntry->GetDirectoryEntries(&pDirEntries);
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

            if(strDirectory)
              PR_Free(strDirectory);

            return NS_OK;
          }
        }
      }
    }
  }
  else
  {
    pQuery->AddFilePath(strTheDirectory.get());
  }

  if(pCallback)
  {
    pCallback->OnMediaScanEnd();
  }

  NS_IF_RELEASE(pCallback);
  
  if(strDirectory)
    PR_Free(strDirectory);

  return NS_OK;
} //ScanDirectory