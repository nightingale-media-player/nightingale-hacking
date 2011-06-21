/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
  if (platform == "Windows_NT") {
    tests = [
      {
        originalURI: null,
        file: "win_parse.pls",
        result: "win_parsepls_result.xml"
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
        originalURI: null,
        file: "win_utf8.pls",
        result: "win_utf8_result.xml"
      }
    ];
  } else {
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
        originalURI: null,
        file: "maclin_utf8.pls",
        result: "maclin_utf8_result.xml"
      },
      {
        enabled: platform == "Darwin",
        originalURI: null,
        file: "maclin_unf.pls",
        result: "maclin_unf_nfd_result.xml"
      },
      {
        enabled: platform != "Darwin",
        originalURI: null,
        file: "maclin_unf.pls",
        result: "maclin_unf_nfc_result.xml"
      }
    ];
  }

  for (var i = 0; i < tests.length; i++) {
    var t = tests[i];
    if (("enabled" in t) && !t.enabled)
      continue;
    log("testing file " + t.file);
    library.clear();
    var handler = Cc["@songbirdnest.com/Songbird/Playlist/Reader/PLS;1"]
                    .createInstance(Ci.sbIPlaylistReader);

    var file = getFile(t.file);
    if (t.originalURI) {
      handler.originalURI = newURI(t.originalURI);
    }
    handler.read(file, library, false);
    assertMediaList(library, getFile(t.result));
  }

  // test local playlists (should only import files that exist on disk)
  library.clear();
  var handler = Cc["@songbirdnest.com/Songbird/Playlist/Reader/PLS;1"]
                  .createInstance(Ci.sbIPlaylistReader);

  var fileName = "";
  if (platform == "Windows_NT") {
    fileName = "win_relative_local.pls";
  } else {
    fileName = "maclin_relative_local.pls";
  }
  log("testing file " + fileName);
  var file = getFile(fileName);
  handler.originalURI = newFileURI(file);
  handler.read(file, library, false);

  var mp3dir = getFile("mp3");
  var file1 = mp3dir.clone();
  file1.append("file1.mp3");
  var file2 = mp3dir.clone();
  file2.append("file2.mp3");
  var musicdir = getFile("music");
  var file3 = musicdir.clone();
  file3.append("file3.mp3");
  var files = [file1, file2, file3];
  
  var prop = "http://songbirdnest.com/data/1.0#contentURL";
  for (var i in files) {
    var result = newFileURI(files[i]).spec;
    var item = getFirstItemByProperty(library, prop, result);
    if (!item) {
      fail("item with property '" + prop + "' equal to '" + result + "' not found");
    }
  }
  assertEqual(library.length, 3);

  // Test duplicates
  library.clear();
  var mediaList = library.createMediaList("simple");
  var handler = Cc["@songbirdnest.com/Songbird/Playlist/Reader/PLS;1"]
                  .createInstance(Ci.sbIPlaylistReader);

  var file = getFile("relative_remote.pls");
  handler.originalURI = newURI("http://www.foo.com");

  handler.read(file, mediaList, false);
  assertEqual(mediaList.length, 3);
  assertEqual(mediaList.library.length, 4);
  handler.read(file, mediaList, false);
  assertEqual(mediaList.length, 6);
  assertEqual(mediaList.library.length, 4);

  // Should not change the list
  handler.read(file, mediaList, true);
  assertEqual(mediaList.library.length, 4);
  assertEqual(mediaList.length, 6);
}

