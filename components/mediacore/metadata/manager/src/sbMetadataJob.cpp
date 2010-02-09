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

/**
 * \file sbMetadataJob.cpp
 * \brief Implementation for sbMetadataJob.h
 */

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsAutoLock.h>
#include <pratom.h>

#include <nsArrayUtils.h>
#include <nsUnicharUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsCRTGlue.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsAutoPtr.h>
#include <nsThreadUtils.h>
#include <nsMemory.h>
#include <nsNetUtil.h>
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIURI.h>
#include <nsIIOService.h>
#include <nsIPrefService.h>
#include <nsIPrefBranch2.h>
#include <nsIStringBundle.h>

#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbILibraryResource.h>
#include <sbILibraryUtils.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbLocalDatabaseLibrary.h>
#include <sbILocalDatabaseMediaItem.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbStandardProperties.h>
#include <sbPropertiesCID.h>
#include <sbIAlbumArtFetcherSet.h>
#include <sbIWatchFolderService.h>

#include <sbStringBundle.h>
#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>

#include "sbIMetadataManager.h"
#include "sbIMetadataHandler.h"
#include "sbMetadataJob.h"
#include "sbMetadataJobItem.h"


// DEFINES ====================================================================

// Queue up 50 job items in mProcessedBackgroundThreadItems
// before calling BatchCompleteItems on the main thread
const PRUint32 NUM_BACKGROUND_ITEMS_BEFORE_FLUSH = 50;

typedef sbStringSet::iterator sbStringSetIter;

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#define TRACE(args) PR_LOG(gMetadataLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMetadataLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

// CLASSES ====================================================================

NS_IMPL_THREADSAFE_ISUPPORTS5(sbMetadataJob,
                              nsIClassInfo,
                              sbIJobProgress,
                              sbIJobProgressUI,
                              sbIJobCancelable,
                              sbIAlbumArtListener);

NS_IMPL_CI_INTERFACE_GETTER5(sbMetadataJob,
                             nsIClassInfo,
                             sbIJobProgress,
                             sbIJobProgressUI,
                             sbIJobCancelable,
                             sbIAlbumArtListener)

NS_DECL_CLASSINFO(sbMetadataJob)
NS_IMPL_THREADSAFE_CI(sbMetadataJob)

sbMetadataJob::sbMetadataJob() :
  mStatus(sbIJobProgress::STATUS_RUNNING),
  mBlocked(PR_FALSE),
  mCompletedItemCount(0),
  mTotalItemCount(0),
  mJobType(TYPE_READ),
  mLibrary(nsnull),
  mRequiredProperties(nsnull),
  mNextMainThreadIndex(0),
  mNextBackgroundThreadIndex(0),
  mBackgroundItemsLock(nsnull),
  mProcessedBackgroundThreadItems(nsnull),
  mProcessedBackgroundItemsLock(nsnull),
  mInLibraryBatch(PR_FALSE),
  mStringBundle(nsnull)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  MOZ_COUNT_CTOR(sbMetadataJob);
}

sbMetadataJob::~sbMetadataJob()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  MOZ_COUNT_DTOR(sbMetadataJob);
  
  // Make extra sure we aren't in a library batch
  EndLibraryBatch();
  
  if (mBackgroundItemsLock) {
    nsAutoLock::DestroyLock(mBackgroundItemsLock);
  }
  if (mProcessedBackgroundItemsLock) {
    nsAutoLock::DestroyLock(mProcessedBackgroundItemsLock);
  }
}


nsresult sbMetadataJob::Init(nsIArray *aMediaItemsArray,
                             nsIStringEnumerator* aRequiredProperties,
                             JobType aJobType)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaItemsArray);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::Init: MUST NOT INIT OFF OF MAIN THREAD!");
  nsresult rv;

  NS_ENSURE_FALSE(mBackgroundItemsLock, NS_ERROR_ALREADY_INITIALIZED);
  mBackgroundItemsLock = nsAutoLock::NewLock(
      "sbMetadataJob background item lock");
  NS_ENSURE_TRUE(mBackgroundItemsLock, NS_ERROR_OUT_OF_MEMORY);

  NS_ENSURE_FALSE(mProcessedBackgroundItemsLock, NS_ERROR_ALREADY_INITIALIZED);
  mProcessedBackgroundItemsLock = nsAutoLock::NewLock(
      "sbMetadataJob processed background items lock");
  NS_ENSURE_TRUE(mProcessedBackgroundItemsLock, NS_ERROR_OUT_OF_MEMORY);

  // Find the library for the items in the array.
  PRUint32 length;
  rv = aMediaItemsArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);  
  NS_ENSURE_TRUE(length > 0, NS_ERROR_INVALID_ARG);

  nsCOMPtr<sbIMediaItem> mediaItem =
    do_QueryElementAt(aMediaItemsArray, 0, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mediaItem->GetLibrary(getter_AddRefs(mLibrary));
  NS_ENSURE_SUCCESS(rv, rv);
  
  mJobType = aJobType;
  
  // Check if we have to remove any properties due to preferences
  if (mJobType == TYPE_WRITE) {
    NS_ENSURE_ARG_POINTER(aRequiredProperties);
    
    // Create a string array of the properties
    PRBool hasMore;
    rv = aRequiredProperties->HasMore(&hasMore);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsString propertyId;
    while (hasMore) {
      rv = aRequiredProperties->GetNext(propertyId);
      NS_ENSURE_SUCCESS(rv, rv);
      
      mRequiredProperties.AppendString(propertyId);
  
      rv = aRequiredProperties->HasMore(&hasMore);
      NS_ENSURE_SUCCESS(rv, rv);
   }

    // Figure out if we are allowed to write rating or artwork values to files
    PRBool enableRatingWrite = PR_FALSE;
    PRBool enableArtworkWrite = PR_FALSE;
    nsCOMPtr<nsIPrefBranch> prefService =
        do_GetService("@mozilla.org/preferences-service;1", &rv);
    NS_ENSURE_SUCCESS( rv, rv);
    prefService->GetBoolPref("songbird.metadata.ratings.enableWriting", 
                             &enableRatingWrite);
    prefService->GetBoolPref("songbird.metadata.artwork.enableWriting", 
                             &enableArtworkWrite);

    if (!enableRatingWrite) {
      // Remove the rating property if we are not allowed to write it out
      mRequiredProperties.RemoveString(NS_LITERAL_STRING(SB_PROPERTY_RATING));
    }
    if (!enableArtworkWrite) {
      // Remove the primaryImageURL property if we are not allowed to write it out
      mRequiredProperties.RemoveString(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL));      
    }
  }

  rv = AppendMediaItems(aMediaItemsArray);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mBackgroundThreadJobItems.Length() > 0) {
    BeginLibraryBatch();
  }

  return rv;
}


