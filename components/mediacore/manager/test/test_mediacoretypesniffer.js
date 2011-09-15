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
  var mediacoreManager = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                           .getService(Ci.sbIMediacoreManager);

  var factories = mediacoreManager.factories;
  log("Number of factories found: " + factories.length);

  var typeSniffer = Cc["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
                      .createInstance(Ci.sbIMediacoreTypeSniffer);

  var uri = newURI("file:///path/to/a/file.mp3");

  var isMedia = typeSniffer.isValidMediaURL(uri);
  var isVideo = typeSniffer.isValidVideoURL(uri);
  var isPlaylist = typeSniffer.isValidPlaylistURL(uri);

  log("uri is media? " + isMedia);
  log("uri is video? " + isVideo);
  log("uri is a playlist? " + isPlaylist);

//  assertTrue(isMedia, "URI should be media.");
//  assertFalse(isVideo, "URI should _not_ be video.");
//  assertFalse(isPlaylist, "URI should _not_ be a playlist.");

  var allExtensions = [];
  var supportedFileExtensions = typeSniffer.mediaFileExtensions;
  while(supportedFileExtensions.hasMore()) {
    allExtensions.push(supportedFileExtensions.getNext());
  }

  allExtensions.sort();
  log("All media file extensions: " + allExtensions.join());

  allExtensions = [];
  supportedFileExtensions = typeSniffer.audioFileExtensions;
  while(supportedFileExtensions.hasMore()) {
    allExtensions.push(supportedFileExtensions.getNext());
  }

  allExtensions.sort();
  log("All audio file extensions: " + allExtensions.join());

  allExtensions = [];
  supportedFileExtensions = typeSniffer.videoFileExtensions;
  while(supportedFileExtensions.hasMore()) {
    allExtensions.push(supportedFileExtensions.getNext());
  }

  allExtensions.sort();
  log("All video file extensions: " + allExtensions.join());

  allExtensions = [];
  supportedFileExtensions = typeSniffer.playlistFileExtensions;
  while(supportedFileExtensions.hasMore()) {
    allExtensions.push(supportedFileExtensions.getNext());
  }

  allExtensions.sort();
  log("All playlist file extensions: " + allExtensions.join());
}
