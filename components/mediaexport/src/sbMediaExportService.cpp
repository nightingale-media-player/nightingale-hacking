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

#include "sbMediaExportService.h"

#include "sbMediaExportDefines.h"
#include <nsComponentManagerUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsICategoryManager.h>
#include <prlog.h>
#include <sbILibraryManager.h>
#include <nsIObserverService.h>
#include <sbIPropertyArray.h>
#include <sbPropertiesCID.h>
#include <sbStandardProperties.h>

#ifdef PR_LOGGING
PRLogModuleInfo *gMediaExportLog = nsnull;
#endif


NS_IMPL_ISUPPORTS2(sbMediaExportService, nsIObserver, sbIMediaListListener)

sbMediaExportService::sbMediaExportService()
  : mIsRunning(PR_FALSE)
{
#ifdef PR_LOGGING
   if (!gMediaExportLog) {
     gMediaExportLog = PR_NewLogModule("sbMediaExportService");
   }
#endif
}

sbMediaExportService::~sbMediaExportService()
{
}

nsresult
sbMediaExportService::Init()
{
  LOG(("%s: Starting the export service", __FUNCTION__));

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_READY_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this,
                                    SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC,
                                    PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Create and start the pref controller
  mPrefController = new sbMediaExportPrefController();
  NS_ENSURE_TRUE(mPrefController, NS_ERROR_OUT_OF_MEMORY);

  rv = mPrefController->Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediaExportService::InitInternal()
{
  nsresult rv;
  // No need to listen for the library manager ready notice
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_READY_TOPIC);
  NS_ENSURE_SUCCESS(rv, rv);

  // Don't bother starting any listeners when the service should not run.
  if (!mPrefController->GetShouldExportTracks() &&
      !mPrefController->GetShouldExportPlaylists() && 
      !mPrefController->GetShouldExportSmartPlaylists())
  {
    return NS_OK;
  }

  mIsRunning = PR_TRUE;
  
  // At least one item needs to be exported, setup the library listener.
  nsCOMPtr<sbILibraryManager> libraryMgr = 
    do_GetService("@songbirdnest.com/Songbird/library/Manager;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryMgr->GetMainLibrary(getter_AddRefs(mMainLibrary));
  NS_ENSURE_SUCCESS(rv, rv);

  // Filter out the properties that are actually needed for export.
  nsCOMPtr<sbIMutablePropertyArray> propArray =
    do_CreateInstance(SB_MUTABLEPROPERTYARRAY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = propArray->SetStrict(PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  // Content URL
  rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_CONTENTURL),
                                 EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // GUID
  rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_GUID),
                                 EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  // Playlist Name
  rv = propArray->AppendProperty(NS_LITERAL_STRING(SB_PROPERTY_MEDIALISTNAME),
                                 EmptyString());
  NS_ENSURE_SUCCESS(rv, rv);

  PRUint32 flags = sbIMediaList::LISTENER_FLAGS_ITEMADDED |
                   sbIMediaList::LISTENER_FLAGS_AFTERITEMREMOVED |
                   sbIMediaList::LISTENER_FLAGS_ITEMUPDATED |
                   sbIMediaList::LISTENER_FLAGS_LISTCLEARED;
  
  rv = mMainLibrary->AddListener(this, PR_FALSE, flags, propArray);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
sbMediaExportService::Shutdown()
{
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->RemoveObserver(this, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mPrefController->Shutdown();
  NS_ENSURE_SUCCESS(rv, rv);

  if (mIsRunning) {
    rv = mMainLibrary->RemoveListener(this);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// nsIObserver

NS_IMETHODIMP
sbMediaExportService::Observe(nsISupports *aSubject,
                              const char *aTopic,
                              const PRUnichar *aData)
{
  nsresult rv;
  if (strcmp(aTopic, SB_LIBRARY_MANAGER_READY_TOPIC) == 0) {
    rv = InitInternal();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else if (strcmp(aTopic, SB_LIBRARY_MANAGER_SHUTDOWN_TOPIC) == 0) {
    rv = Shutdown();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbIMediaListListener

NS_IMETHODIMP 
sbMediaExportService::OnItemAdded(sbIMediaList *aMediaList, 
                                  sbIMediaItem *aMediaItem, 
                                  PRUint32 aIndex, 
                                  PRBool *aRetVal)
{
  LOG(("%s: Media Item Added!", __FUNCTION__));

  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnBeforeItemRemoved(sbIMediaList *aMediaList, 
                                          sbIMediaItem *aMediaItem, 
                                          PRUint32 aIndex, 
                                          PRBool *aRetVal)
{
  LOG(("%s: Before Media Item Removed!", __FUNCTION__));  

  
  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnAfterItemRemoved(sbIMediaList *aMediaList, 
                                         sbIMediaItem *aMediaItem, 
                                         PRUint32 aIndex, PRBool *_retval)
{
  LOG(("%s: After Media Item Removed!!", __FUNCTION__));  
  
  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnItemUpdated(sbIMediaList *aMediaList, 
                                    sbIMediaItem *aMediaItem, 
                                    sbIPropertyArray *aProperties, 
                                    PRBool *aRetVal)
{
  LOG(("%s: Media Item Updated!!", __FUNCTION__));  
  
  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnItemMoved(sbIMediaList *aMediaList, 
                                  PRUint32 aFromIndex, 
                                  PRUint32 aToIndex, 
                                  PRBool *aRetVal)
{
  LOG(("%s: Media Item Moved!", __FUNCTION__));

  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnListCleared(sbIMediaList *aMediaList, 
                                    PRBool *aRetVal)
{
  LOG(("%s: Media List Cleared!", __FUNCTION__));

  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnBatchBegin(sbIMediaList *aMediaList)
{
  LOG(("%s: Media List Batch Begin!", __FUNCTION__));

  return NS_OK;
}

NS_IMETHODIMP 
sbMediaExportService::OnBatchEnd(sbIMediaList *aMediaList)
{
  LOG(("%s: Media List Batch End!", __FUNCTION__));

  return NS_OK;
}

//------------------------------------------------------------------------------
// XPCOM Startup Registration

/* static */ NS_METHOD
sbMediaExportService::RegisterSelf(nsIComponentManager *aCompMgr,
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
                                SONGBIRD_MEDIAEXPORTSERVICE_CLASSNAME,
                                "service,"
                                SONGBIRD_MEDIAEXPORTSERVICE_CONTRACTID,
                                PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}
