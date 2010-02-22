/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef sbDeviceStatus_h
#define sbDeviceStatus_h

/**
 * \file  sbDeviceStatusHelper.h
 * \brief Songbird Device Status Services Definitions.
 */

// Songbird imports.
#include "sbBaseDevice.h"
#include <sbIDeviceStatus.h>
#include <sbMemoryUtils.h>

// Mozilla imports.
#include <nsCOMPtr.h>

/**
 * This class communicates device status to the system. It's a wrapper around
 * sbIDeviceStatus to make things a little easier to report status
 */

class sbDeviceStatusHelper
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Operation types.
  //
  //   OPERATION_TYPE_NONE      No operation type defined.
  //   OPERATION_TYPE_MOUNT     Mount operation.
  //   OPERATION_TYPE_WRITE     Write operation.
  //   OPERATION_TYPE_TRANSCODE Transcode operation
  //   OPERATION_TYPE_DELETE    Delete operation.
  //   OPERATION_TYPE_READ      Read operation
  //   OPERATION_TYPE_FORMAT    Format operation.
  //
  enum Operation {
    OPERATION_TYPE_NONE,
    OPERATION_TYPE_MOUNT,
    OPERATION_TYPE_WRITE,
    OPERATION_TYPE_TRANSCODE,
    OPERATION_TYPE_DELETE,
    OPERATION_TYPE_READ,
    OPERATION_TYPE_FORMAT
  };


  //
  // Operation services.
  //

  void OperationStart(Operation     aOperationType,
                      PRInt32       aItemNum,
                      PRInt32       aItemCount,
                      PRInt32       aItemType,
                      sbIMediaList* aMediaList = nsnull,
                      sbIMediaItem* aMediaItem = nsnull);

  void OperationComplete(nsresult aResult);


  //
  // Item services.
  //

  void ItemStart(PRInt32     aItemNum,
                 PRInt32     aItemCount,
                 PRInt32     aItemType);

  void ItemStart(sbIMediaList* aMediaList,
                 sbIMediaItem* aMediaItem,
                 PRInt32       aItemNum,
                 PRInt32       aItemCount,
                 PRInt32       aItemType = 0);

  void ItemProgress(double aProgress);

  void ItemComplete(nsresult aResult);


  //
  // Public services.
  //
  sbDeviceStatusHelper(sbBaseDevice* aDevice);

  ~sbDeviceStatusHelper();

  nsresult Initialize();

  nsresult ChangeState(PRUint32 aState);

  nsresult GetCurrentStatus(sbIDeviceStatus** aCurrentStatus);

  nsresult UpdateStatus(const nsAString& aOperation,
                        const nsAString& aStateMessage,
                        PRInt32          aItemNum,
                        PRInt32          aItemCount,
                        double           aProgress,
                        PRInt32          aItemType = 0);

  PRUint32 GetSubState() {
    PRUint32 subState;
    nsresult rv = mStatus->GetCurrentSubState(&subState);
    NS_ENSURE_SUCCESS(rv, sbIDevice::STATE_IDLE);
    return subState;
  }

  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Internal services fields.
  //
  //   mDevice                  Device object.  Don't refcount to avoid a cycle.
  //   mStatus                  Base device status object.
  //   mOperationType           Current operation type.
  //   mMediaList               Current operation media list.
  //   mMediaItem               Current operation media item.
  //   mItemNum                 Current operation item number.
  //   mItemCount               Current operation item count.
  //   mItemType                Current item type.
  //

  sbBaseDevice*                 mDevice;
  nsCOMPtr<sbIDeviceStatus>     mStatus;
  Operation                     mOperationType;
  nsCOMPtr<sbIMediaList>        mMediaList;
  nsCOMPtr<sbIMediaItem>        mMediaItem;
  PRInt32                       mItemNum;
  PRInt32                       mItemCount;
  PRInt32                       mItemType;
};


/**
 * These classes complete the current operation or item when going out of scope.
 * The auto-completion may be prevented by calling the forget method.
 *
 *   sbDeviceStatusAutoState             Wrapper to auto set the state to a
 *                                       specified value.
 */

SB_AUTO_CLASS2(sbDeviceStatusAutoState,
               sbDeviceStatusHelper*,
               PRUint32,
               (mValue != nsnull),
               mValue->ChangeState(mValue2),
               mValue = nsnull);

/**
 * Auto completes an operation. Once constructed the auto class is set to
 * auto fail. In order to auto complete a success you must call SetResult with
 * NS_OK.
 */
