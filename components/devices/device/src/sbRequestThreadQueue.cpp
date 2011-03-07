/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
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

#include "sbRequestThreadQueue.h"

// Platform includes
#include <algorithm>

// Mozilla includes
#include <nsArrayUtils.h>
#include <nsAutoLock.h>
#include <nsComponentManagerUtils.h>
#include <nsIProgrammingLanguage.h>
#include <nsISupportsPrimitives.h>
#include <nsThreadUtils.h>

// Mozilla interfaces
#include <nsIMutableArray.h>

// Songbird includes
#include <sbDebugUtils.h>
#include <sbThreadUtils.h>

// Local includes
#include "sbRequestItem.h"

/**
 * This class provides an nsIRunnable interface that may be used to dispatch
 * and process device request added events.
 */

class sbRTQAddedEvent : public nsIRunnable
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE


  //
  // Public methods.
  //

  static nsresult New(sbRequestThreadQueue * aRTQ,
                      nsIRunnable** aEvent);

  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------
protected:
  // Make default constructor protected to ensure derivation
  sbRTQAddedEvent() : mRTQ(nsnull) {}
  /**
   * Initializes our Request Thread Processor pointer
   */
  nsresult Initialize(sbRequestThreadQueue * aRTQ);
private:
  /**
   * Non-owning reference to our Request Thread Processor. We can't
   * outlive our processor.
   */
  sbRequestThreadQueue * mRTQ;
};

sbRequestThreadQueue::Batch::Batch() :
                        mCountableItems(0),
                        mRequestType(sbRequestThreadQueue::REQUEST_TYPE_NOT_SET)
{
}

sbRequestThreadQueue::Batch::~Batch()
{
  clear();
}

void sbRequestThreadQueue::Batch::push_back(sbRequestItem * aItem)
{
  NS_ASSERTION(aItem, "sbRequestThreadQueue::Batch::push_back passed null");
  // If we don't have any request type then set it. If we have set a request
  // type but it's not a user defined one then set it to the defined one.
  // Ideally user defined and reserved requests shouldn't be mixed, but we'll
  // handle it nicely here anyway.
  if (aItem->GetIsCountable()) {
    if (mRequestType <= USER_REQUEST_TYPES) {
      mRequestType = aItem->GetType();
    }
    aItem->SetBatchIndex(mCountableItems++);
  }
  else {
    if (mRequestType == REQUEST_TYPE_NOT_SET) {
      mRequestType = aItem->GetType();
    }
  }
  NS_ADDREF(aItem);
  mRequestItems.push_back(aItem);
}

void sbRequestThreadQueue::Batch::erase(iterator aIter)
{
  // If this is a countable item we'll need to update the indexes and
  // mCountableItems
  if ((*aIter)->GetIsCountable()) {
    PRUint32 index = (*aIter)->GetBatchIndex();
    const const_iterator end = mRequestItems.end();
    iterator iter = aIter;
    // Increment past the item to be removed
    while (++iter != end) {
      if ((*iter)->GetIsCountable()) {
        (*iter)->SetBatchIndex(index++);
      }
    }
    --mCountableItems;
  }

  // Release the request item and remove it from the queue
  NS_RELEASE(*aIter);
  mRequestItems.erase(aIter);

  // Reset the request type if there are no more countable items
  if (mCountableItems == 0) {
    if (mRequestItems.empty()) {
      mRequestType = sbRequestThreadQueue::REQUEST_TYPE_NOT_SET;
    }
    else {
      mRequestType = (*mRequestItems.begin())->GetType();
    }
  }
}

inline void ReleaseRequestItem(sbRequestItem * aItem)
{
  NS_RELEASE(aItem);
}

void sbRequestThreadQueue::Batch::clear()
{
  // Release all the request items
  std::for_each(mRequestItems.begin(), mRequestItems.end(), ReleaseRequestItem);
  mRequestItems.clear();
  mRequestType = sbRequestThreadQueue::REQUEST_TYPE_NOT_SET;
}

