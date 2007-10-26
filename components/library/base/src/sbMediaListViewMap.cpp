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
* \file  sbMediaListViewMap.cpp
* \brief Songbird MediaListViewMap Implementation.
*/
#include "sbMediaListViewMap.h"

#include <sbILibraryManager.h>

#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsIObserver.h>
#include <nsIObserverService.h>
#include <nsISimpleEnumerator.h>
#include <nsISupportsPrimitives.h>

#include <nsArrayEnumerator.h>
#include <nsAutoLock.h>
#include <nsCOMArray.h>
#include <nsComponentManagerUtils.h>
#include <nsEnumeratorUtils.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>
#include <prlog.h>
#include <rdf.h>

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbMediaListViewMap:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* gMediaListViewMapLog = nsnull;
#define TRACE(args) PR_LOG(gMediaListViewMapLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(gMediaListViewMapLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

#define NS_PROFILE_STARTUP_OBSERVER_ID  "profile-after-change"
#define NS_PROFILE_SHUTDOWN_OBSERVER_ID "profile-before-change"

NS_IMPL_THREADSAFE_ISUPPORTS3(sbMediaListViewMap, nsIObserver,
                                                  nsISupportsWeakReference,
                                                  sbIMediaListViewMap)

sbMediaListViewMap::sbMediaListViewMap()
{
  MOZ_COUNT_CTOR(sbMediaListViewMap);
#ifdef PR_LOGGING
  if (!gMediaListViewMapLog)
    gMediaListViewMapLog = PR_NewLogModule("sbMediaListViewMap");
#endif

  TRACE(("sbMediaListViewMap[0x%x] - Created", this));

  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
}

sbMediaListViewMap::~sbMediaListViewMap()
{
  TRACE(("MediaListViewMap[0x%x] - Destroyed", this));
  MOZ_COUNT_DTOR(sbMediaListViewMap);
}

