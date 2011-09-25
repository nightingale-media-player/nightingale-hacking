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

#ifndef _SB_TEMPORARY_FILE_SERVICE_H_
#define _SB_TEMPORARY_FILE_SERVICE_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale temporary file service defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbTemporaryFileService.h
 * \brief Nightingale Temporary File Service Definitions.
 */

//------------------------------------------------------------------------------
//
// Nightingale temporary file service imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
#include <sbITemporaryFileFactory.h>
#include <sbITemporaryFileService.h>

// Mozilla imports.
#include <nsCOMPtr.h>
#include <nsIObserver.h>
#include <nsIObserverService.h>


//------------------------------------------------------------------------------
//
// Nightingale temporary file service definitions.
//
//------------------------------------------------------------------------------

//
// Nightingale temporary file service XPCOM component definitions.
//

#define SB_TEMPORARYFILESERVICE_CLASSNAME "sbTemporaryFileService"
#define SB_TEMPORARYFILESERVICE_DESCRIPTION "Nightingale Temporary File Service"
#define SB_TEMPORARYFILESERVICE_CID                                            \
{                                                                              \
  0xa73256d2,                                                                  \
  0x1dd1,                                                                      \
  0x11b2,                                                                      \
  { 0x9f, 0xaa, 0xb9, 0x70, 0x98, 0x34, 0x17, 0x4c }                           \
}


//
// Nightingale temporary file service configuration.
//
//   SB_TEMPORARY_FILE_SERVICE_DIR_NAME
//                              Name of temporary file service root directory.
//

#define SB_TEMPORARY_FILE_SERVICE_ROOT_DIR_NAME "sbTemporaryFileService"


//------------------------------------------------------------------------------
//
// Nightingale temporary file service classes.
//
//------------------------------------------------------------------------------

/**
 * This class provides services for creating temporary files.
 */

class sbTemporaryFileService : public sbITemporaryFileService,
                               public nsIObserver
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
  NS_DECL_SBITEMPORARYFILESERVICE
  NS_DECL_NSIOBSERVER


  //
  // Public services.
  //

  /**
   * Construct a Nightingale temporary file service object.
   */
  sbTemporaryFileService();

  /**
   * Destroy a Nightingale temporary file service object.
   */
  virtual ~sbTemporaryFileService();

  /**
   * Initialize the Nightingale temporary file service.
   */
  nsresult Initialize();

  /**
   * Finalize the Nightingale temporary file service.
   */
  void Finalize();


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mInitialized               True if initialized.
  // mRootTemporaryFileFactory  Root temporary file factory.
  // mObserverService           Observer service.
  // mProfileAvailable          If true, user profile is available.
  //

  PRBool                        mInitialized;
  nsCOMPtr<sbITemporaryFileFactory>
                                mRootTemporaryFileFactory;
  nsCOMPtr<nsIObserverService>  mObserverService;
  PRBool                        mProfileAvailable;
};


#endif // _SB_TEMPORARY_FILE_SERVICE_H_

