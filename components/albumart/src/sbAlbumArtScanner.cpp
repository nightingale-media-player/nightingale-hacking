/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *
 *=BEGIN SONGBIRD LICENSE
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * For information about the licensing and copyright of this Add-On please
 * contact POTI, Inc. at customer@songbirdnest.com.
 *
 *=END SONGBIRD LICENSE
 *
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird album art scanner.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtScanner.cpp
 * \brief Songbird Album Art Scanner Source.
 */

//------------------------------------------------------------------------------
//
// Songbird album art scanner imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbAlbumArtScanner.h"
#include "sbAlbumArtCommon.h"

// Mozilla imports.
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIURI.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsThreadUtils.h>

// Songbird imports
#include <sbICascadeFilterSet.h>
#include <sbIFilterableMediaListView.h>
#include <sbILibraryManager.h>
#include <sbILibraryConstraints.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
#include <sbIPropertyArray.h>
#include <sbISortableMediaListView.h>
#include <sbPrefBranch.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbAlbumArtScanner:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */
#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gAlbumArtScannerLog = nsnull;
#define TRACE(args) PR_LOG(gAlbumArtScannerLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gAlbumArtScannerLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

#ifndef kNotFound
#define kNotFound -1
#endif

//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_CLASSINFO(sbAlbumArtScanner, NULL, nsIClassInfo::THREADSAFE, SB_ALBUMARTSCANNER_CID);

NS_IMPL_ISUPPORTS7(sbAlbumArtScanner,
                   sbIAlbumArtScanner,
                   nsIClassInfo,
                   sbIJobProgress,
                   sbIJobProgressUI,
                   sbIJobCancelable,
                   nsITimerCallback,
                   sbIAlbumArtListener);

NS_IMPL_CI_INTERFACE_GETTER6(sbAlbumArtScanner,
                             sbIAlbumArtScanner,
                             sbIJobProgress,
                             sbIJobProgressUI,
                             sbIJobCancelable,
                             nsITimerCallback,
                             sbIAlbumArtListener);

NS_IMPL_THREADSAFE_CI(sbAlbumArtScanner)


//------------------------------------------------------------------------------
//
// sbIAlbumArtScanner implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Scan a media list for items missing album artwork and for each item
 *        scan the fetchers to find artwork for the items.
 * \param aMediaList list to scan items for missing artwork.
 */
