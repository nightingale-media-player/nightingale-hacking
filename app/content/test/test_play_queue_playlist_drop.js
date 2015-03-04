/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2011 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

var XUL_NS = XUL_NS ||
             "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/DropHelper.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

function runTest () {
  var url = "data:application/vnd.mozilla.xul+xml," +
            "<?xml-stylesheet href='chrome://global/skin' type='text/css'?>" +
            "<?xml-stylesheet href='chrome://songbird/content/bindings/bindings.css' type='text/css'?>" +
            "<window xmlns='http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul'/>";

  beginWindowTest(url, TestPlayQueueDrop.setup);
}

var TestPlayQueueDrop = {

  setup: function TestPlayQueuPlaylistDrop_setup() {


    testWindow.resizeTo(200,200);

    var document = testWindow.document,
        library = createLibrary("test_play_queue_playlist_drop", null, false),
        pqSvc = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                  .getService(Ci.sbIPlayQueueService),
        pqList = pqSvc.mediaList;

    pqSvc.clearAll();

    function createElement(aList, aType) {
      var playlist = document.createElementNS(XUL_NS, aType);
      playlist.setAttribute("flex", "1");
      document.documentElement.appendChild(playlist);
      playlist.bind(aList.createView(), null);

      return playlist;
    }

    continueWindowTest(TestPlayQueueDrop.run,
                       [ createElement(library, "sb-playlist"),
                         createElement(pqList, "sb-playqueue-playlist") ]);
  },

  run: function TestPlayQueueDrop_run(aLibraryPlaylist,
                                              aPlayQueuePlaylist)
  {
    var tests = K_PLAYQUEUEDROP_TESTCASES;
    for (let i = 0, len = tests.length; i < len; i++) {
      let testCase = new TestCase(tests[i]);
      dump('DESC: ' + testCase.desc + '\n');
      testCase.run([aLibraryPlaylist, aPlayQueuePlaylist]);
    }
    endWindowTest();
  },

  // Helper to create dummy media items with unique uris within a given test run
  createItemsInLibrary:
    function TestPlayQueueDrop_createItemsInLibrary(aLibrary, aCount)
  {
    this._itemCounter = this._itemCounter || 0;

    function testURIFor(index) {
      return newURI('http://test.com/playqueuedrop/' + index);
    }

    var start = this._itemCounter,
        finish,
        indexes = [];

    this._itemCounter += aCount;
    finish = this._itemCounter - 1;

    for (let i = start; i <= finish; i++) {
      indexes.push(i);
    }

    return [ aLibrary.createMediaItem(testURIFor(i)) for each (i in indexes) ];
  }
};

