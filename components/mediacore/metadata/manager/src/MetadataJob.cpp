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

#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbILibraryResource.h>
#include <sbIMediaItem.h>
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

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataJob, sbIMetadataJob);
NS_IMPL_THREADSAFE_ISUPPORTS1(MetadataJobProcessorThread, nsIRunnable)

sbMetadataJob::sbMetadataJob()
{
  nsresult rv;

  mMetadataManager = do_CreateInstance("@songbirdnest.com/Songbird/MetadataManager;1");
  NS_ASSERTION(mMetadataManager, "Unable to create sbMetadataJob::m_pManager");

  mMainThreadQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ASSERTION(mMainThreadQuery, "Unable to create sbMetadataJob::mMainThreadQuery");
  mMainThreadQuery->SetDatabaseGUID( sbMetadataJob::DATABASE_GUID() );
  mMainThreadQuery->SetAsyncQuery( PR_FALSE );

  mTimer = do_CreateInstance(NS_TIMER_CONTRACTID);
  NS_ASSERTION(mTimer, "Unable to create sbMetadataJob::mTimer");
}

sbMetadataJob::~sbMetadataJob()
{
  Cancel();
}

/* readonly attribute AString tableName; */
NS_IMETHODIMP sbMetadataJob::GetTableName(nsAString & aTableName)
{
    return NS_OK;
}

/* readonly attribute unsigned long itemsTotal; */
NS_IMETHODIMP sbMetadataJob::GetItemsTotal(PRUint32 *aItemsTotal)
{
    return NS_OK;
}

/* readonly attribute unsigned long itemsCompleted; */
NS_IMETHODIMP sbMetadataJob::GetItemsCompleted(PRUint32 *aItemsCompleted)
{
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

  // Compose the create table query string
  nsAutoString createTable;
  createTable.AppendLiteral( "CREATE TABLE " );
  createTable += mTableName;
  createTable.AppendLiteral( " (library_guid TEXT, item_guid TEXT UNIQUE NOT NULL, url TEXT, worker_thread INTEGER(0,1), is_scanned INTEGER(0,1))" );

  // Setup and execute it
  rv = mMainThreadQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->AddQuery( createTable );
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool error;
  rv = mMainThreadQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);

  //  - Append any items in the array to the task table
  if ( aMediaItemsArray != nsnull )
  {
    // How many Media Items?
    PRUint32 length;
    rv = aMediaItemsArray->GetLength(&length);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainThreadQuery->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);

    // Add them to our database table so we don't lose context if the app goes away.
    rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("begin"));
    for (PRUint32 i = 0; i < length; i++) {
      // Break out the media item
      nsCOMPtr<sbIMediaItem> mediaItem = do_QueryElementAt(aMediaItemsArray, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      // And shove it into the insertion query.
      rv = this->AddItemToJobTableQuery( mMainThreadQuery, mediaItem );
      NS_ENSURE_SUCCESS(rv, rv);

      // Flush the database transaction every 500 inserts.
      if ( ( i + 1 ) % 500 == 0 ) {
        rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("commit"));
        NS_ENSURE_SUCCESS(rv, rv);
        rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("begin"));
        NS_ENSURE_SUCCESS(rv, rv);
      }
    }
    rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("commit"));
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mMainThreadQuery->SetAsyncQuery( PR_FALSE ); // TODO: Async this.
    NS_ENSURE_SUCCESS(rv, rv);
    PRBool error;
    rv = mMainThreadQuery->Execute( &error );
    NS_ENSURE_SUCCESS(rv, rv);

/*********************
*********************
    // Test the process synchronously
    for ( int z = 0; z < length; z++ )
    {
      sbMetadataJob::jobitem_t *item;
      rv = GetNextItem( mMainThreadQuery, PR_TRUE, &item );
      NS_ENSURE_SUCCESS(rv, rv);

      nsString sp = NS_LITERAL_STRING(" - ");
      nsString alert;
      alert += item->library_guid;
      alert += sp;
      alert += item->item_guid;
      alert += sp;
      alert += item->url;
      alert += sp;
      alert += item->worker_thread;

      TRACE(("sbMetadataJob[0x%.8x] - GetNextItem(%s)", this, alert.get()));

      rv = StartHandlerForItem( item );
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool completed;
      for ( item->handler->GetCompleted( &completed ); !completed; item->handler->GetCompleted( &completed ) )
      {
        PR_Sleep( PR_MillisecondsToInterval(100) ); // ??
      }

      rv = AddMetadataToItem( item );
      NS_ENSURE_SUCCESS(rv, rv);

      rv = SetItemComplete( mMainThreadQuery, item );
      NS_ENSURE_SUCCESS(rv, rv);
    }
/*********************
*********************/
  }

  // TODO:
  // - Migrate the launchers when the insert query is async'd

  // Launch the timer
  rv = mTimer->InitWithFuncCallback( MetadataJobTimer, this, 10, nsITimer::TYPE_REPEATING_SLACK );
  NS_ENSURE_SUCCESS(rv, rv);

  // Launch the thread
  nsCOMPtr<nsIThread> pThread;
  nsCOMPtr<nsIRunnable> pMetatataJobProcessorRunner = new MetadataJobProcessorThread( this );
  NS_ASSERTION(pMetatataJobProcessorRunner, "Unable to create MetatataJobProcessorRunner");
  if ( pMetatataJobProcessorRunner )
  {
    rv = NS_NewThread( getter_AddRefs(mThread), pMetatataJobProcessorRunner );
    NS_ASSERTION(NS_SUCCEEDED(rv), "Unable to start thread");
  }
  NS_ENSURE_SUCCESS(rv, rv);

  // Phew!  We're done!
  return NS_OK;
}