NS_IMETHODIMP
sbAlbumArtScanner::ScanListForArtwork(sbIMediaList* aMediaList)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - ScanListForArtwork", this));
  nsresult rv = NS_OK;

  nsCOMPtr<sbIMediaList> mediaList = aMediaList;

  // If aMediaList is null then we need to grab the main library
  // TODO: Should we make a copy?
  if (aMediaList == nsnull) {
    nsCOMPtr<sbILibraryManager> libManager =
        do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> mLibrary;
    rv = libManager->GetMainLibrary(getter_AddRefs(mLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    mediaList = do_QueryInterface(mLibrary, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Now create a view and get the filter set so that we can filter/sort the
  // items
  rv = mediaList->CreateView(nsnull, getter_AddRefs(mMediaListView));
  NS_ENSURE_SUCCESS(rv, rv);

  // Filter out the lists (it would be nice to filter out the items that
  // already have primaryImageURL set)
  nsCOMPtr<sbIFilterableMediaListView> filterView =
    do_QueryInterface(mMediaListView, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // get the old constraints
  nsCOMPtr<sbILibraryConstraint> constraint;
  rv = filterView->GetFilterConstraint(getter_AddRefs(constraint));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibraryConstraintBuilder> builder =
    do_CreateInstance("@songbirdnest.com/Songbird/Library/ConstraintBuilder;1",
                      &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // push the original constraints into it if there's an existing one
  if (constraint) {
    rv = builder->IncludeConstraint(constraint, nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = builder->Intersect(nsnull);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Add the isList and Hidden filters
  rv = builder->Include(NS_LITERAL_STRING(SB_PROPERTY_ISLIST),
                        NS_LITERAL_STRING("0"),
                        nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = builder->Intersect(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = builder->Include(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN),
                        NS_LITERAL_STRING("0"),
                        nsnull);
                        
  // Only look up audio tracks, and not video / podcasts.
  rv = builder->Intersect(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = builder->Include(NS_LITERAL_STRING(SB_PROPERTY_CONTENTTYPE),
                        NS_LITERAL_STRING("audio"),
                        nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Now reset the constraint on the view
  rv = builder->Get(getter_AddRefs(constraint));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = filterView->SetFilterConstraint(constraint);
  NS_ENSURE_SUCCESS(rv, rv);

  // Now sort the items
  // Sort by:
  //  AlbumName       - Asc
  //  AlbumArtistName - Asc
  //  ArtistName      - Asc
  //  Disc Number     - Asc
  //  Track Number    - Asc
  nsCOMPtr<sbIMutablePropertyArray> newSort =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = newSort->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // AlbumName
  rv = newSort->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                               NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  // AlbumArtistName
  rv = newSort->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMARTISTNAME),
                               NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  // ArtistName
  rv = newSort->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                               NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Disc Number
  rv = newSort->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_DISCNUMBER),
                               NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Track Number
  rv = newSort->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER),
                               NS_LITERAL_STRING("a"));
  NS_ENSURE_SUCCESS(rv, rv);

  // Now set the sort on this list
  nsCOMPtr<sbISortableMediaListView> sortable =
    do_QueryInterface(mMediaListView, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sortable->SetSort(newSort);
  NS_ENSURE_SUCCESS(rv, rv);

  // Reset all the progress information
  rv = mMediaListView->GetLength(&mTotalItemCount);
  NS_ENSURE_SUCCESS(rv, rv);

  mCompletedItemCount = 0;
  mProcessNextAlbum = PR_TRUE;
  
  // Update the progress and inform listeners
  UpdateProgress();

  // Start up the interval timer that will process the albums/items
  rv = mIntervalTimer->InitWithCallback(this,
                                        mIntervalTimerValue,
                                        nsITimer::TYPE_REPEATING_SLACK);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

//
// Getters/setters.
//

/**
 * \brief If true, update any artwork already present in media items.
 */

NS_IMETHODIMP
sbAlbumArtScanner::GetUpdateArtwork(PRBool* _retval)
{
  TRACE(("%s[%.8x] = %d", __FUNCTION__, this, mUpdateArtwork));
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = mUpdateArtwork;
  return NS_OK;
}

NS_IMETHODIMP
sbAlbumArtScanner::SetUpdateArtwork(PRBool aUpdateArtwork)
{
  TRACE(("%s[%.8x] = %d", __FUNCTION__, this, aUpdateArtwork));
  mUpdateArtwork = aUpdateArtwork;
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// sbIJobProgress implementation.
//
//------------------------------------------------------------------------------

/* readonly attribute unsigned short status; */
NS_IMETHODIMP sbAlbumArtScanner::GetStatus(PRUint16* aStatus)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetStatus", this));
  NS_ENSURE_ARG_POINTER( aStatus );
  *aStatus = mStatus;
  return NS_OK;
}

/* readonly attribute boolean blocked; */
NS_IMETHODIMP sbAlbumArtScanner::GetBlocked(PRBool* aBlocked)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetBlocked", this));
  NS_ENSURE_ARG_POINTER( aBlocked );
  *aBlocked = PR_FALSE;
  return NS_OK;
}

/* readonly attribute unsigned AString statusText; */
NS_IMETHODIMP sbAlbumArtScanner::GetStatusText(nsAString& aText)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetStatusText", this));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::GetStatusText is main thread only!");
  nsresult rv;

  if (mStatus == sbIJobProgress::STATUS_RUNNING) {
    nsresult rv = NS_OK;
    nsString outMessage;
    nsString stringKey;

    const PRUnichar *strings[2] = {
      mCurrentAlbumName.get(),
      mCurrentFetcherName.get()
    };
    if (mCurrentFetcherName.IsEmpty()) {
      stringKey.AssignLiteral("albumart.scanning.nofetcher.message");
    } else {
      stringKey.AssignLiteral("albumart.scanning.fetcher.message");
    }    
    rv = mStringBundle->FormatStringFromName(stringKey.get(),
                                             strings,
                                             NS_ARRAY_LENGTH(strings),
                                             getter_Copies(outMessage));

    if (NS_FAILED(rv)) {
      aText.Assign(stringKey);
    } else {
      aText.Assign(outMessage);
    }
  } else {
    rv = mStringBundle->GetStringFromName(
                  NS_LITERAL_STRING("albumart.scanning.completed").get(),
                  getter_Copies(mTitleText));
    if (NS_FAILED(rv)) {
      aText.AssignLiteral("albumart.scanning.completed");
    }
  }
  return NS_OK;
}

/* readonly attribute AString titleText; */
NS_IMETHODIMP sbAlbumArtScanner::GetTitleText(nsAString& aText)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetTitleText", this));
  nsresult rv;

  // If we haven't figured out a title yet, get one from the
  // string bundle.
  if (mTitleText.IsEmpty()) {
    rv = mStringBundle->GetStringFromName(
                  NS_LITERAL_STRING("albumart.scanning.title").get(),
                  getter_Copies(mTitleText));
    if (NS_FAILED(rv)) {
      mTitleText.AssignLiteral("albumart.scanning.title");
    }
  }

  aText = mTitleText;
  return NS_OK;
}


/* readonly attribute unsigned long progress; */
NS_IMETHODIMP sbAlbumArtScanner::GetProgress(PRUint32* aProgress)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetProgress", this));
  NS_ENSURE_ARG_POINTER( aProgress );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::GetProgress is main thread only!");

  *aProgress = mCompletedItemCount;
  return NS_OK;
}

/* readonly attribute unsigned long total; */
NS_IMETHODIMP sbAlbumArtScanner::GetTotal(PRUint32* aTotal)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetTotal", this));
  NS_ENSURE_ARG_POINTER( aTotal );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::GetTotal is main thread only!");

  if (mTotalItemCount > 1) {
    *aTotal = mTotalItemCount;
  } else {
    *aTotal = 0; // indeterminate
  }
  return NS_OK;
}

/* readonly attribute unsigned long errorCount; */
NS_IMETHODIMP sbAlbumArtScanner::GetErrorCount(PRUint32* aCount)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetErrorCount", this));
  NS_ENSURE_ARG_POINTER( aCount );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::GetErrorCount is main thread only!");

  *aCount = mErrorMessages.Length();
  return NS_OK;
}

/* nsIStringEnumerator getErrorMessages(); */
NS_IMETHODIMP sbAlbumArtScanner::GetErrorMessages(nsIStringEnumerator** aMessages)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetErrorMessages", this));
  NS_ENSURE_ARG_POINTER(aMessages);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::GetProgress is main thread only!");

  *aMessages = nsnull;

  nsCOMPtr<nsIStringEnumerator> enumerator =
    new sbTArrayStringEnumerator(&mErrorMessages);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  enumerator.forget(aMessages);
  return NS_OK;
}

