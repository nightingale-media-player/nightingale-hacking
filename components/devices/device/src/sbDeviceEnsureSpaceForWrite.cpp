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

#include "sbDeviceEnsureSpaceForWrite.h"

// Local includes
#include "sbDeviceUtils.h"

// Standard includes
#include <algorithm>
#include <set>

// Mozilla includes
#include <nsIPropertyBag2.h>
#include <nsIVariant.h>

// Songbird includes
#include <sbIDeviceEvent.h>
#include <sbIDeviceProperties.h>
#include <sbLibraryUtils.h>
#include <sbStandardDeviceProperties.h>
#include <sbStringUtils.h>
#include <sbVariantUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseDevice:5
 */
#ifdef PR_LOGGING
extern PRLogModuleInfo* gBaseDeviceLog;
#endif

#undef LOG
#define LOG(args) PR_LOG(gBaseDeviceLog, PR_LOG_WARN, args)

// by COM rules, we are only guaranteed that comparing nsISupports* is valid
struct COMComparator {
  bool operator()(nsISupports* aLeft, nsISupports* aRight) const {
    return nsCOMPtr<nsISupports>(do_QueryInterface(aLeft)) <
             nsCOMPtr<nsISupports>(do_QueryInterface(aRight));
  }
};

struct EnsureSpaceForWriteRemovalHelper {
  EnsureSpaceForWriteRemovalHelper(std::set<sbIMediaItem*,COMComparator>& aItemsToRemove)
    : mItemsToRemove(aItemsToRemove.begin(), aItemsToRemove.end())
  {
  }

  bool operator() (nsRefPtr<sbBaseDevice::TransferRequest> & aRequest) {
    // remove all requests where either the item or the list is to be removed
    return (mItemsToRemove.end() != mItemsToRemove.find(aRequest->item) ||
            mItemsToRemove.end() != mItemsToRemove.find(aRequest->list));
  }

  std::set<sbIMediaItem*, COMComparator> mItemsToRemove;
};


sbDeviceEnsureSpaceForWrite::sbDeviceEnsureSpaceForWrite(
  sbBaseDevice * aDevice,
  Batch & aBatch) :
    mDevice(aDevice),
    mBatch(aBatch),
    mTotalLength(0),
    mFreeSpace(0) {
}

sbDeviceEnsureSpaceForWrite::~sbDeviceEnsureSpaceForWrite() {
  // Default destruction of data members
}

nsresult
sbDeviceEnsureSpaceForWrite::BuildItemsToWrite() {
  nsresult rv;

  PRInt32 order = 0;
  Batch::iterator const batchEnd = mBatch.end();
  nsTArray<Batch::iterator> requestErrorList;
  for (Batch::iterator iter = mBatch.begin(); iter != batchEnd; ++iter) {
    // Add item to write list.  Collect errors for later processing.
    rv = AddItemToWrite(iter, order);
    if (NS_FAILED(rv))
      requestErrorList.AppendElement(iter);
  }

  // Process any errors.
  for (PRUint32 i = 0; i < requestErrorList.Length(); ++i) {
    // Get the next error.
    Batch::iterator batchIter = requestErrorList[i];
    sbBaseDevice::TransferRequest* request = *batchIter;

    // Dispatch an error event.
    mDevice->CreateAndDispatchEvent
               (sbIDeviceEvent::EVENT_DEVICE_ERROR_UNEXPECTED,
                sbNewVariant(request->item),
                PR_TRUE);

    // Remove request library item and remove request from batch.
    Batch::iterator nextBatchIter = batchIter;
    ++nextBatchIter;
    mDevice->RemoveLibraryItems(batchIter, nextBatchIter);
    mBatch.erase(batchIter);
  }

  // Update batch if there were any errors.
  if (!requestErrorList.IsEmpty()) {
    SBUpdateBatchCounts(mBatch);
    SBUpdateBatchIndex(mBatch);
  }

  return NS_OK;
}

