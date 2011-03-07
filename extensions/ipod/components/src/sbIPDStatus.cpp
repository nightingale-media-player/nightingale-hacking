/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//=BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2011 POTI, Inc.
// http://www.songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
//
// Software distributed under the License is distributed
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
//=END SONGBIRD GPL
*/

//------------------------------------------------------------------------------
//
// iPod device status services.
//
//   These services may only be used within the request lock.
//
//------------------------------------------------------------------------------

/**
 * \file  sbIPDStatus.cpp
 * \brief Songbird iPod Device Status Source.
 */

//------------------------------------------------------------------------------
//
// iPod device status imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbIPDStatus.h"

// Local imports.
#include "sbIPDDevice.h"
#include "sbIPDLog.h"
#include "sbIPDUtils.h"

// Songbird imports.
#include <sbIDeviceEvent.h>
#include "sbRequestItem.h"

#include <sbDebugUtils.h>

static const char* SB_UNUSED_IN_RELEASE(gStateStrings[]) =
{ "IDLE", "SYNCING", "COPYING",
  "DELETING", "UPDATING", "MOUNTING", "DOWNLOADING", "UPLOADING",
  "DOWNLOAD_PAUSED", "UPLOAD_PAUSED", "DISCONNECTED", "BUSY" };
#define STATE_STRING(X) \
  (((X)>=0 && (X)<(sizeof(gStateStrings)/sizeof(gStateStrings[0]))) ? \
   gStateStrings[(X)] : "unknown")


//------------------------------------------------------------------------------
//
// iPod device status Constructors/destructors/initializers/finalizers.
//
//   These services may be used outside of the request lock.
//
//------------------------------------------------------------------------------

/**
 * Construct an iPod device status object.
 *
 * \param aDevice               Device for which to construct status.
 */

sbIPDStatus::sbIPDStatus(sbIPDDevice* aDevice) :
  mDevice(aDevice),
  mOperationType(OPERATION_TYPE_NONE)
{
  // Validate arguments.
  NS_ASSERTION(aDevice, "aDevice is null");

  SB_PRLOG_SETUP(sbIPDStatus);
}


/**
 * Destroy an iPod device status object.
 */

sbIPDStatus::~sbIPDStatus()
{
}


/**
 * Initialize the iPod device status object.
 */

