/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
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

#include "sbIDatabaseQuery.h"
#include "sbIMediaLibrary.h"
#include "sbIPlaylist.h"

#define DRIVE_START_LETTER  'A'
#define DRIVE_END_LETTER    'Z'

#define NUM_BYTES_PER_SECTOR 2048

#define DEVICE_STRING        NS_LITERAL_STRING("compactdisc.name").get()
#define DEVICE_STRING_PREFIX NS_LITERAL_STRING(" (").get()
#define DEVICE_STRING_SUFFIX NS_LITERAL_STRING(":)").get()

#define TRACK_TABLE NS_LITERAL_STRING("CDTracks").get()

#define SB_CHECK_FAILURE(_rv)                             \
  PR_BEGIN_MACRO                                          \
    if (NS_FAILED(rv)) {                                  \
      NS_WARNING("*** SB_CHECK_FAILURE ***");             \
      /* return PR_FALSE; /**/                            \
    }                                                     \
  PR_END_MACRO

WinCDObject::WinCDObject(WinCDDeviceManager *parent, char driveLetter):
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

  mDeviceContext.AssignLiteral(CONTEXT_COMPACT_DISC_DEVICE);
  mDeviceContext += driveLetter;
}

WinCDObject::~WinCDObject()
{
  PrimoSDK_ReleaseHandle(mSonicHandle);
}

void
WinCDObject::Initialize()
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

void
WinCDObject::EvaluateCDDrive()
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
void
WinCDObject::ClearCDLibraryData()
{
  nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
  pQuery->SetAsyncQuery(PR_FALSE);
  pQuery->SetDatabaseGUID(GetDeviceContext());

  PRBool retVal;

  nsCOMPtr<sbIPlaylistManager> pPlaylistManager = do_CreateInstance( "@songbirdnest.com/Songbird/PlaylistManager;1" );
  nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbirdnest.com/Songbird/MediaLibrary;1" );

  if(pPlaylistManager.get())
  {
    pPlaylistManager->DeletePlaylist(GetCDTracksTable(), pQuery, &retVal);

    if(pLibrary.get())
    {
      pLibrary->SetQueryObject(pQuery);
      pLibrary->PurgeDefaultLibrary(PR_FALSE, &retVal);
    }
  }
}

char
WinCDObject::GetDriveLetter()
{
  return mDriveLetter;
}

nsString&
WinCDObject::GetDeviceString()
{
  return mDeviceString;
}

nsString&
WinCDObject::GetDeviceContext()
{
  return mDeviceContext;
}

PRUint32
WinCDObject::GetDriveID()
{
  return 0;
}

PRUint32
WinCDObject::GetDriveSpeed()
{
  return 0;
}

PRBool
WinCDObject::SetDriveSpeed(PRUint32 driveSpeed)
{
  return PR_FALSE;
}

PRUint32
WinCDObject::GetUsedDiscSpace()
{
  return mUsedSpace;
}

PRUint32
WinCDObject::GetFreeDiscSpace()
{
  return mFreeSpace;
}


void
WinCDObject::ReadDriveAttributes()
{
  DWORD driveReady;
  DWORD driveUnit = mDriveLetter;
  PrimoSDK_UnitInfo(mSonicHandle, &driveUnit, &mDriveType, (BYTE *) &mDriveDescription, &driveReady);

}

PRBool
WinCDObject::IsDriveWritable()
{
  return mDriveType&PRIMOSDK_CDRW;
}

PRBool
WinCDObject::IsAudioDiscInDrive()
{

  return (mNumTracks > 0);
}

PRBool
WinCDObject::IsDiscAvailableForWrite()
{
  return (mFreeSpace > 0);
}

nsString&
WinCDObject::GetCDTracksTable()
{
  return mCDTrackTable;
}

