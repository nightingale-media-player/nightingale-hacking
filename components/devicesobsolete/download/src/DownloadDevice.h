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
* \file  DownloadDevice.h
* \brief Songbird DownloadDevice Component Definition.
*/

#ifndef __DOWNLOAD_DEVICE_H__
#define __DOWNLOAD_DEVICE_H__

#include <nsStringGlue.h>
#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIRDFLiteral.h"
#include "nsIWebProgressListener.h"
#include "sbIDownloadDevice.h"
#include <time.h>
#include "nsNetUtil.h"


#include "DeviceBase.h"

// DEFINES ====================================================================
#define SONGBIRD_DownloadDevice_CONTRACTID                \
  "@songbirdnest.com/Songbird/Device/DownloadDevice;1"
#define SONGBIRD_DownloadDevice_CLASSNAME                 \
  "Songbird Download Device"
#define SONGBIRD_DownloadDevice_CID                       \
{ /* 961da3f4-5ef1-4ad0-818d-622c7bd17447 */              \
  0x961da3f4,                                             \
  0x5ef1,                                                 \
  0x4ad0,                                                 \
  {0x81, 0x8d, 0x62, 0x2c, 0x7b, 0xd1, 0x74, 0x47}        \
}

// CLASSES ====================================================================

class sbDownloadDevice;

class sbDownloadListener : public nsIWebProgressListener
{

public:

  NS_DECL_ISUPPORTS
  NS_DECL_NSIWEBPROGRESSLISTENER

  sbDownloadListener(sbDownloadDevice *downlodingDeviceObject,
                     PRUnichar* deviceString,
                     PRUnichar* table,
                     PRUnichar* index) : 
    mDownlodingDeviceObject(downlodingDeviceObject),
    mDeviceString(deviceString), 
    mTable(table),
    mIndex(index),
    mShutDown(PR_FALSE),
    mPrevProgVal(0),
    mSuspend(PR_FALSE),
    mSavedRequestObject(nsnull)
  {
    mLastDownloadUpdate = time(nsnull);
  }

  ~sbDownloadListener() {}

  void ShutDown() {
    mShutDown = PR_TRUE;
  }

  void Suspend() {
    mSuspend = PR_TRUE;
  }

  PRBool Resume() {
    if (mSuspend && mSavedRequestObject) {
      mSuspend = PR_FALSE;
      mSavedRequestObject->Resume();
      return PR_TRUE;
    }
    return PR_FALSE;
  }

private:

  sbDownloadDevice* mDownlodingDeviceObject;

  nsString mDeviceString; 

  nsString mTable;

  nsString mIndex;

  PRBool mShutDown;

  PRBool mSuspend;

  PRUint32 mPrevProgVal;

  nsIRequest* mSavedRequestObject;

  time_t mLastDownloadUpdate;
};

// Since download device has only one instance, the "Device String" notion does not
// apply to this device and hence ignored in all the functions.
class sbDownloadDevice :  public sbIDownloadDevice, public sbDeviceBase
{

public:

  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEBASE
  NS_DECL_SBIDOWNLOADDEVICE

  sbDownloadDevice();

private:

  friend class sbDownloadListener;

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
    return IsDownloadInProgress(str.get()) || IsUploadInProgress(str.get());
  }

  PRBool IsDownloadPaused(const PRUnichar* deviceString) {
    return mDeviceState == kSB_DEVICE_STATE_DOWNLOAD_PAUSED;
  }

  PRBool IsUploadPaused(const PRUnichar* deviceString) {
    return mDeviceState == kSB_DEVICE_STATE_UPLOAD_PAUSED;
  }

  PRBool IsTransferPaused(const PRUnichar* deviceString) {
    return IsDownloadPaused(deviceString) || IsUploadPaused(deviceString);
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

  void ReleaseListener();

  PRInt32 mCurrentDownloadRowNumber;

  PRBool mDownloading;

  PRUint32 mDeviceState;

  PRInt32 mCurrentTransferRowNumber;

  ~sbDownloadDevice();

  sbDownloadListener* mListener;
};

#endif // __DOWNLOAD_DEVICE_H__
