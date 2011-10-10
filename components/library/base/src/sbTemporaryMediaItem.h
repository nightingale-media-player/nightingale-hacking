/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

#ifndef _SB_TEMPORARYMEDIAITEM_H_
#define _SB_TEMPORARYMEDIAITEM_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Temporary media item.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbTemporaryMediaItem.h
 * \brief Nightingale Temporary Media Item Definitions.
 */

//------------------------------------------------------------------------------
//
// Temporary media item imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
#include <sbIMediaItem.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsStringGlue.h>


//------------------------------------------------------------------------------
//
// Temporary media item defs.
//
//------------------------------------------------------------------------------

//
// Temporary media item component defs.
//

#define NIGHTINGALE_TEMPORARYMEDIAITEM_CONTRACTID \
          "@getnightingale.com/Nightingale/Library/TemporaryMediaItem;1"
#define NIGHTINGALE_TEMPORARYMEDIAITEM_CLASSNAME "sbTemporaryMediaItem"
#define NIGHTINGALE_TEMPORARYMEDIAITEM_CID                                        \
  /* {f1ea940e-1dd1-11b2-a9a6-cbcc44d9500d} */                                 \
  { 0xf1ea940e,                                                                \
    0x1dd1,                                                                    \
    0x11b2,                                                                    \
    { 0xa9, 0xa6, 0xcb, 0xcc, 0x44, 0xd9, 0x50, 0x0d }                         \
  }


//------------------------------------------------------------------------------
//
// Temporary media item service classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the temporary media item component.
 */

class sbTemporaryMediaItem : public sbIMediaItem
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
  NS_DECL_SBILIBRARYRESOURCE
  NS_DECL_SBIMEDIAITEM


  //
  // Public services.
  //

  /**
   * Construct a temporary media item instance.
   */
  sbTemporaryMediaItem();

  /**
   * Destroy a temporary media item instance.
   */
  virtual ~sbTemporaryMediaItem();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mContentSrc                Media item content source.
  // mContentType               Media item content type.
  //

  nsCOMPtr<nsIURI>              mContentSrc;
  nsString                      mContentType;
};


#endif /* _SB_TEMPORARYMEDIAITEM_H_ */


