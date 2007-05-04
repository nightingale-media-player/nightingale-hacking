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
#include <nsThreadUtils.h>
#include <nsMemory.h>

#include <nsIURI.h>
#include <nsITextToSubURI.h>
#include <pref/nsIPrefService.h>
#include <pref/nsIPrefBranch2.h>

#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbILibraryResource.h>
#include <sbIMediaItem.h>
#include <sbIMediaList.h>
#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>

#include "MetadataJob.h"

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
const PRUint32 NUM_CONCURRENT_MAINTHREAD_ITEMS = 10;
const PRUint32 NUM_ITEMS_BEFORE_FLUSH = 50;
const PRUint32 NUM_ITEMS_PER_INIT_LOOP = 100;

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataJob, sbIMetadataJob);
NS_IMPL_THREADSAFE_ISUPPORTS1(MetadataJobProcessorThread, nsIRunnable)

sbMetadataJob::sbMetadataJob() :
  mInitCount( 0 ),
  mCompleted( PR_FALSE ),
  mInitCompleted( PR_FALSE ),
  mInitExecuted( PR_FALSE ),
  mTimerCompleted( PR_FALSE ),
  mThreadCompleted( PR_FALSE ),
  mMetatataJobProcessor( nsnull )
{
  // Fill with nsnull to signal the timer code that all slots are available.
  for ( PRUint32 i = mTimerWorkers.Length(); i < NUM_CONCURRENT_MAINTHREAD_ITEMS; i++ )
  {
    sbMetadataJob::jobitem_t *item = nsnull;
    mTimerWorkers.AppendElement( item );
  }

  mMainThreadQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ASSERTION(mMainThreadQuery, "Unable to create sbMetadataJob::mMainThreadQuery");
  mMainThreadQuery->SetDatabaseGUID( sbMetadataJob::DATABASE_GUID() );
  mMainThreadQuery->SetAsyncQuery( PR_FALSE );

  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  NS_ASSERTION(mTimer, "Unable to create sbMetadataJob::mTimer");

  mDataStatusDisplay = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1" );
  NS_ASSERTION(mDataStatusDisplay, "Unable to create sbMetadataJob::mDataStatusDisplay");
  mDataStatusDisplay->Init( NS_LITERAL_STRING("songbird.backscan.status"), NS_LITERAL_STRING("") );

  mDataCurrentMetadataJobs = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1" );
  NS_ASSERTION(mDataCurrentMetadataJobs, "Unable to create sbMetadataJob::mDataCurrentMetadataJobs");
  mDataCurrentMetadataJobs->Init( NS_LITERAL_STRING("songbird.backscan.concurrent"), NS_LITERAL_STRING("") );

  // We only use one string, so read it in here.
  nsresult rv;
  nsCOMPtr< nsIStringBundle >     stringBundle;
  nsCOMPtr< nsIStringBundleService > StringBundleService = do_GetService("@mozilla.org/intl/stringbundle;1", &rv );
  if ( NS_SUCCEEDED(rv) )
    rv = StringBundleService->CreateBundle( "chrome://songbird/locale/songbird.properties", getter_AddRefs( stringBundle ) );
  PRUnichar *value = nsnull;
  stringBundle->GetStringFromName( NS_LITERAL_STRING("back_scan.scanning").get(), &value );
  mStatusDisplayString = value;
  mStatusDisplayString.AppendLiteral(" ... ");
  nsMemory::Free(value);
}

