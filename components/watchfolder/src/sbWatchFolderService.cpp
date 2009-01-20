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

#include "sbWatchFolderService.h"

#include "sbWatchFolderServiceCID.h"
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIObserverService.h>
#include <sbILibraryManager.h>
#include <nsILocalFile.h>
#include <nsIURI.h>
#include <sbIJobProgress.h>
#include <sbIFileMetadataService.h>
#include <sbIPropertyArray.h>
#include <sbPropertiesCID.h>
#include <nsIPrefBranch2.h>
#include <sbStandardProperties.h>
#include <sbIURIImportService.h>
#include <sbIApplicationController.h>

#define PREF_WATCHFOLDER_ENABLE "songbird.watch_folder.enable"
#define PREF_WATCHFOLDER_PATH   "songbird.watch_folder.path"

#define ADD_DELAY_TIMER_DELAY    1500
#define CHANGE_DELAY_TIMER_DELAY 30000
#define EVENT_PUMP_TIMER_DELAY   1000 

typedef sbStringVector::const_iterator sbStringVectorIter;


NS_IMPL_ISUPPORTS5(sbWatchFolderService, 
                   sbIWatchFolderService, 
                   sbIFileSystemListener,
                   sbIMediaListEnumerationListener,
                   nsITimerCallback,
                   nsIObserver)

sbWatchFolderService::sbWatchFolderService()
{
  mIsSupported = PR_FALSE;
  mIsEnabled = PR_FALSE;
  mIsWatching = PR_FALSE;
  mEventPumpTimerIsSet = PR_FALSE;
  mAddDelayTimerIsSet = PR_FALSE;
  mChangeDelayTimerIsSet = PR_FALSE;
  mCurrentProcessType = eNone;
}

sbWatchFolderService::~sbWatchFolderService()
{
}

