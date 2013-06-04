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

#ifndef _SB_DEVICE_STREAMING_HANDLER_H_
#define _SB_DEVICE_STREAMING_HANDLER_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device streaming handler defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDeviceStreamingHandler.h
 * \brief Songbird Device Streaming handler Definitions.
 */

//------------------------------------------------------------------------------
//
// Device streaming handler imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIMediaItem.h>
#include <sbIMediaItemController.h>

// Mozilla imports.
#include <prmon.h>

//------------------------------------------------------------------------------
//
// Device streaming handler classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements a media item controller listener that will send
 * notification to a monitor upon validate completion.
 */

class sbDeviceStreamingHandler : public sbIMediaItemControllerListener
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Implemented interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAITEMCONTROLLERLISTENER


  //
  // Public services.
  //

  /**
   * Create a new device streaming handler and return it in
   * aDeviceStreamingHandler. If aCompleteNotifyMonitor is specified, send
   * notification to it upon validate completion.
   *
   * \param aDeviceStreamingHandler Returned device streaming handler.
   * \param aMediaItem              Streaming media item.
   * \param aCompleteNotifyMonitor  Monitor to notify upon validate completion.
   */

  static nsresult
    New(sbDeviceStreamingHandler** aDeviceStreamingHandler,
        sbIMediaItem*              aMediaItem,
        PRMonitor*                 aCompleteNotifyMonitor);


  /**
   * Check trigger the check on whether the streaming item can be transferred
   * to the device.
   */

  nsresult CheckTransferable();

  /**
   * Return true if the validation is complete.
   */

  PRBool IsComplete();

  /**
   * Return true if the streaming item is supported.
   */

  PRBool IsStreamingItemSupported();


  /**
   * Construct a new device streaming handler using the validate completion
   * notification monitor specified by aCompleteNotifyMonitor.
   *
   * \param aMediaItem              Streaming media item.
   * \param aCompleteNotifyMonitor  Monitor to notify upon validate completion.
   */

  sbDeviceStreamingHandler(sbIMediaItem *aMediaItem,
                           PRMonitor* aCompleteNotifyMonitor);


  /**
   * Destroy a device streaming handler.
   */

  virtual ~sbDeviceStreamingHandler();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mCompleteNotifyMonitor     Monitor to notify upon validate completion.
  // mIsComplete                True if validation has completed.
  // mIsSupported               True if the streaming item is supported by the
  //                            device.
  //

  PRMonitor*                    mCompleteNotifyMonitor;
  sbIMediaItem*                 mMediaItem;
  PRBool                        mIsComplete;
  PRBool                        mIsSupported;
};


#endif /* _SB_DEVICE_STREAMING_HANDLER_H_ */