sbMetadataJob::~sbMetadataJob()
{
  Cancel();
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
  createTable.AppendLiteral( "CREATE TABLE '" );
  createTable += mTableName;
  createTable.AppendLiteral(  "' (library_guid TEXT NOT NULL, "
                              "item_guid TEXT NOT NULL, "
                              "url TEXT NOT NULL, "
                              "worker_thread INTEGER NOT NULL DEFAULT 0, "
                              "is_scanned INTEGER NOT NULL DEFAULT 0, "
                              "is_written INTEGER NOT NULL DEFAULT 0, "
                              "PRIMARY KEY (library_guid, item_guid) )" );
  rv = mMainThreadQuery->AddQuery( createTable );
  NS_ENSURE_SUCCESS(rv, rv);

  // Compose the create index query string
  nsAutoString createIndex;
  createIndex.AppendLiteral( "CREATE INDEX 'idx_" );
  createIndex += mTableName;
  createIndex.AppendLiteral(  "_worker_thread_is_scanned' ON " );
  createIndex += mTableName;
  createIndex.AppendLiteral(  " (worker_thread, is_scanned)" );
  rv = mMainThreadQuery->AddQuery( createIndex );
  NS_ENSURE_SUCCESS(rv, rv);

  // Compose the (other) create index query string
  createIndex.AppendLiteral( "CREATE INDEX 'idx_" );
  createIndex += mTableName;
  createIndex.AppendLiteral(  "_is_written' ON " );
  createIndex += mTableName;
  createIndex.AppendLiteral(  " (is_written)" );
  rv = mMainThreadQuery->AddQuery( createIndex );
  NS_ENSURE_SUCCESS(rv, rv);

  // Create all that groovy stuff.
  PRBool error;
  rv = mMainThreadQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);

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

      sbIMediaItem *item = mediaItem;
      // Regenerate it?  Otherwise we crash?
      nsAutoString item_guid;
      rv = mediaItem->GetGuid( item_guid );
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<sbILibrary> pLibrary;
      rv = mediaItem->GetLibrary( getter_AddRefs(pLibrary) );
      NS_ENSURE_SUCCESS(rv, rv);
      rv = pLibrary->GetMediaItem( item_guid, &item );
      NS_ENSURE_SUCCESS(rv, rv);
      // And shove it into an internal array to process asynchronously.
      // Because using the sbMediaItem API to get my data is really slow.
      mInitArray.AppendObject( item );
    }
  }

  // Launch the timer to complete initialization.
  rv = mTimer->InitWithFuncCallback( MetadataJobTimer, this, 100, nsITimer::TYPE_REPEATING_SLACK );
  NS_ENSURE_SUCCESS(rv, rv);

  // So now it's safe to launch the thread
  mMetatataJobProcessor = new MetadataJobProcessorThread( this );
  NS_ASSERTION(mMetatataJobProcessor, "Unable to create MetatataJobProcessorRunner");
  if ( mMetatataJobProcessor )
  {
    rv = NS_NewThread( getter_AddRefs(mThread), mMetatataJobProcessor );
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start thread");
  }
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

  if ( mMetatataJobProcessor )
  {
    // Tell the thread to go away, please.
    mMetatataJobProcessor->mShutdown = PR_TRUE;

    // Wait awhile for it.
    for ( int count = 0; !mThreadCompleted; count++ )
    {
      PR_Sleep( PR_MillisecondsToInterval( 20 ) );

      // Break after a second.  Sucks to be you.
      if ( count == 50 )
      {
        break;
      }
    }
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
      rv = mMainThreadQuery->ResetQuery();
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("begin"));
      for (; mInitCount < max && mInitCount < total; ) 
      {
        // This places the item info on the query.
        rv = this->AddItemToJobTableQuery( mMainThreadQuery, mTableName, mInitArray[ mInitCount++ ] );
//        NS_ENSURE_SUCCESS(rv, rv); // Make this more resilient to people being pissy underneath us.
      }
      rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("commit"));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mMainThreadQuery->SetAsyncQuery( PR_FALSE );
      NS_ENSURE_SUCCESS(rv, rv);
      PRBool error;
      rv = mMainThreadQuery->Execute( &error );
      NS_ENSURE_SUCCESS(rv, rv);

      // Relaunch the thread if it has completed and we add new task items.
      if ( mThreadCompleted )
      {
        mThreadCompleted = PR_FALSE;
        // So now it's safe to re-launch the thread
        rv = NS_NewThread( getter_AddRefs(mThread), mMetatataJobProcessor );
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
      sbMetadataJob::jobitem_t * item = mTimerWorkers[ i ];
      // Check to see if it has completed
      PRBool completed;
      rv = item->handler->GetCompleted( &completed );
      NS_ENSURE_SUCCESS(rv, rv);
      if ( completed )
      {
        // Try to write it out
        rv = AddMetadataToItem( item, PR_TRUE ); // Flush properties cache every time
        if ( ! NS_SUCCEEDED(rv) )
        {
          // If it fails, try to just add a default
          AddDefaultMetadataToItem( item, PR_TRUE );
        }
        rv = SetItemIsWrittenAndDelete( mMainThreadQuery, mTableName, item );
        NS_ENSURE_SUCCESS(rv, rv);
        // And NULL the array entry
        mTimerWorkers[ i ] = nsnull;
      }
    }

    // NULL means we need a new one
    if ( ! mTimerCompleted && mTimerWorkers[ i ] == nsnull )
    {
      // Get the next task
      sbMetadataJob::jobitem_t * item;
      rv = GetNextItem( mMainThreadQuery, mTableName, PR_FALSE, &item );
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

      // We're out of items.  Leave this worker NULL, when all are NULL we quit.
      if ( item->library_guid.EqualsLiteral("") )
      {
        mTimerCompleted = PR_TRUE;
        break;
      }

      // Create the metadata handler and launch it
      rv = StartHandlerForItem( item );
      // Ignore errors on the metadata loop, just set a default and keep going.
      if ( ! NS_SUCCEEDED(rv) )
      {
        // If it fails, try to just add a default
        AddDefaultMetadataToItem( item, PR_TRUE );
        // Give a nice warning
        TRACE(("sbMetadataJob[0x%.8x] - WORKER THREAD - Unable to add metadata for( %s )", this, item->url));
        // Record this item as completed.
        rv = SetItemIsScanned( mMainThreadQuery, mTableName, item );
        NS_ENSURE_SUCCESS(rv, rv);
        rv = SetItemIsWrittenAndDelete( mMainThreadQuery, mTableName, item );
        NS_ENSURE_SUCCESS(rv, rv);
        // Leave mTimerWorkers null for the next loop.
      }
      else
      {
        // If it's good, record that we've started it
        rv = SetItemIsScanned( mMainThreadQuery, mTableName, item );
        NS_ENSURE_SUCCESS(rv, rv);

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

  nsAutoString txt;
  txt.AppendInt( mThreadCompleted );
  // Quit the timer.
  return mTimer->Cancel();
}


class sbMetadataBatchHelper
{
public:
  sbMetadataBatchHelper(sbIMediaList* aList = nsnull)
  {
    SetList( aList );
  }

  ~sbMetadataBatchHelper()
  {
    if ( mList )
      mList->EndUpdateBatch();
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

  nsTArray< sbMetadataJob::jobitem_t * > writePending;
  for ( int count = 0; !(*bShutdown); count++ )
  {
    PRBool flush = ! (( count + 1 ) % NUM_ITEMS_BEFORE_FLUSH); 
    // Get the next task
    sbMetadataJob::jobitem_t * item;
    rv = GetNextItem( WorkerThreadQuery, aTableName, PR_TRUE, &item );
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

    // We're out of items.  Complete the thread.
    if ( item->library_guid.EqualsLiteral("") )
    {
      break;
    }

    if ( !library )
    {
      nsCOMPtr<sbILibraryManager> libraryManager;
      libraryManager = do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      nsCOMPtr<sbILibrary> library;
      rv = libraryManager->GetLibrary( item->library_guid, getter_AddRefs(library) );
      NS_ENSURE_SUCCESS(rv, rv);
      mediaList = do_QueryInterface( library, &rv ); 
      NS_ENSURE_SUCCESS(rv, rv);

      batchHelper.SetList( mediaList );
    }


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
      rv = AddMetadataToItem( item, flush );
    }
    // Ignore errors on the metadata loop, just set a default and keep going.
    if ( ! NS_SUCCEEDED(rv) )
    {
      // Try to just add a default
      AddDefaultMetadataToItem( item, flush );
      // Give a nice warning
      TRACE(("sbMetadataJob[0x%.8x] - WORKER THREAD - Unable to add metadata for( %s )", this, item->url));
    }

    rv = SetItemIsScanned( WorkerThreadQuery, aTableName, item );
    NS_ENSURE_SUCCESS(rv, rv);

    writePending.AppendElement( item );

    if ( flush )
    {
      rv = SetItemsAreWrittenAndDelete( WorkerThreadQuery, aTableName, writePending );
      NS_ENSURE_SUCCESS(rv, rv);

      batchHelper.Flush();
    }
    PR_Sleep( PR_MillisecondsToInterval( mSleepMS ) );
  }

  if ( library )
  {
    // Flush properties cache on completion.
    rv = library->Write();
    NS_ENSURE_SUCCESS(rv, rv);
    // Along with the written bits.
    rv = SetItemsAreWrittenAndDelete( WorkerThreadQuery, aTableName, writePending );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mediaList->EndUpdateBatch();
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
  PRBool error;
  rv = mMainThreadQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);

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
  rv = mMainThreadQuery->AddQuery( dropTable );
  NS_ENSURE_SUCCESS(rv, rv);
  return mMainThreadQuery->Execute( &error );
}

nsresult sbMetadataJob::AddItemToJobTableQuery( sbIDatabaseQuery *aQuery, nsString aTableName, sbIMediaItem *aMediaItem )
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

  // Add it to the query object
  return aQuery->AddQuery( insertItem );
}

