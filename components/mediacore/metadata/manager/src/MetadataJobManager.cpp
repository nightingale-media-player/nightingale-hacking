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

#include <sbISQLBuilder.h>
#include <sbSQLBuilderCID.h>

#include <sbIDataRemote.h>

#include "MetadataJobManager.h"
#include "MetadataJob.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#endif

#define NS_PROFILE_SHUTDOWN_OBSERVER_ID "profile-before-change"

// DEFINES ====================================================================

// GLOBALS ====================================================================
sbMetadataJobManager *gMetadataJobManager = nsnull;

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS2(sbMetadataJobManager, sbIMetadataJobManager, nsIObserver)

sbMetadataJobManager::sbMetadataJobManager()
{
  MOZ_COUNT_CTOR(sbMetadataJobManager);
  nsresult rv;

  // TODO:
  // - Get us instantiated at startup, somehow?
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  if(NS_SUCCEEDED(rv)) {
    observerService->AddObserver(this, NS_PROFILE_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  }
  else {
    NS_ERROR("Unable to register profile-before-change shutdown observer");
  }

  mQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ASSERTION(mQuery, "Unable to create sbMetadataJobManager::mQuery");
  mQuery->SetDatabaseGUID( sbMetadataJob::DATABASE_GUID() );
  mQuery->SetAsyncQuery( PR_FALSE );

  nsCOMPtr< sbIDataRemote > dataDisplayString = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1" );
  NS_ASSERTION(dataDisplayString, "Unable to create sbMetadataJob::dataCurrentMetadataJobs");
  dataDisplayString->Init( NS_LITERAL_STRING("songbird.backscan.status"), NS_LITERAL_STRING("") );
  dataDisplayString->SetStringValue( NS_LITERAL_STRING("") );
  nsCOMPtr< sbIDataRemote > dataCurrentMetadataJobs = do_CreateInstance("@songbirdnest.com/Songbird/DataRemote;1" );
  NS_ASSERTION(dataCurrentMetadataJobs, "Unable to create sbMetadataJob::dataCurrentMetadataJobs");
  dataCurrentMetadataJobs->Init( NS_LITERAL_STRING("songbird.backscan.concurrent"), NS_LITERAL_STRING("") );
  dataCurrentMetadataJobs->SetIntValue( 0 );

  rv = InitCurrentTasks();
}

sbMetadataJobManager::~sbMetadataJobManager()
{
  MOZ_COUNT_DTOR(sbMetadataJobManager);
}

/* sbIMetadataJob newJob (in nsIArray aMediaItemsArray); */
NS_IMETHODIMP 
sbMetadataJobManager::NewJob(nsIArray *aMediaItemsArray, PRUint32 aSleepMS, sbIMetadataJob **_retval)
{
  nsresult rv;
  nsCOMPtr<sbIMetadataJob> task = do_CreateInstance("@songbirdnest.com/Songbird/MetadataJob;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create a resource guid for this job.
  nsCOMPtr<nsIUUIDGenerator> uuidGen =
    do_GetService("@mozilla.org/uuid-generator;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);
  nsID id;
  rv = uuidGen->GenerateUUIDInPlace(&id);
  NS_ENSURE_SUCCESS(rv, rv);
  nsCAutoString guidUtf8(id.ToString());
  nsAutoString fullGuid;
  fullGuid = NS_ConvertUTF8toUTF16(guidUtf8);
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
  rv = insert->ToString( insertItem );
  NS_ENSURE_SUCCESS(rv, rv);
  rv = ExecuteQuery( insertItem );      
  NS_ENSURE_SUCCESS(rv, rv);

  // Kick off the task with the proper data
  rv = task->Init(tableName, aMediaItemsArray, aSleepMS);
  NS_ENSURE_SUCCESS(rv, rv);
  mJobArray.AppendObject( task );
  NS_ADDREF(*_retval = task);
  return NS_OK;
}


NS_IMETHODIMP sbMetadataJobManager::Stop()
{
  for ( PRUint32 i = 0; i < mJobArray.Count(); i++ )
  {
    mJobArray[ i ]->Cancel();
  }
  return NS_OK;
}

sbMetadataJobManager * sbMetadataJobManager::GetSingleton()
{
  if (gMetadataJobManager) {
    NS_ADDREF(gMetadataJobManager);
    return gMetadataJobManager;
  }

  NS_NEWXPCOM(gMetadataJobManager, sbMetadataJobManager);
  if (!gMetadataJobManager)
    return nsnull;

  //One of these addref's is for the global instance we use.
  NS_ADDREF(gMetadataJobManager);
  //This one is for the interface.
  NS_ADDREF(gMetadataJobManager);

  return gMetadataJobManager;
}

// nsIObserver
NS_IMETHODIMP
sbMetadataJobManager::Observe(nsISupports *aSubject, const char *aTopic,
                               const PRUnichar *aData)
{
  if(!strcmp(aTopic, NS_PROFILE_SHUTDOWN_OBSERVER_ID)) {
    nsresult rv;

    PR_LOG(gMetadataLog, PR_LOG_DEBUG, ("Metadata Job Manager shutting down..."));

    Stop();

    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    observerService->RemoveObserver(this, NS_PROFILE_SHUTDOWN_OBSERVER_ID);
  }
  return NS_OK;
}

nsresult sbMetadataJobManager::InitCurrentTasks()
{
  nsresult rv;

  // Compose the create table query string
  nsAutoString createTable;
  createTable.AppendLiteral( "CREATE TABLE " );
  createTable += sbMetadataJob::DATABASE_GUID();
  createTable.AppendLiteral( " (job_guid TEXT NOT NULL PRIMARY KEY, ms_delay INTEGER NOT NULL)" );
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
    nsAutoString tableName, strSleep;
    rv = result->GetRowCell(i, 0, tableName);
    rv = result->GetRowCell(i, 1, strSleep);
    NS_ENSURE_SUCCESS(rv, rv);
    PRUint32 aSleep = strSleep.ToInteger(&rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<sbIMetadataJob> task = do_CreateInstance("@songbirdnest.com/Songbird/MetadataJob;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = task->Init(tableName, nsnull, aSleep);
    NS_ENSURE_SUCCESS(rv, rv);

    mJobArray.AppendObject( task ); // Keep a reference around to it.
  }

  return rv;
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

