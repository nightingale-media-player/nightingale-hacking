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

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird screen saver suppressor services.
//
//   For documentation on the Gnome screen saver D-Bus interface, see
// http://www.gnome.org/~mccann/gnome-screensaver/docs/gnome-screensaver.html
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbScreenSaverSuppressor.cpp
 * \brief Songbird Screen Saver Suppressor Services Source.
 */

//------------------------------------------------------------------------------
//
// Songbird screen saver suppressor imported services.
//
//------------------------------------------------------------------------------

// Self imports.
#include "sbScreenSaverSuppressor.h"

// Local imports.
#include "sbDBus.h"

// Mozilla imports.
#include <nsAutoPtr.h>


//------------------------------------------------------------------------------
//
// Songbird screen saver suppressor nsISupports services.
//
//------------------------------------------------------------------------------

NS_IMPL_ISUPPORTS_INHERITED0(sbScreenSaverSuppressor,
                             sbBaseScreenSaverSuppressor)


//------------------------------------------------------------------------------
//
// Songbird screen saver suppressor sbBaseScreenSaverSuppressor services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// OnSuppress
//

/*virtual*/ nsresult
sbScreenSaverSuppressor::OnSuppress(PRBool aSuppress)
{
  nsresult rv;

  // Suppress or unsupress the screen saver.
  if (aSuppress) {
    rv = Suppress();
    NS_ENSURE_SUCCESS(rv, rv);
  }
  else {
    rv = Unsuppress();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}


//------------------------------------------------------------------------------
//
// Public Songbird screen saver suppressor services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbScreenSaverSuppressor
//

sbScreenSaverSuppressor::sbScreenSaverSuppressor() :
  mSuppressed(PR_FALSE)
{
}


//-------------------------------------
//
// ~sbScreenSaverSuppressor
//

sbScreenSaverSuppressor::~sbScreenSaverSuppressor()
{
  // Unsuppress.
  Unsuppress();
}


//------------------------------------------------------------------------------
//
// Private Songbird screen saver suppressor services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// Suppress
//

nsresult
sbScreenSaverSuppressor::Suppress()
{
  nsresult rv;

  // Do nothing if already suppressed.
  if (mSuppressed)
    return NS_OK;

  // Open a D-Bus connection to the Gnome screen saver.
  nsAutoPtr<sbDBusConnection> dBusConnection;
  rv = sbDBusConnection::New(getter_Transfers(dBusConnection),
                             DBUS_BUS_SESSION,
                             "org.gnome.ScreenSaver",
                             "/org/gnome/ScreenSaver",
                             "org.gnome.ScreenSaver");
  NS_ENSURE_SUCCESS(rv, rv);

  // Inhibit the screen saver.
  static const char* name = "Songbird";
  static const char* reason = "Playing video";
  nsAutoPtr<sbDBusMessage> reply;
  rv = dBusConnection->InvokeMethod("Inhibit",
                                    getter_Transfers(reply),
                                    DBUS_TYPE_STRING,
                                    &name,
                                    DBUS_TYPE_STRING,
                                    &reason,
                                    DBUS_TYPE_INVALID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Get the screen saver inhibit cookie.
  rv = reply->GetArgs(DBUS_TYPE_UINT32, &mInhibitCookie, DBUS_TYPE_INVALID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Screen saver is now suppressed.
  mSuppressed = PR_TRUE;

  return NS_OK;
}


//-------------------------------------
//
// Unsuppress
//

nsresult
sbScreenSaverSuppressor::Unsuppress()
{
  nsresult rv;

  // Do nothing if not suppressed.
  if (!mSuppressed)
    return NS_OK;

  // Open a D-Bus connection to the Gnome screen saver.
  nsAutoPtr<sbDBusConnection> dBusConnection;
  rv = sbDBusConnection::New(getter_Transfers(dBusConnection),
                             DBUS_BUS_SESSION,
                             "org.gnome.ScreenSaver",
                             "/org/gnome/ScreenSaver",
                             "org.gnome.ScreenSaver");
  NS_ENSURE_SUCCESS(rv, rv);

  // Uninhibit the screen saver.
  rv = dBusConnection->InvokeMethod("UnInhibit",
                                    NULL,
                                    DBUS_TYPE_UINT32,
                                    &mInhibitCookie,
                                    DBUS_TYPE_INVALID);
  NS_ENSURE_SUCCESS(rv, rv);

  // Screen saver is no longer suppressed.
  mSuppressed = PR_FALSE;

  return NS_OK;
}

