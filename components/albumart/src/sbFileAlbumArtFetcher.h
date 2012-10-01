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

#ifndef __SB_FILEALBUMARTFETCHER_H__
#define __SB_FILEALBUMARTFETCHER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird local file album art fetcher.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbFileAlbumArtFetcher.h
 * \brief Songbird Local File Album Art Fetcher Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird local file album art fetcher imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIAlbumArtFetcher.h>
#include <sbIAlbumArtService.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIArray.h>
#include <nsIIOService.h>
#include <nsISimpleEnumerator.h>
#include <nsIPrefBranch.h>
#include <nsTArray.h>
#include <nsStringGlue.h>
#include <nsISupportsPrimitives.h>
#include <nsIURL.h>



//------------------------------------------------------------------------------
//
// Songbird local file album art fetcher defs.
//
//------------------------------------------------------------------------------

//
// Songbird local file album art fetcher component defs.
//

#define SB_FILEALBUMARTFETCHER_CLASSNAME "sbFileAlbumArtFetcher"
#define SB_FILEALBUMARTFETCHER_CID                                             \
  /* {5fdeaf80-1a91-4970-8cba-c016cd6edc82} */                                 \
  {                                                                            \
    0x5fdeaf80,                                                                \
    0x1a91,                                                                    \
    0x4970,                                                                    \
    { 0x8c, 0xba, 0xc0, 0x16, 0xcd, 0x6e, 0xdc, 0x82 }                         \
  }


//------------------------------------------------------------------------------
//
// Songbird local file album art fetcher classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the local file album art fetcher component.
 */

class sbFileAlbumArtFetcher : public sbIAlbumArtFetcher
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

  sbFileAlbumArtFetcher();

  virtual ~sbFileAlbumArtFetcher();

  nsresult Initialize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mPrefService               Preference service to get prefs.
  // mAlbumArtService           Art service - used for caching
  // mIOService                 I/O service.
  // mFileExtensionList         List of album art file extensions.
  // mFileBaseNameList          List of album art file names.
  // mAlbumArtSourceList        List of album art sources.
  //

  nsCOMPtr<nsIPrefBranch>       mPrefService;
  nsCOMPtr<sbIAlbumArtService>  mAlbumArtService;
  nsCOMPtr<nsIIOService>        mIOService;
  nsTArray<nsString>            mFileExtensionList;
  nsTArray<nsString>            mFileBaseNameList;
  nsCOMPtr<nsIArray>            mAlbumArtSourceList;


  //
  // Internal services.
  //

  nsresult GetURLDirEntries(nsIURL*               aURL,
                            bool*               aIsLocalFile,
                            nsISimpleEnumerator** aDirEntries);

  nsresult FindAlbumArtFile(sbIMediaItem*        aMediaItem,
                            nsIFile**            aAlbumArtFile);
};


#endif // __SB_FILEALBUMARTFETCHER_H__

