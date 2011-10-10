/* vim: set sw=2 :miv */
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

/** 
* \file  sbDeviceManagerModule.cpp
* \brief Nightingale Device Manager Component Factory and Main Entry Point.
*/

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

#include "sbDeviceManager.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(sbDeviceManager);

// Registration functions for becoming a startup observer
static NS_METHOD
sbDeviceManagerRegisterSelf(nsIComponentManager* aCompMgr,
                            nsIFile* aPath,
                            const char* registryLocation,
                            const char* componentType,
                            const nsModuleComponentInfo* info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->
         AddCategoryEntry(APPSTARTUP_CATEGORY,
                          NIGHTINGALE_DEVICEMANAGER2_DESCRIPTION,
                          "service," NIGHTINGALE_DEVICEMANAGER2_CONTRACTID,
                          PR_TRUE, PR_TRUE, nsnull);
  return rv;
}

static NS_METHOD
sbDeviceManagerUnregisterSelf(nsIComponentManager* aCompMgr,
                              nsIFile* aPath,
                              const char* registryLocation,
                              const nsModuleComponentInfo* info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->DeleteCategoryEntry(APPSTARTUP_CATEGORY,
                                            NIGHTINGALE_DEVICEMANAGER2_DESCRIPTION,
                                            PR_TRUE);

  return rv;
}

static nsModuleComponentInfo sbDeviceManagerComponents[] =
{
  {
    NIGHTINGALE_DEVICEMANAGER2_CLASSNAME,
    NIGHTINGALE_DEVICEMANAGER2_CID,
    NIGHTINGALE_DEVICEMANAGER2_CONTRACTID,
    sbDeviceManagerConstructor,
    sbDeviceManagerRegisterSelf,
    sbDeviceManagerUnregisterSelf
  }
};

NS_IMPL_NSGETMODULE(NightingaleDeviceManager2, sbDeviceManagerComponents)
