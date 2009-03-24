/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
// Songbird media management job.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMediaManagementJob.cpp
 * \brief Songbird Media Management Job Source.
 */

//------------------------------------------------------------------------------
//
// Songbird media management job imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbMediaManagementJob.h"

// Mozilla imports.
#include <nsComponentManagerUtils.h>
#include <nsIClassInfoImpl.h>
#include <nsIFileURL.h>
#include <nsILocalFile.h>
#include <nsIPrefBranch2.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>

// Songbird imports
#include <sbIMediaList.h>
#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <sbPrefBranch.h>
#include <sbStandardProperties.h>

NS_IMPL_THREADSAFE_ISUPPORTS5(sbMediaManagementJob,
                              sbIMediaManagementJob,
                              sbIJobProgress,
                              sbIJobProgressUI,
                              sbIJobCancelable,
                              nsITimerCallback)

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediaManagementJob:5
 * Use the following to output to a file:
 *   NSPR_LOG_FILE=path/to/file.log
 */
#include "prlog.h"
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediaManagementJobLog = nsnull;
#define TRACE(args) PR_LOG(gMediaManagementJobLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediaManagementJobLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

//------------------------------------------------------------------------------
//
// sbMediaManagementJob implementation.
//
//------------------------------------------------------------------------------

sbMediaManagementJob::sbMediaManagementJob() :
  mShouldIgnoreMediaFolder(PR_FALSE),
  mStatus(sbIJobProgress::STATUS_RUNNING),
  mCompletedItemCount(0),
  mTotalItemCount(0)
{
#ifdef PR_LOGGING
  if (!gMediaManagementJobLog) {
    gMediaManagementJobLog = PR_NewLogModule("sbMediaManagementJob");
  }
#endif
  TRACE(("sbMediaManagementJob[0x%.8x] - ctor", this));
  MOZ_COUNT_CTOR(sbMediaManagementJob);
}

sbMediaManagementJob::~sbMediaManagementJob()
{
  TRACE(("sbMediaManagementJob[0x%.8x] - dtor", this));
  MOZ_COUNT_DTOR(sbMediaManagementJob);
  if (mMediaFolder) {
    mMediaFolder = nsnull;
  }
  
  if (mWatchFolderService) {
    mWatchFolderService = nsnull;
  }
  
  if (mMediaList) {
    mMediaList = nsnull;
  }
  
  if (mMediaFolder) {
    mMediaFolder = nsnull;
  }
  
  if (mMediaFileManager) {
    mMediaFileManager = nsnull;
  }
  
  if (mIntervalTimer) {
    mIntervalTimer = nsnull;
  }
  
  if (mStringBundle) {
    mStringBundle = nsnull;
  }
}

/**
 * \brief UpdateProgress - Notify the listeners of the progress information for
 *  this job.
 */
