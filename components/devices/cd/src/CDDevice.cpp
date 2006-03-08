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

/** 
* \file  sbCDDevice.cpp
* \Compact Disc device Component Implementation.
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
#include <string/nsStringAPI.h>
#include "NSString.h"
#include "nsIWebProgressListener.h"
#include "nsNetUtil.h"

/* Implementation file */

#define NAME_COMPACT_DISC_DEVICE_LEN  NS_LITERAL_STRING("Songbird CD Device").Length()
#define NAME_COMPACT_DISC_DEVICE      NS_LITERAL_STRING("Songbird CD Device").get()

#define COMPACT_DISC_DEVICE_TABLE_NAME        NS_LITERAL_STRING("compactDisc").get()
#define COMPACT_DISC_DEVICE_TABLE_READABLE    NS_LITERAL_STRING("&compactDisc.download").get()
#define COMPACT_DISC_DEVICE_TABLE_DESCRIPTION NS_LITERAL_STRING("compactDisc").get()
#define COMPACT_DISC_DEVICE_TABLE_TYPE        NS_LITERAL_STRING("compactDisc").get()


// CLASSES ====================================================================
NS_IMPL_ISUPPORTS2(sbCDDevice, sbIDeviceBase, sbICDDevice)

//-----------------------------------------------------------------------------
sbCDDevice::sbCDDevice()
: sbDeviceBase(PR_TRUE)
,mCDManagerObject(this)
{
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ sbCDDevice::~sbCDDevice() 
{
} //dtor

// ***************************
// sbIDeviceBase implementation 
// Just forwarding calls to mBaseDevice

/* PRBool Initialize (); */
NS_IMETHODIMP sbCDDevice::Initialize(PRBool *_retval)
{
  // Post a message to handle this event asynchronously
  mCDManagerObject.Initialize();
  return NS_OK;
}

/* PRBool Finalize (); */
NS_IMETHODIMP sbCDDevice::Finalize(PRBool *_retval)
{
  // Post a message to handle this event asynchronously
  FinalizeAsync();
  return NS_OK;
}

/* PRBool AddCallback (in sbIDeviceBaseCallback pCallback); */
NS_IMETHODIMP sbCDDevice::AddCallback(sbIDeviceBaseCallback *pCallback, PRBool *_retval)
{
  return sbDeviceBase::AddCallback(pCallback, _retval);
}

/* PRBool RemoveCallback (in sbIDeviceBaseCallback pCallback); */
NS_IMETHODIMP sbCDDevice::RemoveCallback(sbIDeviceBaseCallback *pCallback, PRBool *_retval)
{
  return sbDeviceBase::RemoveCallback(pCallback, _retval);
}

/* wstring EnumDeviceString (in PRUint32 index); */
NS_IMETHODIMP sbCDDevice::EnumDeviceString(PRUint32 index, PRUnichar **_retval)
{
  *_retval = mCDManagerObject.EnumDeviceString(index);

  return NS_OK;
}

/* attribute wstring name; */
NS_IMETHODIMP sbCDDevice::SetName(const PRUnichar * aName)
{
  return sbDeviceBase::SetName(aName);
}

/* wstring GetContext (in wstring deviceString); */
NS_IMETHODIMP sbCDDevice::GetContext(const PRUnichar *deviceString, PRUnichar **_retval)
{
  *_retval = mCDManagerObject.GetContext(deviceString);
  return NS_OK;
}

/* PRBool IsDownloadSupported (); */
NS_IMETHODIMP sbCDDevice::IsDownloadSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

/* PRUint32 GetSupportedFormats (); */
NS_IMETHODIMP sbCDDevice::GetSupportedFormats(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetSupportedFormats(deviceString, _retval);
}

/* PRBool IsUploadSupported (); */
NS_IMETHODIMP sbCDDevice::IsUploadSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsUploadSupported(deviceString, _retval);
}

/* PRBool IsTransfering (); */
NS_IMETHODIMP sbCDDevice::IsTransfering(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsTransfering(deviceString, _retval);
}

/* PRBool IsDeleteSupported (); */
NS_IMETHODIMP sbCDDevice::IsDeleteSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsDeleteSupported(deviceString, _retval);
}

/* PRUint32 GetUsedSpace (); */
NS_IMETHODIMP sbCDDevice::GetUsedSpace(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetUsedSpace(deviceString, _retval);
}

