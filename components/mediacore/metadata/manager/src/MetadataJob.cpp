/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
#include <pratom.h>

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
#include <nsIClassInfoImpl.h>
#include <nsIProgrammingLanguage.h>
#include <nsIURI.h>
#include <nsIIOService.h>
#include <pref/nsIPrefService.h>
#include <pref/nsIPrefBranch2.h>

#include <sbILibrary.h>
#include <sbILibraryManager.h>
#include <sbILibraryResource.h>
#include <sbILocalDatabaseLibrary.h>
#include <sbILocalDatabaseMediaItem.h>
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

#include <sbStringUtils.h>
#include <sbTArrayStringEnumerator.h>


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
const PRUint32 NUM_ITEMS_BEFORE_FLUSH = 400;
const PRUint32 NUM_ITEMS_PER_INIT_LOOP = 100;
const PRUint32 TIMER_LOOP_MS = 100;

// CLASSES ====================================================================

class sbRunInitParams : public nsISupports
{
  NS_DECL_ISUPPORTS
public:
  sbRunInitParams(sbMetadataJob* aJob,
                  nsCOMArray<sbIMediaItem>& aItemArray,
                  PRUint32& aStartIndex,
                  PRUint32 aEndIndex)
  : job(aJob), itemArray(aItemArray), startIndex(aStartIndex), endIndex(aEndIndex)
  { }
  sbMetadataJob* job; // Non-owning. 
  nsCOMArray<sbIMediaItem>& itemArray;
  PRUint32& startIndex;
  const PRUint32 endIndex;
};
NS_IMPL_ISUPPORTS0(sbRunInitParams)

class sbRunThreadParams : public nsISupports
{
  NS_DECL_ISUPPORTS
public:
  sbRunThreadParams(sbMetadataJob* aJob,
                    nsCOMPtr<sbIDatabaseQuery>& aQuery,
                    nsCOMPtr<nsIThread>& aThread,
                    PRBool* const aShutdown)
  : job(aJob), query(aQuery), thread(aThread), shutdown(aShutdown)
  { }
  sbMetadataJob* job; // Non-owning. 
  nsCOMPtr<sbIDatabaseQuery>& query;
  nsCOMPtr<nsIThread>& thread;
  PRBool* const shutdown;
};
NS_IMPL_ISUPPORTS0(sbRunThreadParams)

NS_IMPL_THREADSAFE_ADDREF(sbMetadataJob::jobitem_t)
NS_IMPL_THREADSAFE_RELEASE(sbMetadataJob::jobitem_t)

NS_IMPL_THREADSAFE_ISUPPORTS4(sbMetadataJob, nsIClassInfo, sbIMetadataJob, sbIJobProgress, sbIJobCancelable);
NS_IMPL_CI_INTERFACE_GETTER4(sbMetadataJob, nsIClassInfo, sbIMetadataJob, sbIJobProgress, sbIJobCancelable)
NS_DECL_CLASSINFO(sbMetadataJob)
NS_IMPL_THREADSAFE_CI(sbMetadataJob)


NS_IMPL_THREADSAFE_ISUPPORTS1(sbMetadataJobProcessorThread, nsIRunnable)

sbMetadataJob::sbMetadataJob() :
  mInitCount( 0 ),
  mStatus( sbIJobProgress::STATUS_RUNNING ),
  mCompletedItemCount( 0 ),
  mTotalItemCount( 0 ),
  mErrorCount( 0 ),
  mCurrentItem( nsnull ),
  mCurrentItemLock( nsnull ),
  mEnableRatingWrite( PR_FALSE ),
  mMetadataJobProcessor( nsnull ),
  mInitCompleted( PR_FALSE ),
  mInitExecuted( PR_FALSE ),
  mTimerCompleted( PR_FALSE ),
  mThreadCompleted( PR_FALSE )
{
  TRACE(("sbMetadataJob[0x%.8x] - ctor", this));

  mCurrentItemLock = nsAutoLock::NewLock("MetadataJob status text lock");
  NS_ASSERTION(mCurrentItemLock,
               "sbMetadataJob::mCurrentItemLock failed to create lock!");
}

sbMetadataJob::~sbMetadataJob()
{
  TRACE(("sbMetadataJob[0x%.8x] - dtor", this));
  nsAutoLock::DestroyLock(mCurrentItemLock);
}

// TODO no longer part of the interface.  Move if needed.
nsresult sbMetadataJob::GetTableName(nsAString & aTableName)
{
  aTableName = mTableName;
  return NS_OK;
}

/* readonly attribute unsigned short type; */
NS_IMETHODIMP sbMetadataJob::GetType(PRUint16 *aJobType)
{
  NS_ENSURE_ARG_POINTER( aJobType );
  *aJobType = mJobType;
  return NS_OK;
}

/* readonly attribute unsigned short status; */
NS_IMETHODIMP sbMetadataJob::GetStatus(PRUint16 *aStatus)
{
  NS_ENSURE_ARG_POINTER( aStatus );
  *aStatus = mStatus;
  return NS_OK;
}

