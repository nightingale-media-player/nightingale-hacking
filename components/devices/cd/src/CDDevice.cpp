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
#define COMPACT_DISC_DEVICE_TABLE_READABLE    NS_LITERAL_STRING("compactDisc.trackstable.readable").get()
#define COMPACT_DISC_DEVICE_TABLE_DESCRIPTION NS_LITERAL_STRING("compactDisc.trackstable.description").get()
#define COMPACT_DISC_DEVICE_TABLE_TYPE        NS_LITERAL_STRING("compactDisc.trackstable.type").get()

#define COMPACT_DISC_DEVICE_RIP_TABLE           NS_LITERAL_STRING("CDRip").get()
#define COMPACT_DISC_DEVICE_RIP_TABLE_READABLE  NS_LITERAL_STRING("compactDisc.cdrip").get()
#define COMPACT_DISC_DEVICE_BURN_TABLE          NS_LITERAL_STRING("CDBurn").get()
#define COMPACT_DISC_DEVICE_BURN_TABLE_READABLE NS_LITERAL_STRING("compactDisc.cdburn").get()

// CLASSES ====================================================================
NS_IMPL_ISUPPORTS2(sbCDDevice, sbIDeviceBase, sbICDDevice)

//-----------------------------------------------------------------------------
sbCDDevice::sbCDDevice()
: sbDeviceBase(PR_TRUE),
mCDManagerObject(this)
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
    }
  }

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
  *_retval = mCDManagerObject.IsUploadSupported(deviceString);
  return NS_OK;
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
  *_retval = mCDManagerObject.GetAvailableSpace(deviceString);
  return NS_OK;
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

/* PRBool MakeTransferTable (const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **FilterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRBool bDownloading, PRUnichar **TransferTableName, PRBool *_retval); */
NS_IMETHODIMP sbCDDevice::MakeTransferTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **FilterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRBool bDownloading, PRUnichar **TransferTableName, PRBool *_retval)
{
  return sbDeviceBase::MakeTransferTable(DeviceString, ContextInput, TableName, FilterColumn, FilterCount, FilterValues, sourcePath, destPath, bDownloading, TransferTableName, _retval);
}

/* PRBool SuspendTransfer (); */
NS_IMETHODIMP sbCDDevice::SuspendTransfer(const PRUnichar* deviceString, PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

/* PRBool ResumeTransfer (); */
NS_IMETHODIMP sbCDDevice::ResumeTransfer(const PRUnichar* deviceString, PRBool *_retval)
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
  if (!IsDownloadInProgress(DeviceString))
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(DeviceString, PR_TRUE);
  }

  return sbDeviceBase::AutoDownloadTable(DeviceString, ContextInput, TableName, FilterColumn, FilterCount, filterValues, sourcePath, destPath, TransferTable, _retval);
}

/* PRBool AutoUploadTable (in wstring deviceString, in wstring tableName); */
NS_IMETHODIMP sbCDDevice::AutoUploadTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **filterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval)
{
  if (IsDeviceIdle(DeviceString))
  {
    RemoveExistingTransferTableEntries(DeviceString, PR_FALSE);
    sbDeviceBase::AutoUploadTable(DeviceString, ContextInput, TableName, FilterColumn, FilterCount, filterValues, sourcePath, destPath, TransferTable, _retval);
  }

  return NS_OK;
}