/* void addJobProgressListener( in sbIJobProgressListener aListener ); */
NS_IMETHODIMP
sbAlbumArtScanner::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - AddJobProgressListener", this));
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::AddJobProgressListener is main thread only!");

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
sbAlbumArtScanner::RemoveJobProgressListener(sbIJobProgressListener* aListener)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - RemoveJobProgressListener", this));
  NS_ENSURE_ARG_POINTER(aListener);
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::RemoveJobProgressListener is main thread only!");

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

//------------------------------------------------------------------------------
//
// sbIJobProgressUI Implementation.
//
//------------------------------------------------------------------------------

/* attribute DOMString crop; */
NS_IMETHODIMP sbAlbumArtScanner::GetCrop(nsAString & aCrop)
{
  aCrop.AssignLiteral("end");
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// sbIJobCancelable Implementation.
//
//------------------------------------------------------------------------------

/* boolean canCancel; */
NS_IMETHODIMP sbAlbumArtScanner::GetCanCancel(PRBool* _retval)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetCanCancel", this));
  *_retval = PR_TRUE;
  return NS_OK;
}

/* void cancel(); */
NS_IMETHODIMP sbAlbumArtScanner::Cancel()
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - Cancel", this));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::Cancel is main thread only!");

  // Indicate that we have stopped and call UpdateProgress to take care of cleanup.
  mStatus = sbIJobProgress::STATUS_FAILED;
  UpdateProgress();

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// nsITimerCallback Implementation.
//
//------------------------------------------------------------------------------

