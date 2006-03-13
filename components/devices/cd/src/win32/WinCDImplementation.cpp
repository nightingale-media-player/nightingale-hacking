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

#include "CDDevice.h"
#include "WinCDImplementation.h"
#include "primosdk.h"
#include "nspr.h"

#include <xpcom/nscore.h>
#include <xpcom/nsCOMPtr.h>
#include <necko/nsIURI.h>
#include <necko/nsIFileStreams.h>
#include <xpcom/nsILocalFile.h>
#include <webbrowserpersist/nsIWebBrowserPersist.h>
#include <string/nsStringAPI.h>
#include <string/nsString.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsMemory.h>

#include "IDatabaseQuery.h"
#include "IMediaLibrary.h"
#include "IPlaylist.h"

#define DRIVE_START_LETTER  'A'
#define DRIVE_END_LETTER    'Z'

#define NUM_BYTES_PER_SECTOR 2048

#define DEVICE_STRING_PREFIX NS_LITERAL_STRING("CD (").get()
#define DEVICE_STRING_SUFFIX NS_LITERAL_STRING(":)").get()

#define TRACK_TABLE NS_LITERAL_STRING("CDTracks").get()

WinCDObject::WinCDObject(WinCDObjectManager *parent, char driveLetter):
  mParentCDManager(parent),
  mDriveLetter(driveLetter),
  mNumTracks(0),
  mCDTrackTable(TRACK_TABLE)
{
  mDeviceString = DEVICE_STRING_PREFIX;
  mDeviceString += driveLetter;
  mDeviceString += DEVICE_STRING_SUFFIX;

  mDeviceContext = CONTEXT_COMPACT_DISC_DEVICE;
  mDeviceContext += driveLetter;
}

WinCDObject::~WinCDObject()
{
  PrimoSDK_ReleaseHandle(mSonicHandle);
}

void WinCDObject::Initialize()
{
  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  PrimoSDK_GetHandle(&mSonicHandle);
  if (mSonicHandle)
  {
    // Remove transfer table entries
    mParentCDManager->GetBasesbDevice()->RemoveExistingTransferTableEntries(GetDeviceString().get(), false);

    ReadDriveAttributes();
    ClearCDLibraryData();
    EvaluateCDDrive();
  }
}

void WinCDObject::EvaluateCDDrive()
{
  if (mSonicHandle)
  {
    PRBool mediaChanged;
    ReadDiscAttributes(mediaChanged);

    if (mediaChanged)
      UpdateCDLibraryData();
  }
}

// Clears any previous entries in the database for this CD Drive.
void WinCDObject::ClearCDLibraryData()
{
  nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbird.org/Songbird/DatabaseQuery;1" );
  pQuery->SetAsyncQuery(PR_FALSE);
  pQuery->SetDatabaseGUID(GetDeviceContext().get());

  PRBool retVal;

  nsCOMPtr<sbIPlaylistManager> pPlaylistManager = do_CreateInstance( "@songbird.org/Songbird/PlaylistManager;1" );
  nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbird.org/Songbird/MediaLibrary;1" );

  if(pPlaylistManager.get())
  {
    pPlaylistManager->DeletePlaylist(GetCDTracksTable().get(), pQuery, &retVal);

    if(pLibrary.get())
    {
      pLibrary->SetQueryObject(pQuery.get());
      pLibrary->PurgeDefaultLibrary(PR_FALSE, &retVal);
    }
  }
}

char WinCDObject::GetDriveLetter()
{
  return mDriveLetter;
}

nsString& WinCDObject::GetDeviceString()
{
  return mDeviceString;
}

nsString& WinCDObject::GetDeviceContext()
{
  return mDeviceContext;
}

PRUint32 WinCDObject::GetDriveID()
{
  return 0;
}

PRUint32 WinCDObject::GetDriveSpeed()
{
  return 0;
}

PRBool WinCDObject::SetDriveSpeed(PRUint32 driveSpeed)
{
  return PR_FALSE;
}

PRUint32 WinCDObject::GetUsedDiscSpace()
{
  return mUsedSpace;
}

