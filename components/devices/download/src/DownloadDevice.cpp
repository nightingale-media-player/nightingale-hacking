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
* \file  sbDownloadDevice.cpp
* \brief Songbird DeviceBase Component Implementation.
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
#include <webbrowserpersist/nsIWebBrowserPersist.h>
#include <xpcom/nsILocalFile.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <string/nsStringAPI.h>
#include "nsString.h"
#include "nsIWebProgressListener.h"
#include "nsNetUtil.h"

#include "DownloadDevice.h"

#include "IDatabaseResult.h"
#include "IDatabaseQuery.h"

#include "IMediaLibrary.h"
#include "IPlaylist.h"

/* Implementation file */

#define NAME_DOWNLOAD_DEVICE_LEN          NS_LITERAL_STRING("Songbird Download Device").Length()
#define NAME_DOWNLOAD_DEVICE              NS_LITERAL_STRING("Songbird Download Device").get()

#define CONTEXT_DOWNLOAD_DEVICE_LEN       NS_LITERAL_STRING("downloadDB-1108").Length()
#define CONTEXT_DOWNLOAD_DEVICE           NS_LITERAL_STRING("downloadDB-1108").get()

#define DOWNLOAD_DEVICE_TABLE_NAME        NS_LITERAL_STRING("&device.download").get()
#define DOWNLOAD_DEVICE_TABLE_READABLE    NS_LITERAL_STRING("&device.download").get()
#define DOWNLOAD_DEVICE_TABLE_DESCRIPTION NS_LITERAL_STRING("download").get()
#define DOWNLOAD_DEVICE_TABLE_TYPE        NS_LITERAL_STRING("download").get()

class sbDownloadListener : public nsIWebProgressListener
{
public:
    sbDownloadListener(sbDownloadDevice *downlodingDeviceObject, PRUnichar* deviceString, PRUnichar* table, PRUnichar* index): 
      mDownlodingDeviceObject(downlodingDeviceObject),
      mDeviceString(deviceString), 
      mTable(table),
      mIndex(index),
      mShutDown(PR_FALSE),
      mPrevProgVal(0),
      mSuspend(PR_FALSE),
      mSavedRequestObject(nsnull)
    {
      mLastDownloadUpdate = time(nsnull);
    }
    
    ~sbDownloadListener()
	  {
	  }

    void ShutDown()
    {
      mShutDown = PR_TRUE;
    }

    void Suspend()
    {
      mSuspend = PR_TRUE;
    }

    PRBool Resume()
    {
      if (mSuspend && mSavedRequestObject)
      {
        mSuspend = PR_FALSE;
        mSavedRequestObject->Resume();

        return PR_TRUE;
      }
      return PR_FALSE;
    }

    NS_DECL_ISUPPORTS
    NS_DECL_NSIWEBPROGRESSLISTENER

private:
    sbDownloadDevice* mDownlodingDeviceObject;
    nsString mDeviceString; 
    nsString mTable;
    nsString mIndex;
    PRBool mShutDown;
    PRBool mSuspend;
    PRUint32 mPrevProgVal;
    nsIRequest* mSavedRequestObject;
    time_t mLastDownloadUpdate;
};

NS_IMPL_ISUPPORTS1(sbDownloadListener, nsIWebProgressListener)

NS_IMETHODIMP sbDownloadListener::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
  if (mShutDown)
  {
    aRequest->Cancel(0);
    return NS_OK;
  }

	// If download done for this file then, initiate the next download
  if (aStateFlags & STATE_STOP)
  {
    // Copy the data so you're not sending stale pointers
    // ("DownloadDone" deletes this item and frees the strings)
    nsString deviceString = mDeviceString;
    nsString table = mTable;
    nsString index = mIndex;

    // OK Download complete
    if (mDownlodingDeviceObject)
    {
      mDownlodingDeviceObject->UpdateIOProgress((PRUnichar *)deviceString.get(), (PRUnichar *)table.get(), (PRUnichar *)index.get(), 100 /* Complete */);
      mDownlodingDeviceObject->DownloadDone((PRUnichar *)deviceString.get(), (PRUnichar *)table.get(), (PRUnichar *)index.get());
    }

  }

  return NS_OK;
}

