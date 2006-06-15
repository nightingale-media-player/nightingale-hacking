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
// GNU General Public License Version 2 (the ?GPL?).
// 
// Software distributed under the License is distributed 
// on an ?AS IS? basis, WITHOUT WARRANTY OF ANY KIND, either 
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

#define DEVICE_STRING        NS_LITERAL_STRING("compactdisc.name").get()
#define DEVICE_STRING_PREFIX NS_LITERAL_STRING(" (").get()
#define DEVICE_STRING_SUFFIX NS_LITERAL_STRING(":)").get()

#define TRACK_TABLE NS_LITERAL_STRING("CDTracks").get()

WinCDObject::WinCDObject(PlatformCDObjectManager *parent, char driveLetter):
  mParentCDManager(parent),
  mDriveLetter(driveLetter),
  mNumTracks(0),
  mCDTrackTable(TRACK_TABLE),
  mCurrentTransferRowNumber(-1),
  mDeviceState(kSB_DEVICE_STATE_IDLE),
  mStopTransfer(PR_FALSE),
  mBurnGap(2), // Defaulting to 2 seconds
  mCDRipFileFormat(kSB_DEVICE_FILE_FORMAT_WAV) // Default rip to wave file
{
  nsresult rv = NS_OK;
  // Get the string bundle for our strings
  if ( ! m_StringBundle.get() )
  {
    nsIStringBundleService *  StringBundleService = nsnull;
    rv = CallGetService("@mozilla.org/intl/stringbundle;1", &StringBundleService );
    if ( NS_SUCCEEDED(rv) )
    {
      rv = StringBundleService->CreateBundle( "chrome://songbird/locale/songbird.properties", getter_AddRefs( m_StringBundle ) );
      //        StringBundleService->Release();
    }
  }

  PRUnichar *value = nsnull;

  m_StringBundle->GetStringFromName(DEVICE_STRING, &value);
  mDeviceString = value;
  PR_Free(value);

  mDeviceString += DEVICE_STRING_PREFIX;
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
  return (mFreeSpace > 0);
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
      DWORD sessionNumber, trackType, prepGap, start, lengthSectors;
      PrimoSDK_TrackInfo(mSonicHandle, trackNum, &sessionNumber, &trackType, &prepGap, &start, &lengthSectors);
      // These magic numbers are based on,
      // an audio CD sector size = 2352
      // and audio CD encoded at 44.1KHz, Stereo, with 16-bit/sample
      DWORD numSeconds = (lengthSectors*2352)/(2*2*44100);

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

      PRUnichar formattedLength[10];
      wsprintf(formattedLength, L"%d:%02d", numSeconds/60, numSeconds%60);
      nsString strLength(formattedLength);

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

PRBool WinCDObject::RipTrack(const PRUnichar* source, char* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber)
{
  PRUint32 trackNumber = GetCDTrackNumber(source);

  if (trackNumber != -1) // Invalid track number
  {
    mStopTransfer = PR_FALSE;

    mCurrentTransferRowNumber = curDownloadRowNumber;
    DWORD nuumSectorsRead = 0;
    DWORD driveUnit = mDriveLetter;

    DWORD status = PrimoSDK_ExtractAudioTrack(mSonicHandle, &driveUnit, trackNumber, (PBYTE) destination, &nuumSectorsRead);

    if (status == PRIMOSDK_OK)
    {
      //free(destinationAscii);

      DBInfoForTransfer *ripInfo = new DBInfoForTransfer;
      ripInfo->cdObject = this;
      ripInfo->table = table;
      ripInfo->index = index;
      mTimer->InitWithFuncCallback(MyTimerCallbackFunc, ripInfo, 1000, nsITimer::TYPE_REPEATING_SLACK); // Every second

      return true;
    }

  }

  return false;
}

PRBool WinCDObject::BurnTrack(const PRUnichar* source, char* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber)
{
  if (SetupCDBurnResources())
  {
    DWORD driveUnit = mDriveLetter;
    DWORD burnRetVal = PrimoSDK_WriteAudioTrack(mSonicHandle, &driveUnit, (BYTE*) destination, PRIMOSDK_WRITE, PRIMOSDK_BEST);
    if (PRIMOSDK_OK == burnRetVal)
    {
      DBInfoForTransfer *burnInfo = new DBInfoForTransfer;
      burnInfo->cdObject = this;
      burnInfo->table = table;
      burnInfo->index = index;
      mTimer->Cancel();
      mTimer->InitWithFuncCallback(MyTimerCallbackFunc, burnInfo, 1000, nsITimer::TYPE_REPEATING_SLACK); // Every second
      
      return true;
    }
    else
    {
      // Unable to start CD Burn
      // Release the resources
      ReleaseCDBurnResources();
    }

  }

  return false;
}

PRBool WinCDObject::SetupCDBurnResources()
{
  if (!IsDiscAvailableForWrite())
    return PR_FALSE;

  // Lock the drive
  DWORD   dwReply;
  DWORD   driveUnit = mDriveLetter;

  dwReply = PrimoSDK_UnitVxBlock(mSonicHandle, &driveUnit, PRIMOSDK_LOCK, (BYTE*) "Songbird");
  char msg[232];
  sprintf(msg, "%c", mDriveLetter);
  if (dwReply == PRIMOSDK_OK) // DRIVE IS AVAILABLE
  {
    // CALL VXBLOCK TO RESERVE THE DRIVE(S).
    dwReply = PrimoSDK_UnitAIN(mSonicHandle, &driveUnit, PRIMOSDK_LOCK);
    if (dwReply == PRIMOSDK_OK)
    {
      dwReply = PrimoSDK_UnitLock(mSonicHandle, &driveUnit, PRIMOSDK_LOCK);
    }
  }

  if (dwReply == PRIMOSDK_OK)
    return PR_TRUE;
  else
    return PR_FALSE;
}

PRUint32 WinCDObject::GetCDTrackNumber(const PRUnichar* trackName)
{
  PRUint32 trackNumber = -1;

  // Get the string without extension
  nsString workCopyTrackName(trackName);
  PRUint32 dotPos = workCopyTrackName.Find(NS_L("."));
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

void WinCDObject::UpdateIOProgress(DBInfoForTransfer *ripInfo)
{
  PRBool bRipping = mParentCDManager->GetBasesbDevice()->IsDownloadInProgress(GetDeviceString().get());
  DWORD readSectors = 0;
  DWORD totalSectors = 0;
  DWORD status = PrimoSDK_RunningStatus(mSonicHandle, PRIMOSDK_GETSTATUS, &readSectors, &totalSectors);
  if (status != PRIMOSDK_RUNNING)
  {
    return;
  }

  PRUint32 progress = 0;
  if (totalSectors)
    progress = (readSectors*100)/totalSectors;
  
  nsString trackIndex; 
  if (bRipping == PR_TRUE)
  {
    trackIndex = ripInfo->index;
  }
  else
  {
    // For CD burn the track index has to be obtained based on 
    // where we are at, in regards to the total burn.
    for (std::list<BurnTrackInfo>::iterator burnIterator = mBurnTracksWeight.begin(); burnIterator != mBurnTracksWeight.end(); burnIterator ++)
    {
      if ((*burnIterator).weight >= progress)
      {
        // Currently burning this track to CD
        trackIndex = (*burnIterator).index;
        // Adjust the progress
        if (progress > 0)
          progress = (100 * (*burnIterator).weight) / progress;
        break;
      }
    }
  }

  if (( status == PRIMOSDK_RUNNING) && (mStopTransfer == PR_FALSE))
  { 
    if (totalSectors) // Checking for divide by 0 condition
    {
      mParentCDManager->GetBasesbDevice()->UpdateIOProgress((PRUnichar *) GetDeviceString().get(), (PRUnichar *) ripInfo->table.get(), (PRUnichar *) trackIndex.get(), progress);
    }
  }
  else
  {
    if (mStopTransfer)
    {
      // Abort CD Rip operation
      status = PrimoSDK_RunningStatus(mSonicHandle, PRIMOSDK_ABORT, &readSectors, &totalSectors);
      mStopTransfer = PR_FALSE;
    }
    else
    {
      // Complete the rip progress status to 100%
      if (totalSectors && totalSectors == readSectors)
      {
        mParentCDManager->GetBasesbDevice()->UpdateIOProgress((PRUnichar *) GetDeviceString().get(), (PRUnichar *) ripInfo->table.get(), (PRUnichar *) trackIndex.get(), 100);
        mParentCDManager->GetBasesbDevice()->DownloadDone((PRUnichar *) GetDeviceString().get(), (PRUnichar *) ripInfo->table.get(), (PRUnichar *) trackIndex.get());
      }
    }
  }

  if (status != PRIMOSDK_RUNNING)
  {
    if (mStopTransfer || !mParentCDManager->GetBasesbDevice()->IsTransferInProgress(mDeviceString.get()))
    {
      if (mStopTransfer)
        // Since the transfer stopped reset the progress
        mParentCDManager->GetBasesbDevice()->UpdateIOProgress((PRUnichar *) GetDeviceString().get(), (PRUnichar *) ripInfo->table.get(), (PRUnichar *) ripInfo->index.get(), 0);
     
      mTimer->Cancel();
    }

    delete ripInfo;
  }
}

void WinCDObject::MyTimerCallbackFunc(nsITimer *aTimer, void *aClosure)
{
  DBInfoForTransfer *transferInfo = (DBInfoForTransfer *) aClosure;

  transferInfo->cdObject->UpdateIOProgress(transferInfo);
}

void WinCDObject::StopTransfer()
{
  mStopTransfer = PR_TRUE; // Will be handled later in the timer function
}

PRUint32 WinCDObject::GetWeightTrack(nsString& trackPath)
{
  PRUint32 trackSize = 0;
  FILE* hFile;
  hFile = _wfopen(trackPath.get(), L"rb");
  if (hFile)
  {
    // Get file size
    fseek(hFile, 0, SEEK_END);
    trackSize = ftell(hFile);
  }
  return trackSize;
}

PRBool WinCDObject::BurnTable(const PRUnichar* table)
{
  if (!SetupCDBurnResources())
    return PR_FALSE;

  DWORD dwUnits[16];
  dwUnits[0] = 0x01000044;
  dwUnits[1] = 0xFFFFFFFF;
  DWORD dwReply = PrimoSDK_NewAudio(mSonicHandle, dwUnits);
  
  PRUint32 totalBurnWeight = 0;

  if (dwReply == PRIMOSDK_OK)
  {
    // Get each record in the table and add them to the image
    PRInt32 indexPrevEntry = -1;
    PRInt32 indexCurEntry = -1;
    nsString source, destination;
    mBurnTracksWeight.clear();
    while(mParentCDManager->GetBasesbDevice()->GetNextTransferFileEntry(indexPrevEntry, mDeviceString.get(), false, indexCurEntry, source, destination))
    {
      DWORD dwSize = 0;
      nsString sourceUTFString((const PRUnichar *) source.get());
      nsCString sourceAsciiString;
      NS_UTF16ToCString(sourceUTFString, NS_CSTRING_ENCODING_ASCII, sourceAsciiString);
      DWORD sizeInSectors = 0;
      dwReply = PrimoSDK_AddAudioTrack(mSonicHandle, (PBYTE) sourceAsciiString.get(), 150, &sizeInSectors);
      // this function to be used in licensed version but disabled in demo version
      //dwReply = PrimoSDK_AddAudioTrackEx(mSonicHandle, (PBYTE) sourceAsciiString.get(), 0, 150, 0, &sizeInSectors, NULL, 0, NULL);
      if (dwReply != PRIMOSDK_OK)
        break;
      
      // Add the burn info for this track
      BurnTrackInfo trackInfo;
      trackInfo.index = indexCurEntry;
      trackInfo.weight = sizeInSectors;
      totalBurnWeight += trackInfo.weight;
      mBurnTracksWeight.push_back(trackInfo);

      indexPrevEntry = indexCurEntry;
    }
  }
  if (dwReply == PRIMOSDK_OK)
  {
    // START RECORDING
    dwReply = PrimoSDK_WriteAudioEx(mSonicHandle, PRIMOSDK_WRITE, PRIMOSDK_MAX, NULL);
  }

  if (dwReply == PRIMOSDK_OK || dwReply == PRIMOSDK_ITSADEMO)
  {
    // Adjust the weights for burn tracks by normalizing the values for a scale of 0 to 100
    for (std::list<BurnTrackInfo>::iterator burnIterator = mBurnTracksWeight.begin(); burnIterator != mBurnTracksWeight.end(); burnIterator ++)
    {
      (*burnIterator).weight = (100 * (*burnIterator).weight) / totalBurnWeight;
    }

    // Start a timer to get the burn status
    DBInfoForTransfer *burnInfo = new DBInfoForTransfer;
    burnInfo->cdObject = this;
    burnInfo->table = table;
    mTimer->Cancel();
    mTimer->InitWithFuncCallback(MyTimerCallbackFunc, burnInfo, 1000, nsITimer::TYPE_REPEATING_SLACK); // Every second
  }
  else
  {
    // Operation failed, free resources
    ReleaseCDBurnResources();

    return PR_FALSE;
  }

  return PR_TRUE;
}
void WinCDObject::ReleaseCDBurnResources()
{
  DWORD driveUnit = mDriveLetter;

  // CLOSE AUDIO STRUCTURE
  DWORD dwReply = PrimoSDK_CloseAudio(mSonicHandle);

  // Unblock
  dwReply = PrimoSDK_UnitLock(mSonicHandle, &driveUnit, PRIMOSDK_UNLOCK);

  // UNBLOCK AIN AND FILE SYSTEM ACTIVITY,
  dwReply = PrimoSDK_UnitAIN(mSonicHandle, &driveUnit, PRIMOSDK_UNLOCK);

  // RELEASE DRIVE
  dwReply = PrimoSDK_UnitVxBlock(mSonicHandle, &driveUnit, PRIMOSDK_UNLOCK, NULL);
}

void WinCDObject::TransferComplete()
{
  if (IsUploadInProgress())
  {
    PrimoSDK_CloseAudio(mSonicHandle);
    ReleaseCDBurnResources();
  }
}

bool WinCDObject::SetGapBurnedTrack(PRUint32 numSeconds)
{
  mBurnGap = numSeconds;

  return true;
}

// PlatformCDObjectManager definitions
//

PlatformCDObjectManager::PlatformCDObjectManager(sbCDDevice* parent):
mParentDevice(parent)
{
}

PlatformCDObjectManager::~PlatformCDObjectManager()
{
}

void PlatformCDObjectManager::Initialize()
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

void PlatformCDObjectManager::Finalize()
{
  // Release the memory by deleting the CD objects
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    // Removing any tracks for CD burn and CD rip
    RemoveTransferEntries((*iteratorCDObjectects)->GetDeviceString().get());
    delete *iteratorCDObjectects;
  }

  PrimoSDK_End();

}

void PlatformCDObjectManager::RemoveTransferEntries(const PRUnichar* deviceString)
{
  mParentDevice->RemoveExistingTransferTableEntries(deviceString, true, true);
  mParentDevice->RemoveExistingTransferTableEntries(deviceString, false, true);
}

void PlatformCDObjectManager::EnumerateDrives()
{
  DWORD sonicHandle;
  PrimoSDK_GetHandle(&sonicHandle);
  //for (unsigned char driveLetter = DRIVE_START_LETTER; driveLetter < DRIVE_END_LETTER; driveLetter ++)
  {
//    DWORD driveUnit = driveLetter;
    DWORD driveType;
    char unitDescr[50];
    for (int iHost = 0;  iHost < 256; iHost++ )
    {
      for (int iID = 0;  iID < 64; iID++ )
      {
        DWORD dwUnit = (iID << 16) | (iHost << 24);
        if (PrimoSDK_UnitInfo(sonicHandle,&dwUnit,&driveType,(PBYTE) unitDescr,NULL) == PRIMOSDK_OK)
        {
          if ( driveType == PRIMOSDK_CDR || driveType == PRIMOSDK_CDRW ||
            driveType == PRIMOSDK_DVDRAM || driveType == PRIMOSDK_DVDR ||
            driveType == PRIMOSDK_DVDRW || driveType == PRIMOSDK_DVDPRW )
          {
            WinCDObject* cdObject = new WinCDObject(this, LOBYTE(LOWORD(dwUnit)));
            mCDDrives.push_back(cdObject);
            cdObject->Initialize();
            RemoveTransferEntries(cdObject->GetDeviceString().get());
          }
        }
      }
    }
  }
  PrimoSDK_ReleaseHandle(sonicHandle);
}

PRUnichar* PlatformCDObjectManager::GetContext(const PRUnichar* deviceString)
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

PRUnichar* PlatformCDObjectManager::EnumDeviceString(PRUint32 index)
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

PRBool PlatformCDObjectManager::IsDownloadSupported(const PRUnichar* deviceString)
{
  // All CD Drives should support downloading, a.k.a ripping
  return PR_FALSE;
}

PRUint32 PlatformCDObjectManager::GetSupportedFormats(const PRUnichar* deviceString)
{
  return 0;
}

PRBool PlatformCDObjectManager::IsUploadSupported(const PRUnichar* deviceString)
{
  PRBool retVal = false;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
    retVal = cdObject->IsDriveWritable();

  return retVal;
}

PRBool PlatformCDObjectManager::IsTransfering(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRBool PlatformCDObjectManager::IsDeleteSupported(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRUint32 PlatformCDObjectManager::GetUsedSpace(const PRUnichar* deviceString)
{
  PRUint32 usedSpace = 0;

  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
    usedSpace = cdObject->GetUsedDiscSpace();

  return usedSpace;
}

PRUint32 PlatformCDObjectManager::GetAvailableSpace(const PRUnichar* deviceString)
{
  PRUint32 availableSpace = 0;

  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    cdObject->GetFreeDiscSpace();
  }

  return availableSpace;
}

PRBool PlatformCDObjectManager::GetTrackTable(const PRUnichar* deviceString, PRUnichar** dbContext, PRUnichar** tableName)
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

PRBool PlatformCDObjectManager::EjectDevice(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRBool PlatformCDObjectManager::IsUpdateSupported(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRBool PlatformCDObjectManager::IsEjectSupported(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRUint32 PlatformCDObjectManager::GetNumDevices()
{
  return mCDDrives.size();
}

PRUint32 PlatformCDObjectManager::GetDeviceState(const PRUnichar* deviceString)
{
  return 0;
}

PRUnichar* PlatformCDObjectManager::GetNumDestinations (const PRUnichar* DeviceString)
{
  return nsnull;
}

PRBool PlatformCDObjectManager::StopCurrentTransfer(const PRUnichar*  deviceString)
{
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    cdObject->StopTransfer();
  }

  return PR_TRUE;
}

PRBool PlatformCDObjectManager::SuspendTransfer(const PRUnichar*  DeviceString)
{
  return PR_FALSE;
}

PRBool PlatformCDObjectManager::ResumeTransfer(const PRUnichar*  DeviceString)
{
  return PR_FALSE;
}

PRBool PlatformCDObjectManager::OnCDDriveEvent(PRBool mediaInserted)
{
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    (*iteratorCDObjectects)->EvaluateCDDrive();
  }

  return PR_TRUE;
}

WinCDObject* PlatformCDObjectManager::GetDeviceMatchingString(const PRUnichar* deviceString)
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

bool PlatformCDObjectManager::TransferFile(const PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber)
{ 
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    nsString destUTFString((const PRUnichar *) destination);
    nsCString destAsciiString;
    NS_UTF16ToCString(destUTFString, NS_CSTRING_ENCODING_ASCII, destAsciiString);
    
    if (mParentDevice->IsDownloadInProgress(deviceString))
    {
      // rip
      cdObject->RipTrack(source, (char *) destAsciiString.get(), dbContext, table, index, curDownloadRowNumber);
    }
    else
    {
      // burn
      cdObject->BurnTrack(source, (char *) destAsciiString.get(), dbContext, table, index, curDownloadRowNumber);
    }
  }

  return false;
}

PRBool PlatformCDObjectManager::SetCDRipFormat(const PRUnichar*  deviceString, PRUint32 format)
{
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    cdObject->SetCDRipFormat(format);

    return PR_TRUE;
  }

  return PR_FALSE;
}

PRUint32 PlatformCDObjectManager::GetCDRipFormat(const PRUnichar*  deviceString)
{
  PRUint32 format;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    format = cdObject->GetCDRipFormat();
  }

  return format;
}