/* readonly attribute unsigned AString statusText; */
NS_IMETHODIMP sbMetadataJob::GetStatusText(nsAString& aText)
{
  nsresult rv = NS_OK;
  
  // If the job is still running, report the leaf file name
  // as status.
  if (mStatus == sbIJobProgress::STATUS_RUNNING) {
    // The current item may be updated from the worker thread
    nsString escapedURL;

    { /* scope for mCurrentItemLock */
      nsAutoLock lock(mCurrentItemLock);
      
      if (mCurrentItem) {
        escapedURL = mCurrentItem->url;
      }
    } /* end scope for mCurrentItemLock */
    
    // need to unescape the file name
    nsCOMPtr<nsIIOService> ioSvc =
      do_GetService("@mozilla.org/network/io-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIURI> uri;
    rv = ioSvc->NewURI(NS_ConvertUTF16toUTF8(escapedURL),
                       nsnull,
                       nsnull,
                       getter_AddRefs(uri));
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsIURL> url = do_QueryInterface(uri, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCString escapedFileName;
    rv = url->GetFileName(escapedFileName);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCOMPtr<nsINetUtil> netUtil =
    do_GetService("@mozilla.org/network/util;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsCString fileName;
    rv = netUtil->UnescapeString(escapedFileName,
                                 nsINetUtil::ESCAPE_URL_SKIP_CONTROL,
                                 fileName);
    NS_ENSURE_SUCCESS(rv, rv);
    
    aText.Assign(NS_ConvertUTF8toUTF16(fileName));
    
    // Force the filename to be one line in order to
    // avoid sizeToContent pain
    if (aText.Length() > 45) {
      aText.Replace(20, aText.Length() - 40, NS_LITERAL_STRING("..."));
    }
      
  } else if (mStatus == sbIJobProgress::STATUS_FAILED) {
    // If we've failed, give a localized explanation.
    nsAutoString text;
    
    // Only set the status text for write jobs, since read jobs
    // are not currently reflected in the UI.
    if (mJobType == sbIMetadataJob::JOBTYPE_WRITE) {
    
      nsCOMPtr<nsIStringBundle> stringBundle;
      nsCOMPtr<nsIStringBundleService> StringBundleService =
        do_GetService("@mozilla.org/intl/stringbundle;1", &rv );
      NS_ENSURE_SUCCESS(rv, rv);

      rv = StringBundleService->CreateBundle( "chrome://songbird/locale/songbird.properties",
                                              getter_AddRefs( stringBundle ) );
      NS_ENSURE_SUCCESS(rv, rv);
    
      // Single failure in single item write job
      if (mTotalItemCount == 1) {
        rv = stringBundle->GetStringFromName( NS_LITERAL_STRING("metadatajob.writing.failed.one").get(),
                                              getter_Copies(text) );      
      // Single error in multiple file write job
      } else if (mErrorCount == 1) {
        rv = stringBundle->GetStringFromName( NS_LITERAL_STRING("metadatajob.writing.failed.oneofmany").get(),
                                              getter_Copies(text) );
      // Multiple errors in multiple file write job
      } else {
        rv = stringBundle->GetStringFromName( NS_LITERAL_STRING("metadatajob.writing.failed.manyofmany").get(),
                                              getter_Copies(text) );
      }
      
      if (NS_FAILED(rv)) {
        text.AssignLiteral("Job Failed");
      }
      
      aText = text;
    }
  
  } else {
    // Clear out the status text
    aText = EmptyString();
  } 
  
  return rv;
}

nsresult sbMetadataJob::SetCurrentItem(nsRefPtr<jobitem_t> &aJobItem)
{
  // Current item may be updated from the worker thread
  nsAutoLock lock(mCurrentItemLock);
  mCurrentItem = aJobItem;
  return NS_OK;
}

/* readonly attribute AString titleText; */
NS_IMETHODIMP sbMetadataJob::GetTitleText(nsAString& aText)
{
  aText = mTitleText;
  return NS_OK;
}

/* readonly attribute unsigned long progress; */
NS_IMETHODIMP sbMetadataJob::GetProgress(PRUint32 *aProgress)
{
  NS_ENSURE_ARG_POINTER( aProgress );
  *aProgress = mCompletedItemCount;
  return NS_OK;
}

/* readonly attribute unsigned long total; */
NS_IMETHODIMP sbMetadataJob::GetTotal(PRUint32 *aTotal)
{
  NS_ENSURE_ARG_POINTER( aTotal );
  *aTotal = mTotalItemCount;
  return NS_OK;
}

/* readonly attribute unsigned long errorCount; */
NS_IMETHODIMP sbMetadataJob::GetErrorCount(PRUint32 *aCount)
{
  NS_ENSURE_ARG_POINTER( aCount );
  *aCount = mErrorCount;
  return NS_OK;
}

/* nsIStringEnumerator getErrorMessages(); */
NS_IMETHODIMP sbMetadataJob::GetErrorMessages(nsIStringEnumerator **aMessages)
{
  NS_ENSURE_ARG_POINTER( aMessages );
  *aMessages = nsnull;
  nsresult rv;
  rv = UpdateErrorMessages();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStringEnumerator> enumerator = new sbTArrayStringEnumerator(&mErrorMessages);
  NS_ENSURE_TRUE(enumerator, NS_ERROR_OUT_OF_MEMORY);
  
  enumerator.forget(aMessages);

  return NS_OK;
}

/* void addJobProgressListener( in sbIJobProgressListener aListener ); */
NS_IMETHODIMP sbMetadataJob::AddJobProgressListener(sbIJobProgressListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  
  if (mListeners.Get(aListener, nsnull)) {
    NS_WARNING("Trying to add a listener twice!");
    return NS_OK;
  }
    
  PRBool success = mListeners.Put(aListener, aListener);
  NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  
  return NS_OK;
}

/* void removeJobProgressListener( in sbIJobProgressListener aListener ); */
NS_IMETHODIMP sbMetadataJob::RemoveJobProgressListener(sbIJobProgressListener *aListener)
{
  NS_ENSURE_ARG_POINTER(aListener);
  
#ifdef DEBUG
  if (!mListeners.Get(aListener, nsnull)) {
    NS_WARNING("Trying to remove a listener that was never added!");
  }
#endif
  mListeners.Remove(aListener);
  
  return NS_OK;
}

nsresult sbMetadataJob::Init(const nsAString & aTableName, 
                             nsIArray *aMediaItemsArray, 
                             PRUint32 aSleepMS,
                             PRUint16 aJobType)
{
  nsresult rv;

  NS_ENSURE_TRUE(!aTableName.IsEmpty(), NS_ERROR_INVALID_ARG);
  NS_ASSERTION(NS_IsMainThread(), "Init called off the main thread!");
  
  mDataStatusDisplay =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mDataStatusDisplay->Init( NS_LITERAL_STRING("backscan.status"),
                                 EmptyString() );
  NS_ENSURE_SUCCESS(rv, rv);

  mDataCurrentMetadataJobs =
    do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mDataCurrentMetadataJobs->Init( NS_LITERAL_STRING("backscan.concurrent"),
                                       EmptyString() );
  NS_ENSURE_SUCCESS(rv, rv);

  // Figure out if we are allowed to write rating values to files
  nsCOMPtr<nsIPrefBranch> prefService =
      do_GetService( "@mozilla.org/preferences-service;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv);
  prefService->GetBoolPref( "songbird.metadata.ratings.enableWriting", &mEnableRatingWrite);

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

  nsCOMPtr<sbIURIMetadataHelper> uriMetadataHelper = new sbURIMetadataHelper();
  NS_ENSURE_TRUE(uriMetadataHelper, NS_ERROR_OUT_OF_MEMORY);

  rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                            NS_GET_IID(sbIURIMetadataHelper),
                            uriMetadataHelper,
                            NS_PROXY_SYNC | NS_PROXY_ALWAYS,
                            getter_AddRefs(mURIMetadataHelper));
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  // aMediaItemsArray may be null, but not zero length
  if (aMediaItemsArray) {
    PRUint32 length;
    rv = aMediaItemsArray->GetLength(&length);
    NS_ENSURE_TRUE(length, NS_ERROR_INVALID_ARG);
  }

  mTableName = aTableName;
  mJobType = aJobType;
  mSleepMS = aSleepMS;
                                                    
  PRBool succeeded = mListeners.Init();
  NS_ENSURE_TRUE(succeeded, NS_ERROR_OUT_OF_MEMORY);


  //  - Initialize the task table in the database
  //      (library guid, item guid, url, worker_thread, is_started)

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

                              // True if item processing as begun (partly complete, must restart)
                              "is_started INTEGER NOT NULL DEFAULT 0, "

                              // True if the item has been completely processed (fully complete, no restart needed)
                              "is_completed INTEGER NOT NULL DEFAULT 0, "

                              // True if the item is currently being processed (ignore on restart, item failed)
                              "is_current INTEGER NOT NULL DEFAULT 0, "
                              
                              // True if the item could not be processed
                              "is_failed INTEGER NOT NULL DEFAULT 0, "

                              "PRIMARY KEY (library_guid, item_guid) )" );
  rv = mMainThreadQuery->AddQuery( createTable );
  NS_ENSURE_SUCCESS(rv, rv);

  // Compose the create index query string
  nsAutoString createIndex;
  createIndex.AppendLiteral( "CREATE INDEX IF NOT EXISTS 'idx_" );
  createIndex += mTableName;
  createIndex.AppendLiteral(  "_worker_thread_is_started' ON " );
  createIndex += mTableName;
  createIndex.AppendLiteral(  " (worker_thread, is_started)" );
  rv = mMainThreadQuery->AddQuery( createIndex );
  NS_ENSURE_SUCCESS(rv, rv);

  // Compose the (other) create index query string
  createIndex.Truncate();
  createIndex.AppendLiteral( "CREATE INDEX IF NOT EXISTS 'idx_" );
  createIndex += mTableName;
  createIndex.AppendLiteral(  "_is_completed_is_current' ON " );
  createIndex += mTableName;
  createIndex.AppendLiteral(  " (is_completed, is_current)" );
  rv = mMainThreadQuery->AddQuery( createIndex );
  NS_ENSURE_SUCCESS(rv, rv);

  // Create all that groovy stuff.
  PRInt32 error;
  rv = mMainThreadQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  // Update any old items that were started but not completed
  rv = ResetIncomplete( mMainThreadQuery, mTableName );
  NS_ENSURE_SUCCESS(rv, rv);

  // Init the progress counters based on preexisting items in the table
  rv = InitProgressReporting();
  NS_ENSURE_SUCCESS(rv, rv);

  // Append any new items in the array to the task table
  if ( aMediaItemsArray )
  {
    Append( aMediaItemsArray );
  }
  else {
    // If aMediaItemsArray is not null, this is an old job.  Determine what
    //library is associated with this job.
    rv = GetJobLibrary( mMainThreadQuery, aTableName, getter_AddRefs(mLibrary));
    if (NS_FAILED(rv)) {

      // XXXsteve If GetJobLibrary fails, this means either we have an existing
      // job with no entires, or the library for this job can not be found
      // in the library manager.  Drop the job.  bug 5026 is about adding the
      // ability to keep this job around until the library reappears.
      NS_WARNING("Invalid job or unable to find job's library, dropping");

      rv = DropJobTable(mMainThreadQuery, mTableName);
      NS_ENSURE_SUCCESS(rv, rv);

      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  // At this point we should know the library of this job.
  NS_ENSURE_TRUE(mLibrary, NS_ERROR_UNEXPECTED);

  // Launch the timer to complete initialization.
  rv = mTimer->InitWithFuncCallback( MetadataJobTimer, this, TIMER_LOOP_MS, nsITimer::TYPE_ONE_SHOT );
  NS_ENSURE_SUCCESS(rv, rv);

  // So now it's safe to launch the thread
  mMetadataJobProcessor = new sbMetadataJobProcessorThread( this );
  NS_ENSURE_TRUE(mMetadataJobProcessor, NS_ERROR_OUT_OF_MEMORY);

  rv = NS_NewThread( getter_AddRefs(mThread), mMetadataJobProcessor );
  NS_ENSURE_SUCCESS(rv, rv);

  // Tell the world we're starting to process something.
  IncrementDataRemote();

  // Phew!  We're done!
  return NS_OK;
}

/* void append (in nsIArray aMediaItemsArray); */
NS_IMETHODIMP sbMetadataJob::Append(nsIArray *aMediaItemsArray)
{
  NS_ENSURE_ARG_POINTER( aMediaItemsArray );
  nsresult rv;

  if (mStatus != sbIJobProgress::STATUS_RUNNING)
    return NS_ERROR_UNEXPECTED;

  // How many Media Items?
  PRUint32 length;
  rv = aMediaItemsArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);
  
  // Add to the total length of the job
  mTotalItemCount += length;

  // Determine the library this job is associated with
  if (!mLibrary && length > 0) {
    nsCOMPtr<sbIMediaItem> mediaItem =
      do_QueryElementAt(aMediaItemsArray, 0, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mediaItem->GetLibrary(getter_AddRefs(mLibrary));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Break out the media items into a useful array
  nsCOMArray<sbIMediaItem> appendArray;
  nsCOMPtr<sbILibrary> library;
  for (PRUint32 i = 0; i < length; i++) {
    nsCOMPtr<sbIMediaItem> mediaItem = do_QueryElementAt(aMediaItemsArray, i, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    // Ensure that all of the items in this job are from the same library
    nsCOMPtr<sbILibrary> otherLibrary;
    rv = mediaItem->GetLibrary(getter_AddRefs(otherLibrary));
    NS_ENSURE_SUCCESS(rv, rv);

    PRBool equals;
    rv = otherLibrary->Equals(mLibrary, &equals);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!equals) {
      NS_ERROR("Not all items from the same library");
      return NS_ERROR_INVALID_ARG;
    }

    PRBool success = appendArray.AppendObject( mediaItem );
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
  }

  // Run the loop to send all of these items onto our work queue in the database
  PRUint32 start = 0;
  RunInit(appendArray, start, length);

//  mInitCompleted = PR_FALSE;

  return NS_OK;
}



/* boolean canCancel; */
NS_IMETHODIMP sbMetadataJob::GetCanCancel(PRBool *_retval)
{
  *_retval = PR_TRUE;
  return NS_OK;
}


/* void cancel(); */
NS_IMETHODIMP sbMetadataJob::Cancel()
{
  // Quit the timer.
  CancelTimer();

  if ( mMetadataJobProcessor )
  {
    // Tell the thread to go away, please.
    mMetadataJobProcessor->mShutdown = PR_TRUE;

    // Wait for the thread to shutdown and release all resources.
    if (mThread) {
      mThread->Shutdown();
      mThread = nsnull;
    }
  }

  // Notify observers
  OnJobProgress();
  
  return NS_OK;
}


nsresult sbMetadataJob::RunTimer()
{
  nsresult rv = ProcessTimer();
  NS_ENSURE_SUCCESS(rv, rv);

  if (mStatus != sbIJobProgress::STATUS_RUNNING) {
    mTimer = nsnull;
  }
  else {
    rv = mTimer->InitWithFuncCallback(MetadataJobTimer,
                                      this,
                                      TIMER_LOOP_MS,
                                      nsITimer::TYPE_ONE_SHOT);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult sbMetadataJob::RunInit(nsCOMArray<sbIMediaItem> &aArray, PRUint32 &aStart, PRUint32 aEnd)
{
  nsresult rv;

  nsCOMPtr<sbIMediaList> libraryMediaList = do_QueryInterface(mLibrary, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaListBatchCallback> batchCallback =
    new sbMediaListBatchCallback(&sbMetadataJob::RunInitBatchFunc);
  NS_ENSURE_TRUE(batchCallback, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<sbRunInitParams> params =
    new sbRunInitParams(this, aArray, aStart, aEnd);
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

  rv = libraryMediaList->RunInBatchMode(batchCallback, params);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* static */ nsresult
sbMetadataJob::RunInitBatchFunc(nsISupports* aUserData)
{
  NS_ENSURE_ARG_POINTER(aUserData);

  sbRunInitParams* params = static_cast<sbRunInitParams*>(aUserData);
  params->job->ProcessInit(params->itemArray, params->startIndex, params->endIndex);
  
  return NS_OK;
}

nsresult sbMetadataJob::ProcessInit(nsCOMArray<sbIMediaItem> &aArray, PRUint32 &aStart, PRUint32 aEnd)
{
  NS_ASSERTION(mMainThreadQuery, "Null query!");

  nsresult rv = mMainThreadQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("begin"));
  NS_ENSURE_SUCCESS(rv, rv);

  for ( ; aStart < aEnd; aStart++) {
    // Place the item info on the query to track in our own database, get back
    // a temporary job item.
    nsRefPtr<jobitem_t> tempItem;
    AddItemToJobTableQuery(mMainThreadQuery, mTableName,
                           aArray[aStart],
                           getter_AddRefs(tempItem));

    if (mJobType == sbIMetadataJob::JOBTYPE_READ) {
      // Use the temporary item to stuff a default title value in the properties
      // cache (don't flush to library database, yet)
      AddDefaultMetadataToItem(tempItem, aArray[aStart]);
    }
  }

  rv = mMainThreadQuery->AddQuery(NS_LITERAL_STRING("commit"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mMainThreadQuery->SetAsyncQuery(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error;
  rv = mMainThreadQuery->Execute(&error);
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  return NS_OK;
}


nsresult sbMetadataJob::ProcessTimer()
{
  nsresult rv;

  // First the timer handles the initialization, creating the work table.
  // Then it launches the thread and starts handling any main-thread items.
  if ( ! mInitCompleted )
  {
    PRUint32 total = mInitArray.Count(), max = mInitCount + NUM_ITEMS_PER_INIT_LOOP;
    PRUint32 end = (total < max) ? max : total;

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
      rv = RunInit( mInitArray, mInitCount, end );
      NS_ENSURE_SUCCESS(rv, rv);

      // Relaunch the thread if it has completed and we add new task items.
      if ( mThreadCompleted )
      {
        if (mThread) {
          mThread->Shutdown();
          mThread = nsnull;
        }
        mThreadCompleted = PR_TRUE;
        // So now it's safe to re-launch the thread
        rv = NS_NewThread( getter_AddRefs(mThread), mMetadataJobProcessor );
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
        // NULL the array entry
        mTimerWorkers[ i ] = nsnull;

        rv = ClearItemIsCurrent( mMainThreadQuery, mTableName, item );
        NS_ENSURE_SUCCESS(rv, rv);

        // If this is a read job, try to copy metadata out of the handler
        if (mJobType == sbIMetadataJob::JOBTYPE_READ) {
          rv = AddMetadataToItem( item, mURIMetadataHelper ); // Flush properties cache every time
          // NS_ENSURE_SUCCESS(rv, rv); // Allow it to fail.  We already put a default value in for it.
        }
 
        if (NS_SUCCEEDED(rv)) {
          rv = SetItemIsCompleted( mMainThreadQuery, mTableName, item );
        } else {
          rv = SetItemIsFailed( mMainThreadQuery, mTableName, item );
        }
        NS_ENSURE_SUCCESS(rv, rv);
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

      TRACE(("sbMetadataJob[0x%.8x] - MAIN THREAD - GetNextItem( %s )",
             this, NS_LossyConvertUTF16toASCII(alert).get()));
#endif

      // Get ready to launch a handler
      rv = SetItemIsCurrent( mMainThreadQuery, mTableName, item );
      NS_ENSURE_SUCCESS(rv, rv);
      rv = SetItemIsStarted( mMainThreadQuery, mTableName, item );
      NS_ENSURE_SUCCESS(rv, rv);

      // Create the metadata handler and launch it
      rv = StartHandlerForItem( item, mJobType );
      // Ignore errors on the metadata loop, just set a default and keep going.
      if ( NS_FAILED(rv) )
      {
        // Give a nice warning
        TRACE(("sbMetadataJob[0x%.8x] - WORKER THREAD - "
               "Unable to add metadata for( %s )", this,
               NS_LossyConvertUTF16toASCII(item->url).get()));
        // Record this item as completed.
        rv = SetItemIsFailed( mMainThreadQuery, mTableName, item );
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
  if ( finished && mThreadCompleted ) { 
    this->FinishJob();
  } else {
    // Notify observers
    OnJobProgress();
  }
  
  // Cleanup the timer loop.
  return NS_OK;
}

nsresult sbMetadataJob::CancelTimer()
{
  // Fill with nsnull to release metadata handlers and kill their active channels.
  for ( PRUint32 i = 0; i < NUM_CONCURRENT_MAINTHREAD_ITEMS; i++ )
  {
    nsRefPtr<sbMetadataJob::jobitem_t> item = mTimerWorkers[ i ];
    if (item && item->handler) {
      item->handler->Close();
      item->handler = nsnull;
    }
    mTimerWorkers[ i ] = nsnull;
  }

  // Quit the timer.
  if (mTimer) {
    nsresult rv = mTimer->Cancel();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

nsresult sbMetadataJob::RunThread( PRBool * bShutdown )
{
  NS_ENSURE_STATE(mLibrary);

  nsresult rv;
  nsCOMPtr<sbIDatabaseQuery> workerThreadQuery(
    do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  workerThreadQuery->SetDatabaseGUID(NS_LITERAL_STRING(SBMETADATAJOB_DATABASE_GUID));
  workerThreadQuery->SetAsyncQuery(PR_FALSE);


  nsCOMPtr<sbIMediaListBatchCallback> batchCallback =
    new sbMediaListBatchCallback(&sbMetadataJob::RunThreadBatchFunc);
  NS_ENSURE_TRUE(batchCallback, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<sbRunThreadParams> params =
    new sbRunThreadParams(this, workerThreadQuery, mThread, bShutdown);
  NS_ENSURE_TRUE(params, NS_ERROR_OUT_OF_MEMORY);

  while (!*bShutdown) {
    rv = mLibrary->RunInBatchMode(batchCallback, params);
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      // This is our signal that all items have been processed.
      break;
    }
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // Thread ends when the function returns.
  return NS_OK;
}

/* static */ nsresult
sbMetadataJob::RunThreadBatchFunc(nsISupports* aUserData)
{
  NS_ENSURE_ARG_POINTER(aUserData);
  sbRunThreadParams* params = static_cast<sbRunThreadParams*>(aUserData);
  
  return params->job->ProcessThread(params->shutdown, params->query, params->thread);
}

nsresult sbMetadataJob::ProcessThread(PRBool *aShutdown, sbIDatabaseQuery *aQuery, nsIThread* aThread)
{
  NS_ENSURE_ARG_POINTER(aShutdown);
  NS_ENSURE_ARG_POINTER(aQuery);
  
  nsresult rv = NS_ERROR_NOT_AVAILABLE;

  nsTArray<nsRefPtr<sbMetadataJob::jobitem_t> > completedItems;

  for (PRUint32 count = 0;
       !(*aShutdown) && (count < NUM_ITEMS_BEFORE_FLUSH);
       count++) {

    // Get the next task
    nsRefPtr<sbMetadataJob::jobitem_t> item;
    rv = GetNextItem(aQuery, mTableName, PR_TRUE, getter_AddRefs(item));
    if (rv == NS_ERROR_NOT_AVAILABLE) {
      break;
    }
    NS_ENSURE_SUCCESS(rv, rv);

#ifdef PR_LOGGING
    // Do a little debug output
    NS_NAMED_LITERAL_STRING(sp, " - ");
    nsString alert(item->library_guid);
    alert += sp;
    alert += item->item_guid;
    alert += sp;
    alert += item->url;
    alert += sp;
    alert += item->worker_thread;

    TRACE(("sbMetadataJob[static] - WORKER THREAD - GetNextItem( %s )",
           NS_LossyConvertUTF16toASCII(alert).get()));
#endif

    // Get ready to launch a handler
    rv = SetItemIsCurrent(aQuery, mTableName, item);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = SetItemIsStarted(aQuery, mTableName, item);
    NS_ENSURE_SUCCESS(rv, rv);

    // Create the metadata handler and launch it
    rv = StartHandlerForItem(item, mJobType);
    if (NS_SUCCEEDED(rv)) {
      // Wait at least a half second for it to complete
      PRBool completed;
      int counter = 0;
      for (item->handler->GetCompleted(&completed);
           !completed && !(*aShutdown) && counter < 25;
           item->handler->GetCompleted(&completed), counter++) {

        // Run at most 10 messages.
        PRBool event = PR_FALSE;
        int eventCount = 0;
        for (aThread->ProcessNextEvent(PR_FALSE, &event);
             event && eventCount < 10;
             aThread->ProcessNextEvent(PR_FALSE, &event), eventCount++) {
          PR_Sleep(PR_MillisecondsToInterval(0));
        }
        // Sleep at least 20ms
        PR_Sleep(PR_MillisecondsToInterval(20));
      }

      if (mJobType == sbIMetadataJob::JOBTYPE_READ) {
        // Make an sbIMediaItem and push the metadata into it
        rv = AddMetadataToItem(item, mURIMetadataHelper);
      }
      
      // Close the handler by hand since we know we're done with it and
      // we won't get rid of the item for awhile.
      if (item->handler) {
        item->handler->Close();
        item->handler = nsnull;
      }
    }
    // Ignore errors on the metadata loop, just set a warning and keep going.
    if (NS_FAILED(rv)) {
      // Give a nice warning
      TRACE(("sbMetadataJob[static] - WORKER THREAD - "
             "Unable to add metadata for( %s )",
             NS_LossyConvertUTF16toASCII(item->url).get()));

      SetItemIsFailed(aQuery, mTableName, item);

      if (mJobType == sbIMetadataJob::JOBTYPE_READ) {
        // Don't leave the row completely blank. Please.
        AddDefaultMetadataToItem(item, item->item);
      }
    } else {
      // Mark completed items in batches
      completedItems.AppendElement(item);
      
      // Increment the completed item count immediately so that 
      // the sbIJobProgress is always up to date
      PR_AtomicIncrement(&mCompletedItemCount);
    }

    // Ah, if we got here we must not have crashed.
    rv = ClearItemIsCurrent(aQuery, mTableName, item);
    NS_ENSURE_SUCCESS(rv, rv);

    // TODO: Consider removing this!
    // On my machine I get a 30% speedup with no loss of interactivity.
    // Need to test on a slow single-core machine first.
    
    // XXXAus: I think we could probably make it sleep 0 so that the thread
    //         would at least yield.
    PR_Sleep(PR_MillisecondsToInterval(0));
  }

  // Flush the completed items list.
  nsresult rvOuter = SetItemsAreCompleted(aQuery, 
                                          mTableName,
                                          completedItems);
  NS_ENSURE_SUCCESS(rvOuter, rvOuter);

  // Return the rv from within the loop.
  return rv;
}


nsresult sbMetadataJob::FinishJob()
{
  nsresult rv;
  this->CancelTimer();

  if (mErrorCount == 0) {
    mStatus = sbIJobProgress::STATUS_SUCCEEDED;
  } else {
    // HACK: cause the error list to be cached, so the getMessages won't fail
    // once the database table has been dropped.  
    UpdateErrorMessages();
  
    mStatus = sbIJobProgress::STATUS_FAILED;    
  }
  
  if (mThread) {
    mThread->Shutdown();
    mThread = nsnull;
  }
  
  // Notify observers
  OnJobProgress();
  
  // We're done. No more notifications needed.
  mListeners.Clear();

  // Decrement the atomic counter
  DecrementDataRemote();
  
  rv = DropJobTable(mMainThreadQuery, mTableName);
  NS_ENSURE_SUCCESS(rv, rv);

  // Flush.
  rv = mLibrary->Flush();
  NS_ENSURE_SUCCESS(rv, rv);

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
  rv = insert->AddColumn( NS_LITERAL_STRING("is_started") );
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
                                           NS_LITERAL_STRING("is_started"),
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

    nsAutoString isStarted;
    rv = pResult->GetRowCellByColumn(0, NS_LITERAL_STRING( "is_started" ), isStarted);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> library;
    rv = libraryManager->GetLibrary( libraryGuid, getter_AddRefs(library) );
    if (NS_SUCCEEDED(rv)) {
#if PROXY_LIBRARY
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

#if PROXY_LIBRARY
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
        
        LOG(("metadata job url: %s\n", NS_ConvertUTF16toUTF8(uriSpec).get()));

        nsRefPtr<jobitem_t> jobitem;
        jobitem = new jobitem_t( libraryGuid,
                                 itemGuid,
                                 uriSpec,
                                 workerThread,
                                 isStarted,
                                 item,
                                 nsnull );
        NS_ENSURE_TRUE(jobitem, NS_ERROR_OUT_OF_MEMORY);
        NS_ADDREF(*_retval = jobitem);
        return NS_OK;
      }
    }

    // If we get here, we were unable to find the media item associated with
    // the metadata job.  Just mark the item as started and failed and
    // continue

    TRACE(("sbMetadataJob - GetNextItem - Item with guid '%s' has gone away",
           NS_LossyConvertUTF16toASCII(itemGuid).get()));

    nsRefPtr<jobitem_t> tempItem(new jobitem_t( libraryGuid,
                                                itemGuid,
                                                EmptyString(),
                                                workerThread,
                                                isStarted,
                                                nsnull,
                                                nsnull ));
    NS_ENSURE_TRUE(tempItem, NS_ERROR_OUT_OF_MEMORY);
    rv = SetItemIsStarted( aQuery, aTableName, tempItem );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = SetItemIsFailed( aQuery, aTableName, tempItem );
    NS_ENSURE_SUCCESS(rv, rv);
 }

}

nsresult
sbMetadataJob::GetJobLibrary( sbIDatabaseQuery *aQuery,
                              const nsAString& aTableName,
                              sbILibrary **_retval )
{
  NS_ASSERTION( aQuery, "aQuery is null" );
  NS_ASSERTION( _retval, "_retval is null" );

  nsresult rv;

  nsCOMPtr<sbILibraryManager> libraryManager =
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbISQLSelectBuilder> select =
    do_CreateInstance(SB_SQLBUILDER_SELECT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = select->SetBaseTableName( aTableName );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = select->AddColumn( aTableName, NS_LITERAL_STRING("library_guid") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = select->SetLimit( 1 );
  NS_ENSURE_SUCCESS(rv, rv);

  nsString sql;
  rv = select->ToString( sql );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->AddQuery( sql );
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  nsCOMPtr< sbIDatabaseResult > pResult;
  rv = aQuery->GetResultObject( getter_AddRefs(pResult) );
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = pResult->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ENSURE_TRUE(rowCount == 1, NS_ERROR_UNEXPECTED);

  nsString libraryGuid;
  rv = pResult->GetRowCellByColumn( 0,
                                    NS_LITERAL_STRING( "library_guid" ),
                                    libraryGuid );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = libraryManager->GetLibrary(libraryGuid, _retval);
  NS_ENSURE_SUCCESS(rv, rv);
  return NS_OK;
}

/* static */ nsresult
sbMetadataJob::DropJobTable( sbIDatabaseQuery *aQuery, const nsAString& aTableName )
{
  NS_ASSERTION(aQuery, "aQuery is null");
  NS_ASSERTION(!aTableName.IsEmpty(), "aTableName is empty");

  nsresult rv;

  // Drop the working table
  nsString dropTable;
  dropTable.AppendLiteral("DROP TABLE ");
  dropTable += aTableName;
  rv = aQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->AddQuery( dropTable );
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  // Remove the entry from the tracking table
  nsString delTracking;
  nsCOMPtr<sbISQLBuilderCriterion> criterion;
  nsCOMPtr<sbISQLDeleteBuilder> del =
    do_CreateInstance(SB_SQLBUILDER_DELETE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = del->SetTableName( sbMetadataJob::DATABASE_GUID() );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = del->CreateMatchCriterionString( sbMetadataJob::DATABASE_GUID(),
                                        NS_LITERAL_STRING("job_guid"),
                                        sbISQLUpdateBuilder::MATCH_EQUALS,
                                        aTableName,
                                        getter_AddRefs(criterion) );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = del->AddCriterion( criterion );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = del->ToString( delTracking );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->AddQuery( delTracking );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  return NS_OK;
}

nsresult sbMetadataJob::SetItemIsStarted( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem )
{
  return SetItemIs( NS_LITERAL_STRING("is_started"), aQuery, aTableName, aItem, PR_TRUE );
}

nsresult sbMetadataJob::SetItemIsFailed( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem)
{
  nsresult rv;

  rv = aQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);

  rv = SetItemIs( NS_LITERAL_STRING("is_failed"), aQuery, aTableName, aItem, PR_FALSE, PR_TRUE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = SetItemIsCompleted(aQuery, aTableName, aItem, PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);

  PRInt32 error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);
  
  PR_AtomicIncrement(&mCompletedItemCount);
  PR_AtomicIncrement(&mErrorCount);
  
  return rv;
}

nsresult sbMetadataJob::SetItemIsCompleted( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem, PRBool aExecute )
{
  nsresult rv = SetItemIs( NS_LITERAL_STRING("is_completed"), aQuery, aTableName, aItem, aExecute );
  NS_ENSURE_SUCCESS(rv, rv);
  if ( aItem->handler ) {
    aItem->handler->Close();  // You are so done.
    aItem->handler = nsnull;
  }
  // Items may be marked completed in batches for performance reasons, but
  // we want to increment the counter as soon as an item is completed
  if (aExecute) {
  PR_AtomicIncrement(&mCompletedItemCount);
  }
  return rv;
}

nsresult sbMetadataJob::SetItemsAreCompleted( sbIDatabaseQuery *aQuery, nsString aTableName, nsTArray<nsRefPtr<sbMetadataJob::jobitem_t> > &aItemArray )
{
  nsresult rv;
  // Commit all the completed bits as a single transaction.
  rv = aQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->AddQuery( NS_LITERAL_STRING("begin") );
  NS_ENSURE_SUCCESS(rv, rv);
  for ( PRUint32 i = 0, end = aItemArray.Length(); i < end; i++ )
  {
    SetItemIsCompleted( aQuery, aTableName, aItemArray[ i ], PR_FALSE );
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
  nsRefPtr<jobitem_t> itemRef(aItem);
  nsresult rv = SetCurrentItem(itemRef);
  NS_ENSURE_SUCCESS(rv, rv);
  return SetItemIs( NS_LITERAL_STRING("is_current"), aQuery, aTableName, aItem, PR_TRUE );
}

nsresult sbMetadataJob::ClearItemIsCurrent( sbIDatabaseQuery *aQuery, nsString aTableName, sbMetadataJob::jobitem_t *aItem )
{
  return SetItemIs( NS_LITERAL_STRING("is_current"), aQuery, aTableName, aItem, PR_TRUE, PR_FALSE );
}

nsresult sbMetadataJob::ResetIncomplete( sbIDatabaseQuery *aQuery, nsString aTableName )
{
  NS_ENSURE_ARG_POINTER( aQuery );
  
  LOG(("sbMetadataJob - resetting incomplete jobs\n"));

  nsresult rv;

  // Compose the query string.
  nsAutoString updateIncomplete;
  nsCOMPtr<sbISQLBuilderCriterion> criterionW;
  nsCOMPtr<sbISQLBuilderCriterion> criterionC;
  nsCOMPtr<sbISQLBuilderCriterion> criterionAND;
  nsCOMPtr<sbISQLUpdateBuilder> update =
    do_CreateInstance(SB_SQLBUILDER_UPDATE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->SetTableName( aTableName );
  NS_ENSURE_SUCCESS(rv, rv);
  // Set "is_started" back to 0
  rv = update->AddAssignmentString( NS_LITERAL_STRING("is_started"), NS_LITERAL_STRING("0") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = update->CreateMatchCriterionString( aTableName,
                                            // If we hadn't completed it out yet
                                            NS_LITERAL_STRING("is_completed"),
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
  rv = update->ToString( updateIncomplete );
  NS_ENSURE_SUCCESS(rv, rv);

  // Make it go.
  rv = aQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = aQuery->AddQuery( updateIncomplete );
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 error;
  rv = aQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);
  return NS_OK;
}

/**
 * Set up progress strings and restore progress/total/errorcount 
 * based on the database.
 */
nsresult sbMetadataJob::InitProgressReporting()
{
  NS_ENSURE_TRUE(mMainThreadQuery, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;
  

  // Localize our strings
  nsCOMPtr<nsIStringBundle> stringBundle;
  nsCOMPtr<nsIStringBundleService> StringBundleService =
    do_GetService("@mozilla.org/intl/stringbundle;1", &rv );
  NS_ENSURE_SUCCESS(rv, rv);

  rv = StringBundleService->CreateBundle( "chrome://songbird/locale/songbird.properties",
                                          getter_AddRefs( stringBundle ) );
  NS_ENSURE_SUCCESS(rv, rv);


  if (mJobType == sbIMetadataJob::JOBTYPE_WRITE) {
    rv = stringBundle->GetStringFromName( NS_LITERAL_STRING("metadatajob.writing.title").get(),
                                          getter_Copies(mTitleText) );
    if (NS_FAILED(rv)) {
      mTitleText.AssignLiteral("Metadata Write Job");
    }
    
    rv = stringBundle->GetStringFromName( NS_LITERAL_STRING("back_scan.writing").get(),
                                          getter_Copies(mStatusDisplayString) );
    if (NS_FAILED(rv)) {
      mStatusDisplayString.AssignLiteral("Writing");
    }
    
  } else {
    rv = stringBundle->GetStringFromName( NS_LITERAL_STRING("metadatajob.reading.title").get(),
                                          getter_Copies(mTitleText) );
    if (NS_FAILED(rv)) {
      mTitleText.AssignLiteral("Metadata Read Job");
    }
    
    rv = stringBundle->GetStringFromName( NS_LITERAL_STRING("back_scan.scanning").get(),
                                          getter_Copies(mStatusDisplayString) );
    if (NS_FAILED(rv)) {
      mStatusDisplayString.AssignLiteral("Scanning");
    }
  }
    
  mDataStatusDisplay->SetStringValue( mStatusDisplayString );
  

  // Find the total number of items in the job
  nsAutoString selectCount(NS_LITERAL_STRING("select count(*) from "));
  selectCount.Append(mTableName);
  
  // Find out how many are already complete
  nsAutoString whereComplete(selectCount);
  whereComplete.AppendLiteral(" where is_completed=1");

  // Find out how many are already complete
  nsAutoString whereFailed(selectCount);
  whereFailed.AppendLiteral(" where is_failed=1");


  // Make it go.
  rv = mMainThreadQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->AddQuery( selectCount );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->AddQuery( whereComplete );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mMainThreadQuery->AddQuery( whereFailed );
  NS_ENSURE_SUCCESS(rv, rv);
  PRInt32 error;
  rv = mMainThreadQuery->Execute( &error );
  NS_ENSURE_SUCCESS(rv, rv);
  NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

  // And return what it got.
  nsCOMPtr< sbIDatabaseResult > pResult;
  rv = mMainThreadQuery->GetResultObject( getter_AddRefs(pResult) );
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 rowCount;
  rv = pResult->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  if (rowCount != 3) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsAutoString value;
  
  // Get total
  rv = pResult->GetRowCell(0, 0, value);
  NS_ENSURE_SUCCESS(rv, rv);  
  mTotalItemCount = value.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get complete
  rv = pResult->GetRowCell(1, 0, value);
  NS_ENSURE_SUCCESS(rv, rv);  
  mCompletedItemCount = value.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get failed
  rv = pResult->GetRowCell(2, 0, value);
  NS_ENSURE_SUCCESS(rv, rv);  
  mErrorCount = value.ToInteger(&rv);
  NS_ENSURE_SUCCESS(rv, rv);

  TRACE(("sbMetadataJob[0x%.8x] - MAIN THREAD - InitProgressReporting() %d/%d completed, %d failed",
         this, mCompletedItemCount, mTotalItemCount, mErrorCount));
  
  return NS_OK;
}

/**
 * Update mErrorMessages with the current list of failed
 * URLs from the database.
 */
nsresult sbMetadataJob::UpdateErrorMessages() 
{
  NS_ENSURE_TRUE(mMainThreadQuery, NS_ERROR_NOT_INITIALIZED);
  nsresult rv;

  if (mErrorCount > 0 && mStatus == sbIJobProgress::STATUS_RUNNING) {
   
    // Find all failed items
    nsAutoString selectFailed(NS_LITERAL_STRING("select url from "));
    selectFailed.Append(mTableName);
    selectFailed.AppendLiteral(" where is_failed=1");

    // Make it go.
    rv = mMainThreadQuery->SetAsyncQuery( PR_FALSE );
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainThreadQuery->ResetQuery();
    NS_ENSURE_SUCCESS(rv, rv);
    rv = mMainThreadQuery->AddQuery( selectFailed );
    NS_ENSURE_SUCCESS(rv, rv);
    PRInt32 error;
    rv = mMainThreadQuery->Execute( &error );
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(error == 0, NS_ERROR_FAILURE);

    // And return what it got.
    nsCOMPtr< sbIDatabaseResult > pResult;
    rv = mMainThreadQuery->GetResultObject( getter_AddRefs(pResult) );
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 rowCount;
    rv = pResult->GetRowCount(&rowCount);
    NS_ENSURE_SUCCESS(rv, rv);

    mErrorMessages.Clear();
  
    for (PRUint32 i = 0; i < rowCount; i++) {
      nsString value;
      rv = pResult->GetRowCell(i, 0, value);
      NS_ENSURE_SUCCESS(rv, rv);  
      nsString* appended = mErrorMessages.AppendElement(value);
      NS_ENSURE_TRUE(appended, NS_ERROR_OUT_OF_MEMORY);
    }
  }

  return NS_OK;
}


nsresult sbMetadataJob::StartHandlerForItem( sbMetadataJob::jobitem_t *aItem, PRUint16 aJobType)
{
  NS_ENSURE_ARG_POINTER( aItem );
  nsresult rv = NS_ERROR_FAILURE;
  // No item is a failed metadata attempt.
  // Don't bother to chew the CPU or the filesystem by reading metadata.
  if ( aItem->item && !aItem->url.IsEmpty() )
  {
    LOG(("sbMetadataJob::StartHandlerForItem: getting handler for %s\n",
         NS_LossyConvertUTF16toASCII(aItem->url).get()));
    nsCOMPtr<sbIMetadataManager> MetadataManager =
      do_GetService("@songbirdnest.com/Songbird/MetadataManager;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = MetadataManager->GetHandlerForMediaURL( aItem->url,
                                                 getter_AddRefs(aItem->handler) );
    if (rv == NS_ERROR_UNEXPECTED) {
      // no handler for uri, try getting a handler for originURL if available
      nsString originUrl;
      rv = aItem->item->GetProperty( NS_LITERAL_STRING(SB_PROPERTY_ORIGINURL),
                                     originUrl );
      NS_ENSURE_SUCCESS(rv, rv);
      if (originUrl.IsEmpty()) {
        // no origin URL, too bad - restore the error
        rv = NS_ERROR_UNEXPECTED;
      } else {
        // only use origin url if it's a local file
        // unfortunately, we can't touch the IO service since this can run on
        // a background thread
        if (StringBeginsWith(originUrl, NS_LITERAL_STRING("file://"))) {
          aItem->url = originUrl;
          rv = MetadataManager->GetHandlerForMediaURL( originUrl,
                                                       getter_AddRefs(aItem->handler) );
        } else {
          // nope, can't use non-file origin url
          rv = NS_ERROR_UNEXPECTED;
        }
      }
    }
    #if PR_LOGGING
    if (NS_FAILED(rv)) {
      LOG(("Failed to find metadata handler\n"));
    }
    #endif /* PR_LOGGING */
    NS_ENSURE_SUCCESS(rv, rv);
    NS_ENSURE_TRUE(aItem->handler, NS_ERROR_FAILURE);
    
    PRBool async = PR_FALSE;

    if (aJobType == sbIMetadataJob::JOBTYPE_WRITE) {
      nsCOMPtr<sbIPropertyArray> props;
      rv = aItem->item->GetProperties(nsnull, getter_AddRefs(props));

      nsCOMPtr<sbIMutablePropertyArray> writeProps = do_QueryInterface(props, &rv);
      NS_ENSURE_SUCCESS(rv, rv);
      
      // For all the basic values, if there is no value on 
      // the item, it means we want to null out/remove 
      // the value from the file.  This is painful, 
      // but writing doesn't have to be fast.
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_TRACKNAME), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_ARTISTNAME), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_ALBUMARTISTNAME), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_ALBUMNAME), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_COMMENT), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_LYRICS), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_GENRE), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_PRODUCERNAME), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_COMPOSERNAME), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_CONDUCTORNAME), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_LYRICISTNAME), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_RECORDLABELNAME), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_LANGUAGE), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_KEY), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_COPYRIGHT), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_COPYRIGHTURL), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_YEAR), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_TRACKNUMBER), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_TOTALTRACKS), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_DISCNUMBER), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_TOTALDISCS), writeProps);
      EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_BPM), writeProps);
      
      // If we aren't allowed to write the rating, remove it from 
      // the property array.  :(
      if (!mEnableRatingWrite) {

        PRUint32 propertyCount;
        rv = props->GetLength(&propertyCount);
        NS_ENSURE_SUCCESS(rv, rv);

        nsCOMPtr<nsIMutableArray> array = do_QueryInterface(props, &rv);
        NS_ENSURE_SUCCESS(rv, rv);
        
        for (PRUint32 i = 0; i < propertyCount; i++) {
          nsCOMPtr<sbIProperty> property;
          rv = props->GetPropertyAt(i, getter_AddRefs(property));
          NS_ENSURE_SUCCESS(rv, rv);

          nsString propertyID;
          rv = property->GetId(propertyID);
          NS_ENSURE_SUCCESS(rv, rv);
          if (propertyID.EqualsLiteral(SB_PROPERTY_RATING)) {
            array->RemoveElementAt(i);
            break;
          }
        }
      } else {
        // We are allowed to write rating, so ensure that it
        // is either being set or nulled out
        EnsurePropertyIsInArray(NS_LITERAL_STRING(SB_PROPERTY_RATING), writeProps);
      }

      rv = aItem->handler->SetProps(writeProps);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = aItem->handler->Write( &async );
    } else {
      rv = aItem->handler->Read( &async );
    }
  }
  return rv;
}

