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
* \file  sbWMDevice.cpp
* \Windows Media device Component Implementation.
*/
#include "WMDevice.h"
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
#include <xpcom/nsMemory.h>
#include <string/nsStringAPI.h>
#include "NSString.h"
#include "nsIWebProgressListener.h"
#include "nsNetUtil.h"

/* Implementation file */

#define NAME_WINDOWS_MEDIA_DEVICE_LEN NS_LITERAL_STRING("Songbird Windows Media Device").Length()
#define NAME_WINDOWS_MEDIA_DEVICE NS_LITERAL_STRING("Songbird Windows Media Device").get()

#define CONTEXT_WINDOWS_MEDIA_DEVICE_LEN  NS_LITERAL_STRING("windowsm]ediaDB-1108").Length()
#define CONTEXT_WINDOWS_MEDIA_DEVICE      NS_LITERAL_STRING("windowsmediaDB-1108").get()

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS2(sbWMDevice, sbIDeviceBase, sbIWMDevice)

//-----------------------------------------------------------------------------
sbWMDevice::sbWMDevice():
sbDeviceBase(true)
{
} //ctor

//-----------------------------------------------------------------------------
/*virtual*/ sbWMDevice::~sbWMDevice() 
{
} //dtor

// ***************************
// sbIDeviceBase implementation 
// Just forwarding calls to mBaseDevice

/* PRBool Initialize (); */
NS_IMETHODIMP sbWMDevice::Initialize(PRBool *_retval)
{
  return NS_OK;
}

/* PRBool Finalize (); */
NS_IMETHODIMP sbWMDevice::Finalize(PRBool *_retval)
{
  return NS_OK;
}

/* PRBool AddCallback (in sbIDeviceBaseCallback pCallback); */
NS_IMETHODIMP sbWMDevice::AddCallback(sbIDeviceBaseCallback *pCallback, PRBool *_retval)
{
  return sbDeviceBase::AddCallback(pCallback, _retval);
}

/* PRBool RemoveCallback (in sbIDeviceBaseCallback pCallback); */
NS_IMETHODIMP sbWMDevice::RemoveCallback(sbIDeviceBaseCallback *pCallback, PRBool *_retval)
{
  return sbDeviceBase::RemoveCallback(pCallback, _retval);
}

/* wstring EnumDeviceString (in PRUint32 index); */
NS_IMETHODIMP sbWMDevice::EnumDeviceString(PRUint32 index, PRUnichar **_retval)
{
  return sbDeviceBase::EnumDeviceString(index, _retval);
}

/* attribute wstring name; */
NS_IMETHODIMP sbWMDevice::SetName(const PRUnichar * aName)
{
  return sbDeviceBase::SetName(aName);
}

/* wstring GetContext (in wstring deviceString); */
NS_IMETHODIMP sbWMDevice::GetContext(const PRUnichar *deviceString, PRUnichar **_retval)
{
  size_t nLen = CONTEXT_WINDOWS_MEDIA_DEVICE_LEN + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(CONTEXT_WINDOWS_MEDIA_DEVICE, nLen * sizeof(PRUnichar));

  if(*_retval == nsnull) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

/* PRBool IsDownloadSupported (); */
NS_IMETHODIMP sbWMDevice::IsDownloadSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  *_retval = true;
  return NS_OK;
}

/* PRUint32 GetSupportedFormats (); */
NS_IMETHODIMP sbWMDevice::GetSupportedFormats(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetSupportedFormats(deviceString, _retval);
}

/* PRBool IsUploadSupported (); */
NS_IMETHODIMP sbWMDevice::IsUploadSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsUploadSupported(deviceString, _retval);
}

/* PRBool IsTransfering (); */
NS_IMETHODIMP sbWMDevice::IsTransfering(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsTransfering(deviceString, _retval);
}

/* PRBool IsDeleteSupported (); */
NS_IMETHODIMP sbWMDevice::IsDeleteSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsDeleteSupported(deviceString, _retval);
}

