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

#ifndef SB_BASE_DEVICE_VOLUME_H_
#define SB_BASE_DEVICE_VOLUME_H_

/**
 * \file  sbBaseDeviceVolume.h
 * \brief Songbird Base Device Volume Definitions.
 */

//------------------------------------------------------------------------------
//
// Base device volume imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbDeviceStatistics.h"

// Songbird imports.
#include <sbIDeviceLibrary.h>

// Mozilla imports.
#include <nsAutoPtr.h>
#include <nsStringGlue.h>


//------------------------------------------------------------------------------
//
// Base device volume services defs.
//
//------------------------------------------------------------------------------

class sbBaseDevice;


//------------------------------------------------------------------------------
//
// Base device volume services classes.
//
//------------------------------------------------------------------------------

/**
 * This structure provides a base representation of a storage volume on a
 * device.
 */

class sbBaseDeviceVolume : public nsISupports
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


  //
  // Public base device volume services.
  //

  /**
   * Create and return in aVolume a new base device volume for the device
   * specified by aDevice.
   *
   * \param aVolume             Returned, created volume.
   * \param aDevice             Device owning the volume.
   */
  static nsresult New(sbBaseDeviceVolume** aVolume,
                      sbBaseDevice*        aDevice);

  /**
   * Destroy a base device volume instance.
   */
  virtual ~sbBaseDeviceVolume();


  //
  // Base device volume getters/setters.
  //

  nsresult GetGUID(nsAString& aGUID);

  nsresult SetGUID(const nsAString& aGUID);

  nsresult GetIsMounted(PRBool* aIsMounted);

  nsresult SetIsMounted(PRBool aIsMounted);

  nsresult GetDeviceLibrary(sbIDeviceLibrary** aDeviceLibrary);

  nsresult SetDeviceLibrary(sbIDeviceLibrary* aDeviceLibrary);

  nsresult GetStatistics(sbDeviceStatistics** aStatistics);


  //----------------------------------------------------------------------------
  //
  // Protected interface.
  //
  //----------------------------------------------------------------------------

protected:

  //
  // Protected base device volume fields.
  //
  //   mVolumeLock              All volume fields must be accessed under this
  //                            lock.
  //

  PRLock*                       mVolumeLock;


  //
  // Protected base device volume services.
  //

  /**
   * Initialize the device volume for the device specified by aDevice.
   *
   * \param aDevice             Device owning the volume.
   */
  virtual nsresult Initialize(sbBaseDevice* aDevice);

  /**
   * Construct a base device volume instance.
   */
  sbBaseDeviceVolume();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Private base device volume fields.
  //
  //   mDevice                  Device to which volume belongs.  Device owns
  //                            volume, so don't hold a device reference.
  //   mGUID                    Persistent unique ID specific to the media
  //                            volume.
  //   mIsMounted               If true, volume has been mounted.
  //   mDeviceLibrary           Device library residing on volume.
  //   mStatistics              Volume device statistics.
  //

  sbBaseDevice*                 mDevice;
  nsString                      mGUID;
  PRBool                        mIsMounted;
  nsCOMPtr<sbIDeviceLibrary>    mDeviceLibrary;
  nsRefPtr<sbDeviceStatistics>  mStatistics;
};


#endif /* SB_BASE_DEVICE_VOLUME_H_ */

