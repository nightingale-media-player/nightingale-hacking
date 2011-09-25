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

#ifndef __SB_ALBUMARTSCANNER_H__
#define __SB_ALBUMARTSCANNER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale album art scanner
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtScanner.h
 * \brief Nightingale Album Art Scanner Definitions.
 */

//------------------------------------------------------------------------------
//
// Nightingale album art scanner imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
#include <sbIAlbumArtFetcherSet.h>
#include <sbIAlbumArtListener.h>
#include <sbIAlbumArtScanner.h>
#include <sbIAlbumArtService.h>
#include <sbIJobCancelable.h>
#include <sbIJobProgress.h>
#include <sbIJobProgressUI.h>
#include <sbILibrary.h>

// Mozilla imports.
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIArray.h>
#include <nsIClassInfo.h>
#include <nsIMutableArray.h>
#include <nsIStringBundle.h>
#include <nsITimer.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

//------------------------------------------------------------------------------
//
// Nightingale album art scanner defs.
//
//------------------------------------------------------------------------------

//
// Nightingale album art scanner component defs.
//

#define SB_ALBUMARTSCANNER_CLASSNAME "sbAlbumArtScanner"
#define SB_ALBUMARTSCANNER_CONTRACTID \
          "@getnightingale.com/Nightingale/album-art/scanner;1"
#define SB_ALBUMARTSCANNER_CID                                                 \
  /* {06c6e812-ca0d-4632-8e2d-2b0a8c9b0675} */                                 \
  {                                                                            \
    0x06c6e812,                                                                \
    0xca0d,                                                                    \
    0x4632,                                                                    \
    { 0x8e, 0x2d, 0x2b, 0x0a, 0x8c, 0x9b, 0x06, 0x75 }                         \
  }

// Default values for the timers (milliseconds)
#define ALBUMART_SCANNER_INTERVAL 10

// Preference keys
#define PREF_ALBUMART_SCANNER_BRANCH    "nightingale.albumart.scanner."
#define PREF_ALBUMART_SCANNER_INTERVAL  "interval"

//------------------------------------------------------------------------------
//
// Nightingale album art scanner classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the album art scanner component.
 */

class sbAlbumArtScanner : public sbIAlbumArtScanner,
                          public nsIClassInfo,
                          public sbIJobProgressUI,
                          public sbIJobCancelable,
                          public nsITimerCallback,
                          public sbIAlbumArtListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIALBUMARTSCANNER
  NS_DECL_NSICLASSINFO
  NS_DECL_SBIJOBPROGRESS
  NS_DECL_SBIJOBPROGRESSUI
  NS_DECL_SBIJOBCANCELABLE
  NS_DECL_NSITIMERCALLBACK
  NS_DECL_SBIALBUMARTLISTENER

  sbAlbumArtScanner();

  virtual ~sbAlbumArtScanner();

  nsresult Initialize();

private:
  /**
   * Updates the jobprogress information and calls the listeners
   */
  nsresult UpdateProgress();

  /**
   * Process the next album if any are left to process
   */
  nsresult ProcessAlbum();

  /**
   * Get a list of albums
   */
  nsresult GetNextAlbumItems();

  /**
   * Mark that a remote album art fetch was attempted for the media item
   * specified by aMediaItem.
   *
   * \param aMediaItem          Media item for which remote album art fetch was
   *                            attempted.
   */
  nsresult MarkRemoteFetchAttempted(sbIMediaItem* aMediaItem);

  // Timers for doing our work
  nsCOMPtr<nsITimer>                       mIntervalTimer;
  PRInt32                                  mIntervalTimerValue;

  // Our fetcher set for scanning for album art
  nsCOMPtr<sbIAlbumArtFetcherSet>          mFetcherSet;

  // sbIAlbumArtScanner variables
  PRBool                                   mUpdateArtwork;

  // sbIJobProgress variables
  PRUint16                                 mStatus;
  nsTArray<nsString>                       mErrorMessages;
  nsString                                 mTitleText;
  nsCOMArray<sbIJobProgressListener>       mListeners;
  PRUint32                                 mCompletedItemCount;
  PRUint32                                 mTotalItemCount;

  // Info for fetcher we are currently using
  nsCOMPtr<sbIAlbumArtFetcher>             mCurrentFetcher;
  nsAutoString                             mCurrentFetcherName;
  nsAutoString                             mCurrentAlbumName;

  // Flag to indicate when we are ready to process the next album
  PRBool                                   mProcessNextAlbum;

  // List of items to scan that make up an album
  nsCOMPtr<nsIMutableArray>                mCurrentAlbumItemList;

  // The MediaView of the MediaList we are scanning.
  nsCOMPtr<sbIMediaListView>               mMediaListView;

  // String bundle for status messages.
  nsCOMPtr<nsIStringBundle>                mStringBundle;
};

#endif // __SB_ALBUMARTSCANNER_H__
