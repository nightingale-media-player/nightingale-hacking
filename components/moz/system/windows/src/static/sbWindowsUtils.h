/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

#ifndef __SB_WINDOWS_UTILS_H__
#define __SB_WINDOWS_UTILS_H__

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

#define SB_WIN_WARNING(aMessage)                                               \
    sbWindowsEventLog("WARNING: %s: file %s, line %d",                         \
                      aMessage, __FILE__, __LINE__)


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
//   sbAutoHANDLE               Wrapper to auto-close Windows handles.
//   sbAutoRegKey               Wrapper to automatically close a Windows
//                              registry key.
//   sbAutoIUnknown             Wrapper to automatically release an IUnknown
//                              object.
//

SB_AUTO_CLASS(sbAutoHANDLE,
              HANDLE,
              mValue != INVALID_HANDLE_VALUE,
              CloseHandle(mValue),
              mValue = INVALID_HANDLE_VALUE);
SB_AUTO_NULL_CLASS(sbAutoRegKey, HKEY, RegCloseKey(mValue));
SB_AUTO_NULL_CLASS(sbAutoIUnknown, IUnknown*, mValue->Release());


#endif // __SB_WINDOWS_UTILS_H__

