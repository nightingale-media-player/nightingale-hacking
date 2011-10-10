/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \file sbMainThreadMetadataProcessor.cpp
 * \brief Implementation for sbMainThreadMetadataProcessor.h
 */

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsThreadUtils.h>
#include <nsComponentManagerUtils.h>

#include "sbMainThreadMetadataProcessor.h"
#include "sbFileMetadataService.h"
#include "sbMetadataJobItem.h"

#include "prlog.h"

// DEFINES ====================================================================

// TODO tweak me
#define TIMER_PERIOD            33
#define NUM_ACTIVE_HANDLERS     15

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

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMainThreadMetadataProcessor, nsITimerCallback);

sbMainThreadMetadataProcessor::sbMainThreadMetadataProcessor(
  sbFileMetadataService* aManager) :
    mJobManager(aManager),
    mTimer(nsnull),
    mRunning(PR_FALSE)
{
  MOZ_COUNT_CTOR(sbMainThreadMetadataProcessor);
  TRACE(("sbMainThreadMetadataProcessor[0x%.8x] - ctor", this));
  NS_ASSERTION(NS_IsMainThread(), "sbMainThreadMetadataProcessor called off the main thread");
}

sbMainThreadMetadataProcessor::~sbMainThreadMetadataProcessor()
{
  MOZ_COUNT_DTOR(sbMainThreadMetadataProcessor);
  TRACE(("sbMainThreadMetadataProcessor[0x%.8x] - dtor", this));
  Stop();
  mTimer = nsnull;
  mJobManager= nsnull;
}

nsresult sbMainThreadMetadataProcessor::Start()
{
  NS_ENSURE_STATE(mJobManager);
  NS_ASSERTION(NS_IsMainThread(), "sbMainThreadMetadataProcessor called off the main thread!");
  TRACE(("sbMainThreadMetadataProcessor[0x%.8x] - Start", this));
  nsresult rv;
  
  if (!mTimer) {
    mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    mCurrentJobItems.SetLength(NUM_ACTIVE_HANDLERS);
  } 
  
  if (!mRunning) {  
    rv = mTimer->InitWithCallback(this, 
                                  TIMER_PERIOD,
                                  nsITimer::TYPE_REPEATING_SLACK);
    NS_ENSURE_SUCCESS(rv, rv);
  
    mRunning = PR_TRUE;
    
    // Run the timer method right away so we don't waste any time
    // before starting the handlers (would otherwise wait
    // TIMER_PERIOD before even starting)
    Notify(nsnull);
  }
  
  return NS_OK;
}

nsresult sbMainThreadMetadataProcessor::Stop() 
{
  NS_ASSERTION(NS_IsMainThread(), "sbMainThreadMetadataProcessor called off the main thread");
  TRACE(("sbMainThreadMetadataProcessor[0x%.8x] - Stop", this));
  nsresult rv;
  
  if (mTimer) {
    mTimer->Cancel();
  }
  
  mRunning = PR_FALSE;
  
  // Kill any active handlers
  for (PRUint32 i=0; i < NUM_ACTIVE_HANDLERS; i++) {
    if (mCurrentJobItems[i] != nsnull) {
      nsRefPtr<sbMetadataJobItem> item = mCurrentJobItems[i];
      
      nsCOMPtr<sbIMetadataHandler> handler;
      rv = item->GetHandler(getter_AddRefs(handler));
      NS_ENSURE_SUCCESS(rv, rv);
      
      mCurrentJobItems[i] = nsnull;
      handler->Close();
      
      // Give back the processed job item
      mJobManager->PutProcessedJobItem(item);
    }
  }

  return NS_OK;
}


/**
 * nsITimerCallback Implementation
 */
