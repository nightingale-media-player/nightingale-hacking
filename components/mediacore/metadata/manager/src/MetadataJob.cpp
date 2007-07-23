/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
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
* \file MetadataJob.cpp
* \brief 
*/

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>
#include <nsAutoLock.h>

#include <nsArrayUtils.h>
#include <unicharutil/nsUnicharUtils.h>
#include <nsComponentManagerUtils.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsIObserverService.h>
#include <xpcom/nsCRTGlue.h>
#include <nsISupportsPrimitives.h>
#include <nsIFile.h>
#include <nsIFileURL.h>
#include <nsAutoPtr.h>
#include <nsThreadUtils.h>
#include <nsMemory.h>
#include <nsNetUtil.h>

#include <nsIURI.h>
#include <nsIIOService.h>
#include <pref/nsIPrefService.h>
#include <pref/nsIPrefBranch2.h>

#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbILibraryResource.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbILocalDatabasePropertyCache.h>
#include <sbIPropertyArray.h>
#include <sbIPropertyManager.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbISQLBuilder.h>
#include <sbStandardProperties.h>
#include <sbPropertiesCID.h>
#include <sbSQLBuilderCID.h>
#include <sbProxyUtils.h>

#include "MetadataJob.h"
#include "sbMetadataUtils.h"

// This define is to enable/disable proxying calls to the library and media
// items to the main thread.  I am leaving this here so we can experiment with
// the performance of threads talking to the library via a proxy.  If this is
// acceptable, maybe we can make the library main thread only and save us some
// pain.
#define PROXY_LIBRARY 0

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#define TRACE(args) PR_LOG(gMetadataLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMetadataLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

// DEFINES ====================================================================

// GLOBALS ====================================================================
const PRUint32 NUM_CONCURRENT_MAINTHREAD_ITEMS = 3;
const PRUint32 NUM_ITEMS_BEFORE_FLUSH = 50;
const PRUint32 NUM_ITEMS_PER_INIT_LOOP = 100;
const PRUint32 TIMER_LOOP_MS = 100;

// CLASSES ====================================================================
class sbMetadataBatchHelper
{
public:
  sbMetadataBatchHelper(sbIMediaList* aList = nsnull)
  {
    SetList( aList );
  }

  ~sbMetadataBatchHelper()
  {
    if ( mList ) {
      mList->EndUpdateBatch();
    }
  }

  void SetList(sbIMediaList* aList)
  {
    mList = aList;
    if ( mList )
      mList->BeginUpdateBatch();
  }

  void Flush()
  {
    if ( mList )
    {
      mList->EndUpdateBatch();
      mList->BeginUpdateBatch();
    }
  }

private:

  nsCOMPtr<sbIMediaList> mList;
};

NS_IMPL_ADDREF(sbMetadataJob::jobitem_t)
NS_IMPL_RELEASE(sbMetadataJob::jobitem_t)

NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataJob, sbIMetadataJob);
NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataJobProcessorThread, nsIRunnable)

sbMetadataJob::sbMetadataJob() :
  mInitCount( 0 ),
  mMetadataJobProcessor( nsnull ),
  mCompleted( PR_FALSE ),
  mInitCompleted( PR_FALSE ),
  mInitExecuted( PR_FALSE ),
  mTimerCompleted( PR_FALSE ),
  mThreadCompleted( PR_FALSE )
{
  MOZ_COUNT_CTOR(sbMetadataJob);
}

sbMetadataJob::~sbMetadataJob()
{
  MOZ_COUNT_DTOR(sbMetadataJob);
}

nsresult
sbMetadataJob::FactoryInit()
{
  nsresult rv;

  mTimerWorkers.SetLength(NUM_CONCURRENT_MAINTHREAD_ITEMS);

  mMainThreadQuery =
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainThreadQuery->SetDatabaseGUID( sbMetadataJob::DATABASE_GUID() );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);

  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  mDataStatusDisplay =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataStatusDisplay->Init( NS_LITERAL_STRING("songbird.backscan.status"),
                                 NS_LITERAL_STRING("") );
  NS_ENSURE_SUCCESS(rv, rv);

  mDataCurrentMetadataJobs =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataCurrentMetadataJobs->Init( NS_LITERAL_STRING("songbird.backscan.concurrent"),
                                       NS_LITERAL_STRING("") );
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIURIMetadataHelper> uriMetadataHelper = new sbURIMetadataHelper();
  NS_ENSURE_TRUE(uriMetadataHelper, NS_ERROR_OUT_OF_MEMORY);

  rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                            NS_GET_IID(sbIURIMetadataHelper),
                            uriMetadataHelper,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mURIMetadataHelper));
  NS_ENSURE_SUCCESS(rv, rv);

  // We only use one string, so read it in here.
  nsCOMPtr<nsIStringBundle> stringBundle;
  nsCOMPtr<nsIStringBundleService> StringBundleService =
    do_GetService("@mozilla.org/intl/stringbundle;1", &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StringBundleService->CreateBundle( "chrome://songbird/locale/songbird.properties",
                                          getter_AddRefs( stringBundle ) );
  NS_ENSURE_SUCCESS(rv, rv);

  nsString scanning;
  rv = stringBundle->GetStringFromName( NS_LITERAL_STRING("back_scan.scanning").get(),
                                        getter_Copies(scanning) );
  if (NS_FAILED(rv)) {
    scanning.AssignLiteral("Scanning");
  }
  scanning.AppendLiteral(" ... ");
  mStatusDisplayString = scanning;
  return NS_OK;
}

/* readonly attribute AString tableName; */
NS_IMETHODIMP sbMetadataJob::GetTableName(nsAString & aTableName)
{
  aTableName = mTableName;
  return NS_OK;
}