PRBool
WinCDObject::UpdateCDLibraryData()
{
  nsresult rv;
  PRBool bRet = PR_FALSE;
  if (mNumTracks)
  {
    nsCOMPtr<sbIDatabaseQuery> pQuery =
      do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
    SB_CHECK_FAILURE(rv);

    rv = pQuery->SetAsyncQuery(PR_FALSE);
    SB_CHECK_FAILURE(rv);

    rv = pQuery->SetDatabaseGUID(GetDeviceContext());
    SB_CHECK_FAILURE(rv);

    nsCOMPtr<sbIMediaLibrary> pLibrary =
      do_CreateInstance("@songbirdnest.com/Songbird/MediaLibrary;1", &rv);
    SB_CHECK_FAILURE(rv);

    rv = pLibrary->SetQueryObject(pQuery);
    SB_CHECK_FAILURE(rv);

    rv = pLibrary->CreateDefaultLibrary();
    SB_CHECK_FAILURE(rv);

    nsCOMPtr<sbIPlaylistManager> pPlaylistManager =
      do_CreateInstance("@songbirdnest.com/Songbird/PlaylistManager;1", &rv);
    SB_CHECK_FAILURE(rv);

    rv = pPlaylistManager->CreateDefaultPlaylistManager(pQuery);
    SB_CHECK_FAILURE(rv);

    nsAutoString strDevice(GetDeviceString());
    nsCOMPtr<sbIPlaylist> pPlaylist;
    rv = pPlaylistManager->CreatePlaylist(GetCDTracksTable(), strDevice,
                                          strDevice, NS_LITERAL_STRING("cd"),
                                          pQuery, getter_AddRefs(pPlaylist));
    SB_CHECK_FAILURE(rv);

    rv = pQuery->ResetQuery();
    SB_CHECK_FAILURE(rv);

    // Create record for each track
    for (unsigned int trackNum = 1; trackNum <= mNumTracks; trackNum ++)
    {
      DWORD sessionNumber, trackType, prepGap, start, lengthSectors;
      PrimoSDK_TrackInfo(mSonicHandle, trackNum, &sessionNumber, &trackType,
                         &prepGap, &start, &lengthSectors);
      // These magic numbers are based on,
      // an audio CD sector size = 2352
      // and audio CD encoded at 44.1KHz, Stereo, with 16-bit/sample
      DWORD numSeconds = (lengthSectors*2352)/(2*2*44100);

      PRUnichar driveLetter = (PRUnichar) mDriveLetter;
      nsString url(driveLetter);
      url += NS_LITERAL_STRING(":\\track0");
      url.AppendInt(trackNum);
      url += NS_LITERAL_STRING(".cda");

      nsAutoString name, tim, artist, album, genre;

      const static PRUnichar *aMetaKeys[] =
        {NS_LITERAL_STRING("length").get(), NS_LITERAL_STRING("title").get()};
      const static PRUint32 nMetaKeyCount =
        sizeof(aMetaKeys) / sizeof(aMetaKeys[0]);

      nsString strTitle;
      strTitle.AssignLiteral("CD Track ");
      strTitle.AppendInt(trackNum);

      PRUnichar formattedLength[10];
      wsprintf(formattedLength, L"%d:%02d", numSeconds/60, numSeconds%60);
      nsString strLength(formattedLength);

      PRUnichar** aMetaValues =
        (PRUnichar **) nsMemory::Alloc(nMetaKeyCount * sizeof(PRUnichar *));
      aMetaValues[0] = ToNewUnicode(strLength);
      aMetaValues[1] = ToNewUnicode(strTitle);

      nsAutoString guid;
      rv = pLibrary->AddMedia(url, nMetaKeyCount, aMetaKeys, nMetaKeyCount,
                              const_cast<const PRUnichar **>(aMetaValues),
                              PR_TRUE, PR_FALSE, guid);
      SB_CHECK_FAILURE(rv);

      // Don't know why (thanks to the in-ability to debug javascript) but
      // sometimes "addMedia" fails the first time and calling it again
      // succeeds, needs to be investigated.
      if (guid.IsEmpty()) {
        rv = pLibrary->AddMedia(url, nMetaKeyCount, aMetaKeys, nMetaKeyCount,
                                const_cast<const PRUnichar **>(aMetaValues),
                                PR_FALSE, PR_FALSE, guid);
        SB_CHECK_FAILURE(rv);
      }

      if(!guid.IsEmpty() && pPlaylist) {
        rv = pPlaylist->AddByGUID(guid, GetDeviceContext(), -1, PR_FALSE,
                                  PR_FALSE, &bRet);
        SB_CHECK_FAILURE(rv);
      }

      nsMemory::Free(aMetaValues[0]);
      nsMemory::Free(aMetaValues[1]);
      nsMemory::Free(aMetaValues);
    }

    // Notify listeners of the event
    mParentCDManager->GetBasesbDevice()->DoDeviceConnectCallback(GetDeviceString());
  }
  else
  {
    // Remove all the info about this CD
    ClearCDLibraryData();
    mParentCDManager->GetBasesbDevice()->DoDeviceDisconnectCallback(GetDeviceString());
  }

  return PR_TRUE;
}

