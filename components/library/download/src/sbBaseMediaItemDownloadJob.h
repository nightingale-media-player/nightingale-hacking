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

#ifndef SB_BASE_MEDIA_ITEM_DOWNLOAD_JOB_H_
#define SB_BASE_MEDIA_ITEM_DOWNLOAD_JOB_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Base media item download job defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbBaseMediaItemDownloadJob.h
 * \brief Base Media Item Download Job Definitions.
 */

//------------------------------------------------------------------------------
//
// Base media item download job imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIFileDownloader.h>
#include <sbIJobProgress.h>
#include <sbILibrary.h>
#include <sbIMediaItem.h>
#include <sbIMediaItemDownloadJob.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsTArray.h>


//------------------------------------------------------------------------------
//
// Base media item download job classes.
//
//------------------------------------------------------------------------------

// Class declarations.
class nsIURI;

/**
 * This class provides a base class implementation of sbIMediaItemDownloadJob
 * for downloaders that can download using the sbFileDownloader component.
 */

class sbBaseMediaItemDownloadJob : public sbIMediaItemDownloadJob,
                                   public sbIFileDownloaderListener
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
  NS_DECL_SBIMEDIAITEMDOWNLOADJOB
  NS_DECL_SBIJOBPROGRESS
  NS_DECL_SBIFILEDOWNLOADERLISTENER


  //
  // Public Base media item download job services.
  //

  /**
   * Destroy a Base media item download job instance.
   */
  virtual ~sbBaseMediaItemDownloadJob();


  //----------------------------------------------------------------------------
  //
  // Protected interface.
  //
  //----------------------------------------------------------------------------

protected:

  //
  // mLock                      Lock used for thread support.
  // mMediaItem                 Media item to download.
  // mTargetLibrary             Library for which the media item will be
  //                            downloaded.
  // mFileDownloader            File downloader object.
  // mListenerList              List of download job listeners.
  // mStatus                    Download status.
  //

  PRLock*                       mLock;
  nsCOMPtr<sbIMediaItem>        mMediaItem;
  nsCOMPtr<sbILibrary>          mTargetLibrary;
  nsCOMPtr<sbIFileDownloader>   mFileDownloader;
  nsTArray< nsCOMPtr<sbIJobProgressListener> >
                                mListenerList;
  PRUint16                      mStatus;


  /**
   * Start downloading from the source URI specified by aURI.  Sub-classes that
   * need to make network requests to get the source URI can override
   * sbIMediaItemDownloadJob.Start, get the source URI, and then call this Start
   * method when the source URI is available.
   *
   * \param aURI                Source URI from which to download.
   */
  virtual nsresult Start(nsIURI* aURI);

  /**
   * Return the download source URI in aURI.  This method is intended to be
   * overridden by sub-classes.  The default implementation uses the download
   * media item content source.
   *
   * \param aURI                Returned download source URI.
   */
  virtual nsresult GetDownloadURI(nsIURI** aURI);

  /**
   * Initialize a base media item download job.
   */
  virtual nsresult Initialize();

  /**
   * Construct a base media item download job object to download the media item
   * specified by aMediaItem for the library specified by aTargetLibrary.  If
   * aLibrary is not specified, download media item for the main library.
   *
   * \param aMediaItem          Media item to download.
   * \param aTargetLibrary      Library for which the media item will be
   *                            downloaded.
   */
  sbBaseMediaItemDownloadJob(sbIMediaItem* aMediaItem,
                             sbILibrary*   aTargetLibrary = nsnull);
};


#endif // SB_BASE_MEDIA_ITEM_DOWNLOAD_JOB_H_

