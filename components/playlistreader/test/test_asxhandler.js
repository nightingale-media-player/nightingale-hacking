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
  if (platform == "Windows_NT") {
    tests = [
      {
        originalURI: null,
        file: "kqedradio.asx",
        result: "kqedradio_result.xml"
      },
    ];
  }
  else {
    tests = [];
  }

  for (var i = 0; i < tests.length; i++) {
    var t = tests[i];
    log("testing file " + t.file);
    library.clear();
    var handler = Cc["@songbirdnest.com/Songbird/Playlist/Reader/ASX;1"]
                    .createInstance(Ci.sbIPlaylistReader);

    var file = getFile(t.file);
    if (t.originalURI) {
      handler.originalURI = newURI(t.originalURI);
    }
    handler.read(file, library, false);
    assertMediaList(library, getFile(t.result));
  }
  
}

