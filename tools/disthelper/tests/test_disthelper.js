/* vim: set sw=2 : */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
 * test_disthelper.js
 *
 * Test harness for disthelper tests
 *
 * This launches the disthelper unit testing binary (since it's, among other
 * things, not XPCOM) and checks the return value.  It expects to get zero when
 * all tests pass.
 */

function runTest () {
  // start from compiled/dist, get to the top compiled/ directory
  var bin = Cc["@mozilla.org/file/directory_service;1"]
              .getService(Ci.nsIProperties)
              .get("resource:app", Ci.nsIFile)
              .parent;
  var BIN_SUFFIX = "";
  switch (getPlatform()) {
    case "Darwin":
      // on Mac, we have the extra Songbird.app/Contents/MacOS
      bin = bin.parent.parent.parent;
      break;
    case "Windows_NT":
      BIN_SUFFIX = ".exe";
      break;
    default:
      break;
  }
  var path = "tools/disthelper/tests/test_disthelper" + BIN_SUFFIX;
  for each (let part in path.split("/")) {
    bin.append(part);
  }
  
  var process = Cc["@mozilla.org/process/util;1"].createInstance(Ci.nsIProcess);
  process.init(bin);
  process.run(true,   /* blocking */
              [], 0); /* args, len */
  assertEqual(0, process.exitValue, "test_disthelper binary returned error");
}
