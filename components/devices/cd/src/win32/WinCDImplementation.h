/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
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

#pragma once

#include "windows.h"
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

  WinCDObject(class PlatformCDObjectManager* parent, char driveLetter);
  ~WinCDObject();

  void        Initialize();

  PRUint32    GetDriveID();
  char        GetDriveLetter();
  PRUint32    GetDriveSpeed();
  PRBool      SetDriveSpeed(PRUint32 driveSpeed);
  PRUint32    GetUsedDiscSpace();
  PRUint32    GetFreeDiscSpace();

  PRBool      IsDriveWritable();

  PRBool      IsAudioDiscInDrive();
  PRBool      IsDiscAvailableForWrite();

  nsString&   GetCDTracksTable();

  nsString&   GetDeviceString();
  nsString&   GetDeviceContext();

  void        EvaluateCDDrive();

  void        UpdateIOProgress(DBInfoForTransfer *ripInfo);

  PRBool      RipTrack(const PRUnichar* source, char* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 );
  void        SetCDRipFormat(PRUint32 format) {} // Not yet implemented
  PRUint32    GetCDRipFormat() { return kSB_DEVICE_FILE_FORMAT_WAV; }  // Not yet implemented so default to wave format
  PRUint32    GetCurrentTransferRowNumber() { return mCurrentTransferRowNumber; }

  PRBool      BurnTrack(const PRUnichar* source, char* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 );
  PRBool      BurnTable(const PRUnichar* table);

  void        StopTransfer();
  void        SetTransferState(PRInt32 newState) { mDeviceState = newState; }
  PRBool      IsDeviceIdle() { return mDeviceState == kSB_DEVICE_STATE_IDLE; }
  PRBool      IsDownloadInProgress() { return mDeviceState == kSB_DEVICE_STATE_DOWNLOADING; }
  PRBool      IsUploadInProgress() { return mDeviceState == kSB_DEVICE_STATE_UPLOADING; }
  PRBool      IsTransferInProgress() { return mDeviceState == kSB_DEVICE_STATE_DOWNLOAD_PAUSED; }
  PRBool      IsDownloadPaused() { return mDeviceState == kSB_DEVICE_STATE_UPLOAD_PAUSED; }
  PRBool      IsUploadPaused() { return mDeviceState == kSB_DEVICE_STATE_DELETING; }
  PRBool      IsTransferPaused() { return mDeviceState == kSB_DEVICE_STATE_BUSY; }
  void        TransferComplete();
  bool        SetGapBurnedTrack(PRUint32 numSeconds);

private:
  WinCDObject() {}
  void      ReadDriveAttributes();
  void      ReadDiscAttributes(PRBool& mediaChanged);
  PRBool    UpdateCDLibraryData();
  void      ClearCDLibraryData();
  PRUint32  GetCDTrackNumber(const PRUnichar* trackName);
  PRBool    SetupCDBurnResources();
  void      ReleaseCDBurnResources();
  PRUint32  GetWeightTrack(nsString& trackPath);

  static void MyTimerCallbackFunc(nsITimer *aTimer, void *aClosure);

  char  mDriveLetter;
  DWORD mSonicHandle;
  DWORD mDriveType;
  char  mDriveDescription[50];

  DWORD mNumTracks;
  DWORD mUsedSpace;
  DWORD mFreeSpace;
  DWORD mMediaErasable;

  PRUint32 mCDRipFileFormat;

  nsString mCDTrackTable;
  nsString mDeviceString;
  nsString mDeviceContext;
  PRUint32 mCurrentTransferRowNumber;
  PRInt32  mDeviceState;
  PRBool   mStopTransfer;
  PRUint32 mBurnGap;
  nsCOMPtr<nsIStringBundle> m_StringBundle;

  nsCOMPtr<nsITimer> mTimer;
  
  struct BurnTrackInfo
  {
    PRUint32 index;
    PRUint32 weight;
  };

  std::list<BurnTrackInfo> mBurnTracksWeight;

  class PlatformCDObjectManager* mParentCDManager;
};

