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

/**
 * \file sbBackgroundThreadMetadataProcessor.cpp
 * \brief Implementation for sbBackgroundThreadMetadataProcessor.h
 */

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsThreadUtils.h>
#include <nsComponentManagerUtils.h>

#include "sbBackgroundThreadMetadataProcessor.h"
#include "sbFileMetadataService.h"
#include "sbMetadataJobItem.h"

#include "prlog.h"

// DEFINES ====================================================================

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#define TRACE(args) PR_LOG(gMetadataLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMetadataLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

// GLOBALS ====================================================================
// CLASSES ====================================================================

NS_IMPL_THREADSAFE_ISUPPORTS1(sbBackgroundThreadMetadataProcessor, nsIRunnable);

sbBackgroundThreadMetadataProcessor::sbBackgroundThreadMetadataProcessor(
  sbFileMetadataService* aManager) :
    mJobManager(aManager),
    mThread(nsnull),
    mShouldShutdown(PR_FALSE),
    mMonitor(nsnull)
{
  MOZ_COUNT_CTOR(sbBackgroundThreadMetadataProcessor);
  TRACE(("sbBackgroundThreadMetadataProcessor[0x%.8x] - ctor", this));
  NS_ASSERTION(NS_IsMainThread(), 
    "sbBackgroundThreadMetadataProcessor called off the main thread");
}

sbBackgroundThreadMetadataProcessor::~sbBackgroundThreadMetadataProcessor()
{
  MOZ_COUNT_DTOR(sbBackgroundThreadMetadataProcessor);
  TRACE(("sbBackgroundThreadMetadataProcessor[0x%.8x] - dtor", this));
  Stop();
  mThread = nsnull;
  mJobManager = nsnull;
  if (mMonitor) {
    nsAutoMonitor::DestroyMonitor(mMonitor);
  }
}

nsresult sbBackgroundThreadMetadataProcessor::Start()
{
  NS_ENSURE_STATE(mJobManager);
  NS_ASSERTION(NS_IsMainThread(), 
    "sbBackgroundThreadMetadataProcessor called off the main thread!");
  TRACE(("sbBackgroundThreadMetadataProcessor[0x%.8x] - Start", this));
  nsresult rv;
  
  if (!mMonitor) {
    mMonitor = nsAutoMonitor::NewMonitor(
        "sbBackgroundThreadMetadataProcessor::mMonitor");
    NS_ENSURE_TRUE(mMonitor, NS_ERROR_OUT_OF_MEMORY);
  }

  nsAutoMonitor monitor(mMonitor);

  if (!mThread) {
    mShouldShutdown = PR_FALSE;
    rv = NS_NewThread(getter_AddRefs(mThread), this);
    NS_ENSURE_SUCCESS(rv, rv);    
  } 

  rv = monitor.Notify();
  NS_ASSERTION(NS_SUCCEEDED(rv), 
    "sbBackgroundThreadMetadataProcessor::Start monitor notify failed");
  
  return NS_OK;
}

nsresult sbBackgroundThreadMetadataProcessor::Stop() 
{
  NS_ASSERTION(NS_IsMainThread(), 
    "sbBackgroundThreadMetadataProcessor called off the main thread");
  TRACE(("sbBackgroundThreadMetadataProcessor[0x%.8x] - Stop", this));
  nsresult rv;
  
  {
    nsAutoMonitor monitor(mMonitor);
    // Tell the thread to stop working
    mShouldShutdown = PR_TRUE;

    rv = monitor.Notify();
    NS_ASSERTION(NS_SUCCEEDED(rv), 
      "sbBackgroundThreadMetadataProcessor::Start monitor notify failed");
  }

  // Wait for the thread to shutdown and release all resources.
  if (mThread) {
    mThread->Shutdown();
    mThread = nsnull;
  }
  
  return NS_OK;
}


/**
 * nsIRunnable implementation.  Called by mThread.
 */