/* notify(in nsITimer aTimer); */
NS_IMETHODIMP sbAlbumArtScanner::Notify(nsITimer* aTimer)
{
  NS_ENSURE_ARG_POINTER(aTimer);
  nsresult rv;

  if (aTimer == mIntervalTimer) {
    if (mProcessNextAlbum) {
      rv = ProcessAlbum();
      if (NS_FAILED(rv)) {
        TRACE(("sbAlbumArtScanner::Notify - Fetch Failed for album"));
        // Hmm, not good so we should skip this album
        mProcessNextAlbum = PR_TRUE;
      }
    }
  }
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// sbIAlbumArtListener Implementation.
//
//------------------------------------------------------------------------------

/* onChangeFetcher(in sbIAlbumArtFetcher aFetcher); */
NS_IMETHODIMP
sbAlbumArtScanner::OnChangeFetcher(sbIAlbumArtFetcher* aFetcher)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - OnChangeFetcher", this));
  mCurrentFetcher = aFetcher;
  aFetcher->GetName(mCurrentFetcherName);
  UpdateProgress();
  return NS_OK;
}

/* onTrackResult(in nsIURI aImageLocation, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP
sbAlbumArtScanner::OnTrackResult(nsIURI*       aImageLocation,
                                 sbIMediaItem* aMediaItem)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - OnResult", this));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  // If fetcher is remote, mark that an attempt was made to fetch remote art for
  // the item.
  if (mCurrentFetcher) {
    PRBool isLocal;
    rv = mCurrentFetcher->GetIsLocal(&isLocal);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!isLocal) {
      rv = MarkRemoteFetchAttempted(aMediaItem);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // A null aImageLocation indicates a failure
  if (aImageLocation) {
    rv = SetItemArtwork(aImageLocation, aMediaItem);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/* onAlbumResult(in nsIURI aImageLocation, in nsIArray aMediaItems); */