void sbRequestThreadQueue::Batch::RecalcBatchStats()
{
  PRUint32 index = 0;
  const RequestItems::const_iterator end = mRequestItems.end();
  for (RequestItems::const_iterator iter = mRequestItems.begin();
       iter != end;
       ++iter)
  {
    if ((*iter)->GetIsCountable()) {
      (*iter)->SetBatchIndex(index++);
    }
  }
}

NS_IMPL_THREADSAFE_ADDREF(sbRequestThreadQueue);
NS_IMPL_THREADSAFE_RELEASE(sbRequestThreadQueue);

/**
 * Initialize the various data members. We initialize mCurrentBatchId to 1
 * as requests not belonging to a batch have a batch ID of 0.
 */
sbRequestThreadQueue::sbRequestThreadQueue() :
  mLock(nsnull),
  mBatchDepth(0),
  mStopWaitMonitor(nsnull),
  mThreadStarted(false),
  mStopProcessing(false),
  mAbortRequests(false),
  mIsHandlingRequests(false),
  mCurrentBatchId(1)
{
  SB_PRLOG_SETUP(sbRequestThreadQueue);

  mLock = nsAutoLock::NewLock("sbRequestThreadQueue::mLock");
  // Create the request wait monitor.
  mStopWaitMonitor =
    nsAutoMonitor::NewMonitor("sbRequestThreadQueue::mStopWaitMonitor");
}

sbRequestThreadQueue::~sbRequestThreadQueue()
{
  NS_ASSERTION(mRequestQueue.size() == 0,
               "sbRequestThreadQueue destructor with items in queue");
  if (mStopWaitMonitor) {
    nsAutoMonitor::DestroyMonitor(mStopWaitMonitor);
  }
  if (mLock) {
    nsAutoLock::DestroyLock(mLock);
  }
}

nsresult sbRequestThreadQueue::BatchBegin()
{

  NS_ENSURE_STATE(mLock);
  nsAutoLock lock(mLock);

  ++mBatchDepth;

  return NS_OK;
}

nsresult sbRequestThreadQueue::BatchEnd()
{
  NS_ENSURE_STATE(mLock);
  nsAutoLock lock(mLock);
  NS_ASSERTION(mBatchDepth > 0,
               "sbRequestThreadQueue batch depth out of balance");
  if (mBatchDepth > 0 && --mBatchDepth == 0) {
    ++mCurrentBatchId;
    ProcessRequest();
  }
  return NS_OK;
}

/**
 * Creates a runnable object for a C++ method.
 */
static nsresult
sbRunnableMethod(sbRequestThreadQueue & aObject,
                 nsresult (sbRequestThreadQueue::* aMethod)(int),
                 nsresult aFailureReturnValue,
                 int aArg1,
                 nsIRunnable ** aRunnable)
{
  NS_ENSURE_ARG_POINTER(aRunnable);

  nsresult rv;

  typedef sbRunnableMethod1<sbRequestThreadQueue, nsresult, int> Runnable;
  Runnable * runnable = nsnull;

  // New addrefs runnable when returned
  rv = Runnable::New(&runnable,
                     &aObject,
                     aMethod,
                     aFailureReturnValue,
                     aArg1);
  NS_ENSURE_SUCCESS(rv, rv);

  *aRunnable = runnable;

  return NS_OK;
}

