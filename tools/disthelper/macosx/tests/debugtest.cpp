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

#include "debug.h"
#include "commands.h"

#include <fstream>
#include <iomanip>
#include <ctime>
#include <sstream>
#include <errno.h>

void check(int cond, const char* fmt, ...);

void TestDebug() {
  printf("Starting debug tests...\n");

  // delete any existing log files first...
  tstring logpath = ResolvePathName("$/disthelper.log");
  int result = unlink(logpath.c_str());
  check(result == 0 || errno == ENOENT,
        "TestDebug: failed to remove old file %S, errno %i\n",
        logpath.c_str(),
        errno);

  LogMessage("hello, %s\n", "world!");
  time_t unixtime;
  time(&unixtime);
  tm* now = localtime(&unixtime);
  setlocale(LC_ALL, "C");
  
  std::ifstream logfile(logpath.c_str());
  std::string buffer;
  char cbuffer[0x1000];
  int ibuffer;

  logfile >> std::setw(1) >> buffer;
  check(buffer == "[",
        "TestDebug: log entry does not start with '[' (found '%s')\n",
        buffer.c_str());

  // XXX Mook: this will fail at times near midnight.  Watch me not care ;)

  logfile >> buffer;
  strftime(cbuffer, sizeof(cbuffer), "%a", now);
  check(buffer == cbuffer,
        "TestDebug: expected day-of-week [%s], found [%s]\n",
        cbuffer, buffer.c_str());

  logfile >> buffer;
  strftime(cbuffer, sizeof(cbuffer), "%b", now);
  check(buffer == cbuffer,
        "TestDebug: expected month [%s], found [%s]\n",
        cbuffer, buffer.c_str());

  logfile >> buffer;
  strftime(cbuffer, sizeof(cbuffer), "%d", now);
  check(buffer == cbuffer,
        "TestDebug: expected day [%s], found [%s]\n",
        cbuffer, buffer.c_str());

  logfile >> ibuffer;
  check(ibuffer == now->tm_hour,
        "TestDebug: expected hour [%i], found [%i]\n",
        now->tm_hour, ibuffer);

  logfile >> std::setw(1) >> buffer;
  check(buffer == ":",
        "TestDebug: hour-minute separator not ':' (found '%s')\n",
        buffer.c_str());

  logfile >> ibuffer;
  check(ibuffer == now->tm_min ||
          (ibuffer == now->tm_min - 1 && now->tm_sec == 0),
        "TestDebug: expected minute [%i], found [%i]\n",
        now->tm_min, ibuffer);

  logfile >> std::setw(1) >> buffer;
  check(buffer == ":",
        "TestDebug: minute-second separator not ':' (found '%s')\n",
        buffer.c_str());

  logfile >> ibuffer;
  check(ibuffer == now->tm_sec || ibuffer == now->tm_sec - 1,
        "TestDebug: expected second [%i], found [%i]\n",
        now->tm_sec, ibuffer);

  logfile >> ibuffer;
  check(ibuffer == now->tm_year + 1900,
        "TestDebug: expected year [%i], found [%i]\n",
        now->tm_year + 1900, ibuffer);
  
  std::stringbuf stringbuf;
  logfile.get(stringbuf);
  check (stringbuf.str() == "] hello, world!",
         "TestDebug: expected trailing string \"] hello, world!\", found \"%s\"\n",
         stringbuf.str().c_str());

  printf("TestDebug: pass\n");
}
