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

/**
 * This file tests interactions with the _canDrop and canDrop methods on the
 * playlist binding. The logic for determining drop acceptance is complex,
 * and these tests stub various function calls to ensure the logic is
 * executed correctly. The stubbed function calls themselves are _not_ tested
 * here.
 */

const Cu = Components.utils;

var XUL_NS = XUL_NS ||
             "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Cu.import("resource://app/jsmodules/DropHelper.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");

function runTest () {
  var url = "data:application/vnd.mozilla.xul+xml," +
            "<?xml-stylesheet href='chrome://global/skin' type='text/css'?>" +
            "<?xml-stylesheet href='chrome://songbird/content/bindings/bindings.css' type='text/css'?>" +
            "<window xmlns='http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul'/>";

  beginWindowTest(url, setup);
}

function setup () {

  testWindow.resizeTo(200,200);

  var document = testWindow.document,
      library = createLibrary("test_playlist_candrop", null, false),
      list = library.createMediaList("simple"),
      pqList = Cc["@songbirdnest.com/Songbird/playqueue/service;1"]
                 .getService(Ci.sbIPlayQueueService)
                 .mediaList;

  function createElement(aList, aType) {
    var playlist = document.createElementNS(XUL_NS, aType);
    playlist.setAttribute("flex", "1");
    document.documentElement.appendChild(playlist);
    playlist.bind(aList.createView(), null);

    return playlist;
  }

  continueWindowTest(test_canDrop,
                     [ createElement(library, "sb-playlist"),
                       createElement(list, "sb-playlist"),
                       createElement(pqList, "sb-playqueue-playlist") ]);
}

function test_canDrop(aLibraryPlaylist, aListPlaylist, aPlayQueuePlaylist) {

  function runTests(aPlaylist, aTestCases) {
    for (let i = 0, len = aTestCases.length; i < len; i++) {
      let testCase = new TestCase(aTestCases[i]);
      testCase.run([aPlaylist, aPlayQueuePlaylist]);
    }
  }

  runTests(aLibraryPlaylist, K_DROPONLIBRARY_TESTS);
  runTests(aListPlaylist, K_DROPONLIST_TESTS);
  runTests(aListPlaylist, K_DROPINDICATOR_TESTS);
  runTests(aListPlaylist, K_DROPONPLAYQUEUE_TESTS);

  endWindowTest();
}

// The order of test cases is important, as it represents the order that various
// conditions are checked in the _canDrop method. Many conditions cause early
// returns. For each test case in this array, it should be assumed that previous
// conditions do not cause early returns (i.e. any given test case should
// ensure that the specific condition for the test is actually arrived at)