/* void cancel (); */
NS_IMETHODIMP sbMetadataJob::Cancel()
{
  // Ask the thread to go away, please.
  mShouldShutdown = PR_TRUE;

  // Quit the timer.
  CancelTimer();

  return NS_OK;
}

nsresult sbMetadataJob::RunTimer()
{
  // TODO:
  // - Timer stuff.


  // Cleanup the timer loop.
  return this->CancelTimer();
}

nsresult sbMetadataJob::CancelTimer()
{
  // TODO:
  // - Timer cancellation stuff.


  // Quit the timer.
  return mTimer->Cancel();
}

nsresult sbMetadataJob::RunThread()
{
  // TODO:
  // - Thread stuff.


  // Thread ends when the function returns.
  return CancelThread();
}

nsresult sbMetadataJob::CancelThread()
{
  // TODO:
  // - Thread cancellation stuff.
  return NS_OK;
}

nsresult sbMetadataJob::AddItemToJobTableQuery( sbIDatabaseQuery *aQuery, sbIMediaItem *aMediaItem )
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
  rv = insert->SetIntoTableName( mTableName );
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

nsresult sbMetadataJob::GetNextItem( sbIDatabaseQuery *aQuery, PRBool isWorkerThread, sbMetadataJob::jobitem_t **_retval )
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
  rv = select->SetBaseTableName( mTableName );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = select->AddColumn( mTableName, NS_LITERAL_STRING("*") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = select->CreateMatchCriterionLong( mTableName,
                                            NS_LITERAL_STRING("worker_thread"),
                                            sbISQLSelectBuilder::MATCH_EQUALS,
                                            ( isWorkerThread ) ? 1 : 0,
                                            getter_AddRefs(criterionWT) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = select->CreateMatchCriterionLong( mTableName,
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
                  this->LG( pResult ),
                  this->IG( pResult ),
                  this->URL( pResult ),
                  this->WT( pResult ),
                  this->IS( pResult ),
                  nsnull
                );

  NS_ENSURE_TRUE(*_retval, NS_ERROR_OUT_OF_MEMORY); 
  return NS_OK;
}

nsresult sbMetadataJob::SetItemComplete( sbIDatabaseQuery *aQuery, sbMetadataJob::jobitem_t *aItem )
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
  rv = update->SetTableName( mTableName );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->AddAssignmentString( NS_LITERAL_STRING("is_scanned"), NS_LITERAL_STRING("1") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->CreateMatchCriterionString( mTableName,
                                            NS_LITERAL_STRING("library_guid"),
                                            sbISQLUpdateBuilder::MATCH_EQUALS,
                                            aItem->library_guid,
                                            getter_AddRefs(criterionWT) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->CreateMatchCriterionString( mTableName,
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
  rv = aQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->AddQuery( itemComplete );
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);

  // Delete the object.
  delete aItem;

  return rv;
}

nsresult sbMetadataJob::StartHandlerForItem( sbMetadataJob::jobitem_t *aItem )
{
  NS_ENSURE_ARG_POINTER( aItem );
  nsresult rv;
  rv = mMetadataManager->GetHandlerForMediaURL( aItem->url, getter_AddRefs(aItem->handler) );
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool async = PR_FALSE;
  return aItem->handler->Read( &async );
}

nsresult sbMetadataJob::AddMetadataToItem( sbMetadataJob::jobitem_t *aItem )
{
  NS_ENSURE_ARG_POINTER( aItem );

  nsresult rv;

  // TODO:
  // - Find some way to cache the database updates!
  // - Cache the library lookup in a hashtable
  // - Update the metadata handlers to use the new keystrings

  // Get the sbIMediaItem we're supposed to be updating
  nsCOMPtr<sbILibraryManager> libraryManager = do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
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
    const keys = new Array("title", "length", "album", "artist", "genre", "year", "composer", "track_no",
                         "track_total", "disc_no", "disc_total", "service_uuid");
*/

  // Set the properties (eventually iterate when the sbIMetadataValue have the correct keystrings).
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

  NS_NAMED_LITERAL_STRING( trackNameKey, "http://songbirdnest.com/data/1.0#trackName" );
  nsAutoString trackName;
  rv = values->GetValue( NS_LITERAL_STRING("title"), trackName );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = item->SetProperty( trackNameKey, trackName );
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

  return NS_OK;
}
