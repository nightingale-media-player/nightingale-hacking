/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \brief Test file
 */

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

function runTest () {
  testRegistration();
  testUpdate();
  testPodcast();
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

  var dps = Cc["@getnightingale.com/Nightingale/Library/DynamicPlaylistService;1"]
              .getService(Ci.sbIDynamicPlaylistService);
  var libraryManager = Cc["@getnightingale.com/Nightingale/library/Manager;1"]
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

  assertTrue(list.QueryInterface(Ci.sbIDynamicMediaList));
  assertEqual(list.getProperty(SBProperties.isSubscription), "1");
  assertEqual(list.getProperty(SBProperties.base + "subscriptionURL"), uri.spec);
  assertEqual(list.getProperty(SBProperties.base + "subscriptionInterval"), "60");
  assertEqual(list.getProperty(SBProperties.destination), newFileURI(dest).spec);

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

  var libraryManager = Cc["@getnightingale.com/Nightingale/library/Manager;1"]
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

    var dps = Cc["@getnightingale.com/Nightingale/Library/DynamicPlaylistService;1"]
                .getService(Ci.sbIDynamicPlaylistService);

    var dest = Cc["@mozilla.org/file/directory_service;1"]
                 .getService(Ci.nsIProperties)
                 .get("TmpD", Ci.nsIFile);

    var url = "http://localhost:" + PORT_NUMBER +
              "/test_dynamicplaylist_playlist.m3u";

    var list = dps.createList(library1, newURI(url), 60, dest);

    // Write the first playlist and then update the subscription
    writeFile(playlistFile, playlist1);

    list.update();

    var sleepCount = 0;
    while (list.length < 2 && sleepCount++ < 20) {
      sleep(1000);
    }

    // Check the contents of the list
    assertEqual(list.length, 2);

    // try to wait for a bit until we succeed at getting the track names
    for (sleepCount = 0; sleepCount < 20; ++sleepCount) {
      if (list.getItemByIndex(0).getProperty(SBProperties.trackName) == "test1 title" &&
          list.getItemByIndex(1).getProperty(SBProperties.trackName) == "test2 title")
      {
        break;
      }
      sleep(1000);
    }
    assertEqual(list.getItemByIndex(0).getProperty(SBProperties.trackName), "test1 title");
    assertEqual(list.getItemByIndex(1).getProperty(SBProperties.trackName), "test2 title");

    // Update the playlist file and update again
    writeFile(playlistFile, playlist2);
    list.update();

    sleepCount = 0;
    while (list.length < 3 && sleepCount++ < 20) {
      sleep(1000);
    }

    // Check the contents of the list
    assertEqual(list.length, 3);
    // try to wait for a bit until we succeed at getting the track names
    for (sleepCount = 0; sleepCount < 20; ++sleepCount) {
      if (list.getItemByIndex(0).getProperty(SBProperties.trackName) == "test1 title" &&
          list.getItemByIndex(1).getProperty(SBProperties.trackName) == "test2 title" &&
          list.getItemByIndex(2).getProperty(SBProperties.trackName) == "test3 title")
      {
        break;
      }
      sleep(1000);
    }
    assertEqual(list.getItemByIndex(0).getProperty(SBProperties.trackName), "test1 title");
    assertEqual(list.getItemByIndex(1).getProperty(SBProperties.trackName), "test2 title");
    assertEqual(list.getItemByIndex(2).getProperty(SBProperties.trackName), "test3 title");

    // TODO: How can we check to see if these files were downloaded?
  }
  finally {
    server.stop(function() {});

    libraryManager.unregisterLibrary(library1);
  }
}

function testPodcast() {
  // Create a test library.
  var libraryManager = Cc["@getnightingale.com/Nightingale/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);
  var library1 = createLibrary("test_dynamicplaylist1", null, false);
  library1.clear();
  libraryManager.registerLibrary(library1, false);

  // Create and validate a podcast.
  var dps = Cc["@getnightingale.com/Nightingale/Library/DynamicPlaylistService;1"]
              .getService(Ci.sbIDynamicPlaylistService);
  var uri = newURI("http://foo.com");
  var podcast = dps.createPodcast(library1, uri);
  assertTrue(podcast);
  assertEqual(podcast.getProperty(SBProperties.customType), "podcast");

  // Clean up.
  libraryManager.unregisterLibrary(library1);
}

function writeFile(file, data) {
  // file is nsIFile, data is a string
  var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                   .createInstance(Ci.nsIFileOutputStream);

  foStream.init(file, 0x02 | 0x08 | 0x20, 0664, 0); // write, create, truncate
  foStream.write(data, data.length);
  foStream.close();
}
