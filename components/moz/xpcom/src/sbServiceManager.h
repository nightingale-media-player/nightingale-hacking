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

#ifndef SB_SERVICE_MANAGER_H_
#define SB_SERVICE_MANAGER_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird service manager defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbServiceManager.h
 * \brief Songbird Service Manager Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird service manager imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbIServiceManager.h>

// Mozilla imports.
#include <nsDataHashtable.h>
#include <nsIObserverService.h>


//------------------------------------------------------------------------------
//
// Songbird service manager definitions.
//
//------------------------------------------------------------------------------

//
// Songbird service manager XPCOM component definitions.
//

#define SB_SERVICE_MANAGER_CLASSNAME "sbServiceManager"
#define SB_SERVICE_MANAGER_DESCRIPTION "Songbird Service Manager"
#define SB_SERVICE_MANAGER_CID                                                 \
{                                                                              \
  0xf162173e,                                                                  \
  0x1dd1,                                                                      \
  0x11b2,                                                                      \
  { 0xac, 0x46, 0x96, 0x10, 0x81, 0x18, 0x91, 0x9e }                           \
}


//------------------------------------------------------------------------------
//
// Songbird service manager classes.
//
//------------------------------------------------------------------------------

/**
 * This class implements the Songbird service manager.
 */

class sbServiceManager : public sbIServiceManager
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Implemented interfaces.
  //

  NS_DECL_ISUPPORTS
  NS_DECL_SBISERVICEMANAGER


  //
  // Public services.
  //

  /**
   * Construct a Songbird service manager object.
   */
  sbServiceManager();

  /**
   * Destroy a Songbird service manager object.
   */
  virtual ~sbServiceManager();

  /**
   * Initialize the Songbird service manager.
   */
  nsresult Initialize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mInitialized               True if initialized.
  // mReadyServiceTable         Table of ready services.
  // mObserverService           Observer service.
  //

  PRBool                        mInitialized;
  nsDataHashtableMT<nsStringHashKey, PRBool>
                                mReadyServiceTable;
  nsCOMPtr<nsIObserverService>  mObserverService;
};

#endif // SB_SERVICE_MANAGER_H_

