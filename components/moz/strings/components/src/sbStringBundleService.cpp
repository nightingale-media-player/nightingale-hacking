/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird string bundle service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbStringBundleService.cpp
 * \brief Songbird String Bundle Service Source.
 */

//------------------------------------------------------------------------------
//
// Songbird string bundle service imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbStringBundleService.h"

// Songbird imports.
#include <sbProxyUtils.h>

// Mozilla imports.
#include <nsIStringBundle.h>
#include <nsServiceManagerUtils.h>
#include <nsThreadUtils.h>


//------------------------------------------------------------------------------
//
// nsISupports implementation.
//
//------------------------------------------------------------------------------

NS_IMPL_THREADSAFE_ISUPPORTS1(sbStringBundleService,
                              sbIStringBundleService)


//------------------------------------------------------------------------------
//
// sbIStringBundleService implementation.
//
//------------------------------------------------------------------------------

/**
 * \brief Create a thread-safe string bundle for the bundle with the URL spec
 *        specified by aURLSpec.
 *
 * \param aURLSpec            URL spec of string bundle.
 *
 * \return                    Thread-safe string bundle.
 */

NS_IMETHODIMP
sbStringBundleService::CreateBundle(const char*       aURLSpec,
                                    nsIStringBundle** _retval)
{
  // Validate arguments.
  NS_ENSURE_ARG_POINTER(aURLSpec);
  NS_ENSURE_ARG_POINTER(_retval);

  // Function variables.
  nsresult rv;

  // Get the Mozilla string bundle service.
  nsCOMPtr<nsIStringBundleService>
    stringBundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  // If not on the main thread, proxy the string bundle service.
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIStringBundleService> proxy;
    rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                              NS_GET_IID(nsIStringBundleService),
                              stringBundleService,
                              nsIProxyObjectManager::INVOKE_SYNC |
                              nsIProxyObjectManager::FORCE_PROXY_CREATION,
                              getter_AddRefs(proxy));
    NS_ENSURE_SUCCESS(rv, rv);
    stringBundleService.swap(proxy);
  }

  // Get the string bundle.
  nsCOMPtr<nsIStringBundle> bundle;
  rv = stringBundleService->CreateBundle(aURLSpec, getter_AddRefs(bundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Proxy the string bundle, so it's thread-safe.
  nsCOMPtr<nsIStringBundle> bundleProxy;
  rv = SB_GetProxyForObject(NS_PROXY_TO_MAIN_THREAD,
                            NS_GET_IID(nsIStringBundle),
                            bundle,
                            nsIProxyObjectManager::INVOKE_SYNC |
                            nsIProxyObjectManager::FORCE_PROXY_CREATION,
                            getter_AddRefs(bundleProxy));
  NS_ENSURE_SUCCESS(rv, rv);

  // Return results.
  bundleProxy.forget(_retval);

  return NS_OK;
}


//
// Getters/setters.
//

/**
 * \brief Main Songbird string bundle.  Thread-safe.
 */

NS_IMETHODIMP
sbStringBundleService::GetBundle(nsIStringBundle** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ADDREF(*_retval = mBundle);
  return NS_OK;
}


/**
 * \brief Main Songbird brand string bundle.  Thread-safe.
 */

NS_IMETHODIMP
sbStringBundleService::GetBrandBundle(nsIStringBundle** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  NS_ADDREF(*_retval = mBrandBundle);
  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public services.
//
//------------------------------------------------------------------------------

/**
 * Construct a Songbird string bundle service instance.
 */

sbStringBundleService::sbStringBundleService()
{
}


/**
 * Destroy a Songbird string bundle service instance.
 */

sbStringBundleService::~sbStringBundleService()
{
}


/**
 * Initialize the Songbird string bundle service.
 */

nsresult
sbStringBundleService::Initialize()
{
  nsresult rv;

  // Get the main Songbird string bundle.
  rv = CreateBundle("chrome://songbird/locale/songbird.properties",
                    getter_AddRefs(mBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the main Songbird string bundle.
  rv = CreateBundle("chrome://branding/locale/brand.properties",
                    getter_AddRefs(mBrandBundle));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

