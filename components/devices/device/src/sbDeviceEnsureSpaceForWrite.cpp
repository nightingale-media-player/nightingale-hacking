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

#include "sbDeviceEnsureSpaceForWrite.h"

// Local includes
#include "sbDeviceUtils.h"
#include "sbRequestItem.h"

// Standard includes
#include <algorithm>
#include <set>

// Mozilla interfaces
#include <nsIPropertyBag2.h>
#include <nsIVariant.h>

// Mozilla includes
#include <nsArrayUtils.h>

// Songbird interfaces
#include <sbIDeviceEvent.h>
#include <sbIDeviceProperties.h>

// Songbird includes
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
  bool aInSync,
  Batch & aBatch) :
    mDevice(aDevice),
    mBatch(aBatch),
    mTotalLength(0),
    mFreeSpace(0),
    mInSync(aInSync) {
}

sbDeviceEnsureSpaceForWrite::~sbDeviceEnsureSpaceForWrite() {
  // Default destruction of data members
}

static void RemoveProcessedBatchEntries(sbBaseDevice::Batch & aBatch)
{
  sbBaseDevice::Batch::iterator iter = aBatch.begin();
  while (iter != aBatch.end()) {
    sbRequestItem * request = *iter;
    if (request->GetIsProcessed()) {
      // Get the next iter before we remove the current entry so we can update
      // iter
      sbBaseDevice::Batch::iterator next = iter;
      ++next;
      aBatch.erase(iter);
      iter = next;
    }
    else {
      ++iter;
    }
  }
}

nsresult
sbDeviceEnsureSpaceForWrite::BuildItemsToWrite() {
  nsresult rv;

  PRUint32 order = 0;
  // We don't need to addref these as the batch holds on to them
  typedef std::list<sbBaseDevice::TransferRequest *> RequestErrors;

  // List of items that errors occurred.
  RequestErrors requestErrorList;
  // List of items that we need to report errors for
  RequestErrors requestErrorReport;

  PRUint32 index = 0;
  const Batch::const_iterator end = mBatch.end();
  for (Batch::const_iterator iter = mBatch.begin();
       iter != end;
       ++iter) {
    // Add item to write list.  Collect errors for later processing.
    bool errorReported = false;
    rv = AddItemToWrite(static_cast<TransferRequest*>(*iter), index++, order);
    if (NS_FAILED(rv)) {
      // See if it's an error that would have been already reported
      switch (rv) {
        case SB_ERROR_DOWNLOAD_SIZE_UNAVAILABLE:
        case SB_ERROR_DEVICE_DRM_FAILURE:
        case SB_ERROR_DEVICE_DRM_CERT_FAIL:
          errorReported = true;
          break;
      }
      sbBaseDevice::TransferRequest * request =
        static_cast<sbBaseDevice::TransferRequest*>(*iter);
      NS_ENSURE_SUCCESS(rv, rv);
      if (!errorReported) {
        requestErrorReport.push_back(request);
      }
      requestErrorList.push_back(request);
      // We use the processed flag to flag for delete
      request->SetIsProcessed(true);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  // Process any errors.
  RequestErrors::const_iterator requestErrorReportEnd =
    requestErrorReport.end();
  for (RequestErrors::const_iterator requestErrorReportIter =
       requestErrorReport.begin();
       requestErrorReportIter != requestErrorReportEnd;
       ++requestErrorReportIter) {
    sbBaseDevice::TransferRequest * transferRequest = *requestErrorReportIter;

    // Dispatch an error event.
    mDevice->CreateAndDispatchEvent(
                                sbIDeviceEvent::EVENT_DEVICE_ERROR_UNEXPECTED,
                                sbNewVariant(transferRequest->item),
                                PR_TRUE);
  }

  // Remove the items that had errors from the device library
  mDevice->RemoveLibraryItems(requestErrorList.begin(),
                              requestErrorList.end());

  // Remove all the entries that errors occurred
  RemoveProcessedBatchEntries(mBatch);

  return NS_OK;
}

nsresult
sbDeviceEnsureSpaceForWrite::AddItemToWrite(
                                       TransferRequest * aRequest,
                                       PRUint32 aIndex,
                                       PRUint32 & aOrder)
{
  nsresult rv;

  PRUint32 type = aRequest->GetType();

  if (type != sbBaseDevice::TransferRequest::REQUEST_WRITE) {
    NS_WARNING("EnsureSpaceForWrite received a non-write aRequest");
    return NS_OK;
  }
  nsCOMPtr<sbILibrary> requestLib =
    do_QueryInterface(aRequest->list, &rv);
  if (NS_FAILED(rv) || !requestLib) {
    // this is an add to playlist aRequest, don't worry about size
    return NS_OK;
  }

  if (!mOwnerLibrary) {
    rv = sbDeviceUtils::GetDeviceLibraryForItem(mDevice,
                                                aRequest->list,
                                                getter_AddRefs(mOwnerLibrary));
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_STATE(mOwnerLibrary);
  } else {
    #if DEBUG
      nsCOMPtr<sbIDeviceLibrary> newLibrary;
      rv = sbDeviceUtils::GetDeviceLibraryForItem(mDevice,
                                                  aRequest->list,
                                                  getter_AddRefs(newLibrary));
      NS_ENSURE_SUCCESS(rv, rv);
      NS_ENSURE_STATE(newLibrary);
      NS_ENSURE_STATE(SameCOMIdentity(mOwnerLibrary, newLibrary));
    #endif /* DEBUG */
  }
  ItemsToWrite::iterator const
    itemToWriteIter = mItemsToWrite.find(aRequest->item);
  if (itemToWriteIter != mItemsToWrite.end()) {
    itemToWriteIter->second.mBatchIndexes.push_back(aIndex);
    // this item is already in the set, don't worry about it
    return NS_OK;
  }

  PRUint64 writeLength;
  rv = sbDeviceUtils::GetDeviceWriteLength(mOwnerLibrary,
                                           aRequest->item,
                                           &writeLength);
  NS_ENSURE_SUCCESS(rv, rv);
  writeLength += mDevice->mPerTrackOverhead;

  mTotalLength += writeLength;
  LOG(("r(%p) i(%p) sbBaseDevice::EnsureSpaceForWrite - size %lld\n",
       (void*)aRequest, (void*)(aRequest->item), writeLength));

  mItemsToWrite[aRequest->item] = BatchLink(++aOrder, writeLength, aIndex);

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

  nsresult rv;

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
      if (mInSync) {
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
    if (left.mBatchIndexes.empty() && !right.mBatchIndexes.empty()) {
      return true;
    }
    else if (right.mBatchIndexes.empty()) {
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
      // won't fit, mark rest of items to be removed
      const BatchLink::BatchIndexes::const_iterator end =
        batchLink.mBatchIndexes.end();
      for (BatchLink::BatchIndexes::const_iterator iter =
             batchLink.mBatchIndexes.begin();
           iter != end;
           ++iter) {
        TransferRequest * const request =
          static_cast<TransferRequest*>(mBatch[*iter]);
        if (request) {
          itemsToRemove.push_back(RemoveItemInfo(request->item,
                                                 request->list));
          request->SetIsProcessed(true);
        }
      }
    }
  }

  RemoveProcessedBatchEntries(mBatch);

  rv = RemoveItemsFromLibrary(itemsToRemove.begin(), itemsToRemove.end());
  NS_ENSURE_SUCCESS(rv, rv);

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
