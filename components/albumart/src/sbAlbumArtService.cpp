/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird album art service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtService.cpp
 * \brief Songbird Album Art Service Source.
 */

//------------------------------------------------------------------------------
//
// Songbird album art service imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbAlbumArtService.h"

// Songbird imports.
#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbIAlbumArtFetcherSet.h>
#include <sbIPropertyArray.h>
#include <sbStandardProperties.h>
#include <sbVariantUtils.h>

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsIBinaryOutputStream.h>
#include <nsICategoryManager.h>
#include <nsIConverterInputStream.h>
#include <nsIConverterOutputStream.h>
#include <nsICryptoHash.h>
#include <nsIFileURL.h>
#include <nsIMutableArray.h>
#include <nsIProperties.h>
#include <nsIProxyObjectManager.h>
#include <nsISupportsPrimitives.h>
#include <nsIUnicharLineInputStream.h>
#include <nsIUnicharOutputStream.h>
#include <nsServiceManagerUtils.h>
#include <prprf.h>


//------------------------------------------------------------------------------
//
// Songbird album art service configuration.
//
//------------------------------------------------------------------------------

// Default size for the temporary cache used by various fetchers
#define TEMPORARY_CACHE_SIZE  1000

// Time before clearing the temporary cache (in ms)
#define TEMPORARY_CACHE_CLEAR_TIME  60000

// Interval for checking if any files need to be removed from the album art
// cache folder (in ms).
#define ALBUM_ART_CACHE_CLEANUP_INTERVAL 10000

// The maximum number of URLs to check at one time
#define MAX_ARTWORK_URL_CHECK 50

// Name of log file to save urls we need to check
#define SB_ARTWORK_LOG_FILE "artwork.log"
#define SB_ARTWORK_LOG_CHARSET "UTF-8"
#define SB_ARTWORK_LOG_RWBUFFER 1024
//
// sbAlbumArtServiceValidExtensionList  List of valid album art file extensions.
//

static const char* sbAlbumArtServiceValidExtensionList[] =
{
  "jpg",
  "jpeg",
  "gif",
  "png"
};

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbAlbumArtService:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */
#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gAlbumArtServiceLog = nsnull;
#define TRACE(args) PR_LOG(gAlbumArtServiceLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gAlbumArtServiceLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */


//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS3(sbAlbumArtService,
                   sbIAlbumArtService,
                   sbIMediaListListener,
                   nsIObserver)

//------------------------------------------------------------------------------
//
// sbIAlbumArtService implementation.
//
//------------------------------------------------------------------------------

/**
 * Return a list of album art fetcher contract ID's for the given type
 * (remote, local, all) as an array of nsIVariant's of type ACString. 
 *
 * \param aType               sbIAlbumArtFetcherSet.TYPE_[ALL|REMOTE|LOCAL]
 *
 * \return                    List of album art fetcher contract ID's.
 */