NS_IMETHODIMP sbDownloadListener::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
  if (mShutDown)
  {
    aRequest->Cancel(0);
    return NS_OK;
  }

  if (mSuspend)
  {
    mSavedRequestObject = aRequest;
    aRequest->Suspend();
  }

  PRUint32 newProgVal = (PRInt64)aCurTotalProgress * (PRInt64)100 / (PRInt64)aMaxSelfProgress;
  
  // Update progress (0 ... 100)
  // Check if the update was not done within the last second.
  if (difftime(time(nsnull), mLastDownloadUpdate))
  {
    mLastDownloadUpdate = time(nsnull);
    mDownlodingDeviceObject->UpdateIOProgress((PRUnichar *)mDeviceString.get(), (PRUnichar *)mTable.get(), (PRUnichar *)mIndex.get(), newProgVal);
    mPrevProgVal = newProgVal;
  }
  return NS_OK;
}

NS_IMETHODIMP sbDownloadListener::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *aLocation)
{
  if (mShutDown)
  {
    aRequest->Cancel(0);
    return NS_OK;
  }

  return NS_OK;
}

NS_IMETHODIMP sbDownloadListener::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
  if (mShutDown)
  {
    aRequest->Cancel(0);
    return NS_OK;
  }

  mDownlodingDeviceObject->UpdateIOStatus((PRUnichar *)mDeviceString.get(), (PRUnichar *)mTable.get(), (PRUnichar *)mIndex.get(), aMessage);
  return NS_OK;
}

NS_IMETHODIMP sbDownloadListener::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aState)
{
  if (mShutDown)
  {
    aRequest->Cancel(0);
    return NS_OK;
  }

    return NS_OK;
}


// CLASSES ====================================================================
NS_IMPL_ISUPPORTS2(sbDownloadDevice, sbIDeviceBase, sbIDownloadDevice)

//-----------------------------------------------------------------------------
sbDownloadDevice::sbDownloadDevice():
	sbDeviceBase(PR_FALSE),
  mListener(nsnull)
{
  PRBool retVal = PR_FALSE;
} //ctor

//-----------------------------------------------------------------------------
sbDownloadDevice::~sbDownloadDevice() 
{
  PRBool retVal = PR_FALSE;
  Finalize(&retVal);
} //dtor

/* PRBool Initialize (); */
NS_IMETHODIMP sbDownloadDevice::Initialize(PRBool *_retval)
{
  // Resume transfer if any pending
  ResumeAbortedTransfer();
  return NS_OK;
}

/* PRBool Finalize (); */
NS_IMETHODIMP sbDownloadDevice::Finalize(PRBool *_retval)
{
  StopCurrentTransfer();
  return NS_OK;
}

/* PRBool AddCallback (in sbIDeviceBaseCallback pCallback); */
NS_IMETHODIMP sbDownloadDevice::AddCallback(sbIDeviceBaseCallback *pCallback, PRBool *_retval)
{
  return sbDeviceBase::AddCallback(pCallback, _retval);
}

/* PRBool RemoveCallback (in sbIDeviceBaseCallback pCallback); */
NS_IMETHODIMP sbDownloadDevice::RemoveCallback(sbIDeviceBaseCallback *pCallback, PRBool *_retval)
{
  return sbDeviceBase::RemoveCallback(pCallback, _retval);
}

// ***************************
// sbIDeviceBase implementation 
// Just forwarding calls to mBaseDevice
/* attribute wstring name; */
NS_IMETHODIMP sbDownloadDevice::SetName(const PRUnichar * aName)
{
    return sbDeviceBase::SetName(aName);
}

/* wstring EnumDeviceString (in PRUint32 index); */
NS_IMETHODIMP sbDownloadDevice::EnumDeviceString(PRUint32 index, PRUnichar **_retval)
{
	// There is only one download device, so index is ignored
	GetDeviceCategory(_retval);

    return NS_OK;
}

/* wstring GetContext (in wstring deviceString); */
NS_IMETHODIMP sbDownloadDevice::GetContext(const PRUnichar *deviceString, PRUnichar **_retval)
{
  size_t nLen = CONTEXT_DOWNLOAD_DEVICE_LEN + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(CONTEXT_DOWNLOAD_DEVICE, nLen * sizeof(PRUnichar));
	return NS_OK;
}

/* PRBool IsDownloadSupported (); */
NS_IMETHODIMP sbDownloadDevice::IsDownloadSupported(const PRUnichar *deviceString, PRBool *_retval)
{
    *_retval = PR_TRUE;
    return NS_OK;
}