const K_PLAYQUEUEDROP_TESTCASES = [
{
  desc: "dropping an enumerator of mediaitems on an empty play queue " +
        "should add items to the queue with async queueSomeNext",
  pre: function (libraryPlaylist, playQueuePlaylist) {
    var self = this;

    // Set up the async listener.
    this.pqSvc = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                   .getService(Ci.sbIPlayQueueService);
    this.pqListener = {
      onIndexUpdated: function (aToIndex) {
      },
      onQueueOperationStarted: function () {
        self.asyncOpStarted = true;
      },
      onQueueOperationCompleted: function () {
        assertEqual(3,
                    self.pqSvc.mediaList.length,
                    self.desc + ': incorrect queue length');
        testFinished();
      }
    };
    this.pqSvc.addListener(this.pqListener);

    // Create a fake drop from the test library
    var items =
        TestPlayQueueDrop.createItemsInLibrary(libraryPlaylist.library, 3);

    var view = libraryPlaylist.mediaListView;
    view.selection.selectAll();
    var context = new DNDUtils.MediaListViewSelectionTransferContext(view);
    var handle = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                   .getService(Ci.sbIDndSourceTracker)
                   .registerSource(context);

    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type == "application/x-sb-transfer-media-items") {
        sourceData = { data: handle};
      }
      return sourceData;
    });
    this.asyncOpStarted = false;
    this.fakeSession = {
      getData: function (aTransferable, aItemIndex) {
        return  { data: null };
      },
      isDataFlavorSupported: function (aDataFlavor) {
        return true;
      },
      dragAction: 1
    };
  },
  exec: function (libraryPlaylist, playQueuePlaylist) {
    playQueuePlaylist._dropOnTree(0,
                      Ci.sbIMediaListViewTreeViewObserver.DROP_AFTER,
                      this.fakeSession);
    // If we didn't start an async operation, fail the test so we explicitly
    // know that there is a problem instead of just hanging the test.
    assertTrue(this.asyncOpStarted,
               'async play queue drop did not call async listener');
    testPending();
  },
  post: function(libraryPlaylist) {
    this.pqSvc.removeListener(this.pqListener);
    this.pqSvc.clearAll();
    libraryPlaylist.library.clear();
  }
},
{
  desc: "inserting an enumerator of mediaitems between play queue rows  " +
        "should add items to the queue with async queueSomeBefore",
  pre: function (libraryPlaylist, playQueuePlaylist) {
    var self = this;

    // Set up the async listener
    this.pqSvc = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                   .getService(Ci.sbIPlayQueueService);
    this.pqListener = {
      onIndexUpdated: function (aToIndex) {
      },
      onQueueOperationStarted: function () {
        self.asyncOpStarted = true;
      },
      onQueueOperationCompleted: function () {
        assertEqual(7,
                    self.pqSvc.mediaList.length,
                    self.desc + ': incorrect queue length');
        testFinished();
      }
    };
    this.pqSvc.addListener(this.pqListener);

    // Create some items in the play queue.
    var queueLib = this.pqSvc.mediaList.library;
    var queueItems =
        TestPlayQueueDrop.createItemsInLibrary(queueLib, 4);
    queueItems.forEach(function (item) {
      // We need to actually get the items into the queue list
      self.pqSvc.queueNext(item);
    });

    // Create a fake drop from the test library
    var items =
        TestPlayQueueDrop.createItemsInLibrary(libraryPlaylist.library, 3);
    var view = libraryPlaylist.mediaListView;
    view.selection.selectAll();
    var context = new DNDUtils.MediaListViewSelectionTransferContext(view);
    var handle = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                   .getService(Ci.sbIDndSourceTracker)
                   .registerSource(context);

    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type == "application/x-sb-transfer-media-items") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
    this.asyncOpStarted = false;
    this.fakeSession = {
      getData: function (aTransferable, aItemIndex) {
        return  { data: handle };
      },
      isDataFlavorSupported: function (aDataFlavor) {
        return true;
      },
      dragAction: 1
    };
  },
  exec: function (libraryPlaylist, playQueuePlaylist) {
    playQueuePlaylist._dropOnTree(2,
                      Ci.sbIMediaListViewTreeViewObserver.DROP_BEFORE, this.fakeSession);
    // If we didn't start an async operation, fail the test so we explicitly
    // know that there is a problem instead of just hanging the test.
    assertTrue(this.asyncOpStarted,
               'async play queue drop did not call async listener');
    testPending();
  },
  post: function(libraryPlaylist) {
    this.pqSvc.removeListener(this.pqListener);
    this.pqSvc.clearAll();
    libraryPlaylist.library.clear();
  }
},
{
  desc: "dropping a mediaList on an empty queue should call up to the parent " +
        "playlist binding for synchronous insertion",
  pre: function (libraryPlaylist, playQueuePlaylist) {
    var self = this;

    // Set up the async listener.
    this.pqSvc = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                   .getService(Ci.sbIPlayQueueService);
    this.pqListener = {
      onIndexUpdated: function (aToIndex) {
      },
      onQueueOperationStarted: function () {
        doFail("Synchronous play queue operations should not call the async " +
               "operations listener");
      },
      onQueueOperationCompleted: function () {
        doFail("Synchronous play queue operations should not call the async " +
               "operations listener");
      }
    };
    this.pqSvc.addListener(this.pqListener);


    var library = libraryPlaylist.library;
    var list = library.createMediaList("simple");
    var items = TestPlayQueueDrop.createItemsInLibrary(library, 3);
    items.forEach(function (item) {
      list.add(item);
    });
    var context = new DNDUtils.MediaListTransferContext(list, list);
    var handle = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                   .getService(Ci.sbIDndSourceTracker)
                   .registerSource(context);

    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type === "application/x-sb-transfer-media-list") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
    this.stub(DNDUtils, 'getTransferData', function () {
      return  { data: handle };
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.fakeSession = {
      getData: function (aTransferable, aItemIndex) {
        return  { data: handle };
      },
      isDataFlavorSupported: function (aDataFlavor) {
        return true;
      },
      dragAction: 1
    };
  },
  exec: function (libraryPlaylist, playQueuePlaylist) {
    playQueuePlaylist._dropOnTree(0,
                      Ci.sbIMediaListViewTreeViewObserver.DROP_BEFORE,
                      this.fakeSession);
    assertEqual(this.pqSvc.mediaList.length,
                3,
                this.desc + ': incorrect queue length');
  },
  post: function (libraryPlaylist, playQueuePlaylist) {
    this.pqSvc.removeListener(this.pqListener);
    this.pqSvc.clearAll();
    libraryPlaylist.library.clear();
  }
},
{
  desc: "dropping a mediaList at a specific insertion point in the queue " +
        "should call the parent playlist binding for synchronous insertion",
  pre: function (libraryPlaylist, playQueuePlaylist) {
    var self = this;

    // Set up the async listener.
    this.pqSvc = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                   .getService(Ci.sbIPlayQueueService);
    this.pqListener = {
      onIndexUpdated: function (aToIndex) {
      },
      onQueueOperationStarted: function () {
        doFail("Synchronous play queue operations should not call the async " +
               "operations listener");
      },
      onQueueOperationCompleted: function () {
        doFail("Synchronous play queue operations should not call the async " +
               "operations listener");
      }
    };
    this.pqSvc.addListener(this.pqListener);

    // Add some items to the queue.
    var queueLib = this.pqSvc.mediaList.library;
    var queueItems =
      TestPlayQueueDrop.createItemsInLibrary(queueLib, 4);
    queueItems.forEach(function (item) {
      // We need to actually get the items into the queue list
      self.pqSvc.queueNext(item);
    });

    // Create a fake drop of a medialist from the test library
    var library = libraryPlaylist.library;
    var list = library.createMediaList("simple");
    var items = TestPlayQueueDrop.createItemsInLibrary(library, 3);
    items.forEach(function (item) {
      list.add(item);
    });
    var context = new DNDUtils.MediaListTransferContext(list, list);
    var handle = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                   .getService(Ci.sbIDndSourceTracker)
                   .registerSource(context);

    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type === "application/x-sb-transfer-media-list") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
    this.stub(DNDUtils, 'getTransferData', function () {
      return  { data: handle };
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });

    // This test needs a session that implements nsIDragSession
    this.fakeSession = {
      getData: function (aTransferable, aItemIndex) {
        return  { data: handle };
      },
      isDataFlavorSupported: function (aDataFlavor) {
        return true;
      },
      dragAction: 1
    };
  },
  exec: function (libraryPlaylist, playQueuePlaylist) {
    playQueuePlaylist._dropOnTree(2,
                                  Ci.sbIMediaListViewTreeViewObserver.DROP_BEFORE,
                                  this.fakeSession);
    assertEqual(this.pqSvc.mediaList.length,
                7,
                this.desc + ': incorrect queue length');
  },
  post: function (libraryPlaylist, playQueuePlaylist) {
    this.pqSvc.removeListener(this.pqListener);
    this.pqSvc.clearAll();
    libraryPlaylist.library.clear();
  }
}];