nsresult sbMetadataJob::AppendMediaItems(nsIArray *aMediaItemsArray)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaItemsArray);
  NS_ENSURE_STATE(mLibrary);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::AppendMediaItems is main thread only!");  
  nsresult rv;
  
  if (mStatus != sbIJobProgress::STATUS_RUNNING)
    return NS_ERROR_UNEXPECTED;
  
  PRUint32 length;
  rv = aMediaItemsArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(length > 0, NS_ERROR_INVALID_ARG);

  nsCOMPtr<sbIMediaItem> mediaItem;

  // Ensure that all the items belong to the same
  // library. This is necessary because batch operations
  // can only be performed on the library level.
  // Unfortunate.
  for (PRUint32 i=0; i < length; i++) {
    mediaItem = do_QueryElementAt(aMediaItemsArray, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> library;
    rv = mediaItem->GetLibrary(getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool equals;
    rv = library->Equals(mLibrary, &equals);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!equals) {
      NS_ERROR("Not all items from the same library");
      return NS_ERROR_INVALID_ARG;
    }
  }

  // If this job type is a write, inform the watch folder service of the paths
  // that are about to be written to. This prevents the watch folder service 
  // from re-reading in the changes that this job is about to do.
  PRBool shouldIgnorePaths = PR_FALSE;
  nsCOMPtr<sbIWatchFolderService> wfService;
  if (mJobType == TYPE_WRITE) {
    wfService = do_GetService("@songbirdnest.com/watch-folder-service;1", &rv);
    if (NS_SUCCEEDED(rv) && wfService) {
      rv = wfService->GetIsRunning(&shouldIgnorePaths);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), 
          "Could not determine if watchfolders is running!");
    }
  }

  // Now that we know the input is fine, add it to our 
  // list of things to be done
  
  mTotalItemCount += length;  
  
  // We have to keep two lists, one for items that 
  // must be processed on the main thread, and one for 
  // items that can be processed on a background thread.
  
  // Avoid allocating both lists, since most jobs
  // will be entirely main thread or entirely background thread
  PRBool resizedMainThreadItems = PR_FALSE;
  PRBool resizedBackgroundThreadItems = PR_FALSE;

  for (PRUint32 i=0; i < length; i++) {
    mediaItem = do_QueryElementAt(aMediaItemsArray, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // If the watch folder service is running, append this path to the ignore
    // list on the service. The path will be removed from the ignore list after
    // the import job has completed.
    if (shouldIgnorePaths) {
      nsString mediaItemPath;
      rv = mediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL), 
                                  mediaItemPath);
      if (NS_SUCCEEDED(rv)) {
        rv = wfService->AddIgnorePath(mediaItemPath);
        if (NS_SUCCEEDED(rv)) {
          mIgnoredContentPaths.insert(mediaItemPath);
        }
        else {
          NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
              "Could not add a watch folder ignore path!");
        }
      }
    }

    nsRefPtr<sbMetadataJobItem> jobItem = 
        new sbMetadataJobItem(mJobType, mediaItem, &mRequiredProperties, this);
    NS_ENSURE_TRUE(jobItem, NS_ERROR_OUT_OF_MEMORY);
    
    // Set up the handler now, as this must be done on the main thread
    // and only the handler knows whether it can be processed
    // in the background.
    rv = SetUpHandlerForJobItem(jobItem);
    
    if (NS_FAILED(rv)) {
      // If we couldn't get a handler, instant FAIL
      HandleFailedItem(jobItem);
      mCompletedItemCount++;
    } else {
      
      // Find out if the handler can be run off the main thread
      PRBool requiresMainThread = PR_TRUE;
      nsCOMPtr<sbIMetadataHandler> handler;
      rv = jobItem->GetHandler(getter_AddRefs(handler));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = handler->GetRequiresMainThread(&requiresMainThread);
      NS_ASSERTION(NS_SUCCEEDED(rv), \
        "sbMetadataJob::AppendMediaItems GetRequiresMainThread failed");

      // Separate the list of items based on main thread requirements.
      if (requiresMainThread) {

        // If we haven't allocated space on the main thread queue yet,
        // do so now. Better to waste space than append.
        if (!resizedMainThreadItems) {
          resizedMainThreadItems = 
            mMainThreadJobItems.SetCapacity(mTotalItemCount);
          NS_ENSURE_TRUE(resizedMainThreadItems, NS_ERROR_OUT_OF_MEMORY);          
        }
        
        mMainThreadJobItems.AppendElement(jobItem);
      } else {
        nsAutoLock lock(mBackgroundItemsLock);
        
        // If we haven't allocated space on the background thread queue yet,
        // do so now. Better to waste space than append.
        if (!resizedBackgroundThreadItems) {
          resizedBackgroundThreadItems = 
            mBackgroundThreadJobItems.SetCapacity(mTotalItemCount);
          NS_ENSURE_TRUE(resizedBackgroundThreadItems, NS_ERROR_OUT_OF_MEMORY);
        }
        
        mBackgroundThreadJobItems.AppendElement(jobItem);
      }
    }
  }
  
  return NS_OK;
}


