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

#ifndef __SB_ALBUMARTFETCHERSET_H__
#define __SB_ALBUMARTFETCHERSET_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird album art fetcher set.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtFetcherSet.h
 * \brief Songbird Album Art Fetcher Set Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird album art fetcher set imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIAlbumArtFetcherSet.h>
#include <sbIAlbumArtService.h>
#include <sbIAlbumArtListener.h>
#include <sbIMediaItem.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIConsoleService.h>


//------------------------------------------------------------------------------
//
// Songbird album art fetcher set defs.
//
//------------------------------------------------------------------------------

//
// Songbird album art fetcher set component defs.
//

#define SB_ALBUMARTFETCHERSET_CLASSNAME "sbAlbumArtFetcherSet"
#define SB_ALBUMARTFETCHERSET_CID                                              \
  /* {5da41a79-0ed6-4876-bbd7-348a447c784f} */                                 \
  {                                                                            \
    0x5da41a79,                                                                \
    0x0ed6,                                                                    \
    0x4876,                                                                    \
    { 0xbb, 0xd7, 0x34, 0x8a, 0x44, 0x7c, 0x78, 0x4f }                         \
  }


//------------------------------------------------------------------------------
//
// Songbird album art fetcher set classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the album art fetcher set component.
 */

class sbAlbumArtFetcherSet : public sbIAlbumArtFetcherSet,
                             public sbIAlbumArtListener
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
  NS_DECL_SBIALBUMARTFETCHERSET
  NS_DECL_SBIALBUMARTLISTENER


  //
  // Public services.
  //

  sbAlbumArtFetcherSet();

  virtual ~sbAlbumArtFetcherSet();

  nsresult Initialize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mAlbumArtService           Album art service.
  // mConsoleService            Console service for warning messages.
  // mAlbumArtSourceList        List of album art sources.
  // mLocalOnly                 If true, only fetch locally.
  //

  nsCOMPtr<sbIAlbumArtService>  mAlbumArtService;
  nsCOMPtr<nsIConsoleService>   mConsoleService;
  nsCOMPtr<nsIArray>            mAlbumArtSourceList;
  PRBool                        mLocalOnly;

  //
  // mListener                  Listener for the fetching.
  //
  
  nsCOMPtr<sbIAlbumArtListener> mListener;

  //
  // mFetcherList               List of fetchers to check.
  // mFetcherIndex              Index of current fetcher we are checking.
  // mFetcher                   Current Fetcher we are using.
  //

  nsCOMPtr<nsIArray>            mFetcherList;
  PRInt32                       mFetcherIndex;
  nsCOMPtr<sbIAlbumArtFetcher>  mFetcher;

  //
  // Internal services.
  //

  nsresult FinishFetch(nsIURI*        aImageLocation,
                       sbIMediaItem*  aMediaItem);

  nsresult NextFetcher(sbIMediaItem*  aMediaItem);
};


#endif // __SB_ALBUMARTFETCHERSET_H__

