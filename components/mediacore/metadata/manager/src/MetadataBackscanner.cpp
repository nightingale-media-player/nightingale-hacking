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
* \file MetadataBackscanner.cpp
* \brief 
*/

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsAutoLock.h>

#include <string/nsReadableUtils.h>
#include <unicharutil/nsUnicharUtils.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsEventQueueUtils.h>

#include "MetadataBackscanner.h"

// DEFINES ====================================================================

// FUNCTIONS ==================================================================
//-----------------------------------------------------------------------------
void PrepareStringForQuery(nsAString &str)
{
  NS_NAMED_LITERAL_STRING(strQuote, "\"");
  NS_NAMED_LITERAL_STRING(strQuoteQuote, "\"\"");

  nsAString::const_iterator itStart, itS, itE, itEnd;
  
  str.BeginReading(itStart);
  str.BeginReading(itS);
  str.EndReading(itE);
  str.EndReading(itEnd);
  
  PRInt32 pos = 0;
  while(FindInReadable(strQuote, itS, itE))
  {
    itS--;
    if(*itS != '\\')
    {
      itS++;
      pos = Distance(itStart, itS);
      str.Replace(pos, 1, strQuoteQuote);
      pos += 2;
    }

    str.BeginReading(itStart);
    str.BeginReading(itS);
    str.EndReading(itE);
    str.EndReading(itEnd);

    itS.advance(pos);
  }

  return;
} //PrepareStringForQuery

//-----------------------------------------------------------------------------
void FormatLengthToString(nsAString &str)
{
  nsAutoString strConvert(str);
  PRInt32 errCode = 0;
  PRInt32 length = strConvert.ToInteger(&errCode);

  PRInt32 hours = 0;
  PRInt32 minutes = (length / 1000) / 60;
  PRInt32 seconds = (length / 1000) % 60;

  if(minutes >= 60)
  {
    hours = minutes / 60;
    minutes = minutes % 60;
  }

  strConvert = EmptyString();
  
  if(hours > 0)
  {
    strConvert.AppendInt(hours);
    strConvert.AppendLiteral(":");

    if(minutes < 10)
      strConvert.AppendLiteral("0");
  }

  strConvert.AppendInt(minutes);
  strConvert.AppendLiteral(":");

  if(seconds < 10)
    strConvert.AppendLiteral("0");

  strConvert.AppendInt(seconds);

  str = strConvert;

  return;
} //FormatLengthToString

// GLOBALS ====================================================================
sbMetadataBackscanner *gBackscanner = nsnull;

//Columns
NS_NAMED_LITERAL_STRING(strColUUID, "uuid");
NS_NAMED_LITERAL_STRING(strColSUUID, "service_uuid");
NS_NAMED_LITERAL_STRING(strColURL, "url");
NS_NAMED_LITERAL_STRING(strColLength, "length");
NS_NAMED_LITERAL_STRING(strColArtist, "artist");
NS_NAMED_LITERAL_STRING(strColAlbum, "album");
NS_NAMED_LITERAL_STRING(strColTitle, "title");
NS_NAMED_LITERAL_STRING(strColGenre, "genre");
NS_NAMED_LITERAL_STRING(strColYear, "year");
NS_NAMED_LITERAL_STRING(strColComposer, "composer");
NS_NAMED_LITERAL_STRING(strColTrackNo, "track_no");
NS_NAMED_LITERAL_STRING(strColTrackTotal, "track_total");
NS_NAMED_LITERAL_STRING(strColDiscNo, "disc_no");
NS_NAMED_LITERAL_STRING(strColDiscTotal, "disc_total");

//DB UUID.
NS_NAMED_LITERAL_STRING(strDatabaseGUID, "*");
NS_NAMED_LITERAL_STRING(strSongbirdGUID, "songbird");

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataBackscanner, sbIMetadataBackscanner)
NS_IMPL_THREADSAFE_ISUPPORTS1(BackscannerProcessorThread, nsIRunnable)