nsresult sbRequestThreadQueue::Start()
{
  // Ensure we've allocated our lock and monitor
  NS_ENSURE_TRUE(mLock, NS_ERROR_OUT_OF_MEMORY);
  NS_ENSURE_TRUE(mStopWaitMonitor, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv;

  // No need to lock since Start should not be called while the request thread
  // is running or from multiple threads.
  // Start should not be called more than once before Stop is called.
  NS_ENSURE_FALSE(mThreadStarted, NS_ERROR_FAILURE);

  // Clear the stop request processing flag. No need to lock since this is
  // the only live thread with access to the object.
  mStopProcessing = false;

  // Create the request added event object.
  rv = sbRTQAddedEvent::New(this, getter_AddRefs(mReqAddedEvent));
  NS_ENSURE_SUCCESS(rv, rv);


  rv = sbRunnableMethod(*this,
                        &sbRequestThreadQueue::ThreadShutdownAction,
                        NS_ERROR_FAILURE,
                        0,
                        getter_AddRefs(mShutdownAction));
  NS_ENSURE_SUCCESS(rv, rv);

  mThreadStarted = true;

  // Create the request processing thread.
  rv = NS_NewThread(getter_AddRefs(mThread), nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = PushRequest(sbRequestItem::New(REQUEST_THREAD_START));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbRequestThreadQueue::Stop()
{
  NS_ENSURE_STATE(mLock);
  nsresult rv;

  // Check and set the thread state
  {
    nsAutoLock lock(mLock);

    // If stop was called with no Start or after another stop return an error
    if (!mThreadStarted) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    mThreadStarted = false;

  }

  // Notify external users that we're stopping
  {
    nsAutoMonitor monitor(mStopWaitMonitor);
    // Set the stop processing requests flag.
    mStopProcessing = true;
    monitor.NotifyAll();
  }

  // Push the thread stop request onto the queue. This will signal the request
  // thread to shutdown
  rv = PushRequestInternal(sbRequestItem::New(REQUEST_THREAD_STOP));
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to send thread stop message");

  // Process the request
  rv = ProcessRequest();
  NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to process stop message request");

  return NS_OK;
}

nsresult sbRequestThreadQueue::ThreadShutdownAction(int)
{
  NS_ENSURE_TRUE(NS_IsMainThread(), NS_ERROR_FAILURE);

  OnThreadStop();

  mThread->Shutdown();

  return NS_OK;
}

nsresult sbRequestThreadQueue::ProcessRequest()
{
  // Dispatch processing of the request added event.
  nsresult rv = mThread->Dispatch(mReqAddedEvent, NS_DISPATCH_NORMAL);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbRequestThreadQueue::FindDuplicateRequest(sbRequestItem * aItem,
                                                    bool & aIsDuplicate)
{
  NS_ENSURE_ARG_POINTER(aItem);

  nsresult rv;

  aIsDuplicate = false;
  const RequestQueue::const_reverse_iterator rend = mRequestQueue.rend();
  for (RequestQueue::const_reverse_iterator riter = mRequestQueue.rbegin();
       riter != rend && !aIsDuplicate;
       ++riter) {

    sbRequestItem * request = *riter;
    // We only need to check within the current batch. Duplicate checking is
    // an optimization and we're trying to consolidate multiple requests that
    // come in quickly as a result of a single operation.
    const PRInt32 queueItemBatchId = request->GetBatchId();

    if (queueItemBatchId != mCurrentBatchId) {
      break;
    }
    bool continueChecking = false;
    rv = IsDuplicateRequest(request, aItem, aIsDuplicate, continueChecking);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!continueChecking) {
      break;
    }
  }

  return NS_OK;
}

nsresult sbRequestThreadQueue::PushRequestInternal(sbRequestItem * aRequestItem)
{
  NS_ENSURE_ARG_POINTER(aRequestItem);
  nsresult rv;

  bool isDupe;
  rv = FindDuplicateRequest(aRequestItem, isDupe);
  NS_ENSURE_SUCCESS(rv, rv);
  if (isDupe) {
    return NS_OK;
  }

  NS_ADDREF(aRequestItem);
  mRequestQueue.push_back(aRequestItem);

  return NS_OK;
}

nsresult sbRequestThreadQueue::PushRequest(sbRequestItem * aRequestItem)
{
  NS_ENSURE_ARG_POINTER(aRequestItem);

  NS_ENSURE_STATE(mLock);

  nsresult rv;

  { /* scope for request lock */
    nsAutoLock lock(mLock);

    nsAutoMonitor monitor(mStopWaitMonitor);
    // If we're aborting or shutting down don't accept any more requests
    if (mAbortRequests || mStopProcessing)
    {
      return NS_ERROR_ABORT;
    }

    rv = PushRequestInternal(aRequestItem);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(mBatchDepth >= 0,
               "Batch depth out of whack in sbBaseDevice::PushRequest");
  // Only process requests if we're not in a batch
  if (mBatchDepth == 0) {
    rv = ProcessRequest();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult sbRequestThreadQueue::PopBatch(Batch & aBatch)
{
  NS_ENSURE_STATE(mLock);

  nsAutoLock lock(mLock);

  aBatch.clear();

  // If nothing was found then just return with an empty batch
  if (mRequestQueue.empty()) {
    LOG("No requests found\n");
    return NS_OK;
  }

  // If we're in the middle of a batch just return an empty batch
  if (mBatchDepth > 0) {
    LOG("Waiting on batch to complete\n");
    return NS_OK;
  }
  RequestQueue::iterator queueIter = mRequestQueue.begin();
  // request queue holds on to the object
  sbRequestItem * request = *queueIter;

  // If this isn't countable just return it by itself
  if (!request->GetIsCountable()) {
    LOG("Single non-batch request found\n");
    aBatch.push_back(request);
    mRequestQueue.erase(queueIter);
    // Release our reference to the request, aBatch is holding a reference to it
    NS_RELEASE(request);
    return NS_OK;
  }

  const PRUint32 requestBatchId = request->GetBatchId();

  // find the end of the batch and keep track of the matching batch entries
  const RequestQueue::const_iterator queueEnd = mRequestQueue.end();
  while (queueIter != queueEnd &&
         requestBatchId == (*queueIter)->GetBatchId()) {
    request = *queueIter++;
    aBatch.push_back(request);
    // Release our reference to the request, aBatch is holding a reference to it
    NS_RELEASE(request);
  }

  // Remove all the items we pushed onto the batch
  mRequestQueue.erase(mRequestQueue.begin(), queueIter);

  return NS_OK;
}

nsresult sbRequestThreadQueue::ClearRequestsNoLock(Batch & aBatch)
{
  NS_ENSURE_STATE(mLock);

  // Copy all the requests on the queue to aBatch
  std::insert_iterator<Batch> insertIter(aBatch, aBatch.end());
  std::copy(mRequestQueue.begin(), mRequestQueue.end(), insertIter);

  // Release all of our objects
  std::for_each(mRequestQueue.begin(), mRequestQueue.end(), ReleaseRequestItem);

  // Now that we have copied the requests clear our request queue
  mRequestQueue.clear();

  return NS_OK;
}

nsresult sbRequestThreadQueue::ClearRequests()
{
  nsresult rv;

  NS_ENSURE_STATE(mLock);

  Batch batch;
  // Lock the queue while copy and clear it
  {
    nsAutoLock lock(mLock);

    rv = ClearRequestsNoLock(batch);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Now that we have released the lock, cleanup the unprocessed items in the
  // the batch. Cleanup can result in proxying to the main thread which can
  // deadlock if we're holding the lock.
  rv = CleanupBatch(batch);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbRequestThreadQueue::CancelRequests()
{
  NS_ENSURE_STATE(mStopWaitMonitor);

  nsresult rv;

  Batch batch;
  {
    // Have to lock mLock before mStopWaitMonitor to avoid deadlocks
    nsAutoLock lock(mLock);
    nsAutoMonitor monitor(mStopWaitMonitor);
    // If we're aborting set the flag, reset batch depth and clear requests
    if (!mAbortRequests) {
      if (mIsHandlingRequests) {
        mAbortRequests = true;
        monitor.NotifyAll();
      }
      mBatchDepth = 0;
      rv = ClearRequestsNoLock(batch);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }
  // must not hold onto the monitor while we create sbDeviceStatus objects
  // because that involves proxying to the main thread
  rv = CleanupBatch(batch);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

bool sbRequestThreadQueue::CheckAndResetRequestAbort()
{
  NS_ASSERTION(mLock, "mLock null");

  // Notify interested parties that we are aborting. This would be anyone that
  // called GetStopWaitMonitor and may be blocking on it. Scope just to make
  // sure we follow AB BA lock pattern with mLock
  nsAutoMonitor monitor(mStopWaitMonitor);
  const bool abort = mAbortRequests || mStopProcessing;
  if (abort) {
    mAbortRequests = false;
  }
  return abort;
}

//------------------------------------------------------------------------------
//
// Device request added event nsISupports services.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbRTQAddedEvent, nsIRunnable);


//------------------------------------------------------------------------------
//
// Device request added event nsIRunnable services.
//
//------------------------------------------------------------------------------

nsresult sbRTQAddedEvent::Initialize(sbRequestThreadQueue * aRTQ)
{
  mRTQ = aRTQ;

  return NS_OK;
}

class sbAutoRequestHandling
{
public:
  sbAutoRequestHandling(sbRequestThreadQueue * aRTQ) : mRTQ(aRTQ)
  {
    mAlreadyHandlingRequests = mRTQ->StartRequests();
  }
  ~sbAutoRequestHandling()
  {
    if (!mAlreadyHandlingRequests) {
      mRTQ->CompleteRequests();
    }
  }
  bool AlreadyHandlingRequests() const
  {
    return mAlreadyHandlingRequests;
  }
private:
  // Non-owning reference, object is assured to outlive this
  sbRequestThreadQueue * mRTQ;
  bool mAlreadyHandlingRequests;
};
/**
 * Run the event.
 */

NS_IMETHODIMP
sbRTQAddedEvent::Run()
{
  NS_ENSURE_STATE(mRTQ);

  nsresult rv;

  sbAutoRequestHandling autoIsHandlingRequests(mRTQ);

  // Prevent re-entrancy.  This can happen if some API waits and runs events on
  // the request thread.  Do nothing if already handling requests.  Otherwise,
  // indicate that requests are being handled and set up to automatically set
  // the handling requests flag to false on exit.  This check is only done on
  // the request thread, so locking is not required.
  if (autoIsHandlingRequests.AlreadyHandlingRequests()) {
    return NS_OK;
  }

  // Start processing of the next request batch and set to automatically
  // complete the current request on exit.
  sbRequestThreadQueue::Batch batch;
  rv = mRTQ->PopBatch(batch);
  NS_ENSURE_SUCCESS(rv, rv);

  // Keep processing until there are no more batches.
  while (!batch.empty()) {
    PRUint32 batchRequestType = batch.RequestType();

    // Need to dispatch the thread stop event and then return
    if (batchRequestType == sbRequestThreadQueue::REQUEST_THREAD_STOP) {
      NS_DispatchToMainThread(mRTQ->mShutdownAction);
      return NS_OK;
    }

    // Check for abort.
    if (mRTQ->CheckAndResetRequestAbort()) {
      rv = mRTQ->CleanupBatch(batch);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_ERROR_ABORT;
    }

    /**
     * Thread start and stop are internal events that we handle and use
     * the virtual methods OnThreadStart and OnThreadStop to provide specialized
     * behavior.
     */
    if (batchRequestType == sbRequestThreadQueue::REQUEST_THREAD_START) {
      // Call OnThreadStart on the device thread, for things like
      // CoInitialize
      rv = mRTQ->OnThreadStart();
      NS_ENSURE_SUCCESS(rv, rv);
    }

    rv = mRTQ->ProcessBatch(batch);

    if (rv == NS_ERROR_ABORT) {
      rv = mRTQ->CleanupBatch(batch);
      NS_ENSURE_SUCCESS(rv, rv);
      return NS_OK;
    }
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mRTQ->PopBatch(batch);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}


/**
 * Create a new sbRTQAddedEvent object for the device specified by aDevice
 * and return it in aEvent.
 *
 * \param aRTQ               The request thread processor that will own this
 *                           event
 * \param aEvent             The created event
 */

/* static */
nsresult sbRTQAddedEvent::New(sbRequestThreadQueue* aRTQ,
                              nsIRunnable**    aEvent)
{
  NS_ENSURE_ARG_POINTER(aEvent);

  nsresult rv;

  // Create the event object.
  nsCOMPtr<sbRTQAddedEvent> event = new sbRTQAddedEvent();
  NS_ENSURE_TRUE(event, NS_ERROR_OUT_OF_MEMORY);

  rv = event->Initialize(aRTQ);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIRunnable> runnable = do_QueryInterface(event, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  runnable.forget(aEvent);

  return NS_OK;
}
