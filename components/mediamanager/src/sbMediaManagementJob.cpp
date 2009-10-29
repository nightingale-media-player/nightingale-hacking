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
#include <nsIPropertyBag2.h>
#include <nsIWritablePropertyBag2.h>
#include <nsMemory.h>
#include <nsServiceManagerUtils.h>
#include <nsStringGlue.h>

// Songbird imports
#include <sbIMediaList.h>
#include <sbIMediaManagementJobItem.h>
#include <sbStringBundle.h>
#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>
#include <sbPrefBranch.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>

// constants
// the maximum number of file details to show per type of error message
#define ERROR_DETAILS_MAX_COUNT 10

class sbMediaManagementJobItem : public sbIMediaManagementJobItem
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAMANAGEMENTJOBITEM
  
  sbMediaManagementJobItem(sbIMediaItem*, nsIFile*, PRUint16);
  ~sbMediaManagementJobItem();
protected:
  nsCOMPtr<sbIMediaItem> mItem;
  nsCOMPtr<nsIFile> mTargetPath;
  PRUint16 mAction;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMediaManagementJobItem,
                              sbIMediaManagementJobItem)

sbMediaManagementJobItem::sbMediaManagementJobItem(sbIMediaItem* aItem,
                                                   nsIFile* aTargetPath,
                                                   PRUint16 aAction)
  : mItem(aItem),
    mTargetPath(aTargetPath),
    mAction(aAction)
{
}

sbMediaManagementJobItem::~sbMediaManagementJobItem()
{
}

/* readonly attribute sbIMediaItem item; */
NS_IMETHODIMP
sbMediaManagementJobItem::GetItem(sbIMediaItem * *aItem)
{
  NS_ENSURE_ARG_POINTER(aItem);
  NS_IF_ADDREF(*aItem = mItem);
  return NS_OK;
}

/* readonly attribute nsIFile targetPath; */
NS_IMETHODIMP
sbMediaManagementJobItem::GetTargetPath(nsIFile * *aTargetPath)
{
  NS_ENSURE_ARG_POINTER(aTargetPath);
  NS_IF_ADDREF(*aTargetPath = mTargetPath);
  return NS_OK;
}

/* readonly attribute unsigned short action; */
NS_IMETHODIMP
sbMediaManagementJobItem::GetAction(PRUint16 *aAction)
{
  NS_ENSURE_ARG_POINTER(aAction);
  *aAction = mAction;
  return NS_OK;
}

NS_IMPL_THREADSAFE_ADDREF(sbMediaManagementJob)
NS_IMPL_THREADSAFE_RELEASE(sbMediaManagementJob)
NS_INTERFACE_MAP_BEGIN(sbMediaManagementJob)
  NS_INTERFACE_MAP_ENTRY(sbIMediaManagementJob)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(sbIJobProgress, sbIMediaManagementJob)
  NS_INTERFACE_MAP_ENTRY(sbIJobProgressUI)
  NS_INTERFACE_MAP_ENTRY(sbIJobCancelable)
  NS_INTERFACE_MAP_ENTRY(nsISimpleEnumerator)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, sbIMediaManagementJob)
  NS_IMPL_QUERY_CLASSINFO(sbMediaManagementJob)
NS_INTERFACE_MAP_END

