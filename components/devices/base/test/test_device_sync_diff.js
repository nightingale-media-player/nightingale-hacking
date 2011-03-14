/* vim: set sw=2 :miv */
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
 * \brief Device tests - Device Sync Diff
 */

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

// These are totally bogus times, but we don't care. We only ever compare them
const TimeLastSynced = 1000;
const TimeAfter = TimeLastSynced + 10;
const TimeBefore = TimeLastSynced - 10;

var mainLibItems = {};
[
  ["track1", "Track 1", "Artist 1", "Album 1", TimeBefore],
  ["track2", "Track 2", "Artist 1", "Album 1", TimeBefore],
  ["track3", "Track 3", "Artist 1", "Album 1", TimeBefore],
  ["track5", "Track 5", "Artist 1", "Album 1", TimeBefore],
  ["track6", "Track 6", "Artist 1", "Album 1", TimeBefore],
  ["track7", "Track 7", "Artist 1", "Album 1", TimeBefore],
  ["track8", "Track 8", "Artist 1", "Album 1", TimeAfter],
].forEach(function (data) {
  mainLibItems[data[0]] = {
    track: data[1],
    artist: data[2],
    album: data[3],
    lastModified: data[4]
  };
});

var mainLibLists = {};
[
  ["list1", "A list", false, TimeBefore],
  ["list2", "B list", true,  TimeBefore],
  ["list3", "C list", true,  TimeAfter],
  ["list4", "D list", false, TimeAfter],
  ["list5", "E list", false, TimeBefore],
].forEach(function (data) {
  mainLibLists[data[0]] = {
    name: data[1],
    isSmart: data[2],
    lastModified: data[3],
  };
});

var deviceItems = {};
[
  // 1: Present in both, linked by origin GUID
  ["track1", "Track 1", "Artist 1", "Album 1", "track1"],
  // 2: Present in both, not linked by origin GUID
  ["track2", "Track 2", "Artist 1", "Album 1", null],
  // 3: Track 3 isn't here: it's in the main library only
  // 4: Only present on device
  ["track4", "Track 4", "Artist 1", "Album 1", null],
  // 5: Present in both, but with different filenames. Not linked by origin GUID
  ["track5b", "Track 5", "Artist 1", "Album 1", null],
  // 6: Same filename in both, but different title, and no origin GUID. This
  //    should be considered a different track entirely.
  ["track6", "Track 6b", "Artist 1", "Album 1", null],
  // 7: Same filename, different title, but has an origin GUID. Should be
  //    considered the same track.
  ["track7", "Track 7b", "Artist 1", "Album 1", "track7"],
  // 8: Present in both, but has been modified since last sync, so should
  //    re-sync to device.
  ["track8", "Track 8", "Artist 1", "Album 1", "track8"],
].forEach(function (data) {
  deviceItems[data[0]] = {
    track: data[1],
    artist: data[2],
    album: data[3],
    originPointsTo: mainLibItems[data[4]]
  };
});

var deviceLists = {};
[
  ["list1", "A list"],
  ["list2", "B list"],
  ["list3", "C list"],
  ["list4", "D list"],
  ["list6", "F list"],
].forEach(function (data) {
  deviceLists[data[0]] = {
    name: data[1],
  };
});

const ADDED = Ci.sbIChangeOperation.ADDED;
const MODIFIED = Ci.sbIChangeOperation.MODIFIED;

var expectedExportChanges = [
  {change: ADDED, source: mainLibItems['track3']},
  {change: ADDED, source: mainLibItems['track6']},
  {change: MODIFIED, source: mainLibItems['track8'],
                     dest: deviceItems['track8']},

  {change: MODIFIED, source: mainLibLists['list3'],
                     dest: deviceLists['list3']},
  {change: MODIFIED, source: mainLibLists['list4'],
                     dest: deviceLists['list4']},
  {change: ADDED, source: mainLibLists['list5']},
];

var expectedImportChanges = [
  {change: ADDED, source: deviceItems['track4']},
  {change: ADDED, source: deviceItems['track6']},

  {change: MODIFIED, source: deviceLists['list1'],
                     dest: mainLibLists['list1']},
  {change: ADDED, source: deviceLists['list6']},
];

function verifyChanges(actual, expected)
{
  assertEqual(actual.changes.length, expected.length);

  for (var i = 0; i < actual.changes.length; i++)
  {
    var change = actual.changes.queryElementAt(i, Ci.sbILibraryChange);

    assertEqual(change.operation, expected[i].change);

    assertEqual(change.sourceItem.guid, expected[i].source.item.guid);
    if (change.operation == MODIFIED) {
      assertEqual(change.destinationItem.guid, expected[i].dest.item.guid);
    }
  }

}