/* PRUint32 GetUsedSpace (); */
NS_IMETHODIMP sbWMDevice::GetUsedSpace(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetUsedSpace(deviceString, _retval);
}

/* PRUint32 GetAvailableSpace (); */
NS_IMETHODIMP sbWMDevice::GetAvailableSpace(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetAvailableSpace(deviceString, _retval);
}

/* PRBool GetTrackTable (out wstring dbContext, out wstring tableName); */
NS_IMETHODIMP sbWMDevice::GetTrackTable(const PRUnichar *deviceString, PRUnichar **dbContext, PRUnichar **tableName, PRBool *_retval)
{
  return sbDeviceBase::GetTrackTable(deviceString, dbContext, tableName, _retval);
}

/* PRBool AutoDownloadTable (in wstring deviceString, in wstring tableName); */
NS_IMETHODIMP sbWMDevice::AutoDownloadTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **filterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRBool AutoUploadTable (in wstring deviceString, in wstring tableName); */
NS_IMETHODIMP sbWMDevice::AutoUploadTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **filterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRBool AbortTransfer (); */
NS_IMETHODIMP sbWMDevice::AbortTransfer(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::AbortTransfer(deviceString, _retval);
}

/* PRBool DeleteTable (in wstring dbContext, in wstring tableName); */
NS_IMETHODIMP sbWMDevice::DeleteTable(const PRUnichar *deviceString, const PRUnichar *dbContext, const PRUnichar *tableName, PRBool *_retval)
{
  return sbDeviceBase::DeleteTable(deviceString, dbContext, tableName, _retval);
}

/* PRBool UpdateTable (in wstring dbContext, in wstring tableName); */
NS_IMETHODIMP sbWMDevice::UpdateTable(const PRUnichar *deviceString, const PRUnichar *tableName, PRBool *_retval)
{
  return sbDeviceBase::UpdateTable(deviceString, tableName, _retval);
}

/* PRBool EjectDevice (); */
NS_IMETHODIMP sbWMDevice::EjectDevice(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::EjectDevice(deviceString, _retval);
}

/* End DeviceBase */

NS_IMETHODIMP sbWMDevice::GetDeviceCategory(PRUnichar **_retval)
{   
  size_t nLen = NAME_WINDOWS_MEDIA_DEVICE_LEN + 1; 
  *_retval = (PRUnichar *)nsMemory::Clone(NAME_WINDOWS_MEDIA_DEVICE, nLen * sizeof(PRUnichar));
  if(*_retval == nsnull) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

NS_IMETHODIMP sbWMDevice::GetName(PRUnichar **aName)
{
  size_t nLen = NAME_WINDOWS_MEDIA_DEVICE_LEN + 1; 
  *aName = (PRUnichar *)nsMemory::Clone(NAME_WINDOWS_MEDIA_DEVICE, nLen * sizeof(PRUnichar));
  if(*aName == nsnull) return NS_ERROR_OUT_OF_MEMORY;

  return NS_OK;
}

bool sbWMDevice::IsEjectSupported() 
{
  return false;
}

void sbWMDevice::OnThreadBegin()
{
  CoInitializeEx(0, COINIT_MULTITHREADED);
}

void sbWMDevice::OnThreadEnd()
{
  CoUninitialize();
}

/* PRBool IsUpdateSupported (); */
NS_IMETHODIMP sbWMDevice::IsUpdateSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsUpdateSupported(deviceString, _retval);
}

/* PRBool IsEjectSupported (); */
NS_IMETHODIMP sbWMDevice::IsEjectSupported(const PRUnichar *deviceString, PRBool *_retval)
{
  return sbDeviceBase::IsEjectSupported(deviceString, _retval);
}

/* PRUint32 GetNumDevices (); */
NS_IMETHODIMP sbWMDevice::GetNumDevices(PRUint32 *_retval)
{
  return sbDeviceBase::GetNumDevices(_retval);
}