NS_IMETHODIMP
sbAlbumArtScanner::OnAlbumResult(nsIURI*    aImageLocation,
                                 nsIArray*  aMediaItems)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - OnAlbumResult", this));
  NS_ENSURE_ARG_POINTER(aMediaItems);
  nsresult rv;

  // If fetcher is remote, mark that an attempt was made to fetch remote art for
  // the items.
  if (mCurrentFetcher) {
    PRBool isLocal;
    rv = mCurrentFetcher->GetIsLocal(&isLocal);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!isLocal) {
      PRUint32 itemCount;
      rv = aMediaItems->GetLength(&itemCount);
      NS_ENSURE_SUCCESS(rv, rv);
      for (PRUint32 i = 0; i < itemCount; i++) {
        nsCOMPtr<sbIMediaItem>
          mediaItem = do_QueryElementAt(aMediaItems, i, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        rv = MarkRemoteFetchAttempted(mediaItem);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  // A null aImageLocation indicates a failure
  if (aImageLocation) {
    rv = SetItemsArtwork(aImageLocation, aMediaItems);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/* onSearchComplete(in nsIArray aMediaItems); */
NS_IMETHODIMP
sbAlbumArtScanner::OnSearchComplete(nsIArray* aMediaItems)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - OnSearchComplete", this));
  nsresult rv;

  // Done with the fetchers for now
  mCurrentFetcher = nsnull;

  // Now that we are done this album move on to the next
  mProcessNextAlbum = PR_TRUE;

  // Write the images to metadata
  if (aMediaItems) {
    rv = WriteImageMetadata(aMediaItems);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

/**
 * Construct an album art scanner instance.
 */

sbAlbumArtScanner::sbAlbumArtScanner() :
  mIntervalTimerValue(ALBUMART_SCANNER_INTERVAL),
  mUpdateArtwork(PR_FALSE),
  mStatus(sbIJobProgress::STATUS_RUNNING),
  mCompletedItemCount(0),
  mTotalItemCount(0),
  mProcessNextAlbum(PR_FALSE),
  mCurrentAlbumItemList(nsnull),
  mMediaListView(nsnull)
{
#ifdef PR_LOGGING
  if (!gAlbumArtScannerLog) {
    gAlbumArtScannerLog = PR_NewLogModule("sbAlbumArtScanner");
  }
#endif
  TRACE(("sbAlbumArtScanner[0x%.8x] - ctor", this));
  MOZ_COUNT_CTOR(sbAlbumArtScanner);
}

/**
 * Destroy an album art scanner instance.
 */

sbAlbumArtScanner::~sbAlbumArtScanner()
{
  TRACE(("sbAlbumArtScanner[0x%.8x] - dtor", this));
  MOZ_COUNT_DTOR(sbAlbumArtScanner);
  if (mIntervalTimer) {
    mIntervalTimer->Cancel();
    mIntervalTimer = nsnull;
  }
  mFetcherSet = nsnull;
  mCurrentFetcher = nsnull;
  mCurrentAlbumItemList = nsnull;
  mStringBundle = nsnull;
}

/**
 * Initialize the album art scanner.
 */

nsresult
sbAlbumArtScanner::Initialize()
{
  TRACE(("sbAlbumArtScanner[0x%.8x] - Initialize", this));
  nsresult rv = NS_OK;

  // Create our timer
  mIntervalTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get our timer values
  // Get the preference branch.
  sbPrefBranch prefBranch(PREF_ALBUMART_SCANNER_BRANCH, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mIntervalTimerValue = prefBranch.GetIntPref(PREF_ALBUMART_SCANNER_INTERVAL,
                                              ALBUMART_SCANNER_INTERVAL);

  // Create our fetcher set
  mFetcherSet =
    do_CreateInstance("@songbirdnest.com/Songbird/album-art-fetcher-set;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mFetcherSet->SetFetcherType(sbIAlbumArtFetcherSet::TYPE_ALL);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create an array for items in an album
  mCurrentAlbumItemList =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Grab our string bundle
  nsCOMPtr<nsIStringBundleService> StringBundleService =
    do_GetService("@mozilla.org/intl/stringbundle;1", &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StringBundleService->CreateBundle(
         "chrome://songbird/locale/songbird.properties",
         getter_AddRefs(mStringBundle));
  NS_ENSURE_SUCCESS(rv, rv);


  return rv;
}

//------------------------------------------------------------------------------
//
// Private services.
//
//------------------------------------------------------------------------------

nsresult
sbAlbumArtScanner::UpdateProgress()
{
  TRACE(("sbAlbumArtScanner[0x%.8x] - UpdateProgress", this));
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::UpdateProgress is main thread only!");

  if (mStatus != sbIJobProgress::STATUS_RUNNING) {
    // We are done so shut down, we do this before notifying the
    // listeners since they may take some time and we need to cancel
    // the timers as soon as possible.
    TRACE(("sbAlbumArtScanner::UpdateProgress - Shutting down Job"));
    mCurrentFetcher = nsnull;
    mProcessNextAlbum = PR_FALSE;
    mIntervalTimer->Cancel();
    mFetcherSet->Shutdown();
  }

  for (PRInt32 i = mListeners.Count() - 1; i >= 0; --i) {
    mListeners[i]->OnJobProgress(this);
  }

  if (mStatus != sbIJobProgress::STATUS_RUNNING) {
    // We're done. No more notifications needed.
    mListeners.Clear();
  }

  return NS_OK;
}

nsresult
sbAlbumArtScanner::GetNextAlbumItems()
{
  TRACE(("sbAlbumArtScanner[0x%.8x] - GetNextAlbumItems [%d/%d]",
         this,
         mCompletedItemCount,
         mTotalItemCount));
  nsresult rv;

  nsString mLastAlbumName;
  nsString mLastArtistName;

  // Clear the item list so we can start fresh
  mCurrentAlbumItemList->Clear();
  
  // Loop while we still have items and we haven't gotten to the next album
  // We need to check the albumName first with the previous one, then if that
  // matches we can check the albumArtist/artist with previous artist.
  // If all that matches then we are still on the same album and we do a check
  // to see if the image has already been found or not (since we can not filter
  // out non-null properties).
  while (mCompletedItemCount < mTotalItemCount) {
    TRACE(("sbAlbumArtScanner - Processing %d of %d",
           mCompletedItemCount,
           mTotalItemCount));

    nsCOMPtr<sbIMediaItem> item;
    rv = mMediaListView->GetItemByIndex(mCompletedItemCount,
                                        getter_AddRefs(item));
    if (NS_FAILED(rv)) {
      // Move on to the next item
      TRACE(("sbAlbumArtScanner - Processing : Item %d failed with %08X",
             mCompletedItemCount,
             rv));
      mCompletedItemCount++;
      continue;
    }
    TRACE(("sbAlbumArtScanner - Found Item %d", mCompletedItemCount));

    // We need an album name or this is completely pointless.
    nsString albumName;
    rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME), albumName);
    if (NS_FAILED(rv) || albumName.IsEmpty()) {
      // Move on to the next item
      mCompletedItemCount++;
      continue;
    }

    // Next we use either the albumArtistName (preferred) or artistName
    nsString albumArtistName;
    item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMARTISTNAME),
                      albumArtistName);
    // If the albumArtistName fails we will fall back to the artist name

    nsString artistName;
    if (!albumArtistName.IsEmpty()) {
      // Try to use the album artist name if possible
      artistName = albumArtistName;
    } else {
      rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME),
                             artistName);
      if (NS_FAILED(rv)) {
        mCompletedItemCount++;
        continue;
      }
    }
    
    if (artistName.IsEmpty()) {
      // No artist then how are we going to know who this album belongs to?
      mCompletedItemCount++;
      continue;
    }

#ifdef PR_LOGGING
    // Adding track name for debugging
    nsString trackName;
    rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME), trackName);
    NS_ENSURE_SUCCESS(rv, rv);
    TRACE(("sbAlbumArtScanner - Processing Track [%s] by [%s] from [%s]",
           NS_ConvertUTF16toUTF8(trackName).get(),
           NS_ConvertUTF16toUTF8(artistName).get(),
           NS_ConvertUTF16toUTF8(albumName).get()
           ));
#endif
    
    // if this is the first album then just set the Last* values and append the
    // item to the list
    if (mLastAlbumName.IsEmpty()) {
      mLastAlbumName.Assign(albumName);
      mCurrentAlbumName.Assign(albumName);
      mLastArtistName.Assign(artistName);
      TRACE(("sbAlbumArtScanner - First instance of album."));
    } else if (!mLastAlbumName.Equals(albumName)) {
      // if the album names are different then we definitly have a new album
      // so don't add this track or increment the index, just break out of the
      // loop.
      TRACE(("sbAlbumArtScanner - Sending album to be processed for album art."));
      break;
    } else {
      // Check if the artist is the same as the previous artist
      TRACE(("sbAlbumArtScanner - Checking artist: prev %s, current %s",
             NS_ConvertUTF16toUTF8(mLastArtistName).get(),
             NS_ConvertUTF16toUTF8(artistName).get()
             ));
      if (!mLastArtistName.Equals(artistName) &&
          (artistName.Find(mLastArtistName, PR_TRUE) == kNotFound) &&
          (mLastArtistName.Find(artistName, PR_TRUE) == kNotFound)) {
          // No substring so this must not be the same artist
          TRACE(("sbAlbumArtScanner - Sending album to be processed for album art."));
          break;
      }
    }

    // If not updating artwork, skip items that already have album art.
    if (!mUpdateArtwork) {
      nsString primaryImageUrl;
      rv = item->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                             primaryImageUrl);
      if (NS_FAILED(rv) || !primaryImageUrl.IsEmpty()) {
        TRACE(("sbAlbumArtScanner - Item already has cover."));
        // Move on to the next item
        mCompletedItemCount++;
        continue;
      }
    }
    
    TRACE(("sbAlbumArtScanner - Adding to list"));
    rv = mCurrentAlbumItemList->AppendElement(NS_ISUPPORTS_CAST(sbIMediaItem *,
                                                                item),
                                              PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    mCompletedItemCount++;
  }
  
  return NS_OK;
}

