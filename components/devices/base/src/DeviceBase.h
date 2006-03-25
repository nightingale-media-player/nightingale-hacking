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
 * \file  DeviceBase.h
 * \brief Songbird DeviceBase Component Definition.
 */

#pragma once

#include "nsISupportsImpl.h"
#include "nsISupportsUtils.h"
#include "nsIRDFLiteral.h"
#include "sbIDeviceBase.h"
#include "nsString.h"
#include <nspr/prlock.h>
#include <nspr/prmon.h>
#include <deque>
#include <list>
#include <vector>
#include <map>
#include <nsCOMPtr.h>
#include <nsIThread.h>
#include <nsIRunnable.h>

#define SONGBIRD_DeviceBase_CONTRACTID  "@songbird.org/Songbird/Device/DeviceBase;1"
#define SONGBIRD_DeviceBase_CLASSNAME   "Songbird Device Base"
#define SONGBIRD_DeviceBase_CID { 0x8a4f139a, 0xc504, 0x4470, { 0x8b, 0xb4, 0xa4, 0x3f, 0x33, 0x32, 0x8b, 0x41 } }

#define DEVICE_IDLE                 0
#define DEVICE_BUSY                 1
#define DEVICE_DOWNLOADING          2
#define DEVICE_UPLOADING            3
#define DEVICE_DOWNLOAD_PAUSED      4
#define DEVICE_UPLOAD_PAUSED        5
#define DEVICE_DELETING             6


namespace 
{
    typedef struct
    {
		    nsString            deviceString;
        nsString            dbContext;
        nsString            dbTable;
    } TransferData;
}

    // CLASSES ====================================================================

class sbDeviceBase
{
  friend class sbDeviceThread;

public:
	NS_DECL_SBIDEVICEBASE

	sbDeviceBase(PRBool usingThread = PR_TRUE);
	virtual ~sbDeviceBase();

  PRBool CreateTrackTable(nsString& deviceString, nsString& tableName);
  PRBool AddTrack(nsString& deviceString, nsString& tableName, nsString& url, nsString& name, nsString& tim, nsString& artist, nsString& album, nsString& genre, PRUint32 length);
  void DownloadDone(PRUnichar* deviceString, PRUnichar* table, PRUnichar* index);

  void DoDeviceConnectCallback(const PRUnichar* deviceString);
  void DoDeviceDisconnectCallback(const PRUnichar* deviceString);
  void DoTransferStartCallback(const PRUnichar* sourceURL, const PRUnichar* destinationURL);
  void DoTransferCompleteCallback(const PRUnichar* sourceURL, const PRUnichar* destinationURL, PRInt32 nStatus);
  
  PRBool IsDeviceIdle() { return mDeviceState == DEVICE_IDLE; }
  PRBool IsDownloadInProgress() { return mDeviceState == DEVICE_DOWNLOADING; }
  PRBool IsUploadInProgress() { return mDeviceState == DEVICE_UPLOADING; }
  PRBool IsTransferInProgress() { return (IsDownloadInProgress() || IsUploadInProgress()); }
  PRBool IsDownloadPaused() { return mDeviceState == DEVICE_DOWNLOAD_PAUSED; }
  PRBool IsUploadPaused() { return mDeviceState == DEVICE_UPLOAD_PAUSED; }
  PRBool IsTransferPaused() { return (IsDownloadPaused() || IsUploadPaused()); }

  PRBool UpdateIOProgress(PRUnichar* deviceString, PRUnichar* table, PRUnichar* index, PRUint32 percentComplete);
  PRBool UpdateIOStatus(PRUnichar* deviceString, PRUnichar* table, PRUnichar* index, const PRUnichar* status);
  PRBool TransferNextFile(PRInt32 prevDownloadRowNumber, void *data);
  void RemoveExistingTransferTableEntries(const PRUnichar* DeviceString, PRBool dropTable = false);

protected:

	// Should be overridden in the derived class
  virtual void OnThreadBegin(){}
  virtual void OnThreadEnd(){}

	virtual PRBool TransferFile(PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber){ return false;}
  virtual PRBool StopCurrentTransfer();
  virtual PRBool SuspendCurrentTransfer();
  virtual PRBool ResumeTransfer();