sbMetadataBackscanner::sbMetadataBackscanner()
: m_backscanShouldShutdown(PR_FALSE)
, m_backscanShouldRun(PR_FALSE)
, m_workerHasResultSet(PR_FALSE)
, m_workerCurrentRow(0)
, m_scanInterval(3)
, m_updateQueueSize(33)
, m_pCurrentFileLock(nsnull)
, m_pBackscannerProcessorMonitor(nsnull)
{
  //Metadata columns cache.
  m_columnCache.insert(strColURL);
  m_columnCache.insert(strColLength);
  m_columnCache.insert(strColArtist);
  m_columnCache.insert(strColAlbum);
  m_columnCache.insert(strColTitle);
  m_columnCache.insert(strColGenre);
  m_columnCache.insert(strColYear);
  m_columnCache.insert(strColComposer);
  m_columnCache.insert(strColTrackNo);
  m_columnCache.insert(strColTrackTotal);
  m_columnCache.insert(strColDiscNo);
  m_columnCache.insert(strColDiscTotal);

  m_pManager = do_CreateInstance("@songbirdnest.com/Songbird/MetadataManager;1");
  NS_ASSERTION(m_pManager, "Unable to create sbMetadataBackscanner::m_pManager");

  m_pQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ASSERTION(m_pQuery, "Unable to create sbMetadataBackscanner::m_pQuery");

  m_pIntervalQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ASSERTION(m_pIntervalQuery, "Unable to create sbMetadataBackscanner::m_pQuery");

  m_pWorkerQuery= do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ASSERTION(m_pWorkerQuery, "Unable to create sbMetadataBackscanner::m_pQuery");

  m_pQueryToScan = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ASSERTION(m_pQueryToScan, "Unable to create sbMetadataBackscanner::m_pQueryToScan");

  m_pIntervalTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  NS_ASSERTION(m_pIntervalTimer, "Unable to create sbMetadataBackscanner::m_pIntervalTimer");

  m_pWorkerTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  NS_ASSERTION(m_pWorkerTimer, "Unable to create sbMetadataBackscanner::m_pWorkerTimer");

  m_pBackscannerProcessorMonitor = nsAutoMonitor::NewMonitor("Backscanner Processor Monitor");
  NS_ASSERTION(m_pBackscannerProcessorMonitor, "Failed to create sbMetadataBackscanner::m_pBackscannerProcessorMonitor");

  nsCOMPtr<nsIThread> pThread;
  nsCOMPtr<nsIRunnable> pBackscannerProcessorRunner = new BackscannerProcessorThread(this);
  
  NS_ASSERTION(pBackscannerProcessorRunner, "Unable to create BackscannerProcessorRunner");

  if (pBackscannerProcessorRunner)
  {
    nsresult rv = NS_NewThread(getter_AddRefs(pThread),
      pBackscannerProcessorRunner,
      0,
      PR_JOINABLE_THREAD);

    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start thread");

    if (NS_SUCCEEDED(rv))
    {
      m_pThread = pThread;

      //Windows and MacOSX accept the creation of an event queue on another thread
      //than the main thread. Linux however only accepts event queue usage from the
      //main thread.
#if !defined(XP_UNIX) || defined(XP_MACOSX)
      nsCOMPtr<nsIEventQueueService> pEventQueueService;
      rv = NS_GetEventQueueService(getter_AddRefs(pEventQueueService));
      if(NS_SUCCEEDED(rv)) {
        rv = pEventQueueService->CreateFromIThread(m_pThread, PR_TRUE, getter_AddRefs(m_pEventQueue));
      }
#endif
    }
  }
}

sbMetadataBackscanner::~sbMetadataBackscanner()
{
  m_backscanShouldShutdown = PR_TRUE;
  Stop();

  if(m_pThread)
    m_pThread->Join();

  if(m_pBackscannerProcessorMonitor)
  {
    nsAutoMonitor::DestroyMonitor(m_pBackscannerProcessorMonitor);
    m_pBackscannerProcessorMonitor = nsnull;
  }
}

sbMetadataBackscanner * sbMetadataBackscanner::GetSingleton()
{
  if (gBackscanner) {
    NS_ADDREF(gBackscanner);
    return gBackscanner;
  }

  NS_NEWXPCOM(gBackscanner, sbMetadataBackscanner);
  if (!gBackscanner)
    return nsnull;

  //One of these addref's is for the global instance we use.
  NS_ADDREF(gBackscanner);
  //This one is for the interface.
  NS_ADDREF(gBackscanner);

  return gBackscanner;
}

/* attribute PRUint32 scanInterval; */
NS_IMETHODIMP sbMetadataBackscanner::GetScanInterval(PRUint32 *aScanInterval)
{
  NS_ENSURE_ARG_POINTER(aScanInterval);
  *aScanInterval = m_scanInterval;
  return NS_OK;
}
NS_IMETHODIMP sbMetadataBackscanner::SetScanInterval(PRUint32 aScanInterval)
{
  m_scanInterval = aScanInterval;
  return NS_OK;
}