PRUint32 WinCDObject::GetFreeDiscSpace()
{
  return mFreeSpace;
}


void WinCDObject::ReadDriveAttributes()
{
  DWORD driveReady;
  DWORD driveUnit = mDriveLetter;
  PrimoSDK_UnitInfo(mSonicHandle, &driveUnit, &mDriveType, (BYTE *) &mDriveDescription, &driveReady);

}

PRBool WinCDObject::IsDriveWritable()
{
  return mDriveType&PRIMOSDK_CDRW;
}

PRBool WinCDObject::IsAudioDiscInDrive()
{

  return (mNumTracks > 0);
}

PRBool WinCDObject::IsDiscAvailableForWrite()
{
  return PR_FALSE;
}

nsString& WinCDObject::GetCDTracksTable()
{
  return mCDTrackTable;
}

PRBool WinCDObject::UpdateCDLibraryData()
{
  PRBool bRet = PR_FALSE;
  if (mNumTracks)
  {
    nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbird.org/Songbird/DatabaseQuery;1" );
    pQuery->SetAsyncQuery(PR_FALSE);
    pQuery->SetDatabaseGUID(GetDeviceContext().get());

    nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbird.org/Songbird/MediaLibrary;1" );
    pLibrary->SetQueryObject(pQuery.get());
    pLibrary->CreateDefaultLibrary();

    nsCOMPtr<sbIPlaylistManager> pPlaylistManager = do_CreateInstance( "@songbird.org/Songbird/PlaylistManager;1" );
    pPlaylistManager->CreateDefaultPlaylistManager(pQuery);

    nsCOMPtr<sbIPlaylist> pPlaylist;
    pPlaylistManager->CreatePlaylist(GetCDTracksTable().get(), GetDeviceString().get(), GetDeviceString().get(), nsString(NS_LITERAL_STRING("cd")).get(), pQuery, getter_AddRefs(pPlaylist));

    pQuery->Execute(&bRet);
    pQuery->ResetQuery();

    // Create a table for the tracks
    //mParentCDManager->GetBasesbDevice()->CreateTrackTable(GetDeviceString(), mCDTrackTable);

    // Create record for each track
    for (unsigned int trackNum = 1; trackNum <= mNumTracks; trackNum ++)
    {
      DWORD sessionNumber, trackType, prepGap, start, length;
      PrimoSDK_TrackInfo(mSonicHandle, trackNum, &sessionNumber, &trackType, &prepGap, &start, &length);

      PRUnichar driveLetter = (PRUnichar) mDriveLetter;
      nsString url(driveLetter);
      url += NS_LITERAL_STRING(":\\track0");
      url.AppendInt(trackNum);
      url += NS_LITERAL_STRING(".cda");

      nsString name;
      nsString tim;
      nsString artist;
      nsString album;
      nsString genre;

      const static PRUnichar *aMetaKeys[] = {NS_LITERAL_STRING("length").get(), NS_LITERAL_STRING("title").get()};
      const static PRUint32 nMetaKeyCount = sizeof(aMetaKeys) / sizeof(aMetaKeys[0]);

      PRUnichar *pGUID = nsnull;

      nsString strTitle(NS_LITERAL_STRING("CD Track "));
      strTitle.AppendInt(trackNum);
 
      nsString strLength;
      strLength.AppendInt((unsigned int) length);

      PRUnichar** aMetaValues = (PRUnichar **) nsMemory::Alloc(nMetaKeyCount * sizeof(PRUnichar *));
      aMetaValues[0] = (PRUnichar *) nsMemory::Clone(strLength.get(), (strLength.Length() + 1) * sizeof(PRUnichar));
      aMetaValues[1] = (PRUnichar *) nsMemory::Clone(strTitle.get(), (strTitle.Length() + 1) * sizeof(PRUnichar));

      pLibrary->AddMedia(url.get(), nMetaKeyCount, aMetaKeys, nMetaKeyCount, const_cast<const PRUnichar **>(aMetaValues), PR_TRUE, PR_FALSE, &pGUID);
      // Don't know why (thanks to the in-ability to debug javascript) but sometimes "AddMedia" fails the first time and calling it again succeeds,
      // needs to be investigated.
      if (!pGUID)
        pLibrary->AddMedia(url.get(), nMetaKeyCount, aMetaKeys, nMetaKeyCount, const_cast<const PRUnichar **>(aMetaValues), PR_FALSE, PR_FALSE, &pGUID);

      if(pGUID && pPlaylist.get())
      {
        pPlaylist->AddByGUID(pGUID, GetDeviceContext().get(), -1, PR_FALSE, PR_FALSE, &bRet);
        nsMemory::Free(pGUID);
      }

      nsMemory::Free(aMetaValues[0]);
      nsMemory::Free(aMetaValues[1]);
      nsMemory::Free(aMetaValues);

      //mParentCDManager->GetBasesbDevice()->AddTrack(GetDeviceString(), 
      //                                              mCDTrackTable,
      //                                              url,
      //                                              name,
      //                                              tim,
      //                                              artist,
      //                                              album,
      //                                              genre,
      //                                              length);
    }
    pQuery->Execute(&bRet);

    // Notify listeners of the event
    mParentCDManager->GetBasesbDevice()->DoDeviceConnectCallback(GetDeviceString().get());
  }
  else
  {
    // Remove all the info about this CD
    ClearCDLibraryData();
    mParentCDManager->GetBasesbDevice()->DoDeviceDisconnectCallback(GetDeviceString().get());
  }

  return PR_TRUE;
}

