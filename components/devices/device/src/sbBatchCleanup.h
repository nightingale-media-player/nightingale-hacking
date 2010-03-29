/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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

#ifndef sbBatchCleanup_h
#define sbBatchCleanup_h

#include <sbBaseDevice.h>

/**
 * This class is used to clean up request items left in a batch after a user
 * cancels or an error that prevents the batch from being completed
 */
class sbBatchCleanup
{
public:
  /**
   * aDevice the device associated with teh batch
   * \param aStart The first item of the batch
   * \param aEnd The end of the iterator of the batch
   */
  sbBatchCleanup(sbBaseDevice * aDevice,
                 sbBaseDevice::Batch &aBatch,
                 sbBaseDevice::Batch::const_iterator aStart,
                 sbBaseDevice::Batch::const_iterator aEnd) :
                   mBatch(aBatch),
                   mCurrent(aStart),
                   mEnd(aEnd),
                   mDevice(aDevice),
                   mAtEnd(aStart == aEnd)
  {
  }
  /**
   * Cleans up the items that haven't been proccessed
   */
  ~sbBatchCleanup() {
    if (!mAtEnd && !mBatch.empty()) {
#if DEBUG
      nsresult rv = mDevice->RemoveLibraryItems(mCurrent, mEnd);
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
                          "Unable to cleanup library items");
#else
      mDevice->RemoveLibraryItems(mCurrent, mEnd);
#endif
    }
  }
  /**
   * Sets the next item after the current has been completed
   * \param aNextIter the iterator to the next item being processed
   */
  void ItemCompleted(sbBaseDevice::Batch::const_iterator aNextIter)
  {
    NS_ASSERTION(mCurrent != mEnd,
                 "ItemCompleted called after end of batch encountered");
    mCurrent = aNextIter;
    mAtEnd = mCurrent == mEnd;
  }
  /**
   * This resets the iterators when the batch has been modified
   * \param aCurrent the current iterator to be processed
   * \param aEnd the end of the batch
   */
  void ResetIterators(sbBaseDevice::Batch::const_iterator aCurrent,
                      sbBaseDevice::Batch::const_iterator aEnd)
  {
    mCurrent = aCurrent;
    mEnd = aEnd;
    mAtEnd = mCurrent == mEnd;
  }
private:
  /**
   * Current batch being processed
   */
  sbBaseDevice::Batch &mBatch;
  /**
   * Current item being processed
   */
  sbBaseDevice::Batch::const_iterator mCurrent;
  /**
   * End of the batch
   */

  sbBaseDevice::Batch::const_iterator mEnd;
   /**
    * Pointer to the device, non-owning since we never live beyond it
    */
  sbBaseDevice * mDevice;
  /**
   * We use this because during destruction the iterators may not be valid.
   */
  PRBool mAtEnd;
};

#endif