/* PRUint32 GetAvailableSpace (); */
NS_IMETHODIMP sbCDDevice::GetAvailableSpace(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetAvailableSpace(deviceString, _retval);
}

/* PRBool GetTrackTable (out wstring dbContext, out wstring tableName); */
NS_IMETHODIMP sbCDDevice::GetTrackTable(const PRUnichar *deviceString, PRUnichar **dbContext, PRUnichar **tableName, PRBool *_retval)
{
    *_retval = mCDManagerObject.GetTrackTable(deviceString, dbContext, tableName);
    return NS_OK;
}

/* PRBool AbortTransfer (); */
NS_IMETHODIMP sbCDDevice::AbortTransfer(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::AbortTransfer(deviceString, _retval);
}

/* PRBool DeleteTable (in wstring dbContext, in wstring tableName); */
NS_IMETHODIMP sbCDDevice::DeleteTable(const PRUnichar *deviceString, const PRUnichar *dbContext, const PRUnichar *tableName, PRBool *_retval)
{
  return sbDeviceBase::DeleteTable(deviceString, dbContext, tableName, _retval);
}

/* PRBool UpdateTable (in wstring dbContext, in wstring tableName); */
NS_IMETHODIMP sbCDDevice::UpdateTable(const PRUnichar *deviceString, const PRUnichar *tableName, PRBool *_retval)
{
  return sbDeviceBase::UpdateTable(deviceString, tableName, _retval);
}

/* PRBool EjectDevice (); */
NS_IMETHODIMP sbCDDevice::EjectDevice(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::EjectDevice(deviceString, _retval);
}

/* End DeviceBase */

NS_IMETHODIMP sbCDDevice::GetDeviceCategory(PRUnichar **_retval)
{
  size_t nLen = NAME_COMPACT_DISC_DEVICE_LEN + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(NAME_COMPACT_DISC_DEVICE, nLen * sizeof(PRUnichar));
  return NS_OK;
}

NS_IMETHODIMP sbCDDevice::GetName(PRUnichar * *aName)
{
  size_t nLen = NAME_COMPACT_DISC_DEVICE_LEN + 1;
  *aName = (PRUnichar *) nsMemory::Clone(NAME_COMPACT_DISC_DEVICE, nLen * sizeof(PRUnichar));
  return NS_OK;
}

PRBool sbCDDevice::IsEjectSupported() 
{
  return PR_FALSE;
}

void sbCDDevice::OnThreadBegin()
{
	CoInitializeEx(0, COINIT_MULTITHREADED);
}

void sbCDDevice::OnThreadEnd()
{
	CoUninitialize();
}

/* PRBool IsUpdateSupported (); */
NS_IMETHODIMP sbCDDevice::IsUpdateSupported(const PRUnichar *deviceString, PRBool *_retval)
{
	return sbDeviceBase::IsUpdateSupported(deviceString, _retval);
}

/* PRBool IsEjectSupported (); */
NS_IMETHODIMP sbCDDevice::IsEjectSupported(const PRUnichar *deviceString, PRBool *_retval)
{
	return sbDeviceBase::IsEjectSupported(deviceString, _retval);
}

/* PRUint32 GetNumDevices (); */
NS_IMETHODIMP sbCDDevice::GetNumDevices(PRUint32 *_retval)
{
	return sbDeviceBase::GetNumDevices(_retval);
}

/* wstring GetNumDestinations (in wstring DeviceString); */
NS_IMETHODIMP sbCDDevice::GetNumDestinations(const PRUnichar *DeviceString, PRUnichar **_retval)
{
	return sbDeviceBase::GetNumDestinations(DeviceString, _retval);
}

/* PRBool MakeTransferTable (in wstring DeviceString, in wstring ContextInput, in wstring TableName, out wstring TransferTable); */
NS_IMETHODIMP sbCDDevice::MakeTransferTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, PRUnichar **TransferTable, PRBool *_retval)
{
	return sbDeviceBase::MakeTransferTable(DeviceString, ContextInput, TableName, TransferTable, _retval);
}