void
WinCDObject::ReadDiscAttributes(PRBool& mediaChanged)
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

PRBool
WinCDObject::RipTrack(const PRUnichar* source,
                      char* destination,
                      PRUnichar* dbContext,
                      PRUnichar* table,
                      PRUnichar* index,
                      PRInt32 curDownloadRowNumber)
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

PRBool
WinCDObject::BurnTrack(const PRUnichar* source,
                       char* destination,
                       PRUnichar* dbContext,
                       PRUnichar* table,
                       PRUnichar* index,
                       PRInt32 curDownloadRowNumber)
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

PRBool
WinCDObject::SetupCDBurnResources()
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

PRUint32
WinCDObject::GetCDTrackNumber(const PRUnichar* trackName)
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

void
WinCDObject::UpdateIOProgress(DBInfoForTransfer *ripInfo)
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
    if (mStopTransfer || !mParentCDManager->GetBasesbDevice()->IsTransferInProgress(mDeviceString))
    {
      if (mStopTransfer)
        // Since the transfer stopped reset the progress
        mParentCDManager->GetBasesbDevice()->UpdateIOProgress((PRUnichar *) GetDeviceString().get(), (PRUnichar *) ripInfo->table.get(), (PRUnichar *) ripInfo->index.get(), 0);
     
      mTimer->Cancel();
    }

    delete ripInfo;
  }
}

void
WinCDObject::MyTimerCallbackFunc(nsITimer *aTimer,
                                 void *aClosure)
{
  DBInfoForTransfer *transferInfo = (DBInfoForTransfer *) aClosure;

  transferInfo->cdObject->UpdateIOProgress(transferInfo);
}

void
WinCDObject::StopTransfer()
{
  mStopTransfer = PR_TRUE; // Will be handled later in the timer function
}

PRUint32
WinCDObject::GetWeightTrack(nsString& trackPath)
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

PRBool
WinCDObject::BurnTable(const PRUnichar* table)
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

void
WinCDObject::ReleaseCDBurnResources()
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

void
WinCDObject::TransferComplete()
{
  if (IsUploadInProgress())
  {
    PrimoSDK_CloseAudio(mSonicHandle);
    ReleaseCDBurnResources();
  }
}

PRBool
WinCDObject::SetGapBurnedTrack(PRUint32 numSeconds)
{
  mBurnGap = numSeconds;

  return PR_TRUE;
}

// WinCDDeviceManager definitions
//

WinCDDeviceManager::WinCDDeviceManager() :
  mParentDevice(nsnull)
{
}

WinCDDeviceManager::~WinCDDeviceManager()
{
}

void
WinCDDeviceManager::Initialize(void* aParent)
{
  NS_ASSERTION(aParent, "Null pointer");
  NS_ADDREF(mParentDevice = (sbCDDevice*)aParent);

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

void
WinCDDeviceManager::Finalize()
{
  // Release the memory by deleting the CD objects
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    // Removing any tracks for CD burn and CD rip
    WinCDObject* obj = (*iteratorCDObjectects);
    RemoveTransferEntries((*iteratorCDObjectects)->GetDeviceString().get());
    delete *iteratorCDObjectects;
  }

  NS_RELEASE(mParentDevice);
  mParentDevice = nsnull;

  PrimoSDK_End();
}

void
WinCDDeviceManager::RemoveTransferEntries(const PRUnichar* deviceString)
{
  mParentDevice->RemoveExistingTransferTableEntries(deviceString, true, true);
  mParentDevice->RemoveExistingTransferTableEntries(deviceString, false, true);
}

void
WinCDDeviceManager::EnumerateDrives()
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

