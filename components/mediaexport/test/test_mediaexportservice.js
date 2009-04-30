/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

Components.utils.import("resource://app/jsmodules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");


var TEST_FILES = newAppRelativeFile("testharness/mediaexport/files"); 


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
  _mainLibrary          : null,
  _playlist             : null,
  _smartPlaylist        : null,

  // Make the test controller start the testing.
  startTest: function() {
    this._log("Starting mediaexport service test!");
    this._mainLibrary = LibraryUtils.mainLibrary;

    // Create a the URI's for the tracks to add to the library.
    var rootUri = newFileURI(TEST_FILES).spec.toLowerCase();
    this._tracks[0] = rootUri + "one.mp3";
    this._tracks[1] = rootUri + "two.mp3";
    this._tracks[2] = rootUri + "three.mp3";

    // Make sure the tracks, playlist, and smart playlist are not currently
    // in the library.
    this._mainLibrary.clear();

    // Clean up an existing export tasks
    removeAllTaskFiles();

    // Access the shutdown service via the |sbIShutdownJob| interface
    this._shutdownService = Cc["@songbirdnest.com/media-export-component;1"]
                              .getService(Ci.sbIShutdownJob);
    this._shutdownService.addJobProgressListener(this);

    // Finally, kick off the test.
    this._runCurrentPhase();
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
        assertTrue(parsedTask.getAddedMediaLists().length == 0);

        // There shouldn't be any removed media lists.
        assertTrue(parsedTask.getRemovedMediaLists().length == 0);

        // Ensure the added tracks to the main library
        var addedLibraryItems = parsedTask.getAddedMediaItems()["Library"];
        assertTrue(addedLibraryItems);

        // There should only be three added media items.
        assertTrue(addedLibraryItems.length == 3);
        assertTrue(addedLibraryItems[0] == this._tracks[0]);
        assertTrue(addedLibraryItems[1] == this._tracks[1]);
        assertTrue(addedLibraryItems[2] == this._tracks[2]);
        break;

      case 1:
        // Ensure that the 2 playlists were added and that the 3 mediaitems
        // have been added to the simple playlist.
        var addedMediaLists = parsedTask.getAddedMediaLists();
        assertTrue(addedMediaLists.length == 2);
        assertTrue(addedMediaLists[0] == this._playlist.name);
        assertTrue(addedMediaLists[1] == this._smartPlaylist.name);

        // Should be no removed playlists.
        var removedLists = parsedTask.getRemovedMediaLists();
        assertTrue(removedLists.length == 0);

        // Should be 3 added mediaitems to |this._playlist|.
        var addedMediaItems = 
          parsedTask.getAddedMediaItems()[this._playlist.name];
        assertTrue(addedMediaItems.length == 3);

        assertTrue(addedMediaItems[0] == this._tracks[0]);
        assertTrue(addedMediaItems[1] == this._tracks[1]);
        assertTrue(addedMediaItems[2] == this._tracks[2]);
        break;

      case 2:
        // There shouldn't be any added media lists.
        assertTrue(parsedTask.getAddedMediaLists().length == 0);

        // There should be 2 removed media lists.
        var removedMediaLists = parsedTask.getRemovedMediaLists();
        assertTrue(removedMediaLists.length == 2);

        assertTrue(removedMediaLists[0] == this._playlist.name);
        assertTrue(removedMediaLists[1] == this._smartPlaylist.name);

        // There shouldn't be any added media items.
        var hasAddedContent = false;
        for (var item in parsedTask.getAddedMediaItems()) {
          hasAddedContent = true;
          this._log("FOUND " + item + " as an added mediaitem ERROR!!!!");
        }
        assertTrue(!hasAddedContent);

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
      setExportNothing();
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

