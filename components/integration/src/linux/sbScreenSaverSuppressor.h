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

#ifndef _SB_SCREEN_SAVER_SUPPRESSOR_H_
#define _SB_SCREEN_SAVER_SUPPRESSOR_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Screen saver suppressor service defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbScreenSaverSuppressor.h
 * \brief Screen Saver Suppressor Service Definitions.
 */

//------------------------------------------------------------------------------
//
// Screen saver suppressor imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "../sbBaseScreenSaverSuppressor.h"

// Mozilla imports.
#include <nsITimer.h>

// D-Bus imports.
#include <dbus/dbus.h>


//------------------------------------------------------------------------------
//
// Screen saver suppressor service classes.
//
//------------------------------------------------------------------------------

/**
 * This class provides screen saver suppressor services.
 */

class sbScreenSaverSuppressor : public sbBaseScreenSaverSuppressor
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Interface implementations.
  //

  NS_DECL_ISUPPORTS_INHERITED


  //
  // sbBaseScreenSaverSuppressor implemenation.
  //

  virtual nsresult OnSuppress(PRBool aSuppress);


  //
  // Public services.
  //

  /**
   * Construct a screen saver suppressor object.
   */
  sbScreenSaverSuppressor();

  /**
   * Destroy the screen saver suppressor object.
   */
  virtual ~sbScreenSaverSuppressor();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  //   mSuppressed              If true, screen saver has been suppressed.
  //   mInhibitCookie           Cookie returned from inhibiting screen saver.
  //

  PRBool                        mSuppressed;
  dbus_uint32_t                 mInhibitCookie;


  //
  // Private services.
  //

  /**
   * Suppress the screen saver.
   */
  using sbBaseScreenSaverSuppressor::Suppress;
  nsresult Suppress();

  /**
   * Unsuppress the screen saver.
   */
  nsresult Unsuppress();
};

#endif // _SB_SCREEN_SAVER_SUPPRESSOR_H_

