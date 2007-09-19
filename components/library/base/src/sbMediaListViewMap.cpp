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
NS_IMETHODIMP sbMediaListViewMap::GetView(nsISupports *aParentKey, nsISupports *aPageKey, sbIMediaListView **_retval)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER( aParentKey );
  NS_ENSURE_ARG_POINTER( aPageKey );
  NS_ENSURE_ARG_POINTER( _retval );
  nsInterfaceHashtableMT< nsISupportsHashKey, sbIMediaListView > *innerMap = nsnull;
  sbIMediaListView *innerValue = nsnull;
  *_retval = nsnull;
  if ( mViewMap.Get( aParentKey, &innerMap ) && innerMap && innerMap->Get( aPageKey, &innerValue ) )
  {
    NS_IF_ADDREF( *_retval = innerValue );
  }
  // It's okay to return null on failure.
  return NS_OK;
}

/* void setView (in nsISupports aParentKey, in nsISupports aPageKey, in sbIMediaListView aView); */
NS_IMETHODIMP sbMediaListViewMap::SetView(nsISupports *aParentKey, nsISupports *aPageKey, sbIMediaListView *aView)
{
  return NS_OK;
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER( aParentKey );
  NS_ENSURE_ARG_POINTER( aPageKey );
  NS_ENSURE_ARG_POINTER( aView );

  PRBool success;
  nsInterfaceHashtableMT< nsISupportsHashKey, sbIMediaListView > *innerMap = nsnull;
  sbIMediaListView *innerValue = nsnull;
  // If our inner map does not yet exist, create and stash it
  if ( ! mViewMap.Get( aParentKey, &innerMap ) )
  {
    innerMap = new nsInterfaceHashtableMT< nsISupportsHashKey, sbIMediaListView >;
    NS_ENSURE_TRUE(innerMap, NS_ERROR_OUT_OF_MEMORY);
    success = innerMap->Init();
    NS_ENSURE_TRUE(success, NS_ERROR_OUT_OF_MEMORY);
    success = mViewMap.Put( aParentKey, innerMap ); // Takes over memory management for innerMap
    NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  }

  // Put the view into the inner map
  success = innerMap->Put( aPageKey, aView );
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);

  // And then into the reverse lookup maps
  success = mParentLookup.Put( aView, aParentKey );
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  success = mPageLookup.Put( aView, aPageKey );
  NS_ENSURE_TRUE(success, NS_ERROR_FAILURE);
  return NS_OK;
}

/* void getContext (in sbIMediaListView aView, out nsISupports aParentKey, out nsISupports aPageKey); */
NS_IMETHODIMP sbMediaListViewMap::GetContext(sbIMediaListView *aView, nsISupports **aParentKey, nsISupports **aPageKey)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  NS_ENSURE_ARG_POINTER( aParentKey );
  NS_ENSURE_ARG_POINTER( aPageKey );
  NS_ENSURE_ARG_POINTER( aView );

  if ( ! mParentLookup.Get( aView, aParentKey ) )
    *aParentKey = nsnull;
  if ( ! mPageLookup.Get( aView, aPageKey ) )
    *aPageKey = nsnull;

  return NS_OK;
}

PLDHashOperator PR_CALLBACK
sbMediaListViewMap::ReleaseLookups( nsISupportsHashKey::KeyType aKey,
                                    sbIMediaListView* aEntry,
                                    void* aUserData )
{
  // Okay, we're done with this view.  Nuke it from the reverse lookup maps.
  sbMediaListViewMap *viewMap = reinterpret_cast< sbMediaListViewMap * >( aUserData );
  viewMap->mParentLookup.Remove( aEntry );
  viewMap->mPageLookup.Remove( aEntry );
  return PL_DHASH_NEXT;
}

/* void releaseViews (in nsISupports aParentKey); */
NS_IMETHODIMP sbMediaListViewMap::ReleaseViews(nsISupports *aParentKey)
{
  NS_ASSERTION(NS_IsMainThread(), "Wrong thread!");
  if ( ! aParentKey ) // If NULL, release all.
  {
    // Supposedly, this will release everything.  If it doesn't, blame Ben!
    mParentLookup.Clear();
    mPageLookup.Clear();
    mViewMap.Clear(); 
  }
  else
  {
    nsInterfaceHashtableMT< nsISupportsHashKey, sbIMediaListView > *innerMap = nsnull;
    if ( mViewMap.Get( aParentKey, &innerMap ) )
    {
      // First we must release the reverse lookup entries
      innerMap->Enumerate( (nsInterfaceHashtableMT< nsISupportsHashKey, sbIMediaListView >::EnumFunction)ReleaseLookups, this );

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
    mParentLookup.Init();
    mPageLookup.Init();

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
