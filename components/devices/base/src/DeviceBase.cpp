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
* \file  DeviceBase.cpp
* \brief Songbird DeviceBase Component Implementation.
*/

#include "DeviceBase.h"
#include "sbIDeviceBase.h"

#if defined(XP_WIN)
#include "objbase.h"
#endif

#include "nspr.h"

#include <xpcom/nsXPCOM.h>
#include <xpcom/nsComponentManagerUtils.h>
#include <xpcom/nsAutoLock.h>
#include <xpcom/nsMemory.h>

#include <xpcom/nsCRT.h>

#include "sbIDatabaseResult.h"
#include "sbIDatabaseQuery.h"

#include "sbIMediaLibrary.h"
#include "sbIPlaylist.h"

#define MSG_DEVICE_BASE             (0x2000) // Base message ID

#define MSG_DEVICE_TRANSFER         (MSG_DEVICE_BASE + 0)
#define MSG_DEVICE_UPLOAD           (MSG_DEVICE_BASE + 1)
#define MSG_DEVICE_DELETE           (MSG_DEVICE_BASE + 2)
#define MSG_DEVICE_UPDATE_METADATA  (MSG_DEVICE_BASE + 3)
#define MSG_DEVICE_EVENT            (MSG_DEVICE_BASE + 4)
#define MSG_DEVICE_INITIALIZE       (MSG_DEVICE_BASE + 5)
#define MSG_DEVICE_FINALIZE         (MSG_DEVICE_BASE + 6)
#define MSG_DEVICE_EJECT            (MSG_DEVICE_BASE + 7)

#define TRANSFER_TABLE_NAME         NS_LITERAL_STRING("Transfer")
#define URL_COLUMN_NAME             NS_LITERAL_STRING("url")
#define SOURCE_COLUMN_NAME          NS_LITERAL_STRING("source")
#define DESTINATION_COLUMN_NAME     NS_LITERAL_STRING("destination")
#define INDEX_COLUMN_NAME           NS_LITERAL_STRING("id")

NS_IMPL_THREADSAFE_ISUPPORTS1(sbDeviceThread, nsIRunnable)

//-----------------------------------------------------------------------------
sbDeviceBase::sbDeviceBase(PRBool usingThread)
: mDeviceQueueHasItem(PR_FALSE)
, mpCallbackListLock(PR_NewLock())
, mpDeviceThread(nsnull)
, mpDeviceThreadMonitor(nsAutoMonitor::NewMonitor("sbDeviceBase.mpDeviceThreadMonitor"))
, mDeviceThreadShouldShutdown(PR_FALSE)
{
  NS_ASSERTION(mpCallbackListLock, "sbDeviceBase.mpCallbackListLock failed");
  NS_ASSERTION(mpDeviceThreadMonitor, "sbDeviceBase.mpDeviceThreadMonitor failed");
  if (usingThread) {
    do {
      nsCOMPtr<nsIRunnable> pThread = new sbDeviceThread(this);
      NS_ASSERTION(pThread, "Unable to create sbDeviceThread");
      if (!pThread) 
        break;
      nsresult rv = NS_NewThread(getter_AddRefs(mpDeviceThread),
        pThread,
        0,
        PR_JOINABLE_THREAD);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start sbDeviceThread");
      if (NS_FAILED(rv))
        mpDeviceThread = nsnull;
    } while (PR_FALSE); // Only do this once
  }
} //ctor

//-----------------------------------------------------------------------------
sbDeviceBase::~sbDeviceBase()
{
  // Shutdown the thread
  if (mpDeviceThread) {
    {
      nsAutoMonitor mon(mpDeviceThreadMonitor);
      mDeviceThreadShouldShutdown = PR_TRUE;
      mon.NotifyAll();
    }
    mpDeviceThread->Join();
    mpDeviceThread = nsnull;
  }

  {
    nsAutoLock listLock(mpCallbackListLock);
    if(mCallbackList.size()) {
      for(size_t nCurrent = 0; nCurrent < mCallbackList.size(); ++nCurrent) {
        if(mCallbackList[nCurrent])
          mCallbackList[nCurrent]->Release();
      }
      mCallbackList.clear();
    }
  }

  if (mpCallbackListLock)
    PR_DestroyLock(mpCallbackListLock);
  if (mpDeviceThreadMonitor)
    nsAutoMonitor::DestroyMonitor(mpDeviceThreadMonitor);
} //dtor

