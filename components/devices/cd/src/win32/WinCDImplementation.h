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

#include <list>

class WinCDObject
{
public:
  struct RipDBInfo
  {
    nsString table;
    nsString index;
    WinCDObject* cdObject;
  };

  WinCDObject(class WinCDObjectManager* parent, char driveLetter);
  ~WinCDObject();

  void Initialize();

  PRUint32  GetDriveID();
  char      GetDriveLetter();
  PRUint32  GetDriveSpeed();
  PRBool    SetDriveSpeed(PRUint32 driveSpeed);
  PRUint32  GetUsedDiscSpace();
  PRUint32  GetFreeDiscSpace();

  PRBool    IsDriveWritable();

  PRBool    IsAudioDiscInDrive();
  PRBool    IsDiscAvailableForWrite();

  nsString& GetCDTracksTable();

  nsString& GetDeviceString();
  nsString& GetDeviceContext();

  void      EvaluateCDDrive();

  PRBool    RipTrack(PRUnichar* source, char* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index);
  void      UpdateIOProgress(RipDBInfo *ripInfo);


private:
  WinCDObject() {}
  void ReadDriveAttributes();
  void ReadDiscAttributes(PRBool& mediaChanged);
  PRBool UpdateCDLibraryData();
  void ClearCDLibraryData();
  PRUint32 GetCDTrackNumber(PRUnichar* trackName);

  static void MyTimerCallbackFunc(nsITimer *aTimer, void *aClosure);

  char  mDriveLetter;
  DWORD mSonicHandle;
  DWORD mDriveType;
  char  mDriveDescription[50];

  DWORD mNumTracks;
  DWORD mUsedSpace;
  DWORD mFreeSpace;
  DWORD mMediaErasable;

  nsString mCDTrackTable;
  nsString mDeviceString;
  nsString mDeviceContext;
  
  nsCOMPtr<nsITimer> mTimer;

  class WinCDObjectManager* mParentCDManager;
};

class WinCDObjectManager : public sbCDObjectManager
{
public:
  WinCDObjectManager(class sbCDDevice* parent);
  ~WinCDObjectManager();

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
  virtual PRBool      SuspendTransfer();
  virtual PRBool      ResumeTransfer();
  virtual PRBool      OnCDDriveEvent(PRBool mediaInserted);

  virtual bool TransferFile(PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber);

  class sbCDDevice* GetBasesbDevice() { return mParentDevice; };

private:
  WinCDObject* GetDeviceMatchingString(const PRUnichar* deviceString);
  WinCDObjectManager() {}
  void EnumerateDrives();
  std::list<WinCDObject *> mCDDrives;
  class sbCDDevice* mParentDevice;
};