NS_IMETHODIMP sbBackgroundThreadMetadataProcessor::Run()
{
  TRACE(("sbBackgroundThreadMetadataProcessor[0x%.8x] - Thread Starting", 
        this));
  nsresult rv;
  
  while (!mShouldShutdown) {
    
    // Get the next background job item
    nsRefPtr<sbMetadataJobItem> item;
    
    // Lock to make sure we dont go to sleep right after
    // a Start() call
    {
      nsAutoMonitor monitor(mMonitor);
      
      rv = mJobManager->GetQueuedJobItem(PR_FALSE, getter_AddRefs(item));
      
      // On error, skip the item and try again
      if (NS_FAILED(rv)) {
        // If there are no more job items available just go to sleep.
        if (rv == NS_ERROR_NOT_AVAILABLE) {
          TRACE(("sbBackgroundThreadMetadataProcessor[0x%.8x] - Thread waiting", 
                this));
          rv = monitor.Wait();
          NS_ASSERTION(NS_SUCCEEDED(rv), 
            "sbBackgroundThreadMetadataProcessor::Run monitor wait failed");

        } else {
          NS_ERROR("sbBackgroundThreadMetadataProcessor::Run encountered "
                   " an error while getting a job item.");          
        }
              
        continue;
      } 
    }
    NS_ENSURE_SUCCESS(rv, rv);

    // Get the job owning the job item.
    nsRefPtr<sbMetadataJob> job;
    rv = item->GetOwningJob(getter_AddRefs(job));
    NS_ENSURE_SUCCESS(rv, rv);
    
    // Start the metadata handler for this job item.
    nsCOMPtr<sbIMetadataHandler> handler;
    rv = item->GetHandler(getter_AddRefs(handler));
    // On error just wait, rather than killing the thread
    if (!NS_SUCCEEDED(rv)) {
      NS_ERROR("sbBackgroundThreadMetadataProcessor::Run unable "
               " to get an sbIMetadataHandler.");
      continue; 
    }
    
    sbMetadataJob::JobType jobType;
    rv = item->GetJobType(&jobType);
    if (!NS_SUCCEEDED(rv)) {
      NS_ERROR("sbBackgroundThreadMetadataProcessor::Run unable "
               " to determine job type.");
      continue; 
    }
  
    // If the job item is blocked, wait until it is no longer blocked.
    PRBool skipItem = PR_FALSE;
    while (1) {
      // Check if job item is blocked.
      PRBool jobItemIsBlocked;
      rv = mJobManager->GetJobItemIsBlocked(item, &jobItemIsBlocked);
      if (NS_FAILED(rv)) {
          NS_ERROR("sbBackgroundThreadMetadataProcessor::Run unable "
                   " to determine if job item is blocked.");
          skipItem = PR_TRUE;
          break;
      }

      // Stop waiting if job item is not blocked.
      if (!jobItemIsBlocked)
        break;

      // Set the job as blocked.
      rv = job->SetBlocked(PR_TRUE);
      if (NS_FAILED(rv)) {
          NS_ERROR("sbBackgroundThreadMetadataProcessor::Run unable "
                   " to set job blocked.");
          skipItem = PR_TRUE;
          break;
      }

      // Wait a bit and check again.
      PR_Sleep(PR_MillisecondsToInterval(20));
    }
    if (skipItem)
      continue;

    // Set the job as not blocked.
    rv = job->SetBlocked(PR_FALSE);
    if (NS_FAILED(rv)) {
        NS_ERROR("sbBackgroundThreadMetadataProcessor::Run unable "
                 " to set job not blocked.");
        continue;
    }

    PRBool async = PR_FALSE; 
    if (jobType == sbMetadataJob::TYPE_WRITE) {
      rv = handler->Write(&async);
    } else {
      rv = handler->Read(&async);
    }

    // If we were able to start the handler, 
    // make sure it completes.
    if (NS_SUCCEEDED(rv)) {
      rv = item->SetProcessingStarted(PR_TRUE);
      if (NS_FAILED(rv)) {
        NS_ERROR("sbBackgroundThreadMetadataProcessor::Run unable "
                 " to set item processing started.");
      }

      PRBool handlerCompleted = PR_FALSE;
      rv = handler->GetCompleted(&handlerCompleted);
      if (!NS_SUCCEEDED(rv)) {
        NS_ERROR("sbBackgroundThreadMetadataProcessor::Run unable "
                 " to determine check handler completed.");
      }

      // All handler implementations should complete
      // syncronously at the moment when running
      // on the background thread.  
      
      // However, the stupid sbIHandlerInterface
      // allows async operations, so we should make
      // sure it is possible. 
      if (async && !handlerCompleted) {
        NS_WARNING("Attempting to run an async sbIMetadataHandler on " \
                   "a background thread! This may not work correctly!");
        int counter = 0;
        // Spend max 500ms pumping the event queue and waiting
        // for the handler to complete. If it doesn't finish,
        // give up.   This code was ported from the original
        // MetadataJob implementation.
        for (handler->GetCompleted(&handlerCompleted);
             !handlerCompleted && !(mShouldShutdown) && counter < 25;
             handler->GetCompleted(&handlerCompleted), counter++) {

          // Run at most 10 messages.
          PRBool event = PR_FALSE;
          int eventCount = 0;
          for (mThread->ProcessNextEvent(PR_FALSE, &event);
               event && eventCount < 10;
               mThread->ProcessNextEvent(PR_FALSE, &event), eventCount++) {
            PR_Sleep(PR_MillisecondsToInterval(0));
          }
          // Sleep at least 20ms
          PR_Sleep(PR_MillisecondsToInterval(20));
        }        
      }
            
      // Indicate that we processed this item
      item->SetProcessed(PR_TRUE);

      TRACE(("sbBackgroundThreadMetadataProcessor - item processed"));
    }
    
    // And finally give back the item
    mJobManager->PutProcessedJobItem(item);
  }
  
  TRACE(("sbBackgroundThreadMetadataProcessor[0x%.8x] - Thread Finished", this));
  return NS_OK;
}

