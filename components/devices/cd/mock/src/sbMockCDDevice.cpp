/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include <sbArrayUtils.h>
#include <sbICDDevice.h>
#include <sbICDDeviceService.h>
#include <sbIMockCDDevice.h>
#include <sbIMockCDDeviceController.h>

#include <nsArrayEnumerator.h>
#include <nsCOMArray.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>


//==============================================================================
// Mock CD TOC entry implementation
//==============================================================================

class sbMockCDTOCEntry : public sbICDTOCEntry
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICDTOCENTRY

  static sbMockCDTOCEntry * New(PRInt32 aFrameOffset,
                                PRTime aLength,
                                PRInt32 aTrackNumber,
                                PRInt16 aTrackMode,
                                nsAString const & aDrive)
  {
    return new sbMockCDTOCEntry(aFrameOffset,
                                aLength,
                                aTrackNumber,
                                aTrackMode,
                                aDrive);
  }

protected:
  sbMockCDTOCEntry(PRInt32 aFrameOffset,
                   PRTime aLength,
                   PRInt32 aTrackNumber,
                   PRInt16 aTrackMode,
                   nsAString const & aDrive) :
                     mFrameOffset(aFrameOffset),
                     mLength(aLength),
                     mTrackNumber(aTrackNumber),
                     mTrackMode(aTrackMode),
                     mDrive(aDrive)
  {

  }
  virtual ~sbMockCDTOCEntry() {}
private:
  PRInt32 mFrameOffset;
  PRTime mLength;
  PRInt32 mTrackNumber;
  PRInt16 mTrackMode;
  nsString mDrive;
};

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMockCDTOCEntry, sbICDTOCEntry);


