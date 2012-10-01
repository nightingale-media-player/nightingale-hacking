/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
* \file sbFileMetadataService.cpp
* \brief Implementation for sbFileMetadataService.h
*/

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsIObserverService.h>
#include <nsArrayUtils.h>
#include <nsIPrefBranch.h>
#include <nsIPromptService.h>
#include <nsIWindowMediator.h>
#include <nsIDOMWindow.h>
#include <nsIDOMWindowInternal.h>
#include <nsThreadUtils.h>
#include <prlog.h>

#include <sbILibraryManager.h>
#include <sbIMediacoreSequencer.h>
#include <sbIMediacoreStatus.h>
#include <sbIMediaItem.h>
#include <sbStandardProperties.h>
#include <sbIDataRemote.h>

#include "sbFileMetadataService.h"
#include "sbMetadataJobItem.h"
#include "sbMainThreadMetadataProcessor.h"
#include "sbBackgroundThreadMetadataProcessor.h"
#include "sbMetadataCrashTracker.h"


// DEFINES ====================================================================
#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#define TRACE(args) PR_LOG(gMetadataLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMetadataLog, PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

// Controls how often we send sbIJobProgress notifications
#define TIMER_PERIOD  33

// GLOBALS ====================================================================

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS3( sbFileMetadataService, \
                               sbIFileMetadataService, \
                               sbPIFileMetadataService, \
                               nsIObserver)

sbFileMetadataService::sbFileMetadataService() : 
  mMainThreadProcessor(nsnull),
  mInitialized(PR_FALSE),
  mRunning(PR_FALSE),
  mNotificationTimer(nsnull),
  mNextJobIndex(0),
  mCrashTracker(nsnull)
{
  MOZ_COUNT_CTOR(sbFileMetadataService);
  TRACE(("%s[%.8x]", __FUNCTION__, this));
}

sbFileMetadataService::~sbFileMetadataService()
{
  MOZ_COUNT_DTOR(sbFileMetadataService);
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  
  if (mJobLock) {
    nsAutoLock::DestroyLock(mJobLock); 
  }
}

nsresult sbFileMetadataService::Init()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  nsresult rv;
  
  /////////////////////////////////////////////////////////
  // WARNING: Init may be called off of the main thread. //
  /////////////////////////////////////////////////////////
  
  mJobLock = nsAutoLock::NewLock(
      "sbFileMetadataService job items lock");
  NS_ENSURE_TRUE(mJobLock, NS_ERROR_OUT_OF_MEMORY);

  // Get the mediacore manager.
  mMediacoreManager = do_GetService(SB_MEDIACOREMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Listen for library shutdown.  The observer service must be used on the main
  // thread.
  nsCOMPtr<nsIObserverService> obsSvc;
  if (NS_IsMainThread()) {
    obsSvc = do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  }
  else {
    obsSvc = do_ProxiedGetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  }
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<nsIObserver> observer =
    do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = obsSvc->AddObserver(observer, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);
    
  mInitialized = PR_TRUE;
  return rv;
}


nsresult sbFileMetadataService::Shutdown()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ASSERTION(NS_IsMainThread(), 
    "\n\n\nsbFileMetadataService::Shutdown IS MAIN THREAD ONLY!!!!111\n\n\n");
  nsresult rv;
  
  if (mMainThreadProcessor) {
    rv = mMainThreadProcessor->Stop();
    NS_ASSERTION(NS_SUCCEEDED(rv), 
      "Failed to stop main thread metadata processor");
    mMainThreadProcessor = nsnull;
  }

  // Must not lock mJobLock before calling stop, as
  // the background thread may be in the middle of something
  // that will involve mJobLock
  if (mBackgroundThreadProcessor) {
    rv = mBackgroundThreadProcessor->Stop();
    NS_ASSERTION(NS_SUCCEEDED(rv), 
      "Failed to stop background thread metadata processor");
    mBackgroundThreadProcessor = nsnull;
  }

  nsAutoLock lock(mJobLock);

  if (mNotificationTimer) {
    rv = mNotificationTimer->Cancel();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to cancel a notification timer");
    mNotificationTimer = nsnull;
  }

  mRunning = PR_FALSE;
  mInitialized = PR_FALSE;

  // Cancel the jobs.  No risk of additional jobs as mJobArray is locked.
  for (PRInt32 i = mJobArray.Length() - 1; i >= 0; --i) {
    rv = mJobArray[i]->Cancel();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to cancel a metadata job");
    mJobArray.RemoveElementAt(i);
  }
  NS_ASSERTION(0 == mJobArray.Length(), 
    "Metadata jobs remaining after stopping the manager");
  
  // Update the legacy flags that show up in the status bar
  UpdateDataRemotes(mJobArray.Length());
  
  // Shutdown the crash tracker
  if (mCrashTracker) {
    rv = mCrashTracker->ResetLog();
    NS_ASSERTION(NS_SUCCEEDED(rv),
      "sbFileMetadataService::Shutdown failed to stop crash tracker");
    mCrashTracker = nsnull;
  }
  
  return NS_OK;
}

