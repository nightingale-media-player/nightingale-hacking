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
 * \file  sbWindowsUtils.cpp
 * \brief Nightingale Windows Utilities Source.
 */

//------------------------------------------------------------------------------
//
// Nightingale Windows utilities imported services.
//
//------------------------------------------------------------------------------

// Self import.
#include "sbWindowsUtils.h"

// Local imports.
#include "stringconvert.h"


//------------------------------------------------------------------------------
//
// Nightingale tstring services.
//
//------------------------------------------------------------------------------

//
// Internal prototypes.
//

static int sb_vscprintf(LPCSTR  aFormat,
                        va_list aArgs);

static int sb_vscprintf(LPCWSTR aFormat,
                        va_list aArgs);

static int sb_vsnprintf_s(char*       aBuffer,
                          size_t      aBufferSize,
                          size_t      aCount,
                          const char* aFormat,
                          va_list     aArgs);

static int sb_vsnprintf_s(wchar_t*       aBuffer,
                          size_t         aBufferSize,
                          size_t         aCount,
                          const wchar_t* aFormat,
                          va_list        aArgs);


//
// Constructors.
//

tstring::tstring(const std::string& aString)
{
  #if defined(_UNICODE)
    assign(ConvertUTF8ToUTF16(aString));
  #else
    assign(aString);
  #endif
}

tstring::tstring(LPCSTR aString)
{
  #if defined(_UNICODE)
    assign(ConvertUTF8ToUTF16(aString));
  #else
    assign(aString);
  #endif
}

tstring::tstring(const std::wstring& aString)
{
  #if defined(_UNICODE)
    assign(aString);
  #else
    assign(ConvertUTF16ToUTF8(aString));
  #endif
}

tstring::tstring(LPCWSTR aString)
{
  #if defined(_UNICODE)
    assign(aString);
  #else
    assign(ConvertUTF16ToUTF8(aString));
  #endif
}


/**
 * Return in aString the formatted string with the format specified by aFormat
 * and arguments specified by aArgs.  Return the number of characters written or
 * a negative value on error.
 *
 * \param aString               Returned, formatted string.
 * \param aFormat               Format string.
 * \param aArgs                 Format arguments.
 *
 * \returns                     Number of characters written or a negative value
 *                              on error.
 */

template<class T>
int
sb_vprintf(tstring& aString,
           const T* aFormat,
           va_list  aArgs)
{
  // Default to an empty string.
  aString.clear();

  // Get the length of the formatted string.
  int length = sb_vscprintf(aFormat, aArgs);
  if (length <= 0)
    return length;

  // Allocate a temporary buffer to contain the formatted string.
  sbAutoPtr<T> buffer = new T[length + 1];
  if (!buffer)
    return -1;

  // Produce the formatted string.
  length = sb_vsnprintf_s(buffer,
                          (length + 1) * sizeof(T),
                          length,
                          aFormat,
                          aArgs);
  if (length <= 0)
    return length;

  // Return results.
  aString.assign(ConvertToUTFn(buffer));

  return length;
}


//-------------------------------------
//
// vprintf
//

int tstring::vprintf(LPCSTR  aFormat,
                     va_list aArgs)
{
  return sb_vprintf(*this, aFormat, aArgs);
}

int tstring::vprintf(LPCWSTR aFormat,
                     va_list aArgs)
{
  return sb_vprintf(*this, aFormat, aArgs);
}


/**
 * Overloaded wrapper for the _vscprintf function.  This is needed by the
 * sb_vprintf template so it can support CHAR and WCHAR types regardless of what
 * type TCHAR is.
 */

int sb_vscprintf(LPCSTR  aFormat,
                 va_list aArgs)
{
  return _vscprintf(aFormat, aArgs);
}


/**
 * Overloaded wrapper for the _vscwprintf function.  This is needed by the
 * sb_vprintf template so it can support CHAR and WCHAR types regardless of what
 * type TCHAR is.
 */