/**
 * If the given property is not present in the given property array, 
 * add a value of empty string for the property
 */
nsresult sbMetadataJob::EnsurePropertyIsInArray( const nsAString& aProperty,
                                                 sbIMutablePropertyArray *aPropertyArray)
{
  NS_ASSERTION(!aProperty.IsEmpty(), "Don't pass an empty property id!");
  NS_ENSURE_ARG_POINTER( aPropertyArray );

  nsresult rv;
  nsString value;
  rv = aPropertyArray->GetPropertyValue(aProperty, value);

  // If the property is not in the array, add it as an empty string
  if (NS_FAILED(rv)) {
    value = EmptyString();
    value.SetIsVoid(PR_TRUE);
    rv = aPropertyArray->AppendProperty(aProperty, value);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Metadata job failed to add null property");
  }

  return NS_OK;
}

nsresult sbMetadataJob::AddMetadataToItem( sbMetadataJob::jobitem_t *aItem,
                                           sbIURIMetadataHelper *aURIMetadataHelper )
{
  NS_ENSURE_ARG_POINTER( aItem );
  NS_ENSURE_ARG_POINTER( aURIMetadataHelper );

  nsresult rv;

// TODO:
  // - Update the metadata handlers to use the new keystrings
  
  LOG(("sbMetadataJob::AddMetadataToItem - starting\n"));

  // Get the sbIMediaItem we're supposed to be updating
  nsCOMPtr<sbIMediaItem> item = aItem->item;

  // Get the metadata values that were found
  nsCOMPtr<sbIMutablePropertyArray> props, newProps;
  rv = aItem->handler->GetProps( getter_AddRefs(props) );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get a new array we're going to copy across
  newProps = do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the property manager because we love it so much
  nsCOMPtr<sbIPropertyManager> propMan =
    do_GetService(SB_PROPERTYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Set the properties
  NS_NAMED_LITERAL_STRING( trackNameKey, SB_PROPERTY_TRACKNAME );
  nsAutoString oldName;
  rv = item->GetProperty( trackNameKey, oldName );
  nsAutoString trackName;
  rv = props->GetPropertyValue( trackNameKey, trackName );
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool defaultTrackname = trackName.IsEmpty() && !oldName.IsEmpty();
  // If the metadata read can't even find a song name,
  // AND THERE ISN'T ALREADY A TRACK NAME, cook one up off the url.
  if ( trackName.IsEmpty() && oldName.IsEmpty() ) {
    rv = CreateDefaultItemName( aItem->url, trackName );
    NS_ENSURE_SUCCESS(rv, rv);
    // And then add/change it
    if ( !trackName.IsEmpty() ) {
      rv = AppendIfValid( propMan, newProps, trackNameKey, trackName );
      NS_ENSURE_SUCCESS(rv, rv);
      defaultTrackname = PR_TRUE;
    }
  }

  // Loop through the returned props to copy to the new props
  PRUint32 propsLength = 0;
  rv = props->GetLength(&propsLength);
  NS_ENSURE_SUCCESS(rv, rv);

  for ( PRUint32 i = 0; i < propsLength && NS_SUCCEEDED(rv); i++ ) {
    nsCOMPtr<sbIProperty> prop;
    rv = props->GetPropertyAt( i, getter_AddRefs(prop) );
    if ( NS_FAILED(rv) )
      break;
    nsAutoString id, value;
    prop->GetId( id );
    if ( !defaultTrackname || !id.Equals(trackNameKey) ) {
      prop->GetValue( value );
      if ( !value.IsEmpty() && !value.IsVoid() && !value.EqualsLiteral(" ") ) {
        AppendIfValid( propMan, newProps, id, value );
      }
    }
  }

  // Then calc filesize, just to enjoy it.
  //
  // TODO: Currently proxied to the main thread to placate nsIIOService.
  // This costs 15% when scanning 2500 tracks.
  
  PRInt64 fileSize = 0;
  rv = aURIMetadataHelper->GetFileSize(aItem->url, &fileSize);
  if (NS_SUCCEEDED(rv)) {
    nsAutoString contentLength;
    contentLength.AppendInt(fileSize);
    rv = AppendIfValid( propMan,
                        newProps,
                        NS_LITERAL_STRING(SB_PROPERTY_CONTENTLENGTH),
                        contentLength );
    NS_ENSURE_SUCCESS(rv, rv);
  }

  // And then put the properties into the item
  
  // TODO A great deal of time is spent setting
  // properties on the items, since the items
  // must query for ids, then query for all lists
  // that contain the items, then call notify on all lists.
  // I'd love to just disable this, but it turns out the
  // notifications are necessary when dropping items
  // from the desktop into a playlist.
  // For the next release we should look at caching
  // media item ids and list membership.
/*
  nsCOMPtr<sbILocalDatabaseMediaItem> localDBItem =
    do_QueryInterface(item, &rv);
  if (NS_SUCCEEDED(rv)) {
    localDBItem->SetSuppressNotifications(PR_TRUE);
  } else {
    localDBItem = nsnull;    
  }
*/
  rv = item->SetProperties(newProps);
  // NS_ENSURE_SUCCESS(rv, rv);  
/*
  if (localDBItem) {
    localDBItem->SetSuppressNotifications(PR_FALSE);
  }
*/
  LOG(("sbMetadataJob::AddMetadataToItem - finished with end rv %08x\n",
       rv));

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
                             const nsAString& aID,
                             const nsAString& aValue)
{
  NS_ASSERTION(aPropertyManager, "aPropertyManager is null");
  NS_ASSERTION(aProperties, "aProperties is null");

  nsCOMPtr<sbIPropertyInfo> info;
  nsresult rv = aPropertyManager->GetPropertyInfo(aID, getter_AddRefs(info));
  NS_ENSURE_SUCCESS(rv, rv);

  PRBool isValid = PR_FALSE;
  rv = info->Validate(aValue, &isValid);
  NS_ENSURE_SUCCESS(rv, rv);

  if (isValid) {
    rv = aProperties->AppendProperty(aID, aValue);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

/**
 * Hash callback to add listeners to an array
 */
/* static */ PLDHashOperator PR_CALLBACK
sbMetadataJob::AddListenersToCOMArrayCallback(nsISupportsHashKey::KeyType aKey,
                                                sbIJobProgressListener* aEntry,
                                                void* aUserData)
{
  NS_ASSERTION(aKey, "Null key in hashtable!");
  NS_ASSERTION(aEntry, "Null entry in hashtable!");
  
  nsCOMArray<sbIJobProgressListener>* array =
  static_cast<nsCOMArray<sbIJobProgressListener>*>(aUserData);
  
  PRBool success = array->AppendObject(aEntry);
  NS_ENSURE_TRUE(success, PL_DHASH_STOP);
  
  return PL_DHASH_NEXT;
}

/**
 * Notify listeners of job progress.  Called periodically by the main thread timer.
 */
nsresult 
sbMetadataJob::OnJobProgress() 
{
  nsCOMArray<sbIJobProgressListener> listeners;
  mListeners.EnumerateRead(AddListenersToCOMArrayCallback, &listeners);

  PRInt32 count = listeners.Count();
  for (PRInt32 index = 0; index < count; index++) {
    nsCOMPtr<sbIJobProgressListener> listener = listeners.ObjectAt(index);
    NS_ASSERTION(listener, "Null listener!"); 
    listener->OnJobProgress(this);
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

