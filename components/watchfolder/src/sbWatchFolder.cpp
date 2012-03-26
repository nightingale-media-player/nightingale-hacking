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

#include "sbWatchFolder.h"
#include "sbWatchFolderDefines.h"
#include <sbIWFMoveRenameHelper9000.h>
#include <sbIWFRemoveHelper9001.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <sbIApplicationController.h>
#include <sbIDirectoryImportService.h>
#include <sbIFileMetadataService.h>
#include <sbIJobProgress.h>
#include <sbIJobProgressService.h>
#include <sbIPropertyArray.h>
#include <sbStringBundle.h>
#include <sbIPrompter.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsILocalFile.h>
#include <nsIObserverService.h>
#include <nsIURI.h>
#include <nsTArray.h>
#include <sbIMediacoreTypeSniffer.h>
#include <nsThreadUtils.h>
#include <nsXPCOMCIDInternal.h>
#include <nsIXULRuntime.h>
#include <prlog.h>


#ifdef PR_LOGGING
static PRLogModuleInfo* gLog = nsnull;
#endif

NS_IMPL_ISUPPORTS5(sbWatchFolder,
                   sbIWatchFolder,
                   sbIFileSystemListener,
                   sbIMediaListEnumerationListener,
                   nsITimerCallback,
                   sbIJobProgressListener)

sbWatchFolder::sbWatchFolder()
{
  mCanInteract = PR_TRUE;
  mHasWatcherStarted = PR_FALSE;
  mShouldReinitWatcher = PR_FALSE;
  mShouldSynchronize = PR_FALSE;
  mEventPumpTimerIsSet = PR_FALSE;
  mChangeDelayTimerIsSet = PR_FALSE;
  mShouldProcessEvents = PR_FALSE;
  mCurrentProcessType = eNone;

#ifdef PR_LOGGING
   if (!gLog) {
     gLog = PR_NewLogModule("sbWatchFolder");
   }
#endif
}

sbWatchFolder::~sbWatchFolder()
{
}

nsresult
sbWatchFolder::Init()
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

  PRBool isWatcherSupported = PR_FALSE;
  rv = fileSystemWatcher->GetIsSupported(&isWatcherSupported);
  NS_ENSURE_SUCCESS(rv, rv);

  // if watching is supported check for safe-mode
  if (isWatcherSupported) {
    nsCOMPtr<nsIXULRuntime> appInfo =
      do_GetService(XULRUNTIME_SERVICE_CONTRACTID, &rv);
    // If we can't get or QI the runtime assume we're not in safe-mode
    if (NS_SUCCEEDED(rv)) {
      PRBool isInSafeMode = PR_FALSE;
      rv = appInfo->GetInSafeMode(&isInSafeMode);
      // If we're not in safe mode or we can't determine if we are, enable it
      isWatcherSupported = NS_FAILED(rv) || !isInSafeMode;
    }
  }

  if (!isWatcherSupported) {
    mServiceState = eNotSupported;
    return NS_OK;
  }

  mLibraryUtils =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Assume the service is disabled. This will be verified on the delayed
  // initialization in |InitInternal()|.
  mServiceState = eDisabled;

  return NS_OK;
}

nsresult
sbWatchFolder::InitInternal()
{
  nsresult rv;

  // Set the service as disabled, if this method exits cleanly, the service
  // will be considered 'started'.
  mServiceState = eDisabled;

  // If the service is set to be turned off, do not continue.
  if (!mMediaList) {
    return NS_OK;
  }

  // Don't start the service if the watch path isn't defined.
  if (mWatchPath.Equals(EmptyString())) {
    return NS_ERROR_UNEXPECTED;
  }

  // The service is now considered started.
  mServiceState = eStarted;

  // Now start watching the folder.
  rv = StartWatchingFolder();
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("%s: started watching [%s]",
         __FUNCTION__,
         NS_ConvertUTF16toUTF8(mWatchPath).get()));

  return NS_OK;
}