nsresult
sbDeviceEnsureSpaceForWrite::AddItemToWrite(const Batch::iterator & aIter,
                                            PRInt32&        aOrder)
{
  nsresult rv;

  sbBaseDevice::TransferRequest * request = *aIter;
  if (request->type != sbBaseDevice::TransferRequest::REQUEST_WRITE) {
    NS_WARNING("EnsureSpaceForWrite received a non-write request");
    return NS_OK;
  }
  nsCOMPtr<sbILibrary> requestLib =
    do_QueryInterface(request->list, &rv);
  if (NS_FAILED(rv) || !requestLib) {
    // this is an add to playlist request, don't worry about size
    return NS_OK;
  }

  if (!mOwnerLibrary) {
    rv = sbDeviceUtils::GetDeviceLibraryForItem(mDevice,
                                                request->list,
                                                getter_AddRefs(mOwnerLibrary));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_STATE(mOwnerLibrary);
  } else {
    #if DEBUG
      nsCOMPtr<sbIDeviceLibrary> newLibrary;
      rv = sbDeviceUtils::GetDeviceLibraryForItem(mDevice,
                                                  request->list,
                                                  getter_AddRefs(newLibrary));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_STATE(newLibrary);
      NS_ENSURE_STATE(SameCOMIdentity(mOwnerLibrary, newLibrary));
    #endif /* DEBUG */
  }
  ItemsToWrite::iterator const
    itemToWriteIter = mItemsToWrite.find(request->item);
  if (itemToWriteIter != mItemsToWrite.end()) {
    itemToWriteIter->second.mBatchIters.push_back(aIter);
    // this item is already in the set, don't worry about it
    return NS_OK;
  }

  PRUint64 writeLength;
  rv = sbDeviceUtils::GetDeviceWriteLength(mOwnerLibrary,
                                           request->item,
                                           &writeLength);
  NS_ENSURE_SUCCESS(rv, rv);
  writeLength += mDevice->mPerTrackOverhead;

  mTotalLength += writeLength;
  LOG(("r(%p) i(%p) sbBaseDevice::EnsureSpaceForWrite - size %lld\n",
       (void*)request, (void*)(request->item), writeLength));

  mItemsToWrite[request->item] = BatchLink(++aOrder, writeLength, aIter);

  return NS_OK;
}

