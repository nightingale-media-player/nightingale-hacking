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
* \file  sbDownloadDevice.cpp
* \brief Songbird DeviceBase Component Implementation.
*/
#include "nspr.h"

#if defined(XP_WIN)
#include "objbase.h"
#endif


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

#include "DownloadDevice.h"

#include "sbIDatabaseResult.h"
#include "sbIDatabaseQuery.h"

#include "sbIMediaLibrary.h"
#include "sbIPlaylist.h"

/* Implementation file */

#define NAME_DOWNLOAD_DEVICE_LEN          NS_LITERAL_STRING("Songbird Download Device").Length()
#define NAME_DOWNLOAD_DEVICE              NS_LITERAL_STRING("Songbird Download Device").get()

#define CONTEXT_DOWNLOAD_DEVICE_LEN       NS_LITERAL_STRING("downloadDB").Length()
#define CONTEXT_DOWNLOAD_DEVICE           NS_LITERAL_STRING("downloadDB").get()

#define DOWNLOAD_DEVICE_TABLE_NAME        NS_LITERAL_STRING("download").get()
#define DOWNLOAD_DEVICE_TABLE_READABLE    NS_LITERAL_STRING("&device.download").get()
#define DOWNLOAD_DEVICE_TABLE_DESCRIPTION NS_LITERAL_STRING("&device.download").get()
#define DOWNLOAD_DEVICE_TABLE_TYPE        NS_LITERAL_STRING("&device.download").get()

NS_IMPL_ISUPPORTS1(sbDownloadListener, nsIWebProgressListener)

NS_IMETHODIMP
sbDownloadListener::OnStateChange(nsIWebProgress *aWebProgress,
                                  nsIRequest *aRequest,
                                  PRUint32 aStateFlags,
                                  nsresult aStatus)
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

NS_IMETHODIMP
sbDownloadListener::OnProgressChange(nsIWebProgress *aWebProgress,
                                     nsIRequest *aRequest,
                                     PRInt32 aCurSelfProgress,
                                     PRInt32 aMaxSelfProgress,
                                     PRInt32 aCurTotalProgress,
                                     PRInt32 aMaxTotalProgress)
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

  PRUint32 newProgVal = (PRUint32) ((PRInt64)aCurTotalProgress * (PRInt64)100 / (PRInt64)aMaxSelfProgress);

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