/* PRUint32 GetSupportedFormats (); */
NS_IMETHODIMP sbDownloadDevice::GetSupportedFormats(const PRUnichar *deviceString, PRUint32 *_retval)
{
    return sbDeviceBase::GetSupportedFormats(deviceString, _retval);
}

/* PRBool IsUploadSupported (); */
NS_IMETHODIMP sbDownloadDevice::IsUploadSupported(const PRUnichar *deviceString, PRBool *_retval)
{
    return sbDeviceBase::IsUploadSupported(deviceString, _retval);
}

/* PRBool IsTransfering (); */
NS_IMETHODIMP sbDownloadDevice::IsTransfering(const PRUnichar *deviceString, PRBool *_retval)
{
    return sbDeviceBase::IsTransfering(deviceString, _retval);
}

/* PRBool IsDeleteSupported (); */
NS_IMETHODIMP sbDownloadDevice::IsDeleteSupported(const PRUnichar *deviceString, PRBool *_retval)
{
    return sbDeviceBase::IsDeleteSupported(deviceString, _retval);
}

/* PRUint32 GetUsedSpace (); */
NS_IMETHODIMP sbDownloadDevice::GetUsedSpace(const PRUnichar *deviceString, PRUint32 *_retval)
{
    return sbDeviceBase::GetUsedSpace(deviceString, _retval);
}

/* PRUint32 GetAvailableSpace (); */
NS_IMETHODIMP sbDownloadDevice::GetAvailableSpace(const PRUnichar *deviceString, PRUint32 *_retval)
{
    return sbDeviceBase::GetAvailableSpace(deviceString, _retval);
}

/* PRBool GetTrackTable (out wstring dbContext, out wstring tableName); */
NS_IMETHODIMP sbDownloadDevice::GetTrackTable(const PRUnichar *deviceString, PRUnichar **dbContext, PRUnichar **tableName, PRBool *_retval)
{
    return sbDeviceBase::GetTrackTable(deviceString, dbContext, tableName, _retval);
}

/* PRBool AbortTransfer (); */
NS_IMETHODIMP sbDownloadDevice::AbortTransfer(const PRUnichar *deviceString, PRBool *_retval)
{
    return sbDeviceBase::AbortTransfer(deviceString, _retval);
}

/* PRBool DeleteTable (in wstring dbContext, in wstring tableName); */
NS_IMETHODIMP sbDownloadDevice::DeleteTable(const PRUnichar *deviceString, const PRUnichar *dbContext, const PRUnichar *tableName, PRBool *_retval)
{
    return sbDeviceBase::DeleteTable(deviceString, dbContext, tableName, _retval);
}

/* PRBool UpdateTable (in wstring dbContext, in wstring tableName); */
NS_IMETHODIMP sbDownloadDevice::UpdateTable(const PRUnichar *deviceString, const PRUnichar *tableName, PRBool *_retval)
{
    return sbDeviceBase::UpdateTable(deviceString, tableName, _retval);
}

/* PRBool EjectDevice (); */
NS_IMETHODIMP sbDownloadDevice::EjectDevice(const PRUnichar *deviceString, PRBool *_retval)
{
    return sbDeviceBase::EjectDevice(deviceString, _retval);
}

/* End DeviceBase */

/* wstring GetDeviceCategory (); */
NS_IMETHODIMP sbDownloadDevice::GetDeviceCategory(PRUnichar **_retval)
{
  size_t nLen = NAME_DOWNLOAD_DEVICE_LEN + 1;
  *_retval = (PRUnichar *) nsMemory::Clone(NAME_DOWNLOAD_DEVICE, nLen * sizeof(PRUnichar));
  return NS_OK;
}

NS_IMETHODIMP sbDownloadDevice::GetName(PRUnichar **aName)
{
  size_t nLen = NAME_DOWNLOAD_DEVICE_LEN + 1;
  *aName = (PRUnichar *) nsMemory::Clone(NAME_DOWNLOAD_DEVICE, nLen * sizeof(PRUnichar));
  return NS_OK;
}

