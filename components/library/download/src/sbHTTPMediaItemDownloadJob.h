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

#ifndef SB_HTTP_MEDIA_ITEM_DOWNLOAD_JOB_H_
#define SB_HTTP_MEDIA_ITEM_DOWNLOAD_JOB_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// HTTP media item download job defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbHTTPMediaItemDownloadJob.h
 * \brief HTTP Media Item Download Job Definitions.
 */

//------------------------------------------------------------------------------
//
// HTTP media item download job imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbBaseMediaItemDownloadJob.h"


//------------------------------------------------------------------------------
//
// HTTP media item download job classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements sbIMediaItemDownloadJob for HTTP media items.
 */

class sbHTTPMediaItemDownloadJob : public sbBaseMediaItemDownloadJob
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

  NS_DECL_ISUPPORTS_INHERITED


  //
  // Public HTTP media item download job services.
  //

  /**
   * Create and return in aJob a new HTTP media item download job to download
   * the media item specified by aMediaItem for the library specified by
   * aTargetLibrary.  If aTargetLibrary is not spcified, create a job to
   * download for the main library.
   *
   * \param aJob                Returned download job.
   * \param aMediaItem          Media item to download.
   * \param aTargetLibrary      Library for which the media item will be
   *                            downloaded.
   */
  static nsresult New(sbHTTPMediaItemDownloadJob** aJob,
                      sbIMediaItem*                aMediaItem,
                      sbILibrary*                  aTargetLibrary = nsnull);

  /**
   * Destroy an HTTP media item download job instance.
   */
  virtual ~sbHTTPMediaItemDownloadJob();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  /**
   * Construct an HTTP media item download job object to download the media item
   * specified by aMediaItem for the library specified by aTargetLibrary.  If
   * aTargetLibrary is not specified, create a job for downloading for the main
   * library.
   *
   * \param aMediaItem          Media item to download.
   * \param aTargetLibrary      Library for which the media item will be
   *                            downloaded.
   */
  sbHTTPMediaItemDownloadJob(sbIMediaItem* aMediaItem,
                             sbILibrary*   aTargetLibrary = nsnull);
};


#endif // SB_HTTP_MEDIA_ITEM_DOWNLOAD_JOB_H_

