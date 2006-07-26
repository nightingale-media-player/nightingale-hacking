/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the GPL).
// 
// Software distributed under the License is distributed 
// on an AS IS basis, WITHOUT WARRANTY OF ANY KIND, either 
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
* \file  sbUSBMassStorageDevice.cpp
* \brief Songbird USBMassStorageDevice Implementation.
*/
#include "nspr.h"

#include <time.h>

#include <xpcom/nscore.h>
#include <xpcom/nsXPCOM.h>
#include <xpcom/nsCOMPtr.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <necko/nsIURI.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <string/nsStringAPI.h>
#include "nsString.h"

#include "USBMassStorageDevice.h"

#include "sbIDatabaseResult.h"
#include "sbIDatabaseQuery.h"

#include "sbIMediaLibrary.h"
#include "sbIPlaylist.h"

#if defined(XP_WIN)
#include "objbase.h"
#include "win32/USBMassStorageDeviceWin32.h"
#else
#include "USBMassStorageDeviceHelper.h"
#endif

/* Implementation file */

#define NAME_USBMASSSTORAGE_DEVICE_LEN          NS_LITERAL_STRING("Songbird USBMassStorage Device").Length()
#define NAME_USBMASSSTORAGE_DEVICE              NS_LITERAL_STRING("Songbird USBMassStorage Device").get()

#define CONTEXT_USBMASSSTORAGE_DEVICE_LEN       NS_LITERAL_STRING("USBMassStorage").Length()
#define CONTEXT_USBMASSSTORAGE_DEVICE           NS_LITERAL_STRING("USBMassStorage").get()

#define USBMASSSTORAGE_DEVICE_TABLE_NAME        NS_LITERAL_STRING("usb_mass_storage").get()
#define USBMASSSTORAGE_DEVICE_TABLE_READABLE    NS_LITERAL_STRING("USB Mass Storage").get()
#define USBMASSSTORAGE_DEVICE_TABLE_DESCRIPTION NS_LITERAL_STRING("USB Mass Storage").get()
#define USBMASSSTORAGE_DEVICE_TABLE_TYPE        NS_LITERAL_STRING("USB Mass Storage").get()

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS2(sbUSBMassStorageDevice, sbIDeviceBase, sbIUSBMassStorageDevice)

//-----------------------------------------------------------------------------
sbUSBMassStorageDevice::sbUSBMassStorageDevice()
: sbDeviceBase(PR_FALSE)
, mDeviceState(kSB_DEVICE_STATE_IDLE)
, m_pHelperImpl(nsnull)
{
  PRBool retVal = PR_FALSE;

#if defined(XP_WIN)
  m_pHelperImpl = new CUSBMassStorageDeviceHelperWin32;
#else
  m_pHelperImpl = new CUSBMassStorageDeviceHelperStub;
#endif

  NS_ASSERTION(m_pHelperImpl != nsnull, "Failed to create USB Mass Storage Device Helper.");

} //ctor

//-----------------------------------------------------------------------------
sbUSBMassStorageDevice::~sbUSBMassStorageDevice() 
{
  PRBool retVal = PR_FALSE;
  Finalize(&retVal);
} //dtor