NS_IMETHODIMP 
sbFileMetadataService::Read(nsIArray* aMediaItemsArray,
                            sbIJobProgress** _retval)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  return ProxiedStartJob(aMediaItemsArray,
                         nsnull,
                         sbMetadataJob::TYPE_READ,
                         _retval);
}

NS_IMETHODIMP 
sbFileMetadataService::Write(nsIArray* aMediaItemsArray,
                             nsIStringEnumerator* aRequiredProperties,
                             sbIJobProgress** _retval)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  return ProxiedStartJob(aMediaItemsArray,
                         aRequiredProperties,
                         sbMetadataJob::TYPE_WRITE,
                         _retval);
}

NS_IMETHODIMP
sbFileMetadataService::RestartProcessors(PRUint16 aProcessorsToRestart)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  
  nsresult rv = ProxiedRestartProcessors(aProcessorsToRestart);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbFileMetadataService::ProxiedRestartProcessors(PRUint16 aProcessorsToRestart)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  nsresult rv = NS_OK;

  if (!NS_IsMainThread()) {
    LOG(("%s[%.8x] proxying main thread RestartProcessors()", __FUNCTION__, this));
    nsCOMPtr<nsIThread> target;
    rv = NS_GetMainThread(getter_AddRefs(target));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIFileMetadataService> proxy;
    rv = do_GetProxyForObject(target,
                              NS_GET_IID(sbIFileMetadataService),
                              static_cast<sbIFileMetadataService*>(this),
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(proxy));
    NS_ENSURE_SUCCESS(rv, rv);
    // Can't call ProxiedRestartProcessors via proxy, since it is not
    // an interface method.
    rv = proxy->RestartProcessors(aProcessorsToRestart);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    NS_ENSURE_STATE(mMainThreadProcessor);
    NS_ENSURE_STATE(mBackgroundThreadProcessor);

    if (aProcessorsToRestart & sbIFileMetadataService::MAIN_THREAD_PROCESSOR) {
      rv = mMainThreadProcessor->Start();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    if (aProcessorsToRestart & sbIFileMetadataService::BACKGROUND_THREAD_PROCESSOR) {
      nsCOMPtr<nsIRunnable> event =
        NS_NEW_RUNNABLE_METHOD(sbBackgroundThreadMetadataProcessor,
                               mBackgroundThreadProcessor.get(),
                               Start);
      NS_DispatchToCurrentThread(event);
    }
  }

  return NS_OK;
}

nsresult 
sbFileMetadataService::ProxiedStartJob(nsIArray* aMediaItemsArray,
                                       nsIStringEnumerator* aRequiredProperties,
                                       sbMetadataJob::JobType aJobType,
                                       sbIJobProgress** _retval)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  nsresult rv = NS_OK;  
  
  // Make sure StartJob is called on the main thread
  if (!NS_IsMainThread()) {
    LOG(("%s[%.8x] proxying main thread StartJob()", __FUNCTION__, this));
    nsCOMPtr<nsIThread> target;
    rv = NS_GetMainThread(getter_AddRefs(target));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIFileMetadataService> proxy;
    rv = do_GetProxyForObject(target,
                              NS_GET_IID(sbIFileMetadataService),
                              static_cast<sbIFileMetadataService*>(this),
                              NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                              getter_AddRefs(proxy));
    NS_ENSURE_SUCCESS(rv, rv);
    // Can't call StartJob via proxy, since it is not
    // an interface method.  
    if (aJobType == sbMetadataJob::TYPE_WRITE) {
      rv = proxy->Write(aMediaItemsArray, aRequiredProperties, _retval);
    } else {
      rv = proxy->Read(aMediaItemsArray, _retval);
    }    
  } else {
    rv = StartJob(aMediaItemsArray, aRequiredProperties, aJobType, _retval);
  }
  return rv;
}


