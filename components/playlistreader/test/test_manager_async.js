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

  var server = Cc["@mozilla.org/server/jshttp;1"]
                 .createInstance(Ci.nsIHttpServer);

  server.start(8080);
  server.registerDirectory("/", getFile("."));

  var library = createLibrary("test_playlistmanager");

  var manager = Cc["@songbirdnest.com/Songbird/PlaylistReaderManager;1"]
                  .getService(Ci.sbIPlaylistReaderManager);
  var listener = Cc["@songbirdnest.com/Songbird/PlaylistReaderListener;1"]
                   .createInstance(Ci.sbIPlaylistReaderListener);

  listener.observer = {
    observe: function(aSubject, aTopic, aData) {
      assertMediaList(mediaList, getFile("absolute_remote_localhost_result.xml"));
      server.stop();
      // Prevent closure from leaking
      listener.observer = null;
      testFinished();
    }
  }

  var mediaList = library.createMediaList("simple");
  var uri = newURI("http://localhost:8080/absolute_remote.m3u");
  manager.loadPlaylist(uri, mediaList, null, false, listener);

  testPending();
}

