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
#include <mozilla/ModuleUtils.h>
#include <nsServiceManagerUtils.h>


//------------------------------------------------------------------------------
//
// Songbird album art service.
//
//------------------------------------------------------------------------------

NS_GENERIC_FACTORY_CONSTRUCTOR(sbAlbumArtService);

//------------------------------------------------------------------------------
//
// Songbird album art scanner component 
//
//------------------------------------------------------------------------------

// Construct the sbAlbumArtScanner object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbAlbumArtScanner, Initialize);


//------------------------------------------------------------------------------
//
// Songbird album art fetcher set component.
//
//------------------------------------------------------------------------------

// Construct the sbAlbumArtFetcherSet object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbAlbumArtFetcherSet, Initialize);


//------------------------------------------------------------------------------
//
// Songbird local file album art fetcher component.
//
//------------------------------------------------------------------------------

// Construct the sbFileAlbumArtFetcher object and call its Initialize method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbFileAlbumArtFetcher, Initialize);


//------------------------------------------------------------------------------
//
// Songbird metadata album art fetcher component.
//
//------------------------------------------------------------------------------

// Construct the sbMetadataAlbumArtFetcher object and call its Initialize
// method.
NS_GENERIC_FACTORY_CONSTRUCTOR_INIT(sbMetadataAlbumArtFetcher, Initialize);


//------------------------------------------------------------------------------
//
// Songbird album art components module registration services.
//
//------------------------------------------------------------------------------

NS_DEFINE_NAMED_CID(SB_ALBUMARTSERVICE_CID);
NS_DEFINE_NAMED_CID(SB_ALBUMARTSCANNER_CID);
NS_DEFINE_NAMED_CID(SB_ALBUMARTFETCHERSET_CID);
NS_DEFINE_NAMED_CID(SB_FILEALBUMARTFETCHER_CID);
NS_DEFINE_NAMED_CID(SB_METADATAALBUMARTFETCHER_CID);


static const mozilla::Module::CIDEntry kAlbumArtCIDs[] = {
  { &kSB_ALBUMARTSERVICE_CID, false, NULL, sbAlbumArtServiceConstructor },
  { &kSB_ALBUMARTSCANNER_CID, false, NULL, sbAlbumArtScannerConstructor },
  { &kSB_ALBUMARTFETCHERSET_CID, false, NULL, sbFileAlbumArtFetcherConstructor },
  { &kSB_FILEALBUMARTFETCHER_CID, false, NULL, sbFileAlbumArtFetcherConstructor },
  { &kSB_METADATAALBUMARTFETCHER_CID, false, NULL, sbMetadataAlbumArtFetcherConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kAlbumArtContacts[] = {
  { SB_ALBUMARTSERVICE_CONTRACTID, &kSB_ALBUMARTSERVICE_CID },
  { SB_ALBUMARTSCANNER_CONTRACTID, &kSB_ALBUMARTSCANNER_CID },
  { SB_ALBUMARTFETCHERSET_CONTRACTID, &kSB_ALBUMARTFETCHERSET_CID },
  { SB_FILEALBUMARTFETCHER_CONTRACTID, &kSB_FILEALBUMARTFETCHER_CID },
  { SB_METADATAALBUMARTFETCHER_CONTRACTID, &kSB_METADATAALBUMARTFETCHER_CID },
};

static const mozilla::Module::CategoryEntry kAlbumArtCategories[] = {
  { "app-startup", SB_ALBUMARTSERVICE_CONTRACTID },
  { NULL }
};

static const mozilla::Module kAlbumArtModule = {
  mozilla::Module::kVersion,
  kAlbumArtCIDs,
  kAlbumArtContacts,
  kAlbumArtCategories
};

NSMODULE_DEFN(sbAlbumArtComponents) = &kAlbumArtModule;