/* wstring GetNumDestinations (in wstring DeviceString); */
NS_IMETHODIMP sbWMDevice::GetNumDestinations(const PRUnichar *DeviceString, PRUnichar **_retval)
{
  return sbDeviceBase::GetNumDestinations(DeviceString, _retval);
}

/* PRBool MakeTransferTable (const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **FilterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRBool bDownloading, PRUnichar **TransferTableName, PRBool *_retval); */
NS_IMETHODIMP sbWMDevice::MakeTransferTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **FilterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRBool bDownloading, PRUnichar **TransferTableName, PRBool *_retval)
{
  return sbDeviceBase::MakeTransferTable(DeviceString, ContextInput, TableName, FilterColumn, FilterCount, FilterValues, sourcePath, destPath, bDownloading, TransferTableName, _retval);
}

/* PRBool SuspendTransfer (); */
NS_IMETHODIMP sbWMDevice::SuspendTransfer(const PRUnichar *DeviceString, PRBool *_retval)
{
  *_retval = false;
  return NS_OK;
}

/* PRBool ResumeTransfer (); */
NS_IMETHODIMP sbWMDevice::ResumeTransfer(const PRUnichar *DeviceString, PRBool *_retval)
{
  *_retval = false;
  return NS_OK;
}

/* PRUint32 GetDeviceState (); */
NS_IMETHODIMP sbWMDevice::GetDeviceState(const PRUnichar* deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetDeviceState(deviceString, _retval);
}

/* PRBool RemoveTranferTracks (in PRUint32 index); */
NS_IMETHODIMP sbWMDevice::RemoveTranferTracks(const PRUnichar *deviceString,PRUint32 index, PRBool *_retval)
{
  return sbDeviceBase::RemoveTranferTracks(deviceString, index, _retval);
}

/* wstring GetDownloadTable (in wstring deviceString); */
NS_IMETHODIMP sbWMDevice::GetDownloadTable(const PRUnichar *deviceString, PRUnichar **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* wstring GetUploadTable (in wstring deviceString); */
NS_IMETHODIMP sbWMDevice::GetUploadTable(const PRUnichar *deviceString, PRUnichar **_retval)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* PRBool DownloadTable (in wstring DeviceCategory, in wstring DeviceString, in wstring TableName); */
NS_IMETHODIMP sbWMDevice::UploadTable(const PRUnichar *DeviceString, const PRUnichar *TableName, PRBool *_retval)
{
  return sbDeviceBase::UploadTable(DeviceString, TableName, _retval);
}

/* PRBool DownloadTable (in wstring DeviceCategory, in wstring DeviceString, in wstring TableName); */
NS_IMETHODIMP sbWMDevice::DownloadTable(const PRUnichar *DeviceString, const PRUnichar *TableName, PRBool *_retval)
{
  return sbDeviceBase::DownloadTable(DeviceString, TableName, _retval);
}

/* PRBool SetDownloadFileType (in PRUint32 fileType); */
NS_IMETHODIMP sbWMDevice::SetDownloadFileType(const PRUnichar *deviceString, PRUint32 fileType, PRBool *_retval)
{
  return sbDeviceBase::SetDownloadFileType(deviceString, fileType, _retval);
}

/* PRBool SetUploadFileType (in PRUint32 fileType); */
NS_IMETHODIMP sbWMDevice::SetUploadFileType(const PRUnichar *deviceString, PRUint32 fileType, PRBool *_retval)
{
  return sbDeviceBase::SetUploadFileType(deviceString, fileType, _retval);
}

/* PRUint32 GetDownloadFileType (in wstring deviceString); */
NS_IMETHODIMP sbWMDevice::GetDownloadFileType(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetDownloadFileType(deviceString, _retval);
}

/* PRUint32 GetUploadFileType (in wstring deviceString); */
NS_IMETHODIMP sbWMDevice::GetUploadFileType(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetUploadFileType(deviceString, _retval);
}

/* End of implementation class template. */