NS_IMETHODIMP
sbDeviceBase::Initialize(PRBool *_retval)
{
  NS_NOTYETIMPLEMENTED("sbDeviceBase::Initialize");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbDeviceBase::Finalize(PRBool *_retval)
{
  NS_NOTYETIMPLEMENTED("sbDeviceBase::Finalize");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbDeviceBase::AddCallback(sbIDeviceBaseCallback* aCallback,
                          PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aCallback);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_TRUE;

  nsAutoLock lock(mpCallbackListLock);
  size_t nSize = mCallbackList.size();

  for (size_t nCurrent = 0; nCurrent < nSize; nCurrent++) {
    if (mCallbackList[nCurrent] == aCallback)
      return NS_OK;
  }

  NS_ADDREF(aCallback);
  mCallbackList.push_back(aCallback);

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::RemoveCallback(sbIDeviceBaseCallback* aCallback,
                             PRBool* _retval)
{
  NS_ENSURE_ARG_POINTER(aCallback);
  NS_ENSURE_ARG_POINTER(_retval);

  *_retval = PR_FALSE;

  nsAutoLock lock(mpCallbackListLock);
  std::vector<sbIDeviceBaseCallback *>::iterator itCallback = mCallbackList.begin();

  for(; itCallback != mCallbackList.end(); ++itCallback) {
    if((*itCallback) == aCallback) {
      mCallbackList.erase(itCallback);
      NS_RELEASE(*itCallback);

      *_retval = PR_TRUE;
      return NS_OK;
    }
  }

  return NS_OK;
}

/* static */
void PR_CALLBACK
sbDeviceBase::DeviceProcess(sbDeviceBase* pDevice)
{
  NS_ASSERTION(pDevice, "Null Pointer!");

  ThreadMessage* pDeviceMessage;

  pDevice->OnThreadBegin();

  while (PR_TRUE) {

    pDeviceMessage = nsnull;

    { // Enter Monitor

      nsAutoMonitor mon(pDevice->mpDeviceThreadMonitor);
      while (!pDevice->mDeviceQueueHasItem && !pDevice->mDeviceThreadShouldShutdown)
        mon.Wait();

      if (pDevice->mDeviceThreadShouldShutdown) {
        pDevice->OnThreadEnd();
        return;
      }

      pDeviceMessage = pDevice->mDeviceMessageQueue.front();
      pDevice->mDeviceMessageQueue.pop_front();
      if (pDevice->mDeviceMessageQueue.empty())
        pDevice->mDeviceQueueHasItem = PR_FALSE;

    } // Exit Monitor

    NS_ASSERTION(pDeviceMessage, "No message!");

    switch(pDeviceMessage->message) {
      case MSG_DEVICE_TRANSFER:
        pDevice->TransferNextFile(NS_PTR_TO_INT32(pDeviceMessage->data1), (TransferData *) pDeviceMessage->data2);
        break;
      case MSG_DEVICE_INITIALIZE:
        pDevice->InitializeSync();
        break;
      case MSG_DEVICE_FINALIZE:
        pDevice->FinalizeSync();
        break;
      case MSG_DEVICE_EVENT:
        pDevice->DeviceEventSync(NS_PTR_TO_INT32(pDeviceMessage->data2));
        break;
      default:
        NS_NOTREACHED("Bad DeviceMessage");
        break;
    }

    delete pDeviceMessage;
  } // while
}

PRBool
sbDeviceBase::SubmitMessage(PRUint32 aMessage,
                            void* aData1,
                            void* aData2)
{
  //NS_ENSURE_TRUE(mDeviceThread.GetThread(), PR_FALSE);

  ThreadMessage* pDeviceMessage = new ThreadMessage;
  NS_ENSURE_TRUE(pDeviceMessage, PR_FALSE);

  pDeviceMessage->message = aMessage;
  pDeviceMessage->data1 = aData1;
  pDeviceMessage->data2 = aData2;

  {
    nsAutoMonitor mon(mpDeviceThreadMonitor);
    mDeviceMessageQueue.push_back(pDeviceMessage);
    mDeviceQueueHasItem = PR_TRUE;
    mon.Notify();
  }

  return PR_TRUE;
}

NS_IMETHODIMP
sbDeviceBase::GetDeviceCategory(nsAString& aDeviceCategory)
{
  NS_NOTYETIMPLEMENTED("sbDeviceBase::GetDeviceCategory");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbDeviceBase::GetDeviceStringByIndex(PRUint32 aIndex,
                                     nsAString& _retval)
{
  NS_NOTYETIMPLEMENTED("sbDeviceBase::GetDeviceStringByIndex");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbDeviceBase::GetContext(const nsAString& aDeviceString,
                         nsAString& _retval)
{
  NS_NOTYETIMPLEMENTED("sbDeviceBase::GetContext");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
sbDeviceBase::GetName(nsAString& aName)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::SetName(const nsAString& aName)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::IsDownloadSupported(const nsAString& aDeviceString,
                                  PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::GetSupportedFormats(const nsAString& aDeviceString,
                                  PRUint32 *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::IsUploadSupported(const nsAString& aDeviceString,
                                PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::IsTransfering(const nsAString& aDeviceString,
                            PRBool *_retval)
{
  *_retval = IsTransferInProgress(aDeviceString);
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::IsDeleteSupported(const nsAString& aDeviceString,
                                PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::GetUsedSpace(const nsAString& aDeviceString,
                           PRUint32 *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::GetAvailableSpace(const nsAString& aDeviceString,
                                PRUint32 *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::GetTrackTable(const nsAString& aDeviceString,
                            nsAString& aDBContext,
                            nsAString& aTableName,
                            PRBool *_retval)
{
  return NS_OK;
}

PRBool
sbDeviceBase::StartTransfer(const PRUnichar* deviceString,
                            const PRUnichar* tableName)
{
  TransferData* data = new TransferData;
  NS_ENSURE_TRUE(data, PR_FALSE);

  data->deviceString = deviceString;

  // XXXben Remove me
  nsAutoString strDevice, strContext;
  if (deviceString)
    strDevice.Assign(deviceString);

  GetContext(strDevice, strContext);
  data->dbContext.Assign(strContext);
  data->dbTable.Assign(tableName);

  // Send a thread message if using a thread otherwise just call the Transfer
  // function.
  PRBool retval = (mUsingThread == PR_TRUE) ?
                  SubmitMessage(MSG_DEVICE_TRANSFER, (void*)-1, (void*)data) :
                  TransferNextFile(-1, data);

  return retval;
}

NS_IMETHODIMP
sbDeviceBase::AbortTransfer(const nsAString& aDeviceString,
                            PRBool *_retval)
{
  *_retval = StopCurrentTransfer(aDeviceString);
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::DeleteTable(const nsAString& aDeviceString,
                          const nsAString& aDBContext,
                          const nsAString& aTableName,
                          PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::IsUpdateSupported(const nsAString& aDeviceString,
                                PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::UpdateTable(const nsAString& aDeviceString,
                          const nsAString& aTableName,
                          PRBool *_retval)
{
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::IsEjectSupported(const nsAString& aDeviceString,
                               PRBool *_retval)
{
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::EjectDevice(const nsAString& aDeviceString,
                          PRBool *_retval)
{
  *_retval = PR_FALSE;
  if (IsEjectSupported(aDeviceString, _retval)) {
    SubmitMessage(MSG_DEVICE_EJECT, 0, 0);
    *_retval = PR_TRUE;
  }
  return NS_OK;
}

// Use the prevIndex to get the next row in the table which will have an "id"
// value greater than prevIndex.
PRBool
sbDeviceBase::GetNextTransferFileEntry(PRInt32 prevIndex,
                                       const PRUnichar *deviceString,
                                       PRBool bDownloading,
                                       PRInt32& curIndex,
                                       nsString& recvSource,
                                       nsString& recvDestination)
{

  // XXXben Remove me
  nsAutoString strDevice, strContext;
  if (deviceString)
    strDevice.Assign(deviceString);

  GetContext(strDevice, strContext);
  nsString transferTable;
  GetTransferTable(strDevice, bDownloading, transferTable);

  nsCOMPtr<sbIDatabaseResult> resultset;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");

  if (!query)
    return PR_FALSE;

  nsString query_str(NS_LITERAL_STRING("select * from "));
  query_str += transferTable;
  query_str += NS_LITERAL_STRING(" where id > ");
  query_str.AppendInt(prevIndex);
  query->SetAsyncQuery(PR_FALSE); 
  query->SetDatabaseGUID(strContext);
  query->AddQuery(query_str); 
  PRInt32 ret;
  query->Execute(&ret);
  if (ret)
    return PR_FALSE;

  query->GetResultObject(getter_AddRefs(resultset));

  PRInt32 rowcount = 0;
  resultset->GetRowCount( &rowcount );

  if ( rowcount )
  {
    // We only want the first record
    PRUint32 rowNumber = 0;
    nsAutoString id;
    resultset->GetRowCellByColumn(rowNumber, INDEX_COLUMN_NAME, id);
    
    PRInt32 conversionError = 0;
    curIndex = id.ToInteger(&conversionError);
    
    resultset->GetRowCellByColumn(rowNumber, SOURCE_COLUMN_NAME, recvSource);
    resultset->GetRowCellByColumn(rowNumber, DESTINATION_COLUMN_NAME, recvDestination);

    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool sbDeviceBase::TransferNextFile(PRInt32 prevTransferRowNumber, void *data)
{
  if (!data)
    return PR_FALSE;

  nsString dbContext = ((TransferData *)data)->dbContext;
  nsString tableName = ((TransferData *)data)->dbTable;
  PRUnichar* deviceString = (PRUnichar *) ((TransferData *)data)->deviceString.get();


  // Read the table for Transferring files
  nsCOMPtr<sbIDatabaseResult> resultset;
  nsCOMPtr<sbIDatabaseQuery> query = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );

  // This can happen when the app is shutting down
  if (!query)
    return PR_FALSE;

  nsString query_str(NS_LITERAL_STRING("select * from "));
  query_str += tableName;
  query->SetAsyncQuery(PR_FALSE); 
  query->SetDatabaseGUID(dbContext); 
  query->AddQuery(query_str); 

  PRInt32 ret;
  query->Execute(&ret);
  query->GetResultObject(getter_AddRefs(resultset));

  // These are the names of the columns in Transfer table
  // that will be used in obtaining the column indexes.
  const nsString sourcePathColumnName = SOURCE_COLUMN_NAME;
  const nsString destinationPathColumnName = DESTINATION_COLUMN_NAME;
  const nsString indexColumnName = INDEX_COLUMN_NAME;

  PRInt32 sourcePathColumnIndex = -1;
  PRInt32 destinationPathColumnIndex = -1;
  PRUint32 indexColumnIndex = -1;
  PRInt32 colCount;
  PRBool transferComplete = PR_TRUE;

  // Find the column indexes with the Transfer info
  resultset->GetColumnCount(&colCount);
  for (PRInt32 colNumber = 0; colNumber < colCount; colNumber ++)
  {
    nsAutoString columnName;
    resultset->GetColumnName(colNumber, columnName);
    if (sourcePathColumnName == columnName)
    {
      sourcePathColumnIndex = colNumber;
    }
    else
      if (destinationPathColumnName == columnName)
      {
        destinationPathColumnIndex = colNumber;
      }
      else
        if (indexColumnName == columnName)
        {
          indexColumnIndex = colNumber;
        }
  }

  if ((sourcePathColumnIndex != -1) && (destinationPathColumnIndex != -1)) 
  {

    // Now iterate thru the resultset to get the next Transfer
    // information.
    PRInt32 rowcount;
    resultset->GetRowCount( &rowcount );

/*
    mig - This does not seem to be an error.

    if ((rowcount > 0) && (prevTransferRowNumber == -1))
    {
      NS_NOTREACHED("(rowcount > 0) && (prevTransferRowNumber == -1)");
    }
*/

    // Iterate thru the list of download track records to
    // find the first available track to download.
    PRInt32 rowNumber = 0;
    for ( ; rowNumber < rowcount; rowNumber++ )
    {
      // Get progress value
      nsAutoString progressString;
      resultset->GetRowCellByColumn(rowNumber, NS_LITERAL_STRING("progress"), progressString);
      PRInt32 errorCode;
      PRInt32 progress = progressString.ToInteger(&errorCode);

      if (progress == 100)
      {
        continue; // This track has already completed downloading
      }

      // Transfer this file
      nsAutoString sourcePath, destinationPath, index;
      resultset->GetRowCell(rowNumber, sourcePathColumnIndex, sourcePath);
      resultset->GetRowCell(rowNumber, destinationPathColumnIndex, destinationPath);
      resultset->GetRowCell(rowNumber, indexColumnIndex, index);

      PRBool bRet = TransferFile(deviceString, 
        NS_CONST_CAST(PRUnichar *, PromiseFlatString(sourcePath).get()),
        NS_CONST_CAST(PRUnichar *, PromiseFlatString(destinationPath).get()), 
        NS_CONST_CAST(PRUnichar *, PromiseFlatString(dbContext).get()), 
        NS_CONST_CAST(PRUnichar *, PromiseFlatString(tableName).get()), 
        NS_CONST_CAST(PRUnichar *, PromiseFlatString(index).get()), 
        rowNumber);

      if(bRet)
      {
        DoTransferStartCallback(PromiseFlatString(sourcePath),
                                PromiseFlatString(destinationPath));
      }

      break;

    }

    if (rowNumber != rowcount)
    {
      // Last Transfer is not done yet
      transferComplete = PR_FALSE;
    }
  }

  if (transferComplete)
  {
    TransferComplete();
    DeviceIdle(deviceString);
  }

  delete data;
  data = nsnull;

  return PR_TRUE;
}

PRBool
sbDeviceBase::UpdateIOProgress(PRUnichar* deviceString,
                               PRUnichar* table,
                               PRUnichar* index,
                               PRUint32 percentComplete)
{
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
  nsString query_str;
  PRInt32 ret = -1;

  // XXXben Remove me
  nsAutoString strDevice, strContext;
  if (deviceString)
    strDevice.Assign(deviceString);

  GetContext(strDevice, strContext);

  if ( query ) 
  {
    query->SetAsyncQuery( PR_FALSE ); // Would need callbacks
    query->SetDatabaseGUID(strContext);

    // check if this record is removed, in which case
    // we should skip to transferring the next transfer table record.
    nsCOMPtr<sbIDatabaseResult> resultset;

    query_str = NS_LITERAL_STRING("select * from ");
    query_str += table;
    query_str += NS_LITERAL_STRING(" WHERE id = ");
    query_str += index;
    query->AddQuery(query_str);
    query->Execute(&ret);
    query->GetResultObject(getter_AddRefs(resultset));
    PRInt32 numRows = 0;
    resultset->GetRowCount(&numRows);
    query->ResetQuery();
    if (numRows == 0)
    {
      // XXXben Remove me
      nsAutoString str;
      if (deviceString)
        str.Assign(deviceString);
      // Yes, as we suspected ...
      StopCurrentTransfer(str);
      // Start a new transfer as this would initiate a transfer
      // on the next transfer table record.
      StartTransfer(deviceString, table);
    }
    else
    {

      query_str = NS_LITERAL_STRING("UPDATE ");
      query_str += table;
      query_str += NS_LITERAL_STRING(" SET ");
      query_str += NS_LITERAL_STRING(" Progress = ");
      query_str.AppendInt( percentComplete ); // ahem.  this is not a string.
      query_str += NS_LITERAL_STRING(" WHERE id = ");
      query_str += index;

      query->AddQuery(query_str);

      if (percentComplete == 100) 
      {
        // Update the location
        query_str = NS_LITERAL_STRING("UPDATE ");
        query_str += table;
        query_str += NS_LITERAL_STRING(" SET ");
        query_str += NS_LITERAL_STRING(" url = destination");
        query_str += NS_LITERAL_STRING(" WHERE id = ");
        query_str += index;

        query->AddQuery(query_str);
      }
      query->Execute(&ret);
    }
  }

  return (ret != 0);
}

PRBool
sbDeviceBase::UpdateIOStatus(PRUnichar* deviceString,
                             PRUnichar* table,
                             PRUnichar* index,
                             const PRUnichar* status)
{
  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  // XXXben Remove me
  nsAutoString strDevice, strContext;
  if (deviceString)
    strDevice.Assign(deviceString);

  GetContext(strDevice, strContext);

  nsString query_str(NS_LITERAL_STRING("Update "));
  query_str += table;
  query_str += NS_LITERAL_STRING(" set ");
  query_str += NS_LITERAL_STRING(" Status=\"");
  query_str += status;
  query_str += NS_LITERAL_STRING("\" where id=");
  query_str += index;

  query->SetAsyncQuery( PR_FALSE ); // Would need callbacks
  query->SetDatabaseGUID(strContext);
  query->AddQuery(query_str);
  PRInt32 ret;
  query->Execute(&ret);

  return (ret != 0);
}

NS_IMETHODIMP
sbDeviceBase::GetDestinationCount(const nsAString& aDeviceString,
                                  PRUint32* _retval)
{
  return NS_OK; 
}

NS_IMETHODIMP
sbDeviceBase::MakeTransferTable(const nsAString& aDeviceString,
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
  *_retval = CreateTransferTable(aDeviceString, aContextInput, aTableName,
                                 aFilterColumn, aFilterCount, aFilterValues,
                                 aSourcePath, aDestPath, aDownloading,
                                 aTransferTable);
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::DownloadTable(const nsAString& aDeviceString,
                            const nsAString& aTableName,
                            PRBool *_retval)
{
  // XXXben Remove me
  nsAutoString strDevice(aDeviceString), strTable(aTableName);

  // Initiate a download assuming the caller has previously called
  // MakeTransferTable() to create a download table in Device's DB context.
  *_retval = PR_FALSE;

  if (IsDeviceIdle(strDevice.get()))
  {
    DeviceDownloading(strDevice.get());
    *_retval = StartTransfer(strDevice.get(), strTable.get());
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::UploadTable(const nsAString& aDeviceString,
                          const nsAString& aTableName,
                          PRBool *_retval)
{
  // Initiate a download assuming the caller has previously called
  // MakeTransferTable() to create a download table in Device's DB context.
  *_retval = PR_FALSE;

  // XXXben Remove me
  nsAutoString strDevice(aDeviceString), strTable(aTableName);

  if (IsDeviceIdle(strDevice.get()))
  {
    DeviceUploading(strDevice.get());
    *_retval = StartTransfer(strDevice.get(), strTable.get());
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::GetDeviceCount(PRUint32* aDeviceCount)
{
  NS_ENSURE_ARG_POINTER(aDeviceCount);

  // Needs to change this to actual number of devices for this category
  *aDeviceCount = 1;

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::SuspendTransfer(const nsAString& aDeviceString,
                              PRBool *_retval)
{
  *_retval = PR_FALSE;

  // XXXben Remove me.
  nsAutoString str(aDeviceString);

  if (!IsTransferInProgress(aDeviceString))
    return NS_OK;

  *_retval = SuspendCurrentTransfer(aDeviceString);
  if (*_retval)
  {
    if (IsDownloadInProgress(str.get()))
      DeviceDownloadPaused(str.get());
    else if (IsUploadInProgress(str.get()))
      DeviceUploadPaused(str.get());
  }

  return NS_OK;
}

PRBool
sbDeviceBase::SuspendCurrentTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

NS_IMETHODIMP
sbDeviceBase::ResumeTransfer(const nsAString& aDeviceString,
                             PRBool *_retval)
{
  // XXXben Remove me
  nsAutoString str(aDeviceString);

  *_retval = PR_FALSE;

  if (!IsTransferPaused(str.get()))
    return NS_OK;

  *_retval = ResumeTransfer(aDeviceString);
  if (*_retval) {
    // Correct download status
    if (IsDownloadPaused(str.get()))
      DeviceDownloading(str.get());
    else
      DeviceUploading(str.get());
  }

  return NS_OK;
}

// Should be overridden in the derived class
// XXXben Maybe a different name for this... ResumeTransferInternal?
PRBool
sbDeviceBase::ResumeTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}


PRBool
sbDeviceBase::CreateTransferTable(const nsAString& aDeviceString,
                                  const nsAString& aContextInput,
                                  const nsAString& aTableName,
                                  const nsAString& aFilterColumn,
                                  PRUint32 aFilterCount,
                                  const PRUnichar** aFilterValues,
                                  const nsAString& aSourcePath,
                                  const nsAString& aDestPath,
                                  PRBool aDownloading,
                                  nsAString& aTransferTable)
{
  // XXXben Remove me
  nsAutoString strDevice(aDeviceString), deviceContext;

  GetContext(strDevice, deviceContext);

  // Obtain the schema for the passed table
  // for creating a similar table in device's database context.
  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);

  rv = query->SetAsyncQuery(PR_FALSE); 

  nsCOMPtr<sbIPlaylistManager> pPlaylistManager =
    do_CreateInstance("@songbirdnest.com/Songbird/PlaylistManager;1", &rv);

  nsAutoString destTable;
  GetTransferTable(aDeviceString, aDownloading, destTable);

  PRInt32 queryError = 0;

  // If the transfer table exists then get the number of
  // rows in the existing table.
  PRInt32 numExistingRows = 0;
  nsAutoString selectQueryTransferTable;
  selectQueryTransferTable.AssignLiteral("select * from ");
  selectQueryTransferTable += destTable;
  query->AddQuery(selectQueryTransferTable);
  query->SetDatabaseGUID(deviceContext);
  query->Execute(&queryError);
  if (!queryError)
  {
    nsCOMPtr<sbIDatabaseResult> resultsetSelectTransferTable;
    rv = query->GetResultObjectOrphan(getter_AddRefs(resultsetSelectTransferTable));
    resultsetSelectTransferTable->GetRowCount(&numExistingRows);
  }
  query->ResetQuery(); // Make it re-usable

#if 0
  nsAutoString text(aFilterColumn);
  text += NS_LITERAL_STRING(" - ");
  text.AppendInt(FilterCount);
  text += NS_LITERAL_STRING("\n");
  for (PRUint32 i = 0; i < FilterCount; i++) {
    text += NS_LITERAL_STRING("   ");
    text += filterValues[i];
    text += NS_LITERAL_STRING("\n");
  }
  printf("%S", text.get());
#endif

  nsAutoString transferTableDescription, transferTableReadable,
               transferTableType;

  if (aDownloading) {
    GetDeviceDownloadTableDescription(aDeviceString, transferTableDescription);
    GetDeviceDownloadTableType(aDeviceString, transferTableReadable);
    GetDeviceDownloadReadable(aDeviceString, transferTableReadable);
  }
  else {
    GetDeviceUploadTableDescription(aDeviceString, transferTableDescription);
    GetDeviceUploadTableType(aDeviceString, transferTableReadable);
    GetDeviceUploadTableReadable(aDeviceString, transferTableType);
  }

  query->SetDatabaseGUID(aContextInput);

  sbISimplePlaylist* dummyPlaylist;
  pPlaylistManager->GetSimplePlaylist(aTableName, query, &dummyPlaylist);


  nsCOMPtr<sbISimplePlaylist> pPlaylistDest;
  rv = pPlaylistManager->CopySimplePlaylist(aContextInput, aTableName,
                                            aFilterColumn, aFilterCount,
                                            aFilterValues, deviceContext,
                                            destTable,
                                            transferTableDescription,
                                            transferTableReadable,
                                            transferTableType, query,
                                            getter_AddRefs(pPlaylistDest));

  NS_ENSURE_TRUE(pPlaylistDest, PR_FALSE);

  // We need to add the extra columns and set the info for those columns only
  // if they do not currently exist in the transfer table. Since a greater than zero 
  // numExistingRows indicates that we have already done this, we might as well not 
  // do this when numExistingRows is greater than zero. 
  if (numExistingRows == 0)
  {
    rv = pPlaylistDest->AddColumn(NS_LITERAL_STRING("source"),
                                  NS_LITERAL_STRING("text"));
    rv = pPlaylistDest->AddColumn(NS_LITERAL_STRING("destination"),
                                  NS_LITERAL_STRING("text"));
    rv = pPlaylistDest->AddColumn(NS_LITERAL_STRING("progress"),
                                  NS_LITERAL_STRING("integer(0, 100)"));
    rv = pPlaylistDest->AddColumn(NS_LITERAL_STRING("status"),
                                  NS_LITERAL_STRING("text"));

    rv = pPlaylistDest->SetColumnInfo(NS_LITERAL_STRING("source"),
                                      NS_LITERAL_STRING("Source URL"),
                                      PR_TRUE, PR_TRUE, PR_FALSE, -10000, 70,
                                      NS_LITERAL_STRING("text"), PR_TRUE,
                                      PR_FALSE);
    rv = pPlaylistDest->SetColumnInfo(NS_LITERAL_STRING("destination"),
                                      NS_LITERAL_STRING("Destination URL"),
                                      PR_TRUE, PR_TRUE, PR_FALSE, -9000, 70,
                                      NS_LITERAL_STRING("text"), PR_TRUE,
                                      PR_FALSE);
    rv = pPlaylistDest->SetColumnInfo(NS_LITERAL_STRING("progress"),
                                      NS_LITERAL_STRING("Download Progress"),
                                      PR_TRUE, PR_TRUE, PR_FALSE, -8000, 20,
                                      NS_LITERAL_STRING("progress"), PR_TRUE,
                                      PR_FALSE);
    rv = pPlaylistDest->SetColumnInfo(NS_LITERAL_STRING("status"),
                                      NS_LITERAL_STRING("Status"), PR_TRUE,
                                      PR_TRUE, PR_FALSE, -7000, 20,
                                      NS_LITERAL_STRING("text"), PR_TRUE,
                                      PR_FALSE);
  }

  PRInt32 dummy;
  rv = pPlaylistDest->GetAllEntries(&dummy);

  nsCOMPtr<sbIDatabaseResult> resultset;
  rv = query->GetResultObjectOrphan(getter_AddRefs(resultset));

  PRBool isSourceGiven = !aSourcePath.IsEmpty();
  PRBool isDestinationGiven = !aDestPath.IsEmpty();

  // Now copy the transfer data
  PRInt32 rowcount;
  rv = resultset->GetRowCount(&rowcount);

  // Clean it up since we'll reuse it.
  rv = query->ResetQuery();

  // Set the destination and source info by correctly forming a 
  // fully qualified path for source and destination, but
  // do not set the source/destination for those existing
  // rows in the transfer table (bug 568). So starting 
  // with row = numExistingRows. 
  for (PRInt32 row = numExistingRows; row < rowcount; row++) {
    nsAutoString updateDataQuery, destinationPathFile, sourcePathFile;

    updateDataQuery.AssignLiteral("UPDATE \"");
    updateDataQuery += destTable + NS_LITERAL_STRING("\"") +
                       NS_LITERAL_STRING(" SET source = ");

    rv = resultset->GetRowCellByColumn(row, NS_LITERAL_STRING("source"),
                                       sourcePathFile);
    rv = resultset->GetRowCellByColumn(row, NS_LITERAL_STRING("destination"),
                                       destinationPathFile);

    if (isSourceGiven) {
      sourcePathFile = aSourcePath;
#if defined(XP_WIN)
      sourcePathFile += NS_LITERAL_STRING("\\");
#else
      sourcePathFile += NS_LITERAL_STRING("/");
#endif
    }

    if (isDestinationGiven) {
      destinationPathFile = aDestPath;
#if defined(XP_WIN)
      destinationPathFile += NS_LITERAL_STRING("\\");
#else
      destinationPathFile += NS_LITERAL_STRING("/");
#endif
    }

    nsAutoString dataString;
    rv = resultset->GetRowCellByColumn(row, NS_LITERAL_STRING("url"),
                                       dataString);

    nsAutoString fileName;
    rv = GetFileNameFromURL(NS_CONST_CAST(PRUnichar *, strDevice.get()),
                            dataString, fileName);

    if (!isDestinationGiven)
      destinationPathFile = dataString;
    else {
      fileName.ReplaceChar(FILE_ILLEGAL_CHARACTERS, '_');
      destinationPathFile += fileName;
    }

    if (!isSourceGiven)
      sourcePathFile = dataString;
    else
      sourcePathFile += fileName;

    // These are filled intelligently, well at least not a blind copy!
    AddQuotedString(updateDataQuery, sourcePathFile.get());
    updateDataQuery += NS_LITERAL_STRING(" destination = ");
    AddQuotedString(updateDataQuery, destinationPathFile.get(), PR_FALSE);
    updateDataQuery += NS_LITERAL_STRING(" WHERE url = ");
    AddQuotedString(updateDataQuery, dataString.get(), PR_FALSE);

    rv = query->AddQuery(updateDataQuery);
  }

  rv = query->Execute(&queryError);

  if (queryError) {
    // There's a bad error probably... might want to look into it, later :)
    NS_ERROR("CreateTransferTable queryError");
    return PR_FALSE;
  }

  // Now correct the indexes by matching index to rowid
  // This will create an unique index for each transfer record.
  nsAutoString adjustIndexesQuery;
  adjustIndexesQuery.AssignLiteral("UPDATE "); 
  adjustIndexesQuery += destTable;
  adjustIndexesQuery += NS_LITERAL_STRING(" SET id = rowid");
  rv = query->ResetQuery();
  rv = query->AddQuery(adjustIndexesQuery);
  rv = query->Execute(&queryError); 
  rv = query->ResetQuery();

  //Return the name of transfer table
  aTransferTable.Assign(destTable);

  return PR_TRUE;
}


void
sbDeviceBase::RemoveExistingTransferTableEntries(const PRUnichar* DeviceString,
                                                 PRBool isDownloadTable,
                                                 PRBool dropTable)
{	
  // XXXben Remove me
  nsAutoString strDevice, deviceContext;
  if (DeviceString)
    strDevice.Assign(DeviceString);

  GetContext(strDevice, deviceContext);

  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1" , &rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("Failed to create a DatabaseQuery object");
    return;
  }

  query->SetAsyncQuery(PR_FALSE); 
  query->SetDatabaseGUID(deviceContext);


  nsString deleteEntriesSQL;
  nsAutoString transferTable;
  GetTransferTable(strDevice, isDownloadTable, transferTable);
  if (!dropTable)
  {
    // If DeviceString is not provided then delete everything
    if (DeviceString) 
    {
      deleteEntriesSQL = NS_LITERAL_STRING("delete from ");
      deleteEntriesSQL += transferTable;
    }
    query->AddQuery(deleteEntriesSQL); 

    PRInt32 ret;
    query->Execute(&ret);
    query->ResetQuery();
  }
  else
  {
    nsCOMPtr<sbIPlaylistManager> pPlaylistManager =
      do_CreateInstance("@songbirdnest.com/Songbird/PlaylistManager;1");
    if(pPlaylistManager)
    {
      PRInt32 retVal = 0;
      pPlaylistManager->DeleteSimplePlaylist(transferTable, query, &retVal);
    }
  }
}

inline void
sbDeviceBase::AddQuotedString(nsString &destinationString,
                              const PRUnichar* sourceString,
                              PRBool suffixWithComma)
{
  destinationString += NS_LITERAL_STRING("\"");
  destinationString += sourceString;
  destinationString += NS_LITERAL_STRING("\"");
  if (suffixWithComma)
    destinationString += NS_LITERAL_STRING(",");
}

PRBool
sbDeviceBase::GetFileNameFromURL(const PRUnichar *DeviceString,
                                 nsString& url,
                                 nsString& fileName)
{
  // Extract file name from the url(path) string
  const PRUnichar backSlash = *(NS_LITERAL_STRING("\\").get());
  const PRUnichar frontSlash = *(NS_LITERAL_STRING("/").get());
  const PRUnichar dot = *(NS_LITERAL_STRING(".").get());

  PRInt32 foundPosition = url.RFindChar(frontSlash);
  if((foundPosition != -1) || ((foundPosition = url.RFindChar(backSlash))!=-1))
  {
    fileName = Substring(url, ++foundPosition);
    // Now add the extension if the device changes the format
    // of transferred file.

    // XXXben Remove me
    nsAutoString str;
    if (DeviceString)
      str.Assign(DeviceString);

    PRUint32 fileFormat;
    GetDownloadFileType(str, &fileFormat);
    if (kSB_DEVICE_FILE_FORMAT_UNDEFINED != fileFormat)
    {
      nsString outputFileExtension;
      if (GetFileExtension(fileFormat, outputFileExtension))
      {
        foundPosition = fileName.RFindChar(dot);
        if (foundPosition)
        {
          fileName = Substring(fileName, 0, foundPosition + 1);
          fileName += outputFileExtension;
        }
      }
    }
    return PR_TRUE;
  }

  return PR_FALSE;
}

PRBool
sbDeviceBase::GetFileExtension(PRUint32 fileFormat,
                               nsString& fileExtension)
{
  PRBool retval = true;

  switch (fileFormat)
  {
  case kSB_DEVICE_FILE_FORMAT_WAV:
    fileExtension = NS_LITERAL_STRING("wav");
    break;
  case kSB_DEVICE_FILE_FORMAT_MP3:
    fileExtension = NS_LITERAL_STRING("mp3");
    break;
  case kSB_DEVICE_FILE_FORMAT_WMA:
    fileExtension = NS_LITERAL_STRING("wma");
    break;
  default:
    retval = PR_FALSE;
    break;
  }

  return retval;
}

void
sbDeviceBase::GetTransferTable(const nsAString& aDeviceString,
                               PRBool aGetDownloadTable,
                               nsAString& _retval)
{
  
  if (aGetDownloadTable)
    GetDeviceDownloadTable(aDeviceString, _retval);
  else
    GetDeviceUploadTable(aDeviceString, _retval);
}

PRBool
sbDeviceBase::GetUploadFileFormat(PRUint32& fileFormat)
{
  return PR_FALSE;
}

PRBool
sbDeviceBase::GetDownloadFileFormat(PRUint32& fileFormat)
{
  return PR_FALSE;
}

PRBool
sbDeviceBase::GetSourceAndDestinationURL(const PRUnichar* dbContext,
                                         const PRUnichar* table,
                                         const PRUnichar* index,
                                         nsString& sourceURL,
                                         nsString& destURL)
{
  PRBool bRet = PR_FALSE;
  PRInt32 nRet = 0;

  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
  if (query == nsnull)
    return PR_FALSE;

  query->SetDatabaseGUID(nsAutoString(dbContext));

  nsString strQuery = NS_LITERAL_STRING("SELECT source, destination FROM ");
  AddQuotedString(strQuery, table, PR_FALSE);
  strQuery += NS_LITERAL_STRING(" WHERE id = ");
  strQuery += index;

  query->AddQuery(strQuery);
  query->SetAsyncQuery(PR_FALSE);
  query->Execute(&nRet);

  nsCOMPtr<sbIDatabaseResult> result;
  query->GetResultObject(getter_AddRefs(result));

  if(result) {
    PRUnichar *strSourceURL = nsnull, *strDestURL = nsnull;

    result->GetRowCellByColumn(0, NS_LITERAL_STRING("source"), sourceURL);
    result->GetRowCellByColumn(0, NS_LITERAL_STRING("destination"), destURL);

    bRet = PR_TRUE;
  }

  return bRet;
}

void
sbDeviceBase::DoTransferStartCallback(const nsAString& aSourceURL,
                                      const nsAString& aDestinationURL)
{
  nsAutoLock lock(mpCallbackListLock);
  size_t nSize = mCallbackList.size();
  for(size_t nCurrent = 0; nCurrent < nSize; ++nCurrent)
  {
    if(mCallbackList[nCurrent] != nsnull)
    {
      try
      {
        sbIDeviceBaseCallback *pCallback = mCallbackList[nCurrent];
        pCallback->OnTransferStart(aSourceURL, aDestinationURL);
      }
      catch(...)
      {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnTransferStart threw an exception");
      }
    }
  }
}

void
sbDeviceBase::DoTransferCompleteCallback(const nsAString& aSourceURL,
                                         const nsAString& aDestinationURL,
                                         PRInt32 aStatus)
{
  nsAutoLock lock(mpCallbackListLock);
  size_t nSize = mCallbackList.size();
  for(size_t nCurrent = 0; nCurrent < nSize; ++nCurrent)
  {
    if(mCallbackList[nCurrent] != nsnull)
    {
      try
      {
        sbIDeviceBaseCallback *pCallback = mCallbackList[nCurrent];
        pCallback->OnTransferComplete(aSourceURL, aDestinationURL, aStatus);
      }
      catch(...)
      {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnTransferComplete threw an exception");
      }
    }
  }
}

void
sbDeviceBase::DoDeviceConnectCallback(const nsAString& aDeviceString)
{
  nsAutoLock lock(mpCallbackListLock);
  size_t nSize = mCallbackList.size();
  for(size_t nCurrent = 0; nCurrent < nSize; ++nCurrent)
  {
    if(mCallbackList[nCurrent] != nsnull)
    {
      try
      {
        sbIDeviceBaseCallback *pCallback = mCallbackList[nCurrent];
        pCallback->OnDeviceConnect(aDeviceString);
      }
      catch(...)
      {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnDeviceConnect threw an exception");
      }
    }
  }
}

void
sbDeviceBase::DoDeviceDisconnectCallback(const nsAString& aDeviceString)
{
  nsAutoLock lock(mpCallbackListLock);
  size_t nSize = mCallbackList.size();
  for(size_t nCurrent = 0; nCurrent < nSize; ++nCurrent)
  {
    if(mCallbackList[nCurrent] != nsnull)
    {
      try
      {
        sbIDeviceBaseCallback *pCallback = mCallbackList[nCurrent];
        pCallback->OnDeviceDisconnect(aDeviceString);
      }
      catch(...)
      {
        //Oops. Someone is being really bad.
        NS_ERROR("pCallback->OnDeviceDisconnect threw an exception");
      }
    }
  }
}

NS_IMETHODIMP
sbDeviceBase::GetDeviceState(const nsAString& aDeviceString,
                             PRUint32 *_retval)
{
  *_retval = kSB_DEVICE_STATE_IDLE;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::RemoveTranferTracks(const nsAString& aDeviceString,
                                  PRUint32 aIndex,
                                  PRBool *_retval)
{
  // XXXben Remove me
  nsAutoString strDevice(aDeviceString), deviceContext;

  // Remove this track from transfer table
  GetContext(aDeviceString, deviceContext);

  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString deleteEntriesSQL;
  deleteEntriesSQL.AssignLiteral("delete from ");

  nsAutoString temp;
  GetTransferTable(strDevice, PR_TRUE, temp);
  deleteEntriesSQL += temp;

  deleteEntriesSQL += NS_LITERAL_STRING(" WHERE id = ");
  deleteEntriesSQL.AppendInt(aIndex);

  query->SetAsyncQuery(PR_FALSE); 
  query->SetDatabaseGUID(deviceContext);
  query->AddQuery(deleteEntriesSQL); 

  PRInt32 ret;
  query->Execute(&ret);

  // XXXben WTF
  *_retval = (ret == 0);

  *_retval = PR_FALSE;
  return NS_OK;
}

PRBool
sbDeviceBase::StopCurrentTransfer(const nsAString& aDeviceString)
{
  return PR_FALSE;
}

// ResumeAbortedTransfer() is different than ResumeTransfer()
// as ResumeAbortedTransfer() handles transfer re-initiation ic
// cases where app was closed or crashed in the middle of transfer.
void
sbDeviceBase::ResumeAbortedTransfer(const PRUnichar* deviceString)
{
  ResumeAbortedDownload(deviceString);
  ResumeAbortedUpload(deviceString);
}

void
sbDeviceBase::ResumeAbortedDownload(const PRUnichar* deviceString)
{
  nsString transferTable;
  GetDeviceDownloadTable(EmptyString(), transferTable);

  // XXXben Remove me
  nsAutoString strDevice, dbContext;
  if (deviceString)
    strDevice.Assign(deviceString);

  GetContext(strDevice, dbContext);

  // Read the table for Transferring files
  nsCOMPtr<sbIDatabaseResult> resultset;

  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("do_CreateInstance failed");
    return;
  }

  nsString query_str(NS_LITERAL_STRING("select * from "));
  query_str += transferTable;
  query->SetAsyncQuery(PR_FALSE); 
  query->SetDatabaseGUID(dbContext);
  query->AddQuery(query_str); 

  PRInt32 ret;
  query->Execute(&ret);
  query->GetResultObject(getter_AddRefs(resultset));

  // Now copy the transfer data
  PRInt32 rowcount;
  resultset->GetRowCount( &rowcount );

  for ( PRInt32 row = 0; row < rowcount; row++ ) {
    nsAutoString progressString;
    resultset->GetRowCellByColumn(row, NS_LITERAL_STRING("progress"), progressString);
    PRInt32 errorCode;
    PRInt32 progress = progressString.ToInteger(&errorCode);
    if (progress < 100) {
      // Unfinished transfer,
      // re-initiate it.
      DeviceDownloading(deviceString);

      TransferData *data = new TransferData;
      data->dbContext.Assign(dbContext);
      data->dbTable.Assign(transferTable);
      TransferNextFile(row-1, data);
      break;
    }
  }
}

void
sbDeviceBase::ResumeAbortedUpload(const PRUnichar* deviceString)
{
  nsString transferTable;
  GetDeviceUploadTable(EmptyString(), transferTable);

  // XXXben Remove me
  nsAutoString dbContext;

  GetContext(EmptyString(), dbContext);

  // Read the table for Transferring files
  nsCOMPtr<sbIDatabaseResult> resultset;
  
  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  if (NS_FAILED(rv)) {
    NS_ERROR("do_CreateInstance failed");
    return;
  }

  nsString query_str(NS_LITERAL_STRING("select * from "));
  query_str += transferTable;
  query->SetAsyncQuery(PR_FALSE); 
  query->SetDatabaseGUID(dbContext);
  query->AddQuery(query_str); 

  PRInt32 ret;
  query->Execute(&ret);
  query->GetResultObject(getter_AddRefs(resultset));

  // Now copy the transfer data
  PRInt32 rowcount;
  resultset->GetRowCount( &rowcount );

  for ( PRInt32 row = 0; row < rowcount; row++ )
  {
    nsAutoString progressString;
    resultset->GetRowCellByColumn(row, NS_LITERAL_STRING("progress"), progressString);
    PRInt32 errorCode;
    PRInt32 progress = progressString.ToInteger(&errorCode);
    if (progress < 100)
    {
      // Unfinished transfer,
      // re-initiate it.
      DeviceUploading(deviceString);

      TransferData *data = new TransferData;
      data->dbContext.Assign(dbContext);
      data->dbTable.Assign(transferTable);
      TransferNextFile(row-1, data);
      break;
    }
  }
}


NS_IMETHODIMP
sbDeviceBase::AutoDownloadTable(const nsAString& aDeviceString,
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
  nsAutoString strDevice(aDeviceString), strTable(aTransferTable);

  *_retval = PR_FALSE;
  if (IsDeviceIdle(strDevice.get()) || IsDownloadInProgress(strDevice.get()))
  {
    if (CreateTransferTable(aDeviceString, aContextInput, aTableName,
                            aFilterColumn, aFilterCount, aFilterValues,
                            aSourcePath, aDestPath, PR_TRUE, strTable))
    {
      // If we are already downloading then these newly added requests
      // will be picked up automatically on finishing the previous 
      // requests, so check for idle state ...
      if (IsDeviceIdle(strDevice.get()))
      {
        DeviceDownloading(strDevice.get());
        *_retval = StartTransfer(strDevice.get(), strTable.get());
        // XXXben Remove me
        aTransferTable.Assign(strTable);
      }
    }
    else
    {
      DeviceIdle(strDevice.get());
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::AutoUploadTable(const nsAString& aDeviceString,
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
  NS_ENSURE_ARG_POINTER(_retval);

  // XXXben Remove me
  nsAutoString strDevice(aDeviceString), strTable(aTransferTable);

  *_retval = PR_FALSE;
  if (IsDeviceIdle(strDevice.get()) || IsUploadInProgress(strDevice.get()))
  {
    if (CreateTransferTable(aDeviceString, aContextInput, aTableName,
                            aFilterColumn, aFilterCount, aFilterValues,
                            aSourcePath, aDestPath, PR_TRUE, strTable))
    {
      // If we are already uploading then these newly added requests
      // will be picked up automatically on finishing the previous 
      // requests, so check for idle state ...
      if (IsDeviceIdle(strDevice.get()))
      {
        DeviceUploading(strDevice.get());
        *_retval = StartTransfer(strDevice.get(), strTable.get());
        // XXXben Remove me
        aTransferTable.Assign(strTable);
      }
    }
    else
    {
      DeviceIdle(strDevice.get());
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::GetDownloadTable(const nsAString& aDeviceString,
                               nsAString& _retval)
{
  GetDeviceDownloadTable(aDeviceString, _retval);
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::GetUploadTable(const nsAString& aDeviceString,
                             nsAString& _retval)
{
  GetDeviceUploadTable(aDeviceString, _retval);
  return NS_OK;
}

// Creates a table with the usual columns for storing attributes for tracks
// NOTE: This function will drop the table if it already exists!.
PRBool
sbDeviceBase::CreateTrackTable(nsString& deviceString,
                               nsString& tableName)
{
  PRInt32 dbOpRetVal = 0;

  // XXXben Remove me
  nsAutoString dbContext;
  GetContext(deviceString, dbContext);

  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> query =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  // First get rid of any existing table by that name
  nsString dropTableQuery(NS_LITERAL_STRING("DROP TABLE "));
  dropTableQuery += tableName;
  query->SetAsyncQuery(PR_FALSE); 
  query->SetDatabaseGUID(dbContext);
  query->AddQuery(dropTableQuery); 
  query->Execute(&dbOpRetVal);

  if (dbOpRetVal == 0)
  {
    nsString query_str(NS_LITERAL_STRING("CREATE TABLE "));
    query_str += tableName;
    // Add the columns for attributes
    query_str += NS_LITERAL_STRING(" (idx integer primary key autoincrement, url text, name text, tim text, artist text, album text, genre text, length integer)");

    query->SetAsyncQuery(PR_FALSE); 
    query->SetDatabaseGUID(dbContext);
    query->AddQuery(query_str); 

    query->Execute(&dbOpRetVal);
  }

  return (dbOpRetVal == 0);
}

PRBool sbDeviceBase::AddTrack(nsString& deviceString, 
                              nsString& tableName, 
                              nsString& url,
                              nsString& name,
                              nsString& tim,
                              nsString& artist,
                              nsString& album,
                              nsString& genre,
                              PRUint32 length)
{
  // XXXben Remove me
  nsAutoString dbContext;
  GetContext(deviceString, dbContext);

  nsCOMPtr<sbIDatabaseQuery> query = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
  nsString addRecordQuery(NS_LITERAL_STRING("INSERT INTO "));
  addRecordQuery += tableName;
  addRecordQuery += NS_LITERAL_STRING("(url, name, tim, artist, album, genre, length) VALUES (");
  // Add all the column data here
  AddQuotedString(addRecordQuery, url.get());
  AddQuotedString(addRecordQuery, name.get());
  AddQuotedString(addRecordQuery, tim.get());
  AddQuotedString(addRecordQuery, artist.get());
  AddQuotedString(addRecordQuery, album.get());
  AddQuotedString(addRecordQuery, genre.get());

  addRecordQuery.AppendInt(length);

  addRecordQuery += NS_LITERAL_STRING(")");

  query->SetAsyncQuery(PR_FALSE); 
  query->SetDatabaseGUID(dbContext);
  query->AddQuery(addRecordQuery); 

  PRInt32 dbOpRetVal = 0;
  query->Execute(&dbOpRetVal);

  return (dbOpRetVal == 0);
}

PRBool
sbDeviceBase::InitializeAsync()
{
  if (mpDeviceThread) {
    SubmitMessage(MSG_DEVICE_INITIALIZE, (void *) -1, (void *) nsnull);
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
sbDeviceBase::FinalizeAsync()
{
  if (mpDeviceThread) {
    SubmitMessage(MSG_DEVICE_FINALIZE, (void *) -1, (void *) nsnull);
    return PR_TRUE;
  }
  return PR_FALSE;
}

PRBool
sbDeviceBase::DeviceEventAsync(PRBool mediaInserted)
{  
  if (mpDeviceThread) {
    DeviceEventSync( mediaInserted);
    return PR_TRUE;
  }
  return PR_FALSE;
}

void
sbDeviceBase::DownloadDone(PRUnichar* deviceString,
                           PRUnichar* table,
                           PRUnichar* index)
{
  nsString sourceURL, destURL;

  // XXXben Remove me
  nsAutoString strDevice, dbContext;
  if (deviceString)
    strDevice.Assign(deviceString);

  GetContext(strDevice, dbContext);

  if(sbDeviceBase::GetSourceAndDestinationURL(dbContext.get(), table, index, sourceURL, destURL))
  {
    DoTransferCompleteCallback(sourceURL, destURL, 1);

    const static PRUnichar *aMetaKeys[] = {NS_LITERAL_STRING("title").get()};
    const static PRUint32 nMetaKeyCount = sizeof(aMetaKeys) / sizeof(aMetaKeys[0]);

    nsString strFile;
    GetFileNameFromURL(deviceString, destURL, strFile);

    PRUnichar** aMetaValues = (PRUnichar **) nsMemory::Alloc(nMetaKeyCount * sizeof(PRUnichar *));
    aMetaValues[0] = (PRUnichar *) nsMemory::Clone(strFile.get(), (strFile.Length() + 1) * sizeof(PRUnichar));
    nsCOMPtr<sbIDatabaseQuery> pQuery = do_CreateInstance( "@songbirdnest.com/Songbird/DatabaseQuery;1" );
    pQuery->SetAsyncQuery(PR_FALSE);
    pQuery->SetDatabaseGUID(NS_LITERAL_STRING("songbird"));

    nsCOMPtr<sbIMediaLibrary> pLibrary = do_CreateInstance( "@songbirdnest.com/Songbird/MediaLibrary;1" );
    pLibrary->SetQueryObject(pQuery.get());

    //Make sure the filename is unique when download an item from a remote source.
    nsAutoString guid;
    pLibrary->AddMedia(destURL, nMetaKeyCount, aMetaKeys, nMetaKeyCount, const_cast<const PRUnichar **>(aMetaValues), PR_TRUE, PR_FALSE, guid);

    PRBool bRet = PR_FALSE;
    pLibrary->SetValueByGUID(guid, NS_LITERAL_STRING("url"), destURL, PR_FALSE, &bRet);
    pLibrary->SetValueByGUID(guid, NS_LITERAL_STRING("title"), strFile, PR_FALSE, &bRet);
    pLibrary->SetValueByGUID(guid, NS_LITERAL_STRING("length"), NS_LITERAL_STRING("0"), PR_FALSE, &bRet);

    PR_Free(aMetaValues[0]);
    PR_Free(aMetaValues);
  }

  TransferData* data = new TransferData;
  data->dbContext.Assign(dbContext);
  data->dbTable.Assign(table);
  data->deviceString.Assign(deviceString);
  TransferNextFile(GetCurrentTransferRowNumber(deviceString), data);
}


// The following functions need to be overridden in the derived
// classes to handle the calls synchronously.
PRBool
sbDeviceBase::InitializeSync()
{
  return PR_FALSE;
}

PRBool
sbDeviceBase::FinalizeSync()
{
  return PR_FALSE;
}

PRBool
sbDeviceBase::DeviceEventSync(PRBool mediaInserted)
{
  return PR_FALSE;
}

NS_IMETHODIMP
sbDeviceBase::SetDownloadFileType(const nsAString& aDeviceString,
                                  PRUint32 aFileType,
                                  PRBool *_retval)
{
  // Derived class needs to implement this
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::SetUploadFileType(const nsAString& aDeviceString,
                                PRUint32 aFileType,
                                PRBool *_retval)
{
  // Derived class needs to implement this
  *_retval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::GetDownloadFileType(const nsAString& aDeviceString,
                                  PRUint32 *_retval)
{
  *_retval = kSB_DEVICE_FILE_FORMAT_UNDEFINED;
  return NS_OK;
}

NS_IMETHODIMP
sbDeviceBase::GetUploadFileType(const nsAString& aDeviceString,
                                PRUint32 *_retval)
{
  *_retval = kSB_DEVICE_FILE_FORMAT_UNDEFINED;
  return NS_OK;
}

void
sbDeviceBase::TransferComplete()
{
}


/* End of implementation class template. */
