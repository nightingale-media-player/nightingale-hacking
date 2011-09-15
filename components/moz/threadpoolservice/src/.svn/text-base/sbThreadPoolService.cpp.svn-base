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
 * \file sbThreadPoolService.cpp
 * \brief Songbird Thread Pool Service.
 */
 

#include "sbThreadPoolService.h"

#include <nsIAppStartupNotifier.h>
#include <nsIObserverService.h>

#include <nsComponentManagerUtils.h>
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOM.h>

#include <prlog.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbThreadPoolService:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gThreadPoolServiceLog = nsnull;
#define TRACE(args) PR_LOG(gThreadPoolServiceLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gThreadPoolServiceLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

#define DEFAULT_THREAD_LIMIT    (8)
#define DEFAULT_IDLE_LIMIT      (1)
#define DEFAULT_IDLE_TIMEOUT    (10000) /* in milliseconds */

#define NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID "xpcom-shutdown-threads" 

NS_IMPL_THREADSAFE_ISUPPORTS3(sbThreadPoolService, 
                              nsIEventTarget,
                              nsIThreadPool, 
                              nsIObserver)

sbThreadPoolService::sbThreadPoolService()
{
#ifdef PR_LOGGING
  if (!gThreadPoolServiceLog) {
    gThreadPoolServiceLog = PR_NewLogModule("sbThreadPoolService");
  }

  TRACE(("sbThreadPoolService[0x%x] - ctor", this));
#endif
}

sbThreadPoolService::~sbThreadPoolService() 
{
  TRACE(("sbThreadPoolService[0x%x] - dtor", this));
}

nsresult
sbThreadPoolService::Init()
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  mThreadPool = do_CreateInstance("@mozilla.org/thread-pool;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetThreadLimit(DEFAULT_THREAD_LIMIT);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetIdleThreadLimit(DEFAULT_IDLE_LIMIT);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mThreadPool->SetIdleThreadTimeout(DEFAULT_IDLE_TIMEOUT);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


NS_IMETHODIMP 
sbThreadPoolService::Observe(nsISupports *aSubject,
                             const char *aTopic,
                             const PRUnichar *aData)
{
  LOG(("sbThreadPoolService[0x%x] - Observe (%s)", this, aTopic));
  
  nsresult rv = NS_ERROR_UNEXPECTED;

  if (!strcmp(aTopic, NS_XPCOM_STARTUP_OBSERVER_ID)) {
    nsresult rv = NS_ERROR_UNEXPECTED;

    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->AddObserver(observer, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if(!strcmp(aTopic, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID)) {
    nsCOMPtr<nsIObserverService> obsSvc =
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIObserver> observer =
      do_QueryInterface(NS_ISUPPORTS_CAST(nsIObserver*, this), &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = obsSvc->RemoveObserver(observer, NS_XPCOM_SHUTDOWN_THREADS_OBSERVER_ID);
    NS_ENSURE_SUCCESS(rv, rv);

    // All further attempts to dispatch runnables will fail gracefully
    // after Shutdown is called on the underlying threadpool.
    nsresult rv = mThreadPool->Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
