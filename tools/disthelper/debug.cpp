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

#include "debug.h"
#include "commands.h"

#if XP_WIN
  #include <windows.h>
#else
  #include "tchar_compat.h"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sstream>

#include <stringconvert.h>

bool gEnableLogging = true;

void DebugMessage(const char* fmt, ...) {
  va_list args;
  TCHAR *buffer;

  // retrieve the variable arguments
  va_start(args, fmt);

  #if _MSC_VER
    tstring format = ConvertUTF8toUTFn(fmt);
    int len = _vscwprintf(format.c_str(), args) // _vscprintf doesn't count
                                           + 1; // terminating '\0'
    buffer = (TCHAR*)malloc(len * sizeof(TCHAR));
    vswprintf_s(buffer, len, format.c_str(), args);
  #else
    vasprintf(&buffer, fmt, args);
  #endif

  ::OutputDebugString(buffer);

  free(buffer);
  va_end(args);
}

void LogMessage(const char* fmt, ...) {
  tstring appDir(ResolvePathName("$/disthelper.log"));
  FILE* fout = _tfopen(appDir.c_str(), _T("a"));
  if (fout) {
    time_t timer;
    time(&timer);
    tm* now = localtime(&timer);
    const int BUFFER_LEN = 30;
    char time_str[BUFFER_LEN]; // '[', ']', ' '
    strftime(time_str, BUFFER_LEN - 1, "[%a %b %d %H:%M:%S %Y] ", now);
    tstring output = ConvertUTF8toUTFn(time_str);
    
    // do printf()-style formatting into a string buffer
    va_list args;
    va_start(args, fmt);
    TCHAR* buffer = NULL;
    #if _MSC_VER
      tstring format = ConvertUTF8toUTFn(fmt);
      int len = _vscwprintf(format.c_str(), args) // _vscwprintf doesn't count
                                             + 1; // terminating '\0'
      buffer = (TCHAR*)malloc(len * sizeof(TCHAR));
      vswprintf_s(buffer, len, format.c_str(), args);
    #else
      vasprintf(&buffer, fmt, args);
    #endif
  
    va_end(args);
    output += tstring(buffer);
    free(buffer);
    
    ::OutputDebugString(output.c_str());
    fprintf(fout, "%s\n", ConvertUTFnToUTF8(output).c_str());
  
    fclose(fout);
  }
}