nsresult sbMetadataJob::GetNextItem( sbIDatabaseQuery *aQuery, nsString aTableName, PRBool isWorkerThread, sbMetadataJob::jobitem_t **_retval )
{
  NS_ENSURE_ARG_POINTER( aQuery );
  NS_ENSURE_ARG_POINTER( _retval );

  nsresult rv;

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
  PRBool error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);

  // And return what it got.
  nsCOMPtr< sbIDatabaseResult > pResult;
  rv = aQuery->GetResultObject( getter_AddRefs(pResult) );
  NS_ENSURE_SUCCESS(rv, rv);
  
  *_retval = new jobitem_t(
                  LG( pResult ),
                  IG( pResult ),
                  URL( pResult ),
                  WT( pResult ),
                  IS( pResult ),
                  nsnull
                );

  NS_ENSURE_TRUE(*_retval, NS_ERROR_OUT_OF_MEMORY); 
  return NS_OK;
}

nsresult sbMetadataJob::SetItemIsScanned( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem )
{
  return SetItemIs( NS_LITERAL_STRING("is_scanned"), aQuery, aTableName, aItem, PR_TRUE );
}


nsresult sbMetadataJob::SetItemIsWrittenAndDelete( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem, PRBool aExecute )
{
  nsresult rv = SetItemIs( NS_LITERAL_STRING("is_written"), aQuery, aTableName, aItem, aExecute );
  delete aItem;
  return rv;
}

