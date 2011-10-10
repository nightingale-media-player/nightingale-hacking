/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

#include "sbWatchFolderPrefMgr.h"

#include "sbWatchFolderDefines.h"
#include "sbWatchFolderService.h"
#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIObserverService.h>


NS_IMPL_ISUPPORTS1(sbWatchFolderPrefMgr, nsIObserver)

sbWatchFolderPrefMgr::sbWatchFolderPrefMgr()
{
}

sbWatchFolderPrefMgr::~sbWatchFolderPrefMgr()
{
}

nsresult
sbWatchFolderPrefMgr::Init(sbWatchFolderService *aWFService)
{
  NS_ENSURE_ARG_POINTER(aWFService);

  mWatchFolderService = aWFService;

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    "final-ui-startup",
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    "quit-application-granted",
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbWatchFolderPrefMgr::GetIsUnitTestsRunning(PRBool *aOutIsRunning)
{
  NS_ENSURE_ARG_POINTER(aOutIsRunning);
  *aOutIsRunning = PR_FALSE;

  nsresult rv;
  nsCOMPtr<nsIPrefBranch2> prefBranch =
    do_GetService("@mozilla.org/preferences-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = prefBranch->GetBoolPref("nightingale.__testmode__", aOutIsRunning);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbWatchFolderPrefMgr::OnPrefChanged(const nsAString & aPrefName,
                                    nsIPrefBranch2 *aPrefBranch)
{
  LOG(("%s: aPrefName = %s",
        __FUNCTION__,
        NS_ConvertUTF16toUTF8(aPrefName).get()));

  NS_ENSURE_ARG_POINTER(aPrefBranch);

  nsresult rv;

  // Should enable watch folders pref.
  if (aPrefName.EqualsLiteral(PREF_WATCHFOLDER_ENABLE)) {
    PRBool shouldEnable;
    rv = aPrefBranch->GetBoolPref(PREF_WATCHFOLDER_ENABLE, &shouldEnable);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mWatchFolderService->OnEnableWatchFolderChanged(shouldEnable);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  // Watch folder path
  else if (aPrefName.EqualsLiteral(PREF_WATCHFOLDER_PATH)) {
    nsCOMPtr<nsISupportsString> supportsVal;
    rv = aPrefBranch->GetComplexValue(PREF_WATCHFOLDER_PATH,
                                      NS_GET_IID(nsISupportsString),
                                      getter_AddRefs(supportsVal));
    NS_ENSURE_SUCCESS(rv, rv);

    nsString newWatchPath;
    rv = supportsVal->GetData(newWatchPath);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mWatchFolderService->OnWatchFolderPathChanged(newWatchPath);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    TRACE(("%s: Pref change not handled!", __FUNCTION__));
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbWatchFolderPrefMgr::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);

  nsresult rv;

  // Final UI startup:
  if (strcmp("final-ui-startup", aTopic) == 0) {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);

    // Now is the time to start listening to the pref branch.
    nsCOMPtr<nsIPrefBranch2> prefBranch =
      do_GetService("@mozilla.org/preferences-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = prefBranch->AddObserver(PREF_WATCHFOLDER_ROOT, this, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mWatchFolderService->OnAppStartup();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  // Library shutdown topic:
  else if (strcmp("quit-application-granted", aTopic) == 0) {
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, aTopic);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = mWatchFolderService->OnAppShutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  // Pref changed topic:
  else if (strcmp(NS_PREFBRANCH_PREFCHANGE_TOPIC_ID, aTopic) == 0) {
    nsCOMPtr<nsIPrefBranch2> prefBranch = do_QueryInterface(aSubject, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = OnPrefChanged(nsDependentString(aData), prefBranch);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