class PlatformCDObjectManager : public sbCDObjectManager
{
public:
  PlatformCDObjectManager(class sbCDDevice* parent);
  ~PlatformCDObjectManager();

  void Initialize();
  void Finalize();

  virtual PRUnichar*  GetContext(const PRUnichar* deviceString);
  virtual PRUnichar*  EnumDeviceString(PRUint32 index);
  virtual PRBool      IsDownloadSupported(const PRUnichar*  deviceString);
  virtual PRUint32    GetSupportedFormats(const PRUnichar*  deviceString);
  virtual PRBool      IsUploadSupported(const PRUnichar*  deviceString);
  virtual PRBool      IsTransfering(const PRUnichar*  deviceString);
  virtual PRBool      IsDeleteSupported(const PRUnichar*  deviceString);
  virtual PRUint32    GetUsedSpace(const PRUnichar*  deviceString);
  virtual PRUint32    GetAvailableSpace(const PRUnichar*  deviceString);
  virtual PRBool      GetTrackTable(const PRUnichar*  deviceString, PRUnichar** dbContext, PRUnichar** tableName);
  virtual PRBool      EjectDevice(const PRUnichar*  deviceString) ;
  virtual PRBool      IsUpdateSupported(const PRUnichar*  deviceString);
  virtual PRBool      IsEjectSupported(const PRUnichar*  deviceString);
  virtual PRUint32    GetNumDevices();
  virtual PRUint32    GetDeviceState(const PRUnichar*  deviceString);
  virtual PRUnichar*  GetNumDestinations (const PRUnichar*  DeviceString);
  virtual PRBool      StopCurrentTransfer(const PRUnichar*  DeviceString);
  virtual PRBool      SuspendTransfer(const PRUnichar*  DeviceString);
  virtual PRBool      ResumeTransfer(const PRUnichar*  DeviceString);
  virtual PRBool      OnCDDriveEvent(PRBool mediaInserted);
  virtual PRBool      SetCDRipFormat(const PRUnichar*  deviceString, PRUint32 format);
  virtual PRUint32    GetCDRipFormat(const PRUnichar*  deviceString);
  virtual PRUint32    GetCurrentTransferRowNumber(const PRUnichar* deviceString);
  virtual PRBool      SetGapBurnedTrack(const PRUnichar* deviceString, PRUint32 numSeconds);
  virtual PRBool      GetWritableCDDrive(PRUnichar **deviceString);

  virtual bool        TransferFile(const PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber);

  virtual void        SetTransferState(const PRUnichar* deviceString, PRInt32 newState);

  virtual PRBool      IsDeviceIdle(const PRUnichar* deviceString);
  virtual PRBool      IsDownloadInProgress(const PRUnichar* deviceString);
  virtual PRBool      IsUploadInProgress(const PRUnichar* deviceString);
  virtual PRBool      IsTransferInProgress(const PRUnichar* deviceString);
  virtual PRBool      IsDownloadPaused(const PRUnichar* deviceString);
  virtual PRBool      IsUploadPaused(const PRUnichar* deviceString);
  virtual PRBool      IsTransferPaused(const PRUnichar* deviceString);
  virtual void        TransferComplete(const PRUnichar* deviceString);
  virtual PRBool      UploadTable(const PRUnichar *DeviceString, const PRUnichar *TableName);

  class sbCDDevice*   GetBasesbDevice() { return mParentDevice; };

private:
  WinCDObject* GetDeviceMatchingString(const PRUnichar* deviceString);
  PlatformCDObjectManager() {}
  void EnumerateDrives();
  void RemoveTransferEntries(const PRUnichar* deviceString);

  std::list<WinCDObject *> mCDDrives;
  class sbCDDevice* mParentDevice;
};
