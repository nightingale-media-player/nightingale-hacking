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

#ifndef SB_WINDOWS_UTILS_H_
#define SB_WINDOWS_UTILS_H_

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Songbird Windows utilities services defs.
//
//   The Songbird Windows utilities do not depend upon any Mozilla libraries.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * \file  sbWindowsUtils.h
 * \brief Songbird Windows Utilities Definitions.
 */

//------------------------------------------------------------------------------
//
// Songbird Windows utilities compile warning work-arounds.
//
//------------------------------------------------------------------------------

//
// Include these first to avoid deprecated name warnings.
//
//   C:\Program Files\Microsoft Visual Studio 8\VC\INCLUDE\cstdio(33) :
//   warning C4995 : 'gets': name was marked as #pragma deprecated
//
#include <cstdio>
#include <cstring>
#include <cwchar>


//
// Avoid redefine warnings in strsafe.h.
//
//   C:\Program Files\Microsoft SDKs\Windows\v6.0\include\strsafe.h(60) :
//   warning C4005: 'SUCCEEDED' : macro redefinition;
//   c:\client\trunk\dependencies\windows-i686-msvc8\private\windows_ddk\
//   release\include\winerror.h(16682) : see previous definition of 'SUCCEEDED'
//

#include <windows.h>
#undef S_OK
#undef SUCCEEDED
#undef FAILED
#include <tchar.h>
#include <strsafe.h>


//------------------------------------------------------------------------------
//
// Songbird Windows utilities imported services.
//
//------------------------------------------------------------------------------

// Local imports.
#include "sbWindowsEventLog.h"

// Songbird imports.
#include <sbMemoryUtils.h>

// Windows imports.
#include <objbase.h>
#include <tchar.h>

#include <strsafe.h>

// C++ STL imports.
#include <string>


//------------------------------------------------------------------------------
//
// Songbird tstring services.
//
//------------------------------------------------------------------------------

/**
 * This clas implements a TCHAR version of the STL basic_string class.  It
 * provides support for constructing tstrings from both UTF-8 and UTF-16
 * strings.
 */

typedef std::basic_string<TCHAR,
                          std::char_traits<TCHAR>,
                          std::allocator<TCHAR> > tstringBase;

class tstring : public tstringBase
{

public:

  //
  // Constructors.
  //

  tstring() {}

  tstring(tstringBase& aString) { assign(aString); }

  tstring(LPCSTR aString);

  tstring(const std::string& aString);

  tstring(LPCWSTR aString);

  tstring(const std::wstring& aString);


  //
  // Operators.
  //

  operator LPCTSTR() const
  {
    return c_str();
  }


  //
  // Public services.
  //

  /**
   * Assign the formatted string with the format specified by aFormat and
   * arguments specified by aArgs.  Return the number of characters written or a
   * negative value on error.
   *
   * \param aFormat             Format string.
   * \param aArgs               Format arguments.
   *
   * \returns                   Number of characters written or a negative value
   *                            on error.
   */
  int vprintf(LPCSTR  aFormat,
              va_list aArgs);
  int vprintf(LPCWSTR aFormat,
              va_list aArgs);
};


//------------------------------------------------------------------------------
//
// Songbird Windows debug services macros.
//
//------------------------------------------------------------------------------

/**
 * Post a warning with the message specified by aMessage.
 *
 * \param aMessage              Warning message.
 */

#ifdef SB_LOG_TO_WINDOWS_EVENT_LOG
#define SB_WIN_WARNING(aMessage)                                               \
    sbWindowsEventLog("WARNING: %s: file %s, line %d",                         \
                      aMessage, __FILE__, __LINE__)
#else
#define SB_WIN_WARNING(aMessage)                                               \
    printf("WARNING: %s: file %s, line %d\n", aMessage, __FILE__, __LINE__)
#endif


/**
 * Post a warning with the message specified by aMessage if the condition
 * expression specified by aCondition evaluates to false.
 *
 * \param aCondition            Condition expression.
 * \param aMessage              Warning message.
 */

#define SB_WIN_WARN_IF_FALSE(aCondition, aMessage)                             \
  do {                                                                         \
    if (!(aCondition)) {                                                       \
      SB_WIN_WARNING(aMessage);                                                \
    }                                                                          \
  } while (0)


/**
 * Post warning on failed ensure success using the result and return value
 * expressions specified by aResult and aReturnValue and the evaluated result
 * specified by aEvalResult.
 *
 * \param aResult               Result expression.
 * \param aReturnValue          Return value expression.
 * \param aEvalResult           Evaluated result.
 */

#define SB_WIN_ENSURE_SUCCESS_BODY(aResult, aReturnValue, aEvalResult)         \
  do {                                                                         \
    char message[256];                                                         \
    StringCchPrintfA(message,                                                  \
                     sizeof(message) / sizeof(message[0]),                     \
                     "SB_WIN_ENSURE_SUCCESS(%s, %s) failed with "              \
                     "result 0x%X",                                            \
                     #aResult,                                                 \
                     #aReturnValue,                                            \
                     aEvalResult);                                             \
    SB_WIN_WARNING(message);                                                   \
  } while (0)


/**
 * Ensure that the result expression specified by aResult evaluates to a
 * successfuly Windows result code.  If it doesn't, post a warning and return
 * from the current function with the return value specified by aReturnValue.
 *
 * \param aResult               Result expression.
 * \param aReturnValue          Return value expression.
 */