NS_IMETHODIMP
sbMockCDTOCEntry::GetFrameOffset(PRInt32 *aFrameOffset)
{
  NS_ENSURE_ARG_POINTER(aFrameOffset);

  *aFrameOffset = mFrameOffset;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDTOCEntry::GetLength(PRTime *aLength)
{
  NS_ENSURE_ARG_POINTER(aLength);

  *aLength = mLength;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDTOCEntry::GetTrackNumber(PRInt32 *aTrackNumber)
{
  NS_ENSURE_ARG_POINTER(aTrackNumber);

  *aTrackNumber = mTrackNumber;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDTOCEntry::GetTrackURI(nsAString & aTrackURI)
{
  nsString uri;
  uri.AssignLiteral("cdda:///");
  uri.Append(mDrive);
  uri.AppendLiteral("/");
  uri.AppendInt(mTrackNumber);
  aTrackURI = uri;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDTOCEntry::GetTrackMode(PRInt16 *aTrackMode)
{
  NS_ENSURE_ARG_POINTER(aTrackMode);

  *aTrackMode = mTrackMode;
  return NS_OK;
}

//==============================================================================
// Mock CD TOC implementation
//==============================================================================

class sbMockCDTOC : public sbICDTOC, sbIMockCDTOC
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICDTOC
  NS_DECL_SBIMOCKCDTOC

  static sbMockCDTOC * New(PRInt32 aFirstTrackIndex,
                           PRInt32 aLastTrackIndex,
                           PRInt32 aLeadOutTrackOffset)
  {
    return new sbMockCDTOC(aFirstTrackIndex,
                           aLastTrackIndex,
                           aLeadOutTrackOffset);
  }
  sbMockCDTOC() : mFirstTrackIndex(0),
                  mLastTrackIndex(0),
                  mLeadOutTrackOffset(0) {}
protected:
  sbMockCDTOC(PRInt32 aFirstTrackIndex,
              PRInt32 aLastTrackIndex,
              PRInt32 aLeadOutTrackOffset) :
                mFirstTrackIndex(aFirstTrackIndex),
                mLastTrackIndex(aLastTrackIndex),
                mLeadOutTrackOffset(aLeadOutTrackOffset) {}

private:
  nsCOMArray<sbICDTOCEntry> mTracks;
  PRInt32 mFirstTrackIndex;
  PRInt32 mLastTrackIndex;
  PRInt32 mLeadOutTrackOffset;
};


NS_IMPL_THREADSAFE_ISUPPORTS2(sbMockCDTOC, sbICDTOC, sbIMockCDTOC)

NS_IMETHODIMP
sbMockCDTOC::GetFirstTrackIndex(PRInt32 *aFirstTrackIndex)
{
  NS_ENSURE_ARG_POINTER(aFirstTrackIndex);

  *aFirstTrackIndex = mFirstTrackIndex;

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDTOC::GetLastTrackIndex(PRInt32 *aLastTrackIndex)
{
  NS_ENSURE_ARG_POINTER(aLastTrackIndex);

  *aLastTrackIndex = mLastTrackIndex;

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDTOC::GetLeadOutTrackOffset(PRInt32 *aLeadOutTrackOffset)
{
  NS_ENSURE_ARG_POINTER(aLeadOutTrackOffset);

  *aLeadOutTrackOffset = mLeadOutTrackOffset;

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDTOC::GetTracks(nsIArray * *aTracks)
{
  NS_ENSURE_ARG_POINTER(aTracks);

  nsresult rv = sbCOMArrayTonsIArray(mTracks, aTracks);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDTOC::AddTocEntry(PRInt32 frameOffset,
                         PRTime length,
                         PRInt32 trackNumber,
                         PRInt16 trackMode)
{
  sbMockCDTOCEntry * entry = sbMockCDTOCEntry::New(frameOffset,
                                                   length,
                                                   trackNumber,
                                                   trackMode,
                                                   NS_LITERAL_STRING("f:"));
  NS_ENSURE_TRUE(entry, NS_ERROR_OUT_OF_MEMORY);

  PRBool const added = mTracks.AppendObject(entry);
  NS_ENSURE_TRUE(added, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDTOC::Initialize(PRInt32 aFirstTrackIndex,
                        PRInt32 aLastTrackIndex,
                        PRInt32 aLeadOutTrackOffset)
{
  mFirstTrackIndex = aFirstTrackIndex;
  mLastTrackIndex = aLastTrackIndex;
  mLeadOutTrackOffset = aLeadOutTrackOffset;

  return NS_OK;
}

//==============================================================================
// Mock CD device implementation
//==============================================================================

class sbMockCDDevice : public sbICDDevice, sbIMockCDDevice
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBICDDEVICE
  NS_DECL_SBIMOCKCDDEVICE

  static sbMockCDDevice * New(nsAString const & aName,
                              PRBool aReadable,
                              PRBool aWritable,
                              PRBool aDiscInserted,
                              PRUint32 aDiscType)
  {
    return new sbMockCDDevice(aName,
                              aReadable,
                              aWritable,
                              aDiscInserted,
                              aDiscType);
  }

  sbMockCDDevice() : mReadable(PR_FALSE),
                     mWritable(PR_FALSE),
                     mDiscInserted(PR_FALSE),
                     mIsDeviceLocked(PR_FALSE),
                     mDiscType(sbICDDevice::AUDIO_DISC_TYPE),
                     mEjected(PR_FALSE) {}

  ~sbMockCDDevice()
  {
    NS_ASSERTION(!mIsDeviceLocked, "ERROR: Device is still locked!!!!!!!!");
    mController = nsnull;
  }

protected:
  sbMockCDDevice(nsAString const & aName,
                 PRBool aReadable,
                 PRBool aWritable,
                 PRBool aDiscInserted,
                 PRUint32 aDiscType) :
                   mName(aName),
                   mReadable(aReadable),
                   mWritable(aWritable),
                   mDiscInserted(aDiscInserted),
                   mDiscType(aDiscType),
                   mEjected(PR_FALSE),
                   mController(nsnull) {}
private:
  nsString mName;
  PRBool mReadable;
  PRBool mWritable;
  PRBool mDiscInserted;
  PRBool mIsDeviceLocked;
  PRUint32 mDiscType;
  PRBool mEjected;
  nsCOMPtr<sbICDTOC> mTOC;
  nsCOMPtr<sbIMockCDDeviceController> mController;
};

NS_IMPL_THREADSAFE_ISUPPORTS2(sbMockCDDevice, sbICDDevice, sbIMockCDDevice)

NS_IMETHODIMP
sbMockCDDevice::GetName(nsAString & aName)
{
  aName = mName;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::GetReadable(PRBool *aReadable)
{
  *aReadable = mReadable;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::GetWriteable(PRBool *aWritable)
{
  *aWritable = mWritable;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::GetIsDiscInserted(PRBool *aDiscInserted)
{
  *aDiscInserted = mDiscInserted;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::GetIdentifier(nsACString & aIdentifier)
{
  // For the mock device, just rely on the device name.
  aIdentifier.Assign(NS_ConvertUTF16toUTF8(mName));
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::GetDiscTOC(sbICDTOC * *aDiscTOC)
{
  NS_ENSURE_ARG_POINTER(aDiscTOC);
  *aDiscTOC = mTOC;
  NS_IF_ADDREF(*aDiscTOC);
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::GetDiscType(PRUint32 *aDiscType)
{
  *aDiscType = mDiscType;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::Eject()
{
  // Inform the controller of the eject (so it can inform the listeners).
  mEjected = PR_TRUE;
  mDiscInserted = PR_FALSE;

  if (mController) {
    nsresult rv = mController->NotifyEject(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIMockCDDevice

NS_IMETHODIMP
sbMockCDDevice::Initialize(nsAString const & aName,
                           PRBool aReadable,
                           PRBool aWritable,
                           PRBool aDiscInserted,
                           PRUint32 aDiscType,
                           PRBool aEjected,
                           sbIMockCDDeviceController *aController)
{
  mName = aName;
  mReadable = aReadable;
  mWritable = aWritable;
  mDiscInserted = aDiscInserted;
  mDiscType = aDiscType;
  mEjected = aEjected;
  mController = aController;

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::GetEjected(PRBool * aEjected)
{
  NS_ENSURE_ARG_POINTER(aEjected);

  *aEjected = mEjected;

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::SetEjected(PRBool aEjected)
{
  mEjected = aEjected;

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::SetDiscTOC(sbICDTOC * aDiscTOC)
{
  mTOC = aDiscTOC;
  mDiscInserted = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::GetIsDeviceLocked(PRBool *aIsLocked)
{
  NS_ENSURE_ARG_POINTER(aIsLocked);

  *aIsLocked = mIsDeviceLocked;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::LockDevice()
{
  mIsDeviceLocked = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDDevice::UnlockDevice()
{
  mIsDeviceLocked = PR_FALSE;
  return NS_OK;
}

//==============================================================================
// Mock CD Device TOC Creation Utility Methods 
//==============================================================================

static nsresult
SB_MakeMidnightRock(sbICDTOC **aOutTOC)
{
  NS_ENSURE_ARG_POINTER(aOutTOC);

  nsresult rv;
  nsCOMPtr<sbIMockCDTOC> toc =
    do_CreateInstance("@songbirdnest.com/Songbird/MockCDTOC;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = toc->Initialize(1, 15, 285675);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(32, 309, 0, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(23260, 231, 1, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(40612, 242, 2, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(58770, 191, 3, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(73145, 310, 4, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(96415, 290, 5, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(118232, 301, 6, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(140867, 259, 7, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(160322, 316, 8, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(184085, 222, 9, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(200777, 236, 10, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(218535, 185, 11, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(232437, 211, 12, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(248320, 184, 13, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(262145, 313, 14, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDTOC> qiTOC = do_QueryInterface(toc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  qiTOC.forget(aOutTOC);
  return NS_OK;
}

static nsresult
SB_MakeBabyOneMoreTime(sbICDTOC **aOutTOC)
{
  NS_ENSURE_ARG_POINTER(aOutTOC);

  nsresult rv;
  nsCOMPtr<sbIMockCDTOC> toc =
    do_CreateInstance("@songbirdnest.com/Songbird/MockCDTOC;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = toc->Initialize(1, 12, 260335);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = toc->AddTocEntry(0, 211, 0, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(15847, 200, 1, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(30859, 246, 2, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(49320, 202, 3, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(64479, 245, 4, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(82865, 312, 5, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(106307, 234, 6, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(123929, 243, 7, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(142217, 216, 8, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(158447, 223, 9, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(175179, 223, 10, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(203309, 760, 11, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDTOC> qiTOC = do_QueryInterface(toc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  qiTOC.forget(aOutTOC);
  return NS_OK;
}

static nsresult
SB_MakeAllThatYouCantLeaveBehind(sbICDTOC **aOutTOC)
{
  NS_ENSURE_ARG_POINTER(aOutTOC);

  nsresult rv;
  nsCOMPtr<sbIMockCDTOC> toc =
    do_CreateInstance("@songbirdnest.com/Songbird/MockCDTOC;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = toc->Initialize(1, 11, 225562);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = toc->AddTocEntry(150, 248, 0, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(18843, 272, 1, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(39601, 227, 2, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(56966, 296, 3, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(79487, 267, 4, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(99796, 219, 5, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(116534, 226, 6, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(133832, 288, 7, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(155768, 258, 8, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(175400, 330, 9, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(200468, 331, 10, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDTOC> qiTOC = do_QueryInterface(toc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  qiTOC.forget(aOutTOC);
  return NS_OK;
}

static nsresult
SB_MakeIncredibad(sbICDTOC **aOutTOC)
{
  NS_ENSURE_ARG_POINTER(aOutTOC);

  nsresult rv;
  nsCOMPtr<sbIMockCDTOC> toc =
    do_CreateInstance("@songbirdnest.com/Songbird/MockCDTOC;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = toc->Initialize(1, 19, 190565);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = toc->AddTocEntry(150, 76, 0, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(5896, 155, 1, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(17528, 151, 2, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(28879, 156, 3, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(40599, 126, 4, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(50106, 139, 5, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(60584, 64, 6, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(65394, 193, 7, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(79870, 34, 8, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(82446, 106, 9, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(90457, 123, 10, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(99748, 193, 11, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(114258, 126, 12, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(123750, 161, 13, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(135829, 65, 14, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(140754, 167, 15, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(153283, 175, 16, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(166425, 149, 17, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = toc->AddTocEntry(177440, 179, 18, sbICDTOCEntry::TRACKMODE_AUDIO);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDTOC> qiTOC = do_QueryInterface(toc, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  qiTOC.forget(aOutTOC);
  return NS_OK;
}


//==============================================================================
// Mock CD Device Service
//==============================================================================

class sbMockCDService : public sbICDDeviceService,
                        public sbIMockCDDeviceController
{
public:
  sbMockCDService();
  virtual ~sbMockCDService();

  NS_DECL_ISUPPORTS
  NS_DECL_SBICDDEVICESERVICE
  NS_DECL_SBIMOCKCDDEVICECONTROLLER

  nsresult Init();

protected:
  nsCOMArray<sbICDDeviceListener> mListeners;
  nsCOMArray<sbICDDevice>         mDevices;
};


NS_IMPL_THREADSAFE_ISUPPORTS2(sbMockCDService,
                              sbICDDeviceService,
                              sbIMockCDDeviceController)

sbMockCDService::sbMockCDService()
{
}

sbMockCDService::~sbMockCDService()
{
}

nsresult
sbMockCDService::Init()
{
  NS_ENSURE_TRUE(mDevices.Count() == 0, NS_ERROR_UNEXPECTED);

  // Create two mock cd devices.
  nsresult rv;
  nsCOMPtr<sbIMockCDDevice> device1 =
    do_CreateInstance("@songbirdnest.com/Songbird/MockCDDevice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = device1->Initialize(
      NS_LITERAL_STRING("Songbird MockCD Device 8000"),
      PR_TRUE,   // readable
      PR_FALSE,  // writable
      PR_FALSE,  // is disc inserted
      sbICDDevice::AUDIO_DISC_TYPE,
      PR_FALSE,  // is disc ejected
      this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMockCDDevice> device2 =
    do_CreateInstance("@songbirdnest.com/Songbird/MockCDDevice;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = device2->Initialize(
      NS_LITERAL_STRING("Songbird MockCD Device 7000"),
      PR_TRUE,   // readable
      PR_FALSE,  // writable
      PR_FALSE,  // is disc inserted
      sbICDDevice::AUDIO_DISC_TYPE,
      PR_FALSE,  // is disc ejected
      this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbICDDevice> cdDevice1 = do_QueryInterface(device1, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbICDDevice> cdDevice2 = do_QueryInterface(device2, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  mDevices.AppendObject(cdDevice1);
  mDevices.AppendObject(cdDevice2);

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbICDDeviceService

NS_IMETHODIMP
sbMockCDService::GetWeight(PRInt32 *aWeight)
{
  NS_ENSURE_ARG_POINTER(aWeight);
  // return a weight of 0 effectively making the mock CD service selected
  // only if no other service wants to assume responsibility.
  *aWeight = 0;
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDService::GetNbDevices(PRInt32 *aNbDevices)
{
  NS_ENSURE_ARG_POINTER(aNbDevices);
  *aNbDevices = mDevices.Count();
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDService::GetDevice(PRInt32 deviceIndex, sbICDDevice **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(deviceIndex < mDevices.Count(), NS_ERROR_UNEXPECTED);
  *_retval = mDevices[deviceIndex];
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDService::GetDeviceFromIdentifier(const nsACString & aDeviceIdentifier,
                                         sbICDDevice **_retval)
{
  *_retval = nsnull;
  for (PRInt32 i = 0; i < mDevices.Count(); i++) {
    nsCOMPtr<sbICDDevice> curDevice = mDevices[i];

    nsCString deviceIdentifier;
    nsresult rv = curDevice->GetIdentifier(deviceIdentifier);
    NS_ENSURE_SUCCESS(rv, rv);

    if (deviceIdentifier.Equals(aDeviceIdentifier)) {
      curDevice.forget(_retval);
      break;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDService::GetCDDevices(nsISimpleEnumerator **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  // Convert to a nsIArray and enumerate.
  return NS_NewArrayEnumerator(_retval, mDevices); 
}

NS_IMETHODIMP
sbMockCDService::RegisterListener(sbICDDeviceListener *listener)
{
  NS_ENSURE_ARG_POINTER(listener);
  mListeners.AppendObject(listener);
  return NS_OK;
}

NS_IMETHODIMP
sbMockCDService::RemoveListener(sbICDDeviceListener *listener)
{
  NS_ENSURE_ARG_POINTER(listener);
  mListeners.RemoveObject(listener);
  return NS_ERROR_NOT_IMPLEMENTED;
}

//------------------------------------------------------------------------------
// sbIMockCDDeviceController

NS_IMETHODIMP
sbMockCDService::InsertMedia(sbICDDevice *aCDDevice, PRUint16 aMediaDisc)
{
  NS_ENSURE_ARG_POINTER(aCDDevice);

  // Don't insert media twice.
  nsresult rv;
  PRBool isDiscInserted = PR_FALSE;
  rv = aCDDevice->GetIsDiscInserted(&isDiscInserted);
  if (NS_FAILED(rv) || isDiscInserted) {
    return NS_OK;
  }

  // Determine which TOC to insert.
  nsCOMPtr<sbICDTOC> mediaTOC;
  switch (aMediaDisc) {
    case sbIMockCDDeviceController::MOCK_MEDIA_DISC_MIDNIGHT_ROCK:
      rv = SB_MakeMidnightRock(getter_AddRefs(mediaTOC));
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case sbIMockCDDeviceController::MOCK_MEDIA_DISC_BABY_ONE_MORE_TIME:
      rv = SB_MakeBabyOneMoreTime(getter_AddRefs(mediaTOC));
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case sbIMockCDDeviceController::MOCK_MEDIA_DISC_U2:
      rv = SB_MakeAllThatYouCantLeaveBehind(getter_AddRefs(mediaTOC));
      NS_ENSURE_SUCCESS(rv, rv);
      break;

    case sbIMockCDDeviceController::MOCK_MEDIA_DISC_INCREDIBAD:
      rv = SB_MakeIncredibad(getter_AddRefs(mediaTOC));
      NS_ENSURE_SUCCESS(rv, rv);
      break;
  }

  nsCOMPtr<sbIMockCDDevice> mockDevice = do_QueryInterface(aCDDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mockDevice->SetDiscTOC(mediaTOC);
  NS_ENSURE_SUCCESS(rv, rv);

  // Inform the listeners.
  for (PRInt32 i = 0; i < mListeners.Count(); i++) {
    rv = mListeners[i]->OnMediaInserted(aCDDevice);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "Could not inform the listener of media insertion!");
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDService::EjectMedia(sbICDDevice *aCDDevice)
{
  NS_ENSURE_ARG_POINTER(aCDDevice);

  // Ensure that this is a mock cd device first.
  nsresult rv;
  nsCOMPtr<sbIMockCDDevice> mockDevice = do_QueryInterface(aCDDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isDiscInserted = PR_FALSE;
  rv = aCDDevice->GetIsDiscInserted(&isDiscInserted);
  if (NS_FAILED(rv) || !isDiscInserted) {
    return NS_OK;
  }

  rv = aCDDevice->Eject();
  NS_ENSURE_SUCCESS(rv, rv);

  // Inform the listeners
  for (PRInt32 i = 0; i < mListeners.Count(); i++) {
    rv = mListeners[i]->OnMediaEjected(aCDDevice);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "Could not inform the listener of media eject!");
  }

  return NS_OK;
}

NS_IMETHODIMP
sbMockCDService::NotifyEject(sbICDDevice *aCDDevice)
{
  NS_ENSURE_ARG_POINTER(aCDDevice);

  // Ensure that a mock CD device was removed.
  nsresult rv;
  nsCOMPtr<sbIMockCDDevice> mockDevice = do_QueryInterface(aCDDevice, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Inform the listeners
  for (PRInt32 i = 0; i < mListeners.Count(); i++) {
    rv = mListeners[i]->OnMediaEjected(aCDDevice);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv),
        "Could not inform the listener of media eject!");
  }

  return NS_OK;
}


//==============================================================================
// XPCOM component registration
//==============================================================================

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMockCDDevice)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMockCDTOC)
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMockCDService, Init)

static nsModuleComponentInfo sbMockCDDevice[] =
{
  {
    SB_MOCK_CDDEVICE_CLASSNAME,
    SB_MOCK_CDDEVICE_CID,
    SB_MOCK_CDDEVICE_CONTRACTID,
    sbMockCDDeviceConstructor,
  },
  {
    SB_MOCK_CDTOC_CLASSNAME,
    SB_MOCK_CDTOC_CID,
    SB_MOCK_CDTOC_CONTRACTID,
    sbMockCDTOCConstructor,
  },
  {
    SB_MOCK_CDDEVICECONTROLLER_CLASSNAME,
    SB_MOCK_CDDEVICECONTROLLER_CID,
    SB_MOCK_CDDEVICECONTROLLER_CONTRACTID,
    sbMockCDServiceConstructor
  }
};

NS_IMPL_NSGETMODULE(SongbirdMockCDDevice, sbMockCDDevice)
