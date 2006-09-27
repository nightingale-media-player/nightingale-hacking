/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright� 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the �GPL�).
// 
// Software distributed under the License is distributed 
// on an �AS IS� basis, WITHOUT WARRANTY OF ANY KIND, either 
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
#include <xpcom/nsIObserverService.h>
#include <xpcom/nsCRT.h>
#include <pref/nsIPrefService.h>
#include <pref/nsIPrefBranch2.h>
#include <nsISupportsPrimitives.h>

#include "MetadataBackscanner.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#endif

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

  if (length == 0)
  {
    str = EmptyString();
    return;
  }

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
NS_IMPL_THREADSAFE_ISUPPORTS2(sbMetadataBackscanner, sbIMetadataBackscanner, nsIObserver)
NS_IMPL_THREADSAFE_ISUPPORTS1(BackscannerProcessorThread, nsIRunnable)

sbMetadataBackscanner::sbMetadataBackscanner()
: m_backscanShouldShutdown(PR_FALSE)
, m_backscanShouldRun(PR_FALSE)
, m_workerHasResultSet(PR_FALSE)
, m_activeCount(0)
, m_workerCurrentRow(0)
, m_scanInterval(3)
, m_updateQueueSize(33)
, m_pCurrentFileLock(nsnull)
, m_pBackscannerProcessorMonitor(nsnull)
{
  nsresult rv;

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  if(NS_SUCCEEDED(rv)) {
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  }
  else {
    NS_ERROR("Unable to register xpcom-shutdown observer");
  }

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
    rv = NS_NewThread(getter_AddRefs(m_pThread),
      pBackscannerProcessorRunner,
      0,
      PR_JOINABLE_THREAD);

    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start thread");

    // Create a monitored event queue for the new thread. This is needed for us
    // to get events posted from the channel.  Note that because this is a
    // monitored event queue, we must call ProcessPendingEvents() to cause the
    // events to be processed.
    nsCOMPtr<nsIEventQueueService> pEventQueueService;
    rv = NS_GetEventQueueService(getter_AddRefs(pEventQueueService));
    if(NS_SUCCEEDED(rv)) {
      rv = pEventQueueService->CreateFromIThread(m_pThread,
                                                 PR_FALSE,
                                                 getter_AddRefs(m_pEventQueue));
    }

    NS_ASSERTION(m_pEventQueue, "Unable to create event queue");
  }

  if ( ! m_StringBundle.get() )
  {
    nsIStringBundleService *  StringBundleService = nsnull;
    rv = CallGetService("@mozilla.org/intl/stringbundle;1", &StringBundleService );
    if ( NS_SUCCEEDED(rv) )
      rv = StringBundleService->CreateBundle( "chrome://songbird/locale/songbird.properties", getter_AddRefs( m_StringBundle ) );
  }
}

