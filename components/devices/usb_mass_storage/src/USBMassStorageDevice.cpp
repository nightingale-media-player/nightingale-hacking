/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright 2006 Pioneers of the Inevitable LLC
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

#if defined(XP_WIN)
#include "objbase.h"
#endif

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

#include "IDatabaseResult.h"
#include "IDatabaseQuery.h"

#include "IMediaLibrary.h"
#include "IPlaylist.h"

/* Implementation file */

#define NAME_USBMASSSTORAGE_DEVICE_LEN          NS_LITERAL_STRING("Songbird USBMassStorage Device").Length()
#define NAME_USBMASSSTORAGE_DEVICE              NS_LITERAL_STRING("Songbird USBMassStorage Device").get()

#define CONTEXT_USBMASSSTORAGE_DEVICE_LEN       NS_LITERAL_STRING("USBMassStorage").Length()
#define CONTEXT_USBMASSSTORAGE_DEVICE           NS_LITERAL_STRING("USBMassStorage").get()

#define USBMASSSTORAGE_DEVICE_TABLE_NAME        NS_LITERAL_STRING("usb_mass_storage").get()
#define USBMASSSTORAGE_DEVICE_TABLE_READABLE    NS_LITERAL_STRING("&device.download").get()
#define USBMASSSTORAGE_DEVICE_TABLE_DESCRIPTION NS_LITERAL_STRING("&device.download").get()
#define USBMASSSTORAGE_DEVICE_TABLE_TYPE        NS_LITERAL_STRING("&device.download").get()

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS2(sbUSBMassStorageDevice, sbIDeviceBase, sbIUSBMassStorageDevice)

//-----------------------------------------------------------------------------
sbUSBMassStorageDevice::sbUSBMassStorageDevice():
sbDeviceBase(PR_FALSE),
mDeviceState(kSB_DEVICE_STATE_IDLE)
{
  PRBool retVal = PR_FALSE;
} //ctor

//-----------------------------------------------------------------------------
sbUSBMassStorageDevice::~sbUSBMassStorageDevice() 
{
  PRBool retVal = PR_FALSE;
  Finalize(&retVal);
} //dtor