nsresult 
sbMetadataJob::SetUpHandlerForJobItem(sbMetadataJobItem* aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::SetUpHandlerForJobItem is main thread only!");  
  nsresult rv;
  
  // First lets get the URL for this media item
  nsCOMPtr<sbIMediaItem> mediaItem;
  rv = aJobItem->GetMediaItem(getter_AddRefs(mediaItem));
  NS_ENSURE_SUCCESS(rv, rv);
  nsString stringURL;
  rv = mediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                              stringURL);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = aJobItem->SetURL(NS_ConvertUTF16toUTF8(stringURL));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Now see if we can find a handler for this URL
  nsCOMPtr<sbIMetadataManager> metadataManager =
    do_GetService("@songbirdnest.com/Songbird/MetadataManager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIMetadataHandler> handler;
  rv = metadataManager->GetHandlerForMediaURL(stringURL,
                                              getter_AddRefs(handler));
                                               
  if (rv == NS_ERROR_UNEXPECTED) {
    // No handler found, so try getting a handler for originURL if available
    rv = mediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                                stringURL);
    NS_ENSURE_SUCCESS(rv, rv);
    if (stringURL.IsEmpty()) {
      // No origin URL, too bad - restore the error
      rv = NS_ERROR_UNEXPECTED;
    } else {
      // Only use origin url if it's a local file.
      // Unfortunately, we can't touch the IO service since this can run on
      // a background thread.
      if (StringBeginsWith(stringURL, NS_LITERAL_STRING("file://"))) {        
        // TODO GetFileSize will fail, since it wont use this URL        
        rv = metadataManager->GetHandlerForMediaURL(stringURL,
                                                    getter_AddRefs(handler));
      } else {
        // Nope, can't use non-file origin url
        rv = NS_ERROR_UNEXPECTED;
      }
    }
  }
  #if PR_LOGGING
  if (NS_FAILED(rv)) {
    LOG(("Failed to find metadata handler\n"));
  }
  #endif /* PR_LOGGING */
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Finally, stick the handler into the jobitem 
  rv = aJobItem->SetHandler(handler);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return rv;
}


