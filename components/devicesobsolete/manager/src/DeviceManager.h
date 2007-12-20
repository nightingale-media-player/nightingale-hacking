/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

/** 
 * \file  DeviceManager.h
 * \brief Songbird WMDevice Component Definition.
 */

#ifndef __SB_DEVICEMANAGER_H__
#define __SB_DEVICEMANAGER_H__

#include "sbIDeviceManager.h"

#include <nsCOMArray.h>
#include <nsIObserver.h>
#include <nsStringGlue.h>
#include <prlock.h>

// DEFINES ====================================================================
#define SONGBIRD_DEVICEMANAGER_DESCRIPTION                 \
  "Songbird DeviceManager Service"
#define SONGBIRD_DEVICEMANAGER_CONTRACTID                  \
  "@songbirdnest.com/Songbird/DeviceManager;1"
#define SONGBIRD_DEVICEMANAGER_CLASSNAME                   \
  "Songbird Device Manager"
#define SONGBIRD_DEVICEMANAGER_CID                         \
{ /* d0b017c4-f388-4e78-abf3-5f48ca616a94 */               \
  0xd0b017c4,                                              \
  0xf388,                                                  \
  0x4e78,                                                  \
  { 0xab, 0xf3, 0x5f, 0x48, 0xca, 0x61, 0x6a, 0x94 }       \
}
// CLASSES ====================================================================

class sbDeviceManager : public sbIDeviceManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_SBIDEVICEMANAGER

  sbDeviceManager();

  // Initializes the service. This will fail if we have already been
  // initialized, even thought that should never happen with getService.
  NS_IMETHOD Initialize();

private:
  ~sbDeviceManager();

  // Iterate through the registered contractIDs looking for supported devices.
  NS_IMETHOD LoadSupportedDevices();

  // Finalizes the service and informs all devices that they need to quit.
  NS_IMETHOD Finalize();

  // A helper function to handle has/get device requests
  NS_IMETHOD GetIndexForCategory(const nsAString& aCategory,
                                 PRUint32* _retval);

  // This is a simple static to make sure that we aren't initialized more than
  // once. Consumers should use getService instead of createInstance, but we do
  // this just in case they forget. 
  static PRBool sServiceInitialized;

  // Set after our LoadSupportedDevices has been called. 
  static PRBool sDevicesLoaded;

  // This is a sanity check to make sure that we're finalizing properly
  static PRBool sServiceFinalized;

  // The lock that protects mSupportedDevices
  PRLock* mLock;

  // An array of the supported devices that have been initialized.
  nsCOMArray<sbIDeviceBase> mSupportedDevices;

  // Variables that are set to optimize for the case when consumers call
  // hasDeviceForCategory immediately followed by getDeviceForCategory.
  PRUint32 mLastRequestedIndex;
  nsString mLastRequestedCategory;
};

#endif /* __SB_DEVICEMANAGER_H__ */
