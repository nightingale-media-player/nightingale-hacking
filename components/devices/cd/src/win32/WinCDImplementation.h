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

#ifndef __WIN_CD_IMPLEMENTATION_H__
#define __WIN_CD_IMPLEMENTATION_H__

#include "windows.h"
#include <nsStringGlue.h>
#include "nsISupportsUtils.h"
#include "CDCrossPrlatformDefs.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsIStringBundle.h"

#include <list>

class WinCDObject
{
public:

  struct DBInfoForTransfer
  {
    nsString table;
    nsString index;
    WinCDObject* cdObject;
  };

  WinCDObject(class WinCDDeviceManager* parent, char driveLetter);

  ~WinCDObject();

  void Initialize();

  PRUint32 GetDriveID();

  char GetDriveLetter();

  PRUint32 GetDriveSpeed();

  PRBool SetDriveSpeed(PRUint32 driveSpeed);

  PRUint32 GetUsedDiscSpace();

  PRUint32 GetFreeDiscSpace();

  PRBool IsDriveWritable();

  PRBool IsAudioDiscInDrive();

  PRBool IsDiscAvailableForWrite();

  nsString& GetCDTracksTable();

  nsString& GetDeviceString();

  nsString& GetDeviceContext();

  void EvaluateCDDrive();

  void UpdateIOProgress(DBInfoForTransfer *ripInfo);

  PRBool RipTrack(const PRUnichar* source,
                  char* destination,
                  PRUnichar* dbContext,
                  PRUnichar* table,
                  PRUnichar* index,
                  PRInt32 );

  // Not yet implemented
  void SetCDRipFormat(PRUint32 aFormat) {}

  // Not yet implemented so default to wave format
  PRUint32 GetCDRipFormat() {
    return kSB_DEVICE_FILE_FORMAT_WAV;
  }

  PRUint32 GetCurrentTransferRowNumber() {
    return mCurrentTransferRowNumber;
  }

  PRBool BurnTrack(const PRUnichar* source,
                   char* destination,
                   PRUnichar* dbContext,
                   PRUnichar* table,
                   PRUnichar* index,
                   PRInt32 );

  PRBool BurnTable(const PRUnichar* table);

  void StopTransfer();

  void SetTransferState(PRInt32 newState) {
    mDeviceState = newState;
  }

  PRBool IsDeviceIdle() {
    return mDeviceState == kSB_DEVICE_STATE_IDLE;
  }

  PRBool IsDownloadInProgress() {
    return mDeviceState == kSB_DEVICE_STATE_DOWNLOADING;
  }

  PRBool IsUploadInProgress() {
    return mDeviceState == kSB_DEVICE_STATE_UPLOADING;
  }
  
  PRBool IsTransferInProgress() {
    return mDeviceState == kSB_DEVICE_STATE_DOWNLOAD_PAUSED;
  }

  PRBool IsDownloadPaused() {
    return mDeviceState == kSB_DEVICE_STATE_UPLOAD_PAUSED;
  }

  PRBool IsUploadPaused() {
    return mDeviceState == kSB_DEVICE_STATE_DELETING;
  }

  PRBool IsTransferPaused() {
    return mDeviceState == kSB_DEVICE_STATE_BUSY;
  }

  void TransferComplete();

  PRBool SetGapBurnedTrack(PRUint32 numSeconds);

private:

  WinCDObject() {}

  void ReadDriveAttributes();

  void ReadDiscAttributes(PRBool& mediaChanged);

  PRBool UpdateCDLibraryData();

  void ClearCDLibraryData();

  PRUint32 GetCDTrackNumber(const PRUnichar* trackName);

  PRBool SetupCDBurnResources();

  void ReleaseCDBurnResources();

  PRUint32 GetWeightTrack(nsString& trackPath);

  static void MyTimerCallbackFunc(nsITimer *aTimer,
                                  void *aClosure);

  char mDriveLetter;

  DWORD mSonicHandle;

  DWORD mDriveType;

  char mDriveDescription[50];

  DWORD mNumTracks;

  DWORD mUsedSpace;

  DWORD mFreeSpace;

  DWORD mMediaErasable;

  PRUint32 mCDRipFileFormat;

  nsString mCDTrackTable;

  nsString mDeviceString;

  nsString mDeviceContext;

  PRUint32 mCurrentTransferRowNumber;

  PRInt32 mDeviceState;