/* readonly attribute PRBool completed; */
NS_IMETHODIMP sbMetadataJob::GetCompleted(PRBool *aCompleted)
{
  NS_ENSURE_ARG_POINTER( aCompleted );
  (*aCompleted) = mCompleted;
  return NS_OK;
}

/* void setObserver( in nsIObserver aObserver ); */
NS_IMETHODIMP sbMetadataJob::SetObserver(nsIObserver *aObserver)
{
  NS_ENSURE_ARG_POINTER( aObserver );
  mObserver = aObserver;
  return NS_OK;
}

/* void removeObserver(); */
NS_IMETHODIMP sbMetadataJob::RemoveObserver()
{
  mObserver = nsnull;
  return NS_OK;
}

/* void init (in AString aTableName, in nsIArray aMediaItemsArray); */
NS_IMETHODIMP sbMetadataJob::Init(const nsAString & aTableName, nsIArray *aMediaItemsArray, PRUint32 aSleepMS)
{
  // aMediaItemsArray may be null.
  nsresult rv;

  mTableName = aTableName;
  mSleepMS = aSleepMS;

  //  - Initialize the task table in the database
  //      (library guid, item guid, url, worker_thread, is_scanned)

  // Get ready to do some database lifting
  rv = mMainThreadQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  // Compose the create table query string
  nsAutoString createTable;
  createTable.AppendLiteral( "CREATE TABLE IF NOT EXISTS '" );
  createTable += mTableName;
  createTable.AppendLiteral(  "' (library_guid TEXT NOT NULL, "
                              "item_guid TEXT NOT NULL, "
                              "url TEXT NOT NULL, "

                              // True if the item should run on the worker thread
                              "worker_thread INTEGER NOT NULL DEFAULT 0, "

                              // True if the item has been scanned for metadata (partly complete, must restart)
                              "is_scanned INTEGER NOT NULL DEFAULT 0, "

                              // True if the scan has been written to the database (fully complete, no restart needed)
                              "is_written INTEGER NOT NULL DEFAULT 0, "

                              // True if the item is currently being scanned (ignore on restart, item failed)
                              "is_current INTEGER NOT NULL DEFAULT 0, "

                              "PRIMARY KEY (library_guid, item_guid) )" );
  rv = mMainThreadQuery->AddQuery( createTable );
  NS_ENSURE_SUCCESS(rv, rv);

  // Compose the create index query string
  nsAutoString createIndex;
  createIndex.AppendLiteral( "CREATE INDEX IF NOT EXISTS 'idx_" );
  createIndex += mTableName;
  createIndex.AppendLiteral(  "_worker_thread_is_scanned' ON " );
  createIndex += mTableName;
  createIndex.AppendLiteral(  " (worker_thread, is_scanned)" );
  rv = mMainThreadQuery->AddQuery( createIndex );
  NS_ENSURE_SUCCESS(rv, rv);

  // Compose the (other) create index query string
  createIndex.Truncate();
  createIndex.AppendLiteral( "CREATE INDEX IF NOT EXISTS 'idx_" );
  createIndex += mTableName;
  createIndex.AppendLiteral(  "_is_written_is_current' ON " );
  createIndex += mTableName;
  createIndex.AppendLiteral(  " (is_written, is_current)" );
  rv = mMainThreadQuery->AddQuery( createIndex );
  NS_ENSURE_SUCCESS(rv, rv);

  // Create all that groovy stuff.
  PRInt32 error;
  rv = mMainThreadQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  // Update any old items that were scanned but not written
  rv = ResetUnwritten( mMainThreadQuery, mTableName );
  NS_ENSURE_SUCCESS(rv, rv);

  // Append any new items in the array to the task table
  if ( aMediaItemsArray != nsnull )
  {
    // How many Media Items?
    PRUint32 length;
    rv = aMediaItemsArray->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 0; i < length; i++) {
      // Break out the media item
      nsCOMPtr<sbIMediaItem> mediaItem = do_QueryElementAt(aMediaItemsArray, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      mInitArray.AppendObject( mediaItem );
    }
  }

  // Launch the timer to complete initialization.
  rv = mTimer->InitWithFuncCallback( MetadataJobTimer, this, TIMER_LOOP_MS, nsITimer::TYPE_REPEATING_SLACK );
  NS_ENSURE_SUCCESS(rv, rv);

  // So now it's safe to launch the thread
  mMetadataJobProcessor = new sbMetadataJobProcessorThread( this );
  NS_ENSURE_TRUE(mMetadataJobProcessor, NS_ERROR_OUT_OF_MEMORY);

  rv = NS_NewThread( getter_AddRefs(mThread), mMetadataJobProcessor );
  NS_ENSURE_SUCCESS(rv, rv);

  // Tell the world we're starting to scan something.
  IncrementDataRemote();

  // Phew!  We're done!
  return NS_OK;
}

/* void cancel (); */
NS_IMETHODIMP sbMetadataJob::Cancel()
{
  // Quit the timer.
  CancelTimer();

  if ( mMetadataJobProcessor )
  {
    // Tell the thread to go away, please.
    mMetadataJobProcessor->mShutdown = PR_TRUE;

    // Wait for the thread to shutdown and release all resources.
    mThread->Shutdown();
  }

  if ( mObserver )
    mObserver->Observe( this, "cancel", mTableName.get() );

  return NS_OK;
}