nsresult
sbMediaManagementJob::UpdateProgress()
{
  TRACE(("sbMediaManagementJob[0x%.8x] - UpdateProgress", this));
  nsresult rv;
  
  if (mStatus != sbIJobProgress::STATUS_RUNNING) {
    TRACE(("sbMediaManagementJob::UpdateProgress - Shutting down Job"));
    mIntervalTimer->Cancel();
    mIntervalTimer = nsnull;

    // Remove the media folder from the ignore list in watch folders.
    if (mShouldIgnoreMediaFolder &&
        mMediaFolder &&
        mWatchFolderService)
    {
      nsString mediaFolderPath;
      rv = mMediaFolder->GetPath(mediaFolderPath);
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = mWatchFolderService->RemoveIgnorePath(mediaFolderPath);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  for (PRInt32 i = mListeners.Count() - 1; i >= 0; --i) {
    mListeners[i]->OnJobProgress(this);
  }

  return NS_OK;
}

/**
 * \brief ProcessNextItem - Organize the next item in our list.
 */
nsresult
sbMediaManagementJob::ProcessNextItem()
{
  TRACE(("sbMediaManagementJob[0x%.8x] - ProcessNextItem", this));
  nsresult rv;

  // Get the next item
  nsCOMPtr<sbIMediaItem> nextItem;
  rv = mMediaList->GetItemByIndex(mCompletedItemCount,
                                  getter_AddRefs(nextItem));
  // This is a serious error
  NS_ENSURE_SUCCESS(rv, rv);

  nsString filePath;
  rv = ProcessItem(nextItem, filePath);
  if (NS_FAILED(rv)) {
    // TODO: We need to log this error, we actually want to log common errors
    // so that we don't get thousands of files listed in the error dialog.
    mErrorMessages.AppendElement(filePath);
  }
  
  // Increment our counter and check our status
  mCompletedItemCount++;
  if (mCompletedItemCount >= mTotalItemCount) {
    if (mErrorMessages.Length() > 0) {
      mStatus = sbIJobProgress::STATUS_FAILED;
    } else {
      mStatus = sbIJobProgress::STATUS_SUCCEEDED;
    }
    rv = UpdateProgress();
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    rv = UpdateProgress();
    NS_ENSURE_SUCCESS(rv, rv);

    // Start up the interval timer that will process the next item
    rv = mIntervalTimer->InitWithCallback(this,
                                          mIntervalTimerValue,
                                          nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

nsresult
sbMediaManagementJob::ProcessItem(sbIMediaItem* aItem,
                                  nsAString& outFilePath)
{
  TRACE(("sbMediaManagementJob[0x%.8x] - ProcessItem", this));
  NS_ENSURE_ARG_POINTER(aItem);
  nsresult rv;
  
  // Reset our current url incase something fails
  mCurrentContentURL = EmptyString();

  nsCOMPtr<nsIURI> itemUri;
  rv = aItem->GetContentSrc(getter_AddRefs(itemUri));
  NS_ENSURE_SUCCESS (rv, rv);
  
  PRBool isFile;
  rv = itemUri->SchemeIs("file", &isFile);
  NS_ENSURE_SUCCESS(rv, rv);
  // If this is not a file then just return to go on to the next item
  if (!isFile) {
    return NS_ERROR_FILE_UNRECOGNIZED_PATH;
  }
  
  // Get an nsIFileURL object from the nsIURI
  nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(itemUri, &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the file object
  nsCOMPtr<nsIFile> itemFile;
  rv = fileUrl->GetFile(getter_AddRefs(itemFile));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isInMediaFolder = PR_FALSE;
  rv = mMediaFolder->Contains(itemFile, PR_FALSE, &isInMediaFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 manageType;
  if (isInMediaFolder) {
    manageType = sbIMediaFileManager::MANAGE_RENAME;
  } else {
    manageType = sbIMediaFileManager::MANAGE_COPY;
  }
    
  // Organize the file by calling the sbIMediaFileManager
  PRBool organizedItem;
  rv = mMediaFileManager->OrganizeItem(aItem,
                                       manageType,
                                       &organizedItem);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!organizedItem)
  {
    // Need to log this error somehow
    TRACE(("sbMediaManagementJob - Unable to organize item."));
  } else {
    TRACE(("sbMediaManagementJob - Organized item."));
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// nsITimerCallback Implementation.
//
//------------------------------------------------------------------------------

/* notify(in nsITimer aTimer); */
NS_IMETHODIMP
sbMediaManagementJob::Notify(nsITimer* aTimer)
{
  TRACE(("sbMediaManagementJob[0x%.8x] - Notify", this));
  NS_ENSURE_ARG_POINTER(aTimer);
  nsresult rv;

  if (aTimer == mIntervalTimer) {
    // Process next item
    rv = ProcessNextItem();
    if (NS_FAILED(rv)) {
      mStatus = sbIJobProgress::STATUS_FAILED;
      rv = UpdateProgress();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// sbIMediaManagementJob implementation.
//
//------------------------------------------------------------------------------

/* void organizeMediaList (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaManagementJob::OrganizeMediaList(sbIMediaList *aMediaList)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - OrganizeMediaList", this));
  NS_ENSURE_ARG_POINTER( aMediaList );
  nsresult rv;
  
  // Grab a Media File Manager component
  mMediaFileManager = do_CreateInstance(SB_MEDIAFILEMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the preference branch.
  nsCOMPtr<nsIPrefBranch2> prefBranch =
     do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
 
  // Grab our Media Managed Folder
  mMediaFolder = nsnull;
  rv = prefBranch->GetComplexValue(PREF_MMJOB_LOCATION,
                                   NS_GET_IID(nsILocalFile),
                                   getter_AddRefs(mMediaFolder));
  NS_ENSURE_SUCCESS(rv, rv);

  // Inform the watch folder service of the Media Library Folder we are about
  // to organize. This prevents the watch folder service from re-reading in the
  // changes that this job is about to do.
  mWatchFolderService =
    do_GetService("@songbirdnest.com/watch-folder-service;1", &rv);
  if (NS_SUCCEEDED(rv) && mWatchFolderService) {
    rv = mWatchFolderService->GetIsRunning(&mShouldIgnoreMediaFolder);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "Could not determine if watchfolders is running!");
  }
  
  if (mShouldIgnoreMediaFolder) {
    nsString mediaFolderPath;
    rv = mMediaFolder->GetPath(mediaFolderPath);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = mWatchFolderService->AddIgnorePath(mediaFolderPath);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Get flag to indicate if we are to copy files to the media folder
  rv = prefBranch->GetBoolPref(PREF_MMJOB_COPYFILES, &mCopyFilesToMediaFolder);
  if (NS_FAILED(rv)) {
    mCopyFilesToMediaFolder = PR_FALSE;
  }
  
  // Get our timer values
  rv = prefBranch->GetIntPref(PREF_MMJOB_INTERVAL, &mIntervalTimerValue);
  if (NS_FAILED(rv)) {
    mIntervalTimerValue = MMJOB_SCANNER_INTERVAL;
  }

  // Grab our string bundle
  nsCOMPtr<nsIStringBundleService> StringBundleService =
    do_GetService("@mozilla.org/intl/stringbundle;1", &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StringBundleService->CreateBundle(
         "chrome://songbird/locale/songbird.properties",
         getter_AddRefs(mStringBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Create our timer
  mIntervalTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  ///////////////////////////////////////

  // Hold on to the list.
  mMediaList = aMediaList;

  // Reset all the progress information
  rv = aMediaList->GetLength(&mTotalItemCount);
  NS_ENSURE_SUCCESS(rv, rv);
  mCompletedItemCount = 0;
  mStatus = sbIJobProgress::STATUS_RUNNING;
  mErrorMessages.Clear();
  
  // Update the progress
  rv = UpdateProgress();
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Fire off the first timer notify to process the first item
  rv = Notify(mIntervalTimer);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// sbIJobProgress implementation.
//
//------------------------------------------------------------------------------

/* readonly attribute unsigned short status; */
NS_IMETHODIMP
sbMediaManagementJob::GetStatus(PRUint16* aStatus)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetStatus", this));
  NS_ENSURE_ARG_POINTER( aStatus );
  *aStatus = mStatus;
  return NS_OK;
}

/* readonly attribute unsigned AString statusText; */
NS_IMETHODIMP
sbMediaManagementJob::GetStatusText(nsAString& aText)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetStatusText", this));
  nsresult rv;
  nsString outMessage;
  nsString stringKey;

  if (mStatus == sbIJobProgress::STATUS_RUNNING) {
    stringKey.AppendLiteral("mediamanager.scanning.item.message");

    const PRUnichar *strings[1] = {
      mCurrentContentURL.get()
    };
    rv = mStringBundle->FormatStringFromName(stringKey.get(),
                                             strings,
                                             NS_ARRAY_LENGTH(strings),
                                             getter_Copies(outMessage));
  } else {
    // TODO: Check for Completed (Success/failure) states and display appropriate
    // message.
    // Success: Display completed message
    // Failed : check for message count (> 0 = Failure, 0 = User cancelled)
    stringKey.AppendLiteral("mediamanager.scanning.completed");
    rv = mStringBundle->GetStringFromName(
                  stringKey.get(),
                  getter_Copies(outMessage));
  }

  if (NS_FAILED(rv)) {
    aText.Assign(stringKey);
  } else {
    aText.Assign(outMessage);
  }

  return NS_OK;
}

/* readonly attribute AString titleText; */
NS_IMETHODIMP
sbMediaManagementJob::GetTitleText(nsAString& aText)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetTitleText", this));
  nsresult rv;
  
  nsString titleString;
  
  // TODO: Check for Completed (Success/failure) states and display appropriate
  // message.
  // Success: Display completed message
  // Failed : check for message count (> 0 = Failure, 0 = User cancelled)
  
  // Figure out current percentage
  PRFloat64 percentDone =
    ((PRFloat64) mCompletedItemCount / (PRFloat64) mTotalItemCount) * 100;
  nsString percentString;
  percentString.AppendInt((PRUint32) percentDone);

  const PRUnichar *strings[1] = {
    percentString.get()
  };
  rv = mStringBundle->FormatStringFromName(NS_LITERAL_STRING("mediamanager.scanning.title").get(),
                                           strings,
                                           NS_ARRAY_LENGTH(strings),
                                           getter_Copies(titleString));
  if (NS_FAILED(rv)) {
    titleString.AssignLiteral("mediamanager.scanning.title");
  }

  aText = titleString;
  return NS_OK;
}


/* readonly attribute unsigned long progress; */
NS_IMETHODIMP
sbMediaManagementJob::GetProgress(PRUint32* aProgress)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetProgress", this));
  NS_ENSURE_ARG_POINTER( aProgress );

  *aProgress = mCompletedItemCount;
  return NS_OK;
}

/* readonly attribute unsigned long total; */
NS_IMETHODIMP
sbMediaManagementJob::GetTotal(PRUint32* aTotal)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetTotal", this));
  NS_ENSURE_ARG_POINTER( aTotal );

  // A 0 value makes the progress bar indeterminate
  *aTotal = (mTotalItemCount < 0 ? 0 : mTotalItemCount);
  return NS_OK;
}

/* readonly attribute unsigned long errorCount; */
NS_IMETHODIMP
sbMediaManagementJob::GetErrorCount(PRUint32* aCount)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetErrorCount", this));
  NS_ENSURE_ARG_POINTER( aCount );

  *aCount = mErrorMessages.Length();
  return NS_OK;
}

/* nsIStringEnumerator getErrorMessages(); */
NS_IMETHODIMP
sbMediaManagementJob::GetErrorMessages(nsIStringEnumerator** aMessages)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetErrorMessages", this));
  NS_ENSURE_ARG_POINTER(aMessages);

  *aMessages = nsnull;

  nsCOMPtr<nsIStringEnumerator> enumerator =
    new sbTArrayStringEnumerator(&mErrorMessages);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);

  enumerator.forget(aMessages);
  return NS_OK;
}

/* void addJobProgressListener( in sbIJobProgressListener aListener ); */
NS_IMETHODIMP
sbMediaManagementJob::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - AddJobProgressListener", this));
  NS_ENSURE_ARG_POINTER(aListener);

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
sbMediaManagementJob::RemoveJobProgressListener(sbIJobProgressListener* aListener)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - RemoveJobProgressListener", this));
  NS_ENSURE_ARG_POINTER(aListener);

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
NS_IMETHODIMP
sbMediaManagementJob::GetCrop(nsAString & aCrop)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetCrop", this));
  aCrop.AssignLiteral("center");
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// sbIJobCancelable Implementation.
//
//------------------------------------------------------------------------------

/* boolean canCancel; */
NS_IMETHODIMP
sbMediaManagementJob::GetCanCancel(PRBool* _retval)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetCanCancel", this));
  *_retval = PR_TRUE;
  return NS_OK;
}

/* void cancel(); */
NS_IMETHODIMP
sbMediaManagementJob::Cancel()
{
  TRACE(("sbMediaManagementJob[0x%8.x] - Cancel", this));

  // Indicate that we have stopped and call UpdateProgress to take care of
  // cleanup.
  mStatus = sbIJobProgress::STATUS_FAILED;
  return UpdateProgress();
}
