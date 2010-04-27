/* vim: le=unix sw=2 : */
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

#include "utils.h"

#include "commands.h"
#include "stringconvert.h"

#ifdef XP_WIN
  #include <windows.h>
  #include <tchar.h>
#else
  #include <tchar_compat.h>
#endif /* XP_WIN */

tstring FilterSubstitution(tstring aString) {
  tstring result = aString;
  tstring::size_type start = 0, end = tstring::npos;
  TCHAR *envData;
  while (true) {
    start = result.find(tstring::value_type('$'), start);
    if (start == tstring::npos) {
      break;
    }
    end = result.find(tstring::value_type('$'), start + 1);
    if (end == tstring::npos) {
      break;
    }
    // Try to substitute $APPDIR$
    tstring variable = result.substr(start + 1, end - start - 1);
    if (variable == _T("APPDIR")) {
      tstring appdir = GetAppResoucesDirectory();
      DebugMessage("AppDir: %s", appdir.c_str());
      result.replace(start, end-start+1, appdir);
      start += appdir.length();
      continue;
    }
    // Try to substitute $XXX$ with environment variable %DISTHELPER_XXX%
    tstring envName(_T("DISTHELPER_"));
    envName.append(variable);
    envData = _tgetenv(envName.c_str());
    if (envData && *envData) {
      tstring envValue(envData);
      DebugMessage("Environment %s: %s", envName.c_str(), envValue.c_str());
      result.replace(start, end-start+1, envValue.c_str());
      start += envValue.length();
      continue;
    }
    // Try to substitute $XXX$ with environment variable %XXX%
    envData = _tgetenv(variable.c_str());
    if (envData && *envData) {
      tstring envValue(envData);
      DebugMessage("Environment %s: %s", envName.c_str(), envValue.c_str());
      result.replace(start, end-start+1, envValue.c_str());
      start += envValue.length();
      continue;
    }
    start = end + 1;
  }
  return result;
}