nsresult sbMetadataJob::RunTimer()
{
  nsresult rv;

  // First the timer handles the initialization, creating the work table.
  // Then it launches the thread and starts handling any main-thread items.
  if ( ! mInitCompleted )
  {
    PRUint32 total = mInitArray.Count(), max = mInitCount + NUM_ITEMS_PER_INIT_LOOP;

    // First we're adding items from the array.
    if ( mInitCount < total )
    {
      //
      // WARNING!
      //
      // There is a danger in this code that the user can shut off Songbird and lose
      // the enqueueing of the metadata item-jobs into the database.  We should figure
      // out some way to prevent shutdown, but it is NOT crashproof while in this loop.
      //

      // Add them to our database table so we don't lose context if the app goes away.
      {
        nsCOMPtr<sbILibraryManager> libraryManager =
          do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<sbILibrary> library;
        rv = libraryManager->GetMainLibrary(getter_AddRefs(library));
        NS_ENSURE_SUCCESS(rv, rv);
        nsCOMPtr<sbIMediaList> list = do_QueryInterface(library, &rv);
        NS_ENSURE_SUCCESS(rv, rv);

        // XXXsteve This assumes that we are loading into the main library,
        // but I think we make this assumption all over :(
        sbMetadataBatchHelper batchHelper(list);

        rv = mMainThreadQuery->ResetQuery();
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("begin"));
        NS_ENSURE_SUCCESS(rv, rv);
        for ( ; mInitCount < max && mInitCount < total; mInitCount++ ) 
        {
          // Place the item info on the query to track in our own database, get back a temporary job item.
          nsRefPtr<jobitem_t> tempItem;
          AddItemToJobTableQuery( mMainThreadQuery, mTableName, mInitArray[ mInitCount ], getter_AddRefs(tempItem) );

          // Use the temporary item to stuff a default title value in the properties cache (don't flush to library database, yet)
          AddDefaultMetadataToItem( tempItem, mInitArray[ mInitCount ] );
        }
        rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("commit"));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mMainThreadQuery->SetAsyncQuery( PR_FALSE );
        NS_ENSURE_SUCCESS(rv, rv);
        PRInt32 error;
        rv = mMainThreadQuery->Execute( &error );
        NS_ENSURE_SUCCESS(rv, rv);
        NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);
      }

      // Relaunch the thread if it has completed and we add new task items.
      if ( mThreadCompleted )
      {
        mThreadCompleted = PR_FALSE;
        // So now it's safe to re-launch the thread
        rv = NS_NewThread( getter_AddRefs(mThread), mMetadataJobProcessor );
        NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start thread");
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    else
    {
      // And go on to the metadata in the timer
      mInitCompleted = PR_TRUE;
    }

    // Ah, no, we're not ready to go down there, yet.
    return NS_OK;
  }

  // Loop through them.
  for ( PRUint32 i = 0; i < NUM_CONCURRENT_MAINTHREAD_ITEMS; i++ )
  {
    // If there's someone there
    if ( mTimerWorkers[ i ] != nsnull )
    {
      nsRefPtr<sbMetadataJob::jobitem_t> item = mTimerWorkers[ i ];
      // Check to see if it has completed
      PRBool completed;
      rv = item->handler->GetCompleted( &completed );
      NS_ENSURE_SUCCESS(rv, rv);
      if ( completed )
      {
        rv = ClearItemIsCurrent( mMainThreadQuery, mTableName, item );
        NS_ENSURE_SUCCESS(rv, rv);

        // Try to write it out
        rv = AddMetadataToItem( item, mURIMetadataHelper ); // Flush properties cache every time
        // NS_ENSURE_SUCCESS(rv, rv); // Allow it to fail.  We already put a default value in for it.

        rv = SetItemIsWritten( mMainThreadQuery, mTableName, item );
        NS_ENSURE_SUCCESS(rv, rv);
        // And NULL the array entry
        mTimerWorkers[ i ] = nsnull;
      }
    }

    // NULL means we need a new one
    if ( ! mTimerCompleted && mTimerWorkers[ i ] == nsnull )
    {
      // Get the next task
      nsRefPtr<sbMetadataJob::jobitem_t> item;
      rv = GetNextItem( mMainThreadQuery, mTableName, PR_FALSE, getter_AddRefs(item) );
      if (rv == NS_ERROR_NOT_AVAILABLE) {
        mTimerCompleted = PR_TRUE;
        break;
      }
      NS_ENSURE_SUCCESS(rv, rv);

#if PR_LOGGING
      // Do a little debug output
      nsString sp = NS_LITERAL_STRING(" - ");
      nsString alert;
      alert += item->library_guid;
      alert += sp;
      alert += item->item_guid;
      alert += sp;
      alert += item->url;
      alert += sp;
      alert += item->worker_thread;

      TRACE(("sbMetadataJob[0x%.8x] - MAIN THREAD - GetNextItem( %s )", this, alert.get()));
#endif

      // Get ready to launch a handler
      rv = SetItemIsCurrent( mMainThreadQuery, mTableName, item );
      NS_ENSURE_SUCCESS(rv, rv);
      rv = SetItemIsScanned( mMainThreadQuery, mTableName, item );
      NS_ENSURE_SUCCESS(rv, rv);

      // Create the metadata handler and launch it
      rv = StartHandlerForItem( item );
      // Ignore errors on the metadata loop, just set a default and keep going.
      if ( NS_FAILED(rv) )
      {
        // Give a nice warning
        TRACE(("sbMetadataJob[0x%.8x] - WORKER THREAD - "
               "Unable to add metadata for( %s )", this,
               NS_LossyConvertUTF16toASCII(item->url).get()));
        // Record this item as completed.
        rv = SetItemIsWritten( mMainThreadQuery, mTableName, item );
        NS_ENSURE_SUCCESS(rv, rv);
        // Leave mTimerWorkers null for the next loop.
      }
      else
      {
        // And stuff it in our working array to check next loop
        mTimerWorkers[ i ] = item;
      }
    }
  }

  // Check for all aynchronous tasks to have completed
  PRBool finished = PR_TRUE;
  for ( PRUint32 i = 0; i < NUM_CONCURRENT_MAINTHREAD_ITEMS; i++ )
    if ( mTimerWorkers[ i ] != nsnull )
      finished = PR_FALSE;

  // Keep going through the timer until the thread completes,
  // then cleanup the working table.
  if ( finished && mThreadCompleted )
    this->FinishJob();

  // Cleanup the timer loop.
  return NS_OK;
}