NS_IMETHODIMP
sbDownloadListener::OnLocationChange(nsIWebProgress *aWebProgress,
                                     nsIRequest *aRequest,
                                     nsIURI *aLocation)
{
  if (mShutDown)
  {
    aRequest->Cancel(0);
    return NS_OK;
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDownloadListener::OnStatusChange(nsIWebProgress *aWebProgress,
                                   nsIRequest *aRequest,
                                   nsresult aStatus,
                                   const PRUnichar *aMessage)
{
  if (mShutDown)
  {
    aRequest->Cancel(0);
    return NS_OK;
  }

  mDownlodingDeviceObject->UpdateIOStatus((PRUnichar *)mDeviceString.get(), (PRUnichar *)mTable.get(), (PRUnichar *)mIndex.get(), aMessage);
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadListener::OnSecurityChange(nsIWebProgress *aWebProgress,
                                     nsIRequest *aRequest,
                                     PRUint32 aState)
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
sbDownloadDevice::sbDownloadDevice() :
  sbDeviceBase(PR_FALSE),
  mListener(nsnull),
  mDeviceState(kSB_DEVICE_STATE_IDLE)
{
  SetCurrentTransferRowNumber(-1);
} //ctor

//-----------------------------------------------------------------------------
sbDownloadDevice::~sbDownloadDevice() 
{
  PRBool retVal = PR_FALSE;
  Finalize(&retVal);
} //dtor

NS_IMETHODIMP
sbDownloadDevice::Initialize(PRBool *_retval)
{
  // Resume transfer if any pending
  sbDeviceBase::ResumeAbortedTransfer(NULL);

  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::Finalize(PRBool *_retval)
{
  StopCurrentTransfer(EmptyString());
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::AddCallback(sbIDeviceBaseCallback* aCallback,
                              PRBool* _retval)
{
  return sbDeviceBase::AddCallback(aCallback, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::RemoveCallback(sbIDeviceBaseCallback* aCallback,
                                 PRBool* _retval)
{
  return sbDeviceBase::RemoveCallback(aCallback, _retval);
}

// ***************************
// sbIDeviceBase implementation 
// Just forwarding calls to mBaseDevice

NS_IMETHODIMP
sbDownloadDevice::SetName(const nsAString& aName)
{
  return sbDeviceBase::SetName(aName);
}

NS_IMETHODIMP
sbDownloadDevice::GetDeviceStringByIndex(PRUint32 aIndex,
                                         nsAString& _retval)
{
  // There is only one download device, so index is ignored
  GetDeviceCategory(_retval);

  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::GetContext(const nsAString& aDeviceString,
                             nsAString& _retval)
{
  _retval.Assign(CONTEXT_DOWNLOAD_DEVICE);
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::IsDownloadSupported(const nsAString& aDeviceString,
                                      PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::GetSupportedFormats(const nsAString& aDeviceString,
                                      PRUint32 *_retval)
{
  return sbDeviceBase::GetSupportedFormats(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::IsUploadSupported(const nsAString& aDeviceString,
                                    PRBool *_retval)
{
  return sbDeviceBase::IsUploadSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::IsTransfering(const nsAString& aDeviceString,
                                PRBool *_retval)
{
  return sbDeviceBase::IsTransfering(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::IsDeleteSupported(const nsAString& aDeviceString,
                                    PRBool *_retval)
{
  return sbDeviceBase::IsDeleteSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::GetUsedSpace(const nsAString& aDeviceString,
                               PRUint32* _retval)
{
  return sbDeviceBase::GetUsedSpace(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::GetAvailableSpace(const nsAString& aDeviceString,
                                    PRUint32 *_retval)
{
  return sbDeviceBase::GetAvailableSpace(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::GetTrackTable(const nsAString& aDeviceString,
                                nsAString& aDBContext,
                                nsAString& aTableName,
                                PRBool *_retval)
{
  return sbDeviceBase::GetTrackTable(aDeviceString, aDBContext, aTableName,
                                     _retval);
}

NS_IMETHODIMP
sbDownloadDevice::AbortTransfer(const nsAString& aDeviceString,
                                PRBool *_retval)
{
  return sbDeviceBase::AbortTransfer(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::DeleteTable(const nsAString& aDeviceString,
                              const nsAString& aDBContext,
                              const nsAString& aTableName,
                              PRBool* _retval)
{
  return sbDeviceBase::DeleteTable(aDeviceString, aDBContext, aTableName,
                                   _retval);
}

NS_IMETHODIMP
sbDownloadDevice::UpdateTable(const nsAString& aDeviceString,
                              const nsAString& aTableName,
                              PRBool *_retval)
{
  return sbDeviceBase::UpdateTable(aDeviceString, aTableName, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::EjectDevice(const nsAString& aDeviceString,
                              PRBool *_retval)
{
  return sbDeviceBase::EjectDevice(aDeviceString, _retval);
}

/* End DeviceBase */

NS_IMETHODIMP
sbDownloadDevice::GetDeviceCategory(nsAString& aDeviceCategory)
{
  aDeviceCategory.Assign(NAME_DOWNLOAD_DEVICE);
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::GetName(nsAString& aName)
{
  aName.Assign(NAME_DOWNLOAD_DEVICE);
  return NS_OK;
}

PRBool
sbDownloadDevice::TransferFile(PRUnichar* deviceString,
                               PRUnichar* source,
                               PRUnichar* destination,
                               PRUnichar* dbContext,
                               PRUnichar* table,
                               PRUnichar* index,
                               PRInt32 curDownloadRowNumber)
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

    RemoveTranferTracks(EmptyString(), mediaId, &bRet);

    TransferData* data = new TransferData;
    data->dbContext = dbContext;
    data->dbTable = table;

    // Done with the download
    TransferNextFile(GetCurrentTransferRowNumber(deviceString), data);
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

  SetCurrentTransferRowNumber(curDownloadRowNumber);

  PRUint32 ret = pBrowser->SaveURI(linkURI, nsnull, nsnull, nsnull, nsnull, linkFile);

  return PR_TRUE;
}

void
sbDownloadDevice::OnThreadBegin()
{
}

void
sbDownloadDevice::OnThreadEnd()
{
}

NS_IMETHODIMP
sbDownloadDevice::IsUpdateSupported(const nsAString& aDeviceString,
                                    PRBool *_retval)
{
  return sbDeviceBase::IsUpdateSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::IsEjectSupported(const nsAString& aDeviceString,
                                   PRBool *_retval)
{
  return sbDeviceBase::IsEjectSupported(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::GetDeviceCount(PRUint32* aDeviceCount)
{
  return sbDeviceBase::GetDeviceCount(aDeviceCount);
}

NS_IMETHODIMP
sbDownloadDevice::GetDestinationCount(const nsAString& aDeviceString,
                                      PRUint32* _retval)
{
  return sbDeviceBase::GetDestinationCount(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::MakeTransferTable(const nsAString& aDeviceString,
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
sbDownloadDevice::AutoDownloadTable(const nsAString& aDeviceString,
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

NS_IMETHODIMP
sbDownloadDevice::AutoUploadTable(const nsAString& aDeviceString,
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

  if (!IsUploadInProgress(strDevice.get()))
  {
    // Get rid of previous download entries
    RemoveExistingTransferTableEntries(nsnull, PR_TRUE);
  }

  return sbDeviceBase::AutoUploadTable(aDeviceString, aContextInput,
                                       aTableName, aFilterColumn,
                                       aFilterCount, aFilterValues,
                                       aSourcePath, aDestPath,
                                       aTransferTable, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::SuspendTransfer(const nsAString& aDeviceString,
                                  PRBool *_retval)
{
  sbDeviceBase::SuspendTransfer(aDeviceString, _retval);

  return NS_OK;
}

PRBool
sbDownloadDevice::SuspendCurrentTransfer(const nsAString& aDeviceString)
{
  // XXXben Remove me

  if (IsTransferInProgress(aDeviceString))
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

NS_IMETHODIMP
sbDownloadDevice::ResumeTransfer(const nsAString& aDeviceString,
                                 PRBool *_retval)
{
  sbDeviceBase::ResumeTransfer(aDeviceString, _retval);

  return NS_OK;
}

PRBool
sbDownloadDevice::ResumeTransfer(const nsAString& aDeviceString)
{
  // Resume transfer
  if (mListener && mListener->Resume())
  {
    return PR_TRUE;
  }

  return PR_FALSE;
}


inline void
sbDownloadDevice::ReleaseListener()
{
  if (mListener)
  {
    mListener->Release();
    mListener = nsnull;
  }
}

NS_IMETHODIMP
sbDownloadDevice::GetDeviceState(const nsAString& aDeviceString,
                                 PRUint32 *_retval)
{
  *_retval = mDeviceState;
  return NS_OK;
}

NS_IMETHODIMP
sbDownloadDevice::RemoveTranferTracks(const nsAString& aDeviceString,
                                      PRUint32 aIndex,
                                      PRBool *_retval)
{
  return sbDeviceBase::RemoveTranferTracks(aDeviceString, aIndex, _retval);
}

PRBool
sbDownloadDevice::StopCurrentTransfer(const nsAString& aDeviceString)
{
  if (IsTransferInProgress(aDeviceString)) {
    if (mListener) {
      mListener->ShutDown();
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

NS_IMETHODIMP
sbDownloadDevice::GetDownloadTable(const nsAString& aDeviceString,
                                   nsAString& _retval)
{
  return sbDeviceBase::GetDownloadTable(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::GetUploadTable(const nsAString& aDeviceString,
                                 nsAString& _retval)
{
  return sbDeviceBase::GetUploadTable(aDeviceString, _retval);
}

// Transfer related
void
sbDownloadDevice::GetDeviceDownloadTableDescription(const nsAString& aDeviceString,
                                                    nsAString& _retval)
{ 
  _retval.Assign(DOWNLOAD_DEVICE_TABLE_DESCRIPTION); 
}

void
sbDownloadDevice::GetDeviceUploadTableDescription(const nsAString& aDeviceString,
                                                  nsAString& _retval)
{ 
  _retval.Assign(EmptyString()); 
}

void
sbDownloadDevice::GetDeviceDownloadTableType(const nsAString& aDeviceString,
                                             nsAString& _retval)
{ 
  _retval.Assign(DOWNLOAD_DEVICE_TABLE_TYPE); 
}

void
sbDownloadDevice::GetDeviceUploadTableType(const nsAString& aDeviceString,
                                           nsAString& _retval)
{ 
  _retval.Assign(EmptyString()); 
}

void
sbDownloadDevice::GetDeviceDownloadReadable(const nsAString& aDeviceString,
                                            nsAString& _retval)
{ 
  _retval.Assign(DOWNLOAD_DEVICE_TABLE_READABLE); 
}

void
sbDownloadDevice::GetDeviceUploadTableReadable(const nsAString& aDeviceString,
                                               nsAString& _retval)
{ 
  _retval.Assign(EmptyString()); 
}

void
sbDownloadDevice::GetDeviceDownloadTable(const nsAString& aDeviceString,
                                         nsAString& _retval)
{ 
  _retval.Assign(DOWNLOAD_DEVICE_TABLE_NAME); 
}

void
sbDownloadDevice::GetDeviceUploadTable(const nsAString& aDeviceString,
                                       nsAString& _retval)
{ 
  _retval.Assign(EmptyString()); 
}

NS_IMETHODIMP
sbDownloadDevice::UploadTable(const nsAString& aDeviceString,
                              const nsAString& aTableName,
                              PRBool *_retval)
{
  return sbDeviceBase::UploadTable(aDeviceString, aTableName, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::DownloadTable(const nsAString& aDeviceString,
                                const nsAString& aTableName,
                                PRBool *_retval)
{
  return sbDeviceBase::DownloadTable(aDeviceString, aTableName, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::SetDownloadFileType(const nsAString& aDeviceString,
                                      PRUint32 aFileType,
                                      PRBool *_retval)
{
  return sbDeviceBase::SetDownloadFileType(aDeviceString, aFileType, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::SetUploadFileType(const nsAString& aDeviceString,
                                    PRUint32 aFileType,
                                    PRBool *_retval)
{
  return sbDeviceBase::SetUploadFileType(aDeviceString, aFileType, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::GetDownloadFileType(const nsAString& aDeviceString,
                                      PRUint32 *_retval)
{
  return sbDeviceBase::GetDownloadFileType(aDeviceString, _retval);
}

NS_IMETHODIMP
sbDownloadDevice::GetUploadFileType(const nsAString& aDeviceString,
                                    PRUint32 *_retval)
{
  return sbDeviceBase::GetUploadFileType(aDeviceString, _retval);
}

/* End of implementation class template. */