void
WinCDDeviceManager::GetContext(const nsAString& aDeviceString,
                               nsAString& _retval)
{
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin();
       iteratorCDObjectects != mCDDrives.end();
       iteratorCDObjectects ++) {
    nsAutoString str((*iteratorCDObjectects)->GetDeviceString());
    if (str.Equals(aDeviceString))
      _retval.Assign((*iteratorCDObjectects)->GetDeviceContext());
  }
}

void
WinCDDeviceManager::GetDeviceStringByIndex(PRUint32 aIndex,
                                           nsAString& _retval)
{
  PRUint32 currentIndex = 0;
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    if (currentIndex ++ == aIndex)
    {
      _retval.Assign((*iteratorCDObjectects)->GetDeviceString());
    }
  }
}

PRBool
WinCDDeviceManager::IsDownloadSupported(const nsAString& aDeviceString)
{
  // All CD Drives should support downloading, a.k.a ripping
  return PR_FALSE;
}

PRUint32
WinCDDeviceManager::GetSupportedFormats(const nsAString& aDeviceString)
{
  return 0;
}

PRBool
WinCDDeviceManager::IsUploadSupported(const nsAString& aDeviceString)
{
  // XXXben Remove me
  nsAutoString str(aDeviceString);

  PRBool retVal = false;
  WinCDObject* cdObject = GetDeviceMatchingString(str.get());

  if (cdObject)
    retVal = cdObject->IsDriveWritable();

  return retVal;
}

PRBool
WinCDDeviceManager::IsTransfering(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

PRBool
WinCDDeviceManager::IsDeleteSupported(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

PRUint32
WinCDDeviceManager::GetUsedSpace(const nsAString& aDeviceString)
{
  PRUint32 usedSpace = 0;

  // XXXben Remove me
  nsAutoString strDevice(aDeviceString);

  WinCDObject* cdObject = GetDeviceMatchingString(strDevice.get());

  if (cdObject)
    usedSpace = cdObject->GetUsedDiscSpace();

  return usedSpace;
}

PRUint32
WinCDDeviceManager::GetAvailableSpace(const nsAString& aDeviceString)
{
  PRUint32 availableSpace = 0;

  // XXXben Remove me
  nsAutoString str(aDeviceString);

  WinCDObject* cdObject = GetDeviceMatchingString(str.get());

  if (cdObject)
  {
    cdObject->GetFreeDiscSpace();
  }

  return availableSpace;
}

PRBool
WinCDDeviceManager::GetTrackTable(const nsAString& aDeviceString,
                                  nsAString& aDBContext,
                                  nsAString& aTableName)
{
  aDBContext = EmptyString();
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    nsAutoString currentDeviceString;
    currentDeviceString.Assign((*iteratorCDObjectects)->GetDeviceString());

    if (currentDeviceString.Equals(aDeviceString)) {
      aDBContext.Assign((*iteratorCDObjectects)->GetDeviceContext());
      aTableName.Assign((*iteratorCDObjectects)->GetCDTracksTable());
    }
  }

  return PR_TRUE;
}

PRBool
WinCDDeviceManager::EjectDevice(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

PRBool
WinCDDeviceManager::IsUpdateSupported(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

PRBool
WinCDDeviceManager::IsEjectSupported(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

PRUint32
WinCDDeviceManager::GetDeviceCount()
{
  return mCDDrives.size();
}

PRUint32
WinCDDeviceManager::GetDeviceState(const nsAString& aDeviceString)
{
  return 0;
}

PRUint32
WinCDDeviceManager::GetDestinationCount(const nsAString& aDeviceString)
{
  return 0;
}

PRBool
WinCDDeviceManager::StopCurrentTransfer(const nsAString& aDeviceString)
{
  // XXXben Remove me
  nsAutoString str(aDeviceString);

  WinCDObject* cdObject = GetDeviceMatchingString(str.get());

  if (cdObject)
  {
    cdObject->StopTransfer();
  }

  return PR_TRUE;
}

PRBool
WinCDDeviceManager::SuspendTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

PRBool
WinCDDeviceManager::ResumeTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

PRBool
WinCDDeviceManager::OnCDDriveEvent(PRBool mediaInserted)
{
  for (std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin(); iteratorCDObjectects != mCDDrives.end(); iteratorCDObjectects ++)
  {
    (*iteratorCDObjectects)->EvaluateCDDrive();
  }

  return PR_TRUE;
}

WinCDObject*
WinCDDeviceManager::GetDeviceMatchingString(const PRUnichar* deviceString)
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

PRBool
WinCDDeviceManager::TransferFile(const PRUnichar* deviceString,
                                 PRUnichar* source,
                                 PRUnichar* destination,
                                 PRUnichar* dbContext,
                                 PRUnichar* table,
                                 PRUnichar* index,
                                 PRInt32 curDownloadRowNumber)
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

  return PR_FALSE;
}

PRBool
WinCDDeviceManager::SetCDRipFormat(const nsAString& aDeviceString,
                                   PRUint32 aFormat)
{
  // XXXben Remove me
  nsAutoString str(aDeviceString);
  WinCDObject* cdObject = GetDeviceMatchingString(str.get());

  if (cdObject)
  {
    cdObject->SetCDRipFormat(aFormat);

    return PR_TRUE;
  }

  return PR_FALSE;
}

PRUint32
WinCDDeviceManager::GetCDRipFormat(const nsAString& aDeviceString)
{
  // XXXben Remove me
  nsAutoString str(aDeviceString);

  PRUint32 format;
  WinCDObject* cdObject = GetDeviceMatchingString(str.get());

  if (cdObject)
  {
    format = cdObject->GetCDRipFormat();
  }

  return format;
}

PRUint32
WinCDDeviceManager::GetCurrentTransferRowNumber(const PRUnichar* deviceString)
{
  PRUint32 rowNumber = -1;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    rowNumber = cdObject->GetCurrentTransferRowNumber();
  }

  return rowNumber;
}

void
WinCDDeviceManager::SetTransferState(const PRUnichar* deviceString,
                                     PRInt32 newState)
{
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);

  if (cdObject)
  {
    cdObject->SetTransferState(newState);
  }
}

PRBool
WinCDDeviceManager::IsDeviceIdle(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsDeviceIdle();
  }
  return state;
}

