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

#ifndef SB_HTTP_MEDIA_ITEM_DOWNLOADER_H_
#define SB_HTTP_MEDIA_ITEM_DOWNLOADER_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// HTTP media item downloader defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbHTTPMediaItemDownloader.h
 * \brief HTTP Media Item Downloader Definitions.
 */

//------------------------------------------------------------------------------
//
// HTTP media item downloader imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIMediaItemDownloader.h>


//------------------------------------------------------------------------------
//
// HTTP media item downloader definitions.
//
//------------------------------------------------------------------------------

//
// HTTP media item downloader XPCOM component definitions.
//

#define SB_HTTP_MEDIA_ITEM_DOWNLOADER_CONTRACTID \
          "@songbirdnest.com/Songbird/HTTPMediaItemDownloader;1"
#define SB_HTTP_MEDIA_ITEM_DOWNLOADER_CLASSNAME "sbHTTPMediaItemDownloader"
#define SB_HTTP_MEDIA_ITEM_DOWNLOADER_DESCRIPTION "HTTP Media Item Downloader"
#define SB_HTTP_MEDIA_ITEM_DOWNLOADER_CID                                      \
{                                                                              \
  0x9a364e08,                                                                  \
  0x1dd1,                                                                      \
  0x11b2,                                                                      \
  { 0x91, 0xda, 0xc8, 0xa2, 0xdf, 0x3c, 0x57, 0x85 }                           \
}


//------------------------------------------------------------------------------
//
// HTTP media item downloader classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements an HTTP media item downloader.
 *
 * \see sbIMediaItemDownloader
 */

class sbHTTPMediaItemDownloader : public sbIMediaItemDownloader
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // XPCOM interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBIMEDIAITEMDOWNLOADER


  //
  // Public services.
  //

  /**
   * Construct an HTTP media item downloader object.
   */
  sbHTTPMediaItemDownloader();

  /**
   * Destroy an HTTP media item downloader object.
   */
  virtual ~sbHTTPMediaItemDownloader();
};

#endif // SB_HTTP_MEDIA_ITEM_DOWNLOADER_H_

