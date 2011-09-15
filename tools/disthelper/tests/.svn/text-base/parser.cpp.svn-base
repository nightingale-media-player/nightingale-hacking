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

#include <vector>
#include <string>

void check(int cond, const char* fmt, ...);

struct testdata_t {
  const char * commandline;
  const char * results[0x10];
};

static const testdata_t TEST_DATA[] = {
  // basic test
  { "hello world",                   { "hello", "world", 0 } },
  // stripping crap from end of line
  { "hello world\n",                 { "hello", "world", 0 } },
  { "hello world\t\n",               { "hello", "world", 0 } },
  { "hello world\r\n",               { "hello", "world", 0 } },
  { "hello world    \r\n",           { "hello", "world", 0 } },
  // skipping random bits of whitespace
  { "hello   world",                 { "hello", "world", 0 } },
  { "hello\tworld",                  { "hello", "world", 0 } },
  // quoted
  { "hello \"world domination\"",    { "hello", "world domination", 0 } },
  { "hello \"world domination",      { "hello", "world domination", 0 } },
  { "\"hello world\" yatta",         { "hello world", "yatta", 0 } },
};

void TestParser() {
  printf("Starting parser tests...\n");
  std::vector<std::string> args;
  for (unsigned int i = 0; i < sizeof(TEST_DATA)/sizeof(TEST_DATA[0]); ++i) {
    args = ParseCommandLine(TEST_DATA[i].commandline);
    #if 0
    {
      std::vector<std::string>::const_iterator begin, end = args.end();
      for (begin = args.begin(); begin != end; ++begin) {
        printf("[%s] ", begin->c_str());
      }
    }
    #endif
    for (unsigned int j = 0; j < args.size(); ++j) {
      if (j < 0x10 && TEST_DATA[i].results[j]) {
        check(args[j] == TEST_DATA[i].results[j],
              "TestParser: in testcase [%s], arg #%i [%s] expected [%s] found\n",
              TEST_DATA[i].commandline, j, TEST_DATA[i].results[j], args[j].c_str());
      } else {
        check(false,
              "TestParser: in testcase [%s], parsed extra argument #%i: [%s] (total found %i args)\n",
              TEST_DATA[i].commandline, j, args[j].c_str(), args.size());
      }
    }
    check(!TEST_DATA[i].results[args.size()],
          "TestParser: in testcase [%s], parsed %i args, expected next arg [%s]\n",
          TEST_DATA[i].commandline, args.size(), TEST_DATA[i].results[args.size()]);
  }
  printf("TestParser: pass\n");
}