class sbDeviceStatusAutoOperationComplete
{
public:
  static bool IsItemOp(sbDeviceStatusHelper::Operation aOperation) {
    NS_ASSERTION(aOperation != sbDeviceStatusHelper::OPERATION_TYPE_NONE,
                 "sbMSCStatusAutoOperationComplete::isItemOp pass 'none'");
    // Make sure we're updated and know about all the types
    NS_ASSERTION(aOperation == sbDeviceStatusHelper::OPERATION_TYPE_MOUNT ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_WRITE ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_TRANSCODE ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_DELETE ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_READ ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_FORMAT,
                 "sbDeviceStatusAutoOperationComplete::isItemOp is not current");

    return (aOperation != sbDeviceStatusHelper::OPERATION_TYPE_MOUNT) &&
           (aOperation != sbDeviceStatusHelper::OPERATION_TYPE_FORMAT);
  }
  /**
   * Default constructor used when expecting to transfer auto complete
   */
  sbDeviceStatusAutoOperationComplete() :
    mRequest(nsnull),
    mStatus(nsnull),
    mResult(NS_ERROR_FAILURE),
    mOperation(sbDeviceStatusHelper::OPERATION_TYPE_NONE) {}
  /**
   * Initialize the request and status, start the operation and
   * setup to auto fail. Must Call SetResult for successful completion
   */
  sbDeviceStatusAutoOperationComplete(
                                     sbDeviceStatusHelper * aStatus,
                                     sbDeviceStatusHelper::Operation aOperation,
                                     sbBaseDevice::TransferRequest * aRequest) :
                                       mRequest(aRequest),
                                       mStatus(aStatus),
                                       mResult(NS_ERROR_FAILURE),
                                       mOperation(aOperation)
  {
    // If this is the start of a batch or is not a batch thingy do start op
    if (mRequest->batchIndex == sbBaseDevice::BATCH_INDEX_START) {
      mStatus->OperationStart(mOperation,
                              mRequest->batchIndex,
                              mRequest->batchCount,
                              mRequest->itemType,
                              IsItemOp(mOperation) ? mRequest->list : nsnull,
                              IsItemOp(mOperation) ? mRequest->item : nsnull);
    }
    if (IsItemOp(mOperation)) {
      // Update item status
      mStatus->ItemStart(aRequest->list,
                         aRequest->item,
                         aRequest->batchIndex,
                         aRequest->batchCount,
                         aRequest->itemType);
    }
  }
  sbDeviceStatusAutoOperationComplete(
                                   sbDeviceStatusHelper * aStatus,
                                   sbDeviceStatusHelper::Operation aOperation) :
                                     mRequest(nsnull),
                                     mStatus(aStatus),
                                     mResult(NS_ERROR_FAILURE),
                                     mOperation(aOperation)
  {
    mStatus->OperationStart(mOperation,
                            -1,
                            -1,
                            -1,
                            nsnull,
                            nsnull);
  }
  /**
     * Initialize the request and status, start the operation and
     * setup to auto fail. Must Call SetResult for successful completion
     * This version allows overriding of the batch count
     */
  sbDeviceStatusAutoOperationComplete(
                                     sbDeviceStatusHelper * aStatus,
                                     sbDeviceStatusHelper::Operation aOperation,
                                     sbBaseDevice::TransferRequest * aRequest,
                                     PRInt32 aBatchCount) :
                                       mRequest(aRequest),
                                       mStatus(aStatus),
                                       mResult(NS_ERROR_FAILURE),
                                       mOperation(aOperation) {
    mStatus->OperationStart(mOperation,
                            0,
                            aBatchCount,
                            aRequest->itemType,
                            IsItemOp(mOperation) ? mRequest->list : nsnull,
                            IsItemOp(mOperation) ? mRequest->item : nsnull);
  }

  /**
   * If this is the last item in the batch then call the OperationComplete
   * method.
   */
  ~sbDeviceStatusAutoOperationComplete() {
    Complete();
  }
  /**
   * Complete the operation
   */
  void Complete() {
    if (mStatus && mRequest) {
      if (IsItemOp(mOperation)) {
        mStatus->ItemComplete(mResult);
      }
      if (mRequest->batchIndex == mRequest->batchCount) {
        mStatus->OperationComplete(mResult);
      }
    }
    // We've completed it, lets make sure we don't do it again
    mStatus = nsnull;
    mRequest = nsnull;
  }

  /**
   * Set the result code for auto complete. Success code will set the
   * operation as completed successfully
   */
  void SetResult(nsresult aResult) {
    mResult = aResult;
  }
  /**
   * Transfers the auto complete function to another object
   */
  void Transfer(sbDeviceStatusAutoOperationComplete & aDestination) {
    aDestination = *this;
    // Prevent us from auto completing since aDestination now will
    mStatus = nsnull;
    mRequest = nsnull;
  }
private:
  // This will live in the request queue as long as we're alive
  nsRefPtr<sbBaseDevice::TransferRequest> mRequest;
  sbDeviceStatusHelper * mStatus;
  nsresult mResult;
  sbDeviceStatusHelper::Operation mOperation;

  /**
   * Copies the auto complete status
   */
  sbDeviceStatusAutoOperationComplete &
  operator = (sbDeviceStatusAutoOperationComplete const & aOther) {
    mRequest = aOther.mRequest;
    mStatus = aOther.mStatus;
    mResult = aOther.mResult;
    mOperation = aOther.mOperation;
    return *this;
  }

  // Prevent copying
  sbDeviceStatusAutoOperationComplete(
    sbDeviceStatusAutoOperationComplete const &);
};

#endif /* __SB_MSC_STATUS_H__ */

