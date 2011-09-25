/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#include "sbDirectoryProvider.h"

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIGenericFactory.h>
#include <nsICategoryManager.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbDirectoryProvider, Init)

/**
 * Register the Nightingale directory service provider component.
 */

static NS_METHOD
sbDirectoryProviderRegister(nsIComponentManager*         aCompMgr,
                            nsIFile*                     aPath,
                            const char*                  aLoaderStr,
                            const char*                  aType,
                            const nsModuleComponentInfo* aInfo)
{
  nsresult rv;

  // Get the category manager.
  nsCOMPtr<nsICategoryManager> categoryManager =
                                 do_GetService(NS_CATEGORYMANAGER_CONTRACTID,
                                               &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Add self to the app-startup category.
  rv = categoryManager->AddCategoryEntry
                          ("app-startup",
                           NIGHTINGALE_DIRECTORY_PROVIDER_CLASSNAME,
                           "service," NIGHTINGALE_DIRECTORY_PROVIDER_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Unregister the Nightingale directory service provider component.
 */

static NS_METHOD
sbDirectoryProviderUnregister(nsIComponentManager*         aCompMgr,
                              nsIFile*                     aPath,
                              const char*                  aLoaderStr,
                              const nsModuleComponentInfo* aInfo)
{
  nsresult rv;

  // Get the category manager.
  nsCOMPtr<nsICategoryManager> categoryManager =
                                 do_GetService(NS_CATEGORYMANAGER_CONTRACTID,
                                               &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // Delete self from the app-startup category.
  rv = categoryManager->DeleteCategoryEntry("app-startup",
                                            NIGHTINGALE_DIRECTORY_PROVIDER_CLASSNAME,
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

// fill out data struct to register with component system
static const nsModuleComponentInfo components[] =
{
  {
    NIGHTINGALE_DIRECTORY_PROVIDER_CLASSNAME,
    NIGHTINGALE_DIRECTORY_PROVIDER_CID,
    NIGHTINGALE_DIRECTORY_PROVIDER_CONTRACTID,
    sbDirectoryProviderConstructor,
    sbDirectoryProviderRegister,
    sbDirectoryProviderUnregister
  }
};

// create the module info struct that is used to register
NS_IMPL_NSGETMODULE(sbDirectoryProviderModule, components)