nsresult
sbWatchFolderService::Init()
{
  nsresult rv;

  // The watch folder services are not supported if the file system watcher is
  // not.  The file system watcher component may not be available at all on this
  // OS or may not be supported on this OS version.  In either case, just return
  // without logging any errors.
  nsCOMPtr<sbIFileSystemWatcher> fileSystemWatcher =
    do_CreateInstance("@songbirdnest.com/filesystem/watcher;1", &rv);
  if (NS_FAILED(rv))
    return NS_OK;
  PRBool isSupported = PR_FALSE;
  rv = fileSystemWatcher->GetIsSupported(&isSupported);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isSupported)
    return NS_OK;

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "final-ui-startup", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "quit-application", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch2> prefBranch = 
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // This will fail if the pref doesn't exist yet (i.e. first run)
  rv = prefBranch->GetBoolPref(PREF_WATCHFOLDER_ENABLE, &mIsEnabled);
  if (NS_FAILED(rv)) {
    mIsEnabled = PR_FALSE; 
  }

  // Same as above.
  nsCOMPtr<nsISupportsString> supportsString;
  rv = prefBranch->GetComplexValue(PREF_WATCHFOLDER_PATH,
                                   NS_GET_IID(nsISupportsString),
                                   getter_AddRefs(supportsString));
  if (NS_SUCCEEDED(rv)) {
    rv = supportsString->GetData(mWatchPath);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Listen to changes in preferences.
  rv = prefBranch->AddObserver(PREF_WATCHFOLDER_ENABLE, this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prefBranch->AddObserver(PREF_WATCHFOLDER_PATH, this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  mIOService = do_GetService("@mozilla.org/network/io-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Indicate that the watch folder services are supported.
  mIsSupported = PR_TRUE;

  return NS_OK;
}

nsresult
sbWatchFolderService::StartWatching()
{
  // Don't start if the service isn't supposed to enabled, the watch path
  // is not defined, or the service is already watching.
  if (!mIsEnabled || mWatchPath.IsEmpty() || mIsWatching) {
    return NS_OK;
  }

  nsresult rv;
  if (!mMainLibrary) {
    nsCOMPtr<sbILibraryManager> libraryMgr =
      do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = libraryMgr->GetMainLibrary(getter_AddRefs(mMainLibrary));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mFileSystemWatcher = 
    do_CreateInstance("@songbirdnest.com/filesystem/watcher;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFileSystemWatcher->Init(this, mWatchPath, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFileSystemWatcher->StartWatching();
  NS_ENSURE_SUCCESS(rv, rv);

  mIsWatching = PR_TRUE;
  return NS_OK;
}

nsresult
sbWatchFolderService::StopWatching()
{
  if (!mIsWatching) {
    return NS_OK;
  }

  // Clear all event paths.
  mAddedPaths.clear();
  mRemovedPaths.clear();
  mChangedPaths.clear();
  mDelayedChangedPaths.clear();

  // Stop and kill the file-system watcher.
  mFileSystemWatcher->StopWatching();
  mFileSystemWatcher = nsnull;
  
  mIsWatching = PR_FALSE;
  return NS_OK;
}

nsresult
sbWatchFolderService::SetEventPumpTimer()
{
  if (mEventPumpTimerIsSet) {
    return NS_OK;
  }

  nsresult rv;
  rv = mEventPumpTimer->InitWithCallback(this, 
                                         EVENT_PUMP_TIMER_DELAY, 
                                         nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  mEventPumpTimerIsSet = PR_TRUE;
  return NS_OK;
}

nsresult
sbWatchFolderService::ProcessEventPaths(sbStringVector & aEventPathVector,
                                        EProcessType aProcessType)
{
  if (aEventPathVector.empty()) {
    return NS_OK;
  }

  mCurrentProcessType = aProcessType;

  nsresult rv = EnumerateItemsByPaths(aEventPathVector);
  NS_ENSURE_SUCCESS(rv, rv);

  aEventPathVector.clear();
  return NS_OK;
}

nsresult
sbWatchFolderService::ProcessAddedPaths()
{
  if (mAddedPaths.empty()) { 
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIMutableArray> uriArray = 
    do_CreateInstance("@mozilla.org/array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sbStringVectorIter begin = mAddedPaths.begin();
  sbStringVectorIter end = mAddedPaths.end();
  sbStringVectorIter next;
  for (next = begin; next != end; ++next) {
    nsCOMPtr<nsIURI> fileURI;
    rv = GetFilePathURI(*next, getter_AddRefs(fileURI));
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not get a URI for a file!");
      continue;
    }

    rv = uriArray->AppendElement(fileURI, PR_FALSE);
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not append the URI to the mutable array!");
    }
  }

  mAddedPaths.clear();

  nsCOMPtr<sbIURIImportService> uriImportService =
    do_GetService("@songbirdnest.com/uri-import-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the current songbird window
  nsCOMPtr<nsIDOMWindow> songbirdWindow;
  rv = GetSongbirdWindow(getter_AddRefs(songbirdWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // XXX todo This can cause problems if this fires when the user is dragging
  //          and dropping into a playlist. This will need to be fixed.
  //
  rv = uriImportService->ImportURIArray(uriArray,
                                        songbirdWindow,
                                        mMainLibrary,
                                        -1,
                                        nsnull);  // listener
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}

nsresult
sbWatchFolderService::GetFilePathURI(const nsAString & aFilePath, 
                                     nsIURI **aURIRetVal)
{
  NS_ENSURE_ARG_POINTER(aURIRetVal);

  nsresult rv;
  nsCOMPtr<nsILocalFile> pathFile =
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pathFile->InitWithPath(aFilePath);
  NS_ENSURE_SUCCESS(rv, rv);

  return mIOService->NewFileURI(pathFile, aURIRetVal);
}

nsresult
sbWatchFolderService::EnumerateItemsByPaths(sbStringVector & aPathVector)
{
  nsresult rv;
  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString propName(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL));

  sbStringVectorIter begin = aPathVector.begin();
  sbStringVectorIter end = aPathVector.end();
  sbStringVectorIter next;
  for (next = begin; next != end; ++next) {
    // Convert the current path to a URI, then get the spec.
    nsCOMPtr<nsIURI> fileURI;
    rv = GetFilePathURI(*next, getter_AddRefs(fileURI));
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not get the file path URI!");
      continue;
    }

    nsCString pathSpec;
    rv = fileURI->GetSpec(pathSpec);
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not get the URI spec!");
      continue;
    }

    rv = properties->AppendProperty(propName, NS_ConvertUTF8toUTF16(pathSpec));
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not append a property!");
    }
  }

  PRUint16 enumType = sbIMediaList::ENUMERATIONTYPE_SNAPSHOT;
  rv = mMainLibrary->EnumerateItemsByProperties(properties, this, enumType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbWatchFolderService::GetSongbirdWindow(nsIDOMWindow **aSongbirdWindow)
{
  NS_ENSURE_ARG_POINTER(aSongbirdWindow);

  nsresult rv;
  nsCOMPtr<sbIApplicationController> appController =
    do_GetService("@songbirdnest.com/Songbird/ApplicationController;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return appController->GetActiveMainWindow(aSongbirdWindow);
}

//------------------------------------------------------------------------------
// sbWatchFolderService

NS_IMETHODIMP
sbWatchFolderService::GetIsSupported(PRBool *aIsSupported)
{
  NS_ENSURE_ARG_POINTER(aIsSupported);
  *aIsSupported = mIsSupported;
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIFileSystemListener

NS_IMETHODIMP
sbWatchFolderService::OnWatcherStarted()
{
  // Now start up the timer
  nsresult rv;
  if (!mEventPumpTimer) {
    mEventPumpTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!mAddDelayTimer) {
    mAddDelayTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!mChangeDelayTimer) {
    mChangeDelayTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnWatcherStopped()
{
  if (mEventPumpTimer) {
    mEventPumpTimer->Cancel();
  }
  if (mAddDelayTimer) {
    mAddDelayTimer->Cancel();
  }
  if (mChangeDelayTimer) {
    mChangeDelayTimer->Cancel();
  }
  
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemChanged(const nsAString & aFilePath)
{
  // See if the changed path queue already has this path inside of it.
  PRBool foundMatchingPath = PR_FALSE;
  sbStringVectorIter begin = mChangedPaths.begin();
  sbStringVectorIter end = mChangedPaths.end();
  sbStringVectorIter next;
  for (next = begin; next != end && !foundMatchingPath; ++next) {
    if (aFilePath.Equals(*next)) {
      foundMatchingPath = PR_TRUE;
    }
  }

  nsresult rv;
  if (foundMatchingPath) {
    // If this path is currently in the changed path vector already, 
    // delay processing this path until a later time.
    mDelayedChangedPaths.push_back(nsString(aFilePath));

    // Start the delayed timer if it isn't running already.
    if (!mChangeDelayTimerIsSet) {
      rv = mChangeDelayTimer->InitWithCallback(this,
                                               CHANGE_DELAY_TIMER_DELAY,
                                               nsITimer::TYPE_ONE_SHOT);
      NS_ENSURE_SUCCESS(rv, rv);

      mChangeDelayTimerIsSet = PR_TRUE;
    }
  }
  else {
    mChangedPaths.push_back(nsString(aFilePath));
    rv = SetEventPumpTimer();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemRemoved(const nsAString & aFilePath)
{
  mRemovedPaths.push_back(nsString(aFilePath));
  nsresult rv = SetEventPumpTimer();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemAdded(const nsAString & aFilePath)
{
  mAddedPaths.push_back(nsString(aFilePath));

  if (mAddDelayTimerIsSet) {
    // The timer is currently set, and more added events have been received.
    // Toggle || so that the timer will reset itself instead of processing 
    // the added event paths when it fires.
    mShouldProcessAddedPaths = PR_FALSE;
  }
  else {
    nsresult rv;
    rv = mAddDelayTimer->InitWithCallback(this, 
                                          ADD_DELAY_TIMER_DELAY,
                                          nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mAddDelayTimerIsSet = PR_TRUE;
    mShouldProcessAddedPaths = PR_TRUE;
  }
  
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIMediaListEnumerationListener

NS_IMETHODIMP 
sbWatchFolderService::OnEnumerationBegin(sbIMediaList *aMediaList, 
                                         PRUint16 *aRetVal)
{
  if (!mEnumeratedMediaItems) {
    nsresult rv;
    mEnumeratedMediaItems = do_CreateInstance("@mozilla.org/array;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aRetVal = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnEnumeratedItem(sbIMediaList *aMediaList, 
                                       sbIMediaItem *aMediaItem, 
                                       PRUint16 *aRetVal)
{
  mEnumeratedMediaItems->AppendElement(aMediaItem, PR_FALSE);
  *aRetVal = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnEnumerationEnd(sbIMediaList *aMediaList, 
                                       nsresult aStatusCode)
{
  nsresult rv;

  if (mCurrentProcessType == eRemoval) {
    // Remove the found items from the library.
    nsCOMPtr<nsISimpleEnumerator> mediaItemEnum;
    rv = mEnumeratedMediaItems->Enumerate(getter_AddRefs(mediaItemEnum));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMainLibrary->RemoveSome(mediaItemEnum);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (mCurrentProcessType == eChanged) {
    PRUint32 length;
    rv = mEnumeratedMediaItems->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    if (length > 0) {
      // Rescan the changed items.
      nsCOMPtr<sbIFileMetadataService> metadataService =
        do_GetService("@songbirdnest.com/Songbird/FileMetadataService;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIJobProgress> jobProgress;
      rv = metadataService->Read(mEnumeratedMediaItems, 
                                 getter_AddRefs(jobProgress));
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  rv = mEnumeratedMediaItems->Clear();
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentProcessType = eNone;
  return NS_OK;
}

//------------------------------------------------------------------------------
// nsITimerCallback

NS_IMETHODIMP
sbWatchFolderService::Notify(nsITimer *aTimer)
{
  nsresult rv;
  if (aTimer == mEventPumpTimer) {
    rv = ProcessEventPaths(mChangedPaths, eChanged);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ProcessEventPaths(mRemovedPaths, eRemoval);
    NS_ENSURE_SUCCESS(rv, rv);

    mEventPumpTimerIsSet = PR_FALSE;
  }
  else if (aTimer == mAddDelayTimer) {
    // If there has been no further add events for a second, begin to process
    // the added path queue.
    if (mShouldProcessAddedPaths) {
      rv = ProcessAddedPaths();
      NS_ENSURE_SUCCESS(rv, rv);

      mAddDelayTimerIsSet = PR_FALSE;
    }
    else {
      // More added events were received while the timer was set. Try and
      // wait another timer session before processing events.
      rv = mAddDelayTimer->InitWithCallback(this,
                                            ADD_DELAY_TIMER_DELAY,
                                            nsITimer::TYPE_ONE_SHOT);
      NS_ENSURE_SUCCESS(rv, rv);

      mShouldProcessAddedPaths = PR_TRUE;
    }
  }
  else if (aTimer == mChangeDelayTimer) {
    rv = ProcessEventPaths(mDelayedChangedPaths, eChanged);
    NS_ENSURE_SUCCESS(rv, rv);

    mChangeDelayTimerIsSet = PR_FALSE;
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbWatchFolderService::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);

  nsresult rv;
  if (strcmp("final-ui-startup", aTopic) == 0) {
    if (mIsEnabled) {
      rv = StartWatching();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (strcmp("quit-application", aTopic) == 0) {
    if (mIsWatching) {
      rv = StopWatching();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, "profile-after-change");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, "quit-application");
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrefBranch2> prefBranch = 
      do_GetService("@mozilla.org/preferences-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = prefBranch->RemoveObserver(PREF_WATCHFOLDER_ENABLE, this);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = prefBranch->RemoveObserver(PREF_WATCHFOLDER_PATH, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic) == 0) {
    nsCOMPtr<nsIPrefBranch2> prefBranch = do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsString changedPrefName(aData);
    if (changedPrefName.Equals(NS_LITERAL_STRING(PREF_WATCHFOLDER_ENABLE))) {
      rv = prefBranch->GetBoolPref(PREF_WATCHFOLDER_ENABLE, &mIsEnabled);

      if (NS_SUCCEEDED(rv)) {
        if (mIsEnabled && !mIsWatching) {
          // The service has been previously disabled, and is not watching. 
          // It is now time to start watching.
          rv = StartWatching();
          NS_ENSURE_SUCCESS(rv, rv);
        }
        else if (!mIsEnabled && mIsWatching) {
          // The service is currently running, and the user has opted to stop
          // watching the folder. It is now time to stop the service.
          rv = StopWatching();
          NS_ENSURE_SUCCESS(rv, rv);
        }
      }
    }
    else if (changedPrefName.Equals(NS_LITERAL_STRING(PREF_WATCHFOLDER_PATH))) {
      nsCOMPtr<nsISupportsString> supportsString;
      rv = prefBranch->GetComplexValue(PREF_WATCHFOLDER_PATH,
                                       NS_GET_IID(nsISupportsString),
                                       getter_AddRefs(supportsString));
      if (NS_SUCCEEDED(rv)) {
        nsString newWatchPath;
        rv = supportsString->GetData(newWatchPath);
        NS_ENSURE_SUCCESS(rv, rv);

        // The watch path has changed! Need to update... 
        if (!newWatchPath.Equals(mWatchPath)) {
          mWatchPath = newWatchPath;
          if (mIsEnabled) {
            if (mIsWatching) {
              // Service is already running, need to stop watching the old
              // path and start watching the new path.
              // TODO: Write Me!
            }
            else {
              // Service is not running, it is now time to start it.
              rv = StartWatching();
              NS_ENSURE_SUCCESS(rv, rv);
            }
          }
        }
      }
    }
  }
 
  return NS_OK;
}

//------------------------------------------------------------------------------
// XPCOM Startup Registration

/* static */ NS_METHOD
sbWatchFolderService::RegisterSelf(nsIComponentManager *aCompMgr,
                                   nsIFile *aPath,
                                   const char *aLoaderStr,
                                   const char *aType,
                                   const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> catMgr = 
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catMgr->AddCategoryEntry("app-startup",
                                SONGBIRD_WATCHFOLDERSERVICE_CLASSNAME,
                                "service,"
                                SONGBIRD_WATCHFOLDERSERVICE_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

