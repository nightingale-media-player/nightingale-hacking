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
#include "../WMDCrossPlatformDefs.h"
#include "nsCOMPtr.h"
#include "nsITimer.h"
#include "nsIStringBundle.h"
#include "NSString.h"

#include "mswmdm.h"
#include "wmdrmdeviceapp.h"
#include "sac.h"
#include "SCClient.h"

#include <list>

#define MAX_WMD_SUPPORTED 100

class CWMDProgress : public IWMDMProgress
{
public:
  CWMDProgress() 
  {
    mdwRefCount = 0;
  }

  // IUnknown
  ULONG STDMETHODCALLTYPE AddRef();
  ULONG STDMETHODCALLTYPE Release();
  STDMETHOD(QueryInterface)(REFIID riid, void ** ppvobject);


  // IWMDMProgress
  STDMETHOD (Begin)( DWORD dwestimatedTicks );
  STDMETHOD (Progress)( DWORD dwtranspiredTicks );
  STDMETHOD (End)();

  // Overridables
  virtual void    OnBegin(DWORD dwestimatedTicks) = 0;
  virtual PRBool  OnProgress(DWORD dwtranspiredTicks) = 0;
  virtual void    OnEnd() = 0;

private:
  DWORD mdwRefCount;
};

class WMDeviceTrack 
{
public:
  WMDeviceTrack();
  ~WMDeviceTrack();

  PRBool  AttachStorage(IWMDMStorage *piWMDMStorage, class WMDeviceFolder* pParentWMDFolder = NULL, DWORD deviceType = 0);

  PRInt32         mTrackID;
  PRInt32         mStatus;
  PRBool          mSelected;
  nsString        mName;
  nsString        mArtist;
  nsString        mGenre;
  nsString        mAlbum;
  PRUint32        mYear;
  PRUint64        mSize;
  PRInt32         mPlayTime;
  PRUint32        mDiscTrackNumber;

private:
  IWMDMStorage    *mIWMDMStorage;
  static PRUint32 WMDeviceTrack::GetNextTrackID()
  {
    return mNextTrackID ++;
  }
  static PRUint32   mNextTrackID;
};



class WMDeviceFolder
{

public:
  WMDeviceFolder(WMDeviceFolder* pParentWMDFolder);
  ~WMDeviceFolder();

  nsString&               GetName();
  PRBool                  HasSubFolders();
  PRUint32                GetNumSubFolders();
  WMDeviceFolder*         GetFolder(PRUint32 index);
  PRUint32                GetNumTracks();
  WMDeviceTrack*          GetMediaTrack(PRUint32 index);
  WMDeviceTrack*          GetMediaTrackByID(PRUint32 mediaTrackID);
  void                    AddMediaTrack(WMDeviceTrack *pmediaTrack);
  PRBool                  DeleteMediaTrackByTrackID(PRUint32 mediaTrackID);
  PRUint32                GetNumTracksIncSubFolders();
  PRUint32                GetPlayTimeForAllMediaTracks();
  PRUint32                GetTotalMediaPlayTimeForFolder();
  WMDeviceTrack*          GetMediaTrackIncSubFolders(PRUint32 index);
  WMDeviceTrack*          GetMediaTrackByTrackIDIncSubFolders(PRUint32 trackID);
  PRUint64                GetFreeStorageSize();
  PRUint64                GetTotalStorageSize();
  WMDeviceFolder*         GetParentFolder();
  WMDeviceFolder*         GetSubFolderByName(const nsString& folderName, bool bCreate = false);

private:
  WMDeviceFolder() {}

  PRBool          AttachStorage(IWMDMStorage *piWMDMStorage, PRBool brootFolder = false, DWORD deviceType = 0);
  PRBool          AddSubFolder(WMDeviceFolder* subFolder);

  PRInt32         mFolderID;
  nsString        mName;
  IWMDMStorage    *mIWMDMStorage;
  PRInt32         mTotalPlayTimeForAllMediaTracks;
  PRInt32         mTotalPlayTimeForFolder;
  WMDeviceFolder  *mParentWMDFolder;

  std::list<WMDeviceFolder*> mSubFolders;
  std::list<WMDeviceTrack*> mTracks;

  friend class WMDevice;
}; 



class WMDevice
{
public:
  struct DBInfoForTransfer
  {
    nsString table;
    nsString index;
    WMDevice* deviceObject;
  };

  WMDevice(class WMDManager* parent, IWMDMDevice* pIDevice);
  ~WMDevice();

