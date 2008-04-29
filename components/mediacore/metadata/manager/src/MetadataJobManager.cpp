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
* \file MetadataJobManager.cpp
* \brief 
*/

// INCLUDES ===================================================================
#include <nspr.h>
#include <nscore.h>

#include <unicharutil/nsUnicharUtils.h>
#include <nsComponentManagerUtils.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsIObserverService.h>
#include <nsIAppStartupNotifier.h>
#include <nsArrayUtils.h>
#include "nsIPrefBranch.h"
#include "nsIPromptService.h"
#include "nsIWindowMediator.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowInternal.h"


#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>
#include <sbIMediaItem.h>
#include <sbILibrary.h>

#include <sbIDataRemote.h>

#include "MetadataJobManager.h"
#include "MetadataJob.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#endif

#define NS_APPSTARTUP_CATEGORY           "app-startup"
#define NS_FINAL_UI_STARTUP_OBSERVER_ID  "final-ui-startup"
#define NS_PROFILE_SHUTDOWN_OBSERVER_ID  "profile-before-change"


// DEFINES ====================================================================

// GLOBALS ====================================================================
sbMetadataJobManager *gMetadataJobManager = nsnull;

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS2(sbMetadataJobManager, sbIMetadataJobManager, nsIObserver)

sbMetadataJobManager::sbMetadataJobManager()
{
}

sbMetadataJobManager::~sbMetadataJobManager()
{
}

nsresult sbMetadataJobManager::Init()
{
  nsresult rv;
  
  PR_LOG(gMetadataLog, PR_LOG_DEBUG, ("Metadata Job Manager starting up..."));

  mQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mQuery->SetDatabaseGUID( sbMetadataJob::DATABASE_GUID() );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr< sbIDataRemote > dataDisplayString = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  dataDisplayString->Init( NS_LITERAL_STRING("backscan.status"), NS_LITERAL_STRING("") );
  dataDisplayString->SetStringValue( NS_LITERAL_STRING("") );
  nsCOMPtr< sbIDataRemote > dataCurrentMetadataJobs = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  dataCurrentMetadataJobs->Init( NS_LITERAL_STRING("backscan.concurrent"), NS_LITERAL_STRING("") );
  dataCurrentMetadataJobs->SetIntValue( 0 );
  
  return rv;
}


nsresult sbMetadataJobManager::Shutdown()
{
  PR_LOG(gMetadataLog, PR_LOG_DEBUG, ("Metadata Job Manager shutting down..."));
  
  nsresult rv;
  
  // the act of cancelling jobs may get us more jobs as the threads shut down.
  // so we have to keep checking the number of jobs outstanding.
  while (1) {
    PRInt32 i = mJobArray.Count() - 1;
    if (i < 0)
      break;
    nsCOMPtr<sbIJobCancelable> cancelableJob(do_QueryInterface(mJobArray[ i ]));
    rv = cancelableJob->Cancel();
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to cancel a metadata job");
    mJobArray.RemoveObjectAt(i);
  }
  NS_ASSERTION(0 == mJobArray.Count(), "Metadata jobs remaining after stopping the manager");

  mQuery = nsnull;

  return NS_OK;
}