nsresult sbMetadataJob::GetQueuedItem(PRBool aMainThreadOnly, 
                                      sbMetadataJobItem** aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  nsresult rv;

  // Make sure the job is still running
  if (mStatus != sbIJobProgress::STATUS_RUNNING) {
    TRACE(("%s[%.8x]: not running", __FUNCTION__, this));
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsRefPtr<sbMetadataJobItem> item = nsnull;
  
  if (aMainThreadOnly) {
    // Nobody should be requesting main thread only items off of
    // the main thread
    NS_ASSERTION(NS_IsMainThread(), \
      "sbMetadataJob::GetQueuedItem: main thread item off main thread!");
    
    if (mNextMainThreadIndex < mMainThreadJobItems.Length()) {
      mMainThreadJobItems[mNextMainThreadIndex++].swap(item);
    } else {
      TRACE(("%s[%.8x] - NOT_AVAILABLE (%i/%i)\n",
             __FUNCTION__, this,
             mNextMainThreadIndex, mMainThreadJobItems.Length()));
      return NS_ERROR_NOT_AVAILABLE;
    }
  } else {
    // Background list must be locked, as theoretically
    // more items could be appended (though in the current impl 
    // this does not happen), and because GetStatusText
    // needs to access it to find a file name.
    nsAutoLock lock(mBackgroundItemsLock);
    
    if (mNextBackgroundThreadIndex < mBackgroundThreadJobItems.Length()) {
      mBackgroundThreadJobItems[mNextBackgroundThreadIndex++].swap(item);
    } else {
      TRACE(("%s[%.8x] - background NOT_AVAILABLE (%i/%i)\n",
             __FUNCTION__, this,
             mNextBackgroundThreadIndex, mBackgroundThreadJobItems.Length()));
      return NS_ERROR_NOT_AVAILABLE;
    }
  }
  NS_ENSURE_TRUE(item, NS_ERROR_FAILURE);
  
  // If this is a write job, we need to prepare the
  // list of properties that are to be written
  if (mJobType == TYPE_WRITE) {
    rv = PrepareWriteItem(item);
    
    if (!NS_SUCCEEDED(rv)) {
      NS_ERROR("sbMetadataJob::GetQueuedItem failed to prepare a "
               "job item for writing.");
      PutProcessedItem(item);
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  TRACE(("%s[%.8x]: got %.8x", __FUNCTION__, this, item.get()));  
  item.forget(aJobItem);
  return NS_OK;
}


nsresult sbMetadataJob::PutProcessedItem(sbMetadataJobItem* aJobItem)
{
  NS_ENSURE_ARG_POINTER(aJobItem);
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  
  // Make sure the job is still running
  if (mStatus != sbIJobProgress::STATUS_RUNNING) {
    LOG(("%s[%.8x] ignored. Job canceled\n", __FUNCTION__, this));
    return NS_OK;
  }
  
  // If we're being passed an item on the main thread, just complete
  // it immediately
  if (NS_IsMainThread()) {
    HandleProcessedItem(aJobItem);
  } else {
    // On a background thread, so can't complete the item just yet.
    DeferProcessedItemHandling(aJobItem);
  }
  return NS_OK;
}


nsresult sbMetadataJob::PrepareWriteItem(sbMetadataJobItem *aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  NS_ASSERTION(mJobType == TYPE_WRITE, \
               "sbMetadataJob::PrepareWriteItem called during a read job");
  nsresult rv;

  // Get the properties from the meta data job item
  nsCOMPtr<sbIMutablePropertyArray> writeProps;
  rv = aJobItem->GetProperties(getter_AddRefs(writeProps));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMetadataHandler> handler;
  rv = aJobItem->GetHandler(getter_AddRefs(handler));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Set properties on the handler
  rv = handler->SetProps(writeProps);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}


nsresult sbMetadataJob::HandleProcessedItem(sbMetadataJobItem *aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::HandleProcessedItem is main thread only!");
  nsresult rv;

  mCompletedItemCount++;

  // For read items, filter properties from the handler
  // and set them on the media item
  if (mJobType == TYPE_READ) {
    rv = CopyPropertiesToMediaItem(aJobItem);
    NS_ASSERTION(NS_SUCCEEDED(rv), \
      "sbMetadataJob::HandleProcessedItem CopyPropertiesToMediaItem failed!");
  } else {
    // For write items, we need to check for failure. Also, we need to
    // update the content-length property if we did write to the file.
    PRBool processed = PR_FALSE;
    aJobItem->GetProcessed(&processed);
    if (!processed) {
      rv = HandleFailedItem(aJobItem);
      NS_ASSERTION(NS_SUCCEEDED(rv), \
        "sbMetadataJob::HandleProcessedItem HandleFailedItem failed!");
    }

    rv = HandleWrittenItem(aJobItem);
  }

  // Close the handler
  nsCOMPtr<sbIMetadataHandler> handler;
  rv = aJobItem->GetHandler(getter_AddRefs(handler));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = handler->Close();
  NS_ENSURE_SUCCESS(rv, rv);
  return rv;
}

nsresult sbMetadataJob::HandleWrittenItem(sbMetadataJobItem *aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  nsresult rv;

  nsCOMPtr<sbIMediaItem> item;
  rv = aJobItem->GetMediaItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIURI> contentURI;
  rv = item->GetContentSrc(getter_AddRefs(contentURI));
  NS_ENSURE_SUCCESS(rv, rv);

  /* If it's not a file, that's ok */
  nsCOMPtr<nsIFileURL> fileURL = do_QueryInterface(contentURI, &rv);
  if (NS_FAILED (rv))
    return NS_OK;

  nsCOMPtr<nsIFile> file;
  rv = fileURL->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 fileSize;
  rv = file->GetFileSize(&fileSize);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString strContentLength;
  AppendInt(strContentLength, fileSize);

  rv = item->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                         strContentLength);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbMetadataJob::DeferProcessedItemHandling(sbMetadataJobItem *aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);

  // Even if there is no other work to do, everything
  // needs to be completed on the main thread 
  // (may release nsStandardURLs, etc.)
  
  nsAutoLock lock(mProcessedBackgroundItemsLock);
  
  // Make sure the job is still running.  
  // We do this inside the lock, since Cancel() will 
  // lock the array before updating the status, and we
  // don't want a race condition.
  if (mStatus != sbIJobProgress::STATUS_RUNNING) {
    LOG(("%s[%.8x] - Job canceled\n", __FUNCTION__, this));
    return NS_OK;
  }
        
  // Make sure we've got somewhere to put the items
  if (!mProcessedBackgroundThreadItems) {
    mProcessedBackgroundThreadItems = 
      new nsTArray<nsRefPtr<sbMetadataJobItem> >(
        NUM_BACKGROUND_ITEMS_BEFORE_FLUSH * 2);
  }
  
  // Save it for later
  mProcessedBackgroundThreadItems->AppendElement(aJobItem);

  return NS_OK;
}


nsresult sbMetadataJob::CopyPropertiesToMediaItem(sbMetadataJobItem *aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::CopyPropertiesToMediaItem is main thread only!");
  nsresult rv;

  // Get the sbIMediaItem we're supposed to be updating
  nsCOMPtr<sbIMediaItem> item;
  rv = aJobItem->GetMediaItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a new array we're going to copy across
  nsCOMPtr<sbIMutablePropertyArray> newProps = 
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( trackNameKey, SB_PROPERTY_TRACKNAME );
  nsAutoString oldName;
  rv = item->GetProperty( trackNameKey, oldName );
  nsAutoString trackName;

  // Get the metadata properties that were found
  nsCOMPtr<sbIMetadataHandler> handler;
  rv = aJobItem->GetHandler(getter_AddRefs(handler));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbIMutablePropertyArray> props;
  PRUint32 propsLength = 0;
  rv = handler->GetProps(getter_AddRefs(props));
  
  // If we found properties, check to see if
  // we got a track name
  if (NS_SUCCEEDED(rv)) {
    NS_ENSURE_TRUE(props, NS_ERROR_UNEXPECTED);
    rv = props->GetLength(&propsLength);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = props->GetPropertyValue( trackNameKey, trackName );
    
    if (NS_FAILED(rv)) {
      rv = HandleFailedItem(aJobItem);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  } else {
    rv = HandleFailedItem(aJobItem);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Get the property manager because we love it so much
  nsCOMPtr<sbIPropertyManager> propMan =
    do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Update the title if needed
  PRBool defaultTrackname = trackName.IsEmpty() && !oldName.IsEmpty();
  // If the metadata read can't even find a song name,
  // AND THERE ISN'T ALREADY A TRACK NAME, cook one up off the url.
  if ( trackName.IsEmpty() && oldName.IsEmpty() ) {
    rv = CreateDefaultItemName(item, trackName);
    NS_ENSURE_SUCCESS(rv, rv);
    // And then add/change it
    if ( !trackName.IsEmpty() ) {
      rv = AppendToPropertiesIfValid( propMan, newProps, trackNameKey, trackName );
      NS_ENSURE_SUCCESS(rv, rv);
      defaultTrackname = PR_TRUE;
    }
  }
  
  // Loop through the returned props to copy to the new props
  for (PRUint32 i = 0; i < propsLength && NS_SUCCEEDED(rv); i++) {
    nsCOMPtr<sbIProperty> prop;
    rv = props->GetPropertyAt( i, getter_AddRefs(prop) );
    if (NS_FAILED(rv))
      break;
    nsAutoString id, value;
    prop->GetId( id );
    if (!defaultTrackname || !id.Equals(trackNameKey)) {
      prop->GetValue( value );
      if (!value.IsEmpty() && !value.IsVoid() && !value.EqualsLiteral(" ")) {
        AppendToPropertiesIfValid( propMan, newProps, id, value );
      }
    }
  }

  PRBool isLocalFile = PR_FALSE;

  PRInt64 fileSize = 0;
  rv = GetFileSize(item, &fileSize);
  if (NS_SUCCEEDED(rv)) {
    nsAutoString contentLength;
    AppendInt(contentLength, fileSize);
    rv = AppendToPropertiesIfValid(propMan,
                                   newProps,
                                   NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                                   contentLength);
    NS_ENSURE_SUCCESS(rv, rv);
    
    isLocalFile = PR_TRUE;
  }

  rv = item->SetProperties(newProps);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isLocalFile){
    // For local files we want to trigger an album art lookup.
    // Do this AFTER setting properties, since the fetchers
    // may make use of the metadata.
    rv = ReadAlbumArtwork(aJobItem);
    NS_ASSERTION(NS_SUCCEEDED(rv), 
        "Metadata job failed to run album art fetcher");
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// sbIAlbumArtListner Implementation.
//
//------------------------------------------------------------------------------

/* onChangeFetcher(in sbIAlbumArtFetcher aFetcher); */
NS_IMETHODIMP sbMetadataJob::OnChangeFetcher(sbIAlbumArtFetcher* aFetcher)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  // Ignoring this, it should only be metadata fetching by default
  return NS_OK;
}

/* onTrackResult(in nsIURI aImageLocation, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP sbMetadataJob::OnTrackResult(nsIURI*       aImageLocation,
                                           sbIMediaItem* aMediaItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aMediaItem);

  if (aImageLocation) {
    nsresult rv;
    nsCAutoString imageFileURISpec;
    rv = aImageLocation->GetSpec(imageFileURISpec);
    if (NS_SUCCEEDED(rv)) {
      rv = aMediaItem->SetProperty(
              NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
              NS_ConvertUTF8toUTF16(imageFileURISpec));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

/* onAlbumResult(in nsIURI aImageLocation, in nsIArray aMediaItems); */
NS_IMETHODIMP sbMetadataJob::OnAlbumResult(nsIURI*    aImageLocation,
                                           nsIArray*  aMediaItems)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  // We only called FetchAlbumArtForTrack so we should not get this.
  return NS_OK;
}

/* onSearchComplete(in nsIArray aMediaItems); */
NS_IMETHODIMP sbMetadataJob::OnSearchComplete(nsIArray* aMediaItems)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  // We don't write back here since we are in the import and don't want to take
  // to much time.
  return NS_OK;
}

nsresult sbMetadataJob::ReadAlbumArtwork(sbMetadataJobItem *aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  nsresult rv;

  if (!mArtFetcher) {
    mArtFetcher =
      do_CreateInstance("@songbirdnest.com/Songbird/album-art-fetcher-set;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mArtFetcher->SetFetcherType(sbIAlbumArtFetcherSet::TYPE_LOCAL);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  // Recycle the existing handler to avoid re-reading the file
  nsCOMPtr<nsIMutableArray> sources =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIMetadataHandler> handler;
  rv = aJobItem->GetHandler(getter_AddRefs(handler));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = sources->AppendElement(handler, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mArtFetcher->SetAlbumArtSourceList(sources);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Attempt to fetch some art
  nsCOMPtr<sbIMediaItem> item;
  rv = aJobItem->GetMediaItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mArtFetcher->FetchAlbumArtForTrack(item, this);
  NS_ENSURE_SUCCESS(rv, rv);
  
  TRACE(("%s[%.8x] - finished rv %08x\n", __FUNCTION__, this, rv));

  return NS_OK;
}


nsresult sbMetadataJob::HandleFailedItem(sbMetadataJobItem *aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::HandleFailedItem is main thread only!");
  nsresult rv;
  
  // Get the content URI as escaped UTF8
  nsCAutoString escapedURI, unescapedURI;
  rv = aJobItem->GetURL(escapedURI);
  NS_ENSURE_SUCCESS(rv, rv);

  // Now unescape it
  nsCOMPtr<nsINetUtil> netUtil =
    do_GetService("@mozilla.org/network/util;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = netUtil->UnescapeString(escapedURI, 
                               nsINetUtil::ESCAPE_ALL,
                               unescapedURI);
  NS_ENSURE_SUCCESS(rv, rv);
  nsString stringURL = NS_ConvertUTF8toUTF16(unescapedURI);
  
  // No need to lock, since errors are always accessed on the main 
  // thread.
  
  // Now add the final string into the list of error messages
  mErrorMessages.AppendElement(stringURL);
  
  // If this is a read job, then failed items should get filename
  // as the trackname (this avoids blank rows, and makes it easier 
  // to tell what failed)
  if (mJobType == TYPE_READ) {
    PRInt32 slash = stringURL.RFind(NS_LITERAL_STRING("/"));
    if (slash > 0 && slash < (PRInt32)(stringURL.Length() - 1)) {
      stringURL = nsDependentSubstring(stringURL, slash + 1,
                                       stringURL.Length() - slash - 1);
    }
    
    nsCOMPtr<sbIMediaItem> item;
    rv = aJobItem->GetMediaItem(getter_AddRefs(item));
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = item->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME), stringURL);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


nsresult sbMetadataJob::BatchCompleteItems() 
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::BatchCompleteItems is main thread only!");
  nsresult rv = NS_OK;
  
  // Are there items that need completing
  PRBool needBatchComplete = PR_FALSE;
  {
    nsAutoLock processedLock(mProcessedBackgroundItemsLock);
    if (mProcessedBackgroundThreadItems) {
      // If the list is full, or if we're finished
      // and still have a few things left to complete... 

      if (mProcessedBackgroundThreadItems->Length() > 
           NUM_BACKGROUND_ITEMS_BEFORE_FLUSH)
      {
        needBatchComplete = PR_TRUE;
      } else {
        // If there is nothing left in the waiting background
        // item list and there are items in the incoming list,
        // then process right away.  This is to handle
        // the end of the job.
        // Note must grab locks in the same order as Cancel()
        nsAutoLock waitingLock(mBackgroundItemsLock);
        if (mNextBackgroundThreadIndex > (mBackgroundThreadJobItems.Length() - 1) &&
            mProcessedBackgroundThreadItems->Length() > 0)
        {
          needBatchComplete = PR_TRUE;
        }
      }
    }
  }
  // If so, complete them all in a single batch
  if (needBatchComplete) {
   
    nsCOMPtr<sbIMediaListBatchCallback> batchCallback =
      new sbMediaListBatchCallback(&sbMetadataJob::RunLibraryBatch);
    NS_ENSURE_TRUE(batchCallback, NS_ERROR_OUT_OF_MEMORY);
    
    // If we are inside one mega-batch, just call complete directly.
    if (mInLibraryBatch) {
      rv = BatchCompleteItemsCallback();
    } else {
      // Otherwise run a small library batch
      rv = mLibrary->RunInBatchMode(batchCallback, static_cast<sbIJobProgress*>(this)); 
    }    
    
    NS_ENSURE_SUCCESS(rv, rv);  
  }
  return rv;
}

/* static */
nsresult sbMetadataJob::RunLibraryBatch(nsISupports* aUserData)
{
  TRACE(("%s [static]", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aUserData);
  sbMetadataJob* thisJob = static_cast<sbMetadataJob*>(
        static_cast<sbIJobProgress*>(aUserData));
  return thisJob->BatchCompleteItemsCallback();
}

nsresult sbMetadataJob::BatchCompleteItemsCallback()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  nsresult rv = NS_OK;
  
  // Ok, phew, we're finally inside a library batch.
  
  // Steal the background items array and replace it with a new one.
  // This way we don't block the background thread from pushing
  // more completed items.  
  
  nsAutoPtr<nsTArray<nsRefPtr<sbMetadataJobItem> > > items;
  
  { // Scope the lock
    nsAutoLock lock(mProcessedBackgroundItemsLock);
    NS_ENSURE_STATE(mProcessedBackgroundThreadItems);
    
    items = mProcessedBackgroundThreadItems.forget();

    // Make sure there is lots of room in the new list
    mProcessedBackgroundThreadItems = 
      new nsTArray<nsRefPtr<sbMetadataJobItem> >(
        NUM_BACKGROUND_ITEMS_BEFORE_FLUSH * 2);
  }
    
  // Now go complete everything.
  for (PRUint32 i = 0; i < items->Length(); i++) {
    HandleProcessedItem((*items)[i]);
  }
  return rv;
}


nsresult sbMetadataJob::BeginLibraryBatch() 
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_STATE(mLibrary);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::BeginLibraryBatch is main thread only!");
  nsresult rv = NS_OK;
  
  if (mInLibraryBatch) {
    // Already in a batch
    return NS_OK;
  }
  
  // If this is a local db library, we can get an extra
  // perf increase by starting a batch transaction for
  // the entire job.
  nsCOMPtr<sbILocalDatabaseLibrary> localLibrary = 
    do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv,rv);

  localLibrary->ForceBeginUpdateBatch();    
  mInLibraryBatch = PR_TRUE;
  
  return NS_OK;
}

nsresult sbMetadataJob::EndLibraryBatch() 
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_STATE(mLibrary);  
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::EndLibraryBatch is main thread only!");
  nsresult rv = NS_OK;

  if (!mInLibraryBatch) {
    // Nothing to do, not in a batch
    return NS_OK;
  }
  
  nsCOMPtr<sbILocalDatabaseLibrary> localLibrary = 
    do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv,rv);
  
  localLibrary->ForceEndUpdateBatch();
  mInLibraryBatch = PR_FALSE;

  return NS_OK;
}