NS_IMPL_CI_INTERFACE_GETTER5(sbMediaManagementJob,
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
#if __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif
#endif /* PR_LOGGING */

//------------------------------------------------------------------------------
//
// sbMediaManagementJob implementation.
//
//------------------------------------------------------------------------------

sbMediaManagementJob::sbMediaManagementJob() :
  mStatus(sbIJobProgress::STATUS_RUNNING),
  mCompletedItemCount(0),
  mTotalItemCount(0)
{
#ifdef PR_LOGGING
  if (!gMediaManagementJobLog) {
    gMediaManagementJobLog = PR_NewLogModule("sbMediaManagementJob");
  }
#endif
  TRACE(("%s[0x%.8x]", __FUNCTION__, this));
}

sbMediaManagementJob::~sbMediaManagementJob()
{
  TRACE(("%s[0x%.8x]", __FUNCTION__, this));
}

/**
 * \brief UpdateProgress - Notify the listeners of the progress information for
 *  this job.
 */
void
sbMediaManagementJob::UpdateProgress()
{
  TRACE(("%s[0x%.8x]", __FUNCTION__, this));
  nsresult rv;
  
  if (!NS_IsMainThread()) {
    // this needs to fire on the main thread, because the job progress listeners
    // may be main thread only
    nsCOMPtr<nsIRunnable> runnable =
      NS_NEW_RUNNABLE_METHOD(sbMediaManagementJob, this, UpdateProgress);
    NS_ENSURE_TRUE(runnable, /* void */);
    rv = NS_DispatchToMainThread(runnable);
    NS_ENSURE_SUCCESS(rv, /* void */);
    return;
  }
  
  if (mStatus != sbIJobProgress::STATUS_RUNNING) {
    TRACE(("%s - Shutting down Job", __FUNCTION__));
    if (mIntervalTimer) {
      mIntervalTimer->Cancel();
      mIntervalTimer = nsnull;
    }
  }

  for (PRInt32 i = mListeners.Count() - 1; i >= 0; --i) {
    mListeners[i]->OnJobProgress(static_cast<sbIJobProgress*>(static_cast<sbIMediaManagementJob*>(this)));
  }
}

void
sbMediaManagementJob::SaveError(nsresult aErrorCode,
                                sbMediaManagementJobItem* aJobItem)
{
  // figure out the related text
  nsString errorText;
  while (aJobItem) {
    /*
     * NOTE: this loop exists only as a disguised gigantic if() statement
     *       so that it is easier to bail out if any condition fails, but
     *       still not be an exposed function.
     *       In a language like JavaScript (or GCC-specific C++), this would
     *       just be a local function instead.
     */
    #define _ENSURE_SUCCESS(res, ret)       \
      if (NS_FAILED(res)) {                 \
        nsresult __rv = res;                \
        NS_ENSURE_SUCCESS_BODY(res, ret);   \
        break;                              \
      }
    nsresult rv;
    nsCOMPtr<sbIMediaItem> item;
    rv = aJobItem->GetItem(getter_AddRefs(item));
    _ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIURI> uri;
    rv = item->GetContentSrc(getter_AddRefs(uri));
    _ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIFileURL> url = do_QueryInterface(uri, &rv);
    _ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIFile> file;
    rv = url->GetFile(getter_AddRefs(file));
    _ENSURE_SUCCESS(rv, rv);
    rv = file->GetPath(errorText);
    _ENSURE_SUCCESS(rv, rv);
    break;
    #undef _ENSURE_SUCCESS
  }
  
  // Save the errors in the error map
  sbErrorMapIter detailIt = mErrorMap.find(aErrorCode);
  if (detailIt != mErrorMap.end()) {
    // result already exists
    detailIt->second.first++;
    if (!errorText.IsEmpty() &&
        detailIt->second.second.size() < ERROR_DETAILS_MAX_COUNT)
    {
      detailIt->second.second.push_back(errorText);
    }
  } else {
    // no existing list
    sbErrorDetail detail;
    detail.first = 1;
    if (!errorText.IsEmpty()) {
      detail.second.push_back(errorText);
    }
    mErrorMap.insert(sbErrorPair(aErrorCode, detail));
  }
}

PRBool
sbMediaManagementJob::AppendErrorToList(PRUint32 aErrorCount,
                                        nsString aErrorKey,
                                        nsTArray<nsString> &aErrorMessages)
{
  nsString errorCount;
  errorCount.AppendInt(aErrorCount);

  nsTArray<nsString> params;
  params.AppendElement(errorCount);

  sbStringBundle bundle;
  nsString errorString = bundle.Format(aErrorKey, params, EmptyString());
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
  TRACE(("%s[0x%.8x]", __FUNCTION__, this));
  nsresult rv;

  if (mNextJobItem) {
    rv = ProcessItem(mNextJobItem);
    if (NS_FAILED(rv)) {
      #if PR_LOGGING
        nsresult __rv = rv;
        NS_ENSURE_SUCCESS_BODY(rv, rv);
      #endif
      SaveError(rv, mNextJobItem);
    }
    

    rv = FindNextItem(getter_AddRefs(mNextJobItem));
    if (NS_FAILED(rv)) {
      #if PR_LOGGING
        nsresult __rv = rv;
        NS_ENSURE_SUCCESS_BODY(rv, rv);
      #endif
      SaveError(rv, nsnull);
    }

    if (mNextJobItem) {
      // Update progress only if we have more to do since
      // we will also call UpdateProgress when we finish the
      // loop.
      UpdateProgress();
    }
  }
  
  // check our status
  if (mCompletedItemCount > mTotalItemCount || !mNextJobItem) {
    if (!mErrorMap.empty()) {
      mStatus = sbIJobProgress::STATUS_FAILED;
    } else {
      mStatus = sbIJobProgress::STATUS_SUCCEEDED;
    }
  } else {

    // Start up the interval timer that will process the next item, if it has
    // not been cancelled
    if (mIntervalTimer) {
      rv = mIntervalTimer->InitWithCallback(this,
                                            mIntervalTimerValue,
                                            nsITimer::TYPE_ONE_SHOT);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  UpdateProgress();
  
  return NS_OK;
}

nsresult
sbMediaManagementJob::ProcessItem(sbMediaManagementJobItem* aJobItem)
{
  TRACE(("%s[0x%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  nsresult rv;
  
  nsCOMPtr<nsIFile> targetFile;
  rv = aJobItem->GetTargetPath(getter_AddRefs(targetFile));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Generate the status text
  nsString targetFilePath;
  rv = targetFile->GetPath(targetFilePath);
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool isInMediaFolder;
  rv = mMediaFolder->Contains(targetFile, PR_TRUE, &isInMediaFolder);
  if (NS_FAILED(rv)) {
    isInMediaFolder = PR_FALSE;
  }
  if (isInMediaFolder) {
    nsString mediaFolderPath;
    rv = mMediaFolder->GetPath(mediaFolderPath);
    NS_ENSURE_SUCCESS(rv, rv);
    mStatusText = Substring(targetFilePath, mediaFolderPath.Length());
    TRACE(("%s[0x%.8x]: setting status text to truncated value %s",
           __FUNCTION__,
           this,
           NS_ConvertUTF16toUTF8(mStatusText).get()));
  } else {
    mStatusText = targetFilePath;
    TRACE(("%s[0x%.8x]: setting status text to full path %s",
           __FUNCTION__,
           this,
           NS_ConvertUTF16toUTF8(mStatusText).get()));
  }

  // use a proxy to the main thread because of bug 16065, see bug 15989 comment 3
  nsCOMPtr<nsIThread> mainThread;
  rv = NS_GetMainThread(getter_AddRefs(mainThread));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbIMediaItem> item;
  rv = aJobItem->GetItem(getter_AddRefs(item));
  NS_ENSURE_SUCCESS(rv, rv);
  
  PRUint16 manageType;
  rv = aJobItem->GetAction(&manageType);
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbIMediaItem> proxiedItem;
  rv = do_GetProxyForObject(mainThread,
                            NS_GET_IID(sbIMediaItem),
                            item,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(proxiedItem));
  NS_ENSURE_SUCCESS(rv, rv);
    
  // Organize the file by calling the sbIMediaFileManager, using the target
  // path we got above to avoid re-computation
  PRBool organizedItem;
  rv = mMediaFileManager->OrganizeItem(proxiedItem,
                                       manageType,
                                       targetFile,
                                       &organizedItem);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!organizedItem)
  {
    #if PR_LOGGING
      LOG(("%s - Gracefully? failed to organize item [%s]",
           __FUNCTION__,
           NS_ConvertUTF16toUTF8(targetFilePath).get()));
    #endif
    SaveError(NS_ERROR_FILE_COPY_OR_MOVE_FAILED, aJobItem);
  }

  return NS_OK;
}

nsresult
sbMediaManagementJob::FindNextItem(sbMediaManagementJobItem** _retval)
{
  NS_ENSURE_TRUE(mMediaList, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(_retval);

  TRACE(("%s[0x%.8x]", __FUNCTION__, this));
  nsresult rv;

  while (PR_TRUE) {
    if (!(mCompletedItemCount < mTotalItemCount)) {
      *_retval = nsnull;
      return NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA;
    }
    // Get the next item
    nsCOMPtr<sbIMediaItem> nextItem;
    rv = mMediaList->GetItemByIndex(mCompletedItemCount,
                                    getter_AddRefs(nextItem));
    NS_ENSURE_SUCCESS(rv, rv);
    
    mCompletedItemCount++;
  
    nsString propValue;
    NS_NAMED_LITERAL_STRING(PROP_HIDDEN, SB_PROPERTY_HIDDEN);
    NS_NAMED_LITERAL_STRING(PROP_TYPE, SB_PROPERTY_CONTENTTYPE);
      
    PRBool isHidden =
      (NS_SUCCEEDED(nextItem->GetProperty(PROP_HIDDEN, propValue)) &&
       propValue.EqualsLiteral("1"));
    nsCOMPtr<sbIMediaList> itemAsList = do_QueryInterface(nextItem);
    PRBool isVideo =
      (NS_SUCCEEDED(nextItem->GetProperty(PROP_TYPE, propValue)) &&
       propValue.EqualsLiteral("video"));
  
    if (isHidden || itemAsList || isVideo) {
      // We don't organize lists, hidden items or videos, try the next one
      continue;
    }

    nsCOMPtr<nsIURI> itemUri;
    rv = nextItem->GetContentSrc(getter_AddRefs(itemUri));
    NS_ENSURE_SUCCESS (rv, rv);
  
    // Get an nsIFileURL object from the nsIURI
    nsCOMPtr<nsIFileURL> fileUrl(do_QueryInterface(itemUri, &rv));
    // If this is not a file then just return to go on to the next item
    if (NS_FAILED(rv) || !fileUrl) {
      // item is not a file, try the next one
      continue;
    }

    // Get the file object
    nsCOMPtr<nsIFile> itemFile;
    rv = fileUrl->GetFile(getter_AddRefs(itemFile));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool isInMediaFolder = PR_FALSE;
    rv = mMediaFolder->Contains(itemFile, PR_TRUE, &isInMediaFolder);
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
    if (mShouldCopyFiles) {
      // Copy files to the media folder if not already there
      manageType = manageType | sbIMediaFileManager::MANAGE_COPY;
      TRACE(("sbMediaManagementJob - Adding MANAGE_COPY"));
    }
  
    // Get the managed path
    nsCOMPtr<nsIFile> targetFile;
    rv = mMediaFileManager->GetManagedPath(nextItem,
                                           manageType,
                                           getter_AddRefs(targetFile));
    NS_ENSURE_SUCCESS(rv, rv);
    if (rv == NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA) {
      // there was an error, try the next item
      continue;
    }
    
    // at this point, we have an item, and a target path
    // figure out the required actions
    nsCOMPtr<nsIFile> oldParent;
    rv = itemFile->GetParent(getter_AddRefs(oldParent));
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIFile> newParent;
    rv = targetFile->GetParent(getter_AddRefs(newParent));
    if (isInMediaFolder) {
      manageType &= ~sbIMediaFileManager::MANAGE_COPY;
    } else {
      manageType &= ~sbIMediaFileManager::MANAGE_MOVE;
    }
    PRBool isSameParent;
    rv = oldParent->Equals(newParent, &isSameParent);
    NS_ENSURE_SUCCESS(rv, rv);
    if (isSameParent) {
      // didn't change the directory the file will live in
      manageType &= ~(sbIMediaFileManager::MANAGE_MOVE |
                      sbIMediaFileManager::MANAGE_COPY);
    }
    nsString oldName, newName;
    rv = itemFile->GetLeafName(oldName);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = targetFile->GetLeafName(newName);
    NS_ENSURE_SUCCESS(rv, rv);
    if (oldName.Equals(newName)) {
      manageType &= ~sbIMediaFileManager::MANAGE_RENAME;
    }
    
    if (!manageType) {
      // we didn't have to do anything to this file after all, try the next one
      continue;
    }
    
    nsRefPtr<sbMediaManagementJobItem> result =
      new sbMediaManagementJobItem(nextItem, targetFile, manageType);
    NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);
    result.forget(_retval);
    return NS_OK;
  } /* end of find item loop */

  NS_NOTREACHED("Shouldn't get here from the loop!");
  return NS_ERROR_UNEXPECTED;
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
  TRACE(("%s[0x%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aTimer);
  nsresult rv;

  if (aTimer == mIntervalTimer) {
    // Process next item
    rv = ProcessNextItem();
    if (NS_FAILED(rv)) {
      mStatus = sbIJobProgress::STATUS_FAILED;
      UpdateProgress();
    }
  }
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// nsISimpleEnumerator Implementation.
//
//------------------------------------------------------------------------------

/* boolean hasMoreElements (); */
NS_IMETHODIMP
sbMediaManagementJob::HasMoreElements(PRBool *_retval)
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  NS_ENSURE_TRUE(mMediaList, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = (mCompletedItemCount < mTotalItemCount) || mNextJobItem;
  return NS_OK;
}

/* nsISupports getNext (); */
NS_IMETHODIMP
sbMediaManagementJob::GetNext(nsISupports **_retval)
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  NS_ENSURE_TRUE(mMediaList, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(_retval);
  
  if (!mNextJobItem) {
    return NS_ERROR_FAILURE;
  }

  nsresult rv = CallQueryInterface(mNextJobItem.get(), _retval);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = FindNextItem(getter_AddRefs(mNextJobItem));
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

//------------------------------------------------------------------------------
//
// sbIMediaManagementJob implementation.
//
//------------------------------------------------------------------------------

/* void init (in sbIMediaList aMediaList,
             [optional] in nsIPropertyBag2 aProperties); */
NS_IMETHODIMP
sbMediaManagementJob::Init(sbIMediaList *aMediaList,
                           nsIPropertyBag2 *aProperties)
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  NS_ENSURE_FALSE(mMediaList, NS_ERROR_ALREADY_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aMediaList);

  NS_NAMED_LITERAL_STRING(KEY_MEDIA_FOLDER, "media-folder");
  NS_NAMED_LITERAL_STRING(KEY_MANAGE_MODE, "manage-mode");

  nsresult rv;
  
  mMediaList = aMediaList;

  /**
   * Preferences - BEGIN
   */
  // If no properties were passed in, make an empty property bag
  PRBool hasKey;
  nsCOMPtr<nsIPropertyBag2> properties(aProperties);
  if (!properties) {
    properties = do_CreateInstance("@mozilla.org/hash-property-bag;1");
    NS_ENSURE_TRUE(properties, NS_ERROR_OUT_OF_MEMORY);
  }

  // Get the preference branch.
  nsCOMPtr<nsIPrefBranch2> prefBranch =
     do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Grab our Media Managed Folder
  mMediaFolder = nsnull;
  rv = properties->HasKey(KEY_MEDIA_FOLDER, &hasKey);
  NS_ENSURE_SUCCESS(rv, rv);
  if (hasKey) {
    nsCOMPtr<nsIFile> mediaFolder;
    rv = properties->GetPropertyAsInterface(KEY_MEDIA_FOLDER,
                                            NS_GET_IID(nsIFile),
                                            getter_AddRefs(mediaFolder));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool success;
    rv = mediaFolder->Exists(&success);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(success, NS_ERROR_INVALID_ARG);
    rv = mediaFolder->IsDirectory(&success);
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(success, NS_ERROR_INVALID_ARG);
    mediaFolder.forget(getter_AddRefs(mMediaFolder));
  } else {
    rv = prefBranch->GetComplexValue(PREF_MMJOB_LOCATION,
                                     NS_GET_IID(nsILocalFile),
                                     getter_AddRefs(mMediaFolder));
    if (NS_FAILED(rv) || mMediaFolder == nsnull) {
      return NS_ERROR_NOT_AVAILABLE;
    }
    nsCOMPtr<nsIWritablePropertyBag2> writableBag =
      do_QueryInterface(properties, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = writableBag->SetPropertyAsInterface(KEY_MEDIA_FOLDER,
                                             mMediaFolder);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (NS_SUCCEEDED(properties->HasKey(KEY_MANAGE_MODE, &hasKey)) && hasKey) {
    PRUint32 mode;
    rv = properties->GetPropertyAsUint32(KEY_MANAGE_MODE, &mode);
    NS_ENSURE_SUCCESS(rv, rv);

    mShouldCopyFiles = !!(mode & sbIMediaFileManager::MANAGE_COPY);
    mShouldMoveFiles = !!(mode & sbIMediaFileManager::MANAGE_MOVE);
    mShouldRenameFiles = !!(mode & sbIMediaFileManager::MANAGE_RENAME);
  } else {
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

  // Grab a Media File Manager component
  mMediaFileManager = do_CreateInstance(SB_MEDIAFILEMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  rv = mMediaFileManager->Init(properties);
  NS_ENSURE_SUCCESS(rv, rv);

  // Reset all the progress information
  rv = mMediaList->GetLength(&mTotalItemCount);
  NS_ENSURE_SUCCESS(rv, rv);
  mCompletedItemCount = 0;
  
  rv = FindNextItem(getter_AddRefs(mNextJobItem));
  NS_ENSURE_SUCCESS(rv, rv);
  
  return rv; /* may be NS_SUCCESS_LOSS_OF_INSIGNIFICANT_DATA if no items */
}

/* void organizeMediaList (); */
NS_IMETHODIMP
sbMediaManagementJob::OrganizeMediaList()
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  NS_ENSURE_TRUE(mMediaList, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;
  
  // Create our timer
  mIntervalTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  ///////////////////////////////////////
  mStatus = sbIJobProgress::STATUS_RUNNING;
  
  // Update the progress
  UpdateProgress();
  
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
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aStatus );
  *aStatus = mStatus;
  return NS_OK;
}

/* readonly attribute unsigned AString statusText; */
NS_IMETHODIMP
sbMediaManagementJob::GetStatusText(nsAString& aText)
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  sbStringBundle bundle;
  nsTArray<nsString> params;

  switch (mStatus) {
    case sbIJobProgress::STATUS_RUNNING:
      params.AppendElement(mStatusText);
      aText = bundle.Format("mediamanager.scanning.item.message", params);
      break;
    case sbIJobProgress::STATUS_SUCCEEDED:
      aText = bundle.Get("mediamanager.scanning.completed");
      break;
    case sbIJobProgress::STATUS_FAILED:
      aText = bundle.Get("mediamanager.scanning.completed_with_errors");
      break;
  }
  return NS_OK;
}

/* readonly attribute AString titleText; */
NS_IMETHODIMP
sbMediaManagementJob::GetTitleText(nsAString& aText)
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
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
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aProgress );

  *aProgress = mCompletedItemCount;
  return NS_OK;
}

/* readonly attribute unsigned long total; */
NS_IMETHODIMP
sbMediaManagementJob::GetTotal(PRUint32* aTotal)
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aTotal );

  // A 0 value makes the progress bar indeterminate
  *aTotal = (mTotalItemCount < 0 ? 0 : mTotalItemCount);
  return NS_OK;
}

/* readonly attribute unsigned long errorCount; */
NS_IMETHODIMP
sbMediaManagementJob::GetErrorCount(PRUint32* aCount)
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER( aCount );

  *aCount = 0;

  sbErrorMapIter it;
  sbErrorMapIter end = mErrorMap.end();
  for (it = mErrorMap.begin(); it != end; ++it) {
    *aCount += it->second.first;
  }
  TRACE(("%s - found %n errors", __FUNCTION__, aCount));
  return NS_OK;
}

/* nsIStringEnumerator getErrorMessages(); */
NS_IMETHODIMP
sbMediaManagementJob::GetErrorMessages(nsIStringEnumerator** aMessages)
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMessages);

  nsTArray<nsString> errorMessages;
  *aMessages = nsnull;

  /**
   * Iterate through the grouped errors we have collected.
   */
  sbErrorMapIter currError;
  sbErrorMapIter end = mErrorMap.end();
  PRUint32 unknownCount = 0;
  sbStringBundle bundle;
  NS_NAMED_LITERAL_STRING(errorKeyBase, "mediamanager.errors.");
  for (currError = mErrorMap.begin(); currError != end; ++currError) {
    nsString errorKey(errorKeyBase);
    AppendInt(errorKey, (*currError).first);
    TRACE(("sbMediaManagementJob: Lookup Error [%s]",
           NS_ConvertUTF16toUTF8(errorKey).get()));
    PRBool foundError = AppendErrorToList((*currError).second.first,
                                          errorKey,
                                          errorMessages);
    if (!foundError) {
      unknownCount += (*currError).second.first;
    }
    std::list<nsString>::const_iterator msgIt = currError->second.second.begin();
    std::list<nsString>::const_iterator msgEnd = currError->second.second.end();
    for (/* msgIt = begin */; msgIt != msgEnd; ++msgIt) {
      TRACE(("%s - adding error %s", __FUNCTION__, NS_ConvertUTF16toUTF8(*msgIt).get()));
      nsString message = bundle.Format(NS_LITERAL_STRING("prefs.media_management.error.details"),
                                       *msgIt,
                                       *msgIt);
      errorMessages.AppendElement(message);
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
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
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
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
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
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
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
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));
  *_retval = PR_TRUE;
  return NS_OK;
}

/* void cancel(); */
NS_IMETHODIMP
sbMediaManagementJob::Cancel()
{
  TRACE(("%s[0x%8.x]", __FUNCTION__, this));

  // Indicate that we have stopped and call UpdateProgress to take care of
  // cleanup.
  mStatus = sbIJobProgress::STATUS_FAILED;
  UpdateProgress();
  return NS_OK;
}
