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
// Songbird album art components module.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtModule.cpp
 * \brief Songbird Album Art Module Component Factory and Main Entry Point.
 */

//------------------------------------------------------------------------------
//
// Songbird album art components module imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbAlbumArtFetcherSet.h"
#include "sbAlbumArtScanner.h"
#include "sbAlbumArtService.h"
#include "sbFileAlbumArtFetcher.h"
#include "sbMetadataAlbumArtFetcher.h"

// Mozilla imports.
#include <nsICategoryManager.h>
#include <nsIGenericFactory.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird album art service.
//
//------------------------------------------------------------------------------

NS_GENERIC_FACTORY_CONSTRUCTOR(sbAlbumArtService)


/**
 * Register the Songbird album art service component.
 */

static NS_METHOD
sbAlbumArtServiceRegister(nsIComponentManager*         aCompMgr,
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
                           SB_ALBUMARTSERVICE_CLASSNAME,
                           "service," SB_ALBUMARTSERVICE_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Unregister the Songbird album art service component.
 */

static NS_METHOD
sbAlbumArtServiceUnregister(nsIComponentManager*         aCompMgr,
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
                                            SB_ALBUMARTSERVICE_CLASSNAME,
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird album art scanner component 
//
//------------------------------------------------------------------------------

// Construct the sbAlbumArtScanner object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbAlbumArtScanner, Initialize)


//------------------------------------------------------------------------------
//
// Songbird album art fetcher set component.
//
//------------------------------------------------------------------------------

// Construct the sbAlbumArtFetcherSet object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbAlbumArtFetcherSet, Initialize)


//------------------------------------------------------------------------------
//
// Songbird local file album art fetcher component.
//
//------------------------------------------------------------------------------

// Construct the sbFileAlbumArtFetcher object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbFileAlbumArtFetcher, Initialize)


/**
 * Register the Songbird local file album art component.
 */

static NS_METHOD
sbFileAlbumArtFetcherRegister(nsIComponentManager*         aCompMgr,
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

  // Add self to the album art fetcher category.
  rv = categoryManager->AddCategoryEntry
                          (SB_ALBUM_ART_FETCHER_CATEGORY,
                           SB_FILEALBUMARTFETCHER_CLASSNAME,
                           SB_FILEALBUMARTFETCHER_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Unregister the Songbird local file album art component.
 */

static NS_METHOD
sbFileAlbumArtFetcherUnregister(nsIComponentManager*         aCompMgr,
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

  // Delete self from the album art fetcher category.
  rv = categoryManager->DeleteCategoryEntry(SB_ALBUM_ART_FETCHER_CATEGORY,
                                            SB_FILEALBUMARTFETCHER_CLASSNAME,
                                            PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

//------------------------------------------------------------------------------
//
// Songbird metadata album art fetcher component.
//
//------------------------------------------------------------------------------

// Construct the sbMetadataAlbumArtFetcher object and call its Initialize
// method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMetadataAlbumArtFetcher, Initialize)


/**
 * Register the Songbird metadata album art component.
 */

static NS_METHOD
sbMetadataAlbumArtFetcherRegister(nsIComponentManager*         aCompMgr,
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

  // Add self to the album art fetcher category.
  rv = categoryManager->AddCategoryEntry
                          (SB_ALBUM_ART_FETCHER_CATEGORY,
                           SB_METADATAALBUMARTFETCHER_CLASSNAME,
                           SB_METADATAALBUMARTFETCHER_CONTRACTID,
                           PR_TRUE,
                           PR_TRUE,
                           nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


/**
 * Unregister the Songbird metadata album art component.
 */

static NS_METHOD
sbMetadataAlbumArtFetcherUnregister(nsIComponentManager*         aCompMgr,
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

  // Delete self from the album art fetcher category.
  rv = categoryManager->DeleteCategoryEntry
                          (SB_ALBUM_ART_FETCHER_CATEGORY,
                           SB_METADATAALBUMARTFETCHER_CLASSNAME,
                           PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Songbird album art components module registration services.
//
//------------------------------------------------------------------------------

// Module component information.
static nsModuleComponentInfo sbAlbumArtComponents[] =
{
  // Album art service component info.
  {
    SB_ALBUMARTSERVICE_CLASSNAME,
    SB_ALBUMARTSERVICE_CID,
    SB_ALBUMARTSERVICE_CONTRACTID,
    sbAlbumArtServiceConstructor,
    sbAlbumArtServiceRegister,
    sbAlbumArtServiceUnregister
  },

  // Album art scanner component info.
  {
    SB_ALBUMARTSCANNER_CLASSNAME,
    SB_ALBUMARTSCANNER_CID,
    SB_ALBUMARTSCANNER_CONTRACTID,
    sbAlbumArtScannerConstructor
  },

  // Album art fetcher set component info.
  {
    SB_ALBUMARTFETCHERSET_CLASSNAME,
    SB_ALBUMARTFETCHERSET_CID,
    SB_ALBUMARTFETCHERSET_CONTRACTID,
    sbAlbumArtFetcherSetConstructor
  },

  // Local file album art fetcher component info.
  {
    SB_FILEALBUMARTFETCHER_CLASSNAME,
    SB_FILEALBUMARTFETCHER_CID,
    SB_FILEALBUMARTFETCHER_CONTRACTID,
    sbFileAlbumArtFetcherConstructor,
    sbFileAlbumArtFetcherRegister,
    sbFileAlbumArtFetcherUnregister
  },
  // Metadata album art fetcher component info.
  {
    SB_METADATAALBUMARTFETCHER_CLASSNAME,
    SB_METADATAALBUMARTFETCHER_CID,
    SB_METADATAALBUMARTFETCHER_CONTRACTID,
    sbMetadataAlbumArtFetcherConstructor,
    sbMetadataAlbumArtFetcherRegister,
    sbMetadataAlbumArtFetcherUnregister
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbAlbumArtModule, sbAlbumArtComponents)

