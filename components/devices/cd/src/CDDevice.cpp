/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

NS_IMPL_THREADSAFE_ISUPPORTS2(sbCDDevice, sbIDeviceBase, sbICDDevice)

sbCDDevice::sbCDDevice() :
  sbDeviceBase(PR_TRUE),
  mCDManagerObject(nsnull)
{
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
  mCDManagerObject = new WinCDDeviceManager();
  NS_ENSURE_TRUE(mCDManagerObject, NS_ERROR_OUT_OF_MEMORY);

  // Post a message to handle this event asynchronously
  mCDManagerObject->Initialize((void*)this);
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
sbCDDevice::AddCallback(sbIDeviceBaseCallback* aCallback,
                        PRBool* _retval)
{
  return sbDeviceBase::AddCallback(aCallback, _retval);
}

NS_IMETHODIMP
sbCDDevice::RemoveCallback(sbIDeviceBaseCallback* aCallback,
                           PRBool* _retval)
{
  return sbDeviceBase::RemoveCallback(aCallback, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetDeviceStringByIndex(PRUint32 aIndex,
                                   nsAString& _retval)
{
  mCDManagerObject->GetDeviceStringByIndex(aIndex, _retval);

  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::SetName(const nsAString& aName)
{
  return sbDeviceBase::SetName(aName);
}

NS_IMETHODIMP
sbCDDevice::GetContext(const nsAString& aDeviceString,
                       nsAString& _retval)
{
  mCDManagerObject->GetContext(aDeviceString, _retval);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::IsDownloadSupported(const nsAString& aDeviceString,
                                PRBool* _retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetSupportedFormats(const nsAString& aDeviceString,
                                PRUint32* _retval)
{
  return sbDeviceBase::GetSupportedFormats(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::IsUploadSupported(const nsAString& aDeviceString,
                              PRBool* _retval)
{
  *_retval = mCDManagerObject->IsUploadSupported(aDeviceString);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::IsTransfering(const nsAString& aDeviceString,
                          PRBool* _retval)
{
  return sbDeviceBase::IsTransfering(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::IsDeleteSupported(const nsAString& aDeviceString,
                              PRBool* _retval)
{
  return sbDeviceBase::IsDeleteSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetUsedSpace(const nsAString& aDeviceString,
                         PRUint32* _retval)
{
  return sbDeviceBase::GetUsedSpace(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetAvailableSpace(const nsAString& aDeviceString,
                              PRUint32* _retval)
{
  *_retval = mCDManagerObject->GetAvailableSpace(aDeviceString);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetTrackTable(const nsAString& aDeviceString,
                          nsAString& aDBContext,
                          nsAString& aTableName,
                          PRBool* _retval)
{
  *_retval = mCDManagerObject->GetTrackTable(aDeviceString, aDBContext,
                                             aTableName);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::AbortTransfer(const nsAString& aDeviceString,
                          PRBool* _retval)
{
  return sbDeviceBase::AbortTransfer(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::DeleteTable(const nsAString& aDeviceString,
                        const nsAString& aDBContext,
                        const nsAString& aTableName,
                        PRBool* _retval)
{
  return sbDeviceBase::DeleteTable(aDeviceString, aDBContext, aTableName,
                                   _retval);
}

NS_IMETHODIMP
sbCDDevice::UpdateTable(const nsAString& aDeviceString,
                        const nsAString& aTableName,
                        PRBool* _retval)
{
  return sbDeviceBase::UpdateTable(aDeviceString, aTableName, _retval);
}

NS_IMETHODIMP
sbCDDevice::EjectDevice(const nsAString& aDeviceString,
                        PRBool* _retval)
{
  return sbDeviceBase::EjectDevice(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetDeviceCategory(nsAString& aDeviceCategory)
{
  aDeviceCategory.AssignLiteral(NAME_CDDEVICE);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetName(nsAString& aName)
{
  aName.AssignLiteral(NAME_CDDEVICE);
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
sbCDDevice::IsUpdateSupported(const nsAString& aDeviceString,
                              PRBool* _retval)
{
  return sbDeviceBase::IsUpdateSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::IsEjectSupported(const nsAString& aDeviceString,
                             PRBool* _retval)
{
  return sbDeviceBase::IsEjectSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetDeviceCount(PRUint32* aDeviceCount)
{
  return sbDeviceBase::GetDeviceCount(aDeviceCount);
}

NS_IMETHODIMP
sbCDDevice::GetDestinationCount(const nsAString& aDeviceString,
                                PRUint32* _retval)
{
  return sbDeviceBase::GetDestinationCount(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::MakeTransferTable(const nsAString& aDeviceString,
                              const nsAString& aContextInput,
                              const nsAString& aTableName,
                              const nsAString& aFilterColumn,
                              PRUint32 aFilterCount,
                              const PRUnichar** aFilterValues,
                              const nsAString& aSourcePath,
                              const nsAString& aDestPath,
                              PRBool aDownloading,
                              nsAString& aTransferTable,
                              PRBool* _retval)
{
  return sbDeviceBase::MakeTransferTable(aDeviceString, aContextInput,
                                         aTableName, aFilterColumn,
                                         aFilterCount, aFilterValues,
                                         aSourcePath, aDestPath,
                                         aDownloading, aTransferTable,
                                         _retval);
}

NS_IMETHODIMP
sbCDDevice::SuspendTransfer(const nsAString& aDeviceString,
                            PRBool* _retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::ResumeTransfer(const nsAString& aDeviceString,
                           PRBool* _retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetDeviceState(const nsAString& aDeviceString,
                           PRUint32* _retval)
{
  return sbDeviceBase::GetDeviceState(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::RemoveTranferTracks(const nsAString& aDeviceString,
                                PRUint32 aIndex,
                                PRBool* _retval)
{
  return sbDeviceBase::RemoveTranferTracks(aDeviceString, aIndex, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetDownloadTable(const nsAString& aDeviceString,
                             nsAString& _retval)
{
  return sbDeviceBase::GetDownloadTable(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetUploadTable(const nsAString& aDeviceString,
                           nsAString& _retval)
{
  return sbDeviceBase::GetUploadTable(aDeviceString, _retval);
}

NS_IMETHODIMP
sbCDDevice::AutoDownloadTable(const nsAString& aDeviceString,
                              const nsAString& aContextInput,
                              const nsAString& aTableName,
                              const nsAString& aFilterColumn,
                              PRUint32 aFilterCount,
                              const PRUnichar** aFilterValues,
                              const nsAString& aSourcePath,
                              const nsAString& aDestPath,
                              nsAString& aTransferTable,
                              PRBool* _retval)
{
  // XXXben Remove me
  nsAutoString str(aDeviceString);

  if (!IsDownloadInProgress(str.get()))
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(str.get(), PR_TRUE);
  }

  return sbDeviceBase::AutoDownloadTable(aDeviceString, aContextInput, aTableName,
                                         aFilterColumn, aFilterCount,
                                         aFilterValues, aSourcePath, aDestPath,
                                         aTransferTable, _retval);
}

NS_IMETHODIMP
sbCDDevice::AutoUploadTable(const nsAString& aDeviceString,
                            const nsAString& aContextInput,
                            const nsAString& aTableName,
                            const nsAString& aFilterColumn,
                            PRUint32 aFilterCount,
                            const PRUnichar** aFilterValues,
                            const nsAString& aSourcePath,
                            const nsAString& aDestPath,
                            nsAString& aTransferTable,
                            PRBool* _retval)
{
  // XXXben Remove me
  nsAutoString strDevice(aDeviceString);

  if (IsDeviceIdle(strDevice.get()))
  {
    RemoveExistingTransferTableEntries(strDevice.get(), PR_FALSE);
    sbDeviceBase::AutoUploadTable(aDeviceString, aContextInput, aTableName,
                                  aFilterColumn, aFilterCount, aFilterValues,
                                  aSourcePath, aDestPath, aTransferTable,
                                  _retval);
  }

  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::UploadTable(const nsAString& aDeviceString,
                        const nsAString& aTableName,
                        PRBool* _retval)
{
  *_retval = mCDManagerObject->UploadTable(aDeviceString, aTableName);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::DownloadTable(const nsAString& aDeviceString,
                          const nsAString& aTableName,
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
  mCDManagerObject->Initialize((void*)this);
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

void
sbCDDevice::GetDeviceDownloadTableDescription(const nsAString& aDeviceString,
                                              nsAString& _retval)
{ 
  nsXPIDLString string;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_RIP_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(),
                                    getter_Copies(string));
  _retval.Assign(string);
}

void
sbCDDevice::GetDeviceUploadTableDescription(const nsAString& aDeviceString,
                                            nsAString& _retval)
{ 
  nsXPIDLString string;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_BURN_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(),
                                    getter_Copies(string));
  _retval.Assign(string);
}

void
sbCDDevice::GetDeviceDownloadTableType(const nsAString& aDeviceString,
                                       nsAString& _retval)
{ 
  nsXPIDLString string;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_RIP_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(),
                                    getter_Copies(string));
  _retval.Assign(string);
}

void
sbCDDevice::GetDeviceUploadTableType(const nsAString& aDeviceString,
                                     nsAString& _retval)
{ 
  nsXPIDLString string;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_RIP_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(),
                                    getter_Copies(string));
  _retval.Assign(string);
}

void
sbCDDevice::GetDeviceDownloadReadable(const nsAString& aDeviceString,
                                      nsAString& _retval)
{ 
  nsXPIDLString string;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_RIP_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(),
                                    getter_Copies(string));
  _retval.Assign(string);
}

void
sbCDDevice::GetDeviceUploadTableReadable(const nsAString& aDeviceString,
                                         nsAString& _retval)
{ 
  nsXPIDLString string;

  NS_NAMED_LITERAL_STRING(bundleString, CDDEVICE_BURN_TABLE_READABLE);
  m_StringBundle->GetStringFromName(bundleString.get(),
                                    getter_Copies(string));
  _retval.Assign(string);
}

void
sbCDDevice::GetDeviceDownloadTable(const nsAString& aDeviceString,
                                   nsAString& _retval)
{ 
  _retval.AssignLiteral(CDDEVICE_RIP_TABLE);
}

void
sbCDDevice::GetDeviceUploadTable(const nsAString& aDeviceString,
                                 nsAString& _retval)
{ 
  _retval.AssignLiteral(CDDEVICE_BURN_TABLE); 
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
sbCDDevice::StopCurrentTransfer(const nsAString& aDeviceString)
{
  return mCDManagerObject->StopCurrentTransfer(aDeviceString);
}

PRBool
sbCDDevice::SuspendCurrentTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

PRBool
sbCDDevice::ResumeTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

NS_IMETHODIMP
sbCDDevice::SetDownloadFileType(const nsAString& aDeviceString,
                                PRUint32 aFileType,
                                PRBool* _retval)
{
  *_retval = mCDManagerObject->SetCDRipFormat(aDeviceString, aFileType);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::SetUploadFileType(const nsAString& aDeviceString,
                              PRUint32 aFileType,
                              PRBool* _retval)
{
  return sbDeviceBase::SetUploadFileType(aDeviceString, aFileType, _retval);
}

NS_IMETHODIMP
sbCDDevice::GetDownloadFileType(const nsAString& aDeviceString,
                                PRUint32* _retval)
{
  *_retval = mCDManagerObject->GetCDRipFormat(aDeviceString);
  return NS_OK;
}

NS_IMETHODIMP
sbCDDevice::GetUploadFileType(const nsAString& aDeviceString,
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
