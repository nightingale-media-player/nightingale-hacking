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

  var library = createLibrary("test_playlistmanager");

  var manager = Cc["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
                  .getService(Ci.sbIPlaylistReaderManager);
  var platform = getPlatform();

  // Test mime types
  var mimeTypesCount = {};
  var mimeTypes = manager.supportedMIMETypes(mimeTypesCount);
  var expected = ["audio/mpegurl", "audio/x-mpegurl", "audio/x-scpls"];
  assertEqual(mimeTypesCount.value, expected.length);
  for (var i = 0; i < mimeTypesCount.value; i++) {
    expected = removeFromArray(expected, mimeTypes[i]);
  }
  assertEqual(expected.length, 0);

  // Test extensions
  var extCount = {};
  var exts = manager.supportedFileExtensions(extCount);
  expected = ["pls", "m3u"];
  assertEqual(extCount.value, expected.length);
  for (var i = 0; i < extCount.value; i++) {
    expected = removeFromArray(expected, exts[i]);
  }
  assertEqual(expected.length, 0);

  // Test read with extension
  var mediaList = library.createMediaList("simple");
  var file = getFile("relative_remote.m3u");
  manager.originalURI = newURI("http://www.foo.com/");
  manager.read(file, mediaList, null, false);
  assertMediaList(mediaList, getFile("relative_remote_result.xml"));

  // Test read with content type
  mediaList.clear();
  var file = getFile("relative_remote.pls");
  manager.originalURI = newURI("http://www.foo.com/");
  manager.read(file, mediaList, "audio/x-scpls", false);
  assertMediaList(mediaList, getFile("relative_remote_result.xml"));

  // Test loadPlaylist, local file
  mediaList.clear();
  if (platform == "Linux" || platform == "Darwin") {
    var fileUri = getFileUri(getFile("maclin_parse.m3u"));
    manager.loadPlaylist(fileUri, mediaList, null, false, null);
    assertMediaList(mediaList, getFile("maclin_parsem3u_result.xml"));
  }
  if (platform == "Windows_NT") {
    mediaList.clear();
    var fileUri = getFileUri(getFile("win_parse.m3u"));
    manager.loadPlaylist(fileUri, mediaList, null, false, null);
    assertMediaList(mediaList, getFile("win_parsem3u_result.xml"));
  }

  // Test loadPlaylist, remote file


}

function removeFromArray(a, v) {

  return a.filter(function(e) { return e != v; });

}

function getFileUri(aFile) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  return ioService.newFileURI(aFile);
}

