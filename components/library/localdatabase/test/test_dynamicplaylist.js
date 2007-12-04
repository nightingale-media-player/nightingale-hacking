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

var SB_NS = "http://songbirdnest.com/data/1.0#";
var SB_PROP_ISSUBSCRIPTION       = SB_NS + "isSubscription";
var SB_PROP_SUBSCRIPTIONURL      = SB_NS + "subscriptionURL";
var SB_PROP_SUBSCRIPTIONINTERVAL = SB_NS + "subscriptionInterval";
var SB_PROP_TRACKNAME            = SB_NS + "trackName";
var SB_PROP_DESTINATION          = SB_NS + "destination"

function runTest () {
  testRegistration();
  testUpdate();
}

var PORT_NUMBER = getTestServerPortNumber()

var playlist1 = <>
#EXTM3U
http://localhost:{PORT_NUMBER}/test1.mp3
http://localhost:{PORT_NUMBER}/test2.mp3
</>.toString();

var playlist2 = <>
#EXTM3U
http://localhost:{PORT_NUMBER}/test2.mp3
http://localhost:{PORT_NUMBER}/test3.mp3
</>.toString();

function testRegistration() {

  var dps = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/DynamicPlaylistService;1"]
              .getService(Ci.sbILocalDatabaseDynamicPlaylistService);
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);
  var library1 = createLibrary("test_dynamicplaylist1", null, false);
  library1.clear();

  // Start with no lists scheduled
  assertFalse(dps.scheduledLists.hasMoreElements());

  // Register an empty library, should be empty
  libraryManager.registerLibrary(library1, false);
  assertFalse(dps.scheduledLists.hasMoreElements());

  // Create a new list, should get scheduled
  var uri = newURI("http://foo.com");
  var dest = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("TmpD", Ci.nsIFile);
  var list = dps.createList(library1, uri, 60, dest);

  assertEqual(list.getProperty(SB_PROP_ISSUBSCRIPTION), "1");
  assertEqual(list.getProperty(SB_PROP_SUBSCRIPTIONURL), uri.spec);
  assertEqual(list.getProperty(SB_PROP_SUBSCRIPTIONINTERVAL), "60");
  assertEqual(list.getProperty(SB_PROP_DESTINATION), newFileURI(dest).spec);

  var scheduled = dps.scheduledLists.getNext();
  assertTrue(scheduled, list);

  // Unregister the library, list should be unscheduled
  libraryManager.unregisterLibrary(library1);
  assertFalse(dps.scheduledLists.hasMoreElements());

  // Re-register library, list should be scheduled again
  libraryManager.registerLibrary(library1, false);
  assertTrue(dps.scheduledLists.getNext(), list);

  // Delete list, should be unregistered
  library1.remove(list);
  assertFalse(dps.scheduledLists.hasMoreElements());

  libraryManager.unregisterLibrary(library1);
}

function testUpdate() {

  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);
  var library1 = createLibrary("test_dynamicplaylist1", null, false);
  library1.clear();
  libraryManager.registerLibrary(library1, false);

  var playlistFile = getFile("test_dynamicplaylist_playlist.m3u");
  writeFile(playlistFile, playlist1);

  var server = Cc["@mozilla.org/server/jshttp;1"]
             .createInstance(Ci.nsIHttpServer);

  try {
    server.start(PORT_NUMBER);
    server.registerDirectory("/", getFile("."));

    var dps = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/DynamicPlaylistService;1"]
                .getService(Ci.sbILocalDatabaseDynamicPlaylistService);

    var dest = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties)
                 .get("TmpD", Ci.nsIFile);

    var url = "http://localhost:" + PORT_NUMBER +
              "/test_dynamicplaylist_playlist.m3u";

    var list = dps.createList(library1, newURI(url), 60, dest);

    // Write the first playlist and then update the subscription
    writeFile(playlistFile, playlist1);

    dps.updateNow(list);

    var sleepCount = 0;
    while (list.length < 2 && sleepCount++ < 20) {
      sleep(1000);
    }

    // Check the contents of the list
    assertEqual(list.length, 2);
    // XXXsteve These seem a bit too time sensitive right now
    //assertEqual(list.getItemByIndex(0).getProperty(SB_PROP_TRACKNAME), "test1 title");
    //assertEqual(list.getItemByIndex(1).getProperty(SB_PROP_TRACKNAME), "test2 title");

    // Updat the playlist file and update again
    writeFile(playlistFile, playlist2);
    dps.updateNow(list);

    sleepCount = 0;
    while (list.length < 3 && sleepCount++ < 20) {
      sleep(1000);
    }

    // Check the contents of the list
    assertEqual(list.length, 3);
    // XXXsteve These seem a bit too time sensitive right now
    //assertEqual(list.getItemByIndex(0).getProperty(SB_PROP_TRACKNAME), "test1 title");
    //assertEqual(list.getItemByIndex(1).getProperty(SB_PROP_TRACKNAME), "test2 title");
    //assertEqual(list.getItemByIndex(2).getProperty(SB_PROP_TRACKNAME), "test3 title");

    // TODO: How can we check to see if these files were downloaded?
  }
  finally {
    server.stop();

    libraryManager.unregisterLibrary(library1);
  }
}

function writeFile(file, data) {
  // file is nsIFile, data is a string
  var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                   .createInstance(Ci.nsIFileOutputStream);

  foStream.init(file, 0x02 | 0x08 | 0x20, 0664, 0); // write, create, truncate
  foStream.write(data, data.length);
  foStream.close();
}
