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

/** 
* \file  sbCDDevice.cpp
* \brief Songbird CDDevice Component Implementation.
* \sa    sbICDDevice.idl, sbIDeviceBase.idl, CDDevice.h
*/

#include "CDDevice.h"
#include "nspr.h"
#include "objbase.h"

#include <xpcom/nscore.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <necko/nsIURI.h>
#include <webbrowserpersist/nsIWebBrowserPersist.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <string/nsString.h>
#include "nsIWebProgressListener.h"
#include "nsNetUtil.h"
#include <nsIStringBundle.h>

#include "win32/WinCDImplementation.h"

#define NAME_CDDEVICE                "Songbird CD Device"
#define CDDEVICE_TABLE_NAME          "compactDisc"
#define CDDEVICE_TABLE_READABLE      "compactDisc.trackstable.readable"
#define CDDEVICE_TABLE_DESCRIPTION   "compactDisc.trackstable.description"
#define CDDEVICE_TABLE_TYPE          "compactDisc.trackstable.type"
#define CDDEVICE_RIP_TABLE           "CDRip"
#define CDDEVICE_RIP_TABLE_READABLE  "compactDisc.cdrip"
#define CDDEVICE_BURN_TABLE          "CDBurn"
#define CDDEVICE_BURN_TABLE_READABLE "compactDisc.cdburn"

#define SONGBIRD_PROPERTIES "chrome://songbird/locale/songbird.properties"

NS_IMPL_ISUPPORTS2(sbCDDevice, sbIDeviceBase, sbICDDevice)

sbCDDevice::sbCDDevice()
: sbDeviceBase(PR_TRUE)
{
  mCDManagerObject = new WinCDDeviceManager(this);

  // Get the string bundle for our strings
  if (!m_StringBundle) {
    nsresult rv;
    nsCOMPtr<nsIStringBundleService> stringBundleService =
      do_GetService("@mozilla.org/intl/stringbundle;1", &rv);

    if (NS_FAILED(rv)) {
      NS_WARNING("Failed to get StringBundleService");
      return;
    }

    rv = stringBundleService->CreateBundle(SONGBIRD_PROPERTIES,
                                           getter_AddRefs(m_StringBundle));
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to create bundle");
  }

}

sbCDDevice::~sbCDDevice() 
{
  delete mCDManagerObject;
}

