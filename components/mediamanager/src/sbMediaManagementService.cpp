/* vim: set sw=2 :*/
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

#include "sbMediaManagementService.h"

/**
 * mozilla interfaces
 */
#include <nsIConsoleService.h>
#include <nsIObserverService.h>
#include <nsIPrefBranch2.h>
#include <nsIPrefService.h>
#include <nsIProxyObjectManager.h>
#include <nsIScriptError.h>
#include <nsISupportsPrimitives.h>
#include <nsIThread.h>
#include <nsITimer.h>
#include <nsIURI.h>

/**
 * songbird interfaces
 */
#include <sbIApplicationController.h>
#include <sbIJobProgressService.h>
#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbIMediaFileManager.h>
#include <sbIMediaManagementJob.h>
#include <sbIPrompter.h>
#include <sbStringBundle.h>

/**
 * other mozilla headers
 */
#include <prtime.h>
#include <nsAutoLock.h>
#include <nsIFileURL.h>
#include <nsILocalFile.h>
#include <nsISupportsUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>

/**
 * other songbird headers
 */
#include <sbLibraryUtils.h>
#include <sbPropertiesCID.h>
#include <sbProxiedComponentManager.h>
#include <sbStandardProperties.h>
#include <sbStringUtils.h>

/**
 * constants
 */

#define MMS_STARTUP_DELAY  (10 * PR_MSEC_PER_SEC)
#define MMS_SCAN_DELAY     (5 * PR_MSEC_PER_SEC)
#define MMS_PROGRESS_DELAY (1 * PR_MSEC_PER_SEC)

/**
 * logging
 *
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediaManagementService:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediaManagementServiceLog = nsnull;
#define TRACE(args) PR_LOG(gMediaManagementServiceLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediaManagementServiceLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif /* PR_LOGGING */

/**
 * local type declarations
 */
struct ProcessItemData {
  /**
   * a structure used to pass data to the ProcessItem enumeration callback.
   * the items here are shared across the items being enumerated.
   *
   * these pointers are not owning; they are temporary only
   */
  sbMediaManagementService* mediaMgmtService;
  sbIMediaFileManager* fileMan;
  sbMediaManagementService::DirtyItems_t* dirtyItems;
  bool hadErrors;
};

NS_IMPL_THREADSAFE_ISUPPORTS5(sbMediaManagementService,
                              sbIMediaManagementService,
                              sbIMediaListListener,
                              sbIJobProgressListener,
                              nsITimerCallback,
                              nsIObserver)

sbMediaManagementService::sbMediaManagementService()
  : mEnabled(PR_FALSE)
{
#ifdef PR_LOGGING
  if (!gMediaManagementServiceLog) {
    gMediaManagementServiceLog = PR_NewLogModule("sbMediaManagementService");
  }
#endif
}

sbMediaManagementService::~sbMediaManagementService()
{
  /* destructor code */
}

/*****
 * sbIMediaManagementService
 *****/

/* readonly attribute sbILibrary managedLibrary; */
NS_IMETHODIMP
sbMediaManagementService::GetManagedLibrary(sbILibrary * *aManagedLibrary)
{
  NS_ENSURE_ARG_POINTER(aManagedLibrary);
  NS_ENSURE_TRUE(mLibrary, NS_ERROR_ALREADY_INITIALIZED);

  return CallQueryInterface(mLibrary, aManagedLibrary);
}

