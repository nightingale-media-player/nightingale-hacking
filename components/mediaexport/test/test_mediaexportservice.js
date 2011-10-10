/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");


var TEST_FILES = newAppRelativeFile("testharness/mediaexportservice/files"); 


function runTest()
{
  var controller = new TestController();
  controller.startTest();
}

//------------------------------------------------------------------------------
// Media export service controller

function TestController()
{
}

/*
  Phases:
  0 = Add tracks to main library
  1 = Add playlist, smartplaylist, and add media items to simple playlist
  2 = Delete playlist and smartplaylist.
*/

TestController.prototype = 
{
  _phase                : 0,
  _shutdownService      : null,
  _tracks               : [],
  _tracks2              : [],
  _trackPaths           : [],
  _track2Paths          : [],
  _mainLibrary          : null,
  _playlist             : null,
  _smartPlaylist        : null,

  // Make the test controller start the testing.
  startTest: function() {
    this._log("Starting mediaexport service test!");
    this._mainLibrary = LibraryUtils.mainLibrary;

    // Create a the URI's for the tracks to add to the library.
    this._tracks[0] = newTestFileURI("one.mp3").spec;
    this._tracks[1] = newTestFileURI("two.mp3").spec;
    this._tracks[2] = newTestFileURI("three.mp3").spec;

    // Create another set of fake mediaitems.
    this._tracks2[0] = newTestFileURI("four.mp3").spec;
    this._tracks2[1] = newTestFileURI("five.mp3").spec;
    this._tracks2[2] = newTestFileURI("six.mp3").spec;

    // Get the file-system paths for the URI's created above.
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var fileProtocolHandler = ioService.getProtocolHandler("file")
                              .QueryInterface(Ci.nsIFileProtocolHandler);
    for (var i = 0; i < this._tracks.length; i++) {
      var curTrackURL = this._tracks[i];
      this._trackPaths[i] = 
        fileProtocolHandler.getFileFromURLSpec(curTrackURL).path;
    }

    for (var i = 0; i < this._tracks2.length; i++) {
      var curTrackURL = this._tracks2[i];
      this._track2Paths[i] = 
        fileProtocolHandler.getFileFromURLSpec(curTrackURL).path;
    }

    // Make sure the tracks, playlist, and smart playlist are not currently
    // in the library.
    this._mainLibrary.clear();

    // Clean up an existing export tasks
    removeAllTaskFiles();

    // Access the shutdown service via the |sbIShutdownJob| interface
    this._shutdownService = Cc["@getnightingale.com/media-export-service;1"]
                              .getService(Ci.sbIShutdownJob);
    this._shutdownService.addJobProgressListener(this);

    // Finally, kick off the test.
    this._runCurrentPhase();
    testPending();
  },

  // Run current phase helper
  _runCurrentPhase: function() {
    this._log("-> Phase " + this._phase + " BEGIN");

    // First, set the prefs depending on the export phase
    switch (this._phase) {
      case 0:
        setExportTracksOnly();
        break;
      case 1:
      case 2:
        setExportTracksPlaylistsSmartPlaylists();
    }

    // Next, incrementally add items to the library depending on the task to
    // test that all of the data is getting written out to the task file.
    switch (this._phase) {
      case 0:
        this._addTracks(this._mainLibrary);
        break;

      case 1:
        // Test playlist add and playlist mediaitem add 
        this._addPlaylist();
        this._addSmartPlaylist();
        this._addTracks(this._playlist);
        this._addTracks2(this._mainLibrary);
        break;

      case 2:
        // Test playlist removal
        this._removePlaylist();
        this._removeSmartPlaylist();
        break;
    }

    this._invokeExportService();
  },

  // Validate the exported data based on the current phase.
  _validateExportData: function() {
    this._log("-> Phase " + this._phase + " VALIDATING...");

    var shouldContinue = true;

    var taskFile = getExportedTaskFile();
    var parsedTask = new TaskFileDataParser(taskFile);

    this._logTaskData(parsedTask);

    switch (this._phase) {
      case 0:
        // There shouldn't be any added media lists.
        assertEqual(parsedTask.getAddedMediaLists().length, 0);

        // There shouldn't be any removed media lists.
        assertEqual(parsedTask.getRemovedMediaLists().length, 0);

        // Ensure the added tracks to the main library
        var addedLibraryItems = parsedTask.getAddedMediaItems()["#####NIGHTINGALE_MAIN_LIBRRAY#####"];
        assertTrue(addedLibraryItems);

        // There should only be three added media items.
        assertEqual(addedLibraryItems.length, 3);
        assertEqual(addedLibraryItems[0], this._trackPaths[0]);
        assertEqual(addedLibraryItems[1], this._trackPaths[1]);
        assertEqual(addedLibraryItems[2], this._trackPaths[2]);
        break;

      case 1:
        // Ensure that the 2 playlists were added and that the 3 mediaitems
        // have been added to the simple playlist.
        var addedMediaLists = parsedTask.getAddedMediaLists();
        assertEqual(addedMediaLists.length, 2);
        assertEqual(addedMediaLists[0], this._playlist.name);
        assertEqual(addedMediaLists[1], this._smartPlaylist.name);

        // Should be no removed playlists.
        var removedLists = parsedTask.getRemovedMediaLists();
        assertEqual(removedLists.length, 0);

        // Should be 3 added mediaitems to |this._playlist|.
        var addedMediaItems = 
          parsedTask.getAddedMediaItems()[this._playlist.name];
        assertEqual(addedMediaItems.length, 3);

        assertEqual(addedMediaItems[0], this._trackPaths[0]);
        assertEqual(addedMediaItems[1], this._trackPaths[1]);
        assertEqual(addedMediaItems[2], this._trackPaths[2]);

        // Should be 3 added mediaitems to |this._mainLibrary|.
        addedMediaItems = 
          parsedTask.getAddedMediaItems()["#####NIGHTINGALE_MAIN_LIBRRAY#####"];
        assertEqual(addedMediaItems.length, 3);

        assertEqual(addedMediaItems[0], this._track2Paths[0]);
        assertEqual(addedMediaItems[1], this._track2Paths[1]);
        assertEqual(addedMediaItems[2], this._track2Paths[2]);
        break;

      case 2:
        // There shouldn't be any added media lists.
        assertEqual(parsedTask.getAddedMediaLists().length, 0);

        // There should be 2 removed media lists.
        var removedMediaLists = parsedTask.getRemovedMediaLists();
        assertEqual(removedMediaLists.length, 2);

        assertEqual(removedMediaLists[0], this._playlist.name);
        assertEqual(removedMediaLists[1], this._smartPlaylist.name);

        // There shouldn't be any added media items.
        var hasAddedContent = false;
        for (var item in parsedTask.getAddedMediaItems()) {
          hasAddedContent = true;
          this._log("FOUND " + item + " as an added mediaitem ERROR!!!!");
        }
        assertFalse(hasAddedContent);

        // Shutdown the unit test now.
        shouldContinue = false;
        break;
    }

    this._log("-> Phase " + this._phase + " FINISHED");

    // Don't hang on to this file.
    taskFile.remove(false);
    parsedTask = null;
    sleep(1000);

    if (shouldContinue) {
      this._phase++;
      this._runCurrentPhase();
    }
    else {
      // clean up
      setExportNothing();

      // drop references
      this._shutdownService.removeJobProgressListener(this);
      this._shutdownService = null;

      this._log("Test finished!");
      testFinished();
    }
  },

  // Start the export service via the shutdown job interface.
  _invokeExportService: function() {
    this._log("Export service needs to run = " + 
              this._shutdownService.needsToRunTask);
    assertTrue(this._shutdownService.needsToRunTask);
    this._shutdownService.startTask();
  },

  // sbIJobProgressListener
  onJobProgress: function(aJobProgress) {
    if (aJobProgress.status == Ci.sbIJobProgress.STATUS_SUCCEEDED) {
      this._log("Current job has STATUS_SUCCEEDED");
      this._validateExportData();
    }
  },

  // Add/Remove tracks helpers
  _addTracks: function(aTargetMediaList) {
    for (var i = 0; i < this._tracks.length; i++) {
      var mediaItem = 
        this._mainLibrary.createMediaItem(newURI(this._tracks[i]));
      aTargetMediaList.add(mediaItem);
      this._log("Added '" + this._tracks[i] + "'' (" + 
                    mediaItem.guid + ") to '" + aTargetMediaList.name + "'");
    }
  },

  // Add tracks from |this._tracks2|
  _addTracks2: function(aTargetMediaList) {
    for (var i = 0; i < this._tracks2.length; i++) {
      var mediaItem =
        this._mainLibrary.createMediaItem(newURI(this._tracks2[i]));
      aTargetMediaList.add(mediaItem);
      this._log("Added '" + this._tracks2[i] + "'' (" + 
                    mediaItem.guid + ") to '" + aTargetMediaList.name + "'");
    }
  },
  
  _removeTracks: function() {
    // |clearItems()| doesn't remove any medialists.
    this._mainLibrary.clearItems();
    this._log("Cleared the main library of mediaitems");
  },

  // Add/Remove playlists helpers
  _addPlaylist: function() {
    this._playlist = this._mainLibrary.createMediaList("simple");
    this._playlist.name = "TEST PLAYLIST";
  },

  _removePlaylist: function() {
    if (this._playlist) {
      this._mainLibrary.remove(this._playlist);
    }
  },

  // Add/Remove smart playlists helpers
  _addSmartPlaylist: function() {
    this._smartPlaylist = this._mainLibrary.createMediaList("smart");
    this._smartPlaylist.name = "TEST SMARTPLAYLIST";
  },

  _removeSmartPlaylist: function() {
    if (this._smartPlaylist) {
      this._mainLibrary.remove(this._smartPlaylist);
    }
  },
  
  // Task file logging helper
  _logTaskData: function(aTaskData)
  {
    var addedMediaLists = "ADDED MEDIALISTS:";
    var removedMediaLists = "REMOVED MEDIALISTS";
    var addedMediaItems = "ADDED MEDIAITEMS";

    for (var i = 0; i < aTaskData.getAddedMediaLists().length; i++) {
      addedMediaLists += "\n - " + aTaskData.getAddedMediaLists()[i];
    }
    for (var i = 0; i < aTaskData.getRemovedMediaLists().length; i++) {
      removedMediaLists += "\n - " + aTaskData.getRemovedMediaLists()[i];
    }
    for (var curMediaListName in aTaskData.getAddedMediaItems()) {
      addedMediaItems += "\n - " + curMediaListName;

      var curListArray = aTaskData.getAddedMediaItems()[curMediaListName];
      for (var j = 0; j < curListArray.length; j++) {
        addedMediaItems += "\n    * " + curListArray[j];
      }
    }

    this._log(addedMediaLists);
    this._log(removedMediaLists);
    this._log(addedMediaItems);
  },

  // Logging helper
  _log: function(aString) {
    dump("----------------------------------------------------------\n");
    dump(" " + aString + "\n");
    dump("----------------------------------------------------------\n");
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.sbIJobProgressListener])
};