nsresult sbMetadataJob::CancelTimer()
{
  // Fill with nsnull to release metadata handlers and kill their active channels.
  for ( PRUint32 i = 0; i < NUM_CONCURRENT_MAINTHREAD_ITEMS; i++ )
  {
    mTimerWorkers[ i ] = nsnull;
  }

  // Quit the timer.

  if (mTimer) {
    nsresult rv = mTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);
    mTimer = nsnull;
  }

  return NS_OK;
}

nsresult sbMetadataJob::RunThread( PRBool * bShutdown )
{
  nsresult rv;

  nsCOMPtr<sbILibrary> library; // To be able to call Write()
  nsCOMPtr<sbIMediaList> mediaList; // To be able to call BeginUpdateBatch()
  sbMetadataBatchHelper batchHelper;

  nsString aTableName = mTableName;

  nsCOMPtr<sbIDatabaseQuery> WorkerThreadQuery;
  WorkerThreadQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  WorkerThreadQuery->SetDatabaseGUID( sbMetadataJob::DATABASE_GUID() );
  WorkerThreadQuery->SetAsyncQuery( PR_FALSE );

  nsTArray<nsRefPtr<sbMetadataJob::jobitem_t> > writePending;
  for ( int count = 0; !(*bShutdown); count++ )
  {
    PRBool flush = ! (( count + 1 ) % NUM_ITEMS_BEFORE_FLUSH); 
    // Get the next task
    nsRefPtr<sbMetadataJob::jobitem_t> item;
    rv = GetNextItem( WorkerThreadQuery, aTableName, PR_TRUE, getter_AddRefs(item));
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      break;
    }
    NS_ENSURE_SUCCESS(rv, rv);

#if PR_LOGGING
    // Do a little debug output
    nsString sp = NS_LITERAL_STRING(" - ");
    nsString alert;
    alert += item->library_guid;
    alert += sp;
    alert += item->item_guid;
    alert += sp;
    alert += item->url;
    alert += sp;
    alert += item->worker_thread;

    TRACE(("sbMetadataJob[0x%.8x] - WORKER THREAD - GetNextItem( %s )", this, alert.get()));
#endif

    if ( !library )
    {
      nsCOMPtr<sbILibraryManager> libraryManager;
      libraryManager = do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = libraryManager->GetLibrary( item->library_guid, getter_AddRefs(library) );
      NS_ENSURE_SUCCESS(rv, rv);
      mediaList = do_QueryInterface( library, &rv ); 
      NS_ENSURE_SUCCESS(rv, rv);

      batchHelper.SetList( mediaList );
    }

    // Get ready to launch a handler
    rv = SetItemIsCurrent( mMainThreadQuery, mTableName, item );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = SetItemIsScanned( WorkerThreadQuery, aTableName, item );
    NS_ENSURE_SUCCESS(rv, rv);

    // Create the metadata handler and launch it
    rv = StartHandlerForItem( item );
    if ( NS_SUCCEEDED(rv) )
    {
      // Wait at least a half second for it to complete
      PRBool completed;
      int counter = 0;
      for ( item->handler->GetCompleted( &completed ); !completed && !(*bShutdown) && counter < 25; item->handler->GetCompleted( &completed ), counter++ )
      {
        // Run at most 10 messages.
        PRBool event = PR_FALSE;
        int evtcounter = 0;
        for ( mThread->ProcessNextEvent( PR_FALSE, &event ); event && evtcounter < 10; mThread->ProcessNextEvent( PR_FALSE, &event ), evtcounter++ )
          PR_Sleep( PR_MillisecondsToInterval( 0 ) );

        PR_Sleep( PR_MillisecondsToInterval( 20 ) ); // Sleep at least 20ms
      }

      // Make an sbIMediaItem and push the metadata into it
      rv = AddMetadataToItem( item, mURIMetadataHelper );

      // Close the handler by hand since we know we're done with it and
      // we won't get rid of the item for awhile.
      if ( item->handler )
        item->handler->Close();
    }
    // Ignore errors on the metadata loop, just set a warning and keep going.
    if ( NS_FAILED(rv) )
    {
      // Give a nice warning
      TRACE(("sbMetadataJob[0x%.8x] - WORKER THREAD - "
             "Unable to add metadata for( %s )", this,
             NS_LossyConvertUTF16toASCII(item->url).get()));

      // Don't leave the row completely blank.  Please.
      AddDefaultMetadataToItem( item, item->item );
    }

    // Ah, if we got here we must not have crashed.
    rv = ClearItemIsCurrent( mMainThreadQuery, mTableName, item );
    NS_ENSURE_SUCCESS(rv, rv);

    writePending.AppendElement( item );

    if ( flush )
    {
      rv = SetItemsAreWritten( WorkerThreadQuery, aTableName, writePending );
      NS_ENSURE_SUCCESS(rv, rv);

      batchHelper.Flush();
    }
    PR_Sleep( PR_MillisecondsToInterval( mSleepMS ) );
  }

  if ( library )
  {
    // Flush the written bits.
    rv = SetItemsAreWritten( WorkerThreadQuery, aTableName, writePending );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Thread ends when the function returns.
  return NS_OK;
}