sbMetadataBackscanner::~sbMetadataBackscanner()
{
  if(m_pBackscannerProcessorMonitor) {
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

  PR_LOG(gMetadataLog, PR_LOG_DEBUG, ("Backscanner started"));
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

  PR_LOG(gMetadataLog, PR_LOG_DEBUG, ("Backscanner stopped"));

  return NS_OK;
}

/*static*/ nsresult PR_CALLBACK sbMetadataBackscanner::BackscannerProcessor(sbMetadataBackscanner* pBackscanner)
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
        break;
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

      if(resultRows)
        pBackscanner->m_activeCount++;

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

          if(pBackscanner->m_backscanShouldShutdown) break;

          while(completed == PR_FALSE &&
                !pBackscanner->m_backscanShouldShutdown &&
                values != 0)
          {
            // Process events until the channel has completed reading the
            // metadata.
            // XXX: Should we time out here if this takes too long?
            pBackscanner->m_pEventQueue->ProcessPendingEvents();
            pHandler->GetCompleted(&completed);

            if(pBackscanner->m_backscanShouldShutdown)
            {
              pHandler->Close();
              break;
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

            PR_LOG(gMetadataLog, PR_LOG_WARN,
                   ("Got metadata for url '%s', %d values",
                    NS_ConvertUTF16toUTF8(strURL).get(), values));

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
                      strValue.AssignLiteral("");
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
                strQuery.AppendLiteral(", length=\"\" ");
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
              strQuery.AppendLiteral(" length=\"\" WHERE uuid = \"");
              strQuery += strUUID;
              strQuery.AppendLiteral("\"");
              pBackscanner->m_pQuery->AddQuery(strQuery);
            }

            if(pBackscanner->m_backscanShouldShutdown) break;

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
        else {
          PR_LOG(gMetadataLog, PR_LOG_WARN,
                 ("Unable to get handler for url '%s'",
                  NS_ConvertUTF16toUTF8(strURL).get()));
        }
      }

      if(resultRows > 0 && pBackscanner->m_activeCount > 0)
        pBackscanner->m_activeCount--;
    }
    
  }

  nsCOMPtr<nsIEventQueueService> pEventQueueService;
  rv = NS_GetEventQueueService(getter_AddRefs(pEventQueueService));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = pEventQueueService->DestroyThreadEventQueue();
  NS_ENSURE_SUCCESS(rv, rv);

  PR_LOG(gMetadataLog, PR_LOG_DEBUG, ("BackscannerProcessor thread exited"));

  return NS_OK;
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
      PRInt32 rowCount = 0;
      
      p->m_workerHasResultSet = PR_TRUE;
      p->m_pIntervalResult->GetRowCount(&rowCount);
      
      if(rowCount)
        p->m_activeCount++;
    }
  }

  nsCOMPtr<nsIPrefService> pPref = do_GetService("@mozilla.org/preferences-service;1");
  if(!pPref)
    return;

  nsCOMPtr<nsIPrefBranch> pBranch;
  pPref->GetBranch("songbird.", getter_AddRefs(pBranch));
  if(!pBranch)
    return;

  nsresult rv;
  nsAutoString strValue;
  nsCOMPtr<nsISupportsString> strCurrentValue;

  pBranch->GetComplexValue("backscan.status", NS_GET_IID(nsISupportsString), getter_AddRefs(strCurrentValue));
  if (!strCurrentValue)
  {
    strCurrentValue = do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID, &rv);
    if (NS_FAILED(rv))
      return;
  }
  
  strCurrentValue->GetData(strValue);

  if(p->m_activeCount > 0 &&
     strValue.IsEmpty())
  {
    PRUnichar *value = nsnull;
    p->m_StringBundle->GetStringFromName( NS_LITERAL_STRING("back_scan.scanning").get(), &value );
    
    strValue = value;
    strValue.AppendLiteral(" ...");
    strCurrentValue->SetData(strValue);

    pBranch->SetComplexValue("backscan.status", NS_GET_IID(nsISupportsString), strCurrentValue);

    nsMemory::Free(value);
  }
  else if(p->m_activeCount <= 0 && !strValue.IsEmpty())
  {
    strValue = EmptyString();
    strCurrentValue->SetData(strValue);
    pBranch->SetComplexValue("backscan.status", NS_GET_IID(nsISupportsString), strCurrentValue);
    
    if(p->m_activeCount < 0)
      p->m_activeCount = 0;
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
              if(strValue.EqualsLiteral("0"))  {
                strValue.AssignLiteral("");
              }
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
        if(strQuery[strQuery.Length() - 1] != ',' && values != 0) 
          strQuery.AppendLiteral(",");
        strQuery.AppendLiteral(" length=\"\" ");
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

    if(p->m_activeCount > 0 &&
       rowCount)
      p->m_activeCount--;
  }

  return;
} //BackscannerTimerWorker

// nsIObserver
NS_IMETHODIMP
sbMetadataBackscanner::Observe(nsISupports *aSubject, const char *aTopic,
                               const PRUnichar *aData)
{
  if(!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    nsresult rv;

    PR_LOG(gMetadataLog, PR_LOG_DEBUG, ("Backscanner shutting down..."));

    {
      nsAutoMonitor mon(m_pBackscannerProcessorMonitor);
      m_backscanShouldShutdown = PR_TRUE;
      mon.Notify();
    }

    Stop();

    if(m_pThread) {
      m_pThread->Join();
    }

    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
  }
  return NS_OK;
}

