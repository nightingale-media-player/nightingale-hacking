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

#include "sbLocalDatabaseLibraryLoader.h"

#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsIObserverService.h>
#include <nsILocalFile.h>
#include <nsIProperties.h>
#include <nsServiceManagerUtils.h>
#include <prlog.h>
#include <sbLibraryManager.h>
#include <sbILibrary.h>
#include <sbILocalDatabaseLibraryFactory.h>
#include "sbLocalDatabaseCID.h"
#include <nsComponentManagerUtils.h>

/*
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=sbLocalDatabaseLibraryLoader:5
 */
#ifdef PR_LOGGING
static PRLogModuleInfo* sLibraryLoaderLog = nsnull;
#define TRACE(args) PR_LOG(sLibraryLoaderLog, PR_LOG_DEBUG, args)
#define LOG(args)   PR_LOG(sLibraryLoaderLog, PR_LOG_WARN, args)
#else
#define TRACE(args) /* nothing */
#define LOG(args)   /* nothing */
#endif

#define NS_APPSTARTUP_CATEGORY         "app-startup"
#define NS_PROFILE_STARTUP_OBSERVER_ID "profile-after-change"

NS_IMPL_ISUPPORTS1(sbLocalDatabaseLibraryLoader, nsIObserver)

sbLocalDatabaseLibraryLoader::sbLocalDatabaseLibraryLoader()
{
#ifdef PR_LOGGING
  if (!sLibraryLoaderLog)
    sLibraryLoaderLog = PR_NewLogModule("sbLocalDatabaseLibraryLoader");
#endif
  TRACE(("sbLocalDatabaseLibraryLoader[0x%x] - Created", this));
}

sbLocalDatabaseLibraryLoader::~sbLocalDatabaseLibraryLoader()
{
  TRACE(("sbLocalDatabaseLibraryLoader[0x%x] - Destroyed", this));
}

/**
 * \brief Register this component with the Category Manager for instantiaition
 *        on startup.
 */
/* static */ nsresult
sbLocalDatabaseLibraryLoader::RegisterSelf(nsIComponentManager* aCompMgr,
                                           nsIFile* aPath,
                                           const char* aLoaderStr,
                                           const char* aType,
                                           const nsModuleComponentInfo *aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->AddCategoryEntry(NS_APPSTARTUP_CATEGORY,
                                         SB_LOCALDATABASE_LIBRARYLOADER_DESCRIPTION,
                                         SB_LOCALDATABASE_LIBRARYLOADER_CONTRACTID,
                                         PR_TRUE, PR_TRUE, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

/**
 * \brief Register with the Observer Service to keep the instance alive until
 *        a profile has been loaded.
 */
nsresult
sbLocalDatabaseLibraryLoader::Init()
{
  TRACE(("sbLocalDatabaseLibraryLoader[0x%x] - Init", this));

  // Register ourselves with the Observer Service. We use a strong ref so that
  // we stay alive long enough to catch profile-after-change.
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService = 
    do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->AddObserver(this, NS_PROFILE_STARTUP_OBSERVER_ID, PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * \brief Load all of the libraries we need for startup.
 */
nsresult
sbLocalDatabaseLibraryLoader::LoadLibraries()
{
  TRACE(("sbLocalDatabaseLibraryLoader[0x%x] - LoadLibraries", this));

  nsresult rv;
  nsCOMPtr<sbILibraryManager> libraryManager = 
    do_GetService(SONGBIRD_LIBRARYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // TODO: Figure out how we are going to do all of this fo reals

  nsCOMPtr<sbILocalDatabaseLibraryFactory> factory =
    do_CreateInstance(SB_LOCALDATABASE_LIBRARYFACTORY_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIProperties> ds =
    do_GetService(NS_DIRECTORY_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsILocalFile> file;
  rv = ds->Get("ProfD", NS_GET_IID(nsILocalFile), getter_AddRefs(file));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->AppendRelativePath(NS_LITERAL_STRING("db"));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = file->AppendRelativePath(NS_LITERAL_STRING("test_localdatabaselibrary.db"));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<sbILibrary> library;
  rv = factory->CreateLibraryFromDatabase(file, getter_AddRefs(library));
  NS_ENSURE_SUCCESS(rv, rv);

  rv = libraryManager->RegisterLibrary(library);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

/**
 * See nsIObserver.idl
 */
NS_IMETHODIMP
sbLocalDatabaseLibraryLoader::Observe(nsISupports* aSubject,
                                      const char* aTopic,
                                      const PRUnichar *aData)
{
  TRACE(("sbLocalDatabaseLibraryLoader[0x%x] - Observe: %s", this, aTopic));

  // XXXben The current implementation of the Observer Service keeps its
  //        observers alive during a NotifyObservers call. I can't find anywhere
  //        in the documentation that this is expected or guaranteed behavior
  //        so we're going to take an extra step here to ensure our own
  //        survival.
  nsCOMPtr<nsISupports> kungFuDeathGrip;

  if (strcmp(aTopic, NS_PROFILE_STARTUP_OBSERVER_ID) == 0) {

    // Protect ourselves from deletion until we're done. The Observer Service is
    // the only thing keeping us alive at the moment, so we need to bump our
    // ref count before removing ourselves.
    kungFuDeathGrip = this;

    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService = 
      do_GetService(NS_OBSERVERSERVICE_CONTRACTID, &rv);

    // We have to remove ourselves or we will stay alive until app shutdown.
    if (NS_SUCCEEDED(rv)) {
      observerService->RemoveObserver(this, NS_PROFILE_STARTUP_OBSERVER_ID);
    }

    // Now we load our libraries.
    rv = LoadLibraries();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}
