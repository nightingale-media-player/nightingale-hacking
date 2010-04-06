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
#include "readini.h"
#include "error.h"

#include <errno.h>
#include <string>
#include <vector>

#ifdef XP_WIN
  #include <direct.h>
  #include <windows.h>
#endif /* XP_WIN */

#ifdef XP_MACOSX
  #include "tchar_compat.h"
  #include <sys/stat.h>
#endif /* XP_MACOSX */

#ifndef NS_ARRAY_LENGTH
#define NS_ARRAY_LENGTH(x) ( sizeof(x) / sizeof((x)[0]) )
#endif /* NS_ARRAY_LENGTH */

void check(int cond, const char* fmt, ...);
void TestDistVersion();
void TestAppVersion();

void TestVersion() {
  TestDistVersion();
  TestAppVersion();
}

struct testdata_t {
  const char * oldVersion;
  const char * newVersion;
  bool shouldSucceed;
};

static const testdata_t DIST_TEST_DATA[] = {
  {"0",   "0",   false},
  {"0",   "1",   true },
  {"1",   "0",   false},
  {"0.0", "0",   false},
  {"1",   "1.0", false},
  {"1",   "f",   false}, /* integer only! */
  {"1.1", "1.2", true },
  {NULL,  "1",   false}, /* fail on missing source file */
  {"0",   NULL,  false}, /* fail on missing destination file */
  {"0.1", "1",   true },
  {"1",   "0.1", false}
};

void TestDistVersion() {
  printf("Starting distribution.ini version tests...\n");
  
  int result = _mkdir("../distribution");
  check(result == 0 || errno == EEXIST,
        "Failed to create distribution directory: %d\n", errno);

  FILE* f = NULL;
  
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(DIST_TEST_DATA); ++i) {
    fprintf(stdout, "======================================================\n");
    fprintf(stdout, "Test case %u: old %s new %s expected %s\n",
            i, DIST_TEST_DATA[i].oldVersion, DIST_TEST_DATA[i].newVersion,
            DIST_TEST_DATA[i].shouldSucceed ? "yes" : "no");
    // put in dummy application.ini files :/
    f = fopen("../application.ini", "w");
    check(f != NULL, "Failed to write old application.ini");
    fprintf(f, "[app]\nbuildid=0\n");
    fclose(f);
    f = fopen("application.ini", "w");
    check(f != NULL, "Failed to write new application.ini");
    fprintf(f, "[app]\nbuildid=0\n");
    fclose(f);

    // write out the test data
    if (DIST_TEST_DATA[i].oldVersion) {
      f = fopen("../distribution/test.ini", "w");
      check(f != NULL, "Failed to write old data file");
      fprintf(f, "[global]\nversion=%s\nid=old\n", DIST_TEST_DATA[i].oldVersion);
      fclose(f);
    } else {
      // no old version
      _unlink("../distribution/test.ini");
    }
    if (DIST_TEST_DATA[i].newVersion) {
      f = fopen("test.ini", "w");
      check(f != NULL, "Failed to write new data file");
      fprintf(f, "[global]\nversion=%s\nid=new\n", DIST_TEST_DATA[i].newVersion);
      fclose(f);
    } else {
      // no new version
      _unlink("test.ini");
    }

    fflush(stdout); fflush(stderr);
    #ifdef XP_WIN
      result = system("..\\disthelper.exe test tests\\test.ini");
    #else
      result = system("../disthelper test tests/test.ini");
    #endif /* XP_WIN */
    check(result != -1, "Failed to execute disthelper: %08x\n", errno);
    
    IniFile_t data;
    result = ReadIniFile(_T("../distribution/test.ini"), data);
    check(result == DH_ERROR_OK ||
            (!DIST_TEST_DATA[i].shouldSucceed &&
             (!DIST_TEST_DATA[i].oldVersion || !DIST_TEST_DATA[i].newVersion)),
          "Failed to read output (testcase #%i)", i);
    if (DIST_TEST_DATA[i].shouldSucceed) {
      check(data["global"]["version"].compare(DIST_TEST_DATA[i].newVersion) == 0 &&
              data["global"]["id"].compare("new") == 0,
            "Unexpectedly did not copy new data: %s -> %s result %s (%s)",
            DIST_TEST_DATA[i].oldVersion,
            DIST_TEST_DATA[i].newVersion,
            data["global"]["version"].c_str(),
            data["global"]["id"].c_str());
    } else if (DIST_TEST_DATA[i].oldVersion){
      check(data["global"]["version"].compare(DIST_TEST_DATA[i].oldVersion) == 0 &&
              data["global"]["id"].compare("old") == 0,
            "Unexpectedly overwrote old data: %s -> %s result %s (%s)",
            DIST_TEST_DATA[i].oldVersion,
            DIST_TEST_DATA[i].newVersion,
            data["global"]["version"].c_str(),
            data["global"]["id"].c_str());
    } else {
      // the act of finding the strings for dumping causes the condition to change
      if (data["global"].find("version") != data["global"].end()) {
        check(data["global"].find("version") == data["global"].end(),
              "Unexpectedly overwrote missing data: (null) -> %s result %s (%s)",
              DIST_TEST_DATA[i].newVersion,
              data["global"]["version"].c_str(),
              data["global"]["id"].c_str());
      }
    }
  }
  printf("TestDistVersion: PASS\n");
}