const K_DROPONLIBRARY_TESTS = [
{
  desc: "should always return false when the _disableDrop flag is true",
  pre: function (playlist) {
    this.stub(playlist, '_disableDrop', true);
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  expected: false
},
{
  desc: "should always return false when the target is read only",
  pre: function (playlist) {
    this.stash = playlist.mediaList.getProperty(SBProperties.isReadOnly);
    playlist.mediaList.setProperty(SBProperties.isReadOnly, "1");
  },
  exec: function(playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  post: function (playlist) {
    playlist.mediaList.setProperty(SBProperties.isReadOnly, this.stash);
  },
  expected: false
},
{
  desc: "should always return false when the target has read only content",
  pre: function (playlist) {
    this.stash = playlist.mediaList.getProperty(SBProperties.isContentReadOnly);
    playlist.mediaList.setProperty(SBProperties.isContentReadOnly, "1");
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  post: function (playlist) {
    playlist.mediaList.setProperty(SBProperties.isContentReadOnly, this.stash);
  },
  expected: false
},
{
  desc: "should return true is the drop is a supported external flavor",
  pre: function (playlist) {
    this.stub(ExternalDropHandler, 'isSupported', function () {
      return true;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  expected: true
},
{
  desc: "should return false if the drop is not supported by any handler",
  pre: function (playlist) {
    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return false;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  expected: false
},
{
  desc: "should return false for lists of customType 'download' when " +
        "_canDownloadDrop returns false",
  pre: function (playlist) {
    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(playlist, '_canDownloadDrop', function() {
      return false;
    });
    var list = playlist.mediaList;
    this.stash = list.getProperty(SBProperties.customType);
    list.setProperty(SBProperties.customType, "download");
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  post: function (playlist) {
    playlist.mediaList.setProperty(SBProperties.customType, this.stash);
  },
  expected: false
},
{
  desc: "should return true for lists of customType 'download' when " +
        "_canDownloadDrop returns true",
  pre: function (playlist) {
    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(playlist, '_canDownloadDrop', function() {
      return true;
    });
    var list = playlist.mediaList;
    this.stash = list.getProperty(SBProperties.customType);
    list.setProperty(SBProperties.customType, "download");
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  post: function (playlist) {
    playlist.mediaList.setProperty(SBProperties.customType, this.stash);
  },
  expected: true
},
{
  desc: "should return false if an internal drop does not contain data",
  pre: function (playlist) {
    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(DNDUtils, 'getTransferDataForFlavour', function () {
      return null;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  expected: false
},
{
  desc: "should return true if the internal drop is a supported flavor, " +
        "the drop source is different from the drop target, " +
        "and the drop target accepts drops from the source",
  pre: function (playlist) {

    var dnd = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                .getService(Ci.sbIDndSourceTracker),
        sourceList = playlist.library.createMediaList("simple"),
        context = new DNDUtils.MediaListTransferContext(sourceList, sourceList),
        handle = dnd.registerSource(context);

    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type == "application/x-sb-transfer-media-list") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
    this.stub(playlist, 'acceptDropSource', function () {
      return true;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  expected: true
},
{
  desc: "should return false if the internal drop is a supported flavor, " +
        "the drop source is different from the drop target, " +
        "and the drop target does not accept drops from the source",
  pre: function (playlist) {

    var dnd = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                .getService(Ci.sbIDndSourceTracker),
        sourceList = playlist.library.createMediaList("simple"),
        context = new DNDUtils.MediaListTransferContext(sourceList, sourceList),
        handle = dnd.registerSource(context);

    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type == "application/x-sb-transfer-media-list") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
    this.stub(playlist, 'acceptDropSource', function () {
      return false;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  expected: false
},
{
  desc: "should return false if the internal drop is a supported flavor, " +
        "the drop source is the same as the drop target, " +
        "and the drop target is a library",
  pre: function (playlist) {

    var dnd = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                .getService(Ci.sbIDndSourceTracker),
        sourceList = playlist.mediaList,
        context = new DNDUtils.MediaListTransferContext(sourceList, sourceList),
        handle = dnd.registerSource(context);

    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type == "application/x-sb-transfer-media-list") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  expected: false
}];

const K_DROPONLIST_TESTS = [
{
  desc: "should return true if the internal drop is a supported flavor, " +
        "the drop source is the same as the drop target, " +
        "and the drop target is not a library",
  pre: function (playlist) {

    var dnd = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                .getService(Ci.sbIDndSourceTracker),
        sourceList = playlist.mediaList,
        context = new DNDUtils.MediaListTransferContext(sourceList, sourceList),
        handle = dnd.registerSource(context);

    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type == "application/x-sb-transfer-media-list") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  expected: true
},
{
  desc: "should return false if the internal drop is a supported flavor, " +
        "the drop source is the same as the drop target, " +
        "the drop target is not a library, and the drop target is not " +
        "sorted ordinally",
  pre: function (playlist) {

    var dnd = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                .getService(Ci.sbIDndSourceTracker),
        sourceList = playlist.mediaList,
        context = new DNDUtils.MediaListTransferContext(sourceList, sourceList),
        handle = dnd.registerSource(context);

    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function () {
      return true;
    });
    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type == "application/x-sb-transfer-media-list") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
    this.stub(playlist, 'isOrdinalSort', function() {
      return false;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist._canDrop(), this.desc);
  },
  expected: false
}];

const K_DROPINDICATOR_TESTS = [
{
  desc: "canDrop should return false when _canDrop returns false",
  pre: function (playlist) {
    this.stub(playlist, '_canDrop', function () {
      return false;
    });
    this.stub(playlist, 'isOrdinalSort', function () {
      return true;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist.canDrop(), this.desc);
  },
  expected: false
},
{
  desc: "canDrop should return false when the sort is not ordinal",
  pre: function (playlist) {
    this.stub(playlist, '_canDrop', function () {
      return true;
    });
    this.stub(playlist, 'isOrdinalSort', function () {
      return false;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist.canDrop(), this.desc);
  },
  expected: false
},
{
  desc: "canDrop should return true when canDrop returns true and the sort " +
        "is ordinal",
  pre: function (playlist) {
    this.stub(playlist, '_canDrop', function () {
      return true;
    });
    this.stub(playlist, 'isOrdinalSort', function () {
      return true;
    });
  },
  exec: function (playlist) {
    assertEqual(this.expected, playlist.canDrop(), this.desc);
  },
  expected: true
}];

const K_DROPONPLAYQUEUE_TESTS = [
{
  desc: "_canDrop should return false when the target is the play queue and " +
        "the source belongs to a device",
  pre: function (playlist, playQueuePlaylist) {
    var dnd = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                .getService(Ci.sbIDndSourceTracker),
        sourceList,
        context,
        handle;

    this.dev = Cc["@songbirdnest.com/Songbird/Device/DeviceTester/MockDevice;1"]
                 .createInstance(Ci.sbIDevice),
    this.devMgr = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                   .getService(Ci.sbIDeviceManager2),
    this.devMgr.registerDevice(this.dev);
    sourceList = this.dev.defaultLibrary
                         .QueryInterface(Ci.sbILibrary)
                         .createMediaList("simple");
    context = new DNDUtils.MediaListTransferContext(sourceList, sourceList),
    handle = dnd.registerSource(context);

    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function() {
      return true;
    });
    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type == "application/x-sb-transfer-media-list") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
  },
  exec: function (playlist, playQueuePlaylist) {
    assertEqual(this.expected, playQueuePlaylist._canDrop(), this.desc);
  },
  post: function() {
    this.devMgr.unregisterDevice(this.dev);
  },
  expected: false
},
{
  desc: "_canDrop should return true when the target is the play queue and " +
        "the source does not belong to a device, provided other normal " +
        "drop constraints are satisfied",
  pre: function (playlist, playQueuePlaylist) {
    var dnd = Cc["@songbirdnest.com/Songbird/DndSourceTracker;1"]
                .getService(Ci.sbIDndSourceTracker),
        sourceList = playlist.library.createMediaList("simple"),
        context = new DNDUtils.MediaListTransferContext(sourceList, sourceList),
        handle = dnd.registerSource(context);

    this.stub(ExternalDropHandler, 'isSupported', function () {
      return false;
    });
    this.stub(InternalDropHandler, 'isSupported', function() {
      return true;
    });
    this.stub(DNDUtils, 'getTransferDataForFlavour', function (type) {
      var sourceData = null;
      if (type == "application/x-sb-transfer-media-list") {
        sourceData = { data: handle };
      }
      return sourceData;
    });
  },
  exec: function (playlist, playQueuePlaylist) {
    assertEqual(this.expected, playQueuePlaylist._canDrop(), this.desc);
  },
  expected: true

}];