nsresult sbMetadataJob::OnJobProgress() 
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::OnJobProgress is main thread only!");
  nsresult rv = NS_OK;
  
  // See if there are any items needing main thread completion
  BatchCompleteItems();
  
  // Now figure out where we're at
  if (mCompletedItemCount == mTotalItemCount) {
    mStatus = (mErrorMessages.Length() == 0) ? 
        (PRInt16)sbIJobProgress::STATUS_SUCCEEDED : 
        (PRInt16)sbIJobProgress::STATUS_FAILED;
  }
  
  // Then announce our status to the world
  for (PRInt32 i = mListeners.Count() - 1; i >= 0; --i) {
     mListeners[i]->OnJobProgress(this);
  }
  
  if (mStatus != sbIJobProgress::STATUS_RUNNING) { 
    // We're done. No more notifications needed.
    mListeners.Clear();
    
    EndLibraryBatch();

    // If there are any paths in the ignore path set, remove them from the
    // watchfolders ignore whitelist.
    if (mIgnoredContentPaths.size() > 0) {
      nsCOMPtr<sbIWatchFolderService> wfService =
        do_GetService("@songbirdnest.com/watch-folder-service;1", &rv);
      NS_ASSERTION(NS_SUCCEEDED(rv),
          "Could not get the watch folder service!");
      if (NS_SUCCEEDED(rv) && wfService) {
        sbStringSetIter begin = mIgnoredContentPaths.begin();
        sbStringSetIter end = mIgnoredContentPaths.end();
        sbStringSetIter next;
        for (next = begin; next != end; ++next) {
          rv = wfService->RemoveIgnorePath(*next);
          NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
              "Could not remove a ignored watch folder path!");
        }
      }

      mIgnoredContentPaths.clear();
    }

    // Flush the library to ensure that search and filtering 
    // work immediately.
    rv = mLibrary->Flush();
    NS_ASSERTION(NS_SUCCEEDED(rv), 
      "sbMetadataJob::OnJobProgress failed to flush library!");
  }
  return NS_OK;
}