PRBool sbDownloadDevice::TransferFile(PRUnichar* deviceString, PRUnichar* source, PRUnichar* destination, PRUnichar* dbContext, PRUnichar* table, PRUnichar* index, PRInt32 curDownloadRowNumber)
{
  nsCOMPtr<nsIURI> pSourceURI(do_CreateInstance("@mozilla.org/network/simple-uri;1"));

    if(!pSourceURI)
        return PR_FALSE;

    nsString strSourceURL(source);
    nsCString cstr;

    NS_UTF16ToCString(strSourceURL, NS_CSTRING_ENCODING_ASCII, cstr);
    if(NS_FAILED(pSourceURI->SetSpec(cstr))) return PR_FALSE;
    if(NS_FAILED(pSourceURI->GetScheme(cstr))) return PR_FALSE;
    
    if(cstr.Equals("file") ||
       cstr.Length() == 1)
    {
      PRBool bRet = PR_FALSE;
      
      PRInt32 ec = 0;
      PRInt32 mediaId = nsString(index).ToInteger(&ec);
      
      RemoveTranferTracks(NS_LITERAL_STRING("").get(), mediaId, &bRet);

      TransferData* data = new TransferData;
      data->dbContext = dbContext;
      data->dbTable = table;

      // Done with the download
      TransferNextFile(GetCurrentTransferRowNumber(), data);
      return PR_TRUE;
    }

    nsCOMPtr<nsIWebBrowserPersist> pBrowser(do_CreateInstance("@mozilla.org/embedding/browser/nsWebBrowser;1"));

    if (pBrowser == nsnull)
        return PR_FALSE;

    nsCOMPtr<nsIURI> linkURI;
    nsresult rv = NS_NewURI(getter_AddRefs(linkURI), nsAutoString(source));

    nsAutoString lUrl(destination);
    nsCOMPtr<nsILocalFile> linkFile;
    rv = NS_NewLocalFile(lUrl, PR_FALSE, getter_AddRefs(linkFile));

    ReleaseListener();

    mListener = new sbDownloadListener(this, deviceString, table, index);
    mListener->AddRef();
    pBrowser->SetProgressListener(mListener); 

    PRUint32 ret = pBrowser->SaveURI(linkURI, nsnull, nsnull, nsnull, nsnull, linkFile);

    return PR_TRUE;
}

void sbDownloadDevice::OnThreadBegin()
{
}

void sbDownloadDevice::OnThreadEnd()
{
}

/* PRBool IsUpdateSupported (); */
NS_IMETHODIMP sbDownloadDevice::IsUpdateSupported(const PRUnichar *deviceString, PRBool *_retval)
{
	return sbDeviceBase::IsUpdateSupported(deviceString, _retval);
}

/* PRBool IsEjectSupported (); */
NS_IMETHODIMP sbDownloadDevice::IsEjectSupported(const PRUnichar *deviceString, PRBool *_retval)
{
	return sbDeviceBase::IsEjectSupported(deviceString, _retval);
}

/* PRUint32 GetNumDevices (); */
NS_IMETHODIMP sbDownloadDevice::GetNumDevices(PRUint32 *_retval)
{
	return sbDeviceBase::GetNumDevices(_retval);
}

/* wstring GetNumDestinations (in wstring DeviceString); */
NS_IMETHODIMP sbDownloadDevice::GetNumDestinations(const PRUnichar *DeviceString, PRUnichar **_retval)
{
	return sbDeviceBase::GetNumDestinations(DeviceString, _retval);
}

/* PRBool MakeTransferTable (in wstring DeviceString, in wstring ContextInput, in wstring TableName, out wstring TransferTable); */
NS_IMETHODIMP sbDownloadDevice::MakeTransferTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, PRUnichar **TransferTable, PRBool *_retval)
{
	return sbDeviceBase::MakeTransferTable(DeviceString, ContextInput, TableName, TransferTable, _retval);
}

/* PRBool AutoDownloadTable (const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval); */
NS_IMETHODIMP sbDownloadDevice::AutoDownloadTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **FilterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval)
{
  if (!IsDownloadInProgress())
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(nsnull);
  }

  return sbDeviceBase::AutoDownloadTable(DeviceString, ContextInput, TableName, FilterColumn, FilterCount, FilterValues, sourcePath, destPath, TransferTable, _retval);
}

/* PRBool AutoUploadTable (const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval); */
NS_IMETHODIMP sbDownloadDevice::AutoUploadTable(const PRUnichar *DeviceString, const PRUnichar *ContextInput, const PRUnichar *TableName, const PRUnichar *FilterColumn, PRUint32 FilterCount, const PRUnichar **FilterValues, const PRUnichar *sourcePath, const PRUnichar *destPath, PRUnichar **TransferTable, PRBool *_retval)
{
  if (!IsUploadInProgress())
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(nsnull);
  }

  return sbDeviceBase::AutoUploadTable(DeviceString, ContextInput, TableName, FilterColumn, FilterCount, FilterValues, sourcePath, destPath, TransferTable, _retval);
}