nsresult
sbAlbumArtScanner::ProcessAlbum()
{
  TRACE(("sbAlbumArtScanner[0x%.8x] - ProcessAlbum", this));
  nsresult rv = NS_OK;

  // Clear our flags
  mProcessNextAlbum = PR_FALSE;

  rv = GetNextAlbumItems();
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRUint32 trackCount = 0;
  rv = mCurrentAlbumItemList->GetLength(&trackCount);
  NS_ENSURE_SUCCESS(rv, rv);
  TRACE(("Collected %d of %d items, current list has %d items",
         mCompletedItemCount,
         mTotalItemCount,
         trackCount));
  if (trackCount > 0) {
    TRACE(("sbAlbumArtScanner::ProcessAlbum - Fetching artwork for items."));
    mCurrentFetcherName.Truncate();
    UpdateProgress();
    rv = mFetcherSet->FetchAlbumArtForAlbum(mCurrentAlbumItemList, this);
    NS_ENSURE_SUCCESS(rv, rv);
  } else if (mCompletedItemCount >= mTotalItemCount) {
    // We need to shut everything down.
    TRACE(("sbAlbumArtScanner::ProcessAlbum - All albums scanned."));
    mStatus = sbIJobProgress::STATUS_SUCCEEDED;
    UpdateProgress();
  } else {
    // We have items left but no items in mCurrentAlbumItemList, this probably
    // means we have all the artwork for this album, just move on to the next.
    UpdateProgress();
    mProcessNextAlbum = PR_TRUE;
  }

  return NS_OK;
}

nsresult
sbAlbumArtScanner::MarkRemoteFetchAttempted(sbIMediaItem* aMediaItem)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - MarkRemoteFetchAttempted", this));
  NS_ENSURE_ARG_POINTER(aMediaItem);
  nsresult rv;

  // Set attempted remote art fetch property if not already set.
  nsAutoString attemptedRemoteArtFetch;
  rv = aMediaItem->GetProperty
                     (NS_LITERAL_STRING(SB_PROPERTY_ATTEMPTED_REMOTE_ART_FETCH),
                      attemptedRemoteArtFetch);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!attemptedRemoteArtFetch.Equals(NS_LITERAL_STRING("1"))) {
    rv = aMediaItem->SetProperty
           (NS_LITERAL_STRING(SB_PROPERTY_ATTEMPTED_REMOTE_ART_FETCH),
            NS_LITERAL_STRING("1"));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

