/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
 * \file  CDDevice.h
 * \brief Songbird CDDevice Component Definition.
 * \sa    sbIDeviceBase.idl, sbICDDevice.idl, sbICDDevice.h
 */

#ifndef __CD_DEVICE_H__
#define __CD_DEVICE_H__

#include "sbICDDevice.h"
#include "DeviceBase.h"
#include "CDCrossPrlatformDefs.h"

class nsIStringBundle;

// DEFINES ====================================================================
#define SONGBIRD_CDDevice_CONTRACTID                      \
  "@songbirdnest.com/Songbird/Device/CDDevice;1"
#define SONGBIRD_CDDevice_CLASSNAME                       \
  "Songbird CD Device"
#define SONGBIRD_CDDevice_CID                             \
{ /* 23c714a8-dff3-481e-bc4c-61ece265c7f8 */              \
  0x23c714a8,                                             \
  0xdff3,                                                 \
  0x481e,                                                 \
  {0xbc, 0x4c, 0x61, 0xec, 0xe2, 0x65, 0xc7, 0xf8}        \
}
#define CONTEXT_COMPACT_DISC_DEVICE "compactdiscDB-"

// CLASSES ====================================================================

class sbCDDevice :  public sbICDDevice,
                    public sbDeviceBase
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIDEVICEBASE
  NS_DECL_SBICDDEVICE

  sbCDDevice();

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

  virtual PRBool TransferFile(PRUnichar* aDeviceString,
                              PRUnichar* aSource,
                              PRUnichar* aDestination,
                              PRUnichar* aDBContext,
                              PRUnichar* aTable,
                              PRUnichar* aIndex,
                              PRInt32 aCurDownloadRowNumber);

  virtual void TransferComplete(const PRUnichar* aDeviceString);

  virtual PRBool StopCurrentTransfer(const nsAString& aDeviceString);

  virtual PRBool SuspendCurrentTransfer(const nsAString& aDeviceString);

  virtual PRBool ResumeTransfer(const nsAString& aDeviceString);

  virtual PRUint32 GetCurrentTransferRowNumber(const PRUnichar* aDeviceString);

  virtual PRBool IsDeviceIdle(const PRUnichar* aDeviceString) {
    return mCDManagerObject->IsDeviceIdle(aDeviceString);
  }

  virtual PRBool IsDownloadInProgress(const PRUnichar* aDeviceString) {
    return mCDManagerObject->IsDownloadInProgress(aDeviceString);
  }

  virtual PRBool IsUploadInProgress(const PRUnichar* aDeviceString) {
    return mCDManagerObject->IsUploadInProgress(aDeviceString);
  }

  virtual PRBool IsTransferInProgress(const nsAString& aDeviceString) {
    // XXXben Remove me
    nsAutoString str(aDeviceString);
    return (mCDManagerObject->IsDownloadInProgress(str.get()) ||
            mCDManagerObject->IsUploadInProgress(str.get()));
  }

  virtual PRBool IsDownloadPaused(const PRUnichar* aDeviceString) {
    return mCDManagerObject->IsDownloadPaused(aDeviceString);
  }

  virtual PRBool IsUploadPaused(const PRUnichar* aDeviceString) {
    return mCDManagerObject->IsUploadPaused(aDeviceString);
  }

  virtual PRBool IsTransferPaused(const PRUnichar* aDeviceString) {
    return mCDManagerObject->IsTransferPaused(aDeviceString);
  }

  virtual void DeviceIdle(const PRUnichar* aDeviceString){
    mCDManagerObject->SetTransferState(aDeviceString, kSB_DEVICE_STATE_IDLE);
  }

  virtual void DeviceDownloading(const PRUnichar* aDeviceString) {
    mCDManagerObject->SetTransferState(aDeviceString,
                                       kSB_DEVICE_STATE_DOWNLOADING);
  }

  virtual void DeviceUploading(const PRUnichar* aDeviceString) {
    mCDManagerObject->SetTransferState(aDeviceString,
                                       kSB_DEVICE_STATE_UPLOADING);
  }

  virtual void DeviceDownloadPaused(const PRUnichar* aDeviceString) {
    mCDManagerObject->SetTransferState(aDeviceString,
                                       kSB_DEVICE_STATE_DOWNLOAD_PAUSED);
  }

  virtual void DeviceUploadPaused(const PRUnichar* aDeviceString) {
    mCDManagerObject->SetTransferState(aDeviceString,
                                       kSB_DEVICE_STATE_UPLOAD_PAUSED);
  }

  virtual void DeviceDeleting(const PRUnichar* aDeviceString) {
    mCDManagerObject->SetTransferState(aDeviceString,
                                       kSB_DEVICE_STATE_DELETING);
  }

  virtual void DeviceBusy(const PRUnichar* aDeviceString) {
    mCDManagerObject->SetTransferState(aDeviceString, kSB_DEVICE_STATE_BUSY);
  }

private:
  ~sbCDDevice();

  virtual void OnThreadBegin();
  virtual void OnThreadEnd();

  virtual PRBool IsEjectSupported();

  virtual PRBool InitializeSync();
  virtual PRBool FinalizeSync();
  virtual PRBool DeviceEventSync(PRBool aMediaInserted);

  sbCDDeviceManager* mCDManagerObject;
  nsCOMPtr<nsIStringBundle> m_StringBundle;
};

#endif // __CD_DEVICE_H__