NS_IMETHODIMP sbMainThreadMetadataProcessor::Notify(nsITimer* aTimer)
{
  TRACE(("sbMainThreadMetadataProcessor[0x%.8x] - Notify", this));
  nsresult rv;
  
  PRBool finished = PR_TRUE;
  
  // Check on the state of our active job items.
  // Handle completed items and replace with new items.
  for (PRUint32 i=0; i < NUM_ACTIVE_HANDLERS; i++) {
    nsRefPtr<sbMetadataJobItem> item = mCurrentJobItems[i];
    
    // If this is an active item, check to see if it has completed
    // URGH, sbIMetadataHandler needs a callback!
    if (item) {
      // Check to see if it has completed
      nsCOMPtr<sbIMetadataHandler> handler;
      rv = item->GetHandler(getter_AddRefs(handler));
      NS_ENSURE_SUCCESS(rv, rv);
      PRBool handlerCompleted;
      rv = handler->GetCompleted(&handlerCompleted);
      NS_ENSURE_SUCCESS(rv, rv);
      
      if (handlerCompleted) {
        // NULL the array entry
        mCurrentJobItems[i] = nsnull;
        
        // Indicate that we processed this item
        item->SetProcessed(PR_TRUE);

        // Give back the processed job item
        mJobManager->PutProcessedJobItem(item);
        item = nsnull;
      } else {
        // Still processing.  We aren't ready to shut the timer down.
        finished = PR_FALSE;
      }
    }

    // If this is an empty slot, get a new job item
    if (!item) {
      // Get the next job item
      rv = mJobManager->GetQueuedJobItem(PR_TRUE, getter_AddRefs(item));
      // If there are no more job items available, we may need to shut down soon
      if (rv == NS_ERROR_NOT_AVAILABLE) {
        continue;
      }
      NS_ENSURE_SUCCESS(rv, rv);

      // And stuff it in our working array to check next loop
      mCurrentJobItems[i] = item;

      // Still processing.  We aren't ready to shut the timer down.
      finished = PR_FALSE;
    }

    // If item processing hasn't started, start a new handler
    if (item) {
      PRBool processingStarted;
      rv = item->GetProcessingStarted(&processingStarted);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!processingStarted) {
        // Get the job owning the job item.
        nsRefPtr<sbMetadataJob> job;
        rv = item->GetOwningJob(getter_AddRefs(job));
        NS_ENSURE_SUCCESS(rv, rv);

        // Don't start processing until the job item is not blocked.
        PRBool jobItemIsBlocked;
        rv = mJobManager->GetJobItemIsBlocked(item, &jobItemIsBlocked);
        NS_ENSURE_SUCCESS(rv, rv);
        if (jobItemIsBlocked) {
          // Set the job as blocked.
          rv = job->SetBlocked(PR_TRUE);
          NS_ENSURE_SUCCESS(rv, rv);

          // Still processing.  We aren't ready to shut the timer down.
          finished = PR_FALSE;
          continue;
        }

        // Set the job as not blocked.
        rv = job->SetBlocked(PR_FALSE);
        NS_ENSURE_SUCCESS(rv, rv);

        // Start the metadata handler for this job item.

        nsCOMPtr<sbIMetadataHandler> handler;
        rv = item->GetHandler(getter_AddRefs(handler));
        NS_ENSURE_SUCCESS(rv, rv);

        sbMetadataJob::JobType jobType;
        rv = item->GetJobType(&jobType);
        NS_ENSURE_SUCCESS(rv, rv);

        // We don't care if it is async... we're polling no matter what.
        PRBool async = PR_FALSE;
        if (jobType == sbMetadataJob::TYPE_WRITE) {
          rv = handler->Write(&async);
        }
        else {
          rv = handler->Read(&async);
        }

        // If failed, give back to the job
        if (NS_FAILED(rv)) {
          // must push back immediately
          mJobManager->PutProcessedJobItem(item);
          mCurrentJobItems[i] = nsnull;
        }
        else {
          // Still processing.  We aren't ready to shut the timer down.
          rv = item->SetProcessingStarted(PR_TRUE);
          NS_ENSURE_SUCCESS(rv, rv);
          finished = PR_FALSE;
        }
      }
    }
  } // end mCurrentJobItems for loop

  // If there are not more active job items, then we can 
  // safely shut down
  if (finished) {
    Stop();
  }
  
  return NS_OK;
}