  PRBool mStopTransfer;

  PRUint32 mBurnGap;

  nsCOMPtr<nsIStringBundle> m_StringBundle;

  nsCOMPtr<nsITimer> mTimer;
  
  struct BurnTrackInfo
  {
    PRUint32 index;
    PRUint32 weight;
  };

  std::list<BurnTrackInfo> mBurnTracksWeight;

  class WinCDDeviceManager* mParentCDManager;
};

class WinCDDeviceManager : public sbCDDeviceManager
{
public:

  WinCDDeviceManager();

  ~WinCDDeviceManager();

  void Initialize(void* aParent);

  void Finalize();

  virtual void GetContext(const nsAString& aDeviceString,
                          nsAString& _retval);

  virtual void GetDeviceStringByIndex(PRUint32 aIndex,
                                      nsAString& _retval);

  virtual PRBool IsDownloadSupported(const nsAString& aDeviceString);

  virtual PRUint32 GetSupportedFormats(const nsAString& aDeviceString);

  virtual PRBool IsUploadSupported(const nsAString& aDeviceString);

  virtual PRBool IsTransfering(const nsAString& aDeviceString);

  virtual PRBool IsDeleteSupported(const nsAString& aDeviceString);

  virtual PRUint32 GetUsedSpace(const nsAString& aDeviceString);

  virtual PRUint32 GetAvailableSpace(const nsAString& aDeviceString);

  virtual PRBool GetTrackTable(const nsAString& aDeviceString,
                               nsAString& aDBContext,
                               nsAString& aTableName);

  virtual PRBool EjectDevice(const nsAString&  aDeviceString);

  virtual PRBool IsUpdateSupported(const nsAString& aDeviceString);

  virtual PRBool IsEjectSupported(const nsAString& aDeviceString);

  virtual PRUint32 GetDeviceCount();

  virtual PRUint32 GetDeviceState(const nsAString& aDeviceString);

  virtual PRUint32 GetDestinationCount(const nsAString& aDeviceString);

  virtual PRBool StopCurrentTransfer(const nsAString& aDeviceString);

  virtual PRBool SuspendTransfer(const nsAString& aDeviceString);

  virtual PRBool ResumeTransfer(const nsAString& aDeviceString);

  virtual PRBool OnCDDriveEvent(PRBool mediaInserted);

  virtual PRBool SetCDRipFormat(const nsAString& aDeviceString,
                                PRUint32 aFormat);

  virtual PRUint32 GetCDRipFormat(const nsAString& aDeviceString);

  virtual PRUint32 GetCurrentTransferRowNumber(const PRUnichar* deviceString);

  virtual PRBool SetGapBurnedTrack(const PRUnichar* deviceString,
                                   PRUint32 numSeconds);

  virtual PRBool GetWritableCDDrive(nsAString& aDeviceString);

  virtual PRBool TransferFile(const PRUnichar* deviceString,
                              PRUnichar* source,
                              PRUnichar* destination,
                              PRUnichar* dbContext,
                              PRUnichar* table,
                              PRUnichar* index,
                              PRInt32 curDownloadRowNumber);

  virtual void SetTransferState(const PRUnichar* deviceString,
                                PRInt32 newState);

  virtual PRBool IsDeviceIdle(const PRUnichar* deviceString);

  virtual PRBool IsDownloadInProgress(const PRUnichar* deviceString);

  virtual PRBool IsUploadInProgress(const PRUnichar* deviceString);

  virtual PRBool IsTransferInProgress(const nsAString& aDeviceString);

  virtual PRBool IsDownloadPaused(const PRUnichar* deviceString);

  virtual PRBool IsUploadPaused(const PRUnichar* deviceString);

  virtual PRBool IsTransferPaused(const PRUnichar* deviceString);

  virtual void TransferComplete(const PRUnichar* deviceString);

  virtual PRBool UploadTable(const nsAString& aDeviceString,
                             const nsAString& aTableName);

  sbCDDevice* GetBasesbDevice() {
    return mParentDevice;
  }

private:

  WinCDObject* GetDeviceMatchingString(const PRUnichar* deviceString);

  void EnumerateDrives();

  void RemoveTransferEntries(const PRUnichar* deviceString);

  std::list<WinCDObject *> mCDDrives;

  sbCDDevice* mParentDevice;
};

#endif // __WIN_CD_IMPLEMENTATION_H__
