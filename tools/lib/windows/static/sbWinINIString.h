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

#ifndef SB_WIN_INI_STRING_H_
#define SB_WIN_INI_STRING_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Windows INI string services defs.
//
//   These services provide support for reading strings from INI files.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWinINIString.h
 * \brief Songbird Windows INI String Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows INI string imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbWindowsUtils.h"


//------------------------------------------------------------------------------
//
// Songbird Windows INI string classes.
//
//------------------------------------------------------------------------------

/**
 * This class may be used to retrieve INI strings as tstrings.
 */

class sbWinINIString : public tstring
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  //
  // Configuration
  //
  // SB_WIN_INI_STRING_MAX_LENGTH Maximum length of an INI string.
  //

  static const int SB_WIN_INI_STRING_MAX_LENGTH = 65536;


  /**
   * Construct an INI string using the INI string with the key specified by aKey
   * within the section specified by aSection of the INI file specified by
   * aINIFilePath.  If no INI string is found, use the default string specified
   * by aDefault.
   *
   * \param aSection            INI section.
   * \param aKey                INI string key.
   * \param aINIFilePath        INI file path.
   * \param aDefault            Optional default string.
   */
  sbWinINIString(LPCSTR aSection,
                 LPCSTR aKey,
                 LPCSTR aINIFilePath = NULL,
                 LPCSTR aDefault = NULL);
  sbWinINIString(LPCWSTR aSection,
                 LPCWSTR aKey,
                 LPCWSTR aINIFilePath = NULL,
                 LPCWSTR aDefault = NULL);
};

#endif // SB_WIN_INI_STRING_H_

