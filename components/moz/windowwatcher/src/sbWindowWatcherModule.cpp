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

/** 
 * \file  sbWindowWatcherModule.cpp
 * \brief Songbird Window Watcher Module Component Factory and Main Entry Point.
 */

//------------------------------------------------------------------------------
//
// Songbird window watcher module imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbWindowWatcher.h"

// Mozilla imports.
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird window watcher module services.
//
//------------------------------------------------------------------------------

// Construct the sbWindowWatcher object and call its Init method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbWindowWatcher, Init)


/**
 * \brief Register the Songbird window watcher component.
 */

static NS_METHOD
sbWindowWatcherRegister(nsIComponentManager*         aCompMgr,
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

  // Add self to the device marshall category.
  rv = categoryManager->AddCategoryEntry("app-startup",
                                         SB_WINDOWWATCHER_CLASSNAME,
                                         "service," SB_WINDOWWATCHER_CONTRACTID,
                                         PR_TRUE,
                                         PR_TRUE,
                                         nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * \brief Unregister the Songbird window watcher component.
 */

static NS_METHOD
sbWindowWatcherUnregister(nsIComponentManager*         aCompMgr,
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

  // Delete self from the device marshall category.
  rv = categoryManager->DeleteCategoryEntry("app-startup",
                                            SB_WINDOWWATCHER_CLASSNAME,
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


// Module component information.
static const nsModuleComponentInfo components[] =
{
  {
    SB_WINDOWWATCHER_CLASSNAME,
    SB_WINDOWWATCHER_CID,
    SB_WINDOWWATCHER_CONTRACTID,
    sbWindowWatcherConstructor,
    sbWindowWatcherRegister,
    sbWindowWatcherUnregister
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbWindowWatcher, components)