/* static */ NS_METHOD
sbMediaListViewMap::RegisterSelf(nsIComponentManager* aCompMgr,
                               nsIFile* aPath,
                               const char* aLoaderStr,
                               const char* aType,
                               const nsModuleComponentInfo *aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->AddCategoryEntry(APPSTARTUP_CATEGORY,
                                         SONGBIRD_MEDIALISTVIEWMAP_DESCRIPTION,
                                         "service," SONGBIRD_MEDIALISTVIEWMAP_CONTRACTID,
                                         PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

/**
 * \brief Register with the Observer Service.
 */
nsresult
sbMediaListViewMap::Init()
{
  TRACE(("sbMediaListViewMap[0x%x] - Init", this));
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, NS_PROFILE_STARTUP_OBSERVER_ID,
                                    PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, NS_PROFILE_SHUTDOWN_OBSERVER_ID,
                                    PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/* sbIMediaListView getView (in nsISupports aParentKey, in nsISupports aPageKey); */
NS_IMETHODIMP
sbMediaListViewMap::GetView(nsISupports *aParentKey,
                            nsISupports *aPageKey,
                            sbIMediaListView **_retval)
{
  TRACE(("sbMediaListViewMap[0x%x] - GetView", this));

  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER( aParentKey );
  NS_ENSURE_ARG_POINTER( aPageKey );
  NS_ENSURE_ARG_POINTER( _retval );
  sbViewMapInner *innerMap = nsnull;
  sbViewStateInfo *innerValue = nsnull;
  *_retval = nsnull;
  if ( mViewMap.Get( aParentKey, &innerMap ) && innerMap && innerMap->Get( aPageKey, &innerValue ) )
  {
#ifdef DEBUG
    {
      nsString buff;
      innerValue->state->ToString(buff);
      TRACE(("sbMediaListViewMap[0x%x] - Restoring view %s %s %s", this,
             NS_LossyConvertUTF16toASCII(innerValue->libraryGuid).get(),
             NS_LossyConvertUTF16toASCII(innerValue->libraryGuid).get(),
             NS_LossyConvertUTF16toASCII(buff).get()));
    }
#endif

    nsresult rv;
    nsCOMPtr<sbILibraryManager> lm =
        do_GetService( "@songbirdnest.com/Songbird/library/Manager;1", &rv );
    NS_ENSURE_SUCCESS( rv, rv );

    nsCOMPtr<sbILibrary> library;
    rv = lm->GetLibrary( innerValue->libraryGuid, getter_AddRefs(library) );
    if ( rv == NS_ERROR_NOT_AVAILABLE ) {
      return NS_OK;
    }
    NS_ENSURE_SUCCESS( rv, rv );

    nsCOMPtr<sbIMediaItem> item;
    rv = library->GetItemByGuid( innerValue->listGuid, getter_AddRefs(item) );
    if ( rv == NS_ERROR_NOT_AVAILABLE ) {
      return NS_OK;
    }
    NS_ENSURE_SUCCESS( rv, rv );

    nsCOMPtr<sbIMediaList> list = do_QueryInterface(item, &rv);
    NS_ENSURE_SUCCESS( rv, rv );

    rv = list->CreateView(innerValue->state, _retval);
    NS_ENSURE_SUCCESS( rv, rv );
  }

  // It's okay to return null on failure.
  return NS_OK;
}

/* void setView (in nsISupports aParentKey, in nsISupports aPageKey, in sbIMediaListView aView); */
NS_IMETHODIMP
sbMediaListViewMap::SetView(nsISupports *aParentKey,
                            nsISupports *aPageKey,
                            sbIMediaListView *aView)
{
  TRACE(("sbMediaListViewMap[0x%x] - SetView", this));

  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER( aParentKey );
  NS_ENSURE_ARG_POINTER( aPageKey );
  NS_ENSURE_ARG_POINTER( aView );

  nsresult rv;
  PRBool success;
  sbViewMapInner *innerMap = nsnull;

  // If our inner map does not yet exist, create and stash it
  if ( ! mViewMap.Get( aParentKey, &innerMap ) )
  {
    innerMap = new sbViewMapInner;
    NS_ENSURE_TRUE(innerMap, NS_ERROR_OUT_OF_MEMORY);
    success = innerMap->Init();
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    success = mViewMap.Put( aParentKey, innerMap ); // Takes over memory management for innerMap
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  }

  nsCOMPtr<sbIMediaList> list;
  nsString listGuid;
  rv = aView->GetMediaList(getter_AddRefs(list));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = list->GetGuid(listGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  nsString libraryGuid;
  rv = list->GetLibrary(getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);
  rv = library->GetGuid(libraryGuid);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbIMediaListViewState> state;
  rv = aView->GetState(getter_AddRefs(state));
  NS_ENSURE_SUCCESS(rv, rv);

#ifdef DEBUG
  {
    nsString buff;
    state->ToString(buff);
    TRACE(("sbMediaListViewMap[0x%x] - Saving view %s %s %s", this,
           NS_LossyConvertUTF16toASCII(libraryGuid).get(),
           NS_LossyConvertUTF16toASCII(libraryGuid).get(),
           NS_LossyConvertUTF16toASCII(buff).get()));
  }
#endif

  nsAutoPtr<sbViewStateInfo> si(
    new sbViewStateInfo(libraryGuid, listGuid, state));
  NS_ENSURE_TRUE(si, NS_ERROR_OUT_OF_MEMORY);

  // Put the view into the inner map
  success = innerMap->Put( aPageKey, si );
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  si.forget();

  return NS_OK;
}

/* void releaseViews (in nsISupports aParentKey); */
NS_IMETHODIMP sbMediaListViewMap::ReleaseViews(nsISupports *aParentKey)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  if ( ! aParentKey ) // If NULL, release all.
  {
    // Supposedly, this will release everything.  If it doesn't, blame Ben!
    mViewMap.Clear(); 
  }
  else
  {
    sbViewMapInner *innerMap = nsnull;
    if ( mViewMap.Get( aParentKey, &innerMap ) )
    {
      // Then release the entire branch of the primary map.
      mViewMap.Remove( aParentKey ); // This line deletes the innerMap, automatically clearing it.  Supposedly.
    }
  }
  return NS_OK; // If it's not actually there, we don't care.
}

/**
 * See nsIObserver.idl
 */
NS_IMETHODIMP
sbMediaListViewMap::Observe(nsISupports* aSubject,
                          const char* aTopic,
                          const PRUnichar* aData)
{
  TRACE(("sbMediaListViewMap[0x%x] - Observe: %s", this, aTopic));

  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);

  if (strcmp(aTopic, APPSTARTUP_TOPIC) == 0) {
    return NS_OK; // ???
  }
  else if (strcmp(aTopic, NS_PROFILE_STARTUP_OBSERVER_ID) == 0) {

    // Remove ourselves from the observer service.
    if (NS_SUCCEEDED(rv)) {
      observerService->RemoveObserver(this, NS_PROFILE_STARTUP_OBSERVER_ID);
    }

    // Startup
    mViewMap.Init();

    return NS_OK;
  }
  else if (strcmp(aTopic, NS_PROFILE_SHUTDOWN_OBSERVER_ID) == 0) {

    // Remove ourselves from the observer service.
    if (NS_SUCCEEDED(rv)) {
      observerService->RemoveObserver(this, NS_PROFILE_SHUTDOWN_OBSERVER_ID);
    }

    // Shutdown
    ReleaseViews( nsnull );

    return NS_OK;
  }

  NS_NOTREACHED("Observing a topic that wasn't handled!");
  return NS_OK;
}