nsresult
sbIPDStatus::Initialize()
{
  nsresult rv;

  // Get the device ID and set it up for auto-disposal.
  nsID* deviceID;
  rv = mDevice->GetId(&deviceID);
  NS_ENSURE_SUCCESS(rv, rv);
  sbAutoNSMemPtr autoDeviceID(deviceID);

  // Create the base device status object.
  char deviceIDString[NSID_LENGTH];
  deviceID->ToProvidedString(deviceIDString);
  mStatus = do_CreateInstance("@songbirdnest.com/Songbird/Device/DeviceStatus;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Initialize the base status object
  char idString[NSID_LENGTH];
  deviceID->ToProvidedString(idString);
  rv = mStatus->Init(NS_ConvertASCIItoUTF16(idString, NSID_LENGTH-1));
  NS_ENSURE_SUCCESS(rv, rv);

  // set the state to idle
  ChangeStatus(sbIDevice::STATE_IDLE);

  return NS_OK;
}


/**
 * Finalize the iPod device status object.
 */

void
sbIPDStatus::Finalize()
{
  // Remove object references.
  mDevice = nsnull;
  mStatus = nsnull;
  mMediaList = nsnull;
  mMediaItem = nsnull;
}


//------------------------------------------------------------------------------
//
// iPod device status operation services.
//
//------------------------------------------------------------------------------

/**
 * Process the start of a new operation of the type specified by aOperationType
 * with the media item specified by aMediaItem within the media list specified
 * by aMediaList.
 *
 * \param aOperationType  Type of operation.
 * \param aRequest        Request being started
 */

void
sbIPDStatus::OperationStart(PRUint32        aOperationType,
                            sbBaseDevice::TransferRequest * aRequest,
                            PRUint32        aBatchCount)
{
  // Validate arguments.
  NS_ENSURE_TRUE(aRequest, /* void */);

  if (aRequest) {
    // Update the current media item and list.
    mMediaList = aRequest->list;
    mMediaItem = aRequest->item;
  }

  const PRInt32 batchIndex = aRequest->GetBatchIndex() + 1;

  // Update the current operation type.
  mOperationType = aOperationType;

  // Dispatch operation dependent status processing.
  switch (mOperationType)
  {
    case OPERATION_TYPE_MOUNT :
      FIELD_LOG(("Starting mount operation.\n"));
      UpdateStatus(sbIDevice::STATE_MOUNTING, NS_LITERAL_STRING("mounting"),
          NS_LITERAL_STRING("mounting"), batchIndex, aBatchCount, 0.0);
      mDevice->CreateAndDispatchEvent
                 (sbIDeviceEvent::EVENT_DEVICE_MOUNTING_START,
                  sbIPDVariant(NS_ISUPPORTS_CAST(sbIDevice*, mDevice)).get());
      break;

    case OPERATION_TYPE_WRITE :
      FIELD_LOG(("Starting write track operation.\n"));
      UpdateStatus(sbIDevice::STATE_COPYING, NS_LITERAL_STRING("copying"),
          NS_LITERAL_STRING("copying"), batchIndex, aBatchCount, 0.0);
      mDevice->CreateAndDispatchEvent
                 (sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_START,
                  sbIPDVariant(mMediaItem).get());
      break;

    case OPERATION_TYPE_DELETE :
      FIELD_LOG(("Starting delete operation.\n"));
      UpdateStatus(sbIDevice::STATE_DELETING, NS_LITERAL_STRING("deleting"),
          NS_LITERAL_STRING("deleting"), batchIndex, aBatchCount, 0.0);
      mDevice->CreateAndDispatchEvent
                 (sbIDeviceEvent::EVENT_DEVICE_TRANSFER_START,
                  sbIPDVariant(mMediaItem).get());
      break;

    default :
      break;
  }
}


/**
 * Process the completion of the current operation with the result specified by
 * aResult.
 *
 * \param aResult               Completion result of operation.
 */

void
sbIPDStatus::OperationComplete(nsresult aResult)
{

  nsString stateMessage;
  // Update device status.
  if (NS_SUCCEEDED(aResult)) {
    stateMessage.AssignLiteral("Completed");
  } else {
    stateMessage.AssignLiteral("Failed");
  }

  // Dispatch operation dependent status processing.
  switch(mOperationType)
  {
    case OPERATION_TYPE_MOUNT :
      FIELD_LOG(("Completed mount operation.\n"));
      UpdateStatus(sbIDevice::STATE_MOUNTING, NS_LITERAL_STRING("mount"), stateMessage,
          0, 0, 100.0);
      mDevice->CreateAndDispatchEvent
                 (sbIDeviceEvent::EVENT_DEVICE_MOUNTING_END,
                  sbIPDVariant(NS_ISUPPORTS_CAST(sbIDevice*, mDevice)).get());
      break;

    case OPERATION_TYPE_WRITE :
      FIELD_LOG(("Completed write track operation.\n"));
      UpdateStatus(sbIDevice::STATE_COPYING, NS_LITERAL_STRING("write"), stateMessage,
          0, 0, 100.0);
      mDevice->CreateAndDispatchEvent
                 (sbIDeviceEvent::EVENT_DEVICE_MEDIA_WRITE_END,
                  sbIPDVariant(mMediaItem).get());
      break;

    case OPERATION_TYPE_DELETE :
      FIELD_LOG(("Completed delete track operation.\n"));
      UpdateStatus(sbIDevice::STATE_DELETING, NS_LITERAL_STRING("delete"), stateMessage,
          0, 0, 100.0);
      mDevice->CreateAndDispatchEvent
                 (sbIDeviceEvent::EVENT_DEVICE_TRANSFER_END,
                  sbIPDVariant(mMediaItem).get());
      break;

    default :
      break;
  }

  // Clear the current operation state.
  mOperationType = OPERATION_TYPE_NONE;
  mMediaList = nsnull;
  mMediaItem = nsnull;
}


//------------------------------------------------------------------------------
//
// iPod device status item services.
//
//------------------------------------------------------------------------------

/**
 * Process the start of item processing of the item with item number specified
 * by aItemNum out of the total item count specified by aItemCount.  Don't
 * update the current item num or total item count if the corresponding
 * arguments are negative.
 *
 * \param aItemNum              Current item number.  Defaults to -1.
 * \param aItemCount            Current total item count.  Defaults to -1.
 */

void
sbIPDStatus::ItemStart(PRInt32     aItemNum,
                       PRInt32     aItemCount)
{
  // Dispatch operation dependent status processing.
  switch(mOperationType)
  {
    case OPERATION_TYPE_MOUNT :
      FIELD_LOG(("Starting mount item.\n"));
      UpdateStatus(sbIDevice::STATE_MOUNTING, NS_LITERAL_STRING("mount"),
          NS_LITERAL_STRING("mount"), aItemNum, aItemCount, 0.0);
      mDevice->CreateAndDispatchEvent
                 (sbIDeviceEvent::EVENT_DEVICE_MOUNTING_PROGRESS,
                  sbIPDVariant(NS_ISUPPORTS_CAST(sbIDevice*, mDevice)).get());
      break;

    case OPERATION_TYPE_WRITE :
      FIELD_LOG(("Starting write track item %d %d.\n", aItemNum, aItemCount));
      /*XXXeps what to do with "Starting" state?*/
      UpdateStatus(sbIDevice::STATE_COPYING, NS_LITERAL_STRING("write"),
          NS_LITERAL_STRING("InProgress"), aItemNum, aItemCount, 0.0);
      mDevice->CreateAndDispatchEvent
                 (sbIDeviceEvent::EVENT_DEVICE_TRANSFER_START,
                  sbIPDVariant(mMediaItem).get());
      break;

    default :
      break;
  }
}


/**
 * Process the start of item processing of the iPod track item specified by
 * aTrack with item number specified by aItemNum out of the total item count
 * specified by aItemCount.  Don't update the current item num or total item
 * count if the corresponding arguments are negative.
 *
 * \param aTrack                iPod track item being started.
 * \param aItemNum              Current item number.  Defaults to -1.
 * \param aItemCount            Current total item count.  Defaults to -1.
 */

void
sbIPDStatus::ItemStart(Itdb_Track* aTrack,
                       PRInt32     aItemNum,
                       PRInt32     aItemCount)
{
  // Validate arguments.
  NS_ENSURE_TRUE(aTrack, /* void */);

  // Process item start status.
  ItemStart(aItemNum, aItemCount);
}


/**
 * Process the start of item processing of the iPod track item specified by
 * aTrack within the iPod playlist specified by aPlaylist and with item number
 * specified by aItemNum out of the total item count specified by aItemCount.
 * Don't update the current item num or total item count if the corresponding
 * arguments are negative.
 *
 * \param aPlaylist             iPod playlist being started.
 * \param aTrack                iPod track item being started.
 * \param aItemNum              Current item number.  Defaults to -1.
 * \param aItemCount            Current total item count.  Defaults to -1.
 */

void
sbIPDStatus::ItemStart(Itdb_Playlist* aPlaylist,
                       Itdb_Track*    aTrack,
                       PRInt32        aItemNum,
                       PRInt32        aItemCount)
{
  // Validate arguments.
  NS_ENSURE_TRUE(aPlaylist, /* void */);
  NS_ENSURE_TRUE(aTrack, /* void */);

  // Process item start status.
  ItemStart(aItemNum, aItemCount);
}


/**
 * Process the start of item processing of the media item specified by
 * aMediaItem within the media list specified by aMediaList with item number
 * specified by aItemNum out of the total item count specified by aItemCount.
 * Don't update the current item num or total item count if the corresponding
 * arguments are negative.
 *
 * \param aMediaList            Media list.
 * \param aMediaItem            Media item being started.
 * \param aItemNum              Current item number.  Defaults to -1.
 * \param aItemCount            Current total item count.  Defaults to -1.
 */

void
sbIPDStatus::ItemStart(sbIMediaList* aMediaList,
                       sbIMediaItem* aMediaItem,
                       PRInt32       aItemNum,
                       PRInt32       aItemCount)
{
  // Validate arguments.
  NS_ENSURE_TRUE(aMediaItem, /* void */);

  // Update the current media item and list.
  mMediaList = aMediaList;
  mMediaItem = aMediaItem;

  // Apply default status processing.
  ItemStart(aItemNum, aItemCount);
}


/**
 * Process the progress of the current item with the progress specified by
 * aProgress.
 *
 * \param aProgress             Progress of current item.
 */

void
sbIPDStatus::ItemProgress(double aProgress)
{
  // Update progress status.
  // XXX should we be calling UpdateStatus?
  mStatus->SetProgress(aProgress);
  mDevice->CreateAndDispatchEvent
             (sbIDeviceEvent::EVENT_DEVICE_TRANSFER_PROGRESS,
              sbIPDVariant(mMediaItem).get());
}


/**
 * Process the completion of the current item with the result specified by
 * aResult.
 *
 * \param aResult               Completion result of item.
 */

void
sbIPDStatus::ItemComplete(nsresult aResult)
{
  // Dispatch operation dependent status processing.
  switch(mOperationType)
  {
    case OPERATION_TYPE_WRITE :
      FIELD_LOG(("Completed write track item.\n"));
      mDevice->CreateAndDispatchEvent
                 (sbIDeviceEvent::EVENT_DEVICE_TRANSFER_END,
                  sbIPDVariant(mMediaItem).get());
      break;

    default :
      break;
  }
}

nsresult
sbIPDStatus::GetCurrentStatus(sbIDeviceStatus * *aCurrentStatus)
{
  NS_ENSURE_ARG(aCurrentStatus);
  NS_IF_ADDREF(*aCurrentStatus = mStatus);
  return NS_OK;
}


nsresult sbIPDStatus::ChangeStatus(PRUint32 newState) {
  PRUint32 oldState;
  nsresult rv;
  rv = mStatus->GetCurrentState(&oldState);
  NS_ENSURE_SUCCESS(rv, rv);

  LOG("sbIPDStatus::ChangeStatus(%s) from %s\n", STATE_STRING(newState), STATE_STRING(oldState));

  PRUint32 currentState = newState;
  PRUint32 currentSubState = sbIDevice::STATE_IDLE;

  // Reset item status
  rv = mStatus->SetMediaItem(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mStatus->SetMediaList(nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  // Check first if we are updating the current or sub states
  if (newState != sbIDevice::STATE_IDLE) {
    // Keep the status from switching rapidly, keep a main status and let the
    // extra stuff be sub status (like copy when syncing or updating while
    // copying)
    if (oldState == sbIDevice::STATE_SYNCING ||
        oldState == sbIDevice::STATE_COPYING ||
        oldState == sbIDevice::STATE_MOUNTING) {
      currentSubState = newState;
      currentState = oldState;
    }
  }

  // Only update if we are actually changing states
  if (currentState != oldState) {
    rv = mStatus->SetCurrentState(currentState);
    NS_ENSURE_SUCCESS(rv, rv);
    mDevice->SetState(currentState);
    // XXX check result?
  }

  LOG("sbIPDStatus::ChangeStatus  to state=%s, substate=%s\n", STATE_STRING(currentState), STATE_STRING(currentSubState));

  // Always update the sub state
  rv = mStatus->SetCurrentSubState(currentSubState);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbIPDStatus::UpdateStatus(PRUint32         forState,
                                   const nsAString& currentOperation,
                                   const nsAString& currentStateMessage,
                                   PRInt32          currentIndex,
                                   PRInt32          totalCount,
                                   double           currentProgress) {
  LOG("sbIPDStatus::UpdateStatus(forState=%s, currentIndex=%d, totalCount=%d)\n", STATE_STRING(forState), currentIndex, totalCount);
  PRUint32 currentState;
  nsresult rv;
  rv = mStatus->GetCurrentState(&currentState);
  NS_ENSURE_SUCCESS(rv, rv);

  // If totalCount is zero that means we have no work item progress to report
  if (totalCount != 0) {
    rv = mStatus->SetWorkItemProgress(currentIndex + 1);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mStatus->SetWorkItemProgressEndCount(totalCount);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = mStatus->SetCurrentOperation(currentOperation);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mStatus->SetStateMessage(currentStateMessage);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mStatus->SetMediaItem(mMediaItem);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mStatus->SetMediaList(mMediaList);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mStatus->SetProgress(currentProgress);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
sbIPDStatus::Idle()
{
  LOG("sbIPDStatus::Idle()\n");

  // set the device status state
  mStatus->SetCurrentState(sbIDevice::STATE_IDLE);

  // set the device status sub-state
  mStatus->SetCurrentSubState(sbIDevice::STATE_IDLE);

  // set the device state
  mDevice->SetState(sbIDevice::STATE_IDLE);
}