static const testdata_t APP_TEST_DATA[] = {
  {"0",        "0",        true },
  {"0",        "1",        false},
  {"1",        "0",        false},
  {"0",        "0.1",      false},
  {NULL,       "0",        false},
  {"0",        NULL,       false},
  {"moo",      "moo",      true },
  {"0",        "moo",      false},
  {"20090229", "20090229", true }
};

void TestAppVersion() {
  printf("Starting application.ini version tests...\n");
  
  int result = _mkdir("../distribution");
  check(result == 0 || errno == EEXIST,
        "Failed to create distribution directory: %d\n", errno);
  
  FILE* f = NULL;
  
  for (unsigned int i = 0; i < NS_ARRAY_LENGTH(APP_TEST_DATA); ++i) {
    fprintf(stdout, "======================================================\n");
    fprintf(stdout, "Test case %u: old %s new %s expected %s\n",
            i, APP_TEST_DATA[i].oldVersion, APP_TEST_DATA[i].newVersion,
            APP_TEST_DATA[i].shouldSucceed ? "yes" : "no");
    // put in dummy distribution.ini files :/
    f = fopen("../distribution/test.ini", "w");
    check(f != NULL, "Failed to write old distribution.ini");
    fprintf(f, "[global]\nversion=0\n");
    fclose(f);
    f = fopen("test.ini", "w");
    check(f != NULL, "Failed to write new application.ini");
    fprintf(f, "[global]\nversion=1\n");
    fclose(f);

    // write out the test data
    if (APP_TEST_DATA[i].oldVersion) {
      f = fopen("../application.ini", "w");
      check(f != NULL, "Failed to write old data file");
      fprintf(f, "[app]\nbuildid=%s\nid=old\n", APP_TEST_DATA[i].oldVersion);
      fclose(f);
    } else {
      // no old version
      _unlink("../application.ini");
    }
    if (APP_TEST_DATA[i].newVersion) {
      f = fopen("application.ini", "w");
      check(f != NULL, "Failed to write new data file");
      fprintf(f, "[app]\nbuildid=%s\nid=new\n", APP_TEST_DATA[i].newVersion);
      fclose(f);
    } else {
      // no new version
      _unlink("application.ini");
    }

    fflush(stdout); fflush(stderr);
    #ifdef XP_WIN
      result = system("..\\disthelper.exe test tests\\test.ini");
    #else
      result = system("../disthelper test tests/test.ini");
    #endif /* XP_WIN */
    check(result != -1, "Failed to execute disthelper: %08x\n", errno);
    
    IniFile_t data;
    result = ReadIniFile(_T("../application.ini"), data);
    check(result == DH_ERROR_OK || !APP_TEST_DATA[i].shouldSucceed,
          "Failed to read output (testcase #%i)", i);
    if (APP_TEST_DATA[i].shouldSucceed) {
      check(data["app"]["buildid"].compare(APP_TEST_DATA[i].newVersion) == 0 &&
              data["app"]["id"].compare("new") == 0,
            "Unexpectedly did not copy new data: %s -> %s result %s (%s)",
            APP_TEST_DATA[i].oldVersion,
            APP_TEST_DATA[i].newVersion,
            data["app"]["buildid"].c_str(),
            data["app"]["id"].c_str());
    } else {
      // the act of finding the strings for dumping causes the condition to change
      if (data["app"].find("buildid") != data["app"].end()) {
        check(data["app"].find("buildid") == data["app"].end(),
              "Unexpectedly overwrote missing data: (null) -> %s result %s (%s)",
              APP_TEST_DATA[i].newVersion,
              data["app"]["buildid"].c_str(),
              data["app"]["id"].c_str());
      }
    }
  }
  printf("TestAppVersion: PASS\n");
}