/* attribute boolean isEnabled; */
NS_IMETHODIMP
sbMediaManagementService::GetIsEnabled(PRBool *aIsEnabled)
{
  NS_ENSURE_ARG_POINTER(aIsEnabled);
  *aIsEnabled = mEnabled;
  return NS_OK;
}
NS_IMETHODIMP
sbMediaManagementService::SetIsEnabled(PRBool aIsEnabled)
{
  // XXX Mook: needs to start doing things
  nsresult rv;
  if (aIsEnabled == mEnabled) {
    // no transition, don't do anything
    return NS_OK;
  }

  // mark everything as not scanned, since we don't know if things have been
  // added
  rv = mPrefBranch->ClearUserPref(SB_PREF_MEDIA_MANAGER_COMPLETE);
  if (rv != NS_ERROR_UNEXPECTED) {
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aIsEnabled) {
    // disabled -> enabled transition
    if (!mDelayedStartupTimer) {
      mDelayedStartupTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    TRACE(("%s: arming delayed startup timer due to manual enable", __FUNCTION__));
    rv = mDelayedStartupTimer->InitWithCallback(this,
                                                MMS_SCAN_DELAY,
                                                nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);

    mEnabled = aIsEnabled;

    return NS_OK;

  } else {
    // enabled -> disabled transition
    rv = StopListening();
    NS_ENSURE_SUCCESS(rv, rv);

    mEnabled = aIsEnabled;
    return NS_OK;
  }
  NS_NOTREACHED("neither enabled nor disabled!");
  return NS_ERROR_UNEXPECTED;
}

/* readonly attribute boolean isScanning; */
NS_IMETHODIMP
sbMediaManagementService::GetIsScanning(PRBool *aIsScanning)
{
  NS_ENSURE_ARG_POINTER(aIsScanning);
  *aIsScanning = (mLibraryScanJob != nsnull);
  return NS_OK;
}

/*****
 * sbIMediaManagementService
 *****/

/* void observe (in nsISupports aSubject, in string aTopic, in wstring aData); */
NS_IMETHODIMP
sbMediaManagementService::Observe(nsISupports *aSubject,
                                  const char *aTopic,
                                  const PRUnichar *aData)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(aTopic);
  NS_ASSERTION(NS_IsMainThread(), "observe not on main thread");
  TRACE(("%s: observing %s", __FUNCTION__, aTopic));

  if (!strcmp("profile-after-change", aTopic)) {
    nsCOMPtr<nsIObserverService> obs =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obs->AddObserver(this,
                          SB_LIBRARY_MANAGER_READY_TOPIC,
                          PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obs->AddObserver(this,
                          SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC,
                          PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  }

  if (!strcmp(SB_LIBRARY_MANAGER_READY_TOPIC, aTopic)) {
    // XXX Mook: manage main lib only for now? :(
    rv = GetMainLibrary(getter_AddRefs(mLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserverService> obs =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obs->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mPrefBranch->GetBoolPref(SB_PREF_MEDIA_MANAGER_ENABLED, &mEnabled);
    if (NS_FAILED(rv)) {
      mEnabled = PR_FALSE;
    }

    if (!mEnabled) {
      TRACE(("not enabled, don't bother doing anything else"));
      return NS_OK;
    }

    // library management is enabled, setup startup timer
    if (!mDelayedStartupTimer) {
      mDelayedStartupTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    TRACE(("%s: arming delayed startup timer", __FUNCTION__));
    rv = mDelayedStartupTimer->InitWithCallback(this,
                                                MMS_STARTUP_DELAY,
                                                nsITimer::TYPE_ONE_SHOT);
    if (NS_FAILED(rv)) {
      mDelayedStartupTimer = nsnull;
      NS_ENSURE_SUCCESS(rv, rv);
    }
    return NS_OK;
  }

  if (!strcmp(SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, aTopic)) {
    TRACE(("%s: shutting down", __FUNCTION__));
    rv = StopListening();
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRunnable> runnable =
      NS_NEW_RUNNABLE_METHOD(sbMediaManagementService, this, ShutdownProcessActionThread);
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

    rv = mPerformActionThread->Dispatch(runnable, nsIEventTarget::DISPATCH_SYNC);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mPerformActionThread->Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);

    mPerformActionThread = nsnull;
    mLibrary = nsnull;

    nsAutoLock::DestroyLock(mDirtyItemsLock);
    mDirtyItemsLock = nsnull;
    mDirtyItems->Clear();
    mDirtyItems = nsnull;

    if (mPrefBranch) {
      nsCOMPtr<nsIPrefBranch2> prefBranch2 = do_QueryInterface(mPrefBranch, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = prefBranch2->RemoveObserver(SB_PREF_MEDIA_MANAGER_LISTEN, this);
      NS_ENSURE_SUCCESS(rv, rv);
      mPrefBranch = nsnull;
    }

    return NS_OK;
  }

  if (!strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic)) {
    nsString modifiedPref(aData);
    if (modifiedPref.EqualsLiteral(SB_PREF_MEDIA_MANAGER_FMTFILE) ||
        modifiedPref.EqualsLiteral(SB_PREF_MEDIA_MANAGER_FMTDIR)) {
      return SetupLibraryListener();
    }
    return NS_OK;
  }

  if (!strcmp("app-startup", aTopic)) {
    return NS_OK;
  }

  return NS_ERROR_NOT_IMPLEMENTED;
}

/*****
 * sbIMediaListListener
 *****/

/* boolean onItemAdded (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem,
                        in unsigned long aIndex); */
NS_IMETHODIMP
sbMediaManagementService::OnItemAdded(sbIMediaList *aMediaList,
                                      sbIMediaItem *aMediaItem,
                                      PRUint32 aIndex,
                                      PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE; /* keep listening to messages */
  // need to use copy + move so that the file ends up in the correct directory
  // instead of the root of the management directory
  return this->QueueItem(aMediaItem,
                         sbIMediaFileManager::MANAGE_COPY |
                         sbIMediaFileManager::MANAGE_MOVE);
}

/* boolean onBeforeItemRemoved (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem,
                                in unsigned long aIndex); */
NS_IMETHODIMP
sbMediaManagementService::OnBeforeItemRemoved(sbIMediaList *aMediaList,
                                              sbIMediaItem *aMediaItem,
                                              PRUint32 aIndex,
                                              PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE; /* keep listening to messages */
  return this->QueueItem(aMediaItem, sbIMediaFileManager::MANAGE_DELETE);
}

/* boolean onAfterItemRemoved (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem,
                               in unsigned long aIndex); */
NS_IMETHODIMP
sbMediaManagementService::OnAfterItemRemoved(sbIMediaList *aMediaList,
                                             sbIMediaItem *aMediaItem,
                                             PRUint32 aIndex,
                                             PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE; /* skip all further OnAfterItemRemoved */
  return NS_OK;
}

/* boolean onItemUpdated (in sbIMediaList aMediaList, in sbIMediaItem aMediaItem,
                          in sbIPropertyArray aProperties); */
NS_IMETHODIMP
sbMediaManagementService::OnItemUpdated(sbIMediaList *aMediaList,
                                        sbIMediaItem *aMediaItem,
                                        sbIPropertyArray *aProperties,
                                        PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE; /* keep listening to messages */
  return this->QueueItem(aMediaItem, sbIMediaFileManager::MANAGE_MOVE);
}

/* boolean onItemMoved (in sbIMediaList aMediaList, in unsigned long aFromIndex,
                        in unsigned long aToIndex); */
NS_IMETHODIMP
sbMediaManagementService::OnItemMoved(sbIMediaList *aMediaList,
                                      PRUint32 aFromIndex,
                                      PRUint32 aToIndex,
                                      PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE; /* skip all further OnItemMoved */
  return NS_OK;
}

/* boolean onBeforeListCleared (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaManagementService::OnBeforeListCleared(sbIMediaList *aMediaList,
                                              PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE; /* keep listening to messages */
  return this->QueueItems(aMediaList, sbIMediaFileManager::MANAGE_DELETE);;
}

/* boolean onListCleared (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaManagementService::OnListCleared(sbIMediaList *aMediaList,
                                        PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_TRUE; /* skip all further OnListCleared */
  return NS_OK;
}

/* void onBatchBegin (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaManagementService::OnBatchBegin(sbIMediaList *aMediaList)
{
  /* we don't care about batching, we use a timeout anyway */
  return NS_OK;
}

/* void onBatchEnd (in sbIMediaList aMediaList); */
NS_IMETHODIMP
sbMediaManagementService::OnBatchEnd(sbIMediaList *aMediaList)
{
  /* we don't care about batching, we use a timeout anyway */
  return NS_OK;
}


/*****
 * sbIJobProgressListener
 *****/
/* void onJobProgress (in sbIJobProgress aJobProgress); */
NS_IMETHODIMP
sbMediaManagementService::OnJobProgress(sbIJobProgress *aJobProgress)
{
  NS_ENSURE_ARG_POINTER(aJobProgress);
  NS_ENSURE_TRUE(mPerformActionTimer, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  PRUint16 jobStatus;
  rv = aJobProgress->GetStatus(&jobStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  if (jobStatus == sbIJobProgress::STATUS_RUNNING) {
    // still running, ignore
    return NS_OK;
  }

  // Get progress and total to see if we completed.
  PRUint32 progress;
  PRUint32 total;
  rv = aJobProgress->GetProgress(&progress);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aJobProgress->GetTotal(&total);
  NS_ENSURE_SUCCESS(rv, rv);

  // job complete or canceled
  rv = mLibraryScanJob->RemoveJobProgressListener(this);
  mLibraryScanJob = nsnull;
  NS_ENSURE_SUCCESS(rv, rv);

  // mark that we've done this once so we don't redo this unncessarily
  // the next time we start the app if we completed the job.
  if (progress == total) {
    rv = mPrefBranch->SetBoolPref(SB_PREF_MEDIA_MANAGER_COMPLETE, PR_TRUE);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  PRUint32 itemCount;
  { /* scope */
    nsAutoLock lock(mDirtyItemsLock);
    itemCount = mDirtyItems->Count();
  }
  if (itemCount > 0) {
    // have queued jobs

    TRACE(("%s: job complete, has queue, setting timer", __FUNCTION__));
    rv = mPerformActionTimer->InitWithCallback(this,
                                               MMS_SCAN_DELAY,
                                               nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


/*****
 * nsITimerCallback
 *****/
/* void notify (in nsITimer timer); */
NS_IMETHODIMP
sbMediaManagementService::Notify(nsITimer *aTimer)
{
  NS_ENSURE_ARG_POINTER(aTimer);
  nsresult rv;

  if (aTimer == mDelayedStartupTimer) {
    TRACE(("%s: delayed startup timer fired", __FUNCTION__));
    mDelayedStartupTimer = nsnull;

    rv = StartListening();
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool isScanComplete;
    rv = mPrefBranch->GetBoolPref(SB_PREF_MEDIA_MANAGER_COMPLETE, &isScanComplete);
    if (NS_SUCCEEDED(rv) && isScanComplete) {
      // we've previously completed a full manage, don't do that again
      PRUint32 itemCount;
      { /* scope */
        nsAutoLock lock(mDirtyItemsLock);
        itemCount = mDirtyItems->Count();
      }
      if (itemCount > 0) {
        // have queued jobs

        TRACE(("%s: scan skipped, has queue, setting timer", __FUNCTION__));
        rv = mPerformActionTimer->InitWithCallback(this,
                                                   MMS_SCAN_DELAY,
                                                   nsITimer::TYPE_ONE_SHOT);
        NS_ENSURE_SUCCESS(rv, rv);
      }
      return NS_OK;
    }

    nsCOMPtr<sbIJobProgressService> jobProgressSvc =
      do_GetService("@songbirdnest.com/Songbird/JobProgressService;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIThread> target;
    rv = NS_GetMainThread(getter_AddRefs(target));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = do_GetProxyForObject(target,
                              NS_GET_IID(sbIJobProgressService),
                              jobProgressSvc,
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(mJobProgressSvc));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIRunnable> runnable =
      NS_NEW_RUNNABLE_METHOD(sbMediaManagementService, this, ScanLibrary);
    NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

    rv = mPerformActionThread->Dispatch(runnable, nsIEventTarget::DISPATCH_NORMAL);
    NS_ENSURE_SUCCESS(rv, rv);

    return NS_OK;
  } else if (aTimer == mPerformActionTimer) {
    TRACE(("%s: perform action timer fired", __FUNCTION__));

    NS_ENSURE_TRUE(mDirtyItemsLock, NS_ERROR_NOT_INITIALIZED);
    { /* scope */
      nsAutoLock lock(mDirtyItemsLock);
      NS_ENSURE_TRUE(mDirtyItems, NS_ERROR_NOT_INITIALIZED);
      NS_ENSURE_TRUE(mDirtyItems->IsInitialized(), NS_ERROR_NOT_INITIALIZED);
    }

    // get the management type
    mManageMode = 0;
    PRBool isSet;

    // Check if Copy is enabled
    rv = mPrefBranch->GetBoolPref(SB_PREF_MEDIA_MANAGER_COPY, &isSet);
    if (NS_SUCCEEDED(rv) && isSet) {
      mManageMode |= sbIMediaFileManager::MANAGE_COPY;
    }
    // Check if Move is enabled
    rv = mPrefBranch->GetBoolPref(SB_PREF_MEDIA_MANAGER_MOVE, &isSet);
    if (NS_SUCCEEDED(rv) && isSet) {
      mManageMode |= sbIMediaFileManager::MANAGE_MOVE;
    }
    // Check if Rename is enabled
    rv = mPrefBranch->GetBoolPref(SB_PREF_MEDIA_MANAGER_RENAME, &isSet);
    if (NS_SUCCEEDED(rv) && isSet) {
      mManageMode |= sbIMediaFileManager::MANAGE_RENAME;
    }
    // Check if Delete is enabled
    rv = mPrefBranch->GetBoolPref(SB_PREF_MEDIA_MANAGER_DELETE, &isSet);
    if (NS_SUCCEEDED(rv) && isSet) {
      mManageMode |= sbIMediaFileManager::MANAGE_DELETE;
    }

    if (mManageMode) {
      nsCOMPtr<sbIMediaFileManager> fileMan =
        do_CreateInstance(SB_MEDIAFILEMANAGER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = fileMan->Init(nsnull);
      NS_ENSURE_SUCCESS(rv, rv);

      ProcessItemData data;
      { /* scope */
        nsAutoLock lock(mDirtyItemsLock);
        data.mediaMgmtService = this;
        data.fileMan = fileMan;
        data.dirtyItems = mDirtyItems.forget();
        data.hadErrors = false;
        mDirtyItems = new DirtyItems_t;
        NS_ENSURE_TRUE(mDirtyItems, NS_ERROR_OUT_OF_MEMORY);
        PRBool success = mDirtyItems->Init();
        if (!success) {
          data.dirtyItems->Clear();
          delete data.dirtyItems;
          return NS_ERROR_OUT_OF_MEMORY;
        }
      }

      PRUint32 count = data.dirtyItems->EnumerateRead(ProcessItem, &data);
      // Check if errors occured and inform user...
      if (data.hadErrors) {
        // Reset the scan preference so on next run we do a full scan.
        rv = mPrefBranch->SetBoolPref(SB_PREF_MEDIA_MANAGER_COMPLETE, PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);
        // Report the error to the user.
        rv = ReportError();
        NS_ENSURE_SUCCESS(rv, rv);
      }
      data.dirtyItems->Clear();
      delete data.dirtyItems;
    }
    return NS_OK;
  }

  NS_NOTREACHED("sbMediaManagementService::Notify on an unknown timer");
  return NS_ERROR_UNEXPECTED;
}

/*****
 * sbMediaManagementService
 *****/

NS_METHOD
sbMediaManagementService::Init()
{
  TRACE(("%s: initing", __FUNCTION__));
  NS_ENSURE_FALSE(mLibrary, NS_ERROR_ALREADY_INITIALIZED);
  nsresult rv;

  mDirtyItemsLock = nsAutoLock::NewLock("sbMediaManagementService::mDirtyItemsLock");
  NS_ENSURE_TRUE(mDirtyItemsLock, NS_ERROR_OUT_OF_MEMORY);

  mDirtyItems = new DirtyItems_t;
  NS_ENSURE_TRUE(mDirtyItems, NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mDirtyItems->Init(), NS_ERROR_OUT_OF_MEMORY);

  nsCOMPtr<nsIObserverService> obs =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obs->AddObserver(this,
                        "profile-after-change",
                        PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NEW_RUNNABLE_METHOD(sbMediaManagementService, this, InitProcessActionThread);
  NS_ENSURE_TRUE(runnable, NS_ERROR_OUT_OF_MEMORY);

  rv = NS_NewThread(getter_AddRefs(mPerformActionThread), runnable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefService> prefRoot =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS (rv, rv);

  // Get the branch we are interested in
  nsCOMPtr<nsIThread> mainThread;
  rv = NS_GetMainThread(getter_AddRefs(mainThread));
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIPrefBranch> prefBranch;
  rv = prefRoot->GetBranch(SB_PREF_MEDIA_MANAGER_ROOT,
                           getter_AddRefs(prefBranch));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = do_GetProxyForObject(mainThread,
                            prefBranch.get(),
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mPrefBranch));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbMediaManagementService::InitProcessActionThread()
{
  NS_PRECONDITION(!mPerformActionTimer, "mPerformActionTimer already initialized!");
  nsresult rv;
  mPerformActionTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);
}

void
sbMediaManagementService::ShutdownProcessActionThread()
{
  NS_PRECONDITION(mPerformActionTimer, "mPerformActionTimer not initialized!");
  nsresult rv;

  NS_ENSURE_TRUE(mDirtyItemsLock, /* void */);
  NS_ENSURE_TRUE(mDirtyItems, /* void */);

  PRUint32 itemCount;
  { /* scope */
    nsAutoLock lock(mDirtyItemsLock);
    itemCount = mDirtyItems->Count();
  }
  if (itemCount > 0) {
    // have queued jobs, flush them
    rv = Notify(mPerformActionTimer);
    if (NS_FAILED(rv)) {
      nsresult __rv = rv;
      NS_ENSURE_SUCCESS_BODY(rv, rv);
    }
  }

  if (mPerformActionTimer) {
    rv = mPerformActionTimer->Cancel();
    if (NS_FAILED(rv)) {
      nsresult __rv = rv;
      NS_ENSURE_SUCCESS_BODY(rv, rv);
    }

    mPerformActionTimer = nsnull;
  }
}

void
sbMediaManagementService::ScanLibrary()
{
  NS_ENSURE_TRUE(mLibrary, /* void */);
  NS_ENSURE_FALSE(mLibraryScanJob, /* void */);

  nsresult rv;

  mLibraryScanJob = do_CreateInstance(SB_MEDIAMANAGEMENTJOB_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = mLibraryScanJob->AddJobProgressListener(this);
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = mLibraryScanJob->Init(mLibrary, nsnull);
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = mLibraryScanJob->OrganizeMediaList();
  NS_ENSURE_SUCCESS(rv, /* void */);

  rv = mJobProgressSvc->ShowProgressDialog(mLibraryScanJob,
                                           nsnull,
                                           MMS_PROGRESS_DELAY);
  NS_ENSURE_SUCCESS(rv, /* void */);
}

NS_METHOD
sbMediaManagementService::QueueItem(sbIMediaItem* aItem, PRUint32 aOperation)
{
  NS_ENSURE_TRUE(mDirtyItemsLock, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mDirtyItems, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_ARG_POINTER(aItem);
  PRBool success;
  nsresult rv;

  #if DEBUG
  PRUint32 oldOp;
  { /* scope */
    nsAutoLock lock(mDirtyItemsLock);
    success = mDirtyItems->Get(aItem, &oldOp);
  }
  if (success) {
    NS_ASSERTION(oldOp != sbIMediaFileManager::MANAGE_DELETE,
                 "operations on a deleted media item!");
  }
  #endif

  if (aOperation & sbIMediaFileManager::MANAGE_DELETE) {
    // Check if we _really_ should remove this item (see bug 17272)
    nsAutoString deleteFromDisk;
    rv = aItem->GetProperty(NS_LITERAL_STRING("http://songbirdnest.com/data/1.0#deleteFromDisk"),
                            deleteFromDisk);
    NS_ENSURE_SUCCESS(rv, rv);
    if (!deleteFromDisk.Equals(NS_LITERAL_STRING("1"))) {
      #if PR_LOGGING
        nsString src;
        rv = aItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL), src);
        TRACE(("%s: item %s not really marked for deletion",
               __FUNCTION__,
               src.get()));
      #endif
      // No need to queue an item we are not asked to remove from disk.
      return NS_OK;
    }
  }

  { /* scope */
    nsAutoLock lock(mDirtyItemsLock);
    success = mDirtyItems->Put(aItem, aOperation);
  }
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);

  if (mLibraryScanJob) {
    TRACE(("%s: item changed, not setting timer due to library scan job",
           __FUNCTION__));
    return NS_OK;
  }

  TRACE(("%s: item changed, setting timer", __FUNCTION__));
  rv = mPerformActionTimer->InitWithCallback(this,
                                             MMS_SCAN_DELAY,
                                             nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_METHOD
sbMediaManagementService::QueueItems(sbIMediaList* aList, PRUint32 aOperation)
{
  NS_ENSURE_ARG_POINTER(aList);
  nsresult rv;

  PRUint32 totalItems;
  rv = aList->GetLength(&totalItems);
  NS_ENSURE_SUCCESS(rv, rv);

  for (PRUint32 itemCount = 0; itemCount < totalItems; itemCount++) {
    // Get the next item
    nsCOMPtr<sbIMediaItem> nextItem;
    rv = aList->GetItemByIndex(itemCount, getter_AddRefs(nextItem));
    NS_ENSURE_SUCCESS(rv, rv);
    // Queue it up to be processed.
    rv = this->QueueItem(nextItem, aOperation);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

NS_METHOD
sbMediaManagementService::StartListening()
{
  TRACE(("%s: starting", __FUNCTION__));
  NS_ENSURE_TRUE(mLibrary, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;

  // Call setup which will create filters and such
  rv = SetupLibraryListener();
  NS_ENSURE_SUCCESS(rv, rv);

  // Also listen for changes to the format preferences so we can
  // reload the filter on the listener.
  nsCOMPtr<nsIPrefBranch2> prefBranch2 = do_QueryInterface(mPrefBranch, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prefBranch2->AddObserver(SB_PREF_MEDIA_MANAGER_LISTEN, this, false);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

NS_METHOD
sbMediaManagementService::StopListening()
{
  TRACE(("%s: stopping", __FUNCTION__));
  NS_ENSURE_TRUE(mLibrary, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(mPerformActionTimer, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;

  rv = mLibrary->RemoveListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIPrefBranch2> prefBranch2 = do_QueryInterface(mPrefBranch, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = prefBranch2->RemoveObserver(SB_PREF_MEDIA_MANAGER_LISTEN, this);
  NS_ENSURE_SUCCESS(rv, rv);

  // flush
  rv = mPerformActionTimer->InitWithCallback(this, 0, nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_METHOD
sbMediaManagementService::ReportError()
{
  TRACE(("%s", __FUNCTION__));

  sbStringBundle bundle;
  nsString dialogTitle = bundle.Get("mediamanager.import_manage_error.title2");
  nsString dialogText = bundle.Get("mediamanager.import_manage_error.text2");

  nsresult rv;
  nsCOMPtr<sbIPrompter> prompter =
    do_CreateInstance("@songbirdnest.com/Songbird/Prompter;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prompter->SetWaitForWindow(PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prompter->Alert(nsnull,
                       dialogTitle.BeginReading(),
                       dialogText.BeginReading());
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */
PLDHashOperator
sbMediaManagementService::ProcessItem(nsISupports* aKey,
                                      PRUint32 aOperation,
                                      void* aClosure)
{
  nsresult rv;

  ProcessItemData* data =
    reinterpret_cast<ProcessItemData*>(aClosure);
  PRUint32 opMask = data->mediaMgmtService->mManageMode;

  aOperation &= opMask;
  if ((aOperation & sbIMediaFileManager::MANAGE_COPY) ||
      (aOperation & sbIMediaFileManager::MANAGE_MOVE))
  {
    /* on copy and move, also rename if enabled */
    aOperation |= (opMask & sbIMediaFileManager::MANAGE_RENAME);
  }

  if (!aOperation) {
    // we don't want to manage this
    return PL_DHASH_NEXT;
  }

  nsCOMPtr<sbIMediaItem> item = do_QueryInterface(aKey);
  NS_ENSURE_TRUE(item, PL_DHASH_STOP);

  // Check if this is a valid media item skip to next if not
  PRBool isValid = PR_FALSE;
  rv = data->mediaMgmtService->IsValidMediaItem(item, &isValid);
  NS_ENSURE_SUCCESS(rv, PL_DHASH_NEXT);
  if (!isValid) {
    // This is not a valid media item so skip.
    return PL_DHASH_NEXT;
  }

  PRBool success;
  rv = data->fileMan->OrganizeItem(item, aOperation, nsnull, &success);
  if (NS_FAILED(rv) || !success) {
    data->hadErrors = true;
    // Log what file failed...
    nsString message(NS_LITERAL_STRING("Unable to manage file: "));

    nsCOMPtr<nsIURI> uri;
    rv = item->GetContentSrc(getter_AddRefs(uri));
    NS_ENSURE_SUCCESS( rv, PL_DHASH_NEXT );

    nsCString uriSpec;
    rv = uri->GetSpec(uriSpec);
    if (NS_SUCCEEDED(rv)) {
      message.AppendLiteral(uriSpec.get());
    } else {
      message.AppendLiteral("Unknown File");
    }

    nsCOMPtr<nsIConsoleService> consoleService =
        do_GetService("@mozilla.org/consoleservice;1", &rv);
    NS_ENSURE_SUCCESS( rv, PL_DHASH_NEXT );
    nsCOMPtr<nsIScriptError> scriptError =
        do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS( rv, PL_DHASH_NEXT );
    if (scriptError) {
      rv = scriptError->Init(message.get(),
                             EmptyString().get(),
                             EmptyString().get(),
                             0, // No line number
                             0, // No column number
                             0, // An error message.
                             "MediaManagment:OrganizeItem");
      if (NS_SUCCEEDED(rv)) {
        consoleService->LogMessage(scriptError);
      }
    }

    if (!success) {
      NS_WARNING("%s: OrganizeItem failed with no error code");
    } else {
      nsresult __rv = rv;
      NS_ENSURE_SUCCESS_BODY(rv, rv);
    }
  }
  return PL_DHASH_NEXT;
}

NS_METHOD
sbMediaManagementService::SetupLibraryListener()
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_TRUE(mLibrary, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;

  // First we update the filter properties
  nsCOMPtr<sbIMutablePropertyArray> filterProperties;
  filterProperties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = filterProperties->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CreatePropertyFilter(filterProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  // Now listen to the library (removing previous listeners first)
  rv = mLibrary->RemoveListener(this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mLibrary->AddListener(this,
                             PR_FALSE,
                             sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                               sbIMediaList::LISTENER_FLAGS_BEFOREITEMREMOVED |
                               sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                               sbIMediaList::LISTENER_FLAGS_BEFORELISTCLEARED,
                             filterProperties);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_METHOD
sbMediaManagementService::AddPropertiesToFilter(const char *aKeyName,
                                                sbIMutablePropertyArray *aFilter)
{
  TRACE(("%s", __FUNCTION__));
  NS_ENSURE_ARG_POINTER(aKeyName);
  nsresult rv;

  nsCString pathPropList;
  rv = mPrefBranch->GetCharPref(aKeyName, getter_Copies(pathPropList));
  NS_ENSURE_SUCCESS(rv, rv);

  if (!pathPropList.IsEmpty()) {
    // Split it up and add the properties to an array
    nsTArray<nsString> listValues;
    nsString_Split(NS_ConvertUTF8toUTF16(pathPropList),
                   NS_LITERAL_STRING(","),
                   listValues);
    for (PRUint32 i = 0; i < listValues.Length(); i++) {
      if ((i % 2) == 0) {
        rv = aFilter->AppendProperty(listValues[i], EmptyString());
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
  }

  return NS_OK;
}

NS_METHOD
sbMediaManagementService::CreatePropertyFilter(sbIMutablePropertyArray* aFilter)
{
  TRACE(("%s", __FUNCTION__));
  nsresult rv;

  rv = AddPropertiesToFilter(SB_PREF_MEDIA_MANAGER_FMTFILE, aFilter);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = AddPropertiesToFilter(SB_PREF_MEDIA_MANAGER_FMTDIR, aFilter);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_METHOD
sbMediaManagementService::IsValidMediaItem(sbIMediaItem *aMediaItem,
                                           PRBool *_retval)
{
  nsresult rv;
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = PR_FALSE; // Default to this not being a valid item

  // First check if it is a list
  nsAutoString isList;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_ISLIST), isList);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isList.IsEmpty() && isList.EqualsLiteral("1")) {
    // retval is false so return
    return NS_OK;
  }

  // Now check the hidden property
  nsAutoString isHidden;
  rv = aMediaItem->GetProperty(NS_LITERAL_STRING(SB_PROPERTY_HIDDEN), isHidden);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!isHidden.IsEmpty() && isHidden.EqualsLiteral("1")) {
    // retval is false so return
    return NS_OK;
  }

  // Check if this is a local file, and exists
  nsRefPtr<nsIURI> uri;
  rv = aMediaItem->GetContentSrc(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(uri, &rv);
  if (NS_FAILED(rv)) {
    // Not a local file, not valid media item
    // retval is false so return
    return NS_OK;
  }

  nsCOMPtr<nsIFile> file;
  rv = fileUrl->GetFile(getter_AddRefs(file));
  if (NS_FAILED(rv)) {
    // Not a local file, not valid media item
    // retval is false so return
    return NS_OK;
  }

  PRBool exists;
  rv = file->Exists(&exists);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!exists) {
    // Local file does not exist so this is not a valid media item
    // retval is false so return
    return NS_OK;
  }

  // All checks passed this is a valid item
  *_retval = PR_TRUE;
  return NS_OK;
}

nsresult
sbMediaManagementService::GetManagedFolder(nsIURI ** aMusicFolderURI) {

  NS_ENSURE_ARG_POINTER(aMusicFolderURI);

  nsresult rv;

  static char const SB_MEDIA_MANAGEMENT_PREF[] =
    "songbird.media_management.library.folder";
  static char const SB_MEDIA_MANAGEMENT_ENABLE_PREF[] =
    "songbird.media_management.library.enabled";

  nsCOMPtr<nsIPrefBranch> folderPrefs =
    do_ProxiedGetService("@mozilla.org/preferences-service;1",
                         &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool enabled = PR_FALSE;
  // Ignore error, treat as enabled false
  folderPrefs->GetBoolPref(SB_MEDIA_MANAGEMENT_ENABLE_PREF, &enabled);

  if (!enabled) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsISupportsString> supportsString;

  // Failure is OK, supportsString will just be null
  rv =  folderPrefs->GetComplexValue(SB_MEDIA_MANAGEMENT_PREF,
                                     NS_GET_IID(nsISupportsString),
                                     getter_AddRefs(supportsString));
  if (NS_FAILED(rv) || !supportsString) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsILocalFile> folderPathFile;

  // Just check to be safe
  nsString folderPath;
  rv = supportsString->GetData(folderPath);
  NS_ENSURE_SUCCESS(rv, rv);
  folderPathFile = do_CreateInstance(NS_LOCAL_FILE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = folderPathFile->InitWithPath(folderPath);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = sbLibraryUtils::GetFileContentURI(folderPathFile,
                                         aMusicFolderURI);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