PRUint32 PlatformCDObjectManager::GetCurrentTransferRowNumber(const PRUnichar* deviceString)
{
  PRUint32 rowNumber = -1;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    rowNumber = cdObject->GetCurrentTransferRowNumber();
  }

  return rowNumber;
}

void PlatformCDObjectManager::SetTransferState(const PRUnichar* deviceString, PRInt32 newState)
{
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    cdObject->SetTransferState(newState);
  }
}

PRBool PlatformCDObjectManager::IsDeviceIdle(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsDeviceIdle();
  }
  return state;
}

PRBool PlatformCDObjectManager::IsDownloadInProgress(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsDownloadInProgress();
  }
  return state;
}

PRBool PlatformCDObjectManager::IsUploadInProgress(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsUploadInProgress();
  }
  return state;
}

PRBool PlatformCDObjectManager::IsTransferInProgress(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsDownloadInProgress() || cdObject->IsUploadInProgress();
  }
  return state;
}

PRBool PlatformCDObjectManager::IsDownloadPaused(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsDownloadPaused();
  }
  return state;
}

PRBool PlatformCDObjectManager::IsUploadPaused(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsUploadPaused();
  }
  return state;
}

PRBool PlatformCDObjectManager::IsTransferPaused(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsUploadPaused() || cdObject->IsDownloadPaused();
  }
  return state;
}