nsresult sbMetadataJob::GetType(JobType* aJobType)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aJobType );
  *aJobType = mJobType;
  return NS_OK;
}

nsresult sbMetadataJob::SetBlocked(PRBool aBlocked)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  mBlocked = aBlocked;
  return NS_OK;
}


// =====================================
// ==  sbIJobProgress Implementation  ==
// =====================================


/* readonly attribute unsigned short status; */
NS_IMETHODIMP sbMetadataJob::GetStatus(PRUint16* aStatus)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aStatus );
  *aStatus = mStatus;
  return NS_OK;
}

/* readonly attribute boolean blocked; */
NS_IMETHODIMP sbMetadataJob::GetBlocked(PRBool* aBlocked)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aBlocked );
  *aBlocked = mBlocked;
  return NS_OK;
}

/* readonly attribute unsigned AString statusText; */
NS_IMETHODIMP sbMetadataJob::GetStatusText(nsAString& aText)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::GetStatusText is main thread only!");
  nsresult rv = NS_OK;
  
  // If the job is still running, report the leaf file name
  // for the next item to be processed
  if (mStatus == sbIJobProgress::STATUS_RUNNING) {
    
    nsCOMPtr<sbIMediaItem> mediaItem;
    
    // First check the main thread lists, since it doesn't require locking
    if (mNextMainThreadIndex < mMainThreadJobItems.Length()) {
      rv = mMainThreadJobItems[mNextMainThreadIndex]->GetMediaItem(
              getter_AddRefs(mediaItem));
      NS_ENSURE_SUCCESS(rv, rv);
    } else {

      // Ok, nothing in the main thread list, so we need to lock
      // and access the background list
      nsAutoLock lock(mBackgroundItemsLock);
      if (mNextBackgroundThreadIndex < mBackgroundThreadJobItems.Length()) {
        rv = mBackgroundThreadJobItems[mNextBackgroundThreadIndex]->
                                       GetMediaItem(getter_AddRefs(mediaItem));
        NS_ENSURE_SUCCESS(rv, rv);
      }      
    }

    if (mediaItem) {
      // Get the unescaped file name
      CreateDefaultItemName(mediaItem, aText);
    } else {
      aText = mStatusText;
    }
    
  } else if (mStatus == sbIJobProgress::STATUS_FAILED) {
    // If we've failed, give a localized explanation.
    nsAutoString text;
    
    // Only set the status text for write jobs, since read jobs
    // are not currently reflected in the UI.
    if (mJobType == TYPE_WRITE) {
      const char* textKey;

      // Single failure in single item write job
      if (mTotalItemCount == 1) {
        textKey = "metadatajob.writing.failed.one";
      // Single error in multiple file write job
      } else if (mErrorMessages.Length() == 1) {
        textKey = "metadatajob.writing.failed.oneofmany";
      // Multiple errors in multiple file write job
      } else {
        textKey = "metadatajob.writing.failed.manyofmany";
      }

      aText = sbStringBundle().Get(textKey, "Job Failed");
    }
  
  } else {
    rv = LocalizeString(
          NS_LITERAL_STRING("media_scan.complete"),
          aText);
  } 
  
  mStatusText = aText;
  return rv;
}

