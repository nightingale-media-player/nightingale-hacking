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

#ifndef __SB_IPD_STATUS_H__
#define __SB_IPD_STATUS_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device status services defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbIPDStatus.h
 * \brief Songbird iPod Device Status Definitions.
 */

//------------------------------------------------------------------------------
//
// iPod device status imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbIPDUtils.h"

// Songbird imports.
#include <sbIDeviceStatus.h>
#include <sbBaseDevice.h>

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsStringAPI.h>

// Libgpod imports.
#include <itdb.h>


//------------------------------------------------------------------------------
//
// iPod device status services classes.
//
//------------------------------------------------------------------------------

// Forward declarations
class sbIPDDevice;

/**
 * This class communicates device status to the system.
 */

class sbIPDStatus
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
  //   OPERATION_TYPE_DELETE    Delete operation.
  //

  static const PRUint32         OPERATION_TYPE_NONE = 0;
  static const PRUint32         OPERATION_TYPE_MOUNT = 1;
  static const PRUint32         OPERATION_TYPE_WRITE = 2;
  static const PRUint32         OPERATION_TYPE_DELETE = 3;


  //
  // Constructors/destructors/initializers/finalizers.
  //

  sbIPDStatus(sbIPDDevice* aDevice);

  virtual ~sbIPDStatus();

  nsresult Initialize();

  void Finalize();


  //
  // Operation services.
  //

  void OperationStart(PRUint32                        aOperationType,
                      sbBaseDevice::TransferRequest * aRequest,
                      PRUint32                        aBatchCount);

  void OperationComplete(nsresult aResult);


  //
  // Item services.
  //

  void ItemStart(PRInt32     aItemNum = -1,
                 PRInt32     aItemCount = -1);

  void ItemStart(Itdb_Track* aTrack,
                 PRInt32     aItemNum = -1,
                 PRInt32     aItemCount = -1);

  void ItemStart(Itdb_Playlist* aPlaylist,
                 Itdb_Track*    aTrack,
                 PRInt32        aItemNum = -1,
                 PRInt32        aItemCount = -1);

  void ItemStart(sbIMediaList* aMediaList,
                 sbIMediaItem* aMediaItem,
                 PRInt32       aItemNum = -1,
                 PRInt32       aItemCount = -1);

  void ItemProgress(double aProgress);

  void ItemComplete(nsresult aResult);

  // Get the current sbIDeviceStatus
  nsresult GetCurrentStatus(sbIDeviceStatus * *aCurrentStatus);


  //
  // iPod status services
  //
  nsresult ChangeStatus(PRUint32         newState);
  nsresult UpdateStatus(PRUint32         forState,
                        const nsAString& currentOperation,
                        const nsAString& currentStateMessage,
                        PRInt32          currentIndex,
                        PRInt32          totalCount,
                        double           currentProgress);

  void Idle();
  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Internal services fields.
  //
  //   mDevice                  Device object.
  //   mStatus                  Base device status object.
  //   mOperationType           Current operation type.
  //   mMediaList               Current operation media list.
  //   mMediaItem               Current operation media item.
  //

  sbIPDDevice*                  mDevice;
  nsCOMPtr<sbIDeviceStatus>     mStatus;
  PRUint32                      mOperationType;
  nsCOMPtr<sbIMediaList>        mMediaList;
  nsCOMPtr<sbIMediaItem>        mMediaItem;
};


/**
 * These classes complete the current operation or item when going out of scope.
 * The auto-completion may be prevented by calling the forget method.
 *
 *   sbAutoStatusOperationFailure   Auto-complete operation with
 *                                  NS_ERROR_FAILURE.
 *   sbAutoStatusOperationComplete  Auto-complete operation with the result
 *                                  specified by the second constructor
 *                                  argument.
 *   sbAutoStatusItemFailure        Auto-complete item with NS_ERROR_FAILURE.
 */

SB_AUTO_CLASS(sbAutoStatusOperationFailure,
              sbIPDStatus*,
              mValue,
              mValue->OperationComplete(NS_ERROR_FAILURE),
              mValue = nsnull);
SB_AUTO_CLASS2(sbAutoStatusOperationComplete,
               sbIPDStatus*,
               nsresult,
               mValue,
               mValue->OperationComplete(mValue2),
               mValue = nsnull);
SB_AUTO_CLASS(sbAutoStatusItemFailure,
              sbIPDStatus*,
              mValue,
              mValue->ItemComplete(NS_ERROR_FAILURE),
              mValue = nsnull);


#endif // __SB_IPD_STATUS_H__