nsresult
sbWatchFolder::StartWatchingFolder()
{
  // Don't start if the service is not in the |eStarted| state or if the
  // watch path is empty.
  if (mWatchPath.IsEmpty() || mServiceState != eStarted) {
    return NS_OK;
  }

  nsresult rv;
  mFileSystemWatcher =
    do_CreateInstance("@songbirdnest.com/filesystem/watcher;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mFileSystemWatcherGUID.Equals(EmptyCString())) {
    // Init a new file-system watcher. The session GUID for the new watcher
    // will be saved in StopWatching().
    TRACE(("%s: initiating new FS watcher for [%s]",
           __FUNCTION__,
           NS_ConvertUTF16toUTF8(mWatchPath).get()));
    rv = mFileSystemWatcher->Init(this, mWatchPath, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    TRACE(("%s: initiating saved session %s",
           __FUNCTION__, mFileSystemWatcherGUID.get()));
    rv = mFileSystemWatcher->InitWithSession(mFileSystemWatcherGUID, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mShouldSynchronize) {
    NS_ENSURE_STATE(mMediaList);
    mMediaList->Clear();
  }

  rv = mFileSystemWatcher->StartWatching();
  NS_ENSURE_SUCCESS(rv, rv);

  // The service is now watching
  mServiceState = eWatching;

  if (mShouldSynchronize) {
    mShouldSynchronize = PR_FALSE;
    Rescan();
  }
  return NS_OK;
}

nsresult
sbWatchFolder::StopWatchingFolder()
{
  if (mServiceState != eWatching) {
    return NS_OK;
  }

  NS_ENSURE_STATE(mFileSystemWatcher);

  // Clear all event paths.
  mAddedPaths.clear();
  mRemovedPaths.clear();
  mChangedPaths.clear();
  mDelayedChangedPaths.clear();

  nsresult rv;
  if (mFileSystemWatcherGUID.Equals(EmptyCString())) {
    // This is the first time the file system watcher has run. Save the session
    // guid so changes can be determined when the watcher starts next.
    rv = mFileSystemWatcher->GetSessionGuid(mFileSystemWatcherGUID);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Stop and kill the file-system watcher.
  rv = mFileSystemWatcher->StopWatching(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  // The service is no longer watching - mark as |eStarted|.
  mServiceState = eStarted;
  return NS_OK;
}

nsresult
sbWatchFolder::SetStartupDelayTimer()
{
  nsresult rv;
  if (!mStartupDelayTimer) {
    LOG(("%s: creating new startup delay timer", __FUNCTION__));
    mStartupDelayTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  LOG(("%s: arming up startup delay timer [%08x]",
       __FUNCTION__, mStartupDelayTimer.get()));
  return mStartupDelayTimer->InitWithCallback(this,
                                              STARTUP_TIMER_DELAY,
                                              nsITimer::TYPE_ONE_SHOT);
}

nsresult
sbWatchFolder::SetEventPumpTimer()
{
  if (mHasWatcherStarted) {
    if (mEventPumpTimerIsSet) {
      // The event pump timer is already set, but more events have been
      // received. Set this flags so that the timer will re-arm itself
      // when it is fired.
      mShouldProcessEvents = PR_FALSE;
    }
    else {
      LOG(("%s: arming event pump timer [%08x]",
            __FUNCTION__, mEventPumpTimer.get()));

      nsresult rv =
        mEventPumpTimer->InitWithCallback(this,
                                          EVENT_PUMP_TIMER_DELAY,
                                          nsITimer::TYPE_ONE_SHOT);
      NS_ENSURE_SUCCESS(rv, rv);

      mEventPumpTimerIsSet = PR_TRUE;
      mShouldProcessEvents = PR_TRUE;
    }
  }

  return NS_OK;
}

nsresult
sbWatchFolder::ProcessEventPaths()
{
  TRACE(("%s: Processing the observed event paths", __FUNCTION__));
  TRACE(("%s: mRemovedPaths.size() == %i", __FUNCTION__, mRemovedPaths.size()));
  TRACE(("%s: mAddedPaths.size() == %i", __FUNCTION__, mAddedPaths.size()));
  TRACE(("%s: mChangedPaths.size() == %i", __FUNCTION__, mChangedPaths.size()));

  // For now, just process remove, added, and changed paths.
  nsresult rv;

  // If possible, try to guess moves and renames and avoid
  // just removing and re-adding
  if (mRemovedPaths.size() > 0 && mAddedPaths.size() > 0) {
    LOG(("sbWatchFolder: possible move/rename detected"));
    rv = HandleEventPathList(mRemovedPaths, eMoveOrRename);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = HandleEventPathList(mRemovedPaths, eRemoval);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ProcessAddedPaths();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = HandleEventPathList(mChangedPaths, eChanged);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbWatchFolder::HandleEventPathList(sbStringSet & aEventPathSet,
                                          EProcessType aProcessType)
{
  LOG(("%s: Processing event path list [aEventPathSet.size == %i]",
    __FUNCTION__, aEventPathSet.size()));

  if (aEventPathSet.empty()) {
    return NS_OK;
  }

  mCurrentProcessType = aProcessType;

  nsresult rv = EnumerateItemsByPaths(aEventPathSet);
  NS_ENSURE_SUCCESS(rv, rv);

  aEventPathSet.clear();
  return NS_OK;
}

nsresult
sbWatchFolder::ProcessAddedPaths()
{
  LOG(("%s: Processing the added file paths... [mAddedPaths.size == %i]",
    __FUNCTION__, mAddedPaths.size()));

  if (mAddedPaths.empty()) {
    return NS_OK;
  }

  nsresult rv;
  nsCOMPtr<nsIArray> uriArray;
  rv = GetURIArrayForStringPaths(mAddedPaths, getter_AddRefs(uriArray));
  NS_ENSURE_SUCCESS(rv, rv);

  mAddedPaths.clear();

  PRUint32 uriArrayLength = 0;
  rv = uriArray->GetLength(&uriArrayLength);
  NS_ENSURE_SUCCESS(rv, rv);

  if (uriArrayLength > 0) {
    nsCOMPtr<sbIDirectoryImportService> importService;
    rv = GetImporter(getter_AddRefs(importService));
    NS_ENSURE_SUCCESS(rv, rv);

    //
    // XXX todo This can cause problems if this fires when the user is dragging
    //          and dropping into a playlist. This will need to be fixed.
    //
    nsCOMPtr<sbIDirectoryImportJob> job;
    rv = importService->ImportWithCustomSnifferAndMetadataScanner(
      uriArray,
      mTypeSniffer,
      mMetadataScanner,
      mMediaList,
      -1,
      getter_AddRefs(job));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIJobProgressService> progressService =
      do_GetService("@songbirdnest.com/Songbird/JobProgressService;1", &rv);
    if (NS_SUCCEEDED(rv) && progressService) {
      nsCOMPtr<sbIJobProgress> jobProgress = do_QueryInterface(job, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = progressService->ShowProgressDialog(jobProgress, nsnull, 1);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbWatchFolder::GetURIArrayForStringPaths(sbStringSet & aPathsSet,
                                                nsIArray **aURIs)
{
  NS_ENSURE_ARG_POINTER(aURIs);
  nsresult rv;

  nsCOMPtr<nsIMutableArray> uriArray =
    do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediacoreTypeSniffer> typeSniffer =
    do_CreateInstance("@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  sbStringSetIter begin = aPathsSet.begin();
  sbStringSetIter end = aPathsSet.end();
  sbStringSetIter next;
  for (next = begin; next != end; ++next) {
    nsCOMPtr<nsIURI> fileURI;
    rv = GetFilePathURI(*next, getter_AddRefs(fileURI));
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not get a URI for a file!");
      continue;
    }

    // Don't add every type of file, have the mediacore sniffer validate this
    // is a URI that we can handle.
    PRBool isValid = PR_FALSE;
    rv = typeSniffer->IsValidMediaURL(fileURI, &isValid);
    if (NS_SUCCEEDED(rv) && isValid) {
      rv = uriArray->AppendElement(fileURI, PR_FALSE);
      if (NS_FAILED(rv)) {
        NS_WARNING("Could not append the URI to the mutable array!");
      }
    }
  }

  nsCOMPtr<nsIArray> array = do_QueryInterface(uriArray, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  array.forget(aURIs);
  return rv;
}

nsresult
sbWatchFolder::GetFilePathURI(const nsAString & aFilePath,
                                     nsIURI **aURIRetVal)
{
  NS_ENSURE_ARG_POINTER(aURIRetVal);

  nsresult rv;

  nsCOMPtr<nsILocalFile> pathFile =
    do_CreateInstance("@mozilla.org/file/local;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = pathFile->InitWithPath(aFilePath);
  NS_ENSURE_SUCCESS(rv, rv);

  return mLibraryUtils->GetFileContentURI(pathFile, aURIRetVal);
}

nsresult
sbWatchFolder::EnumerateItemsByPaths(sbStringSet & aPathSet)
{
  nsresult rv;
  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsString propName(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL));

  sbStringSetIter begin = aPathSet.begin();
  sbStringSetIter end = aPathSet.end();
  sbStringSetIter next;
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
  rv = mMediaList->EnumerateItemsByProperties(properties, this, enumType);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbWatchFolder::GetSongbirdWindow(nsIDOMWindow **aSongbirdWindow)
{
  NS_ENSURE_ARG_POINTER(aSongbirdWindow);

  nsresult rv;
  nsCOMPtr<sbIApplicationController> appController =
    do_GetService("@songbirdnest.com/Songbird/ApplicationController;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return appController->GetActiveMainWindow(aSongbirdWindow);
}

nsresult
sbWatchFolder::HandleSessionLoadError()
{
  NS_ASSERTION(NS_IsMainThread(),
      "HandleSessionLoadError() not called on main thread!");
  NS_ENSURE_STATE(mFileSystemWatcher);

  // If the unit tests are running, don't show the dialog (Don't bother
  // checking result of call too).
  nsresult rv;
  if (!mCanInteract) {
    return NS_OK;
  }

  // If this method gets called, than the watcher could not load the stored
  // session. The tree will need to be re-initialized, this time without
  // loading a session.
  if (!mFileSystemWatcherGUID.IsEmpty()) {
    // Attempt the remove the session data. Don't bother returning the result
    // if it fails, just warn about it.
    rv = mFileSystemWatcher->DeleteSession(mFileSystemWatcherGUID);
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not delete the bad session data file!");
    }

    mFileSystemWatcherGUID.Truncate();
  }

  rv = mFileSystemWatcher->Init(this, mWatchPath, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mFileSystemWatcher->StartWatching();
  NS_ENSURE_SUCCESS(rv, rv);

  sbStringBundle bundle;
  nsString dialogTitle =
    bundle.Get("watch_folder.session_load_error.rescan_title");

  nsTArray<nsString> params;
  params.AppendElement(mWatchPath);
  nsString dialogText =
    bundle.Format("watch_folder.session_load_error.rescan_text", params);

  nsCOMPtr<nsIDOMWindow> songbirdWindow;
  rv = GetSongbirdWindow(getter_AddRefs(songbirdWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPrompter> prompter =
    do_CreateInstance("@songbirdnest.com/Songbird/Prompter;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prompter->SetWaitForWindow(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool shouldRescan = PR_FALSE;
  prompter->Confirm(songbirdWindow,
                    dialogTitle.BeginReading(),
                    dialogText.BeginReading(),
                    &shouldRescan);

  // Only start the scan if the user picked the YES button (option 0).
  if (shouldRescan) {
      // The user elected to rescan their watched directory.
      rv = Rescan();
      NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbWatchFolder::Rescan()
{
    nsresult rv;

    // Setup the directory scan service.
    nsCOMPtr<sbIDirectoryImportService> dirImportService;
    rv = GetImporter(getter_AddRefs(dirImportService));
    NS_ENSURE_SUCCESS(rv, rv);

    // The directory import service wants the paths as an array.
    nsCOMPtr<nsILocalFile> watchPathFile =
      do_CreateInstance("@mozilla.org/file/local;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = watchPathFile->InitWithPath(mWatchPath);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIMutableArray> dirArray =
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);

    rv = dirArray->AppendElement(watchPathFile, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIDirectoryImportJob> importJob;
    rv = dirImportService->ImportWithCustomSnifferAndMetadataScanner(
                                  dirArray,
                                  mTypeSniffer,
                                  mMetadataScanner,
                                  mMediaList,
                                  -1,
                                  getter_AddRefs(importJob));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!importJob) {
      return NS_OK;
    }

    if (!mCanInteract) {
      return NS_OK;
    }

    nsCOMPtr<sbIJobProgressService> progressService =
      do_GetService("@songbirdnest.com/Songbird/JobProgressService;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIJobProgress> job = do_QueryInterface(importJob, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIApplicationController> appController =
      do_GetService("@songbirdnest.com/Songbird/ApplicationController;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMWindow> activeWindow;
    rv = appController->GetActiveWindow(getter_AddRefs(activeWindow));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = progressService->ShowProgressDialog(job, activeWindow, 1);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
}

nsresult
sbWatchFolder::HandleRootPathMissing()
{
  // If the unit tests are running, don't show the dialog (Don't bother
  // checking result of call too).
  nsresult rv;
  if (!mCanInteract) {
    return NS_OK;
  }

  sbStringBundle bundle;
  nsString dialogTitle = bundle.Get("watch_folder.root_path_missing.title");

  nsTArray<nsString> params;
  params.AppendElement(mWatchPath);
  nsString dialogText =
    bundle.Format("watch_folder.root_path_missing.text", params);

  nsCOMPtr<nsIDOMWindow> songbirdWindow;
  rv = GetSongbirdWindow(getter_AddRefs(songbirdWindow));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIPrompter> prompter =
    do_CreateInstance("@songbirdnest.com/Songbird/Prompter;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prompter->SetWaitForWindow(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prompter->Alert(songbirdWindow,
                       dialogTitle.BeginReading(),
                       dialogText.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbWatchFolder::DecrementIgnoredPathCount(const nsAString & aFilePath,
                                                PRBool *aIsIgnoredPath)
{
  NS_ENSURE_ARG_POINTER(aIsIgnoredPath);

  sbStringMap::iterator foundIter = mIgnorePaths.find(nsString(aFilePath));
  if (foundIter == mIgnorePaths.end()) {
    *aIsIgnoredPath = PR_FALSE;
  } else {
    *aIsIgnoredPath = PR_TRUE;
    if (foundIter->second.count > 0) {
      --foundIter->second.count;
      if ((foundIter->second.count < 1) && (foundIter->second.depth < 1)) {
        // the count has reached zero, stop ignoring
        mIgnorePaths.erase(foundIter);
      }
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::Start(const nsACString & aSessionGuid)
{
  mFileSystemWatcherGUID = aSessionGuid;

  nsresult rv = SetStartupDelayTimer();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::Stop(nsACString & _retval NS_OUTPARAM)
{
  nsresult rv;
  if (mServiceState == eWatching) {
    rv = StopWatchingFolder();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (mStartupDelayTimer) {
    rv = mStartupDelayTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("%s: app is shutting down, aborting startup delay timer [%08x]",
          __FUNCTION__, mStartupDelayTimer.get()));
  }

  _retval = mFileSystemWatcherGUID;

  // Prevent reference loops and leaks.
  mFileSystemWatcher = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::SetFolder(const nsAString & aNewWatchPath, PRBool aSynchronizeMediaList)
{
  LOG(("%s: %s",
        __FUNCTION__,
        NS_ConvertUTF16toUTF8(aNewWatchPath).get()));

  if (mWatchPath.Equals(aNewWatchPath, CaseInsensitiveCompare)) {
    return NS_OK;
  }

  nsresult rv;

  mWatchPath = aNewWatchPath;
  mShouldSynchronize = aSynchronizeMediaList;

  if (mServiceState == eWatching) {
    TRACE(("%s: already watching, stopping...", __FUNCTION__));
    NS_ENSURE_STATE(mFileSystemWatcher);

    // The service is currently running with a file system watcher
    // that is currently active. The watcher needs to be stopped (
    // without saving a session) and re-started once it has
    // successfully shutdown.

    if (!mFileSystemWatcherGUID.IsEmpty()) {
      // Clear any previously stored data from the session that might
      // be stored in the users profile.
      rv = mFileSystemWatcher->DeleteSession(mFileSystemWatcherGUID);
      if (NS_FAILED(rv)) {
        // Just warn if deleting the previous session fails.
        NS_WARNING("Could not delete old session data!");
      }

      mFileSystemWatcherGUID.Truncate();
    }

    // Set a flag to re-setup a file system watcher once the current
    // one has shutdown.
    mShouldReinitWatcher = PR_TRUE;

    // The service is no longer watching
    mServiceState = eStarted;

    // Flush all event paths, reset flags, and stop the watcher.
    mAddedPaths.clear();
    mRemovedPaths.clear();
    mChangedPaths.clear();
    mDelayedChangedPaths.clear();

    rv = mFileSystemWatcher->StopWatching(PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (mServiceState == eDisabled && !mWatchPath.IsEmpty()) {
    // The service has not started up internally, but the watch path
    // has changed. If the service has been set to be enabled, start
    // the delayed internal startup.
    if (mMediaList) {
      TRACE(("%s: not watching yet, arming...", __FUNCTION__));
      // Now that the service state is disabled, the watch path has
      // been set, and the service should enable - it is time to
      // start the delayed internal init.
      rv = SetStartupDelayTimer();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
sbWatchFolder::Disable()
{
  LOG(("%s", __FUNCTION__));

  nsresult rv;

  // Stop watching since the service is watching and the pref was toggled
  // to not watch.
  if (mServiceState == eWatching) {
    rv = StopWatchingFolder();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult
sbWatchFolder::Enable()
{
  LOG(("%s", __FUNCTION__));

  nsresult rv;

  // Start watching since the service is not watching and the pref was
  // toggled to start watching.
  if (mServiceState == eStarted) {
    rv = StartWatchingFolder();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // The service has not yet attempted to start up and was just turned on.
  // Start the timer if the service is in a disabled state, the watch path
  // has been defined, and the service should enable.
  else if (mServiceState == eDisabled &&
           !mWatchPath.IsEmpty())
  {
    rv = SetStartupDelayTimer();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIWatchFolder

NS_IMETHODIMP sbWatchFolder::GetMediaList(sbIMediaList * *aMediaList)
{
  NS_ENSURE_ARG_POINTER(aMediaList);
  NS_IF_ADDREF(*aMediaList = mMediaList);
  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::SetMediaList(sbIMediaList * aMediaList)
{
  nsresult rv;

  if (mMediaList && aMediaList) {
    PRBool same = PR_FALSE;
    rv = mMediaList->Equals(aMediaList, &same);
    NS_ENSURE_SUCCESS(rv, rv);

    if (same) {
      return NS_OK;
    }
  }

  if (mMediaList) {
    Disable();
  }

  mMediaList = aMediaList;

  if (mMediaList) {
    Enable();
  }

  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::GetPath(nsAString & aPath)
{
  aPath = mWatchPath;
  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::GetImporter(sbIDirectoryImportService * *aImporter)
{
  NS_ENSURE_ARG_POINTER(aImporter);

  nsresult rv;
  nsCOMPtr<sbIDirectoryImportService> importer = mCustomImporter;
  if (!importer) {
    importer = do_GetService(
      "@songbirdnest.com/Songbird/DirectoryImportService;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  importer.forget(aImporter);

  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::SetImporter(sbIDirectoryImportService * aImporter)
{
  mCustomImporter = aImporter;
  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::GetTypeSniffer(sbIMediacoreTypeSniffer * *aTypeSniffer)
{
  NS_ENSURE_ARG_POINTER(aTypeSniffer);
  NS_IF_ADDREF(*aTypeSniffer = mTypeSniffer);
  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::SetTypeSniffer(sbIMediacoreTypeSniffer * aTypeSniffer)
{
  mTypeSniffer = aTypeSniffer;
  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::GetMetadataScanner(sbIFileMetadataService * *aMetadataScanner)
{
  NS_ENSURE_ARG_POINTER(aMetadataScanner);

  nsresult rv;
  nsCOMPtr<sbIFileMetadataService> scanner = mMetadataScanner;
  if (!scanner) {
    scanner = do_GetService(
        "@songbirdnest.com/Songbird/FileMetadataService;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  scanner.forget(aMetadataScanner);

  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::SetMetadataScanner(sbIFileMetadataService * aMetadataScanner)
{
  mMetadataScanner = aMetadataScanner;
  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::GetCanInteract(PRBool *aCanInteract)
{
  NS_ENSURE_ARG_POINTER(aCanInteract);
  *aCanInteract = mCanInteract;
  return NS_OK;
}

NS_IMETHODIMP sbWatchFolder::SetCanInteract(PRBool aCanInteract)
{
  mCanInteract = aCanInteract;
  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::GetIsSupported(PRBool *aIsSupported)
{
  NS_ENSURE_ARG_POINTER(aIsSupported);
  *aIsSupported = mServiceState != eNotSupported;
  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::GetIsRunning(PRBool *aIsRunning)
{
  NS_ENSURE_ARG_POINTER(aIsRunning);
  *aIsRunning = (mServiceState == eWatching);
  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::AddIgnorePath(const nsAString & aFilePath)
{
  LOG(("sbWatchFolder::AddIgnorePath %s",
        NS_LossyConvertUTF16toASCII(aFilePath).get()));

  nsString filePath(aFilePath);

  sbStringMap::iterator it = mIgnorePaths.find(filePath);
  if (it == mIgnorePaths.end()) {
    // new ignore path
    mIgnorePaths[filePath] = ignorePathData_t(1, 0);
  } else {
    // existing ignore path, set the always-ignore flag
    ++(it->second.depth);
  }
  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::RemoveIgnorePath(const nsAString & aFilePath)
{
  LOG(("sbWatchFolder::RemoveIgnorePath %s",
        NS_LossyConvertUTF16toASCII(aFilePath).get()));

  nsString filePath(aFilePath);

  sbStringMap::iterator it = mIgnorePaths.find(filePath);
  // note: there is no error if this is called too many times.
  if (it != mIgnorePaths.end()) {
    it->second.depth = PR_MAX(0, it->second.depth - 1);
    if (it->second.depth < 1 && it->second.count < 1) {
      // there is no counter, the ignore path can be removed
      mIgnorePaths.erase(it);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::AddIgnoreCount(const nsAString & aFilePath,
                                     PRInt32 aCount)
{
  nsString filePath(aFilePath);

  sbStringMap::iterator it = mIgnorePaths.find(filePath);
  if (it == mIgnorePaths.end()) {
    // not found, make a new entry
    ignorePathData_t newData(0, 0);
    it = mIgnorePaths.insert(sbStringMap::value_type(filePath, newData)).first;
  }

  it->second.count += aCount;
  if (it->second.count < 1) {
    it->second.count = 0;
    if (it->second.depth < 1) {
      // the count has reached zero, and there is no always-ignore flag
      mIgnorePaths.erase(it);
    }
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIFileSystemListener

NS_IMETHODIMP
sbWatchFolder::OnWatcherStarted()
{
  nsresult rv;

  // Now start up the timer
  if (!mEventPumpTimer) {
    mEventPumpTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (!mChangeDelayTimer) {
    mChangeDelayTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Process any event received before the watcher has started. These will
  // be all the events from comparing the de-serialized session to the current
  // filesystem state.
  LOG(("%s: arming event pump timer [%08x]",
        __FUNCTION__, mEventPumpTimer.get()));
  rv = mEventPumpTimer->InitWithCallback(this,
                                        EVENT_PUMP_TIMER_DELAY,
                                        nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  mEventPumpTimerIsSet = PR_TRUE;
  mShouldProcessEvents = PR_TRUE;
  mHasWatcherStarted = PR_TRUE;

  TRACE(("sbWatchFolder::OnWatcherStarted (path [%s])",
         NS_ConvertUTF16toUTF8(mWatchPath).get()));
  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::OnWatcherStopped()
{
  if (mEventPumpTimer) {
    mEventPumpTimer->Cancel();
  }
  if (mChangeDelayTimer) {
    mChangeDelayTimer->Cancel();
  }

  mHasWatcherStarted = PR_FALSE;

  // If this flag is set, the watch path has been changed. Since the watcher
  // has now sucesfully stoped, set a callback timer to null out the current
  // file system watcher and create a new one.
  if (mShouldReinitWatcher) {
    nsresult rv;
    if (!mFlushFSWatcherTimer) {
      mFlushFSWatcherTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = mFlushFSWatcherTimer->InitWithCallback(this,
                                                FLUSH_FS_WATCHER_DELAY,
                                                nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  TRACE(("sbWatchFolder::OnWatcherStopped (path [%s])",
         NS_ConvertUTF16toUTF8(mWatchPath).get()));
  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::OnWatcherError(PRUint32 aErrorType,
                                     const nsAString & aDescription)
{
  nsresult rv;
  switch (aErrorType) {
    case sbIFileSystemListener::ROOT_PATH_MISSING:
      NS_WARNING("WARNING: Root path is missing!");
      rv = HandleRootPathMissing();
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case sbIFileSystemListener::INVALID_DIRECTORY:
      NS_WARNING("WARNING: Invalid directory!");
      break;

    case sbIFileSystemListener::SESSION_LOAD_ERROR:
      rv = HandleSessionLoadError();
      NS_ENSURE_SUCCESS(rv, rv);
      break;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::OnFileSystemChanged(const nsAString & aFilePath)
{
  LOG(("sbWatchFolder::OnFileSystemChanged %s",
    NS_LossyConvertUTF16toASCII(aFilePath).get()));

  PRBool isIgnoredPath = PR_FALSE;
  nsresult rv = DecrementIgnoredPathCount(aFilePath, &isIgnoredPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Don't bother with this change if it is currently being ignored.
  if (isIgnoredPath) {
    return NS_OK;
  }

  nsString filePath(aFilePath);

  // The watcher will report all changes from the previous session before the
  // watcher has started. Don't set the timer until then.
  if (mHasWatcherStarted) {
    // See if the changed path set already has this path inside of it.
    sbStringSetIter foundIter = mChangedPaths.find(filePath);
    if (foundIter != mChangedPaths.end()) {
      // If this path is currently in the changed path vector already,
      // delay processing this path until a later time.
      mDelayedChangedPaths.insert(filePath);

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
      mChangedPaths.insert(filePath);
      rv = SetEventPumpTimer();
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else {
    mChangedPaths.insert(filePath);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::OnFileSystemRemoved(const nsAString & aFilePath)
{
  LOG(("sbWatchFolder::OnFileSystemRemoved %s",
    NS_LossyConvertUTF16toASCII(aFilePath).get()));

  PRBool isIgnoredPath = PR_FALSE;
  nsresult rv = DecrementIgnoredPathCount(aFilePath, &isIgnoredPath);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!isIgnoredPath) {
    mRemovedPaths.insert(nsString(aFilePath));

    // The method will guard against |mHasWatcherStarted|
    rv = SetEventPumpTimer();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::OnFileSystemAdded(const nsAString & aFilePath)
{
  LOG(("sbWatchFolder::OnFileSystemAdded %s",
    NS_LossyConvertUTF16toASCII(aFilePath).get()));

  PRBool isIgnoredPath = PR_FALSE;
  nsresult rv = DecrementIgnoredPathCount(aFilePath, &isIgnoredPath);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!isIgnoredPath) {
    mAddedPaths.insert(nsString(aFilePath));

    // The method will guard against |mHasWatcherStarted|
    rv = SetEventPumpTimer();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIMediaListEnumerationListener

NS_IMETHODIMP
sbWatchFolder::OnEnumerationBegin(sbIMediaList *aMediaList,
                                         PRUint16 *aRetVal)
{
  if (!mEnumeratedMediaItems) {
    nsresult rv;
    mEnumeratedMediaItems = 
      do_CreateInstance("@songbirdnest.com/moz/xpcom/threadsafe-array;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aRetVal = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::OnEnumeratedItem(sbIMediaList *aMediaList,
                                       sbIMediaItem *aMediaItem,
                                       PRUint16 *aRetVal)
{
  mEnumeratedMediaItems->AppendElement(aMediaItem, PR_FALSE);
  *aRetVal = sbIMediaListEnumerationListener::CONTINUE;
  return NS_OK;
}

NS_IMETHODIMP
sbWatchFolder::OnEnumerationEnd(sbIMediaList *aMediaList,
                                       nsresult aStatusCode)
{
  nsresult rv;
  PRUint32 length;
  rv = mEnumeratedMediaItems->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG(("%s: Found %i media items during enumeration", __FUNCTION__, length));

  if (length > 0) {
    if (mCurrentProcessType == eRemoval) {
      // Remove the found items from the library, pop up the progress dialog.
      nsCOMPtr<sbIWFRemoveHelper9001> helper =
        do_GetService("@songbirdnest.com/Songbird/RemoveHelper;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      mRemovedPaths.clear();

      helper->Remove(mEnumeratedMediaItems);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    else if (mCurrentProcessType == eChanged) {
      // Rescan the changed items.
      nsCOMPtr<sbIFileMetadataService> metadataService;
      rv = GetMetadataScanner(getter_AddRefs(metadataService));
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbIJobProgress> jobProgress;
      rv = metadataService->Read(mEnumeratedMediaItems,
                                 getter_AddRefs(jobProgress));
      NS_ENSURE_SUCCESS(rv, rv);

    }
    else if (mCurrentProcessType == eMoveOrRename) {
      // Try to detect move/rename
      nsCOMPtr<sbIWFMoveRenameHelper9000> helper =
        do_GetService("@songbirdnest.com/Songbird/MoveRenameHelper;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIArray> uriArray;
      rv = GetURIArrayForStringPaths(mAddedPaths, getter_AddRefs(uriArray));
      NS_ENSURE_SUCCESS(rv, rv);
      mAddedPaths.clear();

#ifdef PR_LOGGING
      PRUint32 length;
      rv = uriArray->GetLength(&length);
      NS_ENSURE_SUCCESS(rv, rv);

      LOG(("%s: Sending %i added item URL's to the move-rename helper",
        __FUNCTION__, length));
#endif

      rv = helper->Process(mEnumeratedMediaItems, uriArray, this);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  else if (mCurrentProcessType == eMoveOrRename) {
    // If no items where found during the move or rename enumeration lookup
    // (i.e. no removed items where found) - add the items that still exist.
    // This usually happens when the a directory changes very fast and none
    // of the removed items actually exist in the library.
    sbStringSet addedPathsCopy = mAddedPaths;
    sbStringSetIter begin = addedPathsCopy.begin();
    sbStringSetIter end = addedPathsCopy.end();
    sbStringSetIter next;
    for (next = begin; next != end; ++next) {
      nsCOMPtr<nsILocalFile> curFile =
        do_CreateInstance("@mozilla.org/file/local;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = curFile->InitWithPath(*next);
      if (NS_FAILED(rv)) {
        NS_WARNING("ERROR: Could not init a nsILocalFile with a path!");
        continue;
      }

      PRBool doesExist = PR_FALSE;
      rv = curFile->Exists(&doesExist);
      if (NS_FAILED(rv) || !doesExist) {
        mAddedPaths.erase(*next);
      }
    }

    rv = ProcessAddedPaths();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mEnumeratedMediaItems->Clear();
  NS_ENSURE_SUCCESS(rv, rv);

  mCurrentProcessType = eNone;
  return NS_OK;
}

//------------------------------------------------------------------------------
// nsITimerCallback

NS_IMETHODIMP
sbWatchFolder::Notify(nsITimer *aTimer)
{
  nsresult rv;

  // Handle startup delay (internally init)
  if (aTimer == mStartupDelayTimer) {
    LOG(("%s: startup delay timer [%08x] fired",
         __FUNCTION__, mStartupDelayTimer.get()));
    rv = InitInternal();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Handle flushing the old file system watcher with a new one
  else if (aTimer == mFlushFSWatcherTimer) {
    mFileSystemWatcher = nsnull;
    mShouldReinitWatcher = PR_FALSE;

    rv = StartWatchingFolder();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Standard processing of removed and non-queued changed paths.
  else if (aTimer == mEventPumpTimer) {
    if (mShouldProcessEvents) {
      // No events have been received since the event pump timer was armed. Go
      // ahead and process the event paths now.
      rv = ProcessEventPaths();
      NS_ENSURE_SUCCESS(rv, rv);

      mEventPumpTimerIsSet = PR_FALSE;
    }
    else {
      // Some event has happened since the last time the event pump timer was
      // armed. Go ahead and wait another |EVENT_PUMP_TIMER_DELAY| milliseconds.
      LOG(("%s: arming event pump timer [%08x]",
            __FUNCTION__, mEventPumpTimer.get()));
      rv = mEventPumpTimer->InitWithCallback(this,
                                             EVENT_PUMP_TIMER_DELAY,
                                             nsITimer::TYPE_ONE_SHOT);
      NS_ENSURE_SUCCESS(rv, rv);

      mShouldProcessEvents = PR_TRUE;
    }
  }

  // Process queued changed event paths.
  else if (aTimer == mChangeDelayTimer) {
    rv = HandleEventPathList(mDelayedChangedPaths, eChanged);
    NS_ENSURE_SUCCESS(rv, rv);

    mChangeDelayTimerIsSet = PR_FALSE;
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIJobProgressListener

NS_IMETHODIMP
sbWatchFolder::OnJobProgress(sbIJobProgress *aJobProgress)
{
  NS_ENSURE_ARG_POINTER(aJobProgress);

  PRUint16 status;
  nsresult rv = aJobProgress->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);

  // This method is the callback for a move-rename process. Simply set the
  // event timer until the job completes. This prevents executing
  // synchronous  move-rename jobs that could cause data loss.

  if (status == sbIJobProgress::STATUS_RUNNING) {
    rv = SetEventPumpTimer();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