void WinCDObject::ReadDiscAttributes(PRBool& mediaChanged)
{
  mediaChanged = PR_FALSE;
  DWORD driveUnit = mDriveLetter;
  DWORD numTracks, mediumType, mediumFormat, flags;
  numTracks = mediumType = mediumFormat = flags = 0;
  DWORD retVal = PRIMOSDK_OK;
  retVal = PrimoSDK_DiscInfoEx(mSonicHandle, &driveUnit, flags, &mediumType, &mediumFormat, &mMediaErasable, &numTracks, &mUsedSpace, &mFreeSpace);
  if (mNumTracks != numTracks)
  {
    mediaChanged = PR_TRUE;
  }
  mNumTracks = numTracks;
}

PRBool WinCDObject::RipTrack(PRUnichar* source, char* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index)
{
  PRUint32 trackNumber = GetCDTrackNumber(source);
  
  if (trackNumber != -1) // Invalid track number
  {

    DWORD nuumSectorsRead = 0;
    DWORD driveUnit = mDriveLetter;

    PrimoSDK_ExtractAudioTrack(mSonicHandle, &driveUnit, trackNumber, (PBYTE) destination, &nuumSectorsRead);
    //free(destinationAscii);

    RipDBInfo *ripInfo = new RipDBInfo;
    ripInfo->cdObject = this;
    ripInfo->table = table;
    ripInfo->index = index;
    mTimer->InitWithFuncCallback(MyTimerCallbackFunc, ripInfo, 1000, nsITimer::TYPE_REPEATING_SLACK); // Every second

    return true;
  }

  return false;
}

PRUint32 WinCDObject::GetCDTrackNumber(PRUnichar* trackName)
{
  PRUint32 trackNumber = -1;

  // Get the string without extension
  nsString workCopyTrackName(trackName);
  PRInt32 dotPos = workCopyTrackName.Find(NS_L("."));
  if (dotPos < 0)
  {
    return trackNumber;
  }
  PRUnichar* trackNameWithoutExt = (PRUnichar *) workCopyTrackName.get();
  trackNameWithoutExt[dotPos] = 0;

  PRUint32 index = 0;
  while (index < dotPos)
  {
    PRUint32 curPos = dotPos - (index+1);

    PRInt32 errorCode = 0;
    if (trackNameWithoutExt[curPos] >= '0' && trackNameWithoutExt[curPos] <= '9')
      trackNumber = nsString(trackNameWithoutExt + curPos).ToInteger(&errorCode);
    else
      break;

    index ++;
  }

  return trackNumber;
}

