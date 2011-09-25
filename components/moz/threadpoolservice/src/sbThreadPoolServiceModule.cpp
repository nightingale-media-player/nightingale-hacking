/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * \file  sbThreadPoolService.cpp
 * \brief Nightingale ThreadPool Service Module Component Factory and Main Entry Point.
 */

#include "sbThreadPoolService.h"

#include <nsIAppStartupNotifier.h>
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>

#include <nsCOMPtr.h>
#include <nsServiceManagerUtils.h>
#include <nsXPCOM.h>

NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbThreadPoolService, Init)

static NS_METHOD
sbThreadPoolServiceRegisterSelf(nsIComponentManager* aCompMgr,
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
         AddCategoryEntry(NS_XPCOM_STARTUP_CATEGORY,
                          SB_THREADPOOLSERVICE_CLASSNAME,
                          SB_THREADPOOLSERVICE_CONTRACTID,
                          PR_TRUE, PR_TRUE, nsnull);
  return rv;
}

static NS_METHOD
sbThreadPoolServiceUnregisterSelf(nsIComponentManager* aCompMgr,
                                  nsIFile* aPath,
                                  const char* registryLocation,
                                  const nsModuleComponentInfo* info)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> categoryManager =
    do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = categoryManager->DeleteCategoryEntry(NS_XPCOM_STARTUP_CATEGORY,
                                            SB_THREADPOOLSERVICE_CLASSNAME,
                                            PR_TRUE);

  return rv;
}

// Module component information.
static const nsModuleComponentInfo components[] =
{
  {
    SB_THREADPOOLSERVICE_CLASSNAME,
    SB_THREADPOOLSERVICE_CID,
    SB_THREADPOOLSERVICE_CONTRACTID,
    sbThreadPoolServiceConstructor,
    sbThreadPoolServiceRegisterSelf,
    sbThreadPoolServiceUnregisterSelf
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbThreadPoolService, components)