  PRBool      Initialize();
  PRBool      Finalize();

  nsString    GetDeviceName();
  nsString&   GetDeviceString();
  nsString&   GetDeviceContext();

  nsString    GetDeviceCanonincalName();
  PRUint32    GetUsedSpace();
  PRUint32    GetAvailableSpace();

  nsString&   GetDeviceTracksTable();
  nsString&   GetTracksTable();

  void        EvaluateDevice();

  void        UpdateIOProgress(DBInfoForTransfer *ripInfo);

  PRUint32    GetCurrentTransferRowNumber() { return mCurrentTransferRowNumber; }

  PRBool      UploadTrack(const PRUnichar* source, char* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 );
  PRBool      UploadTable(const PRUnichar* table);

  void        StopTransfer();
  void        SuspendTransfer();
  void        ResumeTransfer();
  void        SetTransferState(PRInt32 newState) { mDeviceState = newState; }
  PRBool      IsDeviceIdle() { return mDeviceState == kSB_DEVICE_STATE_IDLE; }
  PRBool      IsDownloadInProgress() { return mDeviceState == kSB_DEVICE_STATE_DOWNLOADING; }
  PRBool      IsUploadInProgress() { return mDeviceState == kSB_DEVICE_STATE_UPLOADING; }
  PRBool      IsTransferInProgress() { return mDeviceState == kSB_DEVICE_STATE_DOWNLOAD_PAUSED; }
  PRBool      IsDownloadPaused() { return mDeviceState == kSB_DEVICE_STATE_UPLOAD_PAUSED; }
  PRBool      IsUploadPaused() { return mDeviceState == kSB_DEVICE_STATE_DELETING; }
  PRBool      IsTransferPaused() { return mDeviceState == kSB_DEVICE_STATE_BUSY; }
  void        TransferComplete();
  PRBool      IsDownloadSupported();
  PRBool      IsEjectSupported(){return PR_FALSE;}
  PRBool      EjectDevice(){return PR_FALSE;}
  PRBool      IsUpdateSupported(){return PR_FALSE;}
  PRUint32    GetDeviceNumber() { return mDeviceNum; }

private:
  WMDevice() {}

  static void MyTimerCallbackFunc(nsITimer *aTimer, void *aClosure);

  static PRUint32 mDeviceNumber;

  DWORD mNumTracks;
  DWORD mUsedSpace;
  DWORD mFreeSpace;

  nsString mDeviceName;
  nsString mDeviceCanonicalName;
  nsString mDeviceTracksTable;
  nsString mDeviceString;
  nsString mDeviceContext;
  PRUint32 mCurrentTransferRowNumber;
  PRInt32  mDeviceState;
  DWORD    mDeviceType;
  PRBool   mStopTransfer;
  nsCOMPtr<nsIStringBundle> mStringBundle;

  nsCOMPtr<nsITimer> mTimer;

  struct BurnTrackInfo
  {
    PRUint32 index;
    PRUint32 weight;
  };

  std::list<BurnTrackInfo> mBurnTracksWeight;

  WMDManager* mParentWMDManager;

private:
  PRBool  IsMediaPlayer();
  PRBool  IsTetheredDownloadCapable();
  PRBool  ReadDeviceAttributes(void);
  PRBool  IsFileFormatAcceptable(const char* fileFormat);
  void    EnumTracks();
  PRBool  UpdateDeviceLibraryData();
  void    ClearLibraryData();

  _WAVEFORMATEX   mFormat;
  WMDMDATETIME    mDateTime;
  IWMDMMetaData*  mMetaData;
  _WAVEFORMATEX*  mWaveFormatEx;
  PRUint32        mNumFormats;
  LPWSTR*         mMimeFormats;
  PRUint32        mNumMimeFormats;
  IWMDMDevice*    mIWMDevice;
  IWMDMStorage*   mIWMRootStorage;
  PRBool          mSupportsTetheredDownload;
  nsString        mManufacturer;
  WMDMID          mSerialNumber;
  DWORD           mType;
  WMDeviceFolder  mRootFolder;
  PRUint32        mDeviceNum;
};

class CNotificationHandler : public IWMDMNotification
{
public:
  CNotificationHandler() { mdwRefCount = 0; }

  // IUnknown
  ULONG STDMETHODCALLTYPE AddRef();
  ULONG STDMETHODCALLTYPE Release();
  STDMETHOD(QueryInterface)(REFIID riid, void ** ppvobject);

