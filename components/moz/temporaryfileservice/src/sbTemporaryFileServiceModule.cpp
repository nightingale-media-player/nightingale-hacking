/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale temporary file service module services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbTemporaryFileServiceModule.cpp
 * \brief Nightingale Temporary File Service Component Factory and Main Entry
 *        Point.
 */

//------------------------------------------------------------------------------
//
// Nightingale temporary file service module imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbTemporaryFileFactory.h"
#include "sbTemporaryFileService.h"

// Mozilla imports.
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Nightingale temporary file service module temporary file services.
//
//------------------------------------------------------------------------------

// Nightingale temporary file service defs.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbTemporaryFileService, Initialize)


/**
 * \brief Register the Nightingale temporary file service component.
 */

static NS_METHOD
sbTemporaryFileServiceRegister(nsIComponentManager*         aCompMgr,
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
  rv = categoryManager->AddCategoryEntry
                          ("app-startup",
                           SB_TEMPORARYFILESERVICE_CLASSNAME,
                           "service," SB_TEMPORARYFILESERVICE_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * \brief Unregister the Nightingale temporary file service component.
 */

static NS_METHOD
sbTemporaryFileServiceUnregister(nsIComponentManager*         aCompMgr,
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
                                            SB_TEMPORARYFILESERVICE_CLASSNAME,
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Nightingale temporary file service module temporary file factory services.
//
//------------------------------------------------------------------------------

// Nightingale temporary file factory defs.
NS_GENERIC_FACTORY_CONSTRUCTOR(sbTemporaryFileFactory)


//------------------------------------------------------------------------------
//
// Nightingale temporary file service module registration services.
//
//------------------------------------------------------------------------------

// Module component information.
static nsModuleComponentInfo sbTemporaryFileServiceComponents[] =
{
  // Nightingale temporary file service component info.
  {
    SB_TEMPORARYFILESERVICE_CLASSNAME,
    SB_TEMPORARYFILESERVICE_CID,
    SB_TEMPORARYFILESERVICE_CONTRACTID,
    sbTemporaryFileServiceConstructor,
    sbTemporaryFileServiceRegister,
    sbTemporaryFileServiceUnregister
  },

  // Nightingale temporary file factory component info.
  {
    SB_TEMPORARYFILEFACTORY_CLASSNAME,
    SB_TEMPORARYFILEFACTORY_CID,
    SB_TEMPORARYFILEFACTORY_CONTRACTID,
    sbTemporaryFileFactoryConstructor
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbTemporaryFileServiceModule,
                    sbTemporaryFileServiceComponents)