NS_IMETHODIMP 
sbMetadataJobManager::NewJob(nsIArray *aMediaItemsArray,
                             PRUint32 aSleepMS,
                             PRUint16 aJobType,
                             sbIMetadataJob **_retval)
{
  NS_ENSURE_ARG_POINTER(aMediaItemsArray);
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ENSURE_TRUE(mQuery, NS_ERROR_NOT_INITIALIZED);

  nsresult rv;

  // Do not allow write jobs unless the user has specifically given
  // permission.  
  if (aJobType == sbIMetadataJob::JOBTYPE_WRITE) {
    rv = EnsureWritePermitted();
    NS_ENSURE_SUCCESS(rv, rv);
  } 

  // Ensure that all of the items are from the same library
  PRUint32 length;
  rv = aMediaItemsArray->GetLength(&length);
  NS_ENSURE_SUCCESS(rv, rv);

  if (length > 0) {
    nsCOMPtr<sbIMediaItem> mediaItem =
      do_QueryElementAt(aMediaItemsArray, 0, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbILibrary> library;
    rv = mediaItem->GetLibrary(getter_AddRefs(library));
    NS_ENSURE_SUCCESS(rv, rv);

    for (PRUint32 i = 1; i < length; i++) {
      mediaItem = do_QueryElementAt(aMediaItemsArray, i, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<sbILibrary> otherLibrary;
      rv = mediaItem->GetLibrary(getter_AddRefs(otherLibrary));
      NS_ENSURE_SUCCESS(rv, rv);

      PRBool equals;
      rv = otherLibrary->Equals(library, &equals);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!equals) {
        NS_ERROR("Not all items from the same library");
        return NS_ERROR_INVALID_ARG;
      }
    }

  }

  nsRefPtr<sbMetadataJob> task = new sbMetadataJob();
  NS_ENSURE_TRUE(task, NS_ERROR_OUT_OF_MEMORY);

  // Create a resource guid for this job.
  nsCOMPtr<nsIUUIDGenerator> uuidGen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsID id;
  rv = uuidGen->GenerateUUIDInPlace(&id);
  NS_ENSURE_SUCCESS(rv, rv);

  char guidChars[NSID_LENGTH];
  id.ToProvidedString(guidChars);

  nsString fullGuid(NS_ConvertASCIItoUTF16(nsDependentCString(guidChars,
                                                              NSID_LENGTH - 1)));

  nsAutoString tableName = sbMetadataJob::DATABASE_GUID(); // Can't start a table name with a number
  tableName.AppendLiteral( "_" );
  tableName += Substring(fullGuid, 1, 8); // Or have dashes.  :(
  tableName += Substring(fullGuid, 10, 4);
  tableName += Substring(fullGuid, 15, 4);
  tableName += Substring(fullGuid, 20, 4);
  tableName += Substring(fullGuid, 25, 12);

  // Compose the insert string to the master table
  nsAutoString insertItem;
  nsCOMPtr<sbISQLInsertBuilder> insert =
    do_CreateInstance(SB_SQLBUILDER_INSERT_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->SetIntoTableName( sbMetadataJob::DATABASE_GUID() );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddColumn( NS_LITERAL_STRING("job_guid") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddValueString( tableName );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddColumn( NS_LITERAL_STRING("ms_delay") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddValueLong( aSleepMS );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddColumn( NS_LITERAL_STRING("job_type") );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->AddValueLong( aJobType );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = insert->ToString( insertItem );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ExecuteQuery( insertItem );      
  NS_ENSURE_SUCCESS(rv, rv);

  // Kick off the task with the proper data
  rv = task->Init(tableName, aMediaItemsArray, aSleepMS, aJobType);
  NS_ENSURE_SUCCESS(rv, rv);
  mJobArray.AppendObject( task );

  NS_ADDREF(*_retval = task);
  return NS_OK;
}

// nsIObserver
NS_IMETHODIMP
sbMetadataJobManager::Observe(nsISupports *aSubject, 
                              const char *aTopic,
                              const PRUnichar *aData)
{
  nsresult rv;
  //
  // Initial app start
  //
  if (!strcmp(aTopic, APPSTARTUP_CATEGORY)) {
  
    // Listen for profile startup and profile shutdown messages
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obsSvc->AddObserver(observer, NS_FINAL_UI_STARTUP_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obsSvc->AddObserver(observer, NS_PROFILE_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  } 
  //
  // Profile Up
  //
  else if (!strcmp(NS_FINAL_UI_STARTUP_OBSERVER_ID, aTopic)) {
    
    rv = Init();
    NS_ENSURE_SUCCESS(rv, rv);
  
    // TODO We should defer this until after the UI is fully up.
    // Unfortunately there is no appropriate observer topic at this time.
    rv = RestartExistingJobs();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  //
  // Profile Down
  //
  else if (!strcmp(NS_PROFILE_SHUTDOWN_OBSERVER_ID, aTopic)) {

    rv = Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);

    // Remove all the observer callbacks
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obsSvc->RemoveObserver(observer, NS_FINAL_UI_STARTUP_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);
    rv = obsSvc->RemoveObserver(observer, NS_PROFILE_SHUTDOWN_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  return NS_OK;
}

nsresult sbMetadataJobManager::RestartExistingJobs()
{
  nsresult rv;

  // Compose the create table query string
  nsAutoString createTable;
  createTable.AppendLiteral( "CREATE TABLE IF NOT EXISTS " );
  createTable += sbMetadataJob::DATABASE_GUID();
  createTable.AppendLiteral( " (job_guid TEXT NOT NULL PRIMARY KEY, ms_delay INTEGER NOT NULL, job_type INTEGER NOT NULL)" );
  rv = ExecuteQuery( createTable );      
  NS_ENSURE_SUCCESS(rv, rv);

  // Compose the select query string
  nsAutoString selectTable;
  selectTable.AppendLiteral( "SELECT * FROM " );
  selectTable += sbMetadataJob::DATABASE_GUID();
  rv = ExecuteQuery( selectTable );
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the results
  nsCOMPtr<sbIDatabaseResult> result;
  rv = mQuery->GetResultObject( getter_AddRefs(result) );
  NS_ENSURE_SUCCESS(rv, rv);
  PRUint32 rowCount;
  rv = result->GetRowCount(&rowCount);
  NS_ENSURE_SUCCESS(rv, rv);

  // Launch the unfinished tasks
  for ( PRUint32 i = 0; i < rowCount; i++ )
  {
    nsAutoString tableName, strSleep, strType;
    rv = result->GetRowCell(i, 0, tableName);
    rv = result->GetRowCell(i, 1, strSleep);
    rv = result->GetRowCell(i, 2, strType);
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 aSleep = strSleep.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint16 jobType = strType.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    nsRefPtr<sbMetadataJob> task = new sbMetadataJob();
    NS_ENSURE_TRUE(task, NS_ERROR_OUT_OF_MEMORY);

    rv = task->Init(tableName, nsnull, aSleep, jobType);
    NS_ENSURE_SUCCESS(rv, rv);

    mJobArray.AppendObject( task ); // Keep a reference around to it.
  }

  return rv;
}

/**
 * Return NS_OK if write jobs are permitted, or 
 * NS_ERROR_NOT_AVAILABLE if not.
 *
 * May prompt the user to permit write jobs.
 */
nsresult sbMetadataJobManager::EnsureWritePermitted()
{
  nsresult rv;

  PRBool enableWriting = PR_FALSE;
  nsCOMPtr<nsIPrefBranch> prefService =
  do_GetService( "@mozilla.org/preferences-service;1", &rv );
  NS_ENSURE_SUCCESS( rv, rv);
  prefService->GetBoolPref( "songbird.metadata.enableWriting", &enableWriting );

  if (!enableWriting) {    
    
    // Let the user know what the situation is. 
    // Allow them to enable writing if desired.
    
    PRBool promptOnWrite = PR_TRUE;
    prefService->GetBoolPref( "songbird.metadata.promptOnWrite", &promptOnWrite );
    
    if (promptOnWrite) {
      // Don't bother to prompt unless there is a player window open.
      // This avoids popping a modal dialog mid unit test.
      nsCOMPtr<nsIWindowMediator> windowMediator =
        do_GetService("@mozilla.org/appshell/window-mediator;1", &rv);
      NS_ENSURE_SUCCESS( rv, rv);
      nsCOMPtr<nsIDOMWindowInternal> mainWindow;  
      windowMediator->GetMostRecentWindow(NS_LITERAL_STRING("Songbird:Main").get(),
                                          getter_AddRefs(mainWindow));
      if (mainWindow) {
        nsCOMPtr<nsIPromptService> promptService =
          do_GetService("@mozilla.org/embedcomp/prompt-service;1", &rv);
        NS_ENSURE_SUCCESS( rv, rv);

        PRBool promptResult = PR_FALSE;
        PRBool checkState = PR_FALSE;

        // TODO Clean up, localize, or remove from the product
        rv = promptService->ConfirmCheck(mainWindow,                  
              NS_LITERAL_STRING("WARNING! TAG WRITING IS EXPERIMENTAL!").get(),
              NS_MULTILINE_LITERAL_STRING( 
                NS_LL("Are you sure you want to write metadata changes")
                NS_LL(" back to your media files?\n\nTag writing has not been tested yet,")
                NS_LL(" and may damage your media files.  If you'd like to help us test")
                NS_LL(" this functionality, great, but we advise you to back up your media first.")
              ).get(),
              NS_LITERAL_STRING("Don't show this dialog again").get(),
              &checkState, 
              &promptResult);  
        NS_ENSURE_SUCCESS( rv, rv);
        
        if (checkState) {
          prefService->SetBoolPref( "songbird.metadata.promptOnWrite", PR_FALSE );
        }
        
        if (promptResult) {
          prefService->SetBoolPref( "songbird.metadata.enableWriting", PR_TRUE );
          enableWriting = PR_TRUE;
        }
      }
    }
  }

  return (enableWriting) ? NS_OK : NS_ERROR_NOT_AVAILABLE;
}

nsresult sbMetadataJobManager::ExecuteQuery( const nsAString &aQueryStr )
{
  nsresult rv;
  // Setup and execute it
  rv = mQuery->SetAsyncQuery( PR_FALSE );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mQuery->ResetQuery();
  NS_ENSURE_SUCCESS(rv, rv);
  rv = mQuery->AddQuery( aQueryStr );
  NS_ENSURE_SUCCESS(rv, rv);
  PRBool error;
  return mQuery->Execute( &error );
}

