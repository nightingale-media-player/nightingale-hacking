/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Base media item download job.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbBaseMediaItemDownloadJob.cpp
 * \brief Base Media Item Download Job Source.
 */

//------------------------------------------------------------------------------
//
// Base media item download job imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbBaseMediaItemDownloadJob.h"

// Songbird imports.
#include <sbPropertiesCID.h>
#include <sbProxiedComponentManager.h>
#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>

// Mozilla imports.
#include <mozilla/Mutex.h>
#include <nsIURI.h>
#include <nsIURL.h>


//------------------------------------------------------------------------------
//
// Base media item download job logging services.
//
//------------------------------------------------------------------------------

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseMediaItemDownloadJob:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */

#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gBaseMediaItemDownloadJobLog = nsnull;
#define TRACE(args) PR_LOG(gBaseMediaItemDownloadJobLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gBaseMediaItemDownloadJobLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */


//------------------------------------------------------------------------------
//
// Base media item download job nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS4(sbBaseMediaItemDownloadJob,
                              sbIMediaItemDownloadJob,
                              sbIJobProgress,
                              sbIJobCancelable,
                              sbIFileDownloaderListener)


//------------------------------------------------------------------------------
//
// Base media item download job sbIMediaItemDownloadJob implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// start
//
// \see sbIMediaItemDownloadJob
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::Start()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));

  // Function variables.
  nsresult rv;

  // Get the download URI.
  nsCOMPtr<nsIURI> downloadURI;
  rv = GetDownloadURI(getter_AddRefs(downloadURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // Start downloading from the download URI.
  rv = Start(downloadURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//
// Getters/setters.
//

//-------------------------------------
//
// downloadedFile
//
// \see sbIMediaItemDownloadJob
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetDownloadedFile(nsIFile** aDownloadedFile)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aDownloadedFile);

  // Get the file downloader under the lock.
  nsCOMPtr<sbIFileDownloader> fileDownloader;
  {
    mozilla::MutexAutoLock lock(mLock);
    fileDownloader = mFileDownloader;
  }
  NS_ENSURE_TRUE(fileDownloader, NS_ERROR_NOT_AVAILABLE);

  // Get the downloaded file.
  nsresult rv = fileDownloader->GetDestinationFile(aDownloadedFile);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-------------------------------------
//
// properties
//
// \see sbIMediaItemDownloadJob
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetProperties(sbIPropertyArray** aProperties)
{
  NS_ENSURE_ARG_POINTER(aProperties);
  nsCOMPtr<sbIMutablePropertyArray> properties;
  {
    mozilla::MutexAutoLock lock(mLock);
    properties = mProperties;
  }
  return CallQueryInterface(properties, aProperties);
}


//-------------------------------------
//
// temporaryFileFactory
//
// \see sbIMediaItemDownloadJob
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetTemporaryFileFactory
                              (sbITemporaryFileFactory** aTemporaryFileFactory)
{
  NS_ENSURE_ARG_POINTER(aTemporaryFileFactory);
  mozilla::MutexAutoLock lock(mLock);
  NS_IF_ADDREF(*aTemporaryFileFactory = mTemporaryFileFactory);
  return NS_OK;
}

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::SetTemporaryFileFactory
                              (sbITemporaryFileFactory* aTemporaryFileFactory)
{
  mozilla::MutexAutoLock lock(mLock);
  mTemporaryFileFactory = aTemporaryFileFactory;
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Base media item download job sbIJobProgress implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// addJobProgressListener
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::AddJobProgressListener
                              (sbIJobProgressListener* aListener)
{
  TRACE(("%s[%.8x] 0x%08x", __FUNCTION__, this, aListener));
  NS_ENSURE_ARG_POINTER(aListener);

  // Add the listener to the list of listeners.
  mozilla::MutexAutoLock lock(mLock);
  if (!mListenerList.Contains(aListener)) {
    nsCOMPtr<sbIJobProgressListener>*
      appendedElement = mListenerList.AppendElement(aListener);
    NS_ENSURE_TRUE(appendedElement, NS_ERROR_OUT_OF_MEMORY);
  }
  else {
    NS_WARNING("Listener already added");
  }

  return NS_OK;
}


//-------------------------------------
//
// removeJobProgressListener
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::RemoveJobProgressListener
                              (sbIJobProgressListener* aListener)
{
  TRACE(("%s[%.8x] 0x%08x", __FUNCTION__, this, aListener));
  NS_ENSURE_ARG_POINTER(aListener);

  // Remove the listener from the list of listeners.
  mozilla::MutexAutoLock lock(mLock);
  if (mListenerList.Contains(aListener)) {
    mListenerList.RemoveElement(aListener);
  }
  else {
    NS_WARNING("Listener not present");
  }

  return NS_OK;
}


//
// Getters/setters.
//

//-------------------------------------
//
// status
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetStatus(PRUint16* aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);
  mozilla::MutexAutoLock lock(mLock);
  *aStatus = mStatus;
  return NS_OK;
}


//-------------------------------------
//
// blocked
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetBlocked(PRBool* aBlocked)
{
  NS_ENSURE_ARG_POINTER(aBlocked);
  *aBlocked = PR_FALSE;
  return NS_OK;
}


//-------------------------------------
//
// statusText
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetStatusText(nsAString& aStatusText)
{
  NS_ASSERTION
    (NS_IsMainThread(),
     "sbBaseMediaItemDownloadJob::GetStatusText is main thread only!");
  aStatusText.Assign(SBLocalizedString
                       ("media_item_downloader.job.status.downloading"));
  return NS_OK;
}


//-------------------------------------
//
// titleText
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetTitleText(nsAString& aTitleText)
{
  NS_ASSERTION
    (NS_IsMainThread(),
     "sbBaseMediaItemDownloadJob::GetTitleText is main thread only!");
  aTitleText.Assign(SBLocalizedString("media_item_downloader.job.title"));
  return NS_OK;
}


//-------------------------------------
//
// progress
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetProgress(PRUint32* aProgress)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aProgress);

  // Function variables.
  nsresult rv;

  // Get the file downloader progress.
  PRUint64 progress = 0;
  if (mFileDownloader) {
    rv = mFileDownloader->GetBytesDownloaded(&progress);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  *aProgress = static_cast<PRUint32>(progress & 0xFFFFFFFF);

  return NS_OK;
}


//-------------------------------------
//
// total
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetTotal(PRUint32* aTotal)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aTotal);

  // Function variables.
  nsresult rv;

  // Get the file downloader total bytes to download.
  PRUint64 bytesToDownload = 0;
  if (mFileDownloader) {
    rv = mFileDownloader->GetBytesToDownload(&bytesToDownload);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Return results.
  *aTotal = static_cast<PRUint32>(bytesToDownload & 0xFFFFFFFF);

  return NS_OK;
}


