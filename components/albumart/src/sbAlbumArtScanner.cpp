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

// Mozilla imports.
#include <nsArrayUtils.h>
#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIURI.h>
#include <nsIPrefBranch.h>
#include <nsIProgrammingLanguage.h>
#include <nsMemory.h>
#include <nsIMutableArray.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>
#include <nsThreadUtils.h>

// Songbird imports
#include <sbIAlbumArtDownloader.h>
#include <sbICascadeFilterSet.h>
#include <sbILibraryManager.h>
#include <sbIMediaList.h>
#include <sbIMediaListView.h>
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

//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------
NS_IMPL_THREADSAFE_ADDREF(sbAlbumArtScanner)
NS_IMPL_THREADSAFE_RELEASE(sbAlbumArtScanner)
NS_IMPL_QUERY_INTERFACE6_CI(sbAlbumArtScanner,
                            sbIAlbumArtScanner,
                            nsIClassInfo,
                            sbIJobProgress,
                            sbIJobCancelable,
                            nsITimerCallback,
                            sbIAlbumArtListener)
NS_IMPL_CI_INTERFACE_GETTER5(sbAlbumArtScanner,
                             sbIAlbumArtScanner,
                             sbIJobProgress,
                             sbIJobCancelable,
                             nsITimerCallback,
                             sbIAlbumArtListener)

NS_DECL_CLASSINFO(sbAlbumArtScanner)
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

  // If aMediaList is null then we need to grab the main library
  // TODO: Should we make a copy?
  if (aMediaList == nsnull) {
    nsCOMPtr<sbILibraryManager> libManager =
        do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> mLibrary;
    rv = libManager->GetMainLibrary(getter_AddRefs(mLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    mMediaList = do_QueryInterface(mLibrary, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    mMediaList = aMediaList;
  }

  // Now create a view and get the filter set so that we can filter/sort the items
  // Create the view
  nsCOMPtr<sbIMediaListView> mediaListView;
  rv = mMediaList->CreateView(nsnull, getter_AddRefs(mediaListView));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the filterset for this view
  rv = mediaListView->GetCascadeFilterSet(getter_AddRefs(mFilterSet));
  NS_ENSURE_SUCCESS(rv, rv);

  // Filter by albums
  rv = mFilterSet->AppendFilter(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                                &mAlbumFilterIndex);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get how many albums in this list
  rv = mFilterSet->GetValueCount(mAlbumFilterIndex,
                                 PR_FALSE,            // Do not use cache
                                 &mTotalAlbumCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Reset all the progress information
  mCompletedAlbumCount = 0;
  mCompletedItemCount = 0;
  mProcessNextAlbum = PR_TRUE;
  mProcessNextItem = PR_FALSE;

  // Update the progress and inform listeners
  UpdateProgress();

  // Start up the interval timer that will process the albums/items
  rv = mIntervalTimer->InitWithCallback(this,
                                        mIntervalTimerValue,
                                        nsITimer::TYPE_REPEATING_SLACK);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
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
      mCurrentAlbum.get(),
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

  *aProgress = mCompletedAlbumCount;
  return NS_OK;
}

/* readonly attribute unsigned long total; */
NS_IMETHODIMP sbAlbumArtScanner::GetTotal(PRUint32* aTotal)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - GetTotal", this));
  NS_ENSURE_ARG_POINTER( aTotal );
  NS_ASSERTION(NS_IsMainThread(), \
    "sbAlbumArtScanner::GetTotal is main thread only!");

  *aTotal = mTotalAlbumCount;
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
        mTimeoutTimer->Cancel();
        mCompletedAlbumCount++;
        mProcessNextAlbum = PR_TRUE;
      }
    } else  if (mProcessNextItem) {
      // Make sure we cancel the timeout timer
      mTimeoutTimer->Cancel();
      rv = ProcessItem();
      if (NS_FAILED(rv)) {
        TRACE(("sbAlbumArtScanner::Notify - Fetch Failed for item"));
        // Hmm, not good so we should skip this item
        mTimeoutTimer->Cancel();
        mCompletedItemCount++;
        mProcessNextItem = PR_TRUE;
      }
    }
  } else  if (aTimer == mTimeoutTimer) {
    // We have taken too long to find album art so skip this item.
    TRACE(("sbAlbumArtScanner::Notify - Timeout exceeded so moving to next item"));
    mFetcherSet->Shutdown();
    mCompletedItemCount++;
    mProcessNextItem = PR_TRUE;
  }
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// sbIAlbumArtListner Implementation.
//
//------------------------------------------------------------------------------

