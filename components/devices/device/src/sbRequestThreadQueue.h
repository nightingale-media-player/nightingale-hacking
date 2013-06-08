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

#ifndef SBREQUESTTHREADQUEUE_H_
#define SBREQUESTTHREADQUEUE_H_

// Platform includes
#include <algorithm>
#include <deque>
#include <list>

// Mozilla includes
#include <mozilla/Mutex.h>
#include <mozilla/Monitor.h>
#include <nsAutoPtr.h>
#include <nsCOMPtr.h>
#include <prmon.h>
#include <prlock.h>

// Mozilla interfaces
#include <nsIClassInfo.h>
#include <nsIRunnable.h>
#include <nsIThread.h>
#include <nsITimer.h>

#include "sbLockUtils.h"

// Local includes
#include "sbRequestItem.h"

/**
 * This class implements a mechanism to allow processing of requests off the
 * main thread. PushRequest places requests on the queue. BatchBegin and
 * BatchEnd mark the start and end of batches. Batches of requests are processed
 * via the pure virtual method ProcessBatch. Derived classes implement this to
 * do work specific to their needs. Start is called to start the request thread
 * and Stop is used to stop the request thread.
 *
 * sbRequestItem is owned by sbRequestThreadQueue when pushed onto the queue.
 * A reference is added. When PopBatch is called, ownership is transferred to
 * the Batch object passed in. The sbRequestItem object holds a reference and
 * releases it when the Batch is destroyed or the item is erased via erase or
 * clear methods. Batch's destructor calls clear.
 */
class sbRequestThreadQueue
{
public:

  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  /**
   * A batch contains a sequence of requests. The order of the items are the
   * order they were added. A batch may contain only one type of countable
   * request, however it may contain more than one type of non-countable types.
   */
  class Batch
  {
    typedef std::list<sbRequestItem *> RequestItems;
  public:
    // types
    typedef RequestItems::const_iterator const_iterator;
    typedef RequestItems::iterator iterator;
    typedef RequestItems::size_type size_type;
    typedef RequestItems::reference reference;
    typedef RequestItems::const_reference const_reference;
    typedef RequestItems::value_type value_type;

    Batch();
    ~Batch();

    /**
     * Returns the iterator to the first item in the batch
     */
    const_iterator begin() const
    {
      return mRequestItems.begin();
    }

    /**
     * Returns the iterator to the first item in the batch
     */
    iterator begin()
    {
      return mRequestItems.begin();
    }

    /**
     * Returns the iterator to the end of the batch
     */
    const_iterator end() const
    {
      return mRequestItems.end();
    }

    /**
     * Returns the iterator to the end of the batch
     */
    iterator end()
    {
      return mRequestItems.end();
    }

    /**
     * Returns the size of the batch. This is the physical size not the number
     * of countable items.
     */
    size_type size()
    {
      return mRequestItems.size();
    }
    /**
     * Returns the countable items contained within the batch
     */
    PRUint32 CountableItems() const
    {
      return mCountableItems;
    }

    /**
     * Returns the request type of the countable items.
     */
    PRUint32 RequestType() const
    {
      return mRequestType;
    }

    /**
     * Adds an item to the end of the batch. This will add a reference to aItem.
     */
    void push_back(sbRequestItem * aItem);

    /**
     * Removes the item pointed by aIter from the batch updating indexes and
     * counts. It will also release the reference to the item being erased.
     */
    void erase(iterator aIter);

    /**
     * Returns true if no requests exist in the batch
     */
    bool empty() const
    {
      return mRequestItems.empty();
    }

    /**
     * Insert's a request item at aIter. Needed for the insert iterator for
     * copying.
     */
    iterator insert(iterator aIter, sbRequestItem * aRequestItem)
    {
      NS_ASSERTION(aRequestItem,
                   "sbRequestThreadQueue::Batch::insert passed null");
      NS_IF_ADDREF(aRequestItem);
      return mRequestItems.insert(aIter, aRequestItem);
    }
    /**
     * Sorts the batch using the comparer and recalculates the indexes.
     */
    template <class T>
    void sort(T aComparer)
    {
      std::sort(mRequestItems.begin(), mRequestItems.end(), aComparer);
      RecalcBatchStats();
    }

    /**
     * Array operator. NOTE: this should be avoided as performance is worse
     * iterating over the batch
     */
    sbRequestItem * operator [](int aIndex) const
    {
      const const_iterator end = mRequestItems.end();
      const_iterator iter = mRequestItems.begin();
      while (aIndex-- > 0 && iter != end) ++iter;
      if (iter == end) {
        return nsnull;
      }
      return *iter;
    }

    /**
     * Removes all entries from the batch. This will release all the request
     * items as well.
     */
    void clear();
  private:
    RequestItems mRequestItems;
    PRUint32 mCountableItems;
    PRUint32 mRequestType;