nsresult sbMetadataJob::FinishJob()
{
  nsresult rv;
  this->CancelTimer();
  mCompleted = PR_TRUE;

  if ( mObserver )
    mObserver->Observe( this, "complete", mTableName.get() );

  // Decrement the atomic counter
  DecrementDataRemote();

  // Drop the working table
  nsAutoString dropTable;
  dropTable.AppendLiteral("DROP TABLE ");
  dropTable += mTableName;
  rv = mMainThreadQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->AddQuery( dropTable );
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error;
  rv = mMainThreadQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  // Remove the entry from the tracking table
  nsAutoString delTracking;
  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  nsCOMPtr<sbISQLDeleteBuilder> del =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = del->SetTableName( sbMetadataJob::DATABASE_GUID() );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = del->CreateMatchCriterionString( sbMetadataJob::DATABASE_GUID(),
                                            NS_LITERAL_STRING("job_guid"),
                                            sbISQLUpdateBuilder::MATCH_EQUALS,
                                            mTableName,
                                            getter_AddRefs(criterion) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = del->AddCriterion( criterion );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = del->ToString( delTracking );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainThreadQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->AddQuery( delTracking );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainThreadQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult sbMetadataJob::AddItemToJobTableQuery( sbIDatabaseQuery *aQuery, nsString aTableName, sbIMediaItem *aMediaItem, jobitem_t **_retval )
{
  NS_ENSURE_ARG_POINTER( aMediaItem );
  NS_ENSURE_ARG_POINTER( aQuery );

  nsresult rv;

  // The list of strings that describes an item in the table
  nsString library_guid, item_guid, url, worker_thread;

  // Get its library_guid
  nsCOMPtr<sbILibrary> pLibrary;
  rv = aMediaItem->GetLibrary( getter_AddRefs(pLibrary) );
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbILibraryResource> pLibraryResource = do_QueryInterface(pLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = pLibraryResource->GetGuid( library_guid );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get its item_guid
  rv = aMediaItem->GetGuid( item_guid );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get its url
  nsCAutoString cstr;
  nsCOMPtr<nsIURI> pURI;
  rv = aMediaItem->GetContentSrc( getter_AddRefs(pURI) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = pURI->GetSpec(cstr);
  NS_ENSURE_SUCCESS(rv, rv);
  url = NS_ConvertUTF8toUTF16(cstr);

  // Check to see if it should go in the worker thread
  nsCAutoString cscheme;
  rv = pURI->GetScheme(cscheme);

  // Compose the query string
  nsAutoString insertItem;
  nsCOMPtr<sbISQLInsertBuilder> insert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->SetIntoTableName( aTableName );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddColumn( NS_LITERAL_STRING("library_guid") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddValueString( library_guid );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddColumn( NS_LITERAL_STRING("item_guid") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddValueString( item_guid );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddColumn( NS_LITERAL_STRING("url") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddValueString( url );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddColumn( NS_LITERAL_STRING("worker_thread") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddValueLong( ( cscheme == "file" ) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddColumn( NS_LITERAL_STRING("is_scanned") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddValueLong( 0 );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->ToString( insertItem );
  NS_ENSURE_SUCCESS(rv, rv);

  if ( _retval ) {
    nsRefPtr<jobitem_t> item;
    item = new jobitem_t(library_guid,
                         item_guid,
                         url,
                         worker_thread,
                         NS_LITERAL_STRING("0"),
                         nsnull);
    NS_ENSURE_TRUE(item, NS_ERROR_OUT_OF_MEMORY);
    NS_ADDREF(*_retval = item);
  }

  // Add it to the query object
  return aQuery->AddQuery( insertItem );
}

nsresult sbMetadataJob::GetNextItem( sbIDatabaseQuery *aQuery, nsString aTableName, PRBool isWorkerThread, sbMetadataJob::jobitem_t **_retval )
{
  NS_ENSURE_ARG_POINTER( aQuery );
  NS_ENSURE_ARG_POINTER( _retval );

  nsresult rv;

  nsCOMPtr<sbILibraryManager> libraryManager =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // We break out of this loop either when we find an item or when there are
  // no items left
  while (PR_TRUE) {

    // Compose the query string.
    nsAutoString getNextItem;
    nsCOMPtr<sbISQLBuilderCriterion> criterionWT;
    nsCOMPtr<sbISQLBuilderCriterion> criterionIS;
    nsCOMPtr<sbISQLBuilderCriterion> criterionAND;
    nsCOMPtr<sbISQLSelectBuilder> select =
      do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = select->SetBaseTableName( aTableName );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = select->AddColumn( aTableName, NS_LITERAL_STRING("*") );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = select->CreateMatchCriterionLong( aTableName,
                                           NS_LITERAL_STRING("worker_thread"),
                                           sbISQLSelectBuilder::MATCH_EQUALS,
                                           ( isWorkerThread ) ? 1 : 0,
                                           getter_AddRefs(criterionWT) );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = select->CreateMatchCriterionLong( aTableName,
                                           NS_LITERAL_STRING("is_scanned"),
                                           sbISQLSelectBuilder::MATCH_EQUALS,
                                           0,
                                           getter_AddRefs(criterionIS) );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = select->CreateAndCriterion( criterionWT,
                                     criterionIS,
                                     getter_AddRefs(criterionAND) );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = select->AddCriterion( criterionAND );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = select->SetLimit( 1 );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = select->ToString( getNextItem );
    NS_ENSURE_SUCCESS(rv, rv);

    // Make it go.
    rv = aQuery->SetAsyncQuery( PR_FALSE );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aQuery->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aQuery->AddQuery( getNextItem );
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt32 error;
    rv = aQuery->Execute( &error );
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

    // And return what it got.
    nsCOMPtr< sbIDatabaseResult > pResult;
    rv = aQuery->GetResultObject( getter_AddRefs(pResult) );
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 rowCount;
    rv = pResult->GetRowCount(&rowCount);
    NS_ENSURE_SUCCESS(rv, rv);

    if (rowCount == 0) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    nsAutoString libraryGuid;
    rv = pResult->GetRowCellByColumn(0, NS_LITERAL_STRING( "library_guid" ), libraryGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString itemGuid;
    rv = pResult->GetRowCellByColumn(0, NS_LITERAL_STRING( "item_guid" ), itemGuid);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString workerThread;
    rv = pResult->GetRowCellByColumn(0, NS_LITERAL_STRING( "worker_thread" ), workerThread);
    NS_ENSURE_SUCCESS(rv, rv);

    nsAutoString isScanned;
    rv = pResult->GetRowCellByColumn(0, NS_LITERAL_STRING( "is_scanned" ), isScanned);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> library;
    rv = libraryManager->GetLibrary( libraryGuid, getter_AddRefs(library) );
    if (NS_SUCCEEDED(rv)) {
#ifdef PROXY_LIBRARY
      nsCOMPtr<sbILibrary> proxiedLibrary;
      rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                NS_GET_IID(sbILibrary),
                                library,
                                NS_PROXY_SYNC,
                                getter_AddRefs(proxiedLibrary));
      NS_ENSURE_SUCCESS(rv, rv);
      library = proxiedLibrary;
#endif
      nsCOMPtr<sbIMediaItem> item;
      rv = library->GetMediaItem( itemGuid, getter_AddRefs(item) );
      if (NS_SUCCEEDED(rv)) {

#ifdef PROXY_LIBRARY
        nsCOMPtr<sbIMediaItem> proxiedItem;
        rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                                  NS_GET_IID(sbIMediaItem),
                                  item,
                                  NS_PROXY_SYNC,
                                  getter_AddRefs(proxiedItem));
        NS_ENSURE_SUCCESS(rv, rv);
        item = proxiedItem;
#endif

#ifdef PR_LOGGING
        nsAutoString itemStr;
        item->ToString(itemStr);
        TRACE(("sbMetadataJob - GetNextItem - Got '%s'",
               NS_LossyConvertUTF16toASCII(itemStr).get()));
#endif

        nsAutoString uriSpec;
        rv = item->GetProperty( NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                uriSpec );
        NS_ENSURE_SUCCESS(rv, rv);

        nsRefPtr<jobitem_t> jobitem;
        jobitem = new jobitem_t( libraryGuid,
                                 itemGuid,
                                 uriSpec,
                                 workerThread,
                                 isScanned,
                                 item,
                                 nsnull );
        NS_ENSURE_TRUE(jobitem, NS_ERROR_OUT_OF_MEMORY);
        NS_ADDREF(*_retval = jobitem);
        return NS_OK;
      }
    }

    // If we get here, we were unable to find the media item associated with
    // the metadata job.  Just mark the item as scanned and written and
    // continue

    TRACE(("sbMetadataJob - GetNextItem - Item with guid '%s' has gone away",
           NS_LossyConvertUTF16toASCII(itemGuid).get()));

    nsRefPtr<jobitem_t> tempItem(new jobitem_t( libraryGuid,
                                                itemGuid,
                                                EmptyString(),
                                                workerThread,
                                                isScanned,
                                                nsnull,
                                                nsnull ));
    NS_ENSURE_TRUE(tempItem, NS_ERROR_OUT_OF_MEMORY); 
    rv = SetItemIsScanned( aQuery, aTableName, tempItem );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = SetItemIsWritten( aQuery, aTableName, tempItem );
    NS_ENSURE_SUCCESS(rv, rv);
 }

}

nsresult sbMetadataJob::SetItemIsScanned( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem )
{
  return SetItemIs( NS_LITERAL_STRING("is_scanned"), aQuery, aTableName, aItem, PR_TRUE );
}


nsresult sbMetadataJob::SetItemIsWritten( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem, PRBool aExecute )
{
  nsresult rv = SetItemIs( NS_LITERAL_STRING("is_written"), aQuery, aTableName, aItem, aExecute );
  if ( aItem->handler )
    aItem->handler->Close();  // You are so done.
  return rv;
}

nsresult sbMetadataJob::SetItemsAreWritten( sbIDatabaseQuery *aQuery, nsString aTableName, nsTArray<nsRefPtr<sbMetadataJob::jobitem_t> > &aItemArray )
{
  nsresult rv;
  // Commit all the written bits as a single transaction.
  rv = aQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->AddQuery( NS_LITERAL_STRING("begin") );
  NS_ENSURE_SUCCESS(rv, rv);
  for ( PRUint32 i = 0, end = aItemArray.Length(); i < end; i++ )
  {
    SetItemIsWritten( aQuery, aTableName, aItemArray[ i ], PR_FALSE );
  }
  rv = aQuery->AddQuery( NS_LITERAL_STRING("commit") );
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  aItemArray.Clear();
  return NS_OK;
}

nsresult sbMetadataJob::SetItemIs( const nsAString &aColumnString, sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem, PRBool aExecute, PRBool aValue  )
{
  NS_ENSURE_ARG_POINTER( aQuery );
  NS_ENSURE_ARG_POINTER( aItem );

  nsresult rv;

    // Compose the query string.
  nsAutoString itemComplete;
  nsCOMPtr<sbISQLBuilderCriterion> criterionWT;
  nsCOMPtr<sbISQLBuilderCriterion> criterionIS;
  nsCOMPtr<sbISQLBuilderCriterion> criterionAND;
  nsCOMPtr<sbISQLUpdateBuilder> update =
    do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->SetTableName( aTableName );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->AddAssignmentString( aColumnString, aValue ? NS_LITERAL_STRING("1") : NS_LITERAL_STRING("0") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->CreateMatchCriterionString( aTableName,
                                            NS_LITERAL_STRING("library_guid"),
                                            sbISQLUpdateBuilder::MATCH_EQUALS,
                                            aItem->library_guid,
                                            getter_AddRefs(criterionWT) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->CreateMatchCriterionString( aTableName,
                                            NS_LITERAL_STRING("item_guid"),
                                            sbISQLUpdateBuilder::MATCH_EQUALS,
                                            aItem->item_guid,
                                            getter_AddRefs(criterionIS) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->CreateAndCriterion( criterionWT,
                                    criterionIS,
                                    getter_AddRefs(criterionAND) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->AddCriterion( criterionAND );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->SetLimit( 1 );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->ToString( itemComplete );
  NS_ENSURE_SUCCESS(rv, rv);

  // Make it go.
  if ( aExecute )
  {
    rv = aQuery->SetAsyncQuery( PR_FALSE );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aQuery->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = aQuery->AddQuery( itemComplete );
  NS_ENSURE_SUCCESS(rv, rv);
  if ( aExecute )
  {
    PRInt32 error;
    rv = aQuery->Execute( &error );
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);
  }
  return NS_OK;
}

nsresult sbMetadataJob::SetItemIsCurrent( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem )
{
  return SetItemIs( NS_LITERAL_STRING("is_current"), aQuery, aTableName, aItem, PR_TRUE );
}

nsresult sbMetadataJob::ClearItemIsCurrent( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem )
{
  return SetItemIs( NS_LITERAL_STRING("is_current"), aQuery, aTableName, aItem, PR_TRUE, PR_FALSE );
}

nsresult sbMetadataJob::ResetUnwritten( sbIDatabaseQuery *aQuery, nsString aTableName )
{
  NS_ENSURE_ARG_POINTER( aQuery );

  nsresult rv;

  // Compose the query string.
  nsAutoString updateUnwritten;
  nsCOMPtr<sbISQLBuilderCriterion> criterionW;
  nsCOMPtr<sbISQLBuilderCriterion> criterionC;
  nsCOMPtr<sbISQLBuilderCriterion> criterionAND;
  nsCOMPtr<sbISQLUpdateBuilder> update =
    do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->SetTableName( aTableName );
  NS_ENSURE_SUCCESS(rv, rv);
  // Set "is_scanned" back to 0
  rv = update->AddAssignmentString( NS_LITERAL_STRING("is_scanned"), NS_LITERAL_STRING("0") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->CreateMatchCriterionString( aTableName,
                                            // If we hadn't written it out yet
                                            NS_LITERAL_STRING("is_written"), 
                                            sbISQLUpdateBuilder::MATCH_EQUALS,
                                            NS_LITERAL_STRING("0"),
                                            getter_AddRefs(criterionW) );
  rv = update->CreateMatchCriterionString( aTableName,
                                            // And it wasn't something currently running when we crashed
                                            NS_LITERAL_STRING("is_current"), 
                                            sbISQLUpdateBuilder::MATCH_EQUALS,
                                            NS_LITERAL_STRING("0"),
                                            getter_AddRefs(criterionC) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->CreateAndCriterion(  criterionW,
                                    criterionC,
                                    getter_AddRefs(criterionAND) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->AddCriterion( criterionAND );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->ToString( updateUnwritten );
  NS_ENSURE_SUCCESS(rv, rv);

  // Make it go.
  rv = aQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->AddQuery( updateUnwritten );
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);
  return NS_OK;
}

nsresult sbMetadataJob::StartHandlerForItem( sbMetadataJob::jobitem_t *aItem )
{
  NS_ENSURE_ARG_POINTER( aItem );
  nsresult rv = NS_ERROR_FAILURE;
  // No item is a failed metadata attempt.  
  // Don't bother to chew the CPU or the filesystem by reading metadata.
  if ( aItem->item && !aItem->url.IsEmpty() )
  {
    nsCOMPtr<sbIMetadataManager> MetadataManager = do_GetService("@songbirdnest.com/Songbird/MetadataManager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = MetadataManager->GetHandlerForMediaURL( aItem->url, getter_AddRefs(aItem->handler) );
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool async = PR_FALSE;
    if ( aItem->handler ) // Shouldn't need this?
      rv = aItem->handler->Read( &async );
  }
  return rv;
}

nsresult sbMetadataJob::AddMetadataToItem( sbMetadataJob::jobitem_t *aItem,
                                           sbIURIMetadataHelper *aURIMetadataHelper )
{
  NS_ENSURE_ARG_POINTER( aItem );
  NS_ENSURE_ARG_POINTER( aURIMetadataHelper );

  nsresult rv;

// TODO:
  // - Update the metadata handlers to use the new keystrings

  // Get the sbIMediaItem we're supposed to be updating
  nsCOMPtr<sbIMediaItem> item = aItem->item;

  // Get the metadata values that were found
  nsCOMPtr<sbIMetadataValues> values;
  rv = aItem->handler->GetValues( getter_AddRefs(values) );
  NS_ENSURE_SUCCESS(rv, rv);

/*
  // These are the old API metadata key strings
  const keys = new Array("title", "length", "album", "artist", "genre", "year", "composer", "track_no",
                         "track_total", "disc_no", "disc_total", "service_uuid");
*/
  nsCOMPtr<sbIPropertyManager> propMan =
    do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMutablePropertyArray> properties =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the properties (eventually iterate when the sbIMetadataValue have the correct keystrings).
  NS_NAMED_LITERAL_STRING( trackNameKey, SB_PROPERTY_TRACKNAME );
  nsAutoString trackName;
  rv = values->GetValue( NS_LITERAL_STRING("title"), trackName );
  NS_ENSURE_SUCCESS(rv, rv);

  // If the metadata read can't even find a song name, cook one up off the url.
  if ( trackName.IsEmpty() ) {
    rv = CreateDefaultItemName( aItem->url, trackName );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = AppendIfValid( propMan, properties, trackNameKey, trackName);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( artistKey, SB_PROPERTY_ARTISTNAME );
  nsAutoString artist;
  rv = values->GetValue( NS_LITERAL_STRING("artist"), artist );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AppendIfValid( propMan, properties, artistKey, artist );
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( albumKey, SB_PROPERTY_ALBUMNAME );
  nsAutoString album;
  rv = values->GetValue( NS_LITERAL_STRING("album"), album );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AppendIfValid( propMan, properties, albumKey, album );
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( durationKey, SB_PROPERTY_DURATION );
  nsAutoString duration;
  rv = values->GetValue( NS_LITERAL_STRING("length"), duration );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AppendIfValid( propMan, properties, durationKey, duration );
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( genreKey, SB_PROPERTY_GENRE );
  nsAutoString genre;
  rv = values->GetValue( NS_LITERAL_STRING("genre"), genre );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AppendIfValid( propMan, properties, genreKey, genre );
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( yearKey, SB_PROPERTY_YEAR );
  nsAutoString year;
  rv = values->GetValue( NS_LITERAL_STRING("year"), year );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AppendIfValid( propMan, properties, yearKey, year );
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( trackKey, SB_PROPERTY_TRACKNUMBER );
  nsAutoString track;
  rv = values->GetValue( NS_LITERAL_STRING("track_no"), track );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AppendIfValid( propMan, properties, trackKey, track );
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( discNumberKey, SB_PROPERTY_DISCNUMBER );
  nsAutoString discNumber;
  rv = values->GetValue( NS_LITERAL_STRING("disc_no"), discNumber );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AppendIfValid( propMan, properties, discNumberKey, discNumber );
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( totalTracksKey, SB_PROPERTY_TOTALTRACKS );
  nsAutoString totalTracks;
  rv = values->GetValue( NS_LITERAL_STRING("track_total"), totalTracks );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AppendIfValid( propMan, properties, totalTracksKey, totalTracks );
  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( totalDiscsKey, SB_PROPERTY_TOTALDISCS );
  nsAutoString totalDiscs;
  rv = values->GetValue( NS_LITERAL_STRING("disc_total"), totalDiscs );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AppendIfValid( propMan, properties, totalDiscsKey, totalDiscs );
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt64 fileSize;
  rv = aURIMetadataHelper->GetFileSize(aItem->url, &fileSize);
  if (NS_SUCCEEDED(rv)) {
    nsAutoString contentLength;
    contentLength.AppendInt(fileSize);
    rv = AppendIfValid( propMan,
                        properties,
                        NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                        contentLength );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = item->SetProperties(properties);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult sbMetadataJob::AddDefaultMetadataToItem( sbMetadataJob::jobitem_t *aItem, sbIMediaItem *aMediaItem )
{
  NS_ASSERTION(aItem, "aItem is null");
  NS_ASSERTION(aMediaItem, "aMediaItem is null");

  // Set the default properties 
  // (eventually iterate when the sbIMetadataValue have the correct keystrings).

  // Right now, all we default is the track name.
  NS_NAMED_LITERAL_STRING( trackNameKey, SB_PROPERTY_TRACKNAME );

  nsAutoString oldName;
  nsresult rv = aMediaItem->GetProperty( trackNameKey, oldName );
  if ( NS_FAILED(rv) || oldName.IsEmpty() )  {
    nsAutoString trackName;
    rv = CreateDefaultItemName( aItem->url, trackName );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = aMediaItem->SetProperty( trackNameKey, trackName );
//    NS_ENSURE_SUCCESS(rv, rv); // Ben asked me to remind him that these are here and commented to remind him that we don't care if this call fails.
  }

  return NS_OK;
}

nsresult sbMetadataJob::CreateDefaultItemName(const nsAString &aURLString,
                                              nsAString &retval)
{
  nsresult rv;

  retval = aURLString;

  // First, unescape the url.
  nsCOMPtr<nsINetUtil> netUtil =
    do_GetService("@mozilla.org/network/util;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Blank.  On error, the following function does utf8 -> utf16 automatically.
  nsCAutoString uriString = NS_ConvertUTF16toUTF8(aURLString);
  nsCAutoString result;
  rv = netUtil->UnescapeString(uriString, nsINetUtil::ESCAPE_ALL, result);
  NS_ENSURE_SUCCESS(rv, rv);

  // Then pull out just the text after the last slash
  PRInt32 slash = result.RFindChar( *(NS_LITERAL_STRING("/").get()) );
  if ( slash != -1 )
    result.Cut( 0, slash + 1 ); 
  slash = result.RFindChar( *(NS_LITERAL_STRING("\\").get()) );
  if ( slash != -1 )
    result.Cut( 0, slash + 1 ); 

  retval = NS_ConvertUTF8toUTF16(result);
  return NS_OK;
}

nsresult
sbMetadataJob::AppendIfValid(sbIPropertyManager* aPropertyManager,
                             sbIMutablePropertyArray* aProperties,
                             const nsAString& aName,
                             const nsAString& aValue)
{
  NS_ASSERTION(aPropertyManager, "aPropertyManager is null");
  NS_ASSERTION(aProperties, "aProperties is null");

  nsCOMPtr<sbIPropertyInfo> info;
  nsresult rv = aPropertyManager->GetPropertyInfo(aName, getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isValid;
  rv = info->Validate(aValue, &isValid);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isValid) {
    rv = aProperties->AppendProperty(aName, aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void sbMetadataJob::IncrementDataRemote()
{
  PRInt64 current;
  mDataCurrentMetadataJobs->GetIntValue( &current );
  mDataStatusDisplay->SetStringValue( mStatusDisplayString );
  // Set to the incremented value
  mDataCurrentMetadataJobs->SetIntValue( ++current );
}

void sbMetadataJob::DecrementDataRemote()
{
  PRInt64 current;
  mDataCurrentMetadataJobs->GetIntValue( &current );
  // Set to the decremented value
  mDataCurrentMetadataJobs->SetIntValue( --current );
}


