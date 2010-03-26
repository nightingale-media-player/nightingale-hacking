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

#ifdef XP_WIN
#include <windows.h>
#include <tchar.h>
#include <shellapi.h>
#else
#include "tchar.h"
#endif
#include <stdlib.h>
#include <vector>

#include "readini.h"
#include "stringconvert.h"
#include "commands.h"
#include "error.h"
#include "debug.h"

/*
 * disthelper
 * usage: $0 <mode> [<ini file>]
 * the mode is used to find the [steps:<mode>] section in the ini file
 * steps are sorted in ascii order.  (not numerical order, 2 > 10!)
 * if the ini file is not given, the environment variable DISTHELPER_DISTINI 
 * is used (must be a platform-native absolute path)
 */

int main(int argc, LPTSTR *argv) {
  int result = 0;
  
  if (argc != 2 && argc != 3) {
    OutputDebugString(_T("Incorrect number of arguments"));
    return DH_ERROR_USER;
  }

  LPTSTR ininame;
  tstring distIni;
  bool usingFallback = false;
  if (argc > 2) {
    ininame = argv[2];
  } else {
    ininame = _tgetenv(_T("DISTHELPER_DISTINI"));
    if (!ininame) {
      DebugMessage("No ini file specified; using fallback");
      ininame = _T("songbird.ini");
      usingFallback = true;
    }
  }
  distIni = GetDistIniDirectory(ininame);
  if (distIni.empty()) {
    DebugMessage("Failed to find distribution.ini");
    return DH_ERROR_USER;
  }
  distIni = ininame;
  
  tstring srcAppIniName(GetDistIniDirectory()),
          destAppIniName(ResolvePathName("$/application.ini"));
  srcAppIniName.append(_T("application.ini"));
  
  /// check for the application.ini versions
  // we don't do this during uninstall (because there is no new file), nor
  // if we're using fallback (because there's no distribution)
  if (ConvertUTFnToUTF8(argv[1]) != "uninstall" && !usingFallback) {
    IniFile_t srcAppIni, destAppIni;
    result = ReadIniFile(srcAppIniName.c_str(), srcAppIni);
    if (result) {
      LogMessage("Failed to read source application.ini file %S: %i",
                 srcAppIniName.c_str(), result);
      ShowFatalError("Failed to read source application.ini file %s: %i",
                     srcAppIniName.c_str(), result);
      return result;
    }
    result = ReadIniFile(destAppIniName.c_str(), destAppIni);
    if (result) {
      LogMessage("Failed to read destination application.ini file %S: %i",
                 destAppIniName.c_str(), result);
      ShowFatalError("Failed to read destination application.ini file %s: %i",
                     destAppIniName.c_str(), result);
      return result;
    }
    
    std::string srcAppVer = srcAppIni["app"]["BuildID"],
                destAppVer = destAppIni["app"]["BuildID"];
    LogMessage("Checking application.ini build IDs... old=[%s] new=[%s]",
               srcAppVer.c_str(), destAppVer.c_str());
    if (srcAppVer != destAppVer) {
      LogMessage("source and destination application.ini are for different builds! (%s / %s)",
                 srcAppVer.c_str(),
                 destAppVer.c_str());
      ShowFatalError("source and destination application.ini are for different builds! (%S / %S)",
                     srcAppVer.c_str(),
                     destAppVer.c_str());
      return DH_ERROR_UNKNOWN;
    }
  }
  
  /// read the new distribution.ini
  IniFile_t iniFile;
  result = ReadIniFile(distIni.c_str(), iniFile);
  if (result) {
    LogMessage("Failed to read INI file %S: %i", distIni.c_str(), result);
    if (usingFallback) {
      // we're using the fallback, the fact that distini (really, songbird.ini)
      // is missing is not fatal, just means we have nothing to do
      LogMessage("Squelching error, fallback %S missing is not fatal", ininame);
      if (ConvertUTFnToUTF8(argv[1]) == "uninstall") {
        // uninstall mode, explicitly delete the log
        gEnableLogging = false;
        CommandDeleteFile("$/disthelper.log");
      }
      return DH_ERROR_OK;
    }
    // we're in the distribution case, expecting a custom ini file here but
    // we didn't get one?
    ShowFatalError("Failed to read INI file %s: %i", distIni.c_str(), result);
    return result;
  }
  std::string section;
  section.assign(ConvertUTFnToUTF8(argv[1]));
  section.insert(0, "steps:");
  IniEntry_t::const_iterator it, end = iniFile[section].end();
  
  /// copy the distribution.ini / application.ini files to the appdir
  DebugMessage("Copying %s to %s\n",
               ConvertUTFnToUTF8(distIni).c_str(),
               ConvertUTFnToUTF8(ResolvePathName("$/distribution/")).c_str());

  if (section == "steps:install") {
    LogMessage("Skipping distribution.ini check for installation, forcing copy");
    result = CommandCopyFile(ConvertUTFnToUTF8(distIni), "$/distribution/");
    if (result) {
      LogMessage("Failed to copy distribution.ini file %S", distIni.c_str());
    }
  } else if (section != "steps:uninstall" && !usingFallback) {
    // don't copy on uninstall or using the fallback
    IniFile_t destDistIni;
    std::string destDistPath = GetLeafName(ConvertUTFnToUTF8(distIni));
    destDistPath.insert(0, "$/distribution/");
    tstring destDistIniFile = ResolvePathName(destDistPath);
    result = ReadIniFile(destDistIniFile.c_str(), destDistIni);
    if (result != DH_ERROR_OK) {
      LogMessage("Failed to read existing distribution.ini %S", destDistIniFile.c_str());
      ShowFatalError("Failed to read existing distribution.ini %s", destDistIniFile.c_str());
      return DH_ERROR_UNKNOWN;
    }

   std::string oldVersion = destDistIni["global"]["version"],
                newVersion = iniFile["global"]["version"];
    LogMessage("Checking distribution.ini versions... old=[%s] new=[%s]",
               oldVersion.c_str(), newVersion.c_str());
    if (VersionLessThan()(newVersion, oldVersion)) {
      // the new version is older than the old version? abort!
      LogMessage("Existing distribution.ini %S has version %s, which is newer than replacement %S of version %s",
                 ResolvePathName(destDistPath).c_str(), oldVersion.c_str(),
                 distIni.c_str(), newVersion.c_str());
      ShowFatalError("existing distribution.ini %s has version %S, which is newer than replacement %s of version %S",
                 ResolvePathName(destDistPath).c_str(), oldVersion.c_str(),
                 distIni.c_str(), newVersion.c_str());
      return DH_ERROR_UNKNOWN;
    }

    if (VersionLessThan()(oldVersion, newVersion)) {
      // we have a newer file, copy it over
      result = CommandCopyFile(ConvertUTFnToUTF8(distIni), "$/distribution/");
      if (result) {
        LogMessage("Failed to copy distribution.ini file %S", distIni.c_str());
      }
    } else {
      LogMessage("Not copying identical versions %S (version %s) to %S (version %s)",
                 distIni.c_str(), oldVersion.c_str(),
                 ResolvePathName("$/distribution/").c_str(), newVersion.c_str());
    }
  }

  result = CommandCopyFile(ConvertUTFnToUTF8(srcAppIniName), "$/");
  if (result) {
    LogMessage("Failed to copy application.ini file %S", srcAppIniName.c_str());
  }

  result = SetupEnvironment();
  if (result) {
    LogMessage("Failed to set up environment.");
    return result;
  }

  for (it = iniFile[section].begin(); it != end; ++it) {
    std::string line = it->second;
    LogMessage("Executing command %s", line.c_str());
    std::vector<std::string> command = ParseCommandLine(line);
    if (command[0] == "copy") {
      if (command.size() < 3) {
        OutputDebugString(_T("Not enough arguments for copy"));
        result = DH_ERROR_UNKNOWN;
      } else {
        result = CommandCopyFile(command[1], command[2]);
      }
    } else if (command[0] == "move") {
      if (command.size() < 3) {
        OutputDebugString(_T("Not enough arguments for move"));
        result = DH_ERROR_UNKNOWN;
      } else {
        result = CommandMoveFile(command[1], command[2]);
      }
    } else if (command[0] == "delete") {
      if (command.size() < 2) {
        OutputDebugString(_T("Not enough arguments for delete"));
        result = DH_ERROR_UNKNOWN;
      } else {
        result = CommandDeleteFile(command[1]);
      }
    } else if (command[0] == "icon") {
      if (command.size() < 3) {
        OutputDebugString(_T("Not enough arguments for icon"));
        result = DH_ERROR_UNKNOWN;
      } else {
        std::string iconname;
        if (command.size() > 3) {
          iconname.assign(command[3]);
        }
        result = CommandSetIcon(command[1], command[2], iconname);
      }
    } else if (command[0] == "versioninfo") {
      if (command.size() < 3) {
        OutputDebugString(_T("Not enough arguments for versioninfo"));
        result = DH_ERROR_UNKNOWN;
      } else {
        result = CommandSetVersionInfo(command[1], iniFile[command[2]]);
      }
    } else if (command[0] == "exec") {
      // Run the executable with the full argument string.  This allows the
      // executable to parse the arguments as needed, preserving, for example,
      // escape sequences for quotes (see CommandLineToArgvW).
      std::string executable;
      std::string args;
      ParseExecCommandLine(line, executable, args);
      result = CommandExecuteFile(executable, args);
    } else {
      OutputDebugString(_T("bad command!"));
      result = DH_ERROR_UNKNOWN;
    }
    
    if (result) {
      LogMessage("Command failed.");
      return result;
    }
  }
  
  LogMessage("Disthelper successfully completed");
  
  if (ConvertUTFnToUTF8(argv[1]) == "uninstall") {
    // uninstall mode, explicitly delete the log
    gEnableLogging = false;
    CommandDeleteFile("$/disthelper.log");
  }
  return 0;
}
