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

#ifndef _SB_TEMPORARY_FILE_FACTORY_H_
#define _SB_TEMPORARY_FILE_FACTORY_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird temporary file factory defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbTemporaryFileFactory.h
 * \brief Songbird Temporary File Factory Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird temporary file factory imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
#include <sbITemporaryFileFactory.h>

// Mozilla imports.
#include <nsCOMPtr.h>


//------------------------------------------------------------------------------
//
// Songbird temporary file factory definitions.
//
//------------------------------------------------------------------------------

//
// Songbird temporary file factory XPCOM component definitions.
//

#define SB_TEMPORARYFILEFACTORY_CLASSNAME "sbTemporaryFileFactory"
#define SB_TEMPORARYFILEFACTORY_DESCRIPTION "Songbird Temporary File Factory"
#define SB_TEMPORARYFILEFACTORY_CID                                            \
{                                                                              \
  0x33ecad26,                                                                  \
  0x1dd2,                                                                      \
  0x11b2,                                                                      \
  { 0xb1, 0x1b, 0x84, 0xc2, 0x15, 0xc3, 0x10, 0xa5 }                           \
}


//------------------------------------------------------------------------------
//
// Songbird temporary file factory classes.
//
//------------------------------------------------------------------------------

/**
 * This class provides services for creating temporary files.
 */

class sbTemporaryFileFactory : public sbITemporaryFileFactory
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
  NS_DECL_SBITEMPORARYFILEFACTORY


  //
  // Public services.
  //

  /**
   * Construct a Songbird temporary file factory object.
   */
  sbTemporaryFileFactory();

  /**
   * Destroy a Songbird temporary file factory object.
   */
  virtual ~sbTemporaryFileFactory();


  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // mRootTemporaryDirectory    Root temporary directory.
  //

  nsCOMPtr<nsIFile> mRootTemporaryDirectory;


  //
  // Public services.
  //

  /**
   * Ensure that the root temporary directory is set and exists.
   */
  nsresult EnsureRootTemporaryDirectory();
};


#endif // _SB_TEMPORARY_FILE_FACTORY_H_

