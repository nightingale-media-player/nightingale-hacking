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

#ifndef _SB_DEVICE_PROGRESS_LISTENER_H_
#define _SB_DEVICE_PROGRESS_LISTENER_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device progress listener defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbDeviceProgressListener.h
 * \brief Songbird Device Progress Listener Definitions.
 */

//------------------------------------------------------------------------------
//
// Device progress listener imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIJobProgress.h>

// Mozilla imports.
#include <prmon.h>
#include <mozilla/Monitor.h>


//------------------------------------------------------------------------------
//
// Device progress listener classes.
//
//------------------------------------------------------------------------------

// Used class declarations.
class sbDeviceStatusHelper;

/**
 * This class implements a job progress listener that will send notification to
 * a monitor upon job completion.
 */

class sbDeviceProgressListener : public sbIJobProgressListener
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
  NS_DECL_SBIJOBPROGRESSLISTENER


  //
  // Public services.
  //

  /**
   * Create a new device progress listener and return it in
   * aDeviceProgressListener.  If aCompleteNotifyMonitor is specified, send
   * notification to it upon job completion.  If aDevicesStatusHelper is
   * specified, use it to update device status.
   *
   * \param aDeviceProgressListener Returned device progress listener.
   * \param aCompleteNotifyMonitor  Monitor to notify upon job completion.
   * \param aDeviceStatusHelper     Helper for updating device status.
   */

  static nsresult
    New(sbDeviceProgressListener** aDeviceProgressListener,
        PRMonitor*                 aCompleteNotifyMonitor = nsnull,
        sbDeviceStatusHelper*      aDeviceStatusHelper = nsnull);


  /**
   * Return true if the job is complete.
   */

  PRBool IsComplete();


  /**
   * Construct a new device progress listener using the job completion
   * notification monitor specified by aCompleteNotifyMonitor and the device
   * status helper specified by aDeviceStatusHelper.
   *
   * \param aCompleteNotifyMonitor  Monitor to notify upon job completion.
   * \param aDeviceStatusHelper     Helper for updating device status.
   */

  sbDeviceProgressListener(PRMonitor* aCompleteNotifyMonitor,
                           sbDeviceStatusHelper* aDeviceStatusHelper);


  /**
   * Destroy a device progress listener.
   */

  virtual ~sbDeviceProgressListener();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mCompleteNotifyMonitor     Monitor to notify upon job completion.
  // mDeviceStatusHelper        Helper to update device status.
  // mIsComplete                True if job has completed.
  //

  PRMonitor*                    mCompleteNotifyMonitor;
  sbDeviceStatusHelper*         mDeviceStatusHelper;
  PRInt32                       mIsComplete;
};


#endif /* _SB_DEVICE_PROGRESS_LISTENER_H_ */