  virtual PRBool InitializeAsync();
  virtual PRBool FinalizeAsync();
  virtual PRBool DeviceEventAsync(PRBool mediaInserted);

  virtual PRBool InitializeSync();
  virtual PRBool FinalizeSync();
  virtual PRBool DeviceEventSync(PRBool mediaInserted);

	PRBool SubmitMessage(PRUint32 message, void* data1, void* data2);
    
  typedef struct
  {
    PRUint32 message;
    void*    data1;
    void*    data2;
  } ThreadMessage;

  PRBool CreateTransferTable(const PRUnichar *DeviceString, const PRUnichar* ContextInput, const PRUnichar* TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **filterValues, const PRUnichar* sourcePath, const PRUnichar* destPath, PRUnichar **TransferTableName);
	void AddQuotedString(nsString &destinationString, const PRUnichar* sourceString, PRBool suffixWithComma = PR_TRUE);
  PRBool GetFileNameFromURL(nsString& url, nsString& fileName);
  nsString GetTransferTable(const PRUnichar* deviceString);
  PRBool StartTransfer(const PRUnichar *deviceString, const PRUnichar *tableName);

  PRBool GetSourceAndDestinationURL(const PRUnichar* dbContext, const PRUnichar* table, const PRUnichar* index, nsString& sourceURL, nsString& destURL);

  // Device state
  void DeviceIdle(){mDeviceState = DEVICE_IDLE; mTransferTrackIndex = -1;}
  void DeviceDownloading() {mDeviceState = DEVICE_DOWNLOADING;}
  void DeviceUploading() {mDeviceState = DEVICE_UPLOADING;}
  void DeviceDownloadPaused() {mDeviceState = DEVICE_DOWNLOAD_PAUSED;}
  void DeviceUploadPaused() {mDeviceState = DEVICE_UPLOAD_PAUSED;}
  void DeviceDeleting() {mDeviceState = DEVICE_DELETING;}
  void DeviceBusy() {mDeviceState = DEVICE_BUSY;}

  void ResumeAbortedTransfer();
  void ResumeAbortedDownload();
  void ResumeAbortedUpload();

  // Transfer related
  virtual nsString GetDeviceDownloadTable(const PRUnichar* deviceString){ return nsString(); }
  virtual nsString GetDeviceUploadTable(const PRUnichar* deviceString){ return nsString(); }
  virtual nsString GetDeviceDownloadTableDescription(const PRUnichar* deviceString){ return nsString(); }
  virtual nsString GetDeviceUploadTableDescription(const PRUnichar* deviceString){ return nsString(); }
  virtual nsString GetDeviceDownloadTableType(const PRUnichar* deviceString){ return nsString(); }
  virtual nsString GetDeviceUploadTableType(const PRUnichar* deviceString){ return nsString(); }
  virtual nsString GetDeviceDownloadReadable(const PRUnichar* deviceString){ return nsString(); }
  virtual nsString GetDeviceUploadTableReadable(const PRUnichar* deviceString){ return nsString(); }

  PRUint32 GetCurrentTransferRowNumber() { return mCurrentTransferRowNumber;  }

  
  static void PR_CALLBACK DeviceProcess(sbDeviceBase* pData);

private:
  PRUint32 mDeviceState;
  PRUint32 mTransferTrackIndex;
  PRInt32 mCurrentTransferRowNumber;

  PRLock* mpCallbackListLock;
  std::vector<sbIDeviceBaseCallback *> mCallbackList;

  PRMonitor* mpDeviceThreadMonitor;
  nsCOMPtr<nsIThread> mpDeviceThread;
  PRBool mDeviceThreadShouldShutdown;
  std::deque<ThreadMessage *> mDeviceMessageQueue;
  PRBool mDeviceQueueHasItem;
};

class sbDeviceThread : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS

  sbDeviceThread(sbDeviceBase* pDevice) : mpDevice(pDevice)
  {
    NS_ASSERTION(mpDevice, "Initializing without a sbDeviceBase");
  }

  NS_IMETHOD Run()
  {
    if (!mpDevice)
      return NS_ERROR_NULL_POINTER;
    sbDeviceBase::DeviceProcess(mpDevice);
    return NS_OK;
  }

private:
  sbDeviceBase* mpDevice;
};
