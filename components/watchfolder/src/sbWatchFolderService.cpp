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
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>
#include <sbIApplicationController.h>
#include <sbIDirectoryImportService.h>
#include <sbIFileMetadataService.h>
#include <sbIJobProgress.h>
#include <sbIJobProgressService.h>
#include <sbILibraryManager.h>
#include <sbIPropertyArray.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsILocalFile.h>
#include <nsIPrefBranch2.h>
#include <nsIObserverService.h>
#include <nsIURI.h>

#define PREF_WATCHFOLDER_ROOT        "songbird.watch_folder."
#define PREF_WATCHFOLDER_ENABLE      "songbird.watch_folder.enable"
#define PREF_WATCHFOLDER_PATH        "songbird.watch_folder.path"
#define PREF_WATCHFOLDER_SESSIONGUID "songbird.watch_folder.sessionguid"
#define PREF_FIRSTRUN_CHECK          "songbird.firstrun.check.0.3"

#define STARTUP_TIMER_DELAY      2000
#define FLUSH_FS_WATCHER_DELAY   1000
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
  mHasWatcherStarted = PR_FALSE;
  mShouldReinitWatcher = PR_FALSE;
  mEventPumpTimerIsSet = PR_FALSE;
  mAddDelayTimerIsSet = PR_FALSE;
  mChangeDelayTimerIsSet = PR_FALSE;
  mShouldProcessAddedPaths = PR_FALSE;

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

  PRBool isWatcherSupported = PR_FALSE;
  rv = fileSystemWatcher->GetIsSupported(&isWatcherSupported);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isWatcherSupported) {
    mServiceState = eNotSupported;
    return NS_OK;
  }

  // Assume the service is disabled. This will be verified on the delayed
  // initialization in |InitInternal()|.
  mServiceState = eDisabled;

  // Setup the final-ui-startup launching...
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "quit-application", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

    // songbird.firstrun.check.0.3
  nsCOMPtr<nsIPrefBranch2> prefBranch =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool hasFirstRunCheck = PR_FALSE;
  rv = prefBranch->PrefHasUserValue(PREF_FIRSTRUN_CHECK, &hasFirstRunCheck);
  NS_ENSURE_SUCCESS(rv, rv);

  // First run dialog has run - go ahead and start the delayed init on 
  // "final-ui-startup".
  if (hasFirstRunCheck) {
    rv = observerService->AddObserver(this, "final-ui-startup", PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  // If the first run pref does not exist, wait until after the first run
  // dialog has been completed before delay starting the service.
  else {
    prefBranch->AddObserver(PREF_FIRSTRUN_CHECK, this, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv); 
  }

  return NS_OK;
}

nsresult
sbWatchFolderService::InitInternal()
{
  nsresult rv;
  nsCOMPtr<nsIPrefBranch2> prefBranch =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // First, check to see if the service should be running.
  PRBool shouldEnable;
  rv = prefBranch->GetBoolPref(PREF_WATCHFOLDER_ENABLE, &shouldEnable);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the service as disabled, if this method exits cleanly, the service
  // will be considered 'started'.
  mServiceState = eDisabled;

  // If the service is set to be turned off, do not continue.
  if (!shouldEnable) {
    return NS_OK;
  }

  // Next, read in the watch path.
  nsCOMPtr<nsISupportsString> supportsString;
  rv = prefBranch->GetComplexValue(PREF_WATCHFOLDER_PATH,
                                   NS_GET_IID(nsISupportsString),
                                   getter_AddRefs(supportsString));
  
  // The service can not continue if the watch path does not exist.
  if (NS_FAILED(rv) || !supportsString) {
    return NS_ERROR_UNEXPECTED;
  }

  rv = supportsString->GetData(mWatchPath);
  NS_ENSURE_SUCCESS(rv, rv);

  // Don't start the service if the watch path isn't defined.
  if (mWatchPath.Equals(EmptyString())) {
    return NS_ERROR_UNEXPECTED;
  }

  // Look to see if the watch folder service has saved a previous file system
  // watcher session. 
  // NOTE: Validating the return value is not needed, either there is a 
  // saved session or there is not.
  prefBranch->GetCharPref(PREF_WATCHFOLDER_SESSIONGUID,
                          getter_Copies(mFileSystemWatcherGUID));

  // Setup listeners for the pref branch so the service can update itself
  // when they change.
  rv = prefBranch->AddObserver(PREF_WATCHFOLDER_ROOT, this, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  mLibraryUtils =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Get the main library (all changes will be pushed into this library).
  nsCOMPtr<sbILibraryManager> libraryMgr =
    do_QueryInterface(mLibraryUtils, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryMgr->GetMainLibrary(getter_AddRefs(mMainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // The service is now considered started.
  mServiceState = eStarted;
 
  // Now start watching the folder.
  rv = StartWatchingFolder();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbWatchFolderService::StartWatchingFolder()
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
    rv = mFileSystemWatcher->Init(this, mWatchPath, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = mFileSystemWatcher->InitWithSession(mFileSystemWatcherGUID, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mFileSystemWatcher->StartWatching();
  NS_ENSURE_SUCCESS(rv, rv);

  // The service is now watching
  mServiceState = eWatching;
  return NS_OK;
}

nsresult
sbWatchFolderService::StopWatchingFolder()
{
  if (mServiceState != eWatching) {
    return NS_OK;
  }

  // Clear all event paths.
  mAddedPaths.clear();
  mRemovedPaths.clear();
  mChangedPaths.clear();
  mDelayedChangedPaths.clear();

  nsresult rv;
  if (mFileSystemWatcherGUID.Equals(EmptyCString())) {
    // This is the first time the file system watcher has run. Save the session
    // guid so changes can be determined when the watcher starts next.
    nsCOMPtr<nsIPrefBranch2> prefBranch =
      do_GetService("@mozilla.org/preferences-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mFileSystemWatcher->GetSessionGuid(mFileSystemWatcherGUID);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = prefBranch->SetCharPref(PREF_WATCHFOLDER_SESSIONGUID,
                                 mFileSystemWatcherGUID.get());
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Stop and kill the file-system watcher.
  rv = mFileSystemWatcher->StopWatching(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  mFileSystemWatcher = nsnull;
 
  // The service is no longer watching - mark as |eStarted|.
  mServiceState = eStarted;
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
  
  nsCOMPtr<sbIDirectoryImportService> importService =
    do_GetService("@songbirdnest.com/Songbird/DirectoryImportService;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  //
  // XXX todo This can cause problems if this fires when the user is dragging
  //          and dropping into a playlist. This will need to be fixed.
  //
  nsCOMPtr<sbIDirectoryImportJob> job;
  rv = importService->Import(uriArray, mMainLibrary, -1, getter_AddRefs(job));
  NS_ENSURE_SUCCESS(rv, rv);
  
  nsCOMPtr<sbIJobProgressService> progressService =
    do_GetService("@songbirdnest.com/Songbird/JobProgressService;1", &rv);
  if (NS_SUCCEEDED(rv) && progressService) {
    nsCOMPtr<sbIJobProgress> jobProgress = do_QueryInterface(job, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    rv = progressService->ShowProgressDialog(jobProgress, nsnull, 1);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
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

  return mLibraryUtils->GetFileContentURI(pathFile, aURIRetVal);
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
  *aIsSupported = mServiceState != eNotSupported;
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIFileSystemListener

NS_IMETHODIMP
sbWatchFolderService::OnWatcherStarted()
{
  nsresult rv;

  // Now start up the timer
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

  // Process any event received before the watcher has started. These will
  // be all the events from comparing the de-serialized session to the current
  // filesystem state.
  if (mChangedPaths.size() > 0) {
    rv = SetEventPumpTimer();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  if (mAddedPaths.size() > 0) {
    rv = mAddDelayTimer->InitWithCallback(this, 
                                          ADD_DELAY_TIMER_DELAY,
                                          nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mAddDelayTimerIsSet = PR_TRUE;
    mShouldProcessAddedPaths = PR_TRUE;
  }
  if (mRemovedPaths.size() > 0) {
    rv = SetEventPumpTimer();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mHasWatcherStarted = PR_TRUE;

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
  
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemChanged(const nsAString & aFilePath)
{
  // The watcher will report all changes from the previous session before the
  // watcher has started. Don't set the timer until then.
  if (mHasWatcherStarted) {
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
  }
  else {
    mChangedPaths.push_back(nsString(aFilePath));
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemRemoved(const nsAString & aFilePath)
{
  mRemovedPaths.push_back(nsString(aFilePath));

  // The watcher will report all changes from the previous session before the
  // watcher has started. Don't set the timer until then.
  if (mHasWatcherStarted) {
    nsresult rv = SetEventPumpTimer();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemAdded(const nsAString & aFilePath)
{
  mAddedPaths.push_back(nsString(aFilePath));

  // The watcher will report all changes from the previous session before the
  // watcher has started. Don't set the timer until then.
  if (mHasWatcherStarted) {
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
  
  // Handle startup delay (internally init)
  if (aTimer == mStartupDelayTimer) {
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
    rv = ProcessEventPaths(mChangedPaths, eChanged);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = ProcessEventPaths(mRemovedPaths, eRemoval);
    NS_ENSURE_SUCCESS(rv, rv);

    mEventPumpTimerIsSet = PR_FALSE;
  }

  // Check to see if added paths should be processed.
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

  // Process queued changed event paths. 
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
    // Now that the UI has finally started up, pause shortly before 
    // internally setting up the component.
    mStartupDelayTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mStartupDelayTimer->InitWithCallback(this,
                                              STARTUP_TIMER_DELAY,
                                              nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (strcmp("quit-application", aTopic) == 0) {
    if (mServiceState == eWatching) {
      rv = StopWatchingFolder();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, "final-ui-startup");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, "quit-application");
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIPrefBranch2> prefBranch = 
      do_GetService("@mozilla.org/preferences-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  
    rv = prefBranch->RemoveObserver(PREF_WATCHFOLDER_ROOT, this);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  // Handle pref changing which effects the execution of this service.
  else if (strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic) == 0) {
    nsCOMPtr<nsIPrefBranch2> prefBranch = do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsString changedPrefName(aData);
    if (changedPrefName.Equals(NS_LITERAL_STRING(PREF_WATCHFOLDER_ENABLE))) {
      PRBool shouldEnable = PR_FALSE;
      rv = prefBranch->GetBoolPref(PREF_WATCHFOLDER_ENABLE, &shouldEnable);

      // Stop watching since the service is watching and the pref was toggled
      // to not watch.
      if (mServiceState == eWatching && !shouldEnable) {
        rv = StopWatchingFolder();
        NS_ENSURE_SUCCESS(rv, rv);
      }

      // Start watching since the service is not watching and the pref was
      // toggled to start watching.
      else if (mServiceState == eStarted && shouldEnable) {
        rv = StartWatchingFolder();
        NS_ENSURE_SUCCESS(rv, rv);
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

        if (!newWatchPath.Equals(mWatchPath)) {
          mWatchPath = newWatchPath;
          
          if (mServiceState == eWatching) {
            // The service is currently running with a file system watcher
            // that is currently active. The watcher needs to be stopped (
            // without saving a session) and re-started once it has 
            // successfully shutdown.
          
            // Remove the pref since the watch folder service does not want
            // to load the old session.
            PRBool hasSavedSessionGUID;
            rv = prefBranch->PrefHasUserValue(PREF_WATCHFOLDER_SESSIONGUID,
                                              &hasSavedSessionGUID);
            NS_ENSURE_SUCCESS(rv, rv);
            if (hasSavedSessionGUID) {
              rv = prefBranch->ClearUserPref(PREF_WATCHFOLDER_SESSIONGUID);
              NS_ENSURE_SUCCESS(rv, rv);
              //
              // XXX todo Add delete old session support to 
              //     |sbIFileSystemWatcher|.
              // @see bug 15054
              //
            }

            // Set a flag to re-setup a file system watcher once the current
            // one has shutdown.
            mShouldReinitWatcher = PR_TRUE;

            // Flush all event paths, reset flags, and stop the watcher.
            mAddedPaths.clear();
            mRemovedPaths.clear();
            mChangedPaths.clear();
            mDelayedChangedPaths.clear();

            rv = mFileSystemWatcher->StopWatching(PR_FALSE);
            NS_ENSURE_SUCCESS(rv, rv);
          }
        }
      }
    }
    else if (changedPrefName.Equals(NS_LITERAL_STRING(PREF_FIRSTRUN_CHECK))) {
      // The first run dialog has just closed, start the delayed timer.
      rv = prefBranch->RemoveObserver(PREF_FIRSTRUN_CHECK, this);
      NS_ENSURE_SUCCESS(rv, rv);

      mStartupDelayTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = mStartupDelayTimer->InitWithCallback(this,
                                                STARTUP_TIMER_DELAY,
                                                nsITimer::TYPE_ONE_SHOT);
      NS_ENSURE_SUCCESS(rv, rv);
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

