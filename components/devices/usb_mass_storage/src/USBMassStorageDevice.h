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

#ifndef __USB_MASS_STORAGE_DEVICE_H__
#define __USB_MASS_STORAGE_DEVICE_H__

/** 
* \file  USBMassStorageDevice.h
* \brief Songbird USBMassStorageDevice Component Definition.
*/

#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIRDFLiteral.h"
#include "sbIUSBMassStorageDevice.h"

#include "DeviceBase.h"

#if defined(XP_WIN)
#include "win32/USBMassStorageDeviceWin32.h"
#else
#include "USBMassStorageDeviceHelper.h"
#endif

#ifndef NS_DECL_ISUPPORTS
#error
#endif

// DEFINES ====================================================================
#define SONGBIRD_USBMassStorageDevice_CONTRACTID  "@songbirdnest.com/Songbird/Device/USBMassStorageDevice;1"
#define SONGBIRD_USBMassStorageDevice_CLASSNAME   "Songbird USBMassStorage Device"
#define SONGBIRD_USBMassStorageDevice_CID { 0xa20bf454, 0x673b, 0x4308, { 0x88, 0x98, 0x93, 0xf2, 0xb0, 0x1, 0xd4, 0xe9 } }

// CLASSES ====================================================================

// Since download device has only one instance, the "Device String" notion does not
// apply to this device and hence ignored in all the functions.
class sbUSBMassStorageDevice : public sbIUSBMassStorageDevice,
                               public sbDeviceBase
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEBASE
  NS_DECL_SBIUSBMASSSTORAGEDEVICE

  sbUSBMassStorageDevice();
  ~sbUSBMassStorageDevice();

private:

  PRBool IsDeviceIdle(const PRUnichar* deviceString) {
    return mDeviceState == kSB_DEVICE_STATE_IDLE;
  }

  PRBool IsDownloadInProgress(const PRUnichar* deviceString) {
    return mDeviceState == kSB_DEVICE_STATE_DOWNLOADING;
  }

  PRBool IsUploadInProgress(const PRUnichar* deviceString) {
    return mDeviceState == kSB_DEVICE_STATE_UPLOADING;
  }
  
  PRBool IsTransferInProgress(const nsAString& aDeviceString) {
    // XXXben Remove me
    nsAutoString str(aDeviceString);
    return (IsDownloadInProgress(str.get()) ||
            IsUploadInProgress(str.get()));
  }

  PRBool IsDownloadPaused(const PRUnichar* deviceString) {
    return mDeviceState == kSB_DEVICE_STATE_DOWNLOAD_PAUSED;
  }

  PRBool IsUploadPaused(const PRUnichar* deviceString) {
    return mDeviceState == kSB_DEVICE_STATE_UPLOAD_PAUSED;
  }

  PRBool IsTransferPaused(const PRUnichar* deviceString) {
    return (IsDownloadPaused(deviceString) || IsUploadPaused(deviceString));
  }

  virtual void OnThreadBegin();

  virtual void OnThreadEnd();

  virtual PRBool TransferFile(PRUnichar* deviceString,
                              PRUnichar* source,
                              PRUnichar* destination,
                              PRUnichar* dbContext,
                              PRUnichar* table,
                              PRUnichar* index,
                              PRInt32 curDownloadRowNumber);

  virtual PRBool StopCurrentTransfer(const nsAString& aDeviceString);

  virtual PRBool SuspendCurrentTransfer(const nsAString& aDeviceString);

  virtual PRBool ResumeTransfer(const nsAString& aDeviceString);

  // Transfer related
  virtual void GetDeviceDownloadTable(const nsAString& aDeviceString,
                                      nsAString& _retval);

  virtual void GetDeviceUploadTable(const nsAString& aDeviceString,
                                    nsAString& _retval);

  virtual void GetDeviceDownloadTableDescription(const nsAString& aDeviceString,
                                                 nsAString& _retval);

  virtual void GetDeviceUploadTableDescription(const nsAString& aDeviceString,
                                               nsAString& _retval);

  virtual void GetDeviceDownloadTableType(const nsAString& aDeviceString,
                                          nsAString& _retval);

  virtual void GetDeviceUploadTableType(const nsAString& aDeviceString,
                                        nsAString& _retval);

  virtual void GetDeviceDownloadReadable(const nsAString& aDeviceString,
                                         nsAString& _retval);

  virtual void GetDeviceUploadTableReadable(const nsAString& aDeviceString,
                                            nsAString& _retval);

  virtual PRUint32 GetCurrentTransferRowNumber(const PRUnichar* deviceString) {
    return mCurrentTransferRowNumber; 
  }

  void SetCurrentTransferRowNumber(PRUint32 rowNumber) {
    mCurrentTransferRowNumber = rowNumber;
  }

  virtual void DeviceIdle(const PRUnichar* deviceString) {
    mDeviceState = kSB_DEVICE_STATE_IDLE;
  }

  virtual void DeviceDownloading(const PRUnichar* deviceString) {
    mDeviceState = kSB_DEVICE_STATE_DOWNLOADING;
  }

  virtual void DeviceUploading(const PRUnichar* deviceString) {
    mDeviceState = kSB_DEVICE_STATE_UPLOADING;
  }

  virtual void DeviceDownloadPaused(const PRUnichar* deviceString) {
    mDeviceState = kSB_DEVICE_STATE_DOWNLOAD_PAUSED;
  }

  virtual void DeviceUploadPaused(const PRUnichar* deviceString) {
    mDeviceState = kSB_DEVICE_STATE_UPLOAD_PAUSED;
  }

  virtual void DeviceDeleting(const PRUnichar* deviceString) {
    mDeviceState = kSB_DEVICE_STATE_DELETING;
  }

  virtual void DeviceBusy(const PRUnichar* deviceString) {
    mDeviceState = kSB_DEVICE_STATE_BUSY;
  }

private:

  typedef std::map<nsString, nsString> devicemap_t;

  PRUint32 mDeviceState;

  PRInt32 mCurrentTransferRowNumber;

  IUSBMassStorageDeviceHelper * m_pHelperImpl;
  
  PRLock *mConnectedDevicesLock;

  devicemap_t mConnectedDevices;
};

#endif // __USB_MASS_STORAGE_DEVICE_H__