    void RecalcBatchStats();
  };

  /**
   * Performs simple initialization.
   */
  sbRequestThreadQueue();
  virtual ~sbRequestThreadQueue();

  enum {
    /**
     * Request types below this value are reserved for use by this class
     */
    USER_REQUEST_TYPES = 0x20000000,

    /**
     * Used to signal that a request type has not be set
     */
    REQUEST_TYPE_NOT_SET = 0,

    /**
     * Request type of the request sent as the first request
     */
    REQUEST_THREAD_START = 1,

    /**
     * Request type of the request sent as the last request to signal the thread
     * to shutdown
     */
    REQUEST_THREAD_STOP = 2
  };
  /**
   * Starts the request queue processing
   */
  nsresult Start();

  /**
   * This stops the request processing thread. It returns immediately
   * and does not wait for the request thread to shutdown.. The aShutdownAction
   * is used to perform any post cleanup work. This action is called after the
   * thread has actually shutdown.
   */
  nsresult Stop();

  /**
   * Marks the start of a batch
   */
  nsresult BatchBegin();

  /**
   * Marks the end of a batch
   */
  nsresult BatchEnd();

  /**
   * Pushes a request entry onto the request queue. This will add an owning
   * reference to aRequestItem.
   * \param aRequestItem an object derived from sbRequestItem
   */
  nsresult PushRequest(sbRequestItem * aRequestItem);

  /**
   * Returns the next batch. Request items returned in aBatch are no longer
   * owned by the request queue but belong to aBatch.
   *
   * \param aBatch a collection of sbRequestItem derived objects
   */
  nsresult PopBatch(Batch & aBatch);

  /**
   * Clears the request queue. This will release all request item references
   * owned by the request queue.
   */
  nsresult ClearRequests();

  /**
   * Sets the abort flag and clears cany pending requests. This will release all
   * request item references owned by the request queue.
   */
  nsresult CancelRequests();

  /**
   * Returns whether an abort is active and clears the abort flag. We want to
   * clear the flag since all usage patterns will need to inspect the state and
   * if locked clear the flag. This eliminates the issue of checking and then
   * some other code checking and then both thinking an abort is in progress.
   * This MUST only be called from the request processing thread.
   */
  bool CheckAndResetRequestAbort();

  /**
   * This returns whether the RequestThreadQueue is running currently.
   */
  bool IsHandlingRequests() const {
    sbSimpleAutoLock lock(mLock);
    return mIsHandlingRequests;
  }

  /**
   * This simply returns whether an abort has been signaled. Ideally use
   * checkAndResetRequestAbort unless there is a specific reason not to.
   */
  bool IsRequestAbortActive() const
  {
    // XXX Definitely need to check this! Will it exit on return?
    PR_EnterMonitor(mStopWaitMonitor);
    return mAbortRequests;
  }

  /**
   * Returns the monitor used for signaling process stopping. Ownership
   * is retained by "this" and caller should not delete this pointer.
   * This is used so that users can block and wait until an abort request or
   * stop request has been been processed.
   * Care should be taken when using this on the main thread.
   */
  PRMonitor* GetStopWaitMonitor() const
  {
    return mStopWaitMonitor;
  }

protected:
  /**
   * Thread usage summary:
   * The state of this object is protected by the the mLock lock.
   * Accessed by indicator: MT - Thread pushing requests on to queue. Usually this
   *                             is the main thread, but it could be any thread
   *                             including the RT.
   *                        ST - Thread that calls Start, usually will be the
   *                             the main thread.
   *                        RT - Thread that process requests
   * mRequestQueue - MT/CT Modified
   * mBatchDepth - MT Write, RT Read
   * mLock - Any thread, protects  the state of the object
   * mStopProcessing - ST Written, MT/RT Read
   * mAbortRequests - Can be modified by any thread, read from RT and MT
   * mIsHandlingRequests - RT Written,  Read from any thread canceling requests
   * mCurrentBatchId - ST Written, MT - Read/Written
   *
   *---------------------------
   * Not protected by mlock, only used by the request thread or only the thread
   * calling Start and Stop
   *
   * calling start stop
   * mActions - ST Written, RT/MT Used,
   * mThreadStarted - ST written in Start and Stop
   * mReqAddedEvent - ST Create, MT Read
   * mShutdownAction - ST Written, RT used
   * mStopWaitMonitor - Used from any thread, created and destroyed from ST
   * mThread - ST Create, MT calls shutdown
   */

  /**
   * This protects the state of the sbRequestThreadProc. This lock needs
   * to be acquired before changing any data member
   */
  PRLock *mLock;

  /**
   * Tracks batch depth for begin and end calls
   */
  PRInt32 mBatchDepth;

