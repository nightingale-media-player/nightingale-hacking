/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird strings components module.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbStringsModule.cpp
 * \brief Songbird Strings Component Factory and Main Entry Point.
 */

//------------------------------------------------------------------------------
//
// Songbird strings components module imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbStringBundleService.h"
#include "sbStringMap.h"
#include "sbCharsetDetector.h"

// Mozilla imports.
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird string bundle service.
//
//------------------------------------------------------------------------------

// Construct the sbStringBundleService object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbStringBundleService, Initialize)


/**
 * Register the Songbird string bundle service component.
 */

static NS_METHOD
sbStringBundleServiceRegister(nsIComponentManager*         aCompMgr,
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

  // Add self to the profile-after-change category (so that the locales are ready)
  rv = categoryManager->AddCategoryEntry
                          ("profile-after-change",
                           SB_STRINGBUNDLESERVICE_CLASSNAME,
                           "service," SB_STRINGBUNDLESERVICE_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Unregister the Songbird string bundle service component.
 */

static NS_METHOD
sbStringBundleServiceUnregister(nsIComponentManager*         aCompMgr,
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
  rv = categoryManager->DeleteCategoryEntry("profile-after-change",
                                            SB_STRINGBUNDLESERVICE_CLASSNAME,
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
// sbStringMap stuff
//------------------------------------------------------------------------------

#define SB_STRINGMAP_CLASSNAME "sbStringMap"
#define SB_STRINGMAP_CID \
{ 0x56a00dd5, 0xcfae, 0x4910, \
  { 0xae, 0x12, 0xef, 0x53, 0x93, 0x5d, 0xcf, 0x3e } }

NS_GENERIC_FACTORY_CONSTRUCTOR(sbStringMap)

// Songbird charset detect utilities defs.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbCharsetDetector)

//------------------------------------------------------------------------------
//
// Songbird strings components module registration services.
//
//------------------------------------------------------------------------------

// Module component information.
static nsModuleComponentInfo sbStringsComponents[] =
{
  // String bundle service component info.
  {
    SB_STRINGBUNDLESERVICE_CLASSNAME,
    SB_STRINGBUNDLESERVICE_CID,
    SB_STRINGBUNDLESERVICE_CONTRACTID,
    sbStringBundleServiceConstructor,
    sbStringBundleServiceRegister,
    sbStringBundleServiceUnregister
  },
  // sbStringMap
  {
    SB_STRINGMAP_CLASSNAME,
    SB_STRINGMAP_CID,
    SB_STRINGMAP_CONTRACTID,
    sbStringMapConstructor
  },
  // Songbird charset detect utilities component info.
  {
    SB_CHARSETDETECTOR_CLASSNAME,
    SB_CHARSETDETECTOR_CID,
    SB_CHARSETDETECTOR_CONTRACTID,
    sbCharsetDetectorConstructor
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbStringsModule, sbStringsComponents)