//-------------------------------------
//
// errorCount
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetErrorCount(PRUint32* aErrorCount)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aErrorCount);

  // Function variables.
  PRUint32 errorCount = 0;
  nsresult rv;

  // No errors if no downloader.
  if (!mFileDownloader) {
    *aErrorCount = 0;
    return NS_OK;
  }

  // Check if the file download is complete.
  PRBool complete;
  rv = mFileDownloader->GetComplete(&complete);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check for error.
  if (complete) {
    PRBool succeeded;
    rv = mFileDownloader->GetSucceeded(&succeeded);
    NS_ENSURE_SUCCESS(rv, rv);
    if (complete && !succeeded)
      errorCount++;
  }

  // Return results.
  *aErrorCount = errorCount;

  return NS_OK;
}


//-------------------------------------
//
// errorMessages
//
// \see sbIJobProgress
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetErrorMessages(nsIStringEnumerator** retval)
{
  // Validate arguments and calling context.
  NS_ENSURE_ARG_POINTER(retval);
  NS_ASSERTION
    (NS_IsMainThread(),
     "sbBaseMediaItemDownloadJob::GetErrorMessages is main thread only!");

  // Function variables.
  nsTArray<nsString> errorMessageList;
  nsresult           rv;

  // Get the error count.
  PRUint32 errorCount;
  rv = GetErrorCount(&errorCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add error message if an error occurred.
  if (errorCount) {
    errorMessageList.AppendElement
                       (SBLocalizedString("media_item_downloader.job.error"));
  }

  // Return results.
  nsCOMPtr<nsIStringEnumerator>
    errorMessageEnum = new sbTArrayStringEnumerator(&errorMessageList);
  NS_ENSURE_TRUE(errorMessageEnum, NS_ERROR_OUT_OF_MEMORY);
  errorMessageEnum.forget(retval);

  return NS_OK;
}

//-------------------------------------
//
// canCancel
//
// \see sbIJobCancelable
//
NS_IMETHODIMP
sbBaseMediaItemDownloadJob::GetCanCancel(PRBool *aCanCancel)
{
  // Always cancelable.
  *aCanCancel = PR_TRUE;

  return NS_OK;
}

//-------------------------------------
//
// cancel
//
// \see sbIJobCancelable
//
NS_IMETHODIMP
sbBaseMediaItemDownloadJob::Cancel()
{
  return Stop();
}

//------------------------------------------------------------------------------
//
// Base media item download job sbIFileDownloadListener implementation.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// onProgress
//
// \see sbIFileDownloadListener
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::OnProgress()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));

  // Function variables.
  nsresult rv;

  // Get the listeners under the lock.
  nsTArray< nsCOMPtr<sbIJobProgressListener> > listenerList;
  {
    mozilla::MutexAutoLock lock(mLock);
    listenerList = mListenerList;
  }

  // Notify the listeners.
  PRInt32 const listenerCount = listenerList.Length();
  for (PRInt32 i = 0; i < listenerCount; ++i) {
    rv = listenerList[i]->OnJobProgress(this);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Listener error");
  }

  return NS_OK;
}