int sb_vscprintf(LPCWSTR aFormat,
                 va_list aArgs)
{
  return _vscwprintf(aFormat, aArgs);
}


/**
 * Overloaded wrapper for the _vsnprintf_s function.  This is needed by the
 * sb_vprintf template so it can support CHAR and WCHAR types regardless of what
 * type TCHAR is.
 */

int sb_vsnprintf_s(char*       aBuffer,
                   size_t      aBufferSize,
                   size_t      aCount,
                   const char* aFormat,
                   va_list     aArgs)
{
  return _vsnprintf_s(aBuffer, aBufferSize, aCount, aFormat, aArgs);
}


/**
 * Overloaded wrapper for the _vsnwprintf_s function.  This is needed by the
 * sb_vprintf template so it can support CHAR and WCHAR types regardless of what
 * type TCHAR is.
 */

int sb_vsnprintf_s(wchar_t*       aBuffer,
                   size_t         aBufferSize,
                   size_t         aCount,
                   const wchar_t* aFormat,
                   va_list        aArgs)
{
  return _vsnwprintf_s(aBuffer, aBufferSize, aCount, aFormat, aArgs);
}


//------------------------------------------------------------------------------
//
// Nightingale Windows registry services.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbWinWriteRegStr
//

HRESULT
sbWinWriteRegStr(HKEY           aKey,
                 const tstring& aSubKey,
                 const tstring& aValueName,
                 const tstring& aValue)
{
  LONG result;

  // Ensure the registry key exists and set it up for auto-disposal.
  HKEY key;
  result = RegCreateKeyEx(aKey,
                          aSubKey.c_str(),
                          0,
                          NULL,
                          REG_OPTION_NON_VOLATILE,
                          KEY_SET_VALUE,
                          NULL,
                          &key,
                          NULL);
  SB_WIN_ENSURE_TRUE(result == ERROR_SUCCESS, HRESULT_FROM_WIN32(result));
  sbAutoRegKey autoKey(key);

  // Set the registry key value.
  result = RegSetValueEx(key,
                         aValueName.c_str(),
                         0,
                         REG_SZ,
                         reinterpret_cast<const BYTE*>(aValue.c_str()),
                         (aValue.length() + 1) * sizeof(TCHAR));
  SB_WIN_ENSURE_TRUE(result == ERROR_SUCCESS, HRESULT_FROM_WIN32(result));

  return S_OK;
}


//-------------------------------------
//
// sbWinDeleteRegKey
//

HRESULT
sbWinDeleteRegKey(HKEY           aKey,
                  const tstring& aSubKey)
{
  LONG result;

  // Delete the registry key.
  result = RegDeleteKey(aKey, aSubKey.c_str());
  SB_WIN_ENSURE_TRUE(result == ERROR_SUCCESS, HRESULT_FROM_WIN32(result));

  return S_OK;
}


//-------------------------------------
//
// sbWinDeleteRegValue
//

HRESULT
sbWinDeleteRegValue(HKEY           aKey,
                    const tstring& aSubKey,
                    const tstring& aValueName)
{
  LONG result;

  // Open the registry key and set it up for auto-disposal.
  HKEY key;
  result = RegOpenKeyEx(aKey, aSubKey.c_str(), 0, KEY_SET_VALUE, &key);
  SB_WIN_ENSURE_TRUE(result == ERROR_SUCCESS, HRESULT_FROM_WIN32(result));
  sbAutoRegKey autoKey(key);

  // Delete the registry key value.
  result = RegDeleteValue(key, aValueName.c_str());
  SB_WIN_ENSURE_TRUE(result == ERROR_SUCCESS, HRESULT_FROM_WIN32(result));

  return S_OK;
}


//------------------------------------------------------------------------------
//
// Nightingale Windows environment services.
//
//------------------------------------------------------------------------------

//
// Internal prototypes.
//

