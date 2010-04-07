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

#ifndef SB_BRAND_STRING_H_
#define SB_BRAND_STRING_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird brand string services defs.
//
//   The Songbird brand string services provide support for using brand specific
// Songbird strings.  This allows Songbird partners to customize strings used by
// Songbird.
//   The brand strings are specified in a branding INI file inside a section
// named "Branding".  The branding INI file may be specified by the
// "SB_BRANDING_INI_FILE" environment variable.  If this variable is not set,
// the brinding INI file residing in the running executable directory with the
// name "sbBrandingStrings.ini" is used.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbBrandString.h
 * \brief Songbird Brand String Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird brand string imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbWindowsUtils.h"


//------------------------------------------------------------------------------
//
// Songbird brand string configuration.
//
//------------------------------------------------------------------------------

//
// SB_BRAND_INI_FILE_ENV_VAR_NAME   Name of environment variable specifying the
//                                  brand ini file.
// SB_BRAND_DEFAULT_INI_FILE        Default brand INI file name.
//

#define SB_BRAND_INI_FILE_ENV_VAR_NAME "SB_BRANDING_INI_FILE"
#define SB_BRAND_DEFAULT_INI_FILE "sbBrandingStrings.ini"


//------------------------------------------------------------------------------
//
// Songbird brand string classes.
//
//------------------------------------------------------------------------------

/**
 * This class may be used to retrieve brand specific strings as tstrings.
 */

class sbBrandString : public tstring
{
  //----------------------------------------------------------------------------
  //
  // Public interface.
  //
  //----------------------------------------------------------------------------

public:

  /**
   * Construct a brand string using the brand string with the key specified by
   * aKey.  If the brand string does not exist, use the default string specified
   * by aDefault.  If no default is given, use the key as the string.
   *
   * \param aKey                Brand string key.
   * \param aDefault            Optional default string.
   */
  sbBrandString(LPCWSTR aKey,
                LPCWSTR aDefault = NULL);

  /**
   * Construct a brand string using the brand string with the key specified by
   * aKey.  If the brand string does not exist, use the default string specified
   * by aDefault.  If no default is given, use the key as the string.
   *
   * \param aKey                Brand string key.
   * \param aDefault            Optional default string.
   */
  sbBrandString(LPCSTR aKey,
                LPCSTR aDefault = NULL);


  //----------------------------------------------------------------------------
  //
  // Private interface.
  //
  //----------------------------------------------------------------------------

private:

  //
  // Private services.
  //

  /**
   * Initialize the Songbird brand string using the brand string with the key
   * specified by aKey.  If the brand string does not exist, use the default
   * string specified by aDefault.
   *
   * \param aKey                Brand string key.
   * \param aDefault            Optional default string.
   */
  void Initialize(const tstring& aKey,
                  const tstring& aDefault);


  //
  // Private class services.
  //

  //
  // sClassInitialized          If true, class has been initialized.
  // sBrandingINIFilePath       File path of branding INI file.
  //

  static BOOL                   sClassInitialized;
  static tstring*               sBrandingINIFilePath;


  /**
   * Initialize the class services.
   */
  static HRESULT ClassInitialize();

  /**
   * Finalize the class services.
   */
  static void ClassFinalize();
};


#endif // SB_BRAND_STRING_H_