/* onChangeFetcher(in sbIAlbumArtFetcher aFetcher); */
NS_IMETHODIMP sbAlbumArtScanner::OnChangeFetcher(sbIAlbumArtFetcher* aFetcher)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - OnChangeFetcher", this));
  aFetcher->GetName(mCurrentFetcherName);
  UpdateProgress();
  return NS_OK;
}

/* onResult(in nsIURI aImageLocation, in sbIMediaItem aMediaItem); */
NS_IMETHODIMP sbAlbumArtScanner::OnResult(nsIURI* aImageLocation,
                                          sbIMediaItem* aMediaItem)
{
  TRACE(("sbAlbumArtScanner[0x%8.x] - OnResult", this));
  nsresult rv = NS_OK;

  // Cancel the timeout timer since we have an answer
  mTimeoutTimer->Cancel();

  // A null aImageLocation indicates a failure
  if (!aImageLocation) {
    // Log an error for this item
    TRACE(("sbAlbumArtScanner::OnResult - No image found for item."));
  } else {
    // Get the spec for the image location
    // if it is file then set the property and write metadata back.
    nsCAutoString scheme;
    rv = aImageLocation->GetScheme(scheme);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCAutoString imageFileURISpec;
    rv = aImageLocation->GetSpec(imageFileURISpec);
    NS_ENSURE_SUCCESS(rv, rv);

    if (scheme.EqualsLiteral("file") ||
        scheme.EqualsLiteral("chrome")) {
      TRACE(("sbAlbumArtScanner::OnResult - Setting primaryImageURL to %s",
             imageFileURISpec.get()));
      rv = aMediaItem->SetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                                   NS_ConvertUTF8toUTF16(imageFileURISpec));
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      TRACE(("sbAlbumArtScanner::OnResult - Not a local file so trying to download."));
      // If this is http, https, ftp, etc we need to try and download
      nsCOMPtr<sbIAlbumArtDownloader> albumArtDownloader =
          do_CreateInstance("@songbirdnest.com/Songbird/album-art/downloader;1",
                            &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = albumArtDownloader->DownloadImage(NS_ConvertUTF8toUTF16(imageFileURISpec),
                                             aMediaItem,
                                             this);
      return NS_OK;
    }
  }

  // Now that we are done this item move on to the next
  mCompletedItemCount++;
  mProcessNextItem = PR_TRUE;

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
  mTimeoutTimerValue(ALBUMART_SCANNER_TIMEOUT),
  mStatus(sbIJobProgress::STATUS_RUNNING),
  mCompletedAlbumCount(0),
  mTotalAlbumCount(0),
  mProcessNextAlbum(PR_FALSE),
  mCompletedItemCount(0),
  mTotalItemCount(0),
  mProcessNextItem(PR_FALSE),
  mMediaList(nsnull),
  mAlbumFilterIndex(0)
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
}

/**
 * Initialize the album art scanner.
 */

