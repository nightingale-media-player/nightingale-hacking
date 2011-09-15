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

//
// Currently, this is only used to define the SB_UNUSED_IN_RELEASE macro,
// which is used for variables that don't get referenced in release builds.
//
// The common case seen throughout our codebase is:
//    nsresult rv = sbFoo(&foo, &bar);
//    NS_ASSERTION(NS_ENSURE_SUCCESS(rv, rv))
//
// This will generate an error in release builds from the warning for an 
// unused rv if DISABLE_DEADLY_WARNINGS isn't on (because NS_ASSERTION()s
// get removed.
//
// Code like this gets changed to:
//   nsresult SB_UNUSED_IN_RELEASE(rv) =
//    sbFoo(&foo, &bar);
//   NS_ASSERTION(NS_SUCCEEDED(rv), "Oh no!")
//

#ifndef SBDEBUGUTILS_H_
#define SBDEBUGUTILS_H_

#include <prlog.h>

#if defined(__GNUC__)
  #if defined(DEBUG)
    #define SB_UNUSED_IN_RELEASE(decl) decl
  #else
    #define SB_UNUSED_IN_RELEASE(decl) decl __attribute__((unused))
  #endif 
#else
  #define SB_UNUSED_IN_RELEASE(decl) decl
#endif

#if defined(_MSC_VER)
  #if !defined(DEBUG) 
    // Disable warnings about unused local variables
    #pragma warning(disable: 4101)
  #endif
#endif

#if PR_LOGGING
  namespace {
    static PRLogModuleInfo* gPRLogModule = nsnull;
    
    #define SB_PRLOG_SETUP(aModuleName)                               \
      PR_BEGIN_MACRO                                                  \
        if (!gPRLogModule)                                            \
          gPRLogModule = PR_NewLogModule(#aModuleName);               \
      PR_END_MACRO
  
    #define TRACE(fmt, ...)                                           \
      PR_BEGIN_MACRO                                                  \
        if (PR_LOG_TEST(gPRLogModule, PR_LOG_DEBUG)) {                \
          PR_LogPrint("%s: " fmt, __FUNCTION__, ##__VA_ARGS__);       \
        }                                                             \
      PR_END_MACRO
  
    #define LOG(fmt, ...)                                             \
      PR_BEGIN_MACRO                                                  \
        if (PR_LOG_TEST(gPRLogModule, PR_LOG_WARN)) {                 \
          PR_LogPrint("%s: " fmt, __FUNCTION__, ##__VA_ARGS__);       \
        }                                                             \
      PR_END_MACRO

    struct _sbTraceFunctionClass {
      _sbTraceFunctionClass(const char* function) {
        mFunction = function;
      }
      ~_sbTraceFunctionClass() {
        if (PR_LOG_TEST(gPRLogModule, PR_LOG_DEBUG)) {
          PR_LogPrint("%s() <--", mFunction);
        }
      }
      const char* mFunction;
    };

    #if defined(__GNUC__)
      #define __FUNCTION__ __PRETTY_FUNCTION__
    #endif
  
    #define TRACE_FUNCTION_VAR_NAME2(x) SB_TRACE_FUNCTION_VAR_ ## x
    #define TRACE_FUNCTION_VAR_NAME(x) TRACE_FUNCTION_VAR_NAME2(x)
    #define TRACE_FUNCTION(fmt, ...)                                        \
      if (PR_LOG_TEST(gPRLogModule, PR_LOG_DEBUG)) {                        \
        PR_LogPrint("%s(" fmt ") -->", __FUNCTION__, ##__VA_ARGS__);        \
      }                                                                     \
      _sbTraceFunctionClass TRACE_FUNCTION_VAR_NAME(__LINE__)(__FUNCTION__)
  };
    
#else
  #define SB_PRLOG_SETUP(x)   PR_BEGIN_MACRO PR_END_MACRO
  #define TRACE(...)          PR_BEGIN_MACRO PR_END_MACRO
  #define LOG(...)            PR_BEGIN_MACRO PR_END_MACRO
  #define TRACE_FUNCTION(...) PR_BEGIN_MACRO PR_END_MACRO
#endif

#endif /* SBDEBUGUTILS_H_ */