nsresult
sbDeviceEnsureSpaceForWrite::GetFreeSpace() {
  // no free space if no library
  if (!mOwnerLibrary) {
    mFreeSpace = 0;
    return NS_OK;
  }

  nsresult rv;

  // get the free space
  nsAutoString freeSpaceStr;
  rv = mOwnerLibrary->GetProperty
                        (NS_LITERAL_STRING(SB_DEVICE_PROPERTY_FREE_SPACE),
                         freeSpaceStr);
  NS_ENSURE_SUCCESS(rv, rv);
  mFreeSpace = nsString_ToInt64(freeSpaceStr, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // apply limit to the total space available for music
  PRInt64 freeMusicSpace;
  rv = mDevice->GetMusicFreeSpace(mOwnerLibrary, &freeMusicSpace);
  NS_ENSURE_SUCCESS(rv, rv);
  if (mFreeSpace >= freeMusicSpace)
    mFreeSpace = freeMusicSpace;

  return NS_OK;
}

/**
 *  there are three cases to go from here:
 * (1) fill up the device, with whatever fits, in random order (sync)
 * (2) fill up the device, with whatever fits, in given order (manual)
 * (3) user chose to abort

 * In the (1) case, if we first shuffle the list, it becomes case (2)
 * and case (2) is just case (3) with fewer files to remove
 */
nsresult
sbDeviceEnsureSpaceForWrite::GetWriteMode(WriteMode & aWriteMode) {
  PRBool isManual;
  nsresult rv = mOwnerLibrary->GetIsManualSyncMode(&isManual);
  NS_ENSURE_SUCCESS(rv, rv);

  // if not enough free space is available, ask user what to do
  if (mFreeSpace < mTotalLength) {
    PRBool abort = PR_FALSE;
    // avoid asking user multiple times in one syncing/copying operation
    if (!mDevice->GetEnsureSpaceChecked()) {
      rv = sbDeviceUtils::QueryUserSpaceExceeded(mDevice,
                                                 mOwnerLibrary,
                                                 mTotalLength,
                                                 mFreeSpace,
                                                 &abort);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (abort) {
      aWriteMode = ABORT;
    }
    else {
      if (!isManual) {
        aWriteMode = SHUFFLE;
      }
      else {
        aWriteMode = MANUAL;
      }
      mDevice->SetEnsureSpaceChecked(true);
    }
  }
  else {
    aWriteMode = NOP;
  }
  return NS_OK;
}

class CompareItemOrderInBatch
{
public:
  typedef sbDeviceEnsureSpaceForWrite::ItemsToWrite ItemsToWrite;
  typedef sbDeviceEnsureSpaceForWrite::BatchLink BatchLink;
  typedef sbDeviceEnsureSpaceForWrite::Batch Batch;

  CompareItemOrderInBatch(ItemsToWrite * aItemsToWrite) :
    mItemsToWrite(aItemsToWrite) {}
  bool operator()(sbIMediaItem * aLeft, sbIMediaItem * aRight) const {
    ItemsToWrite::iterator iter = mItemsToWrite->find(aLeft);
    if (iter == mItemsToWrite->end()) {
      return true;
    }
    BatchLink & left = iter->second;
    iter = mItemsToWrite->find(aRight);
    if (iter == mItemsToWrite->end()) {
      return false;
    }
    BatchLink & right = iter->second;
    if (left.mBatchIters.empty() && !right.mBatchIters.empty()) {
      return true;
    }
    else if (right.mBatchIters.empty()) {
      return false;
    }
    return left.mOrder < right.mOrder;
  }
private:
  ItemsToWrite * mItemsToWrite;
};

nsresult
sbDeviceEnsureSpaceForWrite::RemoveItemsFromLibrary(RemoveItems::iterator iter,
                                                    RemoveItems::iterator end) {

  nsresult rv;

  for (;iter != end; ++iter) {

    rv = iter->mList->Remove(iter->mItem);
  }

  return NS_OK;
}

/**
 * Copies the sbIMediaItem pointers into a list so we can order it later
 */
void sbDeviceEnsureSpaceForWrite::CreateItemList(ItemList & aItems) {

  ItemsToWrite::iterator const end = mItemsToWrite.end();
  for (ItemsToWrite::iterator itemsIter = mItemsToWrite.begin();
       itemsIter != end;
       ++itemsIter) {
    aItems.push_back(itemsIter->first);
  }
}

nsresult
sbDeviceEnsureSpaceForWrite::RemoveExtraItems() {
  WriteMode writeMode;
  nsresult rv = GetWriteMode(writeMode);
  NS_ENSURE_SUCCESS(rv, rv);

  ItemList items;
  switch (writeMode) {
    case SHUFFLE: {
      CreateItemList(items);
      // shuffle the list
      std::random_shuffle(items.begin(), items.end());
    }
    break;
    case MANUAL: {
      CreateItemList(items);
      CompareItemOrderInBatch compare(&mItemsToWrite);
      std::sort(items.begin(), items.end(), compare);
    }
    break;
    case ABORT: {
      return NS_ERROR_ABORT;
    }
    case NOP: { // There's enough space just return
      return NS_OK;
    }
    default: {
      NS_WARNING("Unexpected write mode state in "
                 "sbDeviceEnsureSpaceForWrite::RemoveExtraItems");
    }
    break;
  }

  // and a set of items we can't fit
  RemoveItems itemsToRemove;

  // fit as much of the list as possible
  ItemList::iterator const end = items.end();
  PRInt64 bytesRemaining = mFreeSpace;
  for (ItemList::iterator iter = items.begin(); iter != end; ++iter) {
    ItemsToWrite::iterator const itemIter = mItemsToWrite.find(*iter);
    NS_ASSERTION(itemIter != mItemsToWrite.end(),
                 "items and itemsToWrite out of sync");
    BatchLink & batchLink = mItemsToWrite[*iter];
    PRInt64 const length = batchLink.mLength;

    if (bytesRemaining > length) {
      // this will fit
      bytesRemaining -= length;
    } else {
      // won't fit
      BatchLink::BatchIters::iterator const batchEnd =
        batchLink.mBatchIters.end();
      for (BatchLink::BatchIters::iterator batchIter =
             batchLink.mBatchIters.begin();
           batchIter != batchEnd;
           ++batchIter) {

        TransferRequest * const request = **batchIter;
        itemsToRemove.push_back(RemoveItemInfo(request->item, request->list));
        mBatch.erase(*batchIter);
      }
    }
  }
  rv = RemoveItemsFromLibrary(itemsToRemove.begin(), itemsToRemove.end());
  SBUpdateBatchCounts(mBatch);
  SBUpdateBatchIndex(mBatch);

  return NS_OK;
}

nsresult
sbDeviceEnsureSpaceForWrite::EnsureSpace() {
  nsresult rv = BuildItemsToWrite();
  NS_ENSURE_SUCCESS(rv, rv);

  if (mItemsToWrite.empty()) {
    return NS_OK;
  }

  rv = GetFreeSpace();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = RemoveExtraItems();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
