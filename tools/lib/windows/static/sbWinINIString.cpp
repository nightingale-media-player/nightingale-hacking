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

/**
 * \file  sbWinINIString.cpp
 * \brief Nightingale Windows INI String Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale Windows INI string imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbWinINIString.h"

// Local imports.
#include "stringconvert.h"


//------------------------------------------------------------------------------
//
// Nightingale Windows INI string services prototypes.
//
//------------------------------------------------------------------------------

static DWORD sbGetPrivateProfileString(LPCSTR aAppName,
                                       LPCSTR aKeyName,
                                       LPCSTR aDefault,
                                       LPSTR  aReturnedString,
                                       DWORD  aSize,
                                       LPCSTR aFileName);

static DWORD sbGetPrivateProfileString(LPCWSTR aAppName,
                                       LPCWSTR aKeyName,
                                       LPCWSTR aDefault,
                                       LPWSTR  aReturnedString,
                                       DWORD   aSize,
                                       LPCWSTR aFileName);

static void sbApplyINISubstitutions(tstring& aINIString,
                                    tstring& aSection,
                                    tstring& aINIFilePath);


//------------------------------------------------------------------------------
//
// Nightingale Windows INI string services template functions.
//
//------------------------------------------------------------------------------

/**
 * Return in aINIString the INI string with the key specified by aKey within the
 * section specified by aSection of the INI file specified by aINIFilePath.  If
 * no INI string is found, use the default string specified by aDefault.
 *
 * \param aINIString            Returned INI string.
 * \param aSection              INI section.
 * \param aKey                  INI string key.
 * \param aINIFilePath          INI file path.
 * \param aDefault              Optional default string.
 */

template<class T>
void
sbWinGetINIString(tstring& aINIString,
                  const T* aSection,
                  const T* aKey,
                  const T* aINIFilePath,
                  const T* aDefault)
{
  // Read INI string, retrying if allocated buffer is not large enough.
  DWORD size = 256;
  while (1) {
    // Allocate the INI string buffer.
    sbAutoPtr<T> iniString = new T[size];
    if (!iniString)
      return;

    // Get the INI string.
    DWORD charCount = sbGetPrivateProfileString(aSection,
                                                aKey,
                                                aDefault,
                                                iniString,
                                                size,
                                                aINIFilePath);

    // If entire string was read or the maximum size was reached, return result.
    if ((charCount < (size - 1)) ||
        (size >= sbWinINIString::SB_WIN_INI_STRING_MAX_LENGTH)) {
      // Return result with INI substitutions applied.
      aINIString.assign(ConvertToUTFn(iniString));
      sbApplyINISubstitutions(aINIString, aSection, aINIFilePath);

      return;
    }

    // Try a larger buffer size;
    size *= 2;
  }
}


//------------------------------------------------------------------------------
//
// Nightingale Windows INI string public services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbWinINIString
//

sbWinINIString::sbWinINIString(LPCSTR aSection,
                               LPCSTR aKey,
                               LPCSTR aINIFilePath,
                               LPCSTR aDefault)
{
  sbWinGetINIString(*this, aSection, aKey, aINIFilePath, aDefault);
}


//-------------------------------------
//
// sbWinINIString
//

sbWinINIString::sbWinINIString(LPCWSTR aSection,
                               LPCWSTR aKey,
                               LPCWSTR aINIFilePath,
                               LPCWSTR aDefault)
{
  sbWinGetINIString(*this, aSection, aKey, aINIFilePath, aDefault);
}


//------------------------------------------------------------------------------
//
// Internal Nightingale Windows INI string services.
//
//------------------------------------------------------------------------------

/**
 * Overloaded wrapper for the GetPrivateProfileStringA function.
 */

DWORD
sbGetPrivateProfileString(LPCSTR aAppName,
                          LPCSTR aKeyName,
                          LPCSTR aDefault,
                          LPSTR  aReturnedString,
                          DWORD  aSize,
                          LPCSTR aFileName)
{
  return GetPrivateProfileStringA(aAppName,
                                  aKeyName,
                                  aDefault,
                                  aReturnedString,
                                  aSize,
                                  aFileName);
}


/**
 * Overloaded wrapper for the GetPrivateProfileStringW function.
 */

DWORD
sbGetPrivateProfileString(LPCWSTR aAppName,
                          LPCWSTR aKeyName,
                          LPCWSTR aDefault,
                          LPWSTR  aReturnedString,
                          DWORD   aSize,
                          LPCWSTR aFileName)
{
  return GetPrivateProfileStringW(aAppName,
                                  aKeyName,
                                  aDefault,
                                  aReturnedString,
                                  aSize,
                                  aFileName);
}


/**
 * Apply any INI string substitutions to the string specified by aINIString
 * using the INI section specified by aSection and INI file specified by
 * aINIFilePath.
 *
 * \param aINIString            String to which to apply substitutions.
 * \param aSection              INI section.
 * \param aINIFilePath          INI file path.
 */

void sbApplyINISubstitutions(tstring&       aINIString,
                             const tstring& aSection,
                             const tstring& aINIFilePath)
{
  tstring::size_type subStartIndex = 0;
  tstring::size_type subEndIndex = tstring::npos;
  while (1) {
    // Find the next embedded INI string.
    tstring::size_type subLength;
    subStartIndex = aINIString.find(tstring("&"), subStartIndex);
    if (subStartIndex == tstring::npos)
      break;
    subEndIndex = aINIString.find(tstring(";"), subStartIndex);
    if (subEndIndex == tstring::npos)
      break;
    subLength = subEndIndex + 1 - subStartIndex;

    // Get the INI string key.
    tstring subKey = aINIString.substr(subStartIndex + 1, subLength - 2);

    // Get the INI string, special casing "&amp;".
    tstring subString;
    if (subKey == tstring("amp")) {
      subString = "&";
    }
    else {
      sbWinGetINIString(subString,
                        aSection.c_str(),
                        subKey.c_str(),
                        aINIFilePath.c_str(),
                        static_cast<LPCTSTR>(NULL));
    }

    // Apply the embedded INI string.
    aINIString.replace(subStartIndex, subLength, subString);

    // Continue processing after the embedded INI string.
    subStartIndex += subString.size();
  }
}