void PlatformCDObjectManager::TransferComplete(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    cdObject->TransferComplete();
  }
}

PRBool PlatformCDObjectManager::SetGapBurnedTrack(const PRUnichar* deviceString, PRUint32 numSeconds)
{
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    cdObject->SetGapBurnedTrack(numSeconds);
  }

  return PR_TRUE;
}

PRBool PlatformCDObjectManager::GetWritableCDDrive(PRUnichar **deviceString)
{
  *deviceString = nsnull;
  // This should return the actual designated CD Drive for CD writes
  // But for now return the first available drive!
  std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin();
  if (iteratorCDObjectects != mCDDrives.end())
  {
    nsString tempDeviceString = (*iteratorCDObjectects)->GetDeviceString();
    size_t nLen = tempDeviceString.Length() + 1;
    *deviceString = (PRUnichar *) nsMemory::Clone(tempDeviceString.get(), nLen * sizeof(PRUnichar));
    *deviceString[nLen-1] = NULL;

    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool PlatformCDObjectManager::UploadTable(const PRUnichar *DeviceString, const PRUnichar *TableName)
{
  WinCDObject* cdObject = GetDeviceMatchingString(DeviceString);
  if (cdObject)
  {
    return cdObject->BurnTable(TableName);
  }

  return PR_FALSE;
}

