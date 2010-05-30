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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const STATE_PHASE1 = "PHASE 1";
const STATE_PHASE2 = "PHASE 2";
const STATE_PHASE3 = "PHASE 3";


//
// \brief This test will test the session support for the filesystem API.
//
function runTest()
{
  // If the file-system watcher is not supported on this system, just return.
  var fsWatcher = Cc["@songbirdnest.com/filesystem/watcher;1"]
                    .createInstance(Ci.sbIFileSystemWatcher);
  if (!fsWatcher.isSupported) {
    return;
  }

  // Create a directory to use for testing: 
  var watchDir = Cc["@mozilla.org/file/directory_service;1"]
                   .getService(Ci.nsIProperties)
                   .get("ProfD", Ci.nsIFile);

  watchDir.append("watch_dir");
  if (!watchDir.exists() || !watchDir.isDirectory()) {
    watchDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0777);
  }

  var listener = new sbFSListener(watchDir, fsWatcher);
  listener.startTest();
  testPending();
}


//
// \brief Create a FS listener 
//
function sbFSListener(aWatchDir, aFSWatcher)
{
  this._watchDir = aWatchDir;
  this._fsWatcher = aFSWatcher;
}

sbFSListener.prototype =
{
  _state:                "",
  _savedSessionID:       null, 
  _addedFile:            null,
  _changeFile:           null,
  _removeFile:           null,
  _shutdownTimer:        null,
  _restartTimer:         null,
  _addedFile:            null,
  _changedFile:          null,
  _removedFile:          null,
  _receivedAddedEvent:   false,
  _receivedChangedEvent: false,
  _receivedRemovedEvent: false,

  _log: function(aMessage)
  {
    dump("----------------------------------------------------------\n");
    dump(" " + aMessage + "\n");
    dump("----------------------------------------------------------\n");
  },

  _cleanup: function()
  {
    this._addedFile.remove(false);
    this._addedFile = null;

    this._removeFile = null;
    
    this._changeFile.remove(false);
    this._changeFile = null;

    this._watchDir.remove(true);
    this._watchDir = null;

    this._shutdownTimer = null;
    this._restartTimer = null;

    this._fsWatcher = null;
    testFinished();
  },

  //
  // \brief Start running the session testing.
  //
  startTest: function()
  {
    this._log("Starting 'filesystemsession' test");

    this._shutdownTimer = Cc["@mozilla.org/timer;1"]
                            .createInstance(Ci.nsITimer);
    this._restartTimer = Cc["@mozilla.org/timer;1"]
                           .createInstance(Ci.nsITimer);

    // Create some test files before watching starts for the first time.
    this._addedFile = this._watchDir.clone();
    this._addedFile.append("added.file");
    if (this._addedFile.exists()) {
      this._addedFile.remove(false);
    }
    
    this._changeFile = this._watchDir.clone();
    this._changeFile.append("changed.file");
    if (!this._changeFile.exists()) {
      this._changeFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0777); 
    }

    this._removeFile = this._watchDir.clone();
    this._removeFile.append("removed.file");
    if (!this._removeFile.exists()) {
      this._removeFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0777);
    }

    // Set the state to phase 1.
    this._state = STATE_PHASE1;
    this._log(this._state + ": Starting");

    this._fsWatcher.init(this, this._watchDir.path, true);
    this._fsWatcher.startWatching();
  },

  // sbIFileSystemListener
  onWatcherStarted: function()
  {
    switch (this._state) {
      case STATE_PHASE1: 
      {
        this._log(this._state + ": Watcher has started");
        // In this phase, we just want to shut the watcher back down to get
        // a saved session GUID and create some serialized data on disk.
        
        // In this phase, just save the session ID and shutdown the watcher
        // with the flag to save the session.
        this._log(this._state + ": Stopping watcher, saving session.");
        this._savedSessionID = this._fsWatcher.sessionGuid;
        this._shutdownTimer.initWithCallback(this,
                                             1000,
                                             Ci.nsITimerCallback.TYPE_ONE_SHOT);
        break;
      }

      case STATE_PHASE2:
      {
        this._log(this._state + ": Watcher has started");
        // Ensure that the file system events where received. All events that
        // have been discovered between application sessions get reported 
        // before |onWatcherStarted()|.
        assertTrue(this._receivedAddedEvent);
        assertTrue(this._receivedChangedEvent);
        assertTrue(this._receivedRemovedEvent);

        // Now that the events have been received, it's time to shutdown the
        // watcher and begin phase 3.
        this._state = STATE_PHASE3;
        this._log(this._state + ": Starting");
        this._fsWatcher.stopWatching(true);
        break;
      }

      case STATE_PHASE3:
      {
        // If this block is executed, something is wrong. The watcher should
        // have reported an error during startup since the saved application
        // data has been removed.
        this._log(this._state + 
                  ": ERROR: The session was not reported as an error!");
        assertTrue(false);
        this._cleanup();
      }
    }
  },

  onWatcherStopped: function()
  {
    switch (this._state) {
      case STATE_PHASE1:
      {
        this._log(this._state + ": Watcher has stopped");
        // Now that the watcher has stopped in phase 1, it is now time to start
        // phase 2. In phase 2, we want to create some events in the directory 
        // to ensure that they are reported once the watcher is re-initialized.
        this._state = STATE_PHASE2;
        this._log(this._state + ": Starting");

        // Add event:
        this._addedFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0755);

        // Remove event:
        this._removeFile.remove(false);

        // Changed event
        var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                       .createInstance(Ci.nsIFileOutputStream);
        foStream.init(this._changeFile, -1, -1, 0);
        var junk = "garbage garbage garbage";
        foStream.write(junk, junk.length);
        foStream.close();

        this._log(this._state + ": File system events created.");

        // Now set the startup timer to fire in one second to re-initialize the
        // watcher.
        this._restartTimer.initWithCallback(this,
                                            1000,
                                            Ci.nsITimerCallback.TYPE_ONE_SHOT);
        break;
      }

      case STATE_PHASE3:
      {
        this._log(this._state + ": Watcher has stopped");
        this._state = STATE_PHASE3;
        
        // Now set the restart timer back up
        this._restartTimer.initWithCallback(this,
                                            1000,
                                            Ci.nsITimerCallback.TYPE_ONE_SHOT);
        break;
      }
    }
  },

  onWatcherError: function(aErrorType, aDescription)
  {
    switch (aErrorType) {
      case Ci.sbIFileSystemListener.SESSION_LOAD_ERROR:
        if (this._state != STATE_PHASE3) {
          this._log(this._state + 
                    ": ERROR: received session load error, unexpected!");
          assertTrue(false);
        }
        else {
           this._log("PHASE 3: Got the session load error.");
        }
        break;

      case Ci.sbIFileSystemListener.ROOT_PATH_MISSING:
        doFail("ERROR: The root watch path is missing!!");
        break;

      case Ci.sbIFileSystemListener.INVALID_DIRECTORY:
        doFail("ERROR: Invalid directory was passed to watcher!");
        break;
    }

    this._cleanup();
  },

  onFileSystemChanged: function(aFilePath)
  {
    this._log("CHANGED: " + aFilePath);
    this._receivedChangedEvent = true;
  },

  onFileSystemRemoved: function(aFilePath)
  {
    this._log("REMOVED: " + aFilePath);
    this._receivedRemovedEvent = true;
  },

  onFileSystemAdded: function(aFilePath)
  {
    this._log("ADDED: " + aFilePath);
    this._receivedAddedEvent = true;
  },

  // nsITimerCallback
  notify: function(aTimer)
  {
    if (aTimer == this._restartTimer) {
      this._log(this._state + ": Re-starting the watcher with session " +
                this._savedSessionID);

      this._fsWatcher = null;
      this._fsWatcher = Cc["@songbirdnest.com/filesystem/watcher;1"]
                          .createInstance(Ci.sbIFileSystemWatcher);

      // In phase 3, the session data should be removed from disk to test
      // the error reporting API on the filesystem watcher API.
      if (this._state == STATE_PHASE3) {
        this._fsWatcher.deleteSession(this._savedSessionID);
        this._log(this._state + ": Session " + this._savedSessionID + 
                  " has been deleted");
        
      }

      this._fsWatcher.initWithSession(this._savedSessionID, this);
      this._fsWatcher.startWatching();
    }
    else {
      this._log(this._state + ": Stopping watcher.");
      this._fsWatcher.stopWatching(true);
    }
  },

  QueryInterface: 
    XPCOMUtils.generateQI( [Ci.sbIFileSystemListener, Ci.nsITimerCallback] )
};

