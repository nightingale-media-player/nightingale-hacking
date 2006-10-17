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

class sbWMDObjectManager
{
public:
  virtual void        GetContext(const nsAString& deviceString, nsAString& _retval) = 0;
  virtual void        GetDeviceStringByIndex(PRUint32 aIndex, nsAString& _retval) = 0;
  virtual PRBool      IsDownloadSupported(const nsAString& deviceString) = 0;
  virtual PRUint32    GetSupportedFormats(const nsAString& deviceString) = 0;
  virtual PRBool      IsUploadSupported(const nsAString&  deviceString) = 0;
  virtual PRBool      IsTransfering(const nsAString&  deviceString) = 0;
  virtual PRBool      IsDeleteSupported(const nsAString&  deviceString) = 0;
  virtual PRUint32    GetUsedSpace(const nsAString&  deviceString) = 0;
  virtual PRUint32    GetAvailableSpace(const nsAString&  deviceString) = 0;
  virtual PRBool      GetTrackTable(const nsAString&  deviceString, nsAString& dbContext, nsAString& tableName) = 0;
  virtual PRBool      EjectDevice(const nsAString&  deviceString) = 0; 
  virtual PRBool      IsUpdateSupported(const nsAString&  deviceString) = 0;
  virtual PRBool      IsEjectSupported(const nsAString&  deviceString) = 0;
  virtual PRUint32    GetNumDevices() = 0;
  virtual PRUint32    GetDeviceState(const nsAString&  deviceString) = 0;
  virtual PRBool      StopCurrentTransfer(const nsAString&  DeviceString) = 0;
  virtual PRBool      SuspendTransfer(const nsAString&  DeviceString) = 0;
  virtual PRBool      ResumeTransfer(const nsAString&  DeviceString) = 0;
  virtual PRUint32    GetCurrentTransferRowNumber(const nsAString& deviceString) = 0;
  virtual PRBool      UploadTable(const nsAString& DeviceString, const nsAString& TableName) = 0;
  virtual void        SetTransferState(const nsAString& deviceString, PRInt32 newState) = 0;
  virtual PRBool      IsDeviceIdle(const nsAString& deviceString) = 0;
  virtual PRBool      IsDownloadInProgress(const nsAString& deviceString) = 0;
  virtual PRBool      IsUploadInProgress(const nsAString& deviceString) = 0;
  virtual PRBool      IsTransferInProgress(const nsAString& deviceString) = 0;
  virtual PRBool      IsDownloadPaused(const nsAString& deviceString) = 0;
  virtual PRBool      IsUploadPaused(const nsAString& deviceString) = 0;
  virtual PRBool      IsTransferPaused(const nsAString& deviceString) = 0;
  virtual void        TransferComplete(const nsAString& deviceString) = 0;
  virtual void        UpdateDatabase(const nsAString& deviceString) = 0;

  virtual ~sbWMDObjectManager(){}

  virtual void Initialize() = 0;
  virtual void Finalize() = 0;
};