void WinCDObject::UpdateIOProgress(RipDBInfo *ripInfo)
{
  DWORD readSectors = 0;
  DWORD totalSectors = 0;
  DWORD status = PrimoSDK_RunningStatus(mSonicHandle, PRIMOSDK_GETSTATUS, &readSectors, &totalSectors);
  if (totalSectors && status == PRIMOSDK_RUNNING)
  {
    PRUint32 progress = (readSectors*100)/totalSectors;
    mParentCDManager->GetBasesbDevice()->UpdateIOProgress((PRUnichar *) GetDeviceString().get(), (PRUnichar *) ripInfo->table.get(), (PRUnichar *) ripInfo->index.get(), progress);
  }
  else
  {
    // Complete the rip progress status to 100%
    if (totalSectors && totalSectors == readSectors)
      mParentCDManager->GetBasesbDevice()->UpdateIOProgress((PRUnichar *) GetDeviceString().get(), (PRUnichar *) ripInfo->table.get(), (PRUnichar *) ripInfo->index.get(), 100);

    mTimer->Cancel();
    mParentCDManager->GetBasesbDevice()->DownloadDone((PRUnichar *) GetDeviceString().get(), (PRUnichar *) ripInfo->table.get(), (PRUnichar *) ripInfo->index.get());
    delete ripInfo;
  }
}

void WinCDObject::MyTimerCallbackFunc(nsITimer *aTimer, void *aClosure)
{
  RipDBInfo *ripInfo = (RipDBInfo *) aClosure;

  ripInfo->cdObject->UpdateIOProgress(ripInfo);
}

WinCDObjectManager::WinCDObjectManager(sbCDDevice* parent):
  mParentDevice(parent)
{
}

WinCDObjectManager::~WinCDObjectManager()
{
}

void WinCDObjectManager::Initialize()
{
  DWORD release;
  PrimoSDK_Init(&release);
  switch (release)
  {
  case PRIMOSDK_CDDVDVERSION:
    break;
  case PRIMOSDK_DEMOVERSION:
    break;
  default:
    // Need to handle this case
    break;
  }

  // Enumerate all drives for CD Drives
  EnumerateDrives();
}

void WinCDObjectManager::Finalize()
{
  // Release the memory by deleting the CD objects
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    delete *iteratorCDObjectects;
  }

  PrimoSDK_End();
}

void WinCDObjectManager::EnumerateDrives()
{
  DWORD sonicHandle;
  PrimoSDK_GetHandle(&sonicHandle);
  for (unsigned char driveLetter = DRIVE_START_LETTER; driveLetter < DRIVE_END_LETTER; driveLetter ++)
  {
    DWORD driveUnit = driveLetter;
    DWORD driveType;
    char unitDescr[50];
    if (PrimoSDK_UnitInfo(sonicHandle, &driveUnit, &driveType, (BYTE *) unitDescr, nsnull) == PRIMOSDK_OK)
    {
      if ( driveType == PRIMOSDK_CDR || driveType == PRIMOSDK_CDRW ||
        driveType == PRIMOSDK_DVDRAM || driveType == PRIMOSDK_DVDR ||
        driveType == PRIMOSDK_DVDRW || driveType == PRIMOSDK_DVDPRW )
      {
        WinCDObject* cdObject = new WinCDObject(this, driveLetter);
        mCDDrives.push_back(cdObject);
        cdObject->Initialize();
      }
    }
  }
  PrimoSDK_ReleaseHandle(sonicHandle);
}

PRUnichar* WinCDObjectManager::GetContext(const PRUnichar* deviceString)
{
  PRUnichar *context = nsnull;
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    if ((*iteratorCDObjectects)->GetDeviceString() == nsString(deviceString))
    {
      size_t nLen = (*iteratorCDObjectects)->GetDeviceContext().Length() + 1;
      context = (PRUnichar *) nsMemory::Clone((*iteratorCDObjectects)->GetDeviceContext().get(), nLen * sizeof(PRUnichar));
    }
  }

  return context;
}

