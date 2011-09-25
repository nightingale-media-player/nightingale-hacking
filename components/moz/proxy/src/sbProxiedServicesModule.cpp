/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

#include "sbProxiedServices.h"

#include <nsIGenericFactory.h>
#include <nsICategoryManager.h>
#include <nsServiceManagerUtils.h>
#include <nsMemory.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbProxiedServices, Init)

static
NS_METHOD
sbProxiedServicesRegister(nsIComponentManager* aCompMgr,
                          nsIFile* aPath,
                          const char* registryLocation,
                          const char* componentType,
                          const nsModuleComponentInfo* info)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  char* prev = nsnull;
  rv = catman->AddCategoryEntry("app-startup",
                                "sbProxiedServices",
                                "service," NIGHTINGALE_PROXIEDSERVICES_CONTRACTID,
                                PR_TRUE,
                                PR_TRUE,
                                &prev);
  if (prev)
    nsMemory::Free(prev);

  return rv;
}

static
NS_METHOD
sbProxiedServicesUnregister(nsIComponentManager* aCompMgr,
                          nsIFile* aPath,
                          const char* registryLocation,
                          const nsModuleComponentInfo* info)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catman =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = catman->DeleteCategoryEntry("app-startup", "sbProxiedServices", PR_TRUE);

  return rv;
}

static const nsModuleComponentInfo components[] =
{
  {
    NIGHTINGALE_PROXIEDSERVICES_CLASSNAME,
    NIGHTINGALE_PROXIEDSERVICES_CID,
    NIGHTINGALE_PROXIEDSERVICES_CONTRACTID,
    sbProxiedServicesConstructor,
    sbProxiedServicesRegister,
    sbProxiedServicesUnregister
  }
};

NS_IMPL_NSGETMODULE(sbProxiedServices, components)