nsresult 
sbFileMetadataService::StartJob(nsIArray* aMediaItemsArray,
                                nsIStringEnumerator* aRequiredProperties,
                                sbMetadataJob::JobType aJobType,
                                sbIJobProgress** _retval)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aMediaItemsArray);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
  
  NS_ASSERTION(NS_IsMainThread(), 
    "\n\n\nsbFileMetadataService::StartJob IS MAIN THREAD ONLY!!!!!!!\n\n\n");

  nsresult rv = NS_OK;

  // Do not allow write jobs unless the user has specifically given
  // permission.  
  if (aJobType == sbMetadataJob::TYPE_WRITE) {
    rv = EnsureWritePermitted();
    NS_ENSURE_SUCCESS(rv, rv);
  } 

  nsRefPtr<sbMetadataJob> job = new sbMetadataJob();
  NS_ENSURE_TRUE(job, NS_ERROR_OUT_OF_MEMORY);

  rv = job->Init(aMediaItemsArray, aRequiredProperties, aJobType);
  NS_ENSURE_SUCCESS(rv, rv);

  { 
    nsAutoLock lock(mJobLock);
    NS_ENSURE_TRUE(mInitialized, NS_ERROR_NOT_INITIALIZED);
    
    // If the last job in the job array is blocked, the new job will be too.
    PRUint32 jobCount = mJobArray.Length();
    if (jobCount > 0) {
      bool blocked;
      rv = mJobArray[jobCount-1]->GetBlocked(&blocked);
      NS_ENSURE_SUCCESS(rv, rv);
      if (blocked) {
        rv = job->SetBlocked(PR_TRUE);
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }

    mJobArray.AppendElement(job);

    // Update the legacy flags that show up in the status bar
    UpdateDataRemotes(mJobArray.Length());
  }

  if (!mRunning) {
    // Start a timer to send job progress notifications
    if (!mNotificationTimer) {
      mNotificationTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    rv = mNotificationTimer->Init(this, 
                                  TIMER_PERIOD,
                                  nsITimer::TYPE_REPEATING_SLACK);
    NS_ENSURE_SUCCESS(rv, rv);
    
    if (!mCrashTracker) {
      mCrashTracker = new sbMetadataCrashTracker();
      NS_ENSURE_TRUE(mCrashTracker, NS_ERROR_OUT_OF_MEMORY);
      rv = mCrashTracker->Init();
      if (NS_FAILED(rv)) {
        NS_ERROR("sbFileMetadataService::StartJob failed to "
                 "start crash tracker");
        rv = NS_OK;
      }
    }
    
    mRunning = PR_TRUE;
  }
  
  // TODO Not necessary, but could only start processors when needed.
  
  // Start up the main thread (timer driven) metadata processor
  if (!mMainThreadProcessor) {
    mMainThreadProcessor = new sbMainThreadMetadataProcessor(this);
  }
  NS_ENSURE_TRUE(mMainThreadProcessor, NS_ERROR_OUT_OF_MEMORY);  
  rv = mMainThreadProcessor->Start();
  NS_ENSURE_SUCCESS(rv, rv);

  // Start background thread metadata processor
  // (will continue if already started)
  if (!mBackgroundThreadProcessor) {
    mBackgroundThreadProcessor = new sbBackgroundThreadMetadataProcessor(this);
  }
  NS_ENSURE_TRUE(mBackgroundThreadProcessor, NS_ERROR_OUT_OF_MEMORY);  
  rv = mBackgroundThreadProcessor->Start();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = CallQueryInterface(job.get(), _retval);
  NS_ENSURE_SUCCESS(rv, rv);
  
  return NS_OK;
}


nsresult sbFileMetadataService::GetQueuedJobItem(bool aMainThreadOnly,
                                                sbMetadataJobItem** aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  nsresult rv = NS_OK;
  
  nsAutoLock lock(mJobLock);
   
  if (mJobArray.Length() > 0) {    
    nsRefPtr<sbMetadataJobItem> item;
    
    // Look through active jobs for a job item to be processed.
    // Skip any files that may have cause an sbIMetadataHandler
    // to crash in the past.
    bool isURLBlacklisted;
    do {
      isURLBlacklisted = PR_FALSE;
    
      // TODO consider giving preference to the most recent job
    
      // Loop through all active jobs looking for one that can supply a JobItem.
      // (Necessary because jobs may be out of items, but haven't yet 
      // been marked completed by the notification timer)
      for (PRUint32 i=0; i < mJobArray.Length(); i++) {
        if (mNextJobIndex >= mJobArray.Length()) {
          mNextJobIndex = 0;
        }
        rv = mJobArray[mNextJobIndex++]->GetQueuedItem(aMainThreadOnly, 
                                                       getter_AddRefs(item));
        if (rv != NS_ERROR_NOT_AVAILABLE) {
          break;
        }
      }

      // Use the crash tracker to avoid processing files
      // that have previously crashed an sbIMetadataHandler
      if (mCrashTracker && NS_SUCCEEDED(rv)) {
        nsCString url;
        rv = item->GetURL(url);
        NS_ENSURE_SUCCESS(rv, rv);
        
        // Check to see if we've crashed on this file before
        mCrashTracker->IsURLBlacklisted(url, &isURLBlacklisted);      
        if (isURLBlacklisted) {
          // Blacklisted.  Just skip this file and record it as failed.
          LOG(("sbFileMetadataService ignored blacklisted file %s.", 
               url.BeginReading()));
          PutProcessedJobItem(item);
        } else {        
          // Record that this item is being started
          TRACE(("sbFileMetadataService[9x%.8x] GetQueuedJobItem starting %s",
                 this, url.BeginReading()));
          rv = mCrashTracker->LogURLBegin(url);
          if (NS_FAILED(rv)) {
            NS_ERROR("sbFileMetadataService::GetQueuedJobItem couldn't log URL");
          }
        }
      }
    } while (isURLBlacklisted);
      
    // If we still haven't found an item, that's fine
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      TRACE(("sbFileMetadataService[0x%.8x] GetQueuedJobItem NOT_AVAILABLE",
             this));
      return rv;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    item.forget(aJobItem);    
    return NS_OK;
  }
  
  return NS_ERROR_NOT_AVAILABLE;
}


nsresult sbFileMetadataService::PutProcessedJobItem(sbMetadataJobItem* aJobItem)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ENSURE_ARG_POINTER(aJobItem);
  nsresult rv;
  
  // Forward results on to original job
  nsRefPtr<sbMetadataJob> job;
  rv = aJobItem->GetOwningJob(getter_AddRefs(job));
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Record that this item has been completed
  if (mCrashTracker) {
    nsCString url;
    rv = aJobItem->GetURL(url);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mCrashTracker->LogURLEnd(url);
    if (NS_FAILED(rv)) {
      NS_ERROR("sbFileMetadataService::PutProcessedJobItem couldn't log URL");
    }
  }

  return job->PutProcessedItem(aJobItem);
}

