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
 * Portions created by the Initial Developer are Copyright (C) 2008-2010
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
#include "debug.h"
#include "error.h"
#include "readini.h"
#include "stringconvert.h"

#include <windows.h>
#include <shlwapi.h>
#include <shlobj.h>

/**
 * Get the update status directory's parent
 */
tstring GetUpdateRoot() {
  HRESULT hr;
  TCHAR appDirFull[MAX_PATH];
  const DWORD appDirFullLen = sizeof(appDirFull) / sizeof(appDirFull[0]);
  tstring appDir = GetAppResoucesDirectory();

  // AppDir may be a short path. Convert to long path to make sure
  // the consistency of the update folder location
  DWORD len = ::GetLongPathNameW(appDir.c_str(), appDirFull, appDirFullLen);
  if (len > 0 && len < appDirFullLen) {
    appDir = tstring(appDirFull);
  }
  else {
    // failing to get a long path name isn't fatal
  }

  // Use <UserLocalDataDir>\updates\<relative path to app dir from
  // Program Files> if app dir is under Program Files to avoid the
  // folder virtualization mess on Windows Vista
  TCHAR buffer[MAX_PATH];
  hr = SHGetFolderPath(NULL,                /* hwndOwner */
                       CSIDL_PROGRAM_FILES, /* nFolder */
                       NULL,                /* hToken */
                       SHGFP_TYPE_CURRENT,  /* flags */
                       buffer);
  if (FAILED(hr)) {
    return appDir;
  }
  tstring programFiles(buffer);
  programFiles.append(_T("\\"));
  if (_wcsnicmp(programFiles.c_str(), appDir.c_str(), programFiles.size())) {
    DebugMessage("application directory <%s> does not start with program files <%s>",
                 appDir.c_str(), programFiles.c_str());
    // appDir does not start with program files
    return appDir;
  }
  
  hr = SHGetFolderPath(NULL,                /* hwndOwner */
                       CSIDL_LOCAL_APPDATA, /* nFolder */
                       NULL,                /* hToken */
                       SHGFP_TYPE_CURRENT,  /* flags */
                       buffer);
  if (FAILED(hr)) {
    return appDir;
  }
  tstring localData(buffer);
  localData.append(_T("\\"));
  // append the profile name
  { /* scope */
    tstring appIni(appDir);
    appIni.append(_T("distribution\\application.ini"));
    DebugMessage("reading profile path from <%s>", appIni.c_str());
    TCHAR profBuffer[0x100];
    DWORD charsRead = ::GetPrivateProfileString(_T("App"),
                                                _T("Profile"),
                                                _T("Nightingale"),
                                                profBuffer,
                                                sizeof(profBuffer)/sizeof(profBuffer[0]),
                                                appIni.c_str());
    if (charsRead) {
      localData.append(profBuffer);
      localData.append(_T("\\"));
    }
  }
  localData.append(appDir, programFiles.size(), tstring::npos);
  DebugMessage("using update directory <%s>", localData.c_str());
  return localData;
}

int SetupEnvironment() {
  tstring envFile = GetUpdateRoot();
  envFile.append(_T("updates\\0\\disthelper.env"));
  
  if (!::PathFileExists(envFile.c_str())) {
    DebugMessage("environment file %s not found", envFile.c_str());
    // the environment file not existing is not a fatal error
    return DH_ERROR_OK;
  }

  DebugMessage("reading saved environment from <%s>", envFile.c_str());
  IniFile_t iniData;
  int result = ReadIniFile(envFile.c_str(), iniData);
  if (result) {
    return result;
  }
  IniEntry_t::const_iterator it = iniData[""].begin(),
                             end = iniData[""].end();
  result = DH_ERROR_OK;
  const char PREFIX[] = "DISTHELPER_";
  for (; it != end; ++it) {
    if (strncmp(PREFIX, it->first.c_str(), sizeof(PREFIX) - 1)) {
      // variable does not start with DISTHELPER_; ignore it to avoid possible
      // security problems
      continue;
    }
    if (!::SetEnvironmentVariable(ConvertUTF8toUTFn(it->first).c_str(),
                                  ConvertUTF8toUTFn(it->second).c_str()))
    {
      result = DH_ERROR_UNKNOWN;
    }
  }
  return result;
}