//-----------------------------------------------------------------------------
// sbIUSBMassStorageDevice
//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::OnUSBDeviceEvent(PRBool deviceAdded,
                                         const nsAString &deviceName,
                                         const nsAString &deviceIdentifier,
                                         PRBool *_retval)
{
  PRBool bInit = PR_FALSE;
  
  if(deviceAdded)
  {
    Initialize(&bInit);
    *_retval = m_pHelperImpl->Initialize(deviceName, deviceIdentifier);
  }
  else
  {
    
  }

  return NS_OK;
} //OnUSBDeviceEvent

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::Initialize(PRBool *_retval)
{
  *_retval = PR_TRUE;

  // Resume transfer if any pending
  //sbDeviceBase::ResumeAbortedTransfer(NULL);

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::Finalize(PRBool *_retval)
{
  StopCurrentTransfer(EmptyString());
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::AddCallback(sbIDeviceBaseCallback* aCallback,
                                    PRBool* _retval)
{
  return sbDeviceBase::AddCallback(aCallback, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::RemoveCallback(sbIDeviceBaseCallback* aCallback,
                                       PRBool* _retval)
{
  return sbDeviceBase::RemoveCallback(aCallback, _retval);
}

// ***************************
// sbIDeviceBase implementation 
// Just forwarding calls to mBaseDevice

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::SetName(const nsAString& aName)
{
  return sbDeviceBase::SetName(aName);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetDeviceStringByIndex(PRUint32 aIndex,
                                               nsAString& _retval)
{
  GetDeviceCategory(_retval);

  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetContext(const nsAString& aDeviceString,
                                   nsAString& _retval)
{
  _retval.Assign(CONTEXT_USBMASSSTORAGE_DEVICE);
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::IsDownloadSupported(const nsAString& aDeviceString,
                                            PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetSupportedFormats(const nsAString& aDeviceString,
                                            PRUint32 *_retval)
{
  return sbDeviceBase::GetSupportedFormats(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::IsUploadSupported(const nsAString& aDeviceString,
                                          PRBool *_retval)
{
  return sbDeviceBase::IsUploadSupported(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::IsTransfering(const nsAString& aDeviceString,
                                      PRBool *_retval)
{
  return sbDeviceBase::IsTransfering(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::IsDeleteSupported(const nsAString& aDeviceString,
                                          PRBool *_retval)
{
  return sbDeviceBase::IsDeleteSupported(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetUsedSpace(const nsAString& aDeviceString,
                                     PRUint32* _retval)
{
  return sbDeviceBase::GetUsedSpace(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetAvailableSpace(const nsAString& aDeviceString,
                                          PRUint32 *_retval)
{
  return sbDeviceBase::GetAvailableSpace(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetTrackTable(const nsAString& aDeviceString,
                                      nsAString& aDBContext,
                                      nsAString& aTableName,
                                      PRBool *_retval)
{
  return sbDeviceBase::GetTrackTable(aDeviceString, aDBContext, aTableName,
                                     _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::AbortTransfer(const nsAString& aDeviceString,
                                      PRBool *_retval)
{
  return sbDeviceBase::AbortTransfer(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::DeleteTable(const nsAString& aDeviceString,
                                    const nsAString& aDBContext,
                                    const nsAString& aTableName,
                                    PRBool *_retval)
{
  return sbDeviceBase::DeleteTable(aDeviceString, aDBContext, aTableName,
                                   _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::UpdateTable(const nsAString& aDeviceString,
                                    const nsAString& aTableName,
                                    PRBool *_retval)
{
  return sbDeviceBase::UpdateTable(aDeviceString, aTableName, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::EjectDevice(const nsAString& aDeviceString,
                                    PRBool *_retval)
{
  return sbDeviceBase::EjectDevice(aDeviceString, _retval);
}
/* End DeviceBase */

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetDeviceCategory(nsAString& aDeviceCategory)
{
  aDeviceCategory.Assign(NAME_USBMASSSTORAGE_DEVICE);
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetName(nsAString& aName)
{
  aName.Assign(NAME_USBMASSSTORAGE_DEVICE);
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRBool
sbUSBMassStorageDevice::TransferFile(PRUnichar* deviceString,
                                     PRUnichar* source,
                                     PRUnichar* destination,
                                     PRUnichar* dbContext,
                                     PRUnichar* table,
                                     PRUnichar* index,
                                     PRInt32 curDownloadRowNumber)
{
  return PR_TRUE;
}

//-----------------------------------------------------------------------------
void
sbUSBMassStorageDevice::OnThreadBegin()
{
}

//-----------------------------------------------------------------------------
void
sbUSBMassStorageDevice::OnThreadEnd()
{
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::IsUpdateSupported(const nsAString& aDeviceString,
                                          PRBool *_retval)
{
  return sbDeviceBase::IsUpdateSupported(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::IsEjectSupported(const nsAString& aDeviceString,
                                         PRBool *_retval)
{
  return sbDeviceBase::IsEjectSupported(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetDeviceCount(PRUint32* aDeviceCount)
{
  return sbDeviceBase::GetDeviceCount(aDeviceCount);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetDestinationCount(const nsAString& aDeviceString,
                                            PRUint32* _retval)
{
  return sbDeviceBase::GetDestinationCount(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::MakeTransferTable(const nsAString& aDeviceString,
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
 
//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::AutoDownloadTable(const nsAString& aDeviceString,
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
    RemoveExistingTransferTableEntries(nsnull, PR_TRUE);
  }

  return sbDeviceBase::AutoDownloadTable(aDeviceString, aContextInput,
                                         aTableName, aFilterColumn,
                                         aFilterCount, aFilterValues,
                                         aSourcePath, aDestPath,
                                         aTransferTable, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::AutoUploadTable(const nsAString& aDeviceString,
                                        const nsAString& aContextInput,
                                        const nsAString& aTableName,
                                        const nsAString& aFilterColumn,
                                        PRUint32 aFilterCount,
                                        const PRUnichar** aFilterValues,
                                        const nsAString& aSourcePath,
                                        const nsAString& aDestPath,
                                        nsAString& aTransferTable,
                                        PRBool *_retval)
{
  // XXXben Remove me
  nsAutoString strDevice(aDeviceString);

  if (!IsUploadInProgress(strDevice.get()))
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(nsnull, PR_FALSE);
  }

  return sbDeviceBase::AutoUploadTable(aDeviceString, aContextInput,
                                       aTableName, aFilterColumn,
                                       aFilterCount, aFilterValues,
                                       aSourcePath, aDestPath,
                                       aTransferTable, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::SuspendTransfer(const nsAString& aDeviceString,
                                        PRBool *_retval)
{
  sbDeviceBase::SuspendTransfer(aDeviceString, _retval);
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRBool
sbUSBMassStorageDevice::SuspendCurrentTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::ResumeTransfer(const nsAString& aDeviceString,
                                       PRBool *_retval)
{
  sbDeviceBase::ResumeTransfer(aDeviceString, _retval);
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRBool
sbUSBMassStorageDevice::ResumeTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetDeviceState(const nsAString& aDeviceString,
                                       PRUint32 *_retval)
{
  *_retval = mDeviceState;
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::RemoveTranferTracks(const nsAString& aDeviceString,
                                            PRUint32 aIndex,
                                            PRBool *_retval)
{
  return sbDeviceBase::RemoveTranferTracks(aDeviceString, aIndex, _retval);
}

//-----------------------------------------------------------------------------
PRBool
sbUSBMassStorageDevice::StopCurrentTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetDownloadTable(const nsAString& aDeviceString,
                                         nsAString& _retval)
{
  return sbDeviceBase::GetDownloadTable(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetUploadTable(const nsAString& aDeviceString,
                                       nsAString& _retval)
{
  return sbDeviceBase::GetUploadTable(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
// Transfer related
void
sbUSBMassStorageDevice::GetDeviceDownloadTableDescription(const nsAString& aDeviceString,
                                                          nsAString& _retval)
{ 
  _retval.Assign(USBMASSSTORAGE_DEVICE_TABLE_NAME); 
}

//-----------------------------------------------------------------------------
void
sbUSBMassStorageDevice::GetDeviceUploadTableDescription(const nsAString& aDeviceString,
                                                        nsAString& _retval)
{ 
  _retval.Assign(EmptyString()); 
}

//-----------------------------------------------------------------------------
void
sbUSBMassStorageDevice::GetDeviceDownloadTableType(const nsAString& aDeviceString,
                                                   nsAString& _retval)
{ 
  _retval.Assign(USBMASSSTORAGE_DEVICE_TABLE_NAME); 
}

//-----------------------------------------------------------------------------
void
sbUSBMassStorageDevice::GetDeviceUploadTableType(const nsAString& aDeviceString,
                                                 nsAString& _retval)
{ 
  _retval.Assign(EmptyString()); 
}

//-----------------------------------------------------------------------------
void
sbUSBMassStorageDevice::GetDeviceDownloadReadable(const nsAString& aDeviceString,
                                                  nsAString& _retval)
{ 
  _retval.Assign(USBMASSSTORAGE_DEVICE_TABLE_NAME); 
}

//-----------------------------------------------------------------------------
void
sbUSBMassStorageDevice::GetDeviceUploadTableReadable(const nsAString& aDeviceString,
                                                     nsAString& _retval)
{ 
  _retval.Assign(EmptyString()); 
}

//-----------------------------------------------------------------------------
void
sbUSBMassStorageDevice::GetDeviceDownloadTable(const nsAString& aDeviceString,
                                               nsAString& _retval)
{ 
  _retval.Assign(USBMASSSTORAGE_DEVICE_TABLE_NAME); 
}

//-----------------------------------------------------------------------------
void
sbUSBMassStorageDevice::GetDeviceUploadTable(const nsAString& aDeviceString,
                                             nsAString& _retval)
{ 
  _retval.Assign(EmptyString()); 
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::UploadTable(const nsAString& aDeviceString,
                                    const nsAString& aTableName,
                                    PRBool *_retval)
{
  return sbDeviceBase::UploadTable(aDeviceString, aTableName, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::DownloadTable(const nsAString& aDeviceString,
                                      const nsAString& aTableName,
                                      PRBool *_retval)
{
  return sbDeviceBase::DownloadTable(aDeviceString, aTableName, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::SetDownloadFileType(const nsAString& aDeviceString,
                                            PRUint32 aFileType,
                                            PRBool *_retval)
{
  return sbDeviceBase::SetDownloadFileType(aDeviceString, aFileType, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::SetUploadFileType(const nsAString& aDeviceString,
                                          PRUint32 aFileType,
                                          PRBool *_retval)
{
  return sbDeviceBase::SetUploadFileType(aDeviceString, aFileType, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetDownloadFileType(const nsAString& aDeviceString,
                                            PRUint32 *_retval)
{
  return sbDeviceBase::GetDownloadFileType(aDeviceString, _retval);
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP
sbUSBMassStorageDevice::GetUploadFileType(const nsAString& aDeviceString,
                                          PRUint32 *_retval)
{
  return sbDeviceBase::GetUploadFileType(aDeviceString, _retval);
}