  // IWMDMNotification
  STDMETHOD(WMDMMessage)(DWORD dwmessageType, LPCWSTR pcanonicalName);

  // Overridables
  virtual void DeviceConnect(LPCWSTR pwszCanonicalName) = 0;
  virtual void DeviceDisconnect(LPCWSTR pwszCanonicalName) = 0;
  virtual void MediaInsert(LPCWSTR pwszCanonicalName) = 0;
  virtual void MediaRemove(LPCWSTR pwszCanonicalName) = 0;

private:
  DWORD mdwRefCount;
};


class WMDManager : public sbWMDObjectManager, private CNotificationHandler
{
public:
  WMDManager(class sbWMDevice* parent);
  ~WMDManager();

  void Initialize();
  void Finalize();

  virtual void        GetContext(const nsAString& deviceString, nsAString& _retval);
  virtual void        GetDeviceStringByIndex(PRUint32 aIndex, nsAString& _retval);
  virtual PRBool      IsDownloadSupported(const nsAString& deviceString);
  virtual PRUint32    GetSupportedFormats(const nsAString& deviceString);
  virtual PRBool      IsUploadSupported(const nsAString&  deviceString);
  virtual PRBool      IsTransfering(const nsAString&  deviceString);
  virtual PRBool      IsDeleteSupported(const nsAString&  deviceString);
  virtual PRUint32    GetUsedSpace(const nsAString&  deviceString);
  virtual PRUint32    GetAvailableSpace(const nsAString&  deviceString);
  virtual PRBool      GetTrackTable(const nsAString&  deviceString, nsAString& dbContext, nsAString& tableName);
  virtual PRBool      EjectDevice(const nsAString&  deviceString); 
  virtual PRBool      IsUpdateSupported(const nsAString&  deviceString);
  virtual PRBool      IsEjectSupported(const nsAString&  deviceString);
  virtual PRUint32    GetNumDevices();
  virtual PRUint32    GetDeviceState(const nsAString&  deviceString);
  virtual PRBool      StopCurrentTransfer(const nsAString&  DeviceString);
  virtual PRBool      SuspendTransfer(const nsAString&  DeviceString);
  virtual PRBool      ResumeTransfer(const nsAString&  DeviceString);
  virtual PRUint32    GetCurrentTransferRowNumber(const nsAString& deviceString);
  virtual PRBool      UploadTable(const nsAString& DeviceString, const nsAString& TableName);
  virtual void        SetTransferState(const nsAString& deviceString, PRInt32 newState);
  virtual PRBool      IsDeviceIdle(const nsAString& deviceString);
  virtual PRBool      IsDownloadInProgress(const nsAString& deviceString);
  virtual PRBool      IsUploadInProgress(const nsAString& deviceString);
  virtual PRBool      IsTransferInProgress(const nsAString& deviceString);
  virtual PRBool      IsDownloadPaused(const nsAString& deviceString);
  virtual PRBool      IsUploadPaused(const nsAString& deviceString);
  virtual PRBool      IsTransferPaused(const nsAString& deviceString);
  virtual void        TransferComplete(const nsAString& deviceString);

  PRUint32 GetNextAvailableDBNumber();

private:
  WMDevice* GetDeviceMatchingString(const nsAString& deviceString);
  WMDManager() {}
  void EnumerateDevices();
  void RemoveTransferEntries(const PRUnichar* deviceString);

  PRBool AuthenticateWithWMDManager();
  PRBool RegisterWithWMDForNotifications();
  PRBool EnumeratePortablePlayers();
  PRInt32 AddDevice(IWMDMDevice* pIDevice);
  PRInt32 RemoveDevice(LPCWSTR pCanonicalName);
  PRInt32 RemoveDevice(WMDevice *wmdevice);

  // Device notification overrides
  virtual void DeviceConnect(LPCWSTR pcanonicalName);
  virtual void DeviceDisconnect(LPCWSTR pcanonicalName);
  virtual void MediaInsert(LPCWSTR pcanonicalName);
  virtual void MediaRemove(LPCWSTR pcanonicalName);

  std::list<WMDevice *> mDevices;
  class sbWMDevice* mParentDevice;

  IWMDeviceManager  *mWMDevMgr;
  DWORD mdwNotificationCookie;
  CSecureChannelClient* mSAC;
  PRUint32  mDBAvaliable[MAX_WMD_SUPPORTED];
};
