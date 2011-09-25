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

#ifndef __SB_METADATAALBUMARTFETCHER_H__
#define __SB_METADATAALBUMARTFETCHER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale metadata album art fetcher.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMetadataAlbumArtFetcher.h
 * \brief Nightingale Metadata Album Art Fetcher Definitions.
 */

//------------------------------------------------------------------------------
//
// Nightingale metadata album art fetcher imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
#include <sbIAlbumArtFetcher.h>
#include <sbIAlbumArtService.h>
#include <sbIMetadataManager.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIPrefBranch.h>


//------------------------------------------------------------------------------
//
// Nightingale metadata album art fetcher defs.
//
//------------------------------------------------------------------------------

//
// Nightingale metadata album art fetcher component defs.
//

#define SB_METADATAALBUMARTFETCHER_CLASSNAME "sbMetadataAlbumArtFetcher"
#define SB_METADATAALBUMARTFETCHER_CID                                         \
  /* {61f7f510-abef-4b04-9ebf-078976f1bef1} */                                 \
  {                                                                            \
    0x61f7f510,                                                                \
    0xabef,                                                                    \
    0x4b04,                                                                    \
    { 0x9e, 0xbf, 0x07, 0x89, 0x76, 0xf1, 0xbe, 0xf1 }                         \
  }


//------------------------------------------------------------------------------
//
// Nightingale metadata album art fetcher classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the metadata album art fetcher component.
 */

class sbMetadataAlbumArtFetcher : public sbIAlbumArtFetcher
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Inherited interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIALBUMARTFETCHER


  //
  // Public services.
  //

  sbMetadataAlbumArtFetcher();

  virtual ~sbMetadataAlbumArtFetcher();

  nsresult Initialize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mPrefService               Preference service to get prefs.
  // mAlbumArtService           Album art service.
  // mAlbumArtSourceList        List of album art sources.
  //

  nsCOMPtr<nsIPrefBranch>       mPrefService;
  nsCOMPtr<sbIAlbumArtService>  mAlbumArtService;
  nsCOMPtr<nsIArray>            mAlbumArtSourceList;


  //
  // Internal services.
  //

  nsresult GetMetadataHandler(nsIURI*               aContentSrcURI,
                              nsIArray*             aSourceList,
                              sbIMetadataManager*  aManager,
                              sbIMetadataHandler**  aMetadataHandler);
  
  nsresult GetImageForItem(sbIMediaItem*            aMediaItem,
                           nsIArray*                aSourceList,
                           sbIMetadataManager*      aManager,
                           sbIAlbumArtListener*     aListener);
};


#endif // __SB_METADATAALBUMARTFETCHER_H__

