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
 
function newURI(spec) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  
  return ioService.newURI(spec, null, null);
}

function runTest () {
  var typeSniffer = Cc["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
                      .createInstance(Ci.sbIMediacoreTypeSniffer);
  
  var uri = newURI("file:///path/to/a/file.mp3");
  
  var isMedia = typeSniffer.isMediaURL(uri);
  var isVideo = typeSniffer.isVideoURL(uri);
  var isPlaylist = typeSniffer.isPlaylistURL(uri);
  
  assertTrue(isMedia, "URI should be media.");
  assertFalse(isVideo, "URI should _not_ be video.");
  assertFalse(isPlaylist, "URI should _not_ be a playlist.");
  
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
  log("All media file extensions: " + allExtensions.join());

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