//-----------------------------------------------------------------------------
/* PRBool Initialize (); */
NS_IMETHODIMP sbUSBMassStorageDevice::Initialize(PRBool *_retval)
{
  // Resume transfer if any pending
  sbDeviceBase::ResumeAbortedTransfer(NULL);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRBool Finalize (); */
NS_IMETHODIMP sbUSBMassStorageDevice::Finalize(PRBool *_retval)
{
  StopCurrentTransfer(NULL);
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRBool AddCallback (in sbIDeviceBaseCallback pCallback); */
NS_IMETHODIMP sbUSBMassStorageDevice::AddCallback(sbIDeviceBaseCallback *pCallback, PRBool *_retval)
{
  return sbDeviceBase::AddCallback(pCallback, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool RemoveCallback (in sbIDeviceBaseCallback pCallback); */
NS_IMETHODIMP sbUSBMassStorageDevice::RemoveCallback(sbIDeviceBaseCallback *pCallback, PRBool *_retval)
{
  return sbDeviceBase::RemoveCallback(pCallback, _retval);
}

// ***************************
// sbIDeviceBase implementation 
// Just forwarding calls to mBaseDevice
//-----------------------------------------------------------------------------
/* attribute wstring name; */
NS_IMETHODIMP sbUSBMassStorageDevice::SetName(const PRUnichar * aName)
{
  return sbDeviceBase::SetName(aName);
}

//-----------------------------------------------------------------------------
/* wstring EnumDeviceString (in PRUint32 index); */
NS_IMETHODIMP sbUSBMassStorageDevice::EnumDeviceString(PRUint32 index, PRUnichar **_retval)
{
  // There is only one download device, so index is ignored
  GetDeviceCategory(_retval);

  return NS_OK;
}

//-----------------------------------------------------------------------------
/* wstring GetContext (in wstring deviceString); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetContext(const PRUnichar *deviceString, PRUnichar **_retval)
{
  size_t nLen = CONTEXT_USBMASSSTORAGE_DEVICE_LEN + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(CONTEXT_USBMASSSTORAGE_DEVICE, nLen * sizeof(PRUnichar));
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRBool IsDownloadSupported (); */
NS_IMETHODIMP sbUSBMassStorageDevice::IsDownloadSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRUint32 GetSupportedFormats (); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetSupportedFormats(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetSupportedFormats(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool IsUploadSupported (); */
NS_IMETHODIMP sbUSBMassStorageDevice::IsUploadSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsUploadSupported(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool IsTransfering (); */
NS_IMETHODIMP sbUSBMassStorageDevice::IsTransfering(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsTransfering(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool IsDeleteSupported (); */
NS_IMETHODIMP sbUSBMassStorageDevice::IsDeleteSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsDeleteSupported(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRUint32 GetUsedSpace (); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetUsedSpace(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetUsedSpace(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRUint32 GetAvailableSpace (); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetAvailableSpace(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetAvailableSpace(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool GetTrackTable (out wstring dbContext, out wstring tableName); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetTrackTable(const PRUnichar *deviceString, PRUnichar **dbContext, PRUnichar **tableName, PRBool *_retval)
{
  return sbDeviceBase::GetTrackTable(deviceString, dbContext, tableName, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool AbortTransfer (); */
NS_IMETHODIMP sbUSBMassStorageDevice::AbortTransfer(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::AbortTransfer(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool DeleteTable (in wstring dbContext, in wstring tableName); */
NS_IMETHODIMP sbUSBMassStorageDevice::DeleteTable(const PRUnichar *deviceString, const PRUnichar *dbContext, const PRUnichar *tableName, PRBool *_retval)
{
  return sbDeviceBase::DeleteTable(deviceString, dbContext, tableName, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool UpdateTable (in wstring dbContext, in wstring tableName); */
NS_IMETHODIMP sbUSBMassStorageDevice::UpdateTable(const PRUnichar *deviceString, const PRUnichar *tableName, PRBool *_retval)
{
  return sbDeviceBase::UpdateTable(deviceString, tableName, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool EjectDevice (); */
NS_IMETHODIMP sbUSBMassStorageDevice::EjectDevice(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::EjectDevice(deviceString, _retval);
}
/* End DeviceBase */

//-----------------------------------------------------------------------------
/* wstring GetDeviceCategory (); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetDeviceCategory(PRUnichar **_retval)
{
  size_t nLen = NAME_USBMASSSTORAGE_DEVICE_LEN + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(NAME_USBMASSSTORAGE_DEVICE, nLen * sizeof(PRUnichar));
  return NS_OK;
}

//-----------------------------------------------------------------------------
NS_IMETHODIMP sbUSBMassStorageDevice::GetName(PRUnichar **aName)
{
  size_t nLen = NAME_USBMASSSTORAGE_DEVICE_LEN + 1;
  *aName = (PRUnichar *) nsMemory::Clone(NAME_USBMASSSTORAGE_DEVICE, nLen * sizeof(PRUnichar));
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRBool sbUSBMassStorageDevice::TransferFile(PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber)
{
  return PR_TRUE;
}

//-----------------------------------------------------------------------------
void sbUSBMassStorageDevice::OnThreadBegin()
{
}

//-----------------------------------------------------------------------------
void sbUSBMassStorageDevice::OnThreadEnd()
{
}

//-----------------------------------------------------------------------------
/* PRBool IsUpdateSupported (); */
NS_IMETHODIMP sbUSBMassStorageDevice::IsUpdateSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsUpdateSupported(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool IsEjectSupported (); */
NS_IMETHODIMP sbUSBMassStorageDevice::IsEjectSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsEjectSupported(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRUint32 GetNumDevices (); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetNumDevices(PRUint32 *_retval)
{
  return sbDeviceBase::GetNumDevices(_retval);
}

//-----------------------------------------------------------------------------
/* wstring GetNumDestinations (in wstring DeviceString); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetNumDestinations(const PRUnichar *DeviceString, PRUnichar **_retval)
{
  return sbDeviceBase::GetNumDestinations(DeviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool MakeTransferTable (in wstring DeviceString, in wstring ContextInput, in wstring TableName, out wstring TransferTable); */
NS_IMETHODIMP sbUSBMassStorageDevice::MakeTransferTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, PRUnichar **TransferTable, PRBool *_retval)
{
  return sbDeviceBase::MakeTransferTable(DeviceString, ContextInput, TableName, TransferTable, _retval);
}
 
//-----------------------------------------------------------------------------
/* PRBool AutoDownloadTable (const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval); */
NS_IMETHODIMP sbUSBMassStorageDevice::AutoDownloadTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **FilterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval)
{
  if (!IsDownloadInProgress(DeviceString))
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(nsnull);
  }

  return sbDeviceBase::AutoDownloadTable(DeviceString, ContextInput, TableName, FilterColumn, FilterCount, FilterValues, sourcePath, destPath, TransferTable, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool AutoUploadTable (const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval); */
NS_IMETHODIMP sbUSBMassStorageDevice::AutoUploadTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **FilterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval)
{
  if (!IsUploadInProgress(DeviceString))
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(nsnull);
  }

  return sbDeviceBase::AutoUploadTable(DeviceString, ContextInput, TableName, FilterColumn, FilterCount, FilterValues, sourcePath, destPath, TransferTable, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool SuspendTransfer (); */
NS_IMETHODIMP sbUSBMassStorageDevice::SuspendTransfer(const PRUnichar* deviceString, PRBool *_retval)
{
  sbDeviceBase::SuspendTransfer(deviceString, _retval);
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRBool sbUSBMassStorageDevice::SuspendCurrentTransfer(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

//-----------------------------------------------------------------------------
/* PRBool ResumeTransfer (); */
NS_IMETHODIMP sbUSBMassStorageDevice::ResumeTransfer(const PRUnichar* deviceString, PRBool *_retval)
{
  sbDeviceBase::ResumeTransfer(deviceString, _retval);
  return NS_OK;
}

//-----------------------------------------------------------------------------
PRBool sbUSBMassStorageDevice::ResumeTransfer(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

//-----------------------------------------------------------------------------
/* PRUint32 GetDeviceState (); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetDeviceState(const PRUnichar *deviceString, PRUint32 *_retval)
{
  *_retval = mDeviceState;
  return NS_OK;
}

//-----------------------------------------------------------------------------
/* PRBool RemoveTranferTracks (in PRUint32 index); */
NS_IMETHODIMP sbUSBMassStorageDevice::RemoveTranferTracks(const PRUnichar *deviceString, PRUint32 index, PRBool *_retval)
{
  return sbDeviceBase::RemoveTranferTracks(deviceString, index, _retval);
}

//-----------------------------------------------------------------------------
PRBool sbUSBMassStorageDevice::StopCurrentTransfer(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

//-----------------------------------------------------------------------------
/* wstring GetDownloadTable (in wstring deviceString); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetDownloadTable(const PRUnichar *deviceString, PRUnichar **_retval)
{
  return sbDeviceBase::GetDownloadTable(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* wstring GetUploadTable (in wstring deviceString); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetUploadTable(const PRUnichar *deviceString, PRUnichar **_retval)
{
  return sbDeviceBase::GetUploadTable(deviceString, _retval);
}

//-----------------------------------------------------------------------------
// Transfer related
nsString sbUSBMassStorageDevice::GetDeviceDownloadTableDescription(const PRUnichar* deviceString)
{ 
  return nsString(USBMASSSTORAGE_DEVICE_TABLE_DESCRIPTION); 
}

//-----------------------------------------------------------------------------
nsString sbUSBMassStorageDevice::GetDeviceUploadTableDescription(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

//-----------------------------------------------------------------------------
nsString sbUSBMassStorageDevice::GetDeviceDownloadTableType(const PRUnichar* deviceString)
{ 
  return nsString(USBMASSSTORAGE_DEVICE_TABLE_TYPE); 
}

//-----------------------------------------------------------------------------
nsString sbUSBMassStorageDevice::GetDeviceUploadTableType(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

//-----------------------------------------------------------------------------
nsString sbUSBMassStorageDevice::GetDeviceDownloadReadable(const PRUnichar* deviceString)
{ 
  return nsString(USBMASSSTORAGE_DEVICE_TABLE_READABLE); 
}

//-----------------------------------------------------------------------------
nsString sbUSBMassStorageDevice::GetDeviceUploadTableReadable(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

//-----------------------------------------------------------------------------
nsString sbUSBMassStorageDevice::GetDeviceDownloadTable(const PRUnichar* deviceString)
{ 
  return nsString(USBMASSSTORAGE_DEVICE_TABLE_NAME); 
}

//-----------------------------------------------------------------------------
nsString sbUSBMassStorageDevice::GetDeviceUploadTable(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

//-----------------------------------------------------------------------------
/* PRBool DownloadTable (in wstring DeviceCategory, in wstring DeviceString, in wstring TableName); */
NS_IMETHODIMP sbUSBMassStorageDevice::UploadTable(const PRUnichar *DeviceString, const PRUnichar *TableName, PRBool *_retval)
{
  return sbDeviceBase::UploadTable(DeviceString, TableName, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool DownloadTable (in wstring DeviceCategory, in wstring DeviceString, in wstring TableName); */
NS_IMETHODIMP sbUSBMassStorageDevice::DownloadTable(const PRUnichar *DeviceString, const PRUnichar *TableName, PRBool *_retval)
{
  return sbDeviceBase::DownloadTable(DeviceString, TableName, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool SetDownloadFileType (in PRUint32 fileType); */
NS_IMETHODIMP sbUSBMassStorageDevice::SetDownloadFileType(const PRUnichar *deviceString, PRUint32 fileType, PRBool *_retval)
{
  return sbDeviceBase::SetDownloadFileType(deviceString, fileType, _retval);
}

//-----------------------------------------------------------------------------
/* PRBool SetUploadFileType (in PRUint32 fileType); */
NS_IMETHODIMP sbUSBMassStorageDevice::SetUploadFileType(const PRUnichar *deviceString, PRUint32 fileType, PRBool *_retval)
{
  return sbDeviceBase::SetUploadFileType(deviceString, fileType, _retval);
}

//-----------------------------------------------------------------------------
/* PRUint32 GetDownloadFileType (in wstring deviceString); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetDownloadFileType(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetDownloadFileType(deviceString, _retval);
}

//-----------------------------------------------------------------------------
/* PRUint32 GetUploadFileType (in wstring deviceString); */
NS_IMETHODIMP sbUSBMassStorageDevice::GetUploadFileType(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetUploadFileType(deviceString, _retval);
}
