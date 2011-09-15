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
  //   OPERATION_TYPE_DOWNLOAD  Download operation.
  //
  enum Operation {
    OPERATION_TYPE_NONE,
    OPERATION_TYPE_MOUNT,
    OPERATION_TYPE_WRITE,
    OPERATION_TYPE_TRANSCODE,
    OPERATION_TYPE_DELETE,
    OPERATION_TYPE_READ,
    OPERATION_TYPE_FORMAT,
    OPERATION_TYPE_DOWNLOAD
  };


  //
  // Operation services.
  //

  void OperationStart(Operation     aOperationType,
                      PRInt32       aItemNum,
                      PRInt32       aItemCount,
                      PRInt32       aItemType,
                      sbIMediaList* aMediaList = nsnull,
                      sbIMediaItem* aMediaItem = nsnull,
                      PRBool        aNewBatch = PR_TRUE);

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
  typedef sbBaseDevice::TransferRequest TransferRequest;
  static bool IsItemOp(sbDeviceStatusHelper::Operation aOperation) {
    NS_ASSERTION(aOperation != sbDeviceStatusHelper::OPERATION_TYPE_NONE,
                 "sbMSCStatusAutoOperationComplete::isItemOp pass 'none'");
    // Make sure we're updated and know about all the types
    NS_ASSERTION(aOperation == sbDeviceStatusHelper::OPERATION_TYPE_MOUNT ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_WRITE ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_TRANSCODE ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_DELETE ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_READ ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_FORMAT ||
                 aOperation == sbDeviceStatusHelper::OPERATION_TYPE_DOWNLOAD,
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
                                     TransferRequest * aRequest,
                                     PRUint32 aBatchCount);
  sbDeviceStatusAutoOperationComplete(
                                   sbDeviceStatusHelper * aStatus,
                                   sbDeviceStatusHelper::Operation aOperation);
  /**
     * Initialize the request and status, start the operation and
     * setup to auto fail. Must Call SetResult for successful completion
     * This version allows overriding of the batch count
     */
  sbDeviceStatusAutoOperationComplete(
                                     sbDeviceStatusHelper * aStatus,
                                     sbDeviceStatusHelper::Operation aOperation,
                                     TransferRequest * aRequest,
                                     PRInt32 aBatchCount);
  /**
   * If this is the last item in the batch then call the OperationComplete
   * method.
   */
  ~sbDeviceStatusAutoOperationComplete();
  /**
   * Complete the operation
   */
  void Complete();

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
  void Transfer(sbDeviceStatusAutoOperationComplete & aDestination);
private:
  // This will live in the request queue as long as we're alive
  nsRefPtr<TransferRequest> mRequest;
  PRUint32 mBatchCount;
  sbDeviceStatusHelper * mStatus;
  nsresult mResult;
  sbDeviceStatusHelper::Operation mOperation;

  /**
   * Copies the auto complete status
   */
  sbDeviceStatusAutoOperationComplete &
  operator = (sbDeviceStatusAutoOperationComplete const & aOther);

  /**
   * Simple helper function to extract common data from the request object
   */
  nsresult ExtractDataFromRequest(
                                 PRUint32 & aType,
                                 PRInt32 & aBatchIndex,
                                 PRInt32 & aBatchCount,
                                 sbBaseDevice::TransferRequest ** aRequestData);

  // Prevent copying
  sbDeviceStatusAutoOperationComplete(
    sbDeviceStatusAutoOperationComplete const &);
};

#endif /* __SB_MSC_STATUS_H__ */

