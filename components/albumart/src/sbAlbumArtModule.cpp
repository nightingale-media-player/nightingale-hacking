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
#include "sbAlbumArtService.h"
#include "sbFileAlbumArtFetcher.h"
#include "sbMetadataAlbumArtFetcher.h"

// Mozilla imports.
#include <nsIGenericFactory.h>


//------------------------------------------------------------------------------
//
// Songbird album art service.
//
//------------------------------------------------------------------------------

// Construct the sbAlbumArtService object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbAlbumArtService, Initialize)


//------------------------------------------------------------------------------
//
// Songbird local file album art fetcher services.
//
//------------------------------------------------------------------------------

// Construct the sbFileAlbumArtFetcher object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbFileAlbumArtFetcher, Initialize)


//------------------------------------------------------------------------------
//
// Songbird metadata album art fetcher services.
//
//------------------------------------------------------------------------------

// Construct the sbMetadataAlbumArtFetcher object and call its Initialize
// method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMetadataAlbumArtFetcher, Initialize)


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
    sbAlbumArtServiceConstructor
  },

  // Local file album art fetcher component info.
  {
    SB_FILEALBUMARTFETCHER_CLASSNAME,
    SB_FILEALBUMARTFETCHER_CID,
    SB_FILEALBUMARTFETCHER_CONTRACTID,
    sbFileAlbumArtFetcherConstructor
  },

  // Metadata album art fetcher component info.
  {
    SB_METADATAALBUMARTFETCHER_CLASSNAME,
    SB_METADATAALBUMARTFETCHER_CID,
    SB_METADATAALBUMARTFETCHER_CONTRACTID,
    sbMetadataAlbumArtFetcherConstructor
  }
};

// NSGetModule
NS_IMPL_NSGETMODULE(sbAlbumArtModule, sbAlbumArtComponents)