/* PRBool DownloadTable (in wstring DeviceCategory, in wstring DeviceString, in wstring TableName); */
NS_IMETHODIMP sbCDDevice::UploadTable(const PRUnichar *DeviceString, const PRUnichar *TableName, PRBool *_retval)
{
  //return sbDeviceBase::UploadTable(DeviceString, TableName, _retval);
  *_retval = mCDManagerObject.UploadTable(DeviceString, TableName);
  return NS_OK;
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
  nsString displayString;
  PRUnichar *value = nsnull;

  m_StringBundle->GetStringFromName(COMPACT_DISC_DEVICE_RIP_TABLE_READABLE, &value);
  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceUploadTableDescription(const PRUnichar* deviceString)
{ 
  nsString displayString;
  PRUnichar *value = nsnull;

  m_StringBundle->GetStringFromName(COMPACT_DISC_DEVICE_BURN_TABLE_READABLE, &value);
  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceDownloadTableType(const PRUnichar* deviceString)
{ 
  nsString displayString;
  PRUnichar *value = nsnull;

  m_StringBundle->GetStringFromName(COMPACT_DISC_DEVICE_RIP_TABLE_READABLE, &value);
  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceUploadTableType(const PRUnichar* deviceString)
{ 
  nsString displayString;
  PRUnichar *value = nsnull;

  m_StringBundle->GetStringFromName(COMPACT_DISC_DEVICE_BURN_TABLE_READABLE, &value);
  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceDownloadReadable(const PRUnichar* deviceString)
{ 
  nsString displayString;
  PRUnichar *value = nsnull;

  m_StringBundle->GetStringFromName(COMPACT_DISC_DEVICE_RIP_TABLE_READABLE, &value);
  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceUploadTableReadable(const PRUnichar* deviceString)
{ 
  nsString displayString;
  PRUnichar *value = nsnull;

  m_StringBundle->GetStringFromName(COMPACT_DISC_DEVICE_BURN_TABLE_READABLE, &value);
  displayString = value;
  PR_Free(value);

  return displayString;
}

nsString sbCDDevice::GetDeviceDownloadTable(const PRUnichar* deviceString)
{ 
  return nsString(COMPACT_DISC_DEVICE_RIP_TABLE);
}

nsString sbCDDevice::GetDeviceUploadTable(const PRUnichar* deviceString)
{ 
  return nsString(COMPACT_DISC_DEVICE_BURN_TABLE); 
}

PRBool sbCDDevice::TransferFile(PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber)
{
  return mCDManagerObject.TransferFile(deviceString, source, destination, dbContext, table, index, curDownloadRowNumber);
}

PRBool sbCDDevice::StopCurrentTransfer(const PRUnichar* deviceString)
{
  return mCDManagerObject.StopCurrentTransfer(deviceString);
}

PRBool sbCDDevice::SuspendCurrentTransfer(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

PRBool sbCDDevice::ResumeTransfer(const PRUnichar* deviceString)
{
  return PR_FALSE;
}

/* PRBool SetDownloadFileType (in PRUint32 fileType); */
NS_IMETHODIMP sbCDDevice::SetDownloadFileType(const PRUnichar *deviceString, PRUint32 fileType, PRBool *_retval)
{
  *_retval = mCDManagerObject.SetCDRipFormat(deviceString, fileType);

  return NS_OK;
}

/* PRBool SetUploadFileType (in PRUint32 fileType); */
NS_IMETHODIMP sbCDDevice::SetUploadFileType(const PRUnichar *deviceString, PRUint32 fileType, PRBool *_retval)
{
  return sbDeviceBase::SetUploadFileType(deviceString, fileType, _retval);
}

/* PRUint32 GetDownloadFileType (in wstring deviceString); */
NS_IMETHODIMP sbCDDevice::GetDownloadFileType(const PRUnichar *deviceString, PRUint32 *_retval)
{
  *_retval = mCDManagerObject.GetCDRipFormat(deviceString);

  return NS_OK;
}

/* PRUint32 GetUploadFileType (in wstring deviceString); */
NS_IMETHODIMP sbCDDevice::GetUploadFileType(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetUploadFileType(deviceString, _retval);
}

PRUint32 sbCDDevice::GetCurrentTransferRowNumber(const PRUnichar* deviceString)
{
  return mCDManagerObject.GetCurrentTransferRowNumber(deviceString);
}

void sbCDDevice::TransferComplete(const PRUnichar* deviceString)
{
  return mCDManagerObject.TransferComplete(deviceString);
}

/* PRBool SetGap (in PRUint32 numSeconds); */
NS_IMETHODIMP sbCDDevice::SetGapBurnedTrack(const PRUnichar* deviceString, PRUint32 numSeconds, PRBool *_retval)
{
  *_retval = mCDManagerObject.SetGapBurnedTrack(deviceString, numSeconds);
  return NS_OK;
}


/* PRBool GetWritableCDDrive (out wstring deviceString); */
NS_IMETHODIMP sbCDDevice::GetWritableCDDrive(PRUnichar **deviceString, PRBool *_retval)
{
  *_retval = mCDManagerObject.GetWritableCDDrive(deviceString);
  return NS_OK;
}

/* End of implementation class template. */
