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

#ifndef sbCDDeviceMarshall_h_
#define sbCDDeviceMarshall_h_

#include "sbCDDeviceDefines.h"

#include <sbBaseDeviceMarshall.h>
#include <sbIDeviceRegistrar.h>
#include <sbICDDeviceService.h>

#include <nsIClassInfo.h>
#include <nsStringAPI.h>
#include <nsIWritablePropertyBag.h>
#include <nsInterfaceHashtable.h>
#include <a11yGeneric.h>
#include <mozilla/Monitor.h>
#include <nsIThread.h>


class sbCDDeviceMarshall : public sbBaseDeviceMarshall,
                           public sbICDDeviceListener,
                           public nsIClassInfo
{
public:
  sbCDDeviceMarshall();
  virtual ~sbCDDeviceMarshall();

  NS_DECL_ISUPPORTS
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIDEVICEMARSHALL
  NS_DECL_SBICDDEVICELISTENER

  NS_DECLARE_STATIC_IID_ACCESSOR(SB_CDDEVICE_MARSHALL_IID)

  nsresult Init();

protected:
  //
  // @brief Add and register a device to the list of known devices.
  //
  nsresult AddDevice(sbICDDevice *aCDDevice);
  nsresult AddDevice2(nsAString const & aName, sbIDevice *aDevice);

  //
  // @brief Remove a device from the list of known devices.
  //
  nsresult RemoveDevice(nsAString const & aName);

  //
  // @brief Get a device from a given device ID.
  //
  nsresult GetDevice(nsAString const & aName, sbIDevice **aOutDevice);

  //
  // @brief Find out if a device is already in the device hash.
  //
  nsresult GetHasDevice(nsAString const & aName, PRBool *aOutHasDevice);

  //
  // @brief Find and add all the devices that have media already.
  //
  nsresult DiscoverDevices();

  //
  // @brief Event dispatching utility method
  //
  nsresult CreateAndDispatchDeviceManagerEvent(PRUint32 aType,
                                               nsIVariant *aData = nsnull,
                                               nsISupports *aOrigin = nsnull,
                                               PRBool aAsync = PR_FALSE);

  //
  // @brief Internal method for discovering plugged in devices on a
  //        background thread.
  //
  void RunDiscoverDevices();

  //
  // @brief Internal method for sending the device start scan event.
  //
  void RunNotifyDeviceStartScan();

  //
  // @brief Internal method for sending the device stop scan event.
  //
  void RunNotifyDeviceStopScan();

private:
  nsInterfaceHashtableMT<nsStringHashKey, nsISupports> mKnownDevices;
  mozilla::Monitor                                     mKnownDevicesLock;

  // The CD device service to use
  nsCOMPtr<sbICDDeviceService> mCDDeviceService;

  // Threading
  nsCOMPtr<nsIThread> mOwnerContextThread;

  // Prevent copying and assignment
  sbCDDeviceMarshall(sbCDDeviceMarshall const &);
  sbCDDeviceMarshall & operator= (sbCDDeviceMarshall const &);

protected:
  NS_DECL_RUNNABLEMETHOD(sbCDDeviceMarshall, RunDiscoverDevices);
  NS_DECL_RUNNABLEMETHOD(sbCDDeviceMarshall, RunNotifyDeviceStartScan);
  NS_DECL_RUNNABLEMETHOD(sbCDDeviceMarshall, RunNotifyDeviceStopScan);
};

#endif  // sbCDDeviceMarshall_h_