/* PRBool SuspendTransfer (); */
NS_IMETHODIMP sbDownloadDevice::SuspendTransfer(PRBool *_retval)
{
  sbDeviceBase::SuspendTransfer(_retval);

  return NS_OK;
}

PRBool sbDownloadDevice::SuspendCurrentTransfer()
{
  if (IsTransferInProgress())
  {
    // Suspend transfer
    if (mListener)
    {
      mListener->Suspend();
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

/* PRBool ResumeTransfer (); */
NS_IMETHODIMP sbDownloadDevice::ResumeTransfer(PRBool *_retval)
{
  sbDeviceBase::ResumeTransfer(_retval);

  return NS_OK;
}

PRBool sbDownloadDevice::ResumeTransfer()
{
  // Resume transfer
  if (mListener && mListener->Resume())
  {
    return PR_TRUE;
  }

  return PR_FALSE;
}


inline void sbDownloadDevice::ReleaseListener()
{
  if (mListener)
  {
    mListener->Release();
    mListener = nsnull;
  }
}

/* PRUint32 GetDeviceState (); */
NS_IMETHODIMP sbDownloadDevice::GetDeviceState(const PRUnichar *deviceString, PRUint32 *_retval)
{
  return sbDeviceBase::GetDeviceState(deviceString, _retval);
}

/* PRBool RemoveTranferTracks (in PRUint32 index); */
NS_IMETHODIMP sbDownloadDevice::RemoveTranferTracks(const PRUnichar *deviceString, PRUint32 index, PRBool *_retval)
{
  return sbDeviceBase::RemoveTranferTracks(deviceString, index, _retval);
}

PRBool sbDownloadDevice::StopCurrentTransfer()
{
  if (IsTransferInProgress())
  {
    if (mListener)
    {
      mListener->ShutDown();
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

/* wstring GetDownloadTable (in wstring deviceString); */
NS_IMETHODIMP sbDownloadDevice::GetDownloadTable(const PRUnichar *deviceString, PRUnichar **_retval)
{
  return sbDeviceBase::GetDownloadTable(deviceString, _retval);
}

/* wstring GetUploadTable (in wstring deviceString); */
NS_IMETHODIMP sbDownloadDevice::GetUploadTable(const PRUnichar *deviceString, PRUnichar **_retval)
{
  return sbDeviceBase::GetUploadTable(deviceString, _retval);
}

// Transfer related
nsString sbDownloadDevice::GetDeviceDownloadTableDescription(const PRUnichar* deviceString)
{ 
  return nsString(DOWNLOAD_DEVICE_TABLE_DESCRIPTION); 
}

nsString sbDownloadDevice::GetDeviceUploadTableDescription(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

nsString sbDownloadDevice::GetDeviceDownloadTableType(const PRUnichar* deviceString)
{ 
  return nsString(DOWNLOAD_DEVICE_TABLE_TYPE); 
}

nsString sbDownloadDevice::GetDeviceUploadTableType(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

nsString sbDownloadDevice::GetDeviceDownloadReadable(const PRUnichar* deviceString)
{ 
  return nsString(DOWNLOAD_DEVICE_TABLE_READABLE); 
}

nsString sbDownloadDevice::GetDeviceUploadTableReadable(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

nsString sbDownloadDevice::GetDeviceDownloadTable(const PRUnichar* deviceString)
{ 
  return nsString(DOWNLOAD_DEVICE_TABLE_NAME); 
}

nsString sbDownloadDevice::GetDeviceUploadTable(const PRUnichar* deviceString)
{ 
  return nsString(); 
}

/* PRBool DownloadTable (in wstring DeviceCategory, in wstring DeviceString, in wstring TableName); */
NS_IMETHODIMP sbDownloadDevice::UploadTable(const PRUnichar *DeviceString, const PRUnichar *TableName, PRBool *_retval)
{
  return sbDeviceBase::UploadTable(DeviceString, TableName, _retval);
}

/* PRBool DownloadTable (in wstring DeviceCategory, in wstring DeviceString, in wstring TableName); */
NS_IMETHODIMP sbDownloadDevice::DownloadTable(const PRUnichar *DeviceString, const PRUnichar *TableName, PRBool *_retval)
{
  return sbDeviceBase::DownloadTable(DeviceString, TableName, _retval);
}

/* End of implementation class template. */
