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

#ifndef __SB_MEDIAITEMWATCHER_H__
#define __SB_MEDIAITEMWATCHER_H__

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Media item watcher service.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbMediaItemWatcher.h
 * \brief Nightingale Media Item Watcher Definitions.
 */

//------------------------------------------------------------------------------
//
// Media item watcher imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
#include <sbIMediaItemListener.h>
#include <sbIMediaItemWatcher.h>
#include <sbIMediaListListener.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsStringGlue.h>


//------------------------------------------------------------------------------
//
// Media item watcher defs.
//
//------------------------------------------------------------------------------

//
// Media item watcher component defs.
//

#define NIGHTINGALE_MEDIAITEMWATCHER_CLASSNAME "sbMediaItemWatcher"
#define NIGHTINGALE_MEDIAITEMWATCHER_CID                                          \
  /* {3452f193-9bdc-45d4-a626-d8d7d4605035} */                                 \
  {                                                                            \
    0x3452f193,                                                                \
    0x9bdc,                                                                    \
    0x45d4,                                                                    \
    { 0xa6, 0x26, 0xd8, 0xd7, 0xd4, 0x60, 0x50, 0x35 }                         \
  }


//------------------------------------------------------------------------------
//
// Nightingale album art service classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the media item watcher component.
 */

class sbMediaItemWatcher : public sbIMediaItemWatcher,
                           public sbIMediaListListener
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
  NS_DECL_SBIMEDIAITEMWATCHER
  NS_DECL_SBIMEDIALISTLISTENER


  //
  // Public services.
  //

  sbMediaItemWatcher();

  virtual ~sbMediaItemWatcher();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mWatchedMediaItem          Media item being watched.
  // mListener                  Listener to notify of watched media item events.
  // mWatchedPropertyIDs        List of IDs of media item properties to watch.
  // mWatchedLibraryML          Watched media item's library media list.
  // mWacthedMediaItemProperties
  //                            Current values of watched media item properties.
  // mBatchLevel                Current watched library batch level.
  //

  nsCOMPtr<sbIMediaItem>        mWatchedMediaItem;
  nsCOMPtr<sbIMediaItemListener>
                                mListener;
  nsCOMPtr<sbIPropertyArray>    mWatchedPropertyIDs;
  nsCOMPtr<sbIMediaList>        mWatchedLibraryML;
  nsString                      mWatchedMediaItemProperties;
  PRUint32                      mBatchLevel;


  //
  // Internal services.
  //

  nsresult DoItemUpdated();

  nsresult DoItemUpdated(nsAString& aItemProperties);

  nsresult GetWatchedMediaItemProperties(nsAString& aProperties);
};


#endif /* __ SB_MEDIAITEMWATCHER_H__ */

