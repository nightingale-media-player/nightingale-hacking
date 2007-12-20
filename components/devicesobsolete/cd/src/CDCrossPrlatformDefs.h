/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#ifndef __CD_CROSS_PLATFORM_DEFS_H__
#define __CD_CROSS_PLATFORM_DEFS_H__

class sbCDDeviceManager
{
public:

  virtual void GetContext(const nsAString& aDeviceString,
                          nsAString& _retval) = 0;

  virtual void GetDeviceStringByIndex(PRUint32 aIndex,
                                      nsAString& _retval) = 0;

  virtual PRBool IsDownloadSupported(const nsAString& aDeviceString) = 0;

  virtual PRUint32 GetSupportedFormats(const nsAString& aDeviceString) = 0;

  virtual PRBool IsUploadSupported(const nsAString& aDeviceString) = 0;

  virtual PRBool IsTransfering(const nsAString& aDeviceString) = 0;

  virtual PRBool IsDeleteSupported(const nsAString& aDeviceString) = 0;

  virtual PRUint32 GetUsedSpace(const nsAString& aDeviceString) = 0;

  virtual PRUint32 GetAvailableSpace(const nsAString& aDeviceString) = 0;

  virtual PRBool GetTrackTable(const nsAString& aDeviceString,
                               nsAString& aDBContext,
                               nsAString& aTableName) = 0;

  virtual PRBool EjectDevice(const nsAString&  aDeviceString) = 0;

  virtual PRBool IsUpdateSupported(const nsAString& aDeviceString) = 0;

  virtual PRBool IsEjectSupported(const nsAString&  aDeviceString) = 0;

  virtual PRUint32 GetDeviceCount() = 0;

  virtual PRUint32 GetDeviceState(const nsAString& aDeviceString) = 0;

  virtual PRUint32 GetDestinationCount(const nsAString& aDeviceString) = 0;

  virtual PRBool StopCurrentTransfer(const nsAString& aDeviceString) = 0;

  virtual PRBool SuspendTransfer(const nsAString& aDeviceString) = 0;

  virtual PRBool ResumeTransfer(const nsAString& aDeviceString) = 0;

  virtual PRBool OnCDDriveEvent(PRBool mediaInserted) = 0;

  virtual PRBool SetCDRipFormat(const nsAString& aDeviceString,
                                PRUint32 aFormat) = 0;

  virtual PRUint32 GetCDRipFormat(const nsAString& aDeviceString) = 0;

  virtual PRUint32 GetCurrentTransferRowNumber(const PRUnichar* deviceString) = 0;

  virtual PRBool SetGapBurnedTrack(const PRUnichar* deviceString,
                                   PRUint32 numSeconds) = 0;

  virtual PRBool GetWritableCDDrive(nsAString& aDeviceString) = 0;

  virtual PRBool UploadTable(const nsAString& aDeviceString,
                             const nsAString& aTableName) = 0;

  virtual void SetTransferState(const PRUnichar* deviceString,
                                PRInt32 newState) = 0;

  virtual PRBool TransferFile(const PRUnichar* deviceString,
                              PRUnichar* source,
                              PRUnichar* destination,
                              PRUnichar* dbContext,
                              PRUnichar* table,
                              PRUnichar* index,
                              PRInt32 curDownloadRowNumber) = 0;

  virtual PRBool IsDeviceIdle(const PRUnichar* deviceString) = 0;

  virtual PRBool IsDownloadInProgress(const PRUnichar* deviceString) = 0;

  virtual PRBool IsUploadInProgress(const PRUnichar* deviceString) = 0;

  virtual PRBool IsTransferInProgress(const nsAString& aDeviceString) = 0;

  virtual PRBool IsDownloadPaused(const PRUnichar* deviceString) = 0;

  virtual PRBool IsUploadPaused(const PRUnichar* deviceString) = 0;

  virtual PRBool IsTransferPaused(const PRUnichar* deviceString) = 0;

  virtual void  TransferComplete(const PRUnichar* deviceString) = 0;

  virtual ~sbCDDeviceManager() {}

  virtual void Initialize(void* parent) = 0;

  virtual void Finalize() = 0;
};

#endif // __CD_CROSS_PLATFORM_DEFS_H__