#define SB_WIN_ENSURE_SUCCESS(aResult, aReturnValue)                           \
  do {                                                                         \
    HRESULT evalResult = aResult;                                              \
    if (FAILED(evalResult)) {                                                  \
      SB_WIN_ENSURE_SUCCESS_BODY(aResult, aReturnValue, evalResult);           \
      return aReturnValue;                                                     \
    }                                                                          \
  } while (0)


/**
 * Ensure that the condition expression specified by aCondition evaluates to
 * true.  If it doesn't, post a warning and return from the current function
 * with the return value specified by aReturnValue.
 *
 * \param aCondition            Condition expression.
 * \param aReturnValue          Return value expression.
 */

#define SB_WIN_ENSURE_TRUE(aCondition, aReturnValue)                           \
  do {                                                                         \
    if (!(aCondition)) {                                                       \
      SB_WIN_WARNING("SB_WIN_ENSURE_TRUE(" #aCondition ") failed");            \
      return aReturnValue;                                                     \
    }                                                                          \
  } while (0)


/**
 * Ensure that the function argument specified by aArg is a valid pointer
 * argument.  If it isn't, post a warning and return from the current function
 * with an E_POINTER error.
 *
 * \param aArg                  Function argument to ensure is a pointer.
 */

#define SB_WIN_ENSURE_ARG_POINTER(aArg) \
  SB_WIN_ENSURE_TRUE(aArg, E_POINTER)


//------------------------------------------------------------------------------
//
// Songbird Windows auto-disposal services macros.
//
//------------------------------------------------------------------------------

//
// Auto-disposal class wrappers.
//
//   sbAutoPtr                  Wrapper to auto-delete classes.  Use in place of
//                              nsAutoPtr when writing native Windows apps.
//   sbAutoHANDLE               Wrapper to auto-close Windows handles.
//   sbAutoRegKey               Wrapper to automatically close a Windows
//                              registry key.
//   sbAutoIUnknown             Wrapper to automatically release an IUnknown
//                              object.
//   sbAutoLocalFree            Wrapper to automatically free a local memory
//                              object.
//

template<typename T> SB_AUTO_NULL_CLASS(sbAutoPtr, T*, delete mValue);
SB_AUTO_CLASS(sbAutoHANDLE,
              HANDLE,
              mValue != INVALID_HANDLE_VALUE,
              CloseHandle(mValue),
              mValue = INVALID_HANDLE_VALUE);
SB_AUTO_NULL_CLASS(sbAutoRegKey, HKEY, RegCloseKey(mValue));
SB_AUTO_NULL_CLASS(sbAutoIUnknown, IUnknown*, mValue->Release());
SB_AUTO_NULL_CLASS(sbAutoLocalFree, HLOCAL, LocalFree(mValue));


//------------------------------------------------------------------------------
//
// Songbird Windows registry services.
//
//------------------------------------------------------------------------------

/**
 * Set the value of the Windows registry key specified by aKey and aSubKey with
 * the value name specified by aValueName to the value specified by aValue.
 * Create the key if necessary.  If aValueName is an empty string, set the
 * default key value.
 *
 * \param aKey                  The base registry key.
 * \param aSubKey               The registry sub-key to which to write.
 * \param aValueName            Name of the value to write.
 * \param aValue                Value to write.
 */
HRESULT sbWinWriteRegStr(HKEY           aKey,
                         const tstring& aSubKey,
                         const tstring& aValueName,
                         const tstring& aValue);

/**
 * Delete the Windows registry key specified by aKey and aSubKey.
 *
 * \param aKey                  The base registry key.
 * \param aSubKey               The registry sub-key to delete.
 */
HRESULT sbWinDeleteRegKey(HKEY           aKey,
                          const tstring& aSubKey);

/**
 * Delete the Windows registry value for the key specified by aKey and aSubKey
 * and value name specified by aValueName.
 *
 * \param aKey                  The base registry key.
 * \param aSubKey               The registry sub-key from which to delete value.
 * \param aValueName            Name of value to delete.
 */
HRESULT sbWinDeleteRegValue(HKEY           aKey,
                            const tstring& aSubKey,
                            const tstring& aValueName);


//------------------------------------------------------------------------------
//
// Songbird Windows environment services.
//
//------------------------------------------------------------------------------

/**
 * This class may be used to retrieve environment variable strings as tstrings.
 */

class sbEnvString : public tstring
{

public:

  /**
   * Construct an environment variable string using the environment variable
   * with the name specified by aEnvVarName.  If the environment variable is not
   * set, use the default string specified by aDefault.
   *
   * \param aEnvVarName         Environment variable name.
   * \param aDefault            Optional default string.
   */
  sbEnvString(LPCSTR aEnvVarName,
              LPCSTR aDefault = NULL);
  sbEnvString(LPCWSTR aEnvVarName,
              LPCWSTR aDefault = NULL);
};


//------------------------------------------------------------------------------
//
// Songbird Windows utilities.
//
//------------------------------------------------------------------------------

/**
 * Return in aModuleFilePath and aModuleDirPath the file path and directory path
 * for the module specified by aModule.
 *
 * \param aModule               Module for which to get path.
 * \param aModuleFilePath       Module file path.
 * \param aModuleDirPath        Module directory path.
 */
HRESULT sbWinGetModuleFilePath(HMODULE  aModule,
                               tstring& aModuleFilePath,
                               tstring& aModuleDirPath);


#endif // SB_WINDOWS_UTILS_H_

