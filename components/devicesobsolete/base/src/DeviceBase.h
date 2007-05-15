/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
* \file  DeviceBase.h
* \brief Songbird DeviceBase Component Definition.
*/

#ifndef __DEVICE_BASE_H__
#define __DEVICE_BASE_H__

#include "sbIDeviceBase.h"

#include <sbILibrary.h>
#include <sbIMediaList.h>
#include <sbIMediaListListener.h>
#include <sbIMediaItem.h>

#include <sbILocalDatabaseLibrary.h>
#include <sbILocalDatabaseSimpleMediaList.h>

#include <nsIMutableArray.h>
#include <nsInterfaceHashtable.h>
#include <nsIThread.h>
#include <nsIRunnable.h>
#include <nsIURI.h>

#include <nsCOMPtr.h>
#include <nsStringGlue.h>

#define SONGBIRD_DeviceBase_CONTRACTID                    \
  "@songbirdnest.com/Songbird/Device/DeviceBase;1"
#define SONGBIRD_DeviceBase_CLASSNAME                     \
  "Songbird Device Base"
#define SONGBIRD_DeviceBase_CID                           \
{ /* 937d0ae7-9ba1-475d-82fd-609ff5f78508 */              \
  0x937d0ae7,                                             \
  0x9ba1,                                                 \
  0x475d,                                                 \
  {0x82, 0xfd, 0x60, 0x9f, 0xf5, 0xf7, 0x85, 0x8}         \
}

// CLASSES ====================================================================

class sbDeviceBaseLibraryListener : public sbIMediaListListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIALISTLISTENER

  sbDeviceBaseLibraryListener();
  virtual ~sbDeviceBaseLibraryListener();

  nsresult Init(const nsAString &aDeviceIdentifier,
                sbIDeviceBase* aDevice);

protected:
  nsCOMPtr<sbIDeviceBase> mDevice;
  nsString mDeviceIdentifier;
      
};

class sbDeviceBaseLibraryCopyListener : public sbILocalDatabaseMediaListCopyListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBILOCALDATABASEMEDIALISTCOPYLISTENER

  sbDeviceBaseLibraryCopyListener();
  virtual ~sbDeviceBaseLibraryCopyListener();

  nsresult Init(const nsAString &aDeviceIdentifier,
                sbIDeviceBase* aDevice);

protected:
  nsCOMPtr<sbIDeviceBase> mDevice;
  nsString mDeviceIdentifier;

};

class sbDeviceBaseTransferListener
{
};

class sbDeviceBase
{
  friend class sbDeviceThread;

public:
  sbDeviceBase();
  virtual ~sbDeviceBase();

  /**
   * \brief Create a library for a device instance.
   *
   * Creating a library provides you with storage for all data relating
   * to media items present on the device. After creating a library you will
   * typically want to register it so that it may be shown to the user.
   * 
   * When a library is created, two listeners are added to it. One listener
   * takes care of advising the sbIDeviceBase interface instance when items
   * need to be transferred to it, deleted from it or updated because data
   * relating to those items have changed.
   *
   * The second listener is responsible for detecting when items are transferred
   * from the devices library to another library.
   * 
   * \param aDeviceIdentifier The device unique identifier.
   * \param aDeviceDatabaseURI Optional URI for the database.
   * \param aDevice The device base interface instance to be used
   * for this device instance.
   * \sa RemoveDeviceLibrary, RegisterDeviceLibrary, UnregisterDeviceLibrary
   */
  nsresult CreateDeviceLibrary(const nsAString &aDeviceIdentifier, 
                               nsIURI *aDeviceDatabaseURI,
                               sbIDeviceBase *aDevice);

  /**
   * \brief Remove a library for a device instance.
   *
   * Remove a library instance. This frees up resources that were used
   * by the device library instance.
   *
   * \param aDeviceIdentifier The device unique identifier.
   */
  nsresult RemoveDeviceLibrary(const nsAString &aDeviceIdentifier);

  /**
   * \brief Get the library instance for a device.
   *
   * Get the library instance for a device. If no library instance
   * is found, NS_ERROR_INVALID_ARG is returned.
   *
   * \param aDeviceIdentifier The device unique identifier.
   * \param aDeviceLibrary Pointer for device library instance.
   */
  nsresult GetLibraryForDevice(const nsAString &aDeviceIdentifier,
                               sbILibrary* *aDeviceLibrary);