/* PRBool SuspendTransfer (); */
NS_IMETHODIMP sbCDDevice::SuspendTransfer(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

/* PRBool ResumeTransfer (); */
NS_IMETHODIMP sbCDDevice::ResumeTransfer(PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

/* PRUint32 GetDeviceState (); */
NS_IMETHODIMP sbCDDevice::GetDeviceState(const PRUnichar* deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetDeviceState(deviceString, _retval);
}

/* PRBool RemoveTranferTracks (in PRUint32 index); */
NS_IMETHODIMP sbCDDevice::RemoveTranferTracks(const PRUnichar* deviceString, PRUint32 index, PRBool *_retval)
{
  return sbDeviceBase::RemoveTranferTracks(deviceString, index, _retval);
}

/* wstring GetDownloadTable (in wstring deviceString); */
NS_IMETHODIMP sbCDDevice::GetDownloadTable(const PRUnichar *deviceString, PRUnichar **_retval)
{
  return sbDeviceBase::GetDownloadTable(deviceString, _retval);
}

/* wstring GetUploadTable (in wstring deviceString); */
NS_IMETHODIMP sbCDDevice::GetUploadTable(const PRUnichar *deviceString, PRUnichar **_retval)
{
  return sbDeviceBase::GetUploadTable(deviceString, _retval);
}

/* PRBool AutoDownloadTable (in wstring deviceString, in wstring tableName); */
NS_IMETHODIMP sbCDDevice::AutoDownloadTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **filterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval)
{
  if (!IsDownloadInProgress())
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(DeviceString);
  }

  return sbDeviceBase::AutoDownloadTable(DeviceString, ContextInput, TableName, FilterColumn, FilterCount, filterValues, sourcePath, destPath, TransferTable, _retval);
}

/* PRBool AutoUploadTable (in wstring deviceString, in wstring tableName); */
NS_IMETHODIMP sbCDDevice::AutoUploadTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **filterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRBool DownloadTable (in wstring DeviceCategory, in wstring DeviceString, in wstring TableName); */
NS_IMETHODIMP sbCDDevice::UploadTable(const PRUnichar *DeviceString, const PRUnichar *TableName, PRBool *_retval)
{
  return sbDeviceBase::UploadTable(DeviceString, TableName, _retval);
}

/* PRBool DownloadTable (in wstring DeviceCategory, in wstring DeviceString, in wstring TableName); */
NS_IMETHODIMP sbCDDevice::DownloadTable(const PRUnichar *DeviceString, const PRUnichar *TableName, PRBool *_retval)
{
  return sbDeviceBase::DownloadTable(DeviceString, TableName, _retval);
}

/* PRBool OnCDDriveEvent (in PRBool mediaInserted); */
NS_IMETHODIMP sbCDDevice::OnCDDriveEvent(PRBool mediaInserted, PRBool *_retval)
{
  // Post a message to handle this event asynchronously
  DeviceEventAsync(mediaInserted);
  return NS_OK;
}

PRBool sbCDDevice::InitializeSync()
{
  mCDManagerObject.Initialize();
  return PR_TRUE;
}

PRBool sbCDDevice::FinalizeSync()
{
  mCDManagerObject.Finalize();
  return PR_TRUE;
}

PRBool sbCDDevice::DeviceEventSync(PRBool mediaInserted)
{
  return mCDManagerObject.OnCDDriveEvent(mediaInserted);
}


// Transfer related
nsString sbCDDevice::GetDeviceDownloadTableDescription(const PRUnichar* deviceString)
{ 
  return nsString(L"cd-rip"); 
}

nsString sbCDDevice::GetDeviceUploadTableDescription(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

nsString sbCDDevice::GetDeviceDownloadTableType(const PRUnichar* deviceString)
{ 
  return nsString(L"cd-rip"); 
}

nsString sbCDDevice::GetDeviceUploadTableType(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

nsString sbCDDevice::GetDeviceDownloadReadable(const PRUnichar* deviceString)
{ 
  return nsString(L"CD Ripping"); 
}

nsString sbCDDevice::GetDeviceUploadTableReadable(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

nsString sbCDDevice::GetDeviceDownloadTable(const PRUnichar* deviceString)
{ 
  return nsString(L"cdrip"); 
}

nsString sbCDDevice::GetDeviceUploadTable(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

PRBool sbCDDevice::TransferFile(PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber)
{
  return mCDManagerObject.TransferFile(deviceString, source, destination, dbContext, table, index, curDownloadRowNumber);
}

PRBool sbCDDevice::StopCurrentTransfer()
{
  return PR_FALSE;
}

PRBool sbCDDevice::SuspendCurrentTransfer()
{
  return PR_FALSE;
}

PRBool sbCDDevice::ResumeTransfer()
{
  return PR_FALSE;
}


/* End of implementation class template. */