//-------------------------------------
//
// onComplete
//
// \see sbIFileDownloadListener
//

NS_IMETHODIMP
sbBaseMediaItemDownloadJob::OnComplete()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));

  // Function variables.
  nsresult rv;

  // Get the file downloader under the lock.
  nsCOMPtr<sbIFileDownloader> fileDownloader;
  {
    mozilla::MutexAutoLock lock(mLock);
    fileDownloader = mFileDownloader;
  }
  NS_ENSURE_TRUE(fileDownloader, NS_ERROR_NOT_AVAILABLE);

  // Clear file downloader listener.
  rv = fileDownloader->SetListener(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if the file download succeeded.
  PRBool succeeded;
  rv = fileDownloader->GetSucceeded(&succeeded);
  NS_ENSURE_SUCCESS(rv, rv);

  // Update status and get the listeners under the lock.
  nsTArray< nsCOMPtr<sbIJobProgressListener> > listenerList;
  {
    mozilla::MutexAutoLock lock(mLock);

    // Update status.
    if (succeeded)
      mStatus = sbIJobProgress::STATUS_SUCCEEDED;
    else
      mStatus = sbIJobProgress::STATUS_FAILED;

    // Get the listeners.
    listenerList = mListenerList;
  }

  // Notify the listeners.
  PRInt32 const listenerCount = listenerList.Length();
  for (PRInt32 i = 0; i < listenerCount; ++i) {
    rv = listenerList[i]->OnJobProgress(this);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Listener error");
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Base media item download job public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// ~sbBaseMediaItemDownloadJob
//

sbBaseMediaItemDownloadJob::~sbBaseMediaItemDownloadJob()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
}


//------------------------------------------------------------------------------
//
// Base media item download job protected services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// Start
//

nsresult
sbBaseMediaItemDownloadJob::Start(nsIURI* aURI)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aURI);

  // Function variables.
  nsresult rv;

  // Create a file downloader and set it up.
  nsCOMPtr<sbIFileDownloader> fileDownloader =
    do_ProxiedCreateInstance("@songbirdnest.com/Songbird/FileDownloader;1",
                             &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fileDownloader->SetSourceURI(aURI);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fileDownloader->SetListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the destination file extension.
  nsCOMPtr<nsIURL> url = do_MainThreadQueryInterface(aURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCAutoString fileExtension;
    rv = url->GetFileExtension(fileExtension);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = fileDownloader->SetDestinationFileExtension
                           (NS_ConvertUTF8toUTF16(fileExtension));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Set the file downloader temporary file factory.
  nsCOMPtr<sbITemporaryFileFactory> temporaryFileFactory;
  rv = GetTemporaryFileFactory(getter_AddRefs(temporaryFileFactory));
  NS_ENSURE_SUCCESS(rv, rv);
  if (temporaryFileFactory) {
    rv = fileDownloader->SetTemporaryFileFactory(temporaryFileFactory);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Set the file downloader under the lock.
  {
    mozilla::MutexAutoLock lock(mLock);
    mFileDownloader = fileDownloader;
  }

  // Start downloading.
  rv = fileDownloader->Start();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//-------------------------------------
//
// Stop
//
nsresult
sbBaseMediaItemDownloadJob::Stop()
{
  if (mFileDownloader) {
    nsresult rv = mFileDownloader->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//-------------------------------------
//
// GetDownloadURI
//

nsresult
sbBaseMediaItemDownloadJob::GetDownloadURI(nsIURI** aURI)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aURI);

  // Function variables.
  nsresult rv;

  // Get the media item under the lock.
  nsCOMPtr<sbIMediaItem> mediaItem;
  {
    mozilla::MutexAutoLock lock(mLock);
    mediaItem = mMediaItem;
  }

  // Get the media item content source URI.
  rv = mediaItem->GetContentSrc(aURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-------------------------------------
//
// Initialize
//

nsresult
sbBaseMediaItemDownloadJob::Initialize()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));

  // Function variables.
  nsresult rv;

  // Create the lock.
  mozilla::MutexAutoLock lock(mLock);

  // Create the downloaded media item properties array.
  mProperties = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-------------------------------------
//
// sbBaseMediaItemDownloadJob
//

sbBaseMediaItemDownloadJob::sbBaseMediaItemDownloadJob
                              (sbIMediaItem* aMediaItem,
                               sbILibrary*   aTargetLibrary) :
  mMediaItem(aMediaItem),
  mTargetLibrary(aTargetLibrary),
  mStatus(sbIJobProgress::STATUS_RUNNING),
  mLock(nsnull)
{
  // Initialize logging.
#ifdef PR_LOGGING
  if (!gBaseMediaItemDownloadJobLog) {
    gBaseMediaItemDownloadJobLog =
      PR_NewLogModule("sbBaseMediaItemDownloadJob");
  }
#endif

  TRACE(("%s[%.8x]", __FUNCTION__, this));
}

