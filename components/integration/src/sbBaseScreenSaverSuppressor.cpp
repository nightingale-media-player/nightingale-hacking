/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

#include "sbBaseScreenSaverSuppressor.h"

#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <mozilla/ModuleUtils.h>
#include <nsIObserverService.h>

#include <nsServiceManagerUtils.h>

#include <prlog.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbBaseScreenSaverSuppressor:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gBaseScreenSaverSuppressor = nsnull;
#define TRACE(args) PR_LOG(gBaseScreenSaverSuppressor , PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gBaseScreenSaverSuppressor , PR_LOG_WARN, args)
#ifdef __GNUC__
#define __FUNCTION__ __PRETTY_FUNCTION__
#endif /* __GNUC__ */
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

NS_IMPL_ISUPPORTS1(sbBaseScreenSaverSuppressor, 
                   sbIScreenSaverSuppressor)

sbBaseScreenSaverSuppressor::sbBaseScreenSaverSuppressor()
: mSuppress(0)
, mUserSetting(PR_FALSE)
, mHasUserSetting(PR_FALSE)
{
  #ifdef PR_LOGGING
    if (!gBaseScreenSaverSuppressor)
      gBaseScreenSaverSuppressor =
        PR_NewLogModule("sbBaseScreenSaverSuppressor");
  #endif
}

sbBaseScreenSaverSuppressor::~sbBaseScreenSaverSuppressor() 
{
}

/*static*/ nsresult 
sbBaseScreenSaverSuppressor::RegisterSelf(nsIComponentManager* aCompMgr,
                                          nsIFile* aPath,
                                          const char* aLoaderStr,
                                          const char* aType,
                                          const nsModuleComponentInfo *aInfo)
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->AddCategoryEntry(APPSTARTUP_CATEGORY,
                                         SB_BASE_SCREEN_SAVER_SUPPRESSOR_DESC,
                                         "service,"
                                         SB_BASE_SCREEN_SAVER_SUPPRESSOR_CONTRACTID,
                                         PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbBaseScreenSaverSuppressor::Init() 
{
  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "xpcom-shutdown", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//-----------------------------------------------------------------------------
// sbIScreenSaverSuppressor
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbBaseScreenSaverSuppressor::Suppress(PRBool aSuppress, PRInt32 *aSuppressionCount)
{
  TRACE(("[sbBaseScreenSaverSuppressor] - Suppress"));

  nsresult rv = NS_ERROR_UNEXPECTED;
  PRBool shouldCallOnSuppress = PR_FALSE;

  if(aSuppress) {
    if(mSuppress == 0) {
      shouldCallOnSuppress = PR_TRUE;
    }

    // Yes, this may overflow if you call it more than 4 billion times.
    // Please don't call this method more than 4 billion times. Thanks.
    mSuppress++;
  }
  else {
    if(mSuppress > 0) {
      mSuppress--;
    }
    if(mSuppress == 0) {
      shouldCallOnSuppress = PR_TRUE;
    }
  }

  if(shouldCallOnSuppress) {
    rv = OnSuppress(aSuppress);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("[sbBaseScreenSaverSuppressor] - OnSuppress Succeeded - Suppressed? %s", 
         aSuppress ? "true" : "false"));
  }

  *aSuppressionCount = mSuppress;

  return NS_OK;
}


//-----------------------------------------------------------------------------
// nsIObserver
//-----------------------------------------------------------------------------

NS_IMETHODIMP
sbBaseScreenSaverSuppressor::Observe(nsISupports* aSubject,
                                     const char* aTopic,
                                     const PRUnichar* aData)
{
  if(!strcmp(aTopic, "xpcom-shutdown")) {
    
    TRACE(("[sbBaseScreenSaverSuppressor] - Observe (%s)", aTopic));

    nsresult rv = NS_ERROR_UNEXPECTED;
    nsCOMPtr<nsIObserverService> observerService = 
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), 
      "Failed to get observer service, observer will not removed");

    if(NS_SUCCEEDED(rv)) {
      rv = observerService->RemoveObserver(this, "xpcom-shutdown");
      NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to remove observer");
    }

    rv = OnSuppress(PR_FALSE);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "Failed to unsuppress screen saver!");
  }

  return NS_OK;
}