function populateLibraries(mainLib, deviceLib) {

  deviceLib.setProperty("http://songbirdnest.com/data/1.0#lastSyncTime",
                        TimeLastSynced);

  for (var itemFilename in mainLibItems) {
    var itemData = mainLibItems[itemFilename];
    var properties = SBProperties.createArray([
            [SBProperties.artistName, itemData.artist],
            [SBProperties.albumName, itemData.album],
            [SBProperties.trackName, itemData.track]]);

    var mainItem = mainLib.createMediaItem(
            newURI("file:///main/"+itemFilename),
            properties);

    // Have to set this separately, or it gets overwritten
    mainItem.setProperty(SBProperties.updated, itemData.lastModified);

    // Now store this
    itemData.item = mainItem;
  }

  for (var listId in mainLibLists) {
    var listData = mainLibLists[listId];

    var type = listData.isSmart ? "smart" : "simple";
    var item = mainLib.createMediaList(type);

    if (listData.isSmart) {
      // We actually have to have a condition attached to the smart media list
      // for various functions on it to work correctly. It doesn't matter what
      // the condition(s) are.
      var propMan =
          Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
            .getService(Ci.sbIPropertyManager);
      var prop = SBProperties.albumName;
      var op = propMan.getPropertyInfo(prop).getOperator("=");
      item.appendCondition(prop, op, "Album Foo", null, "a");
    }

    item.name = listData.name;
    item.setProperty(SBProperties.updated, listData.lastModified);

    listData.item = item;
  }

  for (var itemFilename in deviceItems) {
    var itemData = deviceItems[itemFilename];
    var properties = SBProperties.createArray([
            [SBProperties.artistName, itemData.artist],
            [SBProperties.albumName, itemData.album],
            [SBProperties.trackName, itemData.track]]);
    if (itemData.originPointsTo) {
      properties.appendProperty(SBProperties.originLibraryGuid, mainLib.guid);
      properties.appendProperty(SBProperties.originItemGuid,
                                itemData.originPointsTo.item.guid);
    }
    var devItem = deviceLib.createMediaItem(
            newURI("file:///device/"+itemFilename),
            properties);
    itemData.item = devItem;
  }

  for (var listId in deviceLists) {
    var listData = deviceLists[listId];

    var item = deviceLib.createMediaList("simple");
    item.name = listData.name;

    listData.item = item;
  }

}

function dumpChanges(title, changeset)
{
  dump("Dumping "+title+" ("+changeset.changes.length +" changes)\n");

  for (var i = 0; i < changeset.changes.length; i++)
  {
    var change = changeset.changes.queryElementAt(i, Ci.sbILibraryChange);

    if (change.operation == Ci.sbIChangeOperation.ADDED) {
      dump("Added: "+change.sourceItem.contentSrc.spec+"\n");
    }
    else if (change.operation == Ci.sbIChangeOperation.MODIFIED) {
      dump("Clobber: "+change.sourceItem.contentSrc.spec+
           " --> "+change.destinationItem.contentSrc.spec+"\n");
    }
    else {
      dump("INTERNAL ERROR\n");
    }

  }
}

function runTest () {
  var sync = Cc["@songbirdnest.com/Songbird/Device/DeviceLibrarySyncDiff;1"]
               .createInstance(Ci.sbIDeviceLibrarySyncDiff);

  // These are not the actual device/main libraries, just named so because in
  // this test they work as if they were.
  var mainLib = createLibrary("test_sync_main", null, false);
  var deviceLib = createLibrary("test_sync_device", null, false);

  mainLib.clear();
  deviceLib.clear();

  populateLibraries(mainLib, deviceLib);

  var exportChanges = {};
  var importChanges = {};
  sync.generateSyncLists(Ci.sbIDeviceLibrarySyncDiff.SYNC_FLAG_EXPORT_ALL |
                           Ci.sbIDeviceLibrarySyncDiff.SYNC_FLAG_IMPORT,
                         false,
                         Ci.sbIDeviceLibrarySyncDiff.SYNC_TYPE_AUDIO,
                         mainLib,
                         deviceLib,
                         null,
                         exportChanges,
                         importChanges);

  //dumpChanges("Export Changes", exportChanges.value);
  //dumpChanges("Import Changes", importChanges.value);

  verifyChanges(exportChanges.value, expectedExportChanges);
  verifyChanges(importChanges.value, expectedImportChanges);
}

