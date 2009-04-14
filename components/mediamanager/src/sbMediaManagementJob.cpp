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
#include <sbStringBundle.h>
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

void
sbMediaManagementJob::SaveError(nsresult aErrorCode)
{
  // Save the errors in the error map
  sbErrorPairResult result = mErrorMap.insert(sbErrorPair(aErrorCode, 1));
  if (!result.second) {
    // result already exists so increment
    result.first->second++;
  }
}

PRBool
sbMediaManagementJob::AppendErrorToList(PRUint32 aErrorCount,
                                        nsString aErrorKey,
                                        nsTArray<nsString> &aErrorMessages)
{
  nsresult rv;
  
  nsString errorCount;
  errorCount.AppendInt(aErrorCount);

  nsTArray<nsString> params;
  params.AppendElement(errorCount);

  sbStringBundle bundle;
  nsString errorString = bundle.Format(NS_ConvertUTF16toUTF8(aErrorKey).get(),
                                       params);
  if (!errorString.IsEmpty()) {
    aErrorMessages.AppendElement(errorString);
  } else {
    return PR_FALSE;
  }
  
  return PR_TRUE;
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

  rv = ProcessItem(nextItem);
  if (NS_FAILED(rv)) {
    SaveError(rv);
  }
  
  // Increment our counter and check our status
  mCompletedItemCount++;
  if (mCompletedItemCount >= mTotalItemCount) {
    if ((mErrorMap.size()) > 0) {
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
sbMediaManagementJob::ProcessItem(sbIMediaItem* aItem)
{
  TRACE(("sbMediaManagementJob[0x%.8x] - ProcessItem", this));
  NS_ENSURE_ARG_POINTER(aItem);
  nsresult rv;
  
  nsString propValue;
  nsString propHidden;
  nsString propIsList;
  
  propHidden.AssignLiteral(SB_PROPERTY_HIDDEN);
  propIsList.AssignLiteral(SB_PROPERTY_ISLIST);
    
  PRBool isHidden =
    (NS_SUCCEEDED(aItem->GetProperty(propHidden, propValue)) &&
     propValue.EqualsLiteral("1"));
  PRBool isList =
    (NS_SUCCEEDED(aItem->GetProperty(propIsList, propValue)) &&
     propValue.EqualsLiteral("1"));

  if (isHidden || isList) {
    // We don't organize lists or hidden items
    return NS_OK;
  }
  
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
  
  // Get the path for the progress display
  rv = itemFile->GetPath(mCurrentContentURL);
  NS_ENSURE_SUCCESS(rv, rv);
  // Update the progress for the user
  rv = UpdateProgress();
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isInMediaFolder = PR_FALSE;
  rv = mMediaFolder->Contains(itemFile, PR_FALSE, &isInMediaFolder);
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 manageType = 0;
  if (mShouldMoveFiles) {
    // Move files so that they are in the correct folder structure
    manageType = manageType | sbIMediaFileManager::MANAGE_MOVE;
    TRACE(("sbMediaManagementJob - Adding MANAGE_MOVE"));
  }
  if (mShouldRenameFiles) {
    // Rename files so they have the correct filename
    manageType = manageType | sbIMediaFileManager::MANAGE_RENAME;
    TRACE(("sbMediaManagementJob - Adding MANAGE_RENAME"));
  }
  if (!isInMediaFolder && mShouldCopyFiles) {
    // Copy files to the media folder if not already there
    manageType = manageType | sbIMediaFileManager::MANAGE_COPY;
    TRACE(("sbMediaManagementJob - Adding MANAGE_COPY"));
  }
  // Check if there is anything to do for this item
  if (manageType == 0) {
    TRACE(("sbMediaManagementJob - No need to organize this file."));
    return NS_OK;
  }

  // Organize the file by calling the sbIMediaFileManager
  PRBool organizedItem;
  rv = mMediaFileManager->OrganizeItem(aItem,
                                       manageType,
                                       &organizedItem);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!organizedItem)
  {
    SaveError(NS_ERROR_FILE_COPY_OR_MOVE_FAILED);
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
  
  /**
   * Preferences - BEGIN
   */
  // Get the preference branch.
  nsCOMPtr<nsIPrefBranch2> prefBranch =
     do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Grab our Media Managed Folder
  mMediaFolder = nsnull;
  rv = prefBranch->GetComplexValue(PREF_MMJOB_LOCATION,
                                   NS_GET_IID(nsILocalFile),
                                   getter_AddRefs(mMediaFolder));
  if (NS_FAILED(rv) || mMediaFolder == nsnull) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  // Get flag to indicate if we are to copy files to the media folder
  mShouldCopyFiles = PR_FALSE;
  rv = prefBranch->GetBoolPref(PREF_MMJOB_COPYFILES, &mShouldCopyFiles);
  if (NS_FAILED(rv)) {
    mShouldCopyFiles = PR_FALSE;
  }
  
  // Get flag to indicate if we are to move files in the media folder
  mShouldMoveFiles = PR_FALSE;
  rv = prefBranch->GetBoolPref(PREF_MMJOB_MOVEFILES, &mShouldMoveFiles);
  if (NS_FAILED(rv)) {
    mShouldMoveFiles = PR_FALSE;
  }
  
  // Get flag to indicate if we are to rename files in the media folder
  mShouldRenameFiles = PR_FALSE;
  rv = prefBranch->GetBoolPref(PREF_MMJOB_RENAMEFILES, &mShouldRenameFiles);
  if (NS_FAILED(rv)) {
    mShouldRenameFiles = PR_FALSE;
  }
  
  // Get our timer value
  mIntervalTimerValue = MMJOB_SCANNER_INTERVAL;
  rv = prefBranch->GetIntPref(PREF_MMJOB_INTERVAL, &mIntervalTimerValue);
  if (NS_FAILED(rv)) {
    mIntervalTimerValue = MMJOB_SCANNER_INTERVAL;
  }
  /**
   * Preference - END
   */

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

  // Grab a Media File Manager component
  mMediaFileManager = do_CreateInstance(SB_MEDIAFILEMANAGER_CONTRACTID, &rv);
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
  sbStringBundle bundle;

  if (mStatus == sbIJobProgress::STATUS_RUNNING) {
    nsTArray<nsString> params;
    params.AppendElement(mCurrentContentURL);
    aText = bundle.Format("mediamanager.scanning.item.message", params);
  } else {
    aText = bundle.Get("mediamanager.scanning.completed");
  }

  return NS_OK;
}

/* readonly attribute AString titleText; */
NS_IMETHODIMP
sbMediaManagementJob::GetTitleText(nsAString& aText)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetTitleText", this));
  nsresult rv;
  sbStringBundle bundle;

  if (mStatus == sbIJobProgress::STATUS_RUNNING) {
    PRFloat64 percentDone =
      ((PRFloat64) mCompletedItemCount / (PRFloat64) mTotalItemCount) * 100;
    nsString percentString;
    AppendInt(percentString, percentDone);
    
    nsTArray<nsString> params;
    params.AppendElement(percentString);
    aText = bundle.Format("mediamanager.scanning.title", params);
  } else {
    aText = bundle.Get("mediamanager.scanning.completed");
  }

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

  *aCount = mErrorMap.size();
  return NS_OK;
}

/* nsIStringEnumerator getErrorMessages(); */
NS_IMETHODIMP
sbMediaManagementJob::GetErrorMessages(nsIStringEnumerator** aMessages)
{
  TRACE(("sbMediaManagementJob[0x%8.x] - GetErrorMessages", this));
  NS_ENSURE_ARG_POINTER(aMessages);

  nsTArray<nsString> errorMessages;
  *aMessages = nsnull;

  /**
   * Iterate through the grouped errors we have collected.
   */
  sbErrorMapIter begin = mErrorMap.begin();
  sbErrorMapIter end = mErrorMap.end();
  sbErrorMapIter currError;
  PRUint32 unknownCount = 0;
  nsString errorKeyBase;
  errorKeyBase.AssignLiteral("mediamanager.errors.");
  for (currError = begin; currError != end; ++currError++) {
    nsString errorKey;
    errorKey.Assign(errorKeyBase);
    AppendInt(errorKey, (*currError).first);
    TRACE(("sbMediaManagementJob: Lookup Error [%s]",
           NS_ConvertUTF16toUTF8(errorKey).get()));
    PRBool foundError = AppendErrorToList((*currError).second,
                                          errorKey,
                                          errorMessages);
    if (!foundError) {
      unknownCount += (*currError).second;
    }
  }
  
  // Append the unknown totals
  if (unknownCount > 0) {
    AppendErrorToList(unknownCount,
                      NS_LITERAL_STRING("mediamanager.errors.unknown"),
                      errorMessages);
  }

  nsCOMPtr<nsIStringEnumerator> enumerator =
    new sbTArrayStringEnumerator(&errorMessages);
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
