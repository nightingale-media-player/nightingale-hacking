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

#include "error.h"
#include "stringconvert.h"
#include "debug.h"
#include "toolslib.h"
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <stdlib.h>

#ifndef UNICODE
#error This only supports the UNICODE configuration (no SBCS/MBCS)
#endif

#define MAX_LONG_PATH 0x8000 /* 32767 + 1, maximum size for \\?\ style paths */

#define NS_ARRAY_LENGTH(x) (sizeof(x) / sizeof(x[0]))

/* Disabled for now 'cause we don't have GetDistIniDirectory()

tstring ResolvePathName(std::string aSrc) {
  std::wstring src(ConvertUTF8ToUTF16(aSrc));
  std::wstring::iterator begin(src.begin());
  #if DEBUG
    DebugMessage("Resolving path name %S", src.c_str());
  #endif
  // replace all forward slashes with backward ones
  std::wstring::size_type i = src.find(L'/');
  while (i != std::wstring::npos) {
    src[i] = L'\\';
    i = src.find(L'/', i);
  }
  if (begin != src.end()) {
    if (L'$' == *begin) {
      std::wstring::iterator next = begin;
      ++++next; // skip two characters
      src.replace(begin, next, GetAppDirectory());
    }
    WCHAR buffer[MAX_LONG_PATH + 1];
    DWORD length = SearchPath(GetDistIniDirectory().c_str(),
                              src.c_str(),
                              NULL,
                              MAX_LONG_PATH,
                              buffer,
                              NULL);
    if (length > 0) {
      buffer[length] = '\0';
      src.assign(buffer, length + 1);
    } else {
      DebugMessage("Failed to resolve path name %S", src.c_str());
    }
  }
  #if DEBUG
    DebugMessage("Resolved path name %S", src.c_str());
  #endif
  return src;
}
*/

std::vector<std::string> ParseCommandLine(const std::string& aCommandLine) {
  static const char WHITESPACE[] = " \t\r\n";
  std::vector<std::string> args;
  std::string::size_type prev = 0, offset;
  offset = aCommandLine.find_last_not_of(WHITESPACE);
  if (offset == std::string::npos) {
    // there's nothing that's not whitespace, don't bother
    return args;
  }
  std::string commandLine = aCommandLine.substr(0, offset + 1);
  std::string::size_type length = commandLine.length();
  do {
    prev = commandLine.find_first_not_of(WHITESPACE, prev);
    if (prev == std::string::npos) {
      // nothing left that's not whitespace
      break;
    }
    if (commandLine[prev] == '"') {
      // start of quoted param
      ++prev; // eat the quote
      offset = commandLine.find('"', prev);
      if (offset == std::string::npos) {
        // no matching end quote; assume it lasts to the end of the command
        offset = commandLine.length();
      }
    } else {
      // unquoted
      offset = commandLine.find_first_of(WHITESPACE, prev);
      if (offset == std::string::npos) {
        offset = commandLine.length();
      }
    }
    args.push_back(commandLine.substr(prev, offset - prev));
    prev = offset + 1;
  } while (prev < length);
  
  return args;
}

tstring GetAppDirectory() {
  WCHAR buffer[MAX_LONG_PATH + 1] = {0}; 
  HMODULE hExeModule = ::GetModuleHandle(NULL);
  DWORD length = ::GetModuleFileName(hExeModule, buffer, MAX_LONG_PATH);
  buffer[MAX_LONG_PATH - 1] = '\0';
  if (length != MAX_LONG_PATH) {
    buffer[length] = '\0';
  }
  tstring result(buffer);
  tstring::size_type sep = result.rfind(L'\\');
  if (sep != tstring::npos) {
    result.erase(sep + 1);
  }
  #if DEBUG
    DebugMessage("Found app directory %S", result.c_str());
  #endif
  return result;
}

std::string GetLeafName(std::string aSrc) {
  std::string::size_type slash, backslash;
  backslash = aSrc.rfind('\\');
  if (backslash != std::string::npos) {
    return aSrc.substr(backslash);
  }
  slash = aSrc.rfind('/');
  if (slash != std::string::npos) {
    return aSrc.substr(slash);
  }
  return aSrc;
}