nsresult
sbAlbumArtScanner::Initialize()
{
  TRACE(("sbAlbumArtScanner[0x%.8x] - Initialize", this));
  nsresult rv = NS_OK;

  // Create our timers
  mIntervalTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mTimeoutTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get our timer values
  // Get the preference branch.
  nsCOMPtr<nsIPrefBranch>
    prefService = do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 prefType;
  rv = prefService->GetPrefType(PREF_ALBUMART_SCANNER_INTERVAL, &prefType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (prefType == nsIPrefBranch::PREF_INT) {
    rv = prefService->GetIntPref(PREF_ALBUMART_SCANNER_INTERVAL,
                                &mIntervalTimerValue);
  }
  rv = prefService->GetPrefType(PREF_ALBUMART_SCANNER_INTERVAL, &prefType);
  NS_ENSURE_SUCCESS(rv, rv);
  if (prefType == nsIPrefBranch::PREF_INT) {
    rv = prefService->GetIntPref(PREF_ALBUMART_SCANNER_TIMEOUT,
                                &mTimeoutTimerValue);
  }

  // Create our fetcher set
  mFetcherSet =
    do_CreateInstance("@songbirdnest.com/Songbird/album-art-fetcher-set;1", &rv);
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
    mProcessNextItem = PR_FALSE;
    mProcessNextAlbum = PR_FALSE;
    mIntervalTimer->Cancel();
    mTimeoutTimer->Cancel();
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
sbAlbumArtScanner::ProcessAlbum()
{
  TRACE(("sbAlbumArtScanner[0x%.8x] - ProcessAlbum", this));
  nsresult rv = NS_OK;

  // Clear our flags
  mProcessNextItem = PR_FALSE;
  mProcessNextAlbum = PR_FALSE;

  if (mCompletedAlbumCount < mTotalAlbumCount) {
    rv = mFilterSet->GetValueAt(mAlbumFilterIndex,
                                mCompletedAlbumCount,
                                mCurrentAlbum);
    NS_ENSURE_SUCCESS(rv, rv);

    TRACE(("sbAlbumArtScanner::ProcessAlbum - Scanning %s [%u of %u]",
            NS_ConvertUTF16toUTF8(mCurrentAlbum).get(),
            mCompletedAlbumCount,
            mTotalAlbumCount));

    // Get the list of items for this album
    mCurrentAlbumItemList = nsnull;
    rv = mMediaList->GetItemsByProperty(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME),
                                        mCurrentAlbum,
                                        getter_AddRefs(mCurrentAlbumItemList));
    NS_ENSURE_SUCCESS(rv, rv);
    mCompletedItemCount = 0;
    rv = mCurrentAlbumItemList->GetLength(&mTotalItemCount);
    NS_ENSURE_SUCCESS(rv, rv);

    // Trigger to get next item
    UpdateProgress();
    mProcessNextItem = PR_TRUE;
  } else {
    // We need to shut everything down.
    TRACE(("sbAlbumArtScanner::ProcessAlbum - All albums scanned."));
    mStatus = sbIJobProgress::STATUS_SUCCEEDED;
    UpdateProgress();
  }

  return NS_OK;
}

nsresult
sbAlbumArtScanner::ProcessItem()
{
  TRACE(("sbAlbumArtScanner[0x%.8x] - ProcessItem", this));
  nsresult rv = NS_OK;

  // Clear our flags
  mProcessNextItem = PR_FALSE;
  mProcessNextAlbum = PR_FALSE;

  // Now we try to get the album art for this item
  // This will be async so we have a timeout timer and
  // we listen for events on the fetcher
  if (mCompletedItemCount < mTotalItemCount) {
    TRACE(("sbAlbumArtScanner::ProcessItem - Checking for artwork for %s Item %u of %u",
            NS_ConvertUTF16toUTF8(mCurrentAlbum).get(),
            mCompletedItemCount,
            (mTotalItemCount - 1)));
    // Shutdown any existing fetching so we start clean
    rv = mFetcherSet->Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the next item
    nsCOMPtr<sbIMediaItem> nextItem;
    nextItem = do_QueryElementAt(mCurrentAlbumItemList,
                                 mCompletedItemCount,
                                 &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Clear the fetchername and update the progress
    mCurrentFetcherName.Truncate();
    UpdateProgress();

    // Get the current primaryImageURL value
    nsString currentArtwork;
    rv = nextItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_PRIMARYIMAGEURL),
                               currentArtwork);
    NS_ENSURE_SUCCESS(rv, rv);

    if (currentArtwork.IsEmpty()) {
      TRACE(("sbAlbumArtScanner::ProcessItem - Searching for artwork for %s Item %u of %u",
              NS_ConvertUTF16toUTF8(mCurrentAlbum).get(),
              mCompletedItemCount,
              (mTotalItemCount - 1)));
      // Start the timeouttimer
      mTimeoutTimer->Cancel();
      TRACE(("sbAlbumArtScanner::ProcessItem - Setting timeout for %u",
             mTimeoutTimerValue));
      rv = mTimeoutTimer->InitWithCallback(this,
                                           mTimeoutTimerValue,
                                           nsITimer::TYPE_ONE_SHOT);
      NS_ENSURE_SUCCESS(rv, rv);

      // Now search for the artwork
      rv = mFetcherSet->FetchAlbumArtForMediaItem(nextItem, this);
      NS_ENSURE_SUCCESS(rv, rv);
    } else {
      TRACE(("sbAlbumArtScanner::ProcessItem -  Artwork already set for %s Item %u",
              NS_ConvertUTF16toUTF8(mCurrentAlbum).get(),
              mCompletedItemCount));
      // This item does not need to be scanned so advance to the next one.
      mCompletedItemCount++;
      mProcessNextItem = PR_TRUE;
    }
  } else {
    // All items have been processed so move to the next album.
    TRACE(("sbAlbumArtScanner::ProcessItem - All Items in album scanned."));
    mCompletedAlbumCount++;
    mProcessNextAlbum = PR_TRUE;
  }

  return NS_OK;
}