static errno_t sb_tgetenv_s(size_t* aSize,
                            LPSTR   aBuffer,
                            size_t  aNumberOfElements,
                            LPCSTR  aVarName);

static errno_t sb_tgetenv_s(size_t* aSize,
                            LPWSTR  aBuffer,
                            size_t  aNumberOfElements,
                            LPCWSTR aVarName);


/**
 * Return in aEnvString the string for the environment variable with the name
 * specified by aEnvVarName.  If the environment variable is not set, used the
 * default string specified by aDefault.
 *
 * \param aEnvString            Returned environment variable string.
 * \param aEnvVarName           Environment variable name.
 * \param aDefault              Optional default string.
 */

template<class T>
void
sbWinGetEnvString(tstring& aEnvString,
                  const T* aEnvVarName,
                  const T* aDefault)
{
  errno_t error;

  // Set default result.
  if (aDefault)
    aEnvString.assign(ConvertToUTFn(aDefault));
  else
    aEnvString.clear();

  // Get the size of the environment variable string.
  size_t size;
  error = sb_tgetenv_s(&size, NULL, 0, aEnvVarName);
  if (error || !size)
    return;

  // Allocate the environment variable string buffer.
  sbAutoPtr<T> envString = new T[size];
  if (!envString)
    return;

  // Get the environment variable string.
  error = sb_tgetenv_s(&size, envString, size, aEnvVarName);
  if (error)
    return;

  // Return results.
  aEnvString.assign(ConvertToUTFn(envString));
}


//-------------------------------------
//
// sbEnvString
//

sbEnvString::sbEnvString(LPCSTR aEnvVarName,
                         LPCSTR aDefault)
{
  sbWinGetEnvString(*this, aEnvVarName, aDefault);
}

sbEnvString::sbEnvString(LPCWSTR aEnvVarName,
                         LPCWSTR aDefault)
{
  sbWinGetEnvString(*this, aEnvVarName, aDefault);
}


/**
 * Overloaded wrapper for the getenv_s function.
 */

errno_t
sb_tgetenv_s(size_t* aSize,
             LPSTR   aBuffer,
             size_t  aNumberOfElements,
             LPCSTR  aVarName)
{
  return getenv_s(aSize, aBuffer, aNumberOfElements, aVarName);
}


/**
 * Overloaded wrapper for the _wgetenv_s function.
 */

errno_t
sb_tgetenv_s(size_t* aSize,
             LPWSTR  aBuffer,
             size_t  aNumberOfElements,
             LPCWSTR aVarName)
{
  return _wgetenv_s(aSize, aBuffer, aNumberOfElements, aVarName);
}


//------------------------------------------------------------------------------
//
// Nightingale Windows utilities.
//
//------------------------------------------------------------------------------

//-------------------------------------
//
// sbWinGetModuleFilePath
//

HRESULT
sbWinGetModuleFilePath(HMODULE  aModule,
                       tstring& aModuleFilePath,
                       tstring& aModuleDirPath)
{
  errno_t err;

  // Get the module file path.
  TCHAR moduleFilePath[MAX_PATH];
  DWORD length;
  length = GetModuleFileName(aModule, moduleFilePath, _countof(moduleFilePath));
  SB_WIN_ENSURE_TRUE((length > 0) && (length < _countof(moduleFilePath)),
                     E_FAIL);

  // Get the module file path drive and directory.
  TCHAR drive[MAX_PATH];
  TCHAR dir[MAX_PATH];
  err = _tsplitpath_s(moduleFilePath,
                      drive,
                      _countof(drive),
                      dir,
                      _countof(dir),
                      NULL,
                      0,
                      NULL,
                      0);
  SB_WIN_ENSURE_TRUE(err == 0, E_FAIL);

  // Return results.
  aModuleFilePath.assign(moduleFilePath);
  aModuleDirPath.assign(drive);
  aModuleDirPath.append(dir);

  return S_OK;
}