/* readonly attribute AString titleText; */
NS_IMETHODIMP sbMetadataJob::GetTitleText(nsAString& aText)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  nsresult rv;
  
  // If we haven't figured out a title yet, get one from the 
  // string bundle.
  if (mTitleText.IsEmpty()) {
    TRACE(("sbMetadataJob::GetTitleText[0x%.8x] localizing string", this));
    if (mJobType == TYPE_WRITE) {
      rv = LocalizeString(NS_LITERAL_STRING("metadatajob.writing.title"), 
                          mTitleText);
      if (NS_FAILED(rv)) {
        mTitleText.AssignLiteral("Metadata Write Job");
      }
    } else {
      rv = LocalizeString(NS_LITERAL_STRING("metadatajob.reading.title"), 
                          mTitleText);
      if (NS_FAILED(rv)) {
        mTitleText.AssignLiteral("Metadata Read Job");
      }
    }
  }
  
  aText = mTitleText;
  return NS_OK;
}

/* readonly attribute unsigned long progress; */
NS_IMETHODIMP sbMetadataJob::GetProgress(PRUint32* aProgress)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aProgress );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::GetProgress is main thread only!");

  *aProgress = mNextBackgroundThreadIndex + mNextMainThreadIndex;
  return NS_OK;
}

/* readonly attribute unsigned long total; */
NS_IMETHODIMP sbMetadataJob::GetTotal(PRUint32* aTotal)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aTotal );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::GetTotal is main thread only!");

  *aTotal = mTotalItemCount;
  return NS_OK;
}

/* readonly attribute unsigned long errorCount; */
NS_IMETHODIMP sbMetadataJob::GetErrorCount(PRUint32* aCount)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aCount );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::GetErrorCount is main thread only!");

  // No locking needed since mErrorMessages is only updated on 
  // the main thread
  *aCount = mErrorMessages.Length();
  return NS_OK;
}

/* nsIStringEnumerator getErrorMessages(); */
NS_IMETHODIMP sbMetadataJob::GetErrorMessages(nsIStringEnumerator** aMessages)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMessages);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::GetProgress is main thread only!");

  *aMessages = nsnull;

  // No locking needed since mErrorMessages is only updated on 
  // the main thread
  nsCOMPtr<nsIStringEnumerator> enumerator = 
    new sbTArrayStringEnumerator(&mErrorMessages);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);
  
  enumerator.forget(aMessages);
  return NS_OK;
}

/* void addJobProgressListener( in sbIJobProgressListener aListener ); */
NS_IMETHODIMP
sbMetadataJob::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::AddJobProgressListener is main thread only!");
  
  PRInt32 index = mListeners.IndexOf(aListener);
  if (index >= 0) {
    // the listener already exists, do not re-add
    return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
  }
  PRBool succeeded = mListeners.AppendObject(aListener);
  return succeeded ? NS_OK : NS_ERROR_FAILURE;
}

/* void removeJobProgressListener( in sbIJobProgressListener aListener ); */
NS_IMETHODIMP
sbMetadataJob::RemoveJobProgressListener(sbIJobProgressListener* aListener)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::RemoveJobProgressListener is main thread only!");

  PRInt32 indexToRemove = mListeners.IndexOf(aListener);
  if (indexToRemove < 0) {
    // Err, no such listener
    return NS_ERROR_UNEXPECTED;
  }

  // remove the listener
  PRBool succeeded = mListeners.RemoveObjectAt(indexToRemove);
  NS_ENSURE_TRUE(succeeded, NS_ERROR_FAILURE);

  return NS_OK;
}

// =======================================
// ==  sbIJobProgressUI Implementation  ==
// =======================================