PRBool
WinCDDeviceManager::IsDownloadInProgress(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsDownloadInProgress();
  }
  return state;
}

PRBool
WinCDDeviceManager::IsUploadInProgress(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsUploadInProgress();
  }
  return state;
}

PRBool
WinCDDeviceManager::IsTransferInProgress(const nsAString& aDeviceString)
{
  // XXXben Remove me
  nsAutoString str(aDeviceString);

  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(str.get());
  if (cdObject)
  {
    state = cdObject->IsDownloadInProgress() || cdObject->IsUploadInProgress();
  }
  return state;
}

PRBool
WinCDDeviceManager::IsDownloadPaused(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsDownloadPaused();
  }
  return state;
}

PRBool
WinCDDeviceManager::IsUploadPaused(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsUploadPaused();
  }
  return state;
}

PRBool
WinCDDeviceManager::IsTransferPaused(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    state = cdObject->IsUploadPaused() || cdObject->IsDownloadPaused();
  }
  return state;
}

void
WinCDDeviceManager::TransferComplete(const PRUnichar* deviceString)
{
  PRBool state = PR_FALSE;
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    cdObject->TransferComplete();
  }
}

PRBool
WinCDDeviceManager::SetGapBurnedTrack(const PRUnichar* deviceString,
                                      PRUint32 numSeconds)
{
  WinCDObject* cdObject = GetDeviceMatchingString(deviceString);
  if (cdObject)
  {
    cdObject->SetGapBurnedTrack(numSeconds);
  }

  return PR_TRUE;
}

PRBool
WinCDDeviceManager::GetWritableCDDrive(nsAString& aDeviceString)
{
  // This should return the actual designated CD Drive for CD writes
  // But for now return the first available drive!
  std::list<WinCDObject *>::iterator iteratorCDObjectects = mCDDrives.begin();
  if (iteratorCDObjectects != mCDDrives.end())
  {
    aDeviceString.Assign((*iteratorCDObjectects)->GetDeviceString());
    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
WinCDDeviceManager::UploadTable(const nsAString& aDeviceString,
                                const nsAString& aTableName)
{
  // XXXben Remove me
  nsAutoString strDevice(aDeviceString), strTable(aTableName);

  WinCDObject* cdObject = GetDeviceMatchingString(strDevice.get());
  if (cdObject)
  {
    return cdObject->BurnTable(strTable.get());
  }

  return PR_FALSE;
}

