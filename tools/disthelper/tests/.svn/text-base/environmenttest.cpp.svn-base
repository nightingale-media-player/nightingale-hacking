// vim: set sw=2 :
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Songbird Distribution Stub Helper.
 *
 * The Initial Developer of the Original Code is
 * POTI <http://www.songbirdnest.com/>.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *   Mook <mook@songbirdnest.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

#include "commands.h"
#include "utils.h"

#ifdef XP_WIN
  #include <tchar.h>
  #include <windows.h>
#else
  #include "tchar_compat.h"
#endif /* XP_WIN */

void check(int cond, const char* fmt, ...);

void TestEnvironment()
{
  tstring result;

  // non-Windows only: test actually unsetting the environment variables
  // (as opposed to set to empty strings) - there is no empty strings on
  // Windows.
  #if !defined(XP_WIN)
    unsetenv("DISTHELPER_ENV_TEST=");
    unsetenv("DISTHELPER_DISTHELPER_ENV_TEST=");
    result = FilterSubstitution(_T("@$DISTHELPER_ENV_TEST$@"));
    check(result == _T("@$DISTHELPER_ENV_TEST$@"),
          "FilterSubstitution should be failing, but got %s",
          result.c_str());
  #endif /* !XP_WIN */

  // Clear any environment variable we might already have
  _tputenv(_T("DISTHELPER_ENV_TEST="));
  _tputenv(_T("DISTHELPER_DISTHELPER_ENV_TEST="));
  
  // test the failing to find a value case
  result = FilterSubstitution(_T("@$DISTHELPER_ENV_TEST$@"));
  check(result == _T("@$DISTHELPER_ENV_TEST$@"),
        "FilterSubstitution should be failing, but got %s",
        result.c_str());

  // test the case where no prefix needs to be added
  _tputenv(_T("DISTHELPER_ENV_TEST=hello"));
  result = FilterSubstitution(_T("@$DISTHELPER_ENV_TEST$@"));
  check(result == _T("@hello@"),
        "FilterSubstitution should get @hello@, but got %s",
        result.c_str());

  // test that the DISTHELPER_ prefix is preferred
  _tputenv(_T("DISTHELPER_DISTHELPER_ENV_TEST=world"));
  result = FilterSubstitution(_T("@$DISTHELPER_ENV_TEST$@"));
  check(result == _T("@world@"),
        "FilterSubstitution should get @world@, but got %s",
        result.c_str());

  // test getting the application directory gives the same value
  result = FilterSubstitution(_T("$APPDIR$"));
  check(result == GetAppResoucesDirectory(),
        "FilterSubstitution should get application directory %s, but got %s",
        GetAppResoucesDirectory().c_str(), result.c_str());

  printf("TestEnvironment: PASS\n");
}
