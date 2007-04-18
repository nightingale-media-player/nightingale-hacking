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
#include <nsAutoLock.h>

#include <unicharutil/nsUnicharUtils.h>
#include <nsComponentManagerUtils.h>
#include <xpcom/nsServiceManagerUtils.h>
#include <xpcom/nsIObserverService.h>
#include <xpcom/nsCRTGlue.h>
#include <pref/nsIPrefService.h>
#include <pref/nsIPrefBranch2.h>
#include <nsISupportsPrimitives.h>
#include <nsThreadUtils.h>
#include <nsMemory.h>

#include "MetadataJobManager.h"

#include "prlog.h"
#ifdef PR_LOGGING
extern PRLogModuleInfo* gMetadataLog;
#endif

// DEFINES ====================================================================

// GLOBALS ====================================================================
sbMetadataJobManager *gMetadataJobManager = nsnull;

// CLASSES ====================================================================
NS_IMPL_THREADSAFE_ISUPPORTS2(sbMetadataJobManager, sbIMetadataJobManager, nsIObserver)

sbMetadataJobManager::sbMetadataJobManager()
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

  mQuery = do_CreateInstance("@songbirdnest.com/Songbird/DatabaseQuery;1");
  NS_ASSERTION(mQuery, "Unable to create sbMetadataJobManager::mQuery");

  // TODO (maybe on some startup event?):
  //  - Create the database
  //  - Create the task tracking table
  //  - Read and initialize any tasks in the tracking table
}

sbMetadataJobManager::~sbMetadataJobManager()
{
  Stop();
}

/* sbIMetadataJob newJob (in nsIArray aMediaItemsArray); */
NS_IMETHODIMP 
sbMetadataJobManager::NewJob(nsIArray *aMediaItemsArray, PRUint32 aSleepMS, sbIMetadataJob **_retval)
{
  nsresult rv;
  nsCOMPtr<sbIMetadataJob> task = do_CreateInstance("@songbirdnest.com/Songbird/MetadataJob;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO:
  //  - Create a guid
  //  - Add the task guid to the tracking table
  //  - Initialize the task

  NS_ADDREF(*_retval = task);
  return NS_OK;
}


void sbMetadataJobManager::Stop()
{
  // TODO:
  //  - Cancel all outstanding tasks
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
  if(!strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    nsresult rv;

    PR_LOG(gMetadataLog, PR_LOG_DEBUG, ("Metadata Job Manager shutting down..."));

    Stop();

    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);
  }
  return NS_OK;
}