PRUnichar* WinCDObjectManager::EnumDeviceString(PRUint32 index)
{
  PRUnichar* context = nsnull;
  int currentIndex = 0;
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    if (currentIndex ++ == index)
    {
      size_t nLen = (*iteratorCDObjectects)->GetDeviceString().Length() + 1;
      context = (PRUnichar *) nsMemory::Clone((*iteratorCDObjectects)->GetDeviceString().get(), nLen * sizeof(PRUnichar));
    }
  }

  return context;
}

PRBool WinCDObjectManager::IsDownloadSupported(const PRUnichar* deviceString)
{
  // All CD Drives should support uploading, a.k.a ripping
  return PR_FALSE;
}

PRUint32 WinCDObjectManager::GetSupportedFormats(const PRUnichar* deviceString)
{
  return 0;
}

PRBool WinCDObjectManager::IsUploadSupported(const PRUnichar* deviceString)
{
  PRBool retVal = false;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
    retVal = cdObject->IsDriveWritable();

  return retVal;
}

PRBool WinCDObjectManager::IsTransfering(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRBool WinCDObjectManager::IsDeleteSupported(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRUint32 WinCDObjectManager::GetUsedSpace(const PRUnichar* deviceString)
{
  PRUint32 usedSpace = 0;

  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
    usedSpace = cdObject->GetUsedDiscSpace();

  return usedSpace;
}

PRUint32 WinCDObjectManager::GetAvailableSpace(const PRUnichar* deviceString)
{
  return 0;
}

PRBool WinCDObjectManager::GetTrackTable(const PRUnichar* deviceString, PRUnichar** dbContext, PRUnichar** tableName)
{
  *dbContext = nsnull;
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    if ((*iteratorCDObjectects)->GetDeviceString() == nsString(deviceString))
    {
      size_t nLen = (*iteratorCDObjectects)->GetDeviceContext().Length() + 1;
      *dbContext = (PRUnichar *) nsMemory::Clone((*iteratorCDObjectects)->GetDeviceContext().get(), nLen * sizeof(PRUnichar));

      nLen = (*iteratorCDObjectects)->GetCDTracksTable().Length() + 1;
      *tableName = (PRUnichar *) nsMemory::Clone((*iteratorCDObjectects)->GetCDTracksTable().get(), nLen * sizeof(PRUnichar));
    }
  }

  return PR_TRUE;
}

PRBool WinCDObjectManager::EjectDevice(const PRUnichar* deviceString)
{
  return PR_FALSE;
}
 
PRBool WinCDObjectManager::IsUpdateSupported(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRBool WinCDObjectManager::IsEjectSupported(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRUint32 WinCDObjectManager::GetNumDevices()
{
  return mCDDrives.size();
}

PRUint32 WinCDObjectManager::GetDeviceState(const PRUnichar* deviceString)
{
  return 0;
}

PRUnichar* WinCDObjectManager::GetNumDestinations (const PRUnichar* DeviceString)
{
  return nsnull;
}

PRBool WinCDObjectManager::SuspendTransfer()
{
  return PR_FALSE;
}

PRBool WinCDObjectManager::ResumeTransfer()
{
  return PR_FALSE;
}

PRBool WinCDObjectManager::OnCDDriveEvent(PRBool mediaInserted)
{
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    (*iteratorCDObjectects)->EvaluateCDDrive();
  }

  return PR_TRUE;
}

WinCDObject* WinCDObjectManager::GetDeviceMatchingString(const PRUnichar* deviceString)
{
  WinCDObject* retVal = NULL;

  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    if ((*iteratorCDObjectects)->GetDeviceString() == nsString(deviceString))
    {
      retVal = (*iteratorCDObjectects);
      break;
    }
  } 

  return retVal;
}

bool WinCDObjectManager::TransferFile(PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber)
{ 
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    if (mParentDevice->IsDownloadInProgress())
    {
      // rip
      nsString destUTFString((const PRUnichar *) destination);
      nsCString destAsciiString;
      NS_UTF16ToCString(destUTFString, NS_CSTRING_ENCODING_ASCII, destAsciiString);

      cdObject->RipTrack(source, (char *) destAsciiString.get(), dbContext, table, index);
    }
    else
    {
      // burn
    }
  }

  return false;
}
