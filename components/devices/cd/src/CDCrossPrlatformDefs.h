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

#ifndef __CD_CROSS_PLATFORM_DEFS_H__
#define __CD_CROSS_PLATFORM_DEFS_H__

class sbCDDeviceManager
{
public:
  virtual PRUnichar*  GetContext(const PRUnichar* deviceString) = 0;
  virtual PRUnichar*  EnumDeviceString(PRUint32 index) = 0;
  virtual PRBool      IsDownloadSupported(const PRUnichar*  deviceString) = 0;
  virtual PRUint32    GetSupportedFormats(const PRUnichar*  deviceString) = 0;
  virtual PRBool      IsUploadSupported(const PRUnichar*  deviceString) = 0;
  virtual PRBool      IsTransfering(const PRUnichar*  deviceString) = 0;
  virtual PRBool      IsDeleteSupported(const PRUnichar*  deviceString) = 0;
  virtual PRUint32    GetUsedSpace(const PRUnichar*  deviceString) = 0;
  virtual PRUint32    GetAvailableSpace(const PRUnichar*  deviceString) = 0;
  virtual PRBool      GetTrackTable(const PRUnichar*  deviceString, PRUnichar** dbContext, PRUnichar** tableName) = 0;
  virtual PRBool      EjectDevice(const PRUnichar*  deviceString) = 0; 
  virtual PRBool      IsUpdateSupported(const PRUnichar*  deviceString) = 0;
  virtual PRBool      IsEjectSupported(const PRUnichar*  deviceString) = 0;
  virtual PRUint32    GetNumDevices() = 0;
  virtual PRUint32    GetDeviceState(const PRUnichar*  deviceString) = 0;
  virtual PRUnichar*  GetNumDestinations (const PRUnichar*  DeviceString) = 0;
  virtual PRBool      StopCurrentTransfer(const PRUnichar*  DeviceString) = 0;
  virtual PRBool      SuspendTransfer(const PRUnichar*  DeviceString) = 0;
  virtual PRBool      ResumeTransfer(const PRUnichar*  DeviceString) = 0;
  virtual PRBool      OnCDDriveEvent(PRBool mediaInserted) = 0;
  virtual PRBool      SetCDRipFormat(const PRUnichar*  DeviceString, PRUint32 format) = 0;
  virtual PRUint32    GetCDRipFormat(const PRUnichar*  DeviceString) = 0;
  virtual PRUint32    GetCurrentTransferRowNumber(const PRUnichar* deviceString) = 0;
  virtual PRBool      SetGapBurnedTrack(const PRUnichar* deviceString, PRUint32 numSeconds) = 0;
  virtual PRBool      GetWritableCDDrive(PRUnichar **deviceString) = 0;
  virtual PRBool      UploadTable(const PRUnichar *DeviceString, const PRUnichar *TableName) = 0;

  virtual void        SetTransferState(const PRUnichar* deviceString, PRInt32 newState) = 0;
  virtual bool        TransferFile(const PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber) = 0;

  virtual PRBool      IsDeviceIdle(const PRUnichar* deviceString) = 0;
  virtual PRBool      IsDownloadInProgress(const PRUnichar* deviceString) = 0;
  virtual PRBool      IsUploadInProgress(const PRUnichar* deviceString) = 0;
  virtual PRBool      IsTransferInProgress(const PRUnichar* deviceString) = 0;
  virtual PRBool      IsDownloadPaused(const PRUnichar* deviceString) = 0;
  virtual PRBool      IsUploadPaused(const PRUnichar* deviceString) = 0;
  virtual PRBool      IsTransferPaused(const PRUnichar* deviceString) = 0;
  virtual void        TransferComplete(const PRUnichar* deviceString) = 0;

  virtual ~sbCDDeviceManager(){}

  virtual void Initialize() = 0;
  virtual void Finalize() = 0;
};

#endif // __CD_CROSS_PLATFORM_DEFS_H__

