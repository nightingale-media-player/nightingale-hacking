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

#ifndef SBREQUESTITEM_H_
#define SBREQUESTITEM_H_

// Mozilla includes
#include <nsCOMPtr.h>

// Mozilla interfaces
#include <nsIClassInfo.h>

/**
 * This provides the basic information for a request. The class does not need
 * to be completely thread safe. Creation is to occur on the thread calling
 * sbIRequestThreadQueue::pushRequest. Further access will only occur on the
 * request thread queue's thread when the request is removed. The object does
 * use the thread safe addref/release to prevent warnings in XPCOM since the
 * object is created on one thread and used on another.
 */
class sbRequestItem
{
public:

  friend class sbRequestThreadQueue;

  NS_IMETHOD_(nsrefcnt) AddRef(void);
  NS_IMETHOD_(nsrefcnt) Release(void);

  /**
   * Returns the unique Id of the request
   */
  PRUint32 GetRequestId() const
  {
    return mRequestId;
  }

  /**
   * Returns the type of the request. This type is defined by the derived
   * classes.
   */
  PRUint32 GetType() const
  {
    return mType;
  }

  /**
   * Sets the type of the request
   */
  void SetType(PRUint32 aType)
  {
    mType = aType;
  }

  /**
   * Returns the batch ID of the item
   */
  PRUint32 GetBatchId() const
  {
    return mBatchId;
  }

  /**
   * Returns index of the item within the batch. This is for counting purposes
   * and so is the logical location not the physical location.
   */
  PRUint32 GetBatchIndex() const
  {
    return mBatchIndex;
  }

  /**
   * Sets the batch index for this item.
   */
  void SetBatchIndex(PRInt32 aBatchIndex)
  {
    mBatchIndex = aBatchIndex;
  }


  /**
   * Returns the timestamp that this item was created
   */
  PRInt64 GetTimeStamp() const
  {
    return mTimeStamp;
  }

  /**
   * Returns whether this item is countable
   */
  bool GetIsCountable() const
  {
    return mIsCountable;
  }

  /**
   * Returns the is processed flag which denotes whether the request as
   * successfully processed.
   */
  bool GetIsProcessed() const
  {
    return mIsProcessed;
  }

  /**
   * Sets state of the processed flag
   */
  void SetIsProcessed(bool aIsProcessed)
  {
    mIsProcessed = aIsProcessed;
  }

  static sbRequestItem * New(PRUint32 aType, bool aIsCountable = false)
  {
    sbRequestItem * requestItem;
    requestItem = new sbRequestItem;
    if (requestItem) {
      requestItem->SetType(aType);
      requestItem->SetIsCountable(aIsCountable);
    }
    return requestItem;
  }

protected:
  /**
   * Initializes the request type and defaults for data members.
   */
  sbRequestItem();

  virtual ~sbRequestItem();
  // Protected interface to be used by the request queue and derived classes

  /**
   * Sets the flag determining if the request is countable
   */
  void SetIsCountable(bool aIsCountable)
  {
    mIsCountable = aIsCountable;
  }

private:

  /**
   * Generated sequential, unique ID of the request
   */
  PRUint32 mRequestId;

  /**
   * The type of the request. The values are defined by the user of the
   * request thread queue.
   */
  PRUint32 mType;

  /**
   * Generated sequential, unique ID of the batch the request belongs to
   */
  PRUint32 mBatchId;

  /**
   * The position of the request within the batch. This is the logical position
   * not the physical and it is 0 a zero based index
   */
  PRUint32 mBatchIndex;

  /**
   * The time this request was created
   */
  PRInt64 mTimeStamp;

  /**
   * Whether this request is part of a batch
   */
  bool mIsCountable;

  /**
   * Indicates whether the request has been successfully processed.
   */
  bool mIsProcessed;

  /**
   * Reference counter
   */
  nsAutoRefCnt mRefCnt;
  /**
   * Used to generate the sequential unique requst ID
   */
  static PRInt32 sLastRequestId;
};

#endif
