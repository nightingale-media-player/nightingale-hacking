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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var batchListener = {
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaListListener]),
  onItemAdded: function() true,
  onBeforeItemRemoved: function() true,
  onAfterItemRemoved: function() true,
  onItemUpdated: function() true,
  onItemMoved: function() true,
  onBeforeListCleared: function() true,
  onListCleared: function() true,
  onBatchBegin: function(aMediaList) {
    ++this.depth;
    log("depth: " + this.depth);
  },
  onBatchEnd: function(aMediaList) {
    --this.depth;
    log("depth: " + this.depth);
  },
  depth: 0
};

function runTest () {
  var mediaItem1;
  var mediaItem2;
  var mediaItemWatcher;
  var filter;
  var func;

  // Import some services.
  Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

  // Make a test library.
  var library = createLibrary("test_mediaitemwatcher");
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);
  libraryManager.registerLibrary(library, false);

  // Set up a test media item listener.
  var listener = {
    itemRemovedCount: 0,
    itemUpdatedCount: 0,

    onItemRemoved: function(aMediaItem) {
      this.itemRemovedCount++;
    },
    onItemUpdated: function(aMediaItem) {
      log(aMediaItem.contentSrc.spec);
      this.itemUpdatedCount++;
    },

    reset: function() {
      this.itemRemovedCount = 0;
      this.itemUpdatedCount = 0;
    }
  };


  //----------------------------------------------------------------------------
  //
  // Test basic update and removal notification.
  //

  // Reset the listener.
  listener.reset();
  library.addListener(batchListener,
                      false,
                      Ci.sbIMediaList.LISTENER_FLAGS_BATCHBEGIN |
                        Ci.sbIMediaList.LISTENER_FLAGS_BATCHEND);

  // Create a test media item.
  mediaItem1 = library.createMediaItem(newURI("http://test.com/test1"));
  assertTrue(mediaItem1);
  mediaItem2 = library.createMediaItem(newURI("http://test.com/test2"));
  assertTrue(mediaItem2);

  // Set up a test media item watcher.
  mediaItemWatcher = Cc["@songbirdnest.com/Songbird/Library/MediaItemWatcher;1"]
                       .createInstance(Ci.sbIMediaItemWatcher);
  assertTrue(mediaItemWatcher);
  mediaItemWatcher.watch(mediaItem1, listener);

  // Test item updated notification.
  mediaItem1.setProperty(SBProperties.trackName, "test1");
  assertEqual(listener.itemRemovedCount, 0);
  assertEqual(listener.itemUpdatedCount, 1);
  mediaItem1.setProperty(SBProperties.albumName, "test1");
  assertEqual(listener.itemRemovedCount, 0);
  assertEqual(listener.itemUpdatedCount, 2);
  mediaItem2.setProperty(SBProperties.trackName, "test2");
  assertEqual(listener.itemRemovedCount, 0);
  assertEqual(listener.itemUpdatedCount, 2);

  // Test item removed notification.
  listener.reset();
  library.remove(mediaItem1);
  library.remove(mediaItem2);
  assertEqual(listener.itemRemovedCount, 1);
  assertEqual(listener.itemUpdatedCount, 0);

  // Clean up.
  mediaItemWatcher.cancel();
  library.remove(mediaItem1);
  library.remove(mediaItem2);


  //----------------------------------------------------------------------------
  //
  // Test watcher cancel.
  //

  // Reset the listener.
  listener.reset();

  // Create a test media item.
  mediaItem1 = library.createMediaItem(newURI("http://test.com/test1"));
  assertTrue(mediaItem1);

  // Set up a test media item watcher.
  mediaItemWatcher = Cc["@songbirdnest.com/Songbird/Library/MediaItemWatcher;1"]
                       .createInstance(Ci.sbIMediaItemWatcher);
  assertTrue(mediaItemWatcher);
  mediaItemWatcher.watch(mediaItem1, listener);

  // Test media item watcher cancel.
  mediaItem1.setProperty(SBProperties.trackName, "test1");
  assertEqual(listener.itemRemovedCount, 0);
  assertEqual(listener.itemUpdatedCount, 1);
  mediaItemWatcher.cancel();
  mediaItem1.setProperty(SBProperties.trackName, "test2");
  assertEqual(listener.itemRemovedCount, 0);
  assertEqual(listener.itemUpdatedCount, 1);

  // Clean up.
  mediaItemWatcher.cancel();
  library.remove(mediaItem1);


  //----------------------------------------------------------------------------
  //
  // Test item updated notification with filter.
  //

  // Reset the listener.
  listener.reset();

  // Create a test media item.
  mediaItem1 = library.createMediaItem(newURI("http://test.com/test"));
  assertTrue(mediaItem1);

  // Set up a test media item watcher to just watch for track name changes.
  mediaItemWatcher = Cc["@songbirdnest.com/Songbird/Library/MediaItemWatcher;1"]
                       .createInstance(Ci.sbIMediaItemWatcher);
  assertTrue(mediaItemWatcher);
  filter = SBProperties.createArray([ [ SBProperties.trackName, null ] ]);
  mediaItemWatcher.watch(mediaItem1, listener, filter);

  // Test item updated notification with filter.
  mediaItem1.setProperty(SBProperties.trackName, "test");
  assertEqual(listener.itemRemovedCount, 0);
  assertEqual(listener.itemUpdatedCount, 1);
  mediaItem1.setProperty(SBProperties.albumName, "test");
  assertEqual(listener.itemRemovedCount, 0);
  assertEqual(listener.itemUpdatedCount, 1);

  // Clean up.
  mediaItemWatcher.cancel();
  library.remove(mediaItem1);


  //----------------------------------------------------------------------------
  //
  // Test item updated notification in a batch.
  //

  // Reset the listener.
  listener.reset();

  // Create two test media items.
  mediaItem1 = library.createMediaItem(newURI("http://test.com/test"));
  assertTrue(mediaItem1);
  mediaItem2 = library.createMediaItem(newURI("http://test.com/test"));
  assertTrue(mediaItem2);

  // Set up a test media item watcher to just watch for track name changes.
  mediaItemWatcher = Cc["@songbirdnest.com/Songbird/Library/MediaItemWatcher;1"]
                       .createInstance(Ci.sbIMediaItemWatcher);
  assertTrue(mediaItemWatcher);
  mediaItemWatcher.watch(mediaItem1, listener);

  // Test item updated notification in a batch.
  func = function() {
    mediaItem1.setProperty(SBProperties.trackName, "test");
    mediaItem2.setProperty(SBProperties.trackName, "test");
    assertEqual(listener.itemRemovedCount, 0);
    assertEqual(listener.itemUpdatedCount, 0);
  };
  library.runInBatchMode(func);
  assertEqual(listener.itemRemovedCount, 0);
  assertEqual(listener.itemUpdatedCount, 1);

  // Clean up.
  mediaItemWatcher.cancel();
  library.remove(mediaItem1);
  library.remove(mediaItem2);


  //----------------------------------------------------------------------------
  //
  // Test item removal notification in a batch.
  //

  // Reset the listener.
  listener.reset();

  // Create two test media items.
  mediaItem1 = library.createMediaItem(newURI("http://test.com/test"));
  assertTrue(mediaItem1);
  mediaItem2 = library.createMediaItem(newURI("http://test.com/test"));
  assertTrue(mediaItem2);

  // Set up a test media item watcher.
  mediaItemWatcher = Cc["@songbirdnest.com/Songbird/Library/MediaItemWatcher;1"]
                       .createInstance(Ci.sbIMediaItemWatcher);
  assertTrue(mediaItemWatcher);
  mediaItemWatcher.watch(mediaItem1, listener);

  // Test item updated notification in a batch.
  func = function() {
    library.remove(mediaItem1);
    library.remove(mediaItem2);
    assertEqual(listener.itemRemovedCount, 0);
    assertEqual(listener.itemUpdatedCount, 0);
  };
  library.runInBatchMode(func);
  assertEqual(listener.itemRemovedCount, 1);
  assertEqual(listener.itemUpdatedCount, 0);

  // Clean up.
  mediaItemWatcher.cancel();
  library.remove(mediaItem1);
  library.remove(mediaItem2);
  library.removeListener(batchListener);

  // Final clean up.
  libraryManager.unregisterLibrary(library);
}

