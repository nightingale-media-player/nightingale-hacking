/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

/**
 * \brief Test file
 */

function runTest () {

  var library = createLibrary("test_playlistreader");

  var tests;
  var platform = getPlatform();
  if (platform == "Linux" || platform == "Darwin") {
    tests = [
      {
        originalURI: null,
        file: "maclin_parse.pls",
        result: "maclin_parsepls_result.xml"
      },
      {
        originalURI: "http://www.foo.com/",
        file: "relative_remote.pls",
        result: "relative_remote_result.xml"
      },
      {
        originalURI: "http://www.foo.com/mp3",
        file: "absolute_remote.pls",
        result: "absolute_remote_result.xml"
      },
      {
        originalURI: "file:///home/steve/blah.pls",
        file: "maclin_relative_local.pls",
        result: "maclin_relative_local_result.xml"
      },
      {
        originalURI: null,
        file: "maclin_utf8.pls",
        result: "maclin_utf8_result.xml"
      }
    ];
  }

  if (platform == "Windows_NT") {
    tests = [
      {
        originalURI: "http://www.foo.com/",
        file: "relative_remote.pls",
        result: "relative_remote_result.xml"
      }
    ];
  }

  for (var i = 0; i < tests.length; i++) {
    var t = tests[i];
    log("testing file " + t.file);
    library.clear();
    var handler = Cc["@songbirdnest.com/Songbird/Playlist/Reader/PLS;1"]
                    .createInstance(Ci.sbIPlaylistReader);

    var file = getFile(t.file);
    if (t.originalURI) {
      handler.originalURI = newURI(t.originalURI);
    }
    handler.read(file, library, false, {});
    assertMediaList(library, getFile(t.result));
  }

  // Test duplicates
  library.clear();
  var handler = Cc["@songbirdnest.com/Songbird/Playlist/Reader/PLS;1"]
                  .createInstance(Ci.sbIPlaylistReader);

  var file = getFile("relative_remote.pls");
  handler.originalURI = newURI("http://www.foo.com");

  handler.read(file, library, false, {});
  assertEqual(library.length, 3);
  handler.read(file, library, false, {});
  assertEqual(library.length, 6);
  handler.read(file, library, true, {});
  assertEqual(library.length, 6);
}

