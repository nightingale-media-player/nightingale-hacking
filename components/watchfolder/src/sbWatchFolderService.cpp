/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

#include "sbWatchFolderService.h"

#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <nsIObserverService.h>
#include <nsStringAPI.h>
#include "sbWatchFolderServiceCID.h"


NS_IMPL_ISUPPORTS4(sbWatchFolderService, 
                   sbIWatchFolderService, 
                   sbIFileSystemListener,
                   nsITimerCallback,
                   nsIObserver)

sbWatchFolderService::sbWatchFolderService()
{
}

sbWatchFolderService::~sbWatchFolderService()
{
}

nsresult
sbWatchFolderService::Init()
{
  nsresult rv;


  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "profile-after-change", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, "quit-application", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbWatchFolderService::StartWatching()
{
  //
  // TODO:
  //  1.) Check to see if the service should be run
  //  2/) Find out what the saved watch folder path is
  //

  // For now, just use my hardcoded path
  nsString path(NS_LITERAL_STRING("/Users/nickkreeger/Music/WFTest"));

  return NS_OK;
}

nsresult
sbWatchFolderService::StopWatching()
{
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbWatchFolderService

NS_IMETHODIMP
sbWatchFolderService::GetIsSupported(PRBool *aIsSupported)
{
  NS_ENSURE_ARG_POINTER(aIsSupported);
  // For now...
  *aIsSupported = PR_TRUE;
  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIFileSystemListener

NS_IMETHODIMP
sbWatchFolderService::OnWatcherStarted()
{
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnWatcherStopped()
{
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemChanged(const nsAString & aFilePath)
{
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemRemoved(const nsAString & aFilePath)
{
  return NS_OK;
}

NS_IMETHODIMP 
sbWatchFolderService::OnFileSystemAdded(const nsAString & aFilePath)
{
  return NS_OK;
}

//------------------------------------------------------------------------------
// nsITimerCallback

NS_IMETHODIMP
sbWatchFolderService::Notify(nsITimer *aTimer)
{
  NS_ENSURE_ARG_POINTER(aTimer);

  // TODO: Clear out change queue.

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbWatchFolderService::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
  NS_ENSURE_ARG_POINTER(aTopic);
  
  if (strcmp("profile-after-change", aTopic) == 0) {
  
  }
  else if (strcmp("quit-application", aTopic) == 0) {

    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService =
      do_GetService("@mozilla.org/observer-service;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, "profile-after-change");
    NS_ENSURE_SUCCESS(rv, rv);

    rv = observerService->RemoveObserver(this, "quit-application");
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  return NS_OK;
}

//------------------------------------------------------------------------------
// XPCOM Startup Registration

/* static */ NS_METHOD
sbWatchFolderService::RegisterSelf(nsIComponentManager *aCompMgr,
                                   nsIFile *aPath,
                                   const char *aLoaderStr,
                                   const char *aType,
                                   const nsModuleComponentInfo *aInfo)
{
  NS_ENSURE_ARG_POINTER(aCompMgr);
  NS_ENSURE_ARG_POINTER(aPath);
  NS_ENSURE_ARG_POINTER(aLoaderStr);
  NS_ENSURE_ARG_POINTER(aType);
  NS_ENSURE_ARG_POINTER(aInfo);

  nsresult rv = NS_ERROR_UNEXPECTED;
  nsCOMPtr<nsICategoryManager> catMgr = 
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catMgr->AddCategoryEntry("app-startup",
                                SONGBIRD_WATCHFOLDERSERVICE_CLASSNAME,
                                "service,"
                                SONGBIRD_WATCHFOLDERSERVICE_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

