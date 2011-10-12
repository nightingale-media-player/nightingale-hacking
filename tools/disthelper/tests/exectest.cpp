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
 * Portions created by the Initial Developer are Copyright (C) 2010
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

#include <errno.h>

#ifdef XP_WIN
  #include <direct.h>
  #include <tchar.h>
  #include <windows.h>
#else
  #include "tchar_compat.h"
#endif /* XP_WIN */

#ifndef ARRAY_LENGTH
#define ARRAY_LENGTH(x) ( sizeof(x) / sizeof((x)[0]) )
#endif /* ARRAY_LENGTH */

struct testdata_t {
  const char * command;
  bool shouldSucceed;
};

#if defined(XP_WIN)
  #define PLATFORM "win"
  static const testdata_t DIST_TEST_DATA[] = {
    {"cmd /c \"exit 0\"", true},
    {"cmd /c \"exit 1\"", false}
  };
#elif defined(XP_MACOSX)
  #define PLATFORM "mac"
  static const testdata_t DIST_TEST_DATA[] = {
    {"true", true},
    {"false", false}
  };
#else
  #error Unknown platform
#endif

void check(int cond, const char* fmt, ...);

void TestExec()
{
  printf("Starting command=exec tests...\n");

  int result = _mkdir("../distribution");
  check(result == 0 || errno == EEXIST,
        "Failed to create distribution directory: %d\n", errno);

  FILE* f = NULL;

  // write out application.ini data
  f = fopen("../application.ini", "w");
  check(f != NULL, "Failed to write old application.ini");
  fprintf(f, "[app]\nbuildid=0\n");
  fclose(f);
  f = fopen("application.ini", "w");
  check(f != NULL, "Failed to write new application.ini");
  fprintf(f, "[app]\nbuildid=0\n");
  fclose(f);
  
  f = fopen("../distribution/test.ini", "wb");
  check(f != NULL, "Failed to write old data file");
  fprintf(f, "[global]\nversion=0\nid=old\n");
  fclose(f);

  for (unsigned int i = 0; i < ARRAY_LENGTH(DIST_TEST_DATA); ++i) {
    f = fopen("test.ini", "w");
    check(f != NULL, "Failed to write new data file");
    fprintf(f, "[global]\nversion=0\nid=new\n");
    fprintf(f,
            "[steps:test:%s]\n0001=exec %s",
            PLATFORM,
            DIST_TEST_DATA[i].command);
    fclose(f);
  
    #ifdef XP_WIN
      result = system("..\\disthelper.exe test tests\\test.ini");
    #else
      result = system("../disthelper test tests/test.ini");
    #endif /* XP_WIN */
    check(result != -1, "Failed to execute disthelper: %08x\n", errno);
    if (DIST_TEST_DATA[i].shouldSucceed) {
      check(result == 0,
            "disthelper unexpected fail running [%s]: %i\n",
            ConvertUTF8toUTFn(DIST_TEST_DATA[i].command).c_str(),
            result);
    }
    else {
      check(result != 0,
            "disthelper unexpected success running [%s]: %i\n",
            ConvertUTF8toUTFn(DIST_TEST_DATA[i].command).c_str(),
            result);
    }
  }
  
  printf("TestExec: PASS\n");
}
