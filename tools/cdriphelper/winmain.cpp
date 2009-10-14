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

#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#include <stdlib.h>
#include <vector>

#include "stringconvert.h"
#include "error.h"
#include "reghandlers.h"
#include "toolslib.h"
#include "debug.h"

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
  // don't use any of the args, because we want to be Unicode-compatible
  
  LPWSTR commandLine = GetCommandLineW();
  if (!commandLine) {
    OutputDebugString(_T("Failed to get command line!"));
    return RH_ERROR_USER;
  }
  
  int argc;
  LPWSTR *argvw = CommandLineToArgvW(commandLine, &argc);
  if (!argvw) {
    OutputDebugString(_T("Failed to parse command line!"));
    return RH_ERROR_USER;
  }
  
  int result = -1;

  if (argc != 2) {
    OutputDebugString(_T("Incorrect number of arguments"));
    return RH_ERROR_USER;
  }

  std::string mode;
  mode.assign(ConvertUTFnToUTF8(argvw[1]));

  if (mode == "install") {
    OutputDebugString(_T("In Install mode"));
    result = InstallAspiDriver();

  } else if (mode == "upgrade") {
    OutputDebugString(_T("In Upgrade mode"));

    tstring appDir = GetAppDirectory();
    if (CheckAspiDriversInstalled(appDir.c_str()) != RH_OK)
       result = InstallAspiDriver();
    else
       result = RH_SUCCESS_NOACTION;

  } else if ("remove" == mode) {
    OutputDebugString(_T("In Remove mode"));
    result = RemoveAspiDriver();

  } else if ("repair" == mode) {
    OutputDebugString(_T("In Repair mode"));
    result = InstallAspiDriver();

  } else {
    OutputDebugString(_T("Unknown mode"));
    result = RH_ERROR_USER;
  }

  DebugMessage("%S returning %d", argvw[0], result);

  LocalFree(argvw);
  return result;
}
