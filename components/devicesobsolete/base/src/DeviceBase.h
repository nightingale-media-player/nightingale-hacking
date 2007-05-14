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
   * \brief 
   */
  nsresult CreateDeviceLibrary(const nsAString &aDeviceIdentifier, 
                               nsIURI *aDeviceDatabaseURI,
                               sbIDeviceBase *aDevice);

  /**
   * \brief 
   */
  nsresult RemoveDeviceLibrary(const nsAString &aDeviceIdentifier);

  /**
   * \brief 
   */
  nsresult GetLibraryForDevice(const nsAString &aDeviceIdentifier,
                               sbILibrary* *aDeviceLibrary);

  /**
   * \brief 
   */
  nsresult RegisterDeviceLibrary(sbILibrary* aDeviceLibrary);

  /**
   * \brief 
   */
  nsresult UnregisterDeviceLibrary(sbILibrary* aDeviceLibrary);

  /**
   * \brief 
   */
  nsresult CreateTransferQueue(const nsAString &aDeviceIdentifier);

  /**
   * \brief
   */
  nsresult RemoveTransferQueue(const nsAString &aDeviceIdentifier);

  /** 
   * \brief
   */
  nsresult AddItemToTransferQueue(const nsAString &aDeviceIdentifier, 
                                  sbIMediaItem* aMediaItem);

  /** 
   * \brief
   */
  nsresult RemoveItemFromTransferQueue(const nsAString &aDeviceIdentifier,
                                       sbIMediaItem* aMediaItem);
  /** 
   * \brief
   */
  nsresult GetNextItemFromTransferQueue(const nsAString &aDeviceIdentifier,
                                        sbIMediaItem* *aMediaItem);
  /** 
   * \brief
   */
  nsresult GetItemByIndexFromTransferQueue(const nsAString &aDeviceIdentifier,
                                           PRUint32 aItemIndex,
                                           sbIMediaItem* *aMediaItem);
  /** 
   * \brief
   */
  nsresult GetTransferQueue(const nsAString &aDeviceIdentifier,
                            nsIMutableArray* *aTransferQueue);
  
  /** 
   * \brief
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