nsresult sbMetadataJob::SetItemsAreWrittenAndDelete( sbIDatabaseQuery *aQuery, nsString aTableName, nsTArray< sbMetadataJob::jobitem_t * > &aItemArray )
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
    rv = SetItemIsWrittenAndDelete( aQuery, aTableName, aItemArray[ i ], PR_FALSE );
    NS_ENSURE_SUCCESS(rv, rv);
  }
  rv = aQuery->AddQuery( NS_LITERAL_STRING("commit") );
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  aItemArray.Clear();
  return NS_OK;
}

nsresult sbMetadataJob::SetItemIs( const nsAString &aColumnString, sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem, PRBool aExecute )
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
  rv = update->AddAssignmentString( aColumnString, NS_LITERAL_STRING("1") );
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
    PRBool error;
    rv = aQuery->Execute( &error );
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult sbMetadataJob::ResetUnwritten( sbIDatabaseQuery *aQuery, nsString aTableName )
{
  NS_ENSURE_ARG_POINTER( aQuery );

  nsresult rv;

  // Compose the query string.
  nsAutoString updateUnwritten;
  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  nsCOMPtr<sbISQLUpdateBuilder> update =
    do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->SetTableName( aTableName );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->AddAssignmentString( NS_LITERAL_STRING("is_scanned"), NS_LITERAL_STRING("0") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->CreateMatchCriterionString( aTableName,
                                            NS_LITERAL_STRING("is_written"),
                                            sbISQLUpdateBuilder::MATCH_EQUALS,
                                            NS_LITERAL_STRING("0"),
                                            getter_AddRefs(criterion) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->AddCriterion( criterion );
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
  PRBool error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

nsresult sbMetadataJob::StartHandlerForItem( sbMetadataJob::jobitem_t *aItem )
{
  NS_ENSURE_ARG_POINTER( aItem );
  nsresult rv;
  nsCOMPtr<sbIMetadataManager> MetadataManager = do_GetService("@songbirdnest.com/Songbird/MetadataManager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = MetadataManager->GetHandlerForMediaURL( aItem->url, getter_AddRefs(aItem->handler) );
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool async = PR_FALSE;
  return aItem->handler->Read( &async );
}

nsresult sbMetadataJob::AddMetadataToItem( sbMetadataJob::jobitem_t *aItem, PRBool aShouldFlush )
{
  NS_ENSURE_ARG_POINTER( aItem );

  nsresult rv;

  // TODO:
  // - Update the metadata handlers to use the new keystrings

  // Get the sbIMediaItem we're supposed to be updating
  nsCOMPtr<sbILibraryManager> libraryManager;
  libraryManager = do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbILibrary> library;
  rv = libraryManager->GetLibrary( aItem->library_guid, getter_AddRefs(library) );
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIMediaItem> item;
  rv = library->GetMediaItem( aItem->item_guid, getter_AddRefs(item) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the metadata values that were found
  nsCOMPtr<sbIMetadataValues> values;
  rv = aItem->handler->GetValues( getter_AddRefs(values) );
  NS_ENSURE_SUCCESS(rv, rv);

/*
  // These are the old API metadata key strings
  const keys = new Array("title", "length", "album", "artist", "genre", "year", "composer", "track_no",
                         "track_total", "disc_no", "disc_total", "service_uuid");
*/

  // Set the properties (eventually iterate when the sbIMetadataValue have the correct keystrings).
  NS_NAMED_LITERAL_STRING( trackNameKey, "http://songbirdnest.com/data/1.0#trackName" );
  nsAutoString trackName;
  rv = values->GetValue( NS_LITERAL_STRING("title"), trackName );
  // If the metadata read can't even find a song name, cook one up off the url.
  if ( trackName.EqualsLiteral("") )
    trackName = CreateDefaultItemName( aItem->url );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( trackNameKey, trackName );
//  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( artistKey, "http://songbirdnest.com/data/1.0#artistName" );
  nsAutoString artist;
  rv = values->GetValue( NS_LITERAL_STRING("artist"), artist );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( artistKey, artist );
//  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( albumKey, "http://songbirdnest.com/data/1.0#albumName" );
  nsAutoString album;
  rv = values->GetValue( NS_LITERAL_STRING("album"), album );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( albumKey, album );
//  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( durationKey, "http://songbirdnest.com/data/1.0#duration" );
  nsAutoString duration;
  rv = values->GetValue( NS_LITERAL_STRING("length"), duration );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( durationKey, duration );
//  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( genreKey, "http://songbirdnest.com/data/1.0#genre" );
  nsAutoString genre;
  rv = values->GetValue( NS_LITERAL_STRING("genre"), genre );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( genreKey, genre );
//  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( yearKey, "http://songbirdnest.com/data/1.0#year" );
  nsAutoString year;
  rv = values->GetValue( NS_LITERAL_STRING("year"), year );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( yearKey, year );
//  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( trackKey, "http://songbirdnest.com/data/1.0#track" );
  nsAutoString track;
  rv = values->GetValue( NS_LITERAL_STRING("track_no"), track );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( trackKey, track );
//  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( discNumberKey, "http://songbirdnest.com/data/1.0#discNumber" );
  nsAutoString discNumber;
  rv = values->GetValue( NS_LITERAL_STRING("disc_no"), discNumber );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( discNumberKey, discNumber );
//  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( totalTracksKey, "http://songbirdnest.com/data/1.0#totalTracks" );
  nsAutoString totalTracks;
  rv = values->GetValue( NS_LITERAL_STRING("track_total"), totalTracks );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( totalTracksKey, totalTracks );
//  NS_ENSURE_SUCCESS(rv, rv);

  NS_NAMED_LITERAL_STRING( totalDiscsKey, "http://songbirdnest.com/data/1.0#totalDiscs" );
  nsAutoString totalDiscs;
  rv = values->GetValue( NS_LITERAL_STRING("disc_total"), totalDiscs );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( totalDiscsKey, totalDiscs );
//  NS_ENSURE_SUCCESS(rv, rv);

  if ( aShouldFlush )
  {
    rv = item->Write();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult sbMetadataJob::AddDefaultMetadataToItem( sbMetadataJob::jobitem_t *aItem, PRBool aShouldFlush )
{
  NS_ENSURE_ARG_POINTER( aItem );

  nsresult rv;

  // Get the sbIMediaItem we're supposed to be updating
  nsCOMPtr<sbILibraryManager> libraryManager;
  libraryManager = do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbILibrary> library;
  rv = libraryManager->GetLibrary( aItem->library_guid, getter_AddRefs(library) );
  NS_ENSURE_SUCCESS(rv, rv);
  nsCOMPtr<sbIMediaItem> item;
  rv = library->GetMediaItem( aItem->item_guid, getter_AddRefs(item) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the properties (eventually iterate when the sbIMetadataValue have the correct keystrings).
  NS_NAMED_LITERAL_STRING( trackNameKey, "http://songbirdnest.com/data/1.0#trackName" );
  nsAutoString trackName = CreateDefaultItemName( aItem->url );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( trackNameKey, trackName );
//  NS_ENSURE_SUCCESS(rv, rv);

  if ( aShouldFlush )
  {
    rv = item->Write();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsString sbMetadataJob::CreateDefaultItemName( const nsString &aURLString )
{
  nsString retval = aURLString;

  // First, unescape the url.
  nsCOMPtr< nsITextToSubURI > textToSubURI = do_GetService("@mozilla.org/intl/texttosuburi;1");
  nsCAutoString charset; // Blank.  On error, the following function does utf8 -> utf16 automatically.
  textToSubURI->UnEscapeURIForUI( charset, NS_ConvertUTF16toUTF8(aURLString), retval );

  // Then pull out just the text after the last slash
  PRInt32 slash = retval.RFindChar( *(NS_LITERAL_STRING("/").get()) );
  if ( slash != -1 )
    retval.Cut( 0, slash + 1 ); 
  slash = retval.RFindChar( *(NS_LITERAL_STRING("\\").get()) );
  if ( slash != -1 )
    retval.Cut( 0, slash + 1 ); 

  return retval;
}

void sbMetadataJob::IncrementDataRemote()
{
  PRInt32 current;
  mDataCurrentMetadataJobs->GetIntValue( &current );
  mDataStatusDisplay->SetStringValue( mStatusDisplayString );
  // Set to the incremented value
  mDataCurrentMetadataJobs->SetIntValue( ++current );
}

void sbMetadataJob::DecrementDataRemote()
{
  PRInt32 current;
  mDataCurrentMetadataJobs->GetIntValue( &current );
  // Set to the decremented value
  mDataCurrentMetadataJobs->SetIntValue( --current );
}
