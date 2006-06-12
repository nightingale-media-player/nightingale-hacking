/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
// http://songbirdnest.com
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
// END SONGBIRD GPL
//
 */

/** 
 * \file  DeviceManager.h
 * \brief Songbird WMDevice Component Definition.
 */

#ifndef __DeviceManager_h__
#define __DeviceManager_h__

#include "sbIDeviceManager.h"

#include <nsCOMArray.h>
#include <prlock.h>

// DEFINES ====================================================================
#define SONGBIRD_DEVICEMANAGER_DESCRIPTION                 \
  "Songbird DeviceManager Service"
#define SONGBIRD_DEVICEMANAGER_CONTRACTID                  \
  "@songbird.org/Songbird/DeviceManager;1"
#define SONGBIRD_DEVICEMANAGER_CLASSNAME                   \
  "Songbird Device Manager"
#define SONGBIRD_DEVICEMANAGER_CID                         \
{ /* 4403c45d-0bb8-47cb-ab89-2221f62e5614 */               \
  0x4403c45d,                                              \
  0x0bb8,                                                  \
  0x47cb,                                                  \
  { 0xab, 0x89, 0x22, 0x21, 0xf6, 0x2e, 0x56, 0x14 }       \
}

// CLASSES ====================================================================

class sbDeviceManager : public sbIDeviceManager
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEMANAGER

  sbDeviceManager();

  // Initializes the service. This will fail if we have already been
  // initialized, even thought that should never happen with getService.
  NS_IMETHOD Initialize();

private:
  ~sbDeviceManager();

  NS_IMETHOD GetIndexForCategory(const nsAString& aCategory,
                                 PRUint32* _retval);

  // This is a simple static to make sure that we aren't initialized more than
  // once. Consumers should use getService instead of createInstance, but we do
  // this just in case they forget. 
  static PRBool sServiceInitialized;

  // The lock that protects mSupportedDevices
  PRLock* mLock;

  // An array of the supported devices that have been initialized.
  nsCOMArray<sbIDeviceBase> mSupportedDevices;
};

#endif /* __DeviceManager_h__ */