  /**
   * \brief Register a device library with the library manager.
   *
   * Registering a device library with the library manager enables the user
   * to view the library. It becomes accessible to others programmatically as well
   * through the library manager.
   *
   * \param aDeviceLibrary The library instance to register.
   */
  nsresult RegisterDeviceLibrary(sbILibrary* aDeviceLibrary);

  /**
   * \brief Unregister a device library with the library manager.
   *
   * Unregister a device library with the library manager will make the library
   * vanish from the list of libraries and block out access to it programatically 
   * as well.
   * 
   * A device should always unregister the device library when the application
   * shuts down, when the device is disconnected suddenly and when the user ejects
   * the device.
   *
   * \param aDeviceLibrary The library instance to unregister.
   */
  nsresult UnregisterDeviceLibrary(sbILibrary* aDeviceLibrary);

  /**
   * \brief Create an internal transfer queue for a device instance.
   *
   * Creating a transfer queue twice for the same device instance will cause
   * the original queue to be overridden.
   *
   * \param aDeviceIdentifier The device unique identifier.
   */
  nsresult CreateTransferQueue(const nsAString &aDeviceIdentifier);

  /**
   * \brief Remove an internal transfer queue for a device instance.
   *
   * This will not cancel pending transfers. This is only meant to free up
   * the resources used by the queue itself. One would most likely call
   * this method at shutdown.
   *
   * \param aDeviceLibrary The library instance to register.
   */
  nsresult RemoveTransferQueue(const nsAString &aDeviceIdentifier);

  /** 
   * \brief Add an item to the internal transfer queue.
   * \param aDeviceIdentifier The device unique identifier.
   * \param aMediaItem The item to add to the transfer queue.
   */
  nsresult AddItemToTransferQueue(const nsAString &aDeviceIdentifier, 
                                  sbIMediaItem* aMediaItem);

  /** 
   * \brief Remove an item from the internal transfer queue.
   * \param aDeviceLibrary The library instance to register.
   * \param aMediaItem
   */
  nsresult RemoveItemFromTransferQueue(const nsAString &aDeviceIdentifier,
                                       sbIMediaItem* aMediaItem);
  /** 
   * \brief Get the next item in the transfer queue.
   *
   * \param aDeviceIdentifier The device unique identifier.
   * \param aMediaItem
   */
  nsresult GetNextItemFromTransferQueue(const nsAString &aDeviceIdentifier,
                                        sbIMediaItem* *aMediaItem);
  /** 
   * \brief Get an item from the transfer queue by index.
   *
   * \param aDeviceIdentifier The device unique identifier.
   * \param aItemIndex
   * \param aMediaItem
   */
  nsresult GetItemByIndexFromTransferQueue(const nsAString &aDeviceIdentifier,
                                           PRUint32 aItemIndex,
                                           sbIMediaItem* *aMediaItem);
  /** 
   * \brief Get the internnal transfer queue for a device instance.
   * \param aDeviceIdentifier The device unique identifier.
   * \param aTransferQueue The device transfer queue.
   */
  nsresult GetTransferQueue(const nsAString &aDeviceIdentifier,
                            nsIMutableArray* *aTransferQueue);
  
  /** 
   * \brief Clear the entire transfer queue. 
   * 
   * Clearing the entire transfer queue does not cause transfers to be cancelled.
   * If you wish to cancel all transfers, you must do so yourself.
   *
   * \param aDeviceIdentifier The device unique identifier.
   */
  nsresult ClearTransferQueue(const nsAString &aDeviceIdentifier);

protected:
  nsInterfaceHashtableMT<nsStringHashKey, sbILibrary> mDeviceLibraries;
  nsInterfaceHashtableMT<nsStringHashKey, nsIMutableArray> mDeviceQueues;
  
};

//class sbDeviceThread : public nsIRunnable
//{
//public:
//  NS_DECL_ISUPPORTS
//
//  sbDeviceThread(sbDeviceBase* pDevice) : mpDevice(pDevice) {
//    NS_ASSERTION(mpDevice, "Initializing without a sbDeviceBase");
//  }
//
//  NS_IMETHOD Run() {
//    if (!mpDevice)
//      return NS_ERROR_NULL_POINTER;
//    sbDeviceBase::DeviceProcess(mpDevice);
//    return NS_OK;
//  }
//
//private:
//  sbDeviceBase* mpDevice;
//};

#endif // __DEVICE_BASE_H__

