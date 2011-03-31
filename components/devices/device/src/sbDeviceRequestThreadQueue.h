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

#ifndef SBDEVICEREQUESTTHREADQUEUE_H_
#define SBDEVICEREQUESTTHREADQUEUE_H_

// Mozzila includes
#include <nsAutoPtr.h>
#include <pldhash.h>

// Mozilla interfaces
#include "sbRequestThreadQueue.h"

class nsIMutableArray;
class sbBaseDevice;

class sbDeviceRequestThreadQueue : public sbRequestThreadQueue
{
public:
  static sbDeviceRequestThreadQueue * New();
  nsresult Start(sbBaseDevice * aBaseDevice);
private:
  // There is a cycle between the device, thread queue processor and the actions
  // this will be broken on disconnect. We manually manage the ref count due
  // to the ambiguities of nsISupports..
  sbBaseDevice * mBaseDevice;
  sbDeviceRequestThreadQueue();
  ~sbDeviceRequestThreadQueue();
  static PLDHashOperator RemoveLibraryEnumerator(
                                             nsISupports * aList,
                                             nsCOMPtr<nsIMutableArray> & aItems,
                                             void * aUserArg);

  /**
   * This is called when the request thread is stopped
   */
  virtual nsresult OnThreadStop();

  /**
   * Determines if the request is a duplicate of an existing item in the queue
   * \param aItem1 the item being checked for duplicates
   * \param aItem2 The other item being checked as a duplicate
   * \return true if the two items are duplicates
   */
  virtual nsresult IsDuplicateRequest(sbRequestItem * aItem1,
                                      sbRequestItem * aItem2,
                                      bool & aIsDuplicate,
                                      bool & aContinueChecking);

  /**
   * Called to process a batch of requests. The implementation shoudl set the
   * processed flag as cleanupBatch will be called after this.
   */
  virtual nsresult ProcessBatch(Batch & aBatch);

  /**
   * Called to cleanup requests that haven't been processed. Default behavior
   * is to do nothing
   */
  virtual nsresult CleanupBatch(Batch & aBatch);
};

#endif
