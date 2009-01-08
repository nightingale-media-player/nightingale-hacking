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

#ifndef __SB_ALBUMARTSCANNER_H__
#define __SB_ALBUMARTSCANNER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird album art scanner
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtScanner.h
 * \brief Songbird Album Art Scanner Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird album art scanner imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIAlbumArtFetcherSet.h>
#include <sbIAlbumArtListener.h>
#include <sbIAlbumArtScanner.h>
#include <sbILibrary.h>
#include <sbIJobProgress.h>
#include <sbIJobCancelable.h>

// Mozilla imports.
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIArray.h>
#include <nsIStringBundle.h>
#include <nsITimer.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

//------------------------------------------------------------------------------
//
// Songbird album art scanner defs.
//
//------------------------------------------------------------------------------

//
// Songbird album art scanner component defs.
//

#define SB_ALBUMARTSCANNER_CLASSNAME "sbAlbumArtScanner"
#define SB_ALBUMARTSCANNER_CONTRACTID \
          "@songbirdnest.com/Songbird/album-art/scanner;1"
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
#define ALBUMART_SCANNER_TIMEOUT  10000

// Preference keys
#define PREF_ALBUMART_SCANNER_INTERVAL "songbird.albumart.scanner.interval"
#define PREF_ALBUMART_SCANNER_TIMEOUT  "songbird.albumart.scanner.timeout"

//------------------------------------------------------------------------------
//
// Songbird album art scanner classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the album art scanner component.
 */

class sbAlbumArtScanner : public sbIAlbumArtScanner,
                          public sbIJobProgress,
                          public sbIJobCancelable,
                          public nsITimerCallback,
                          public sbIAlbumArtListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIALBUMARTSCANNER
  NS_DECL_SBIJOBPROGRESS
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
   * Process the next item in the album if any are left to process
   */
  nsresult ProcessItem();

  // Timers for doing our work
  nsCOMPtr<nsITimer>                       mIntervalTimer;
  PRInt32                                  mIntervalTimerValue;
  nsCOMPtr<nsITimer>                       mTimeoutTimer;
  PRInt32                                  mTimeoutTimerValue;

  // Our fetcher set for scanning for album art
  nsCOMPtr<sbIAlbumArtFetcherSet>          mFetcherSet;

  // sbIJobProgress variables
  PRUint16                                 mStatus;
  nsTArray<nsString>                       mErrorMessages;
  nsString                                 mTitleText;
  nsCOMArray<sbIJobProgressListener>       mListeners;

  // Progress information (Albums)
  nsAutoString                             mCurrentAlbum;
  PRUint32                                 mCompletedAlbumCount;
  PRUint32                                 mTotalAlbumCount;
  PRBool                                   mProcessNextAlbum;
  // Progress information (Items per album)
  nsCOMPtr<nsIArray>                       mCurrentAlbumItemList;
  nsAutoString                             mCurrentFetcherName;
  PRUint32                                 mCompletedItemCount;
  PRUint32                                 mTotalItemCount;
  PRBool                                   mProcessNextItem;

  // The MediaList we are scanning.
  nsCOMPtr<sbIMediaList>                   mMediaList;
  nsCOMPtr<sbICascadeFilterSet>            mFilterSet;
  PRUint16                                 mAlbumFilterIndex;

  // String bundle for status messages.
  nsCOMPtr<nsIStringBundle>                mStringBundle;
};

#endif // __SB_ALBUMARTSCANNER_H__
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

#ifndef __SB_ALBUMARTSCANNER_H__
#define __SB_ALBUMARTSCANNER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird album art scanner
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbAlbumArtScanner.h
 * \brief Songbird Album Art Scanner Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird album art scanner imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIAlbumArtFetcherSet.h>
#include <sbIAlbumArtListener.h>
#include <sbIAlbumArtScanner.h>
#include <sbILibrary.h>
#include <sbIJobProgress.h>
#include <sbIJobCancelable.h>

// Mozilla imports.
#include <nsCOMArray.h>
#include <nsCOMPtr.h>
#include <nsIArray.h>
#include <nsIStringBundle.h>
#include <nsITimer.h>
#include <nsStringGlue.h>
#include <nsTArray.h>

//------------------------------------------------------------------------------
//
// Songbird album art scanner defs.
//
//------------------------------------------------------------------------------

//
// Songbird album art scanner component defs.
//

#define SB_ALBUMARTSCANNER_CLASSNAME "sbAlbumArtScanner"
#define SB_ALBUMARTSCANNER_CONTRACTID \
          "@songbirdnest.com/Songbird/album-art/scanner;1"
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
#define ALBUMART_SCANNER_TIMEOUT  10000

// Preference keys
#define PREF_ALBUMART_SCANNER_INTERVAL "songbird.albumart.scanner.interval"
#define PREF_ALBUMART_SCANNER_TIMEOUT  "songbird.albumart.scanner.timeout"

//------------------------------------------------------------------------------
//
// Songbird album art scanner classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the album art scanner component.
 */

class sbAlbumArtScanner : public sbIAlbumArtScanner,
                          public sbIJobProgress,
                          public sbIJobCancelable,
                          public nsITimerCallback,
                          public sbIAlbumArtListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_SBIALBUMARTSCANNER
  NS_DECL_SBIJOBPROGRESS
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
   * Process the next item in the album if any are left to process
   */
  nsresult ProcessItem();

  // Timers for doing our work
  nsCOMPtr<nsITimer>                       mIntervalTimer;
  PRInt32                                  mIntervalTimerValue;
  nsCOMPtr<nsITimer>                       mTimeoutTimer;
  PRInt32                                  mTimeoutTimerValue;

  // Our fetcher set for scanning for album art
  nsCOMPtr<sbIAlbumArtFetcherSet>          mFetcherSet;

  // sbIJobProgress variables
  PRUint16                                 mStatus;
  nsTArray<nsString>                       mErrorMessages;
  nsString                                 mTitleText;
  nsCOMArray<sbIJobProgressListener>       mListeners;

  // Progress information (Albums)
  nsAutoString                             mCurrentAlbum;
  PRUint32                                 mCompletedAlbumCount;
  PRUint32                                 mTotalAlbumCount;
  PRBool                                   mProcessNextAlbum;
  // Progress information (Items per album)
  nsCOMPtr<nsIArray>                       mCurrentAlbumItemList;
  nsAutoString                             mCurrentFetcherName;
  PRUint32                                 mCompletedItemCount;
  PRUint32                                 mTotalItemCount;
  PRBool                                   mProcessNextItem;

  // The MediaList we are scanning.
  nsCOMPtr<sbIMediaList>                   mMediaList;
  nsCOMPtr<sbICascadeFilterSet>            mFilterSet;
  PRUint16                                 mAlbumFilterIndex;

  // String bundle for status messages.
  nsCOMPtr<nsIStringBundle>                mStringBundle;
};

#endif // __SB_ALBUMARTSCANNER_H__
