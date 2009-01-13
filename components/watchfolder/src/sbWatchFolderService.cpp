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

#define PREF_WATCHFOLDER_ENABLE "songbird.watch_folder.enable"
#define PREF_WATCHFOLDER_PATH   "songbird.watch_folder.path"


NS_IMPL_ISUPPORTS5(sbWatchFolderService, 
                   sbIWatchFolderService, 
                   sbIFileSystemListener,
                   sbIMediaListEnumerationListener,
                   nsITimerCallback,
                   nsIObserver)

sbWatchFolderService::sbWatchFolderService()
{
  mIsEnabled = PR_FALSE;
  mIsWatching = PR_FALSE;
  mTimerIsSet = PR_FALSE;
}

sbWatchFolderService::~sbWatchFolderService()
{
}

nsresult
sbWatchFolderService::Init()
{
  nsresult rv;

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
  mAddedPaths.Clear();
  mRemovedPaths.Clear();
  mChangedPaths.Clear();

  // Stop and kill the file-system watcher.
  mFileSystemWatcher->StopWatching();
  mFileSystemWatcher = nsnull;
  
  mIsWatching = PR_FALSE;
  return NS_OK;
}

nsresult
sbWatchFolderService::SetTimer()
{
  if (mTimerIsSet) {
    return NS_OK;
  }

  nsresult rv = mTimer->InitWithCallback(this, 500, nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  mTimerIsSet = PR_TRUE;
  return NS_OK;
}

nsresult
sbWatchFolderService::ProcessChangedPaths()
{
  //
  // TODO: Write Me! 
  //       - see bug 14769.
  //

  mChangedPaths.Clear();
  return NS_OK;
}

nsresult
sbWatchFolderService::ProcessAddedPaths()
{
  PRUint32 addedCount = mAddedPaths.Length();
  if (addedCount == 0) {
    return NS_OK;
  }

  //
  // TODO: Write Me! 
  //       - see bug 14768.
  //

  mAddedPaths.Clear();
  return NS_OK;
}

nsresult
sbWatchFolderService::ProcessRemovedPaths()
{
  PRUint32 removeCount = mRemovedPaths.Length();
  if (removeCount == 0) {
    return NS_OK;
  }

  //
  // TODO: Write Me! 
  //       - see bug 14768.
  //

  mRemovedPaths.Clear();
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbWatchFolderService

NS_IMETHODIMP
sbWatchFolderService::GetIsSupported(PRBool *aIsSupported)
{
  NS_ENSURE_ARG_POINTER(aIsSupported);
  
  //
  // For now, just return PR_TRUE.
  // @see bug 10274.
  //
  *aIsSupported = PR_TRUE;
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIFileSystemListener

NS_IMETHODIMP
sbWatchFolderService::OnWatcherStarted()
{
  // Now start up the timer
  if (!mTimer) {
    nsresult rv;
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnWatcherStopped()
{
  if (mTimer) {
    mTimer->Cancel();
  }
  
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemChanged(const nsAString & aFilePath)
{
  mChangedPaths.AppendElement(aFilePath);
  nsresult rv = SetTimer();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemRemoved(const nsAString & aFilePath)
{
  mRemovedPaths.AppendElement(aFilePath);
  nsresult rv = SetTimer();
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemAdded(const nsAString & aFilePath)
{
  mAddedPaths.AppendElement(aFilePath);
  nsresult rv = SetTimer();
  NS_ENSURE_SUCCESS(rv, rv);
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
  nsresult rv = mEnumeratedMediaItems->Clear();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsITimerCallback

NS_IMETHODIMP
sbWatchFolderService::Notify(nsITimer *aTimer)
{
  nsresult rv = ProcessChangedPaths();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ProcessAddedPaths();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = ProcessRemovedPaths();
  NS_ENSURE_SUCCESS(rv, rv);

  mTimerIsSet = PR_FALSE;

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