nsresult
sbFileMetadataService::GetJobItemIsBlocked(sbMetadataJobItem* aJobItem,
                                           bool*            aJobItemIsBlocked)
{
  NS_ENSURE_ARG_POINTER(aJobItem);
  NS_ENSURE_ARG_POINTER(aJobItemIsBlocked);

  nsresult rv;

  // Get the job type.
  sbMetadataJob::JobType jobType;
  rv = aJobItem->GetJobType(&jobType);
  NS_ENSURE_SUCCESS(rv, rv);

  // Only writing jobs can be blocked.
  if (jobType != sbMetadataJob::TYPE_WRITE) {
    *aJobItemIsBlocked = PR_FALSE;
    return NS_OK;
  }

  // Job is not blocked if the mediacore is not playing.
  nsCOMPtr<sbIMediacoreStatus> status;
  PRUint32                     state = 0;
  rv = mMediacoreManager->GetStatus(getter_AddRefs(status));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = status->GetState(&state);
  NS_ENSURE_SUCCESS(rv, rv);
  if (state != sbIMediacoreStatus::STATUS_PLAYING) {
    *aJobItemIsBlocked = PR_FALSE;
    return NS_OK;
  }

  // Get the currently playing media item.
  nsCOMPtr<sbIMediacoreSequencer> sequencer;
  nsCOMPtr<sbIMediaItem>          sequencerCurrentItem;
  rv = mMediacoreManager->GetSequencer(getter_AddRefs(sequencer));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = sequencer->GetCurrentItem(getter_AddRefs(sequencerCurrentItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the job media item.
  nsCOMPtr<sbIMediaItem> jobItem;
  rv = aJobItem->GetMediaItem(getter_AddRefs(jobItem));
  NS_ENSURE_SUCCESS(rv, rv);

  // Job is not blocked if the job media item is not currently being played.
  bool equals;
  rv = jobItem->Equals(sequencerCurrentItem, &equals);
  NS_ENSURE_SUCCESS(rv, rv);
  if (!equals) {
    *aJobItemIsBlocked = PR_FALSE;
    return NS_OK;
  }

  // Job is blocked.
  *aJobItemIsBlocked = PR_TRUE;

  return NS_OK;
}


// nsIObserver
NS_IMETHODIMP
sbFileMetadataService::Observe(nsISupports *aSubject, 
                              const char *aTopic,
                              const PRUnichar *aData)
{
  TRACE(("%s[%.8x] (%s)", __FUNCTION__, this, aTopic));
  nsresult rv;

  //
  // Library Down
  //
  if (!strcmp(SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC, aTopic)) {

    rv = Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);

    // Remove all the observer callbacks
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obsSvc->RemoveObserver(observer, SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC);
    NS_ENSURE_SUCCESS(rv, rv);
  
  //
  // Notification Timer
  //
  } else if (!strcmp(NS_TIMER_CALLBACK_TOPIC, aTopic)) {
    TRACE(("%s[%.8x] - Notification Timer Callback", __FUNCTION__, this));
        
    // Snapshot the job array so that we don't leave
    // things locked while we call onJobProgress and
    // potentially do a bunch of work
    nsTArray<nsRefPtr<sbMetadataJob> > jobs;
    {
      nsAutoLock lock(mJobLock);
      jobs.AppendElements(mJobArray);

      // Update blocked status of jobs.  If any job is blocked, all jobs after
      // it are also blocked.
      bool blocked = PR_FALSE;
      PRUint32 jobCount = jobs.Length();
      for (PRUint32 i=0; i < jobCount; i++) {
        // If no jobs are blocked yet, check current job.  Otherwise, mark
        // current job as blocked.
        if (!blocked)
          rv = jobs[i]->GetBlocked(&blocked);
        else
          rv = jobs[i]->SetBlocked(PR_TRUE);
      }
    }
    
    PRUint16 status;
    for (PRUint32 i=0; i < jobs.Length(); i++) {
      jobs[i]->OnJobProgress();
      // Ignore errors
    }

    // Now lock again and see if there are any active jobs left
    {
      bool allComplete = PR_TRUE;
      nsAutoLock lock(mJobLock);
      for (PRUint32 i=0; i < mJobArray.Length(); i++) {
        mJobArray[i]->GetStatus(&status);
        if (status == sbIJobProgress::STATUS_RUNNING) {
          allComplete = PR_FALSE;
        }
      }
      // If none of the jobs are active shut everything down
      // This is stupid-ish. Could just remove items one by one
      // as they complete.
      if (allComplete) {
        TRACE(("%s[%.8x] - Notification - All Complete", __FUNCTION__, this));
        rv = mNotificationTimer->Cancel();
        NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to cancel a notification timer");
        mRunning = PR_FALSE;
      
        // TODO Not necessary, but we could shut down the scanners here
      
        mJobArray.Clear();

        // Update the legacy flags that show up in the status bar
        UpdateDataRemotes(mJobArray.Length());
        
        // The crash tracker can forget about everything we've just done
        if (mCrashTracker) {
          rv = mCrashTracker->ResetLog();
          if (NS_FAILED(rv)) {
            NS_ERROR("sbFileMetadataService::Shutdown failed to stop crash tracker");
          }
        }
      }
    }
  }
  return NS_OK;
}

/**
 * Return NS_OK if write jobs are permitted, or 
 * NS_ERROR_NOT_AVAILABLE if not.
 *
 * May prompt the user to permit write jobs.
 */
nsresult sbFileMetadataService::EnsureWritePermitted()
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  nsresult rv;

  bool enableWriting = PR_FALSE;
  nsCOMPtr<nsIPrefBranch> prefService =
  do_GetService( "@mozilla.org/preferences-service;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv);
  prefService->GetBoolPref( "songbird.metadata.enableWriting", &enableWriting );

  if (!enableWriting) {    
    
    // Let the user know what the situation is. 
    // Allow them to enable writing if desired.
    
    bool promptOnWrite = PR_TRUE;
    prefService->GetBoolPref( "songbird.metadata.promptOnWrite", &promptOnWrite );
    
    if (promptOnWrite) {
      // Don't bother to prompt unless there is a player window open.
      // This avoids popping a modal dialog mid unit test.
      nsCOMPtr<nsIWindowMediator> windowMediator =
        do_GetService("@mozilla.org/appshell/window-mediator;1", &rv);
      NS_ENSURE_SUCCESS( rv, rv);
      nsCOMPtr<nsIDOMWindowInternal> mainWindow;  
      windowMediator->GetMostRecentWindow(nsnull,
                                          getter_AddRefs(mainWindow));
      if (mainWindow) {
        nsCOMPtr<nsIPromptService> promptService =
          do_GetService("@mozilla.org/embedcomp/prompt-service;1", &rv);
        NS_ENSURE_SUCCESS( rv, rv);

        bool promptResult = PR_FALSE;
        bool checkState = PR_FALSE;

        // TODO Clean up, localize, or remove from the product
        rv = promptService->ConfirmCheck(mainWindow,                  
              NS_LITERAL_STRING("WARNING! TAG WRITING IS EXPERIMENTAL!").get(),
              NS_MULTILINE_LITERAL_STRING( 
                NS_LL("Are you sure you want to write metadata changes")
                NS_LL(" back to your media files?\n\nTag writing has not been tested yet,")
                NS_LL(" and may damage your media files.  If you'd like to help us test")
                NS_LL(" this functionality, great, but we advise you to back up your media first.")
              ).get(),
              NS_LITERAL_STRING("Don't show this dialog again").get(),
              &checkState, 
              &promptResult);  
        NS_ENSURE_SUCCESS( rv, rv);
        
        if (checkState) {
          prefService->SetBoolPref( "songbird.metadata.promptOnWrite", PR_FALSE );
        }
        
        if (promptResult) {
          prefService->SetBoolPref( "songbird.metadata.enableWriting", PR_TRUE );
          enableWriting = PR_TRUE;
        }
      }
    }
  }

  return (enableWriting) ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

nsresult sbFileMetadataService::UpdateDataRemotes(PRInt64 aJobCount)
{
  TRACE(("%s[%.8x]", __FUNCTION__, this));
  NS_ASSERTION(NS_IsMainThread(), 
    "sbFileMetadataService::UpdateDataRemotes is main thread only!");
  nsresult rv = NS_OK;

  // Get the legacy dataremote used for signaling metadata state
  if (!mDataCurrentMetadataJobs) {
    mDataCurrentMetadataJobs =
      do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mDataCurrentMetadataJobs->Init(NS_LITERAL_STRING("backscan.concurrent"),
                                        EmptyString());
    NS_ENSURE_SUCCESS(rv, rv);  
  }

  return mDataCurrentMetadataJobs->SetIntValue(aJobCount);;
}

/* void sbPIFileMetadataService::AddBlacklistURL (in ACString aURL); */
NS_IMETHODIMP
sbFileMetadataService::AddBlacklistURL(const nsACString & aURL)
{
  LOG(("%s[%.8x] Adding blacklist url \"%s\"",
       __FUNCTION__,
       this,
       aURL.BeginReading()));
  nsresult rv;
  if (!mCrashTracker) {
    // probably was running the unit test by itself
    mCrashTracker = new sbMetadataCrashTracker();
    NS_ENSURE_TRUE(mCrashTracker, NS_ERROR_OUT_OF_MEMORY);
    rv = mCrashTracker->Init();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = mCrashTracker->AddBlacklistURL(aURL);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