  /**
   * Monitor to use to signal process stopping to external sources.
   */
  PRMonitor *mStopWaitMonitor;

protected:
  /**
   * Flag that signals we're aborting all requests. Any new requests
   * are ignored while this flag is active.
   */
  bool mAbortRequests;

  /**
   * Flag that indicates we're currently handling a request
   */
  bool mIsHandlingRequests;

  /**
   * Performs simple cleanup of the requests queue without locking and returns
   * the cleared requests as an array. It is the caller's responsibility to
   * lock mLock
   * \param aRequests the requests that were cleared
   */
  nsresult ClearRequestsNoLock(Batch & aRequests);

  /**
   * Signals completion of the current batch of requests
   */
  virtual void CompleteRequests()
  {
    sbSimpleAutoLock lock(mLock);
    NS_ASSERTION(mIsHandlingRequests,
                 "CompleteRequests called while no requests pending");
    mIsHandlingRequests = false;
    PR_EnterMonitor(mStopWaitMonitor);
    mAbortRequests = false;
    PR_ExitMonitor(mStopWaitMonitor);
  }

  /**
   * Used by Start and Stop to ensure callers don't call Start multiple times
   * or erroneously call Stop when not started or stop multiple times.
   */
  bool mThreadStarted;

  /**
   * Flag that signals we're stopping request processing
   */
  bool mStopProcessing;
  
private:
  /**
   * The request queue type
   */
  typedef std::deque<sbRequestItem *> RequestQueue;

  /**
   * This is the request queue that holds the requests to be processed by the
   * request thread. These are raw pointers but the reference is managed by
   * sbRequestThreadQueue.
   */
  RequestQueue mRequestQueue;

  /**
   * The thread for processing requests
   */
  nsCOMPtr<nsIThread> mThread;

  /**
   * Event used to dispatch notification of pending events
   */
  nsCOMPtr<nsIRunnable> mReqAddedEvent;

  /**
   * The current batch ID
   */
  PRInt32 mCurrentBatchId;

  /**
   * Holds the action to be performed when the thread is actually shutdown. This
   * is used internally only to call sbRequestThreadQueue::ThreadShutdownAction
   * when the thread has actually shut down
   */
  nsCOMPtr<nsIRunnable> mShutdownAction;

  /**
   * Determines if the request is a duplicate of an existing item in the queue
   * \param aItem the item being checked for duplicates
   * \param aIsDuplicate Holds the duplicate indication on return
   */
  nsresult FindDuplicateRequest(sbRequestItem * aItem, bool &aIsDuplicate);

  /**
   * Processes any pending requests
   */
  nsresult ProcessRequest();

  /**
   * This is called on the main thread to shutdown the processing thread after
   * all work as been done.
   */
  nsresult ThreadShutdownAction(int);

  /**
   * Reference counter
   */
  nsAutoRefCnt mRefCnt;

  /**
   * Singles the processing of a batch of requests. Returns true if we're
   * already handling requests.
   */
  bool StartRequests()
  {
    sbSimpleAutoLock lock(mLock);
    const bool isHandlingRequests = mIsHandlingRequests;
    mIsHandlingRequests = true;
    return isHandlingRequests;
  }

  nsresult PushRequestInternal(sbRequestItem * aRequestItem);

  // Private interface derive classes must implement
  /**
   * Called when the request thread is started.
   */
  virtual nsresult OnThreadStart() { return NS_OK; };

  /**
   * This is called when the request thread is stopped
   */
  virtual nsresult OnThreadStop() { return NS_OK; }

  /**
   * Determines if the request is a duplicate of an existing item in the queue
   * \param aItem1 the item being checked for duplicates
   * \param aItem2 The other item being checked as a duplicate
   * \return true if the two items are duplicates
   */
  virtual nsresult IsDuplicateRequest(sbRequestItem * aItem1,
                                      sbRequestItem * aItem2,
                                      bool & aIsDuplicate,
                                      bool & aContinue)
  {
    aIsDuplicate = false;
    aContinue = true;
    return NS_OK;
  }

  /**
   * Called to process a batch of requests. The implementation shoudl set the
   * processed flag as cleanupBatch will be called after this.
   */
  virtual nsresult ProcessBatch(Batch & aBatch) = 0;

  /**
   * Called to cleanup requests that haven't been processed. Default behavior
   * is to do nothing
   */
  virtual nsresult CleanupBatch(Batch & aBatch)
  {
    return NS_OK;
  }

  // Friend declarations

  /**
   * Our event object used to dispatch event requests
   */
  friend class sbRTQAddedEvent;
  /**
   * Needs access to IsRequestBatchable which otherwise doesn't need to be
   * public. This is an internal utility class
   */
  friend class sbRTQIsBatchable;

  /**
   * Needs access to the StartRequests and CompleteRequests. This is an internal
   * utility class.
   */
  friend class sbAutoRequestHandling;

};



#endif
