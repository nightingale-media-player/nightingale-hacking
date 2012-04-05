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

#ifndef SB_MEDIA_ITEM_DOWNLOAD_SERVICE_H_
#define SB_MEDIA_ITEM_DOWNLOAD_SERVICE_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird media item download service defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMediaItemDownloadService.h
 * \brief Songbird Media Item Download Service Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird media item download service imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIMediaItemDownloadService.h>
#include <sbIServiceManager.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIObserver.h>
#include <nsTArray.h>


//------------------------------------------------------------------------------
//
// Songbird media item download service definitions.
//
//------------------------------------------------------------------------------

//
// Songbird media item download service XPCOM component definitions.
//

#define SB_MEDIA_ITEM_DOWNLOADER_CATEGORY "songbird-media-item-downloader"

#define SB_MEDIA_ITEM_DOWNLOAD_SERVICE_CLASSNAME "sbMediaItemDownloadService"
#define SB_MEDIA_ITEM_DOWNLOAD_SERVICE_DESCRIPTION \
          "Songbird Media Item Download Service"
#define SB_MEDIA_ITEM_DOWNLOAD_SERVICE_CID                                     \
{                                                                              \
  0x35443420,                                                                  \
  0x1dd2,                                                                      \
  0x11b2,                                                                      \
  { 0x8d, 0x65, 0xfe, 0xf3, 0x91, 0x81, 0x82, 0xcf }                           \
}


//------------------------------------------------------------------------------
//
// Songbird media item download service classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the sbIMediaItemDownloadService interface.
 *
 * \see sbIMediaItemDownloadService
 */

class sbMediaItemDownloadService : public sbIMediaItemDownloadService,
                                   public nsIObserver
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
  NS_DECL_SBIMEDIAITEMDOWNLOADSERVICE
  NS_DECL_NSIOBSERVER


  //
  // Public services.
  //

  /**
   * Construct a media item download service object.
   */
  sbMediaItemDownloadService();

  /**
   * Destroy a media item download service object.
   */
  virtual ~sbMediaItemDownloadService();

  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mInitialized               True if initialized.
  // mServiceManager            Songbird service manager object.
  // mDownloaderList            List of media item downloaders.
  //

  PRBool                        mInitialized;
  nsCOMPtr<sbIServiceManager>   mServiceManager;
  nsTArray< nsCOMPtr<sbIMediaItemDownloader> >
                                mDownloaderList;


  /**
   * Initialize the media item download service.
   */
  nsresult Initialize();

  /**
   * Finalize the media item download service.
   */
  void Finalize();
};

#endif // SB_MEDIA_ITEM_DOWNLOAD_SERVICE_H_