/* attribute PRUint32 scannedMediaCount; */
NS_IMETHODIMP sbMetadataBackscanner::GetScannedMediaCount(PRUint32 *aScannedMediaCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbMetadataBackscanner::SetScannedMediaCount(PRUint32 aScannedMediaCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute PRUint32 totalMediaCount; */
NS_IMETHODIMP sbMetadataBackscanner::GetTotalMediaCount(PRUint32 *aTotalMediaCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbMetadataBackscanner::SetTotalMediaCount(PRUint32 aTotalMediaCount)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* attribute PRUint32 updateQueueSize; */
NS_IMETHODIMP sbMetadataBackscanner::GetUpdateQueueSize(PRUint32 *aUpdateQueueSize)
{
  NS_ENSURE_ARG_POINTER(aUpdateQueueSize);
  *aUpdateQueueSize = m_updateQueueSize;
  return NS_OK;
}
NS_IMETHODIMP sbMetadataBackscanner::SetUpdateQueueSize(PRUint32 aUpdateQueueSize)
{
  m_updateQueueSize = aUpdateQueueSize;
  return NS_OK;
}

/* attribute AString currentFile; */
NS_IMETHODIMP sbMetadataBackscanner::GetCurrentFile(nsAString & aCurrentFile)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP sbMetadataBackscanner::SetCurrentFile(const nsAString & aCurrentFile)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

/* void start (); */
NS_IMETHODIMP sbMetadataBackscanner::Start()
{
  {
    nsAutoMonitor mon(m_pBackscannerProcessorMonitor);
    m_backscanShouldRun = PR_TRUE;
    mon.Notify();
  }

  m_pIntervalTimer->InitWithFuncCallback(&BackscannerTimerInterval, this, 
    m_scanInterval * 500, nsITimer::TYPE_REPEATING_SLACK);

  m_pWorkerTimer->InitWithFuncCallback(&BackscannerTimerWorker, this, 
    99, nsITimer::TYPE_REPEATING_SLACK);

  return NS_OK;
}

/* void stop (); */
NS_IMETHODIMP sbMetadataBackscanner::Stop()
{
  {
    nsAutoMonitor mon(m_pBackscannerProcessorMonitor);
    m_backscanShouldRun = PR_FALSE;
    mon.Notify();
  }

  m_pIntervalTimer->Cancel();
  m_pWorkerTimer->Cancel();

  return NS_OK;
}

/*static*/ void PR_CALLBACK sbMetadataBackscanner::BackscannerProcessor(sbMetadataBackscanner* pBackscanner)
{
  nsresult rv = 0;

  //Columns
  NS_NAMED_LITERAL_STRING(strColUUID, "uuid");
  NS_NAMED_LITERAL_STRING(strColSUUID, "service_uuid");
  NS_NAMED_LITERAL_STRING(strColURL, "url");
  NS_NAMED_LITERAL_STRING(strColLength, "length");
  NS_NAMED_LITERAL_STRING(strColArtist, "artist");
  NS_NAMED_LITERAL_STRING(strColAlbum, "album");
  NS_NAMED_LITERAL_STRING(strColTitle, "title");
  NS_NAMED_LITERAL_STRING(strColGenre, "genre");
  NS_NAMED_LITERAL_STRING(strColYear, "year");
  NS_NAMED_LITERAL_STRING(strColComposer, "composer");
  NS_NAMED_LITERAL_STRING(strColTrackNo, "track_no");
  NS_NAMED_LITERAL_STRING(strColTrackTotal, "track_total");
  NS_NAMED_LITERAL_STRING(strColDiscNo, "disc_no");
  NS_NAMED_LITERAL_STRING(strColDiscTotal, "disc_total");
  
  //DB UUID.
  NS_NAMED_LITERAL_STRING(strDatabaseGUID, "songbird");
  NS_NAMED_LITERAL_STRING(strSongbirdGUID, "songbird");
  
  //The query to find stuff that needs to be scanned.
  NS_NAMED_LITERAL_STRING(strDatabaseQueryToScan, 
    "SELECT uuid, url, length, title, service_uuid FROM library where length=\"0\" and substr(url, 1, 5) LIKE \"file:\" ");

  //Metadata columns cache.
  metacolscache_t columnCache, attachedCache;
  columnCache.insert(strColURL);
  columnCache.insert(strColLength);
  columnCache.insert(strColArtist);
  columnCache.insert(strColAlbum);
  columnCache.insert(strColTitle);
  columnCache.insert(strColGenre);
  columnCache.insert(strColYear);
  columnCache.insert(strColComposer);
  columnCache.insert(strColTrackNo);
  columnCache.insert(strColTrackTotal);
  columnCache.insert(strColDiscNo);
  columnCache.insert(strColDiscTotal);

  while(PR_TRUE)
  {
    { // Enter Monitor
      nsAutoMonitor mon(pBackscanner->m_pBackscannerProcessorMonitor);
      mon.Wait(PR_SecondsToInterval(pBackscanner->m_scanInterval));

      // Handle shutdown request
      if (pBackscanner->m_backscanShouldShutdown) {
        return;
      }

      if (!pBackscanner->m_backscanShouldRun) {
        continue;
      }
    } // Exit Monitor

    pBackscanner->m_pQueryToScan->ResetQuery();

    //Find items that need to be scanned in the database.
    pBackscanner->m_pQueryToScan->SetDatabaseGUID(strDatabaseGUID);
    pBackscanner->m_pQueryToScan->SetAsyncQuery(PR_FALSE);
    pBackscanner->m_pQueryToScan->AddQuery(strDatabaseQueryToScan);
    
    PRInt32 dbError = 0;
    pBackscanner->m_pQueryToScan->Execute(&dbError);

    rv = pBackscanner->m_pQueryToScan->GetResultObject(getter_AddRefs(pBackscanner->m_pResultToScan));
    if(NS_SUCCEEDED(rv))
    {
      //Go through each item, try and read metadata. If at any time we are requested to stop,
      //initiate the stop request and return to wait state.

      PRInt32 currentRow = 0;
      PRInt32 resultRows = 0;
      rv = pBackscanner->m_pResultToScan->GetRowCount(&resultRows);

      for(; currentRow < resultRows; currentRow++)
      {
        nsCOMPtr<sbIMetadataHandler> pHandler;
        nsAutoString strURL, strUUID, strSUUID;

        pBackscanner->m_pResultToScan->GetRowCellByColumn(currentRow, strColURL, strURL);
        pBackscanner->m_pResultToScan->GetRowCellByColumn(currentRow, strColUUID, strUUID);
        pBackscanner->m_pResultToScan->GetRowCellByColumn(currentRow, strColSUUID, strSUUID);

        rv = pBackscanner->m_pManager->GetHandlerForMediaURL(strURL, getter_AddRefs(pHandler));

        if(NS_SUCCEEDED(rv) && pHandler)
        {
          PRInt32 values = -1;
          PRBool  completed = PR_FALSE;
          
          rv = pHandler->Read(&values);
          pHandler->GetCompleted(&completed);

          while(completed == PR_FALSE &&
                !pBackscanner->m_backscanShouldShutdown &&
                values != 0)
          {
            pHandler->GetCompleted(&completed);
            if(!completed)
              PR_Sleep(PR_MillisecondsToInterval(33)); //Don't chew up all the cpu time.

            if(pBackscanner->m_backscanShouldShutdown)
            {
              pHandler->Close();
              return;
            }
          }

          nsCOMPtr<sbIMetadataValues> pValues;
          if(completed &&
             NS_SUCCEEDED(rv = pHandler->GetValues(getter_AddRefs(pValues))) &&
             pValues)
          {
            nsAutoString strQuery;
            nsAutoString strKey;
            nsAutoString strValue;

            PRInt32 queryCount = 0;
            PRInt32 curValue = 0;

            PRBool mustAttachDatabase = PR_FALSE;

            metacolscache_t::const_iterator itAttached = attachedCache.find(strSUUID);
            if(!strSUUID.Equals(strSongbirdGUID))
            {
              mustAttachDatabase = PR_TRUE;

              if(itAttached == attachedCache.end())
              {
                strQuery.AppendLiteral("ATTACH DATABASE \"");
                strQuery += strSUUID;
                strQuery.AppendLiteral(".db\" AS \"");
                strQuery += strSUUID;
                strQuery.AppendLiteral("\"");

                pBackscanner->m_pQuery->AddQuery(strQuery);
                strQuery = EmptyString();

                attachedCache.insert(strSUUID);
              }
            }

            strQuery.AppendLiteral("UPDATE ");

            if(mustAttachDatabase)
            {
              strQuery.AppendLiteral("\"");
              strQuery += strSUUID;
              strQuery.AppendLiteral("\".");
            }

            strQuery.AppendLiteral("library SET ");
            pValues->GetNumValues(&values);

            if(values)
            {
              PRBool seenLength = PR_FALSE;
              for(; curValue < values; curValue++)
              {
                pValues->GetKey(curValue, strKey);
                pValues->GetValue(strKey, strValue);

                if(columnCache.find(strKey) != columnCache.end())
                {
                  if(strKey.Equals(strColLength))
                  {
                    seenLength = PR_TRUE;
                    if(strValue.EqualsLiteral("0")) 
                    {
                      strValue.AssignLiteral("0:00");
                    }
                    else
                    {
                      FormatLengthToString(strValue);
                    }
                  }
                  
                  PrepareStringForQuery(strValue);

                  strQuery += strKey;
                  strQuery.AppendLiteral(" = \"");
                  strQuery += strValue;
                  strQuery.AppendLiteral("\"");

                  if(curValue + 1 < values)
                    strQuery.AppendLiteral(",");
                }

                strKey = EmptyString();
                strValue = EmptyString();
              }

              if(strQuery[strQuery.Length() - 1] == ',')
              {
                strQuery.Cut(strQuery.Length() - 1, 1);
              }

              if(!seenLength)
              {
                strQuery.AppendLiteral(", length=\"0:00\" ");
              }

              strQuery.AppendLiteral(" WHERE uuid = \"");
              strQuery += strUUID;
              strQuery.AppendLiteral("\"");

              pBackscanner->m_pQuery->AddQuery(strQuery);
              strQuery = EmptyString();

              mustAttachDatabase = PR_FALSE;
            }
            else
            {
              strQuery.AppendLiteral(" length=\"0:00\" WHERE uuid = \"");
              strQuery += strUUID;
              strQuery.AppendLiteral("\"");
              pBackscanner->m_pQuery->AddQuery(strQuery);
            }

            if(pBackscanner->m_backscanShouldShutdown) return;

            pBackscanner->m_pQuery->GetQueryCount(&queryCount);
            if(queryCount >= pBackscanner->m_updateQueueSize ||
               currentRow + 1 >= resultRows)
            {
              PRInt32 queryError = 0;
              
              nsAutoString strQuery;
              metacolscache_t::const_iterator itAttached = attachedCache.begin();
              for(; itAttached != attachedCache.end(); itAttached++)
              {
                strQuery.AppendLiteral("DETACH DATABASE \"");
                strQuery += strSUUID;
                strQuery.AppendLiteral("\"");

                pBackscanner->m_pQuery->AddQuery(strQuery);
                strQuery = EmptyString();
              }

              pBackscanner->m_pQuery->SetAsyncQuery(PR_FALSE);
              pBackscanner->m_pQuery->SetDatabaseGUID(strSongbirdGUID);

              pBackscanner->m_pQuery->Execute(&queryError);

              pBackscanner->m_pQuery->ResetQuery();
              attachedCache.clear();
            }
          }
        }
      }
    }
    
  }

  return;
}

/*static*/ void sbMetadataBackscanner::BackscannerTimerInterval(nsITimer *aTimer, void *aClosure)
{
  if(!aTimer) return;
  if(!aClosure) return;

  sbMetadataBackscanner *p = NS_REINTERPRET_CAST(sbMetadataBackscanner *, aClosure);

  PRBool isQueryRunning = PR_FALSE;
  PRInt32 queryCount = 0;
  p->m_pIntervalQuery->IsExecuting(&isQueryRunning);
  p->m_pIntervalQuery->GetQueryCount(&queryCount);

  if(!isQueryRunning &&
     !p->m_workerHasResultSet &&
     queryCount == 0)
  {
    //DB UUID.
    NS_NAMED_LITERAL_STRING(strDatabaseGUID, "*");

    //The query to find stuff that needs to be scanned.
    NS_NAMED_LITERAL_STRING(strDatabaseQueryToScan, 
      "SELECT uuid, url, length, title, service_uuid FROM library where length=\"0\" and substr(url, 1, 5) NOT LIKE \"file:\" ");

    p->m_pIntervalQuery->ResetQuery();

    //Find items that need to be scanned in the database.
    p->m_pIntervalQuery->SetDatabaseGUID(strDatabaseGUID);
    p->m_pIntervalQuery->SetAsyncQuery(PR_TRUE);
    p->m_pIntervalQuery->AddQuery(strDatabaseQueryToScan);

    PRInt32 errQuery = 0;
    p->m_pIntervalQuery->Execute(&errQuery);
  }
  else if(!isQueryRunning &&
    !p->m_workerHasResultSet &&
    queryCount == 1)
  {
    nsresult rv = p->m_pIntervalQuery->GetResultObject(getter_AddRefs(p->m_pIntervalResult));
    if(NS_SUCCEEDED(rv))
    {
      p->m_workerHasResultSet = PR_TRUE;
    }
  }

  return;
}

/*static*/ void sbMetadataBackscanner::BackscannerTimerWorker(nsITimer *aTimer, void *aClosure)
{
  if(!aTimer) return;
  if(!aClosure) return;

  sbMetadataBackscanner *p = NS_REINTERPRET_CAST(sbMetadataBackscanner *, aClosure);

  if(!p->m_workerHasResultSet) return;
  
  PRBool queryRunning = PR_FALSE;
  p->m_pWorkerQuery->IsExecuting(&queryRunning);
  if(queryRunning) return;
  
  PRInt32 rowCount = 0;
  p->m_pIntervalResult->GetRowCount(&rowCount);

  if(p->m_workerCurrentRow < rowCount)
  {
    nsAutoString strURL, strUUID, strSUUID;
    PRInt32 values = -1;
    PRBool  completed = PR_FALSE;

    p->m_pIntervalResult->GetRowCellByColumn(p->m_workerCurrentRow, strColURL, strURL);
    p->m_pIntervalResult->GetRowCellByColumn(p->m_workerCurrentRow, strColUUID, strUUID);
    p->m_pIntervalResult->GetRowCellByColumn(p->m_workerCurrentRow, strColSUUID, strSUUID);

    //First time attempting to read metadata from this row.
    if(!p->m_pWorkerHandler)
    {
      nsresult rv = p->m_pManager->GetHandlerForMediaURL(strURL, getter_AddRefs(p->m_pWorkerHandler));
      if(NS_FAILED(rv) || !p->m_pWorkerHandler) return;
      rv = p->m_pWorkerHandler->Read(&values);
      NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to initiate read of metadata!");
    }

    p->m_pWorkerHandler->GetCompleted(&completed);
    if(completed)
    {
      PRBool seenLength = PR_FALSE;
      nsAutoString strQuery;
      nsAutoString strKey;
      nsAutoString strValue;
      nsCOMPtr<sbIMetadataValues> pValues;

      strQuery.AppendLiteral("UPDATE ");
      strQuery.AppendLiteral("library SET ");

      p->m_pWorkerHandler->GetValues(getter_AddRefs(pValues));
      pValues->GetNumValues(&values);

      if(values)
      {
        for(PRInt32 curValue = 0; curValue < values; curValue++)
        {
          pValues->GetKey(curValue, strKey);
          pValues->GetValue(strKey, strValue);

          if(p->m_columnCache.find(strKey) != p->m_columnCache.end())
          {
            if(strKey.Equals(strColLength))
            {
              seenLength = PR_TRUE;
              if(strValue.EqualsLiteral("0")) 
                strValue.AssignLiteral("0:00");
              else
                FormatLengthToString(strValue);
            }

            PrepareStringForQuery(strValue);

            strQuery += strKey;
            strQuery.AppendLiteral(" = \"");
            strQuery += strValue;
            strQuery.AppendLiteral("\"");

            if(curValue + 1 < values)
              strQuery.AppendLiteral(",");
          }

          strKey = EmptyString();
          strValue = EmptyString();
        }
      }

      if(!seenLength)
      {
        if(strQuery[strQuery.Length() - 1] != ',' && values != 0) strQuery.AppendLiteral(",");
        strQuery.AppendLiteral(" length=\"0:00\" ");
      }

      if(strQuery[strQuery.Length() - 1] == ',')
        strQuery.Cut(strQuery.Length() - 1, 1);

      strQuery.AppendLiteral(" WHERE uuid = \"");
      strQuery += strUUID;
      strQuery.AppendLiteral("\"");

      PRInt32 errQuery = 0;
      p->m_pWorkerQuery->ResetQuery();
      p->m_pWorkerQuery->SetDatabaseGUID(strSUUID);
      p->m_pWorkerQuery->AddQuery(strQuery);
      p->m_pWorkerQuery->SetAsyncQuery(PR_TRUE);
      p->m_pWorkerQuery->Execute(&errQuery);

      p->m_pWorkerHandler->Close();
      p->m_pWorkerHandler = nsnull;
      p->m_workerCurrentRow++;
    }
  }
  else
  {
    p->m_pIntervalQuery->ResetQuery();
    p->m_workerHasResultSet = PR_FALSE;
    p->m_workerCurrentRow = 0;
  }

  
  return;
} //BackscannerTimerWorker