/* readonly attribute DOMString crop; */
NS_IMETHODIMP sbMetadataJob::GetCrop(nsAString & aCrop)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  aCrop.AssignLiteral("center");
  return NS_OK;
}

// =======================================
// ==  sbIJobCancelable Implementation  ==
// =======================================

/* boolean canCancel; */
NS_IMETHODIMP sbMetadataJob::GetCanCancel(PRBool* _retval)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  *_retval = PR_TRUE;
  return NS_OK;
}

/* void cancel(); */
NS_IMETHODIMP sbMetadataJob::Cancel()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbMetadataJob::Cancel is main thread only!");

  // Cancel!
  // Don't worry about items that are off being processed.
  // Just clear out all work and raise a flag indicating
  // that we're done.  The processing will finish and
  // the items will be thrown away.
  
  // Clear main thread items
  mMainThreadJobItems.Clear();
  mNextMainThreadIndex = 0;
  
  // Scope locks for background thread items
  {
    // Must lock in the same order as BatchCompleteItemsCallback.
    nsAutoLock processedLock(mProcessedBackgroundItemsLock);
    nsAutoLock waitingLock(mBackgroundItemsLock);
    
    // Set cancelled inside of the lock to make sure
    // there is no race between checking the cancelled flag
    // and accessing the arrays
    mStatus = sbIJobProgress::STATUS_FAILED;
    
    // Clear background thread items    
    mBackgroundThreadJobItems.Clear();
    mNextBackgroundThreadIndex = 0;
    
    // Clear items that were processed in the background
    // and are waiting to be completed
    if (mProcessedBackgroundThreadItems) {
      mProcessedBackgroundThreadItems->Clear();
    }
  }

  // Let everyone else know
  OnJobProgress();

  // The sbFileMetadataService will notice that we are complete
  // and remove us from its list.
  return NS_OK;
}


// =========================
// ==  Utility Functions  ==
// =========================

nsresult sbMetadataJob::CreateDefaultItemName(sbIMediaItem* aItem,
                                              nsAString& retval)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ASSERTION(NS_IsMainThread(), 
    "sbMetadataJob::CreateDefaultItemName is main thread only!");
  NS_ENSURE_ARG_POINTER(aItem);
  nsresult rv;
  
  // Get the content URL
  nsCOMPtr<nsIURI> uri;
  rv = aItem->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(uri, &rv);
  nsString filename;
  if (NS_SUCCEEDED(rv) && fileUrl) {
    nsCOMPtr<nsIFile> sourceFile, canonicalFile;
    rv = fileUrl->GetFile(getter_AddRefs(sourceFile));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibraryUtils> libraryUtils =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = libraryUtils->GetCanonicalPath(sourceFile,
                                        getter_AddRefs(canonicalFile));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = canonicalFile->GetLeafName(filename);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    nsCOMPtr<nsIURL> url = do_QueryInterface(uri, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the filename as UTF8
    nsCAutoString escapedURI, unescapedURI;
    rv = url->GetFileName(escapedURI);
    NS_ENSURE_SUCCESS(rv, rv);

    // Now unescape it
    nsCOMPtr<nsINetUtil> netUtil =
      do_GetService("@mozilla.org/network/util;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = netUtil->UnescapeString(escapedURI,
                                 nsINetUtil::ESCAPE_ALL,
                                 unescapedURI);
    NS_ENSURE_SUCCESS(rv, rv);

    filename = NS_ConvertUTF8toUTF16(unescapedURI);
  }

  PRInt32 dot = filename.RFind(NS_LITERAL_STRING("."));
  if (dot > 0 && dot < (PRInt32)(filename.Length() - 1)) {
    retval = nsDependentSubstring(filename, 0, dot);
  }
  else {
    retval = filename;
  }

  return NS_OK;
}


nsresult
sbMetadataJob::AppendToPropertiesIfValid(sbIPropertyManager* aPropertyManager,
                                         sbIMutablePropertyArray* aProperties,
                                         const nsAString& aID,
                                         const nsAString& aValue)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ASSERTION(aPropertyManager, "aPropertyManager is null");
  NS_ASSERTION(aProperties, "aProperties is null");

  nsCOMPtr<sbIPropertyInfo> info;
  nsresult rv = aPropertyManager->GetPropertyInfo(aID, getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isValid = PR_FALSE;
  rv = info->Validate(aValue, &isValid);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isValid) {
    rv = aProperties->AppendProperty(aID, aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


nsresult
sbMetadataJob::GetFileSize(sbIMediaItem* aMediaItem, PRInt64* aFileSize)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  NS_ENSURE_ARG_POINTER(aFileSize);
  NS_ASSERTION(NS_IsMainThread(), "GetFileSize is main thread only!");

  nsresult rv;
  nsCOMPtr<nsIURI> uri;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  // If this is not a file url, just return the error
  nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(uri, &rv);
  if (rv == NS_ERROR_NO_INTERFACE) {
    // If this isn't a local file and we can't get the filesize
    // that's fine.  Don't NS_WARN about it.
    return rv;
  }
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFile> file;
  rv = fileUrl->GetFile(getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  return file->GetFileSize(aFileSize);;
}


nsresult 
sbMetadataJob::LocalizeString(const nsAString& aName, nsAString& aValue)
{
  TRACE(("%s[%.8x] (%s)", __FUNCTION__, this,
         NS_LossyConvertUTF16toASCII(aName).get()));
  nsresult rv = NS_OK;
  
  if (!mStringBundle) {
    nsCOMPtr<nsIStringBundleService> StringBundleService =
      do_GetService("@mozilla.org/intl/stringbundle;1", &rv );
    NS_ENSURE_SUCCESS(rv, rv);

    rv = StringBundleService->CreateBundle(
         "chrome://songbird/locale/songbird.properties",
         getter_AddRefs(mStringBundle));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  nsString name(aName);
  nsString value;
  rv = mStringBundle->GetStringFromName(name.get(),
                                        getter_Copies(value));
  if (NS_FAILED(rv)) {
    NS_ERROR("sbMetadataJob::LocalizeString failed to find a string");
  }
  aValue = value;
  return rv;
}
