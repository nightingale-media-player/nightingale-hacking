/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include <nsCOMArray.h>

#include <sbArrayUtils.h>
#include <sbICDDevice.h>
#include <sbIMockCDDevice.h>
#include <sbIMockCDDeviceController.h>

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
sbMockCDTOC::GetStatus(PRUint16 *aStatus)
{
  NS_ENSURE_ARG_POINTER(aStatus);

  *aStatus = sbICDTOC::STATUS_OK;
  return NS_OK;
}

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
                     mDiscType(sbICDDevice::AUDIO_DISC_TYPE),
                     mEjected(PR_FALSE),
                     mIsDeviceLocked(PR_FALSE) {}

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
// XPCOM component registration
//==============================================================================

NS_GENERIC_FACTORY_CONSTRUCTOR(sbMockCDDevice)
NS_GENERIC_FACTORY_CONSTRUCTOR(sbMockCDTOC)

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
  }
};

NS_IMPL_NSGETMODULE(SongbirdMockCDDevice, sbMockCDDevice)