NS_IMETHODIMP
sbAlbumArtService::GetFetcherList(PRUint32 aType,
                                  nsIArray** _retval)
{
  TRACE(("sbAlbumArtService[0x%8.x] - GetFetcherList", this));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;
  
  // Update the fetcher information first so our priorities are correct
  rv = UpdateAlbumArtFetcherInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the fetcher list array.
  nsCOMPtr<nsIMutableArray>
    fetcherList = do_CreateInstance
                    ("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add each fetcher contract ID to fetcher list.
  PRUint32 fetcherCount = mFetcherInfoList.Length();
  for (PRUint32 i = 0; i < fetcherCount; i++) {
    // Append the fetcher to the list only if it is enabled
    if (NS_SUCCEEDED(rv)) {    
      if (mFetcherInfoList[i].enabled) {
        // Make sure we only add the desired type
        if ((sbIAlbumArtFetcherSet::TYPE_LOCAL == aType &&
             !mFetcherInfoList[i].local) ||
            (sbIAlbumArtFetcherSet::TYPE_REMOTE == aType &&
             mFetcherInfoList[i].local))
        {
          // Not the right type, ignore
          continue;
        }
        nsCOMPtr<nsIVariant>
          contractID = sbNewVariant(mFetcherInfoList[i].contractID).get();
        NS_ENSURE_TRUE(contractID, NS_ERROR_OUT_OF_MEMORY);

        rv = fetcherList->AppendElement(contractID, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }
  
  // Return results.
  NS_ADDREF(*_retval = fetcherList);

  return NS_OK;
}


/**
 * \brief Determine whether the image specified by aData and aDataLen of type
 *        specified by aMimeType is a valid album art image.  Return true if
 *        so.
 *
 * \param aMimeType           MIME type of image data.
 * \param aData               Album art image data.
 * \param aDataLen            Length in bytes of image data.
 *
 * \return                    True if image is valid album art.
 */

NS_IMETHODIMP
sbAlbumArtService::ImageIsValidAlbumArt(const nsACString& aMimeType,
                                        const PRUint8*    aData,
                                        PRUint32          aDataLen,
                                        PRBool*           _retval)
{
  TRACE(("sbAlbumArtService[0x%8.x] - ImageIsValidAlbumArt", this));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Ensure image is not empty.
  if (!aData || !aDataLen) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  // Ensure a valid album art file extension can be obtained for the image.
  nsAutoString fileExtension;
  rv = GetAlbumArtFileExtension(aMimeType, fileExtension);
  if (NS_FAILED(rv)) {
    *_retval = PR_FALSE;
    return NS_OK;
  }

  // Image is valid.
  *_retval = PR_TRUE;

  return NS_OK;
}


/**
 * \brief Write the album art image specified by aData and aDataLen of type
 *        specified by aMimeType to a cache file and return the cache file
 *        URL.
 *
 * \param aMimeType           MIME type of image data.
 * \param aData               Album art image data.
 * \param aDataLen            Length in bytes of image data.
 *
 * \return                    Album art image cache file URL.
 */

#define NS_FILE_OUTPUT_STREAM_OPEN_DEFAULT -1

NS_IMETHODIMP
sbAlbumArtService::CacheImage(const nsACString& aMimeType,
                              const PRUint8*    aData,
                              PRUint32          aDataLen,
                              nsIFileURL**      _retval)
{
  TRACE(("sbAlbumArtService[0x%8.x] - CacheImage", this));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aData);
  NS_ENSURE_ARG_POINTER(_retval);
  // We need to be sure that mAlbumArtCacheDir is not null.
  NS_ENSURE_TRUE(mAlbumArtCacheDir, NS_ERROR_NOT_INITIALIZED);
  
  // Function variables.
  nsresult rv;

  // Get the image cache file base name.
  nsAutoString fileBaseName;
  rv = GetCacheFileBaseName(aData, aDataLen, fileBaseName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the image cache file extension.
  nsAutoString fileExtension;
  rv = GetAlbumArtFileExtension(aMimeType, fileExtension);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the image cache file object within the album art cache directory.
  nsAutoString fileName;
  fileName.Assign(fileBaseName);
  fileName.Append(NS_LITERAL_STRING("."));
  fileName.Append(fileExtension);
  nsCOMPtr<nsIFile> cacheFile;
  rv = mAlbumArtCacheDir->Clone(getter_AddRefs(cacheFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cacheFile->Append(fileName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the image cache file nsIFileURL object.
  nsCOMPtr<nsIURI> cacheFileURI;
  rv = mIOService->NewFileURI(cacheFile, getter_AddRefs(cacheFileURI));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIFileURL> cacheFileURL = do_QueryInterface(cacheFileURI, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // If cache file already exists, return it.
  PRBool exists;
  rv = cacheFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (exists) {
    cacheFileURL.forget(_retval);
    return NS_OK;
  }

  // Open a file output stream to the cache file and set to auto-close.
  nsCOMPtr<nsIFileOutputStream> fileOutputStream =
    do_CreateInstance("@mozilla.org/network/file-output-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fileOutputStream->Init(cacheFile,
                              NS_FILE_OUTPUT_STREAM_OPEN_DEFAULT,
                              NS_FILE_OUTPUT_STREAM_OPEN_DEFAULT,
                              0);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoFileOutputStream autoFileOutputStream(fileOutputStream);

  // Write the cache file.
  nsCOMPtr<nsIBinaryOutputStream> binaryOutputStream =
    do_CreateInstance("@mozilla.org/binaryoutputstream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = binaryOutputStream->SetOutputStream(fileOutputStream);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = binaryOutputStream->WriteByteArray((PRUint8*) aData, aDataLen);
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  cacheFileURL.forget(_retval);

  return NS_OK;
}



/**
 * \brief Add arbitrary data to a temporary cache.
 *
 * Used by art fetchers to cache intermediate results
 * for a short period of time.  Allows fetchers to 
 * avoid additional work without keeping their own
 * static cache.
 *
 * Note: The contents of this cache is flushed periodically
 *
 * \param aKey           Hash key
 * \param aData          Arbitrary data to store.
 */
NS_IMETHODIMP
sbAlbumArtService::CacheTemporaryData(const nsAString& aKey,
                                      nsISupports* aData)
{
  TRACE(("sbAlbumArtService[0x%8.x] - CacheTemporaryData", this));
  NS_ENSURE_ARG_POINTER(aData);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  
  PRBool succeeded = mTemporaryCache.Put(aKey, aData);
  NS_ENSURE_TRUE(succeeded, NS_ERROR_FAILURE);
  
  // Start a timer empty out the cache at some point
  if (!mCacheFlushTimer) {
    nsresult rv = NS_OK;
    mCacheFlushTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mCacheFlushTimer->Init(this, 
                                TEMPORARY_CACHE_CLEAR_TIME,
                                nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}



/**
 * \brief Get data previously placed into the temporary cache
 *
 * \param aKey                Hash key
 * \return                    Arbitrary data
 *
 * \throws NS_ERROR_NOT_AVAILABLE if the key is not found
 */
NS_IMETHODIMP
sbAlbumArtService::RetrieveTemporaryData(const nsAString& aKey,
                                         nsISupports** _retval)
{
  TRACE(("sbAlbumArtService[0x%8.x] - RetrieveTemporaryData", this));
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  *_retval = nsnull;
  PRBool succeeded = mTemporaryCache.Get(aKey, _retval);
  return succeeded ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

//------------------------------------------------------------------------------
//
// sbIMediaListListener implementation.
//
//------------------------------------------------------------------------------

NS_IMETHODIMP
sbAlbumArtService::OnItemAdded(sbIMediaList *aMediaList,
                               sbIMediaItem *aMediaItem,
                               PRUint32 aIndex,
                               PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);
  TRACE(("sbAlbumArtService[0x%8.x] - OnItemAdded", this));

  // We don't need to worry about items being added
  *_retval = PR_TRUE;
  return NS_OK;
}
 
NS_IMETHODIMP
sbAlbumArtService::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                       sbIMediaItem *aMediaItem,
                                       PRUint32 aIndex,
                                       PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);
  TRACE(("sbAlbumArtService[0x%8.x] - OnBeforeItemRemoved", this));
 
  // We want after not before
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtService::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                      sbIMediaItem *aMediaItem,
                                      PRUint32 aIndex,
                                      PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(_retval);
  TRACE(("sbAlbumArtService[0x%8.x] - OnAfterItemRemoved", this));
  // We want to be notified of each item removed
  *_retval = PR_FALSE;

  nsresult rv;
  nsString oldImageUrl;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                               oldImageUrl);
  if (NS_FAILED(rv) || oldImageUrl.IsEmpty()) {
    return NS_OK;
  }

  return StoreAlbumArtUrlForCheck(oldImageUrl);
}

NS_IMETHODIMP
sbAlbumArtService::OnItemUpdated(sbIMediaList *aMediaList,
                                 sbIMediaItem *aMediaItem,
                                 sbIPropertyArray *aProperties,
                                 PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aProperties);
  NS_ENSURE_ARG_POINTER(_retval);
  TRACE(("sbAlbumArtService[0x%8.x] - OnItemUpdated", this));
  *_retval = PR_FALSE;

  nsresult rv;
  nsString oldImageUrl;

  rv = aProperties->GetPropertyValue(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                                     oldImageUrl);
  if (NS_FAILED(rv) || oldImageUrl.IsEmpty()) {
    return NS_OK;
  }

  return StoreAlbumArtUrlForCheck(oldImageUrl);
}

NS_IMETHODIMP
sbAlbumArtService::OnItemMoved(sbIMediaList *aMediaList,
                               PRUint32 aFromIndex,
                               PRUint32 aToIndex,
                               PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);
  TRACE(("sbAlbumArtService[0x%8.x] - OnItemMoved", this));
  
  // Not interested in moved items
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtService::OnListCleared(sbIMediaList *aMediaList,
                                 PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_ENSURE_ARG_POINTER(_retval);
  TRACE(("sbAlbumArtService[0x%8.x] - OnListCleared", this));
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtService::OnBatchBegin(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  TRACE(("sbAlbumArtService[0x%8.x] - OnBatchBegin", this));

  // Increment our counter so we know we are in a batch operation
  mBatchHelper.Begin();
  return NS_OK;
}
 
NS_IMETHODIMP
sbAlbumArtService::OnBatchEnd(sbIMediaList *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  
  // Decrement our counter so we can tell when we are no longer in a batch
  // operation
  mBatchHelper.End();
  TRACE(("sbAlbumArtService[0x%8.x] - OnBatchEnd %d",
         this,
         mBatchHelper.Depth()));

  if (!mBatchHelper.IsActive()) {
    // Start the timer so we can clean the cache
    nsresult rv;
    rv = mAlbumArtCleanTimer->Init(this, 
                                   ALBUM_ART_CACHE_CLEANUP_INTERVAL,
                                   nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// nsIObserver implementation.
//
//------------------------------------------------------------------------------

/**
 * Observe will be called when there is a notification for the
 * topic |aTopic|.  This assumes that the object implementing
 * this interface has been registered with an observer service
 * such as the nsIObserverService.
 *
 * If you expect multiple topics/subjects, the impl is
 * responsible for filtering.
 *
 * You should not modify, add, remove, or enumerate
 * notifications in the implemention of observe.
 *
 * @param aSubject : Notification specific interface pointer.
 * @param aTopic   : The notification topic or subject.
 * @param aData    : Notification specific wide string.
 *                    subject event.
 */

NS_IMETHODIMP
sbAlbumArtService::Observe(nsISupports*     aSubject,
                           const char*      aTopic,
                           const PRUnichar* aData)
{
  TRACE(("sbAlbumArtService[0x%8.x] - Observe", this));
  nsresult rv;

  // Dispatch processing of event.
  if (!strcmp(aTopic, "profile-after-change")) {
    // Mark preferences as available and continue with initialization.
    mPrefsAvailable = PR_TRUE;
    rv = Initialize();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (!strcmp(aTopic, SB_LIBRARY_MANAGER_READY_TOPIC)) {
    // Initialize Library specific stuff.
    rv = InitializeLibraryWatch();
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (!strcmp(aTopic, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC)) {
    // Finalize the album art service.
    Finalize();
  } else if (!strcmp(NS_TIMER_CALLBACK_TOPIC, aTopic)) {
    nsCOMPtr<nsITimer> aObserveTimer = do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    if (aObserveTimer == mCacheFlushTimer) {
      // Time to flush the cache
      rv = mCacheFlushTimer->Cancel();
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to cancel the cache timer");
      mCacheFlushTimer = nsnull;
  
      // Expire cached data
      mTemporaryCache.Clear();
    } else if (aObserveTimer == mAlbumArtCleanTimer) {
      // Time to check artwork cache files
      rv = ScanAlbumArtUrls();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

/**
 * Construct an album art service instance.
 */

sbAlbumArtService::sbAlbumArtService() :
  mInitialized(PR_FALSE),
  mPrefsAvailable(PR_FALSE),
  mCacheFlushTimer(nsnull)
{
#ifdef PR_LOGGING
  if (!gAlbumArtServiceLog) {
    gAlbumArtServiceLog = PR_NewLogModule("sbAlbumArtService");
  }
#endif
}


/**
 * Destroy an album art service instance.
 */

sbAlbumArtService::~sbAlbumArtService()
{
  Finalize();
}


/**
 * Initialize the album art service.
 */

nsresult
sbAlbumArtService::Initialize()
{
  TRACE(("sbAlbumArtService[0x%8.x] - Initialize", this));
  nsresult rv;

  // Do nothing if already initialized.
  if (mInitialized)
    return NS_OK;

  // Add observers.
  if (!mObserverService) {
    // Get the observer service.
    mObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Add observers.
    rv = mObserverService->AddObserver(this,
                                       "profile-after-change",
                                       PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mObserverService->AddObserver(this,
                                       SB_LIBRARY_MANAGER_READY_TOPIC,
                                       PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mObserverService->AddObserver(this,
                                       SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC,
                                       PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Do nothing more if preferences are not available.
  if (!mPrefsAvailable)
    return NS_OK;

  // Get the I/O service
  mIOService = do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the MIME service.
  mMIMEService = do_GetService("@mozilla.org/mime;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the valid extension list.
  for (PRUint32 i = 0;
       i < NS_ARRAY_LENGTH(sbAlbumArtServiceValidExtensionList);
       i++) {
    mValidExtensionList.AppendElement(sbAlbumArtServiceValidExtensionList[i]);
  }

  // Get the album art fetcher info.
  rv = GetAlbumArtFetcherInfo();
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the album art cache directory.
  rv = GetAlbumArtCacheDir();
  NS_ENSURE_SUCCESS(rv, rv);

  // Set up the arbitrary data cache
  PRBool succeeded = mTemporaryCache.Init(TEMPORARY_CACHE_SIZE);
  NS_ENSURE_TRUE(succeeded, NS_ERROR_FAILURE);

  // Create our timer to clean up album artwork from the cache folder
  mAlbumArtCleanTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Mark component as initialized.
  mInitialized = PR_TRUE;

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Internal services.
//
//------------------------------------------------------------------------------

/**
 * Start watching the main library for any changes to items artwork or items
 * being removed. Also load any previous cache files that need to be checked.
 */

nsresult
sbAlbumArtService::InitializeLibraryWatch()
{
  nsresult rv;

  // Get the main library so we can listen to changes
  nsCOMPtr<sbILibrary> mainLibrary;
  rv = GetMainLibrary(getter_AddRefs(mainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  mMainLibraryList = do_QueryInterface(mainLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainLibraryList->AddListener(this,
                                    PR_FALSE,  /* not weak */
                                    sbIMediaList::LISTENER_FLAGS_BATCHBEGIN |
                                      sbIMediaList::LISTENER_FLAGS_BATCHEND  |
                                      sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                                      sbIMediaList::LISTENER_FLAGS_ITEMUPDATED,
                                    nsnull     /* filter */);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Now that the library is available lets check for any images we could
  // not remove last time due to shutdown.
  rv = LoadSetFromFile();
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

/**
 * Finalize the album art service.
 */

void
sbAlbumArtService::Finalize()
{
  TRACE(("sbAlbumArtService[0x%8.x] - Finalize", this));
  nsresult rv;

  // Clear the fetcher info.
  mFetcherInfoList.Clear();
  
  // Clear any cache info
  mTemporaryCache.Clear();
  
  // Remove observers.
  if (mObserverService) {
    mObserverService->RemoveObserver(this,
                                     "profile-after-change");
    mObserverService->RemoveObserver(this,
                                     SB_LIBRARY_MANAGER_READY_TOPIC);
    mObserverService->RemoveObserver(this,
                                     SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC);
    mObserverService = nsnull;
  }

  if (mCacheFlushTimer) {
    rv = mCacheFlushTimer->Cancel();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to cancel a cache timer");
    mCacheFlushTimer = nsnull;
  }

  if (mAlbumArtCleanTimer) {
    rv = mAlbumArtCleanTimer->Cancel();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to cancel artwork clean up timer");
    mAlbumArtCleanTimer = nsnull;
  }

  if (mMainLibraryList) {
    rv = mMainLibraryList->RemoveListener(this);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to remove main library listener");
  }
  
  // Dump anything left in our URLSet to file so we can load it later
  rv = SaveSetToFile();
  NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to save list of cache files to check");
}


/**
 * Get the album art cache directory.
 */

nsresult
sbAlbumArtService::GetAlbumArtCacheDir()
{
  TRACE(("sbAlbumArtService[0x%8.x] - GetAlbumArtCacheDir", this));
  nsresult rv;

  // Get the album art cache directory.
  nsCOMPtr<nsIProperties>
    directoryService = do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = directoryService->Get("ProfLD",
                             NS_GET_IID(nsIFile),
                             getter_AddRefs(mAlbumArtCacheDir));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mAlbumArtCacheDir->Append(NS_LITERAL_STRING("artwork"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create the album art cache directory if it doesn't exist.
  PRBool exists;
  rv = mAlbumArtCacheDir->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    nsCOMPtr<nsIFile> parent;
    rv = mAlbumArtCacheDir->GetParent(getter_AddRefs(parent));
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 permissions;
    rv = parent->GetPermissions(&permissions);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mAlbumArtCacheDir->Create(nsIFile::DIRECTORY_TYPE, permissions);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


/**
 * Get the album art fetcher info.
 */

nsresult
sbAlbumArtService::GetAlbumArtFetcherInfo()
{
  TRACE(("sbAlbumArtService[0x%8.x] - GetAlbumArtFetcherInfo", this));
  nsresult rv;

  // Get the category manager.
  nsCOMPtr<nsICategoryManager>
    categoryManager = do_GetService("@mozilla.org/categorymanager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the enumeration of album art fetchers.
  nsCOMPtr<nsISimpleEnumerator> albumArtFetcherEnum;
  rv = categoryManager->EnumerateCategory(SB_ALBUM_ART_FETCHER_CATEGORY,
                                          getter_AddRefs(albumArtFetcherEnum));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get each album art fetcher info.
  while (1) {
    // Check if there are any more album art fetchers.
    PRBool hasMoreElements;
    rv = albumArtFetcherEnum->HasMoreElements(&hasMoreElements);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!hasMoreElements) {
      break;
    }

    // Get the next album art fetcher category entry name.
    nsCOMPtr<nsISupports>        entryNameSupports;
    nsCOMPtr<nsISupportsCString> entryNameSupportsCString;
    nsCAutoString                entryName;
    rv = albumArtFetcherEnum->GetNext(getter_AddRefs(entryNameSupports));
    NS_ENSURE_SUCCESS(rv, rv);
    entryNameSupportsCString = do_QueryInterface(entryNameSupports, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = entryNameSupportsCString->GetData(entryName);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the next album art fetcher.
    char* contractID;
    rv = categoryManager->GetCategoryEntry(SB_ALBUM_ART_FETCHER_CATEGORY,
                                           entryName.get(),
                                           &contractID);
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("sbAlbumArtService::GetAlbumArtFetcherInfo - Found fetcher [%s]", contractID));
    sbAutoNSMemPtr autoContractID(contractID);
    nsCOMPtr<sbIAlbumArtFetcher> albumArtFetcher = do_CreateInstance(contractID,
                                                                     &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 priority = 0;
    albumArtFetcher->GetPriority(&priority);
    
    PRBool isEnabled = PR_FALSE;
    albumArtFetcher->GetIsEnabled(&isEnabled);

    PRBool isLocal = PR_FALSE;
    albumArtFetcher->GetIsLocal(&isLocal);

    // Add the album art fetcher info to the list.
    FetcherInfo fetcherInfo;
    fetcherInfo.contractID.Assign(contractID);
    fetcherInfo.priority = priority;
    fetcherInfo.enabled = isEnabled;
    fetcherInfo.local = isLocal;
    
    NS_ENSURE_TRUE(mFetcherInfoList.AppendElement(fetcherInfo),
                   NS_ERROR_OUT_OF_MEMORY);
  }

  // Sort the album art fetcher info.
  mFetcherInfoList.Sort(); 

  return NS_OK;
}


/**
 * Update the list of album art fetchers (updates priority and enabled flags)
 */
nsresult
sbAlbumArtService::UpdateAlbumArtFetcherInfo()
{
  TRACE(("sbAlbumArtService[0x%8.x] - UpdateAlbumArtFetcherInfo", this));
  nsresult rv;
  
  // Update each fetcher in the fetcher list.
  for (PRUint32 i = 0; i < mFetcherInfoList.Length(); i++) {
    nsCOMPtr<sbIAlbumArtFetcher> albumArtFetcher =
        do_CreateInstance(mFetcherInfoList[i].contractID.get(), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 priority = 0;
    albumArtFetcher->GetPriority(&priority);
    
    PRBool isEnabled = PR_FALSE;
    albumArtFetcher->GetIsEnabled(&isEnabled);

    mFetcherInfoList[i].priority = priority;
    mFetcherInfoList[i].enabled = isEnabled;
  }

  mFetcherInfoList.Sort();

  return NS_OK;
}


/**
 * Get the album art cache file base name for the image data specified by aData
 * and aDataLen and return it in aFileBaseName.
 *
 * \param aData               Album art image data.
 * \param aDataLen            Length in bytes of image data.
 * \param aFileBaseName       Returned image cache file base name.
 */

nsresult
sbAlbumArtService::GetCacheFileBaseName(const PRUint8* aData,
                                        PRUint32       aDataLen,
                                        nsAString&     aFileBaseName)
{
  TRACE(("sbAlbumArtService[0x%8.x] - GetCacheFileBaseName", this));
  // Validate arguments.
  NS_ASSERTION(aData, "aData is null");

  // Function variables.
  nsresult rv;

  // Clear file base name.
  aFileBaseName.Truncate();

  // Generate a hash of the image data.
  nsCAutoString hashValue;
  nsCOMPtr<nsICryptoHash>
    cryptoHash = do_CreateInstance("@mozilla.org/security/hash;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cryptoHash->Init(nsICryptoHash::MD5);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cryptoHash->Update(aData, aDataLen);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cryptoHash->Finish(PR_FALSE, hashValue);
  NS_ENSURE_SUCCESS(rv, rv);

  // Produce the image cache file base name.
  PRUint32 hashLength = hashValue.Length();
  PRUint8* hashData = (PRUint8*) hashValue.get();
  for (PRUint32 i = 0; i < hashLength; i++) {
    char hexValue[3];
    PR_snprintf(hexValue, 3, "%02x", hashData[i]);
    aFileBaseName.AppendLiteral(hexValue);
  }

  return NS_OK;
}


/**
 * Get the album art file extension for the image with the MIME type specified
 * by aMimeType.
 *
 * \param aMimeType           MIME type of image data.
 * \param aFileExtension      Returned image cache file extension.
 */

nsresult
sbAlbumArtService::GetAlbumArtFileExtension(const nsACString& aMimeType,
                                            nsAString&        aFileExtension)
{
  TRACE(("sbAlbumArtService[0x%8.x] - GetAlbumArtFileExtension", this));
  nsCAutoString fileExtension;
  nsresult      rv;

  // Look up the file extension from the MIME type.
  rv = mMIMEService->GetPrimaryExtension(aMimeType,
                                         NS_LITERAL_CSTRING(""),
                                         fileExtension);
  if (NS_FAILED(rv))
    fileExtension.Truncate();

  // Extract the file extension from the MIME type.
  if (fileExtension.IsEmpty()) {
    PRInt32 mimeSubTypeIndex = aMimeType.RFind("/");
    if (mimeSubTypeIndex >= 0) {
      fileExtension.Assign(nsDependentCSubstring(aMimeType,
                                                 mimeSubTypeIndex + 1));
    } else {
      fileExtension.Assign(aMimeType);
    }
  }

  // Convert file extension to lower-case.
  ToLowerCase(fileExtension);

  // Validate the extension.
  if (!mValidExtensionList.Contains(fileExtension))
    return NS_ERROR_FAILURE;

  // Return results.
  aFileExtension.AssignLiteral(fileExtension.get());

  return NS_OK;
}


/**
 * Stores the aCacheURL if unique and if no batch is running starts the
 * Album Art Clean Timer.
 */

nsresult
sbAlbumArtService::StoreAlbumArtUrlForCheck(nsString& aCacheURL)
{
  TRACE(("sbAlbumArtService[0x%8.x] - StoreAlbumArtUrlForCheck", this));
  nsresult rv;
  
  // Add the URL to the set, result.second indicates if the URL was inserted
  std::pair<sbStringSetIter, bool> result = mAlbumArtUrlSet.insert(aCacheURL);
  if (result.second && !mBatchHelper.IsActive()) {
    rv = mAlbumArtCleanTimer->Init(this, 
                                   ALBUM_ART_CACHE_CLEANUP_INTERVAL,
                                   nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

/**
 * Stores the set of album art cache URLs to check to a file in case we are
 * unable to check them before the application quits.
 */

nsresult
sbAlbumArtService::SaveSetToFile()
{
  nsresult rv;
  
  nsCOMPtr<nsIFile> cacheFile;
  rv = mAlbumArtCacheDir->Clone(getter_AddRefs(cacheFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cacheFile->Append(NS_LITERAL_STRING(SB_ARTWORK_LOG_FILE));
  NS_ENSURE_SUCCESS(rv, rv);

  if (mAlbumArtUrlSet.empty()) {
    // erase the file
    rv = cacheFile->Remove(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    // Dump the set to the file
    nsCOMPtr<nsIFileOutputStream> fileOutputStream =
      do_CreateInstance("@mozilla.org/network/file-output-stream;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = fileOutputStream->Init(cacheFile,
                                NS_FILE_OUTPUT_STREAM_OPEN_DEFAULT,
                                NS_FILE_OUTPUT_STREAM_OPEN_DEFAULT,
                                0);
    NS_ENSURE_SUCCESS(rv, rv);
    sbAutoFileOutputStream autoFileOutputStream(fileOutputStream);
    
    nsCOMPtr<nsIConverterOutputStream> converterStream =
      do_CreateInstance("@mozilla.org/intl/converter-output-stream;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = converterStream->Init(fileOutputStream,
                               SB_ARTWORK_LOG_CHARSET,
                               SB_ARTWORK_LOG_RWBUFFER,
                               nsIConverterInputStream::DEFAULT_REPLACEMENT_CHARACTER);
    
    nsCOMPtr<nsIUnicharOutputStream> outputStream =
      do_QueryInterface(converterStream, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    sbStringSetIter currUrl;
    for (currUrl = mAlbumArtUrlSet.begin();
         currUrl != mAlbumArtUrlSet.end();
         currUrl++) {
      nsAutoString writeUrl(*currUrl);
      writeUrl.Append(NS_LITERAL_STRING("\n"));
      PRUint32 writeCount;
      PRBool writeOk;
      rv = outputStream->WriteString(writeUrl, &writeOk);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    
    // Flush and close the outputStream
    outputStream->Flush();
    outputStream->Close();
  }
 
  return NS_OK;
}

/**
 * Retrieves any album art cache URLs from a file to be checked that were not
 * checked in a previous run due to a shutdown.
 */

nsresult
sbAlbumArtService::LoadSetFromFile()
{
  nsresult rv;
  
  nsCOMPtr<nsIFile> cacheFile;
  rv = mAlbumArtCacheDir->Clone(getter_AddRefs(cacheFile));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = cacheFile->Append(NS_LITERAL_STRING(SB_ARTWORK_LOG_FILE));
  NS_ENSURE_SUCCESS(rv, rv);

  // Check if the file even exists
  PRBool exists;
  rv = cacheFile->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    return NS_OK;
  }

  nsCOMPtr<nsIFileInputStream> fileInputStream =
    do_CreateInstance("@mozilla.org/network/file-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = fileInputStream->Init(cacheFile,
                              PR_RDONLY | nsIFileInputStream::CLOSE_ON_EOF,
                              PR_IRUSR,
                              0);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIConverterInputStream> converterStream =
    do_CreateInstance("@mozilla.org/intl/converter-input-stream;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = converterStream->Init(fileInputStream,
                             SB_ARTWORK_LOG_CHARSET,
                             SB_ARTWORK_LOG_RWBUFFER,
                             nsIConverterInputStream::DEFAULT_REPLACEMENT_CHARACTER);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<nsIUnicharLineInputStream> lineInputStream =
    do_QueryInterface(converterStream, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Read the file into the set
  PRBool hasMore = PR_TRUE;
  while(hasMore) {
    nsString inLine;
    rv = lineInputStream->ReadLine(inLine, &hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mAlbumArtUrlSet.insert(inLine);
  }

  if (!mAlbumArtUrlSet.empty()) {
    // Start the timer (if we have urls left)
    rv = mAlbumArtCleanTimer->Init(this, 
                                   ALBUM_ART_CACHE_CLEANUP_INTERVAL,
                                   nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/**
 * Scan the urls of the images that have been collected from items removed or
 * updated and if they no longer exist in the library then delete the files.
 *
 */

nsresult
sbAlbumArtService::ScanAlbumArtUrls()
{
  TRACE(("sbAlbumArtService[0x%8.x] - ScanAlbumArtUrls", this));
  
  if (mAlbumArtUrlSet.empty()) {
    // Nothing to scan
    return NS_OK;
  }

  nsresult rv;

  PRUint32 urlCounter = 0;
  while (!mAlbumArtUrlSet.empty() && urlCounter < MAX_ARTWORK_URL_CHECK) {
    sbStringSetIter begin = mAlbumArtUrlSet.begin();
    rv = CheckAlbumArtCache(*begin);
    if (NS_FAILED(rv)) {
      // Warn that we failed to check/remove the file...
    }
    mAlbumArtUrlSet.erase(begin);
    urlCounter++;
  }

  // Save what is left in the set if anything
  rv = SaveSetToFile();
  NS_ENSURE_SUCCESS(rv, rv);
  
  if (!mAlbumArtUrlSet.empty()) {
    // Start the timer (if we have urls left)
    rv = mAlbumArtCleanTimer->Init(this, 
                                   ALBUM_ART_CACHE_CLEANUP_INTERVAL,
                                   nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


/**
 * Searches the library for items that have the same primaryImageURL as
 * aCacheURL and if no items exists removes that image from the cache.
 *
 * \param aCacheURL - String URL of artwork cache file to check if other items
 *                    reference it.
 */

nsresult
sbAlbumArtService::CheckAlbumArtCache(const nsAString& aCacheURL)
{
  TRACE(("sbAlbumArtService[0x%8.x] - CheckAlbumArtCache", this));
  NS_ENSURE_STATE(mMainLibraryList);
  nsresult rv;

  nsCOMPtr<nsIArray> items;
  rv = mMainLibraryList->GetItemsByProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                                            aCacheURL,
                                            getter_AddRefs(items));
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    rv = RemoveAlbumArtCache(aCacheURL);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (NS_SUCCEEDED(rv)) {
    PRUint32 itemCount;
    rv = items->GetLength(&itemCount);
    if (NS_SUCCEEDED(rv) && items == 0) {
      rv = RemoveAlbumArtCache(aCacheURL);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}


/**
 * Removes a file from the artwork cache folder as indicated by aCacheURL.
 *
 * \param aCacheURL - String URL of artwork cache to remove from the artwork
 *                    cache folder.
 */

nsresult
sbAlbumArtService::RemoveAlbumArtCache(const nsAString& aCacheURL)
{
  TRACE(("sbAlbumArtService[0x%8.x] - RemoveAlbumArtCache", this));
  // Turn the url into a nsIFile and remove
  nsresult rv;
  nsCOMPtr<nsIURI> pURI;
  
  // Convert to nsIURI
  rv = mIOService->NewURI(NS_ConvertUTF16toUTF8(aCacheURL),
                          nsnull,
                          nsnull,
                          getter_AddRefs(pURI));
  NS_ENSURE_SUCCESS(rv, rv);

  // With a file we can just grab the nsIFile and remove
  // Note: resource:// URIs are converted to file:// if possible by the QI.
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(pURI, &rv);
  if (NS_SUCCEEDED(rv)) {
    nsCOMPtr<nsIFile> file;
    rv = fileURL->GetFile(getter_AddRefs(file));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool isInCache = PR_FALSE;
    rv = mAlbumArtCacheDir->Contains(file, PR_FALSE, &isInCache);
    NS_ENSURE_SUCCESS(rv, rv);
    if (NS_SUCCEEDED(rv) && isInCache) {
      rv = file->Remove(PR_FALSE); // Should be a single file so no recursive.
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}