NS_IMETHODIMP
sbCDDevice::Initialize(PRBool* _retval)
{
  // Post a message to handle this event asynchronously
  mCDManagerObject->Initialize();
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::Finalize(PRBool* _retval)
{
  // Post a message to handle this event asynchronously
  FinalizeAsync();
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::AddCallback(sbIDeviceBaseCallback *pCallback,
                        PRBool* _retval)
{
  return sbDeviceBase::AddCallback(pCallback, _retval);
}

NS_IMETHODIMP
sbCDDevice::RemoveCallback(sbIDeviceBaseCallback *pCallback,
                           PRBool* _retval)
{
  return sbDeviceBase::RemoveCallback(pCallback, _retval);
}

NS_IMETHODIMP
sbCDDevice::EnumDeviceString(PRUint32 aIndex,
                             PRUnichar** _retval)
{
  *_retval = mCDManagerObject->EnumDeviceString(aIndex);

  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::SetName(const PRUnichar* aName)
{
  return sbDeviceBase::SetName(aName);
}

NS_IMETHODIMP
sbCDDevice::GetContext(const PRUnichar* aDeviceString,
                       PRUnichar** _retval)
{
  *_retval = mCDManagerObject->GetContext(aDeviceString);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::IsDownloadSupported(const PRUnichar* aDeviceString,
                                PRBool* _retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetSupportedFormats(const PRUnichar* aDeviceString,
                                PRUint32* _retval)
{
  return sbDeviceBase::GetSupportedFormats(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::IsUploadSupported(const PRUnichar* aDeviceString,
                              PRBool* _retval)
{
  *_retval = mCDManagerObject->IsUploadSupported(aDeviceString);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::IsTransfering(const PRUnichar* aDeviceString,
                          PRBool* _retval)
{
  return sbDeviceBase::IsTransfering(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::IsDeleteSupported(const PRUnichar* aDeviceString,
                              PRBool* _retval)
{
  return sbDeviceBase::IsDeleteSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetUsedSpace(const PRUnichar* aDeviceString,
                         PRUint32* _retval)
{
  return sbDeviceBase::GetUsedSpace(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetAvailableSpace(const PRUnichar* aDeviceString,
                              PRUint32* _retval)
{
  *_retval = mCDManagerObject->GetAvailableSpace(aDeviceString);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetTrackTable(const PRUnichar* aDeviceString,
                          PRUnichar** aDBContext,
                          PRUnichar** aTableName,
                          PRBool* _retval)
{
  *_retval = mCDManagerObject->GetTrackTable(aDeviceString, aDBContext,
                                             aTableName);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::AbortTransfer(const PRUnichar* aDeviceString,
                          PRBool* _retval)
{
  return sbDeviceBase::AbortTransfer(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::DeleteTable(const PRUnichar* aDeviceString,
                        const PRUnichar* aDBContext,
                        const PRUnichar* aTableName,
                        PRBool* _retval)
{
  return sbDeviceBase::DeleteTable(aDeviceString, aDBContext, aTableName,
                                   _retval);
}

NS_IMETHODIMP
sbCDDevice::UpdateTable(const PRUnichar* aDeviceString,
                        const PRUnichar* aTableName,
                        PRBool* _retval)
{
  return sbDeviceBase::UpdateTable(aDeviceString, aTableName, _retval);
}

NS_IMETHODIMP
sbCDDevice::EjectDevice(const PRUnichar* aDeviceString,
                        PRBool* _retval)
{
  return sbDeviceBase::EjectDevice(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetDeviceCategory(PRUnichar** _retval)
{
  *_retval = ToNewUnicode(NS_LITERAL_STRING(NAME_CDDEVICE));
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetName(PRUnichar** aName)
{
  *aName = ToNewUnicode(NS_LITERAL_STRING(NAME_CDDEVICE));
  return NS_OK;
}

PRBool
sbCDDevice::IsEjectSupported() 
{
  return PR_FALSE;
}

// XXXben These two Co* calls shouldn't be here!

void
sbCDDevice::OnThreadBegin()
{
  CoInitializeEx(0, COINIT_MULTITHREADED);
}

void
sbCDDevice::OnThreadEnd()
{
  CoUninitialize();
}

NS_IMETHODIMP
sbCDDevice::IsUpdateSupported(const PRUnichar* aDeviceString,
                              PRBool* _retval)
{
  return sbDeviceBase::IsUpdateSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::IsEjectSupported(const PRUnichar* aDeviceString,
                             PRBool* _retval)
{
  return sbDeviceBase::IsEjectSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetNumDevices(PRUint32* _retval)
{
  return sbDeviceBase::GetNumDevices(_retval);
}

NS_IMETHODIMP
sbCDDevice::GetNumDestinations(const PRUnichar* aDeviceString,
                               PRUnichar** _retval)
{
  return sbDeviceBase::GetNumDestinations(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::MakeTransferTable(const PRUnichar* aDeviceString,
                              const PRUnichar* aContextInput,
                              const PRUnichar* aTableName,
                              const PRUnichar* aFilterColumn,
                              PRUint32 aFilterCount,
                              const PRUnichar** aFilterValues,
                              const PRUnichar* aSourcePath,
                              const PRUnichar* aDestPath,
                              PRBool aDownloading,
                              PRUnichar** aTransferTableName,
                              PRBool* _retval)
{
  return sbDeviceBase::MakeTransferTable(aDeviceString, aContextInput, aTableName,
                                         aFilterColumn, aFilterCount,
                                         aFilterValues, aSourcePath, aDestPath,
                                         aDownloading, aTransferTableName,
                                         _retval);
}

NS_IMETHODIMP
sbCDDevice::SuspendTransfer(const PRUnichar* aDeviceString,
                            PRBool* _retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::ResumeTransfer(const PRUnichar* aDeviceString,
                           PRBool* _retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetDeviceState(const PRUnichar* aDeviceString,
                           PRUint32* _retval)
{
  return sbDeviceBase::GetDeviceState(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::RemoveTranferTracks(const PRUnichar* aDeviceString,
                                PRUint32 aIndex,
                                PRBool* _retval)
{
  return sbDeviceBase::RemoveTranferTracks(aDeviceString, aIndex, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetDownloadTable(const PRUnichar* aDeviceString,
                             PRUnichar** _retval)
{
  return sbDeviceBase::GetDownloadTable(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetUploadTable(const PRUnichar* aDeviceString,
                           PRUnichar** _retval)
{
  return sbDeviceBase::GetUploadTable(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::AutoDownloadTable(const PRUnichar* aDeviceString,
                              const PRUnichar* aContextInput,
                              const PRUnichar* aTableName,
                              const PRUnichar* aFilterColumn,
                              PRUint32 aFilterCount,
                              const PRUnichar** aFilterValues,
                              const PRUnichar* aSourcePath,
                              const PRUnichar* aDestPath,
                              PRUnichar** aTransferTable,
                              PRBool* _retval)
{
  if (!IsDownloadInProgress(aDeviceString))
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(aDeviceString, PR_TRUE);
  }

  return sbDeviceBase::AutoDownloadTable(aDeviceString, aContextInput, aTableName,
                                         aFilterColumn, aFilterCount,
                                         aFilterValues, aSourcePath, aDestPath,
                                         aTransferTable, _retval);
}

NS_IMETHODIMP
sbCDDevice::AutoUploadTable(const PRUnichar* aDeviceString,
                            const PRUnichar* aContextInput,
                            const PRUnichar* aTableName,
                            const PRUnichar* aFilterColumn,
                            PRUint32 aFilterCount,
                            const PRUnichar** aFilterValues,
                            const PRUnichar* aSourcePath,
                            const PRUnichar* aDestPath,
                            PRUnichar** aTransferTable,
                            PRBool* _retval)
{
  if (IsDeviceIdle(aDeviceString))
  {
    RemoveExistingTransferTableEntries(aDeviceString, PR_FALSE);
    sbDeviceBase::AutoUploadTable(aDeviceString, aContextInput, aTableName,
                                  aFilterColumn, aFilterCount, aFilterValues,
                                  aSourcePath, aDestPath, aTransferTable,
                                  _retval);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::UploadTable(const PRUnichar* aDeviceString,
                        const PRUnichar* aTableName,
                        PRBool* _retval)
{
  *_retval = mCDManagerObject->UploadTable(aDeviceString, aTableName);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::DownloadTable(const PRUnichar* aDeviceString,
                          const PRUnichar* aTableName,
                          PRBool* _retval)
{
  return sbDeviceBase::DownloadTable(aDeviceString, aTableName, _retval);
}

NS_IMETHODIMP
sbCDDevice::OnCDDriveEvent(PRBool aMediaInserted,
                           PRBool* _retval)
{
  // Post a message to handle this event asynchronously
  DeviceEventAsync(aMediaInserted);
  return NS_OK;
}

PRBool
sbCDDevice::InitializeSync()
{
  mCDManagerObject->Initialize();
  return PR_TRUE;
}

PRBool
sbCDDevice::FinalizeSync()
{
  mCDManagerObject->Finalize();
  return PR_TRUE;
}

PRBool
sbCDDevice::DeviceEventSync(PRBool aMediaInserted)
{
  return mCDManagerObject->OnCDDriveEvent(aMediaInserted);
}

nsString sbCDDevice::GetDeviceDownloadTableDescription(const PRUnichar* aDeviceString)
{ 
  nsString displayString;
  PRUnichar* value = nsnull;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_RIP_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(), &value);

  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceUploadTableDescription(const PRUnichar* aDeviceString)
{ 
  nsString displayString;
  PRUnichar* value = nsnull;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_BURN_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(), &value);

  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceDownloadTableType(const PRUnichar* aDeviceString)
{ 
  nsString displayString;
  PRUnichar* value = nsnull;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_RIP_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(), &value);

  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceUploadTableType(const PRUnichar* aDeviceString)
{ 
  nsString displayString;
  PRUnichar* value = nsnull;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_RIP_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(), &value);

  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceDownloadReadable(const PRUnichar* aDeviceString)
{ 
  nsString displayString;
  PRUnichar* value = nsnull;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_RIP_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(), &value);

  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceUploadTableReadable(const PRUnichar* aDeviceString)
{ 
  nsString displayString;
  PRUnichar* value = nsnull;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_BURN_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(), &value);

  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceDownloadTable(const PRUnichar* aDeviceString)
{ 
  return nsString(NS_LITERAL_STRING(CDDEVICE_RIP_TABLE));
}

nsString sbCDDevice::GetDeviceUploadTable(const PRUnichar* aDeviceString)
{ 
  return nsString(NS_LITERAL_STRING(CDDEVICE_BURN_TABLE)); 
}

// XXXben Why isn't this aDeviceString const like the rest?
PRBool
sbCDDevice::TransferFile(PRUnichar* aDeviceString,
                         PRUnichar* aSource,
                         PRUnichar* aDestination,
                         PRUnichar* aDBContext,
                         PRUnichar* aTable,
                         PRUnichar* aIndex,
                         PRInt32 aCurDownloadRowNumber)
{
  return mCDManagerObject->TransferFile(aDeviceString, aSource, aDestination,
                                        aDBContext, aTable, aIndex,
                                        aCurDownloadRowNumber);
}

PRBool
sbCDDevice::StopCurrentTransfer(const PRUnichar* aDeviceString)
{
  return mCDManagerObject->StopCurrentTransfer(aDeviceString);
}

PRBool
sbCDDevice::SuspendCurrentTransfer(const PRUnichar* aDeviceString)
{
  return PR_FALSE;
}

PRBool
sbCDDevice::ResumeTransfer(const PRUnichar* aDeviceString)
{
  return PR_FALSE;
}

NS_IMETHODIMP
sbCDDevice::SetDownloadFileType(const PRUnichar* aDeviceString,
                                PRUint32 aFileType,
                                PRBool* _retval)
{
  *_retval = mCDManagerObject->SetCDRipFormat(aDeviceString, aFileType);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::SetUploadFileType(const PRUnichar* aDeviceString,
                              PRUint32 aFileType,
                              PRBool* _retval)
{
  return sbDeviceBase::SetUploadFileType(aDeviceString, aFileType, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetDownloadFileType(const PRUnichar* aDeviceString,
                                PRUint32* _retval)
{
  *_retval = mCDManagerObject->GetCDRipFormat(aDeviceString);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetUploadFileType(const PRUnichar* aDeviceString,
                              PRUint32* _retval)
{
  return sbDeviceBase::GetUploadFileType(aDeviceString, _retval);
}

PRUint32
sbCDDevice::GetCurrentTransferRowNumber(const PRUnichar* aDeviceString)
{
  return mCDManagerObject->GetCurrentTransferRowNumber(aDeviceString);
}

void
sbCDDevice::TransferComplete(const PRUnichar* aDeviceString)
{
  mCDManagerObject->TransferComplete(aDeviceString);
}

NS_IMETHODIMP
sbCDDevice::SetGapBurnedTrack(const nsAString& aDeviceString,
                              PRUint32 aNumSeconds,
                              PRBool* _retval)
{
  *_retval = mCDManagerObject->
    SetGapBurnedTrack(PromiseFlatString(aDeviceString).get(), aNumSeconds);
  return NS_OK;
}


NS_IMETHODIMP
sbCDDevice::GetWritableCDDrive(nsAString& aDeviceString,
                               PRBool* _retval)
{
  *_retval = mCDManagerObject->GetWritableCDDrive(aDeviceString);
  return NS_OK;
}
