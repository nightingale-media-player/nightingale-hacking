/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef _SB_FILE_UTILS_H_
#define _SB_FILE_UTILS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Nightingale file utilities defs.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbFileUtils.h
 * \brief Nightingale File Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Nightingale file utilities imported services.
//
//------------------------------------------------------------------------------

// Nightingale imports.
#include <sbIFileUtils.h>


//------------------------------------------------------------------------------
//
// Nightingale file utilities definitions.
//
//------------------------------------------------------------------------------

//
// Nightingale file utilities XPCOM component definitions.
//

#define SB_FILEUTILS_CLASSNAME "sbFileUtils"
#define SB_FILEUTILS_DESCRIPTION "Nightingale File Utilities"
#define SB_FILEUTILS_CID                                                       \
{                                                                              \
  0x88d4bc8a,                                                                  \
  0x1dd2,                                                                      \
  0x11b2,                                                                      \
  { 0xa2, 0x27, 0xd7, 0x46, 0xb1, 0xd0, 0x04, 0x58 }                           \
}


//------------------------------------------------------------------------------
//
// Nightingale file utilities classes.
//
//------------------------------------------------------------------------------

/**
 * This class provides various file utilities.
 */

class sbFileUtils : public sbIFileUtils
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
  NS_DECL_SBIFILEUTILS


  //
  // Public services.
  //

  sbFileUtils();

  virtual ~sbFileUtils();
};


#endif /* _SB_FILE_UTILS_H_ */

