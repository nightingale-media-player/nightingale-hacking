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


function runTest()
{
  // If the file-system watcher is not supported on this system, just return.
  var fsWatcher = Cc["@songbirdnest.com/filesystem/watcher;1"]
                    .createInstance(Ci.sbIFileSystemWatcher);
  if (!fsWatcher.isSupported) {
    return;
  }

  // To test the file system watcher, create a directory inside of the 
  // temporary working directory.

  // First, create the temporary directory.
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
// \brief Create a FS listener with a directory to watch.
// \param aWatchDir The directory to watch (as nsIFile).
// \param aFSWatcher The FS watcher to use (sbIFileSystemWatcher).
//
function sbFSListener(aWatchDir, aFSWatcher)
{
  this._watchDir = aWatchDir;
  this._fsWatcher = aFSWatcher;
}

sbFSListener.prototype = 
{
  _eventTimer:           null,
  _shutdownTimer:        null,
  _addedFile:            null,
  _changeFile:           null,
  _removeFile:           null,
  _receivedAddedEvent:   false,
  _receivedChangedEvent: false,
  _receivedRemovedEvent: false,

  //
  // \brief Start the listener and invoke a add, change, and remove event.
  //
  startTest: function()
  {
    this._log("Starting 'filesystemevents' test");

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

    this._fsWatcher.init(this, this._watchDir.path, true);
    this._fsWatcher.startWatching();
  },

  _log: function(aMessage)
  {
    dump("----------------------------------------------------------\n");
    dump(" " + aMessage + "\n");
    dump("----------------------------------------------------------\n");
  },

  // sbIFileSystemListener
  onWatcherStarted: function()
  {
    this._log(" ... file system watcher started");

    // Wait at least 1 second to start the file system events.
    this._eventTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._eventTimer.initWithCallback(this, 
                                      1000, 
                                      Ci.nsITimerCallback.TYPE_ONE_SHOT);
  },

  onWatcherStopped: function()
  {
    this._log("... file system watcher has stopped");
    
    // Cleanup
    this._addedFile.remove(false);
    this._addedFile = null;

    this._removeFile = null;

    this._changeFile.remove(false);
    this._changeFile = null;

    var retry = 5;
    while (retry) {
      try {
       this._watchDir.remove(true);
       this._watchDir = null;
       retry = 0;
      } catch (e) {
        // Try a few more times, sometimes the watche doesn't clean up in time
        --retry;
        sleep(1000);
      }
    }
    this._log("... ensuring events were received");

    // Ensure that all three of the events have been received.
    assertTrue(this._receivedAddedEvent);
    assertTrue(this._receivedChangedEvent);
    assertTrue(this._receivedRemovedEvent);

    this._fsWatcher = null;
    testFinished();    
  },

  onWatcherError: function(aErrorType, aDescription)
  {
    // Not used, but logging might be useful.
    this._log("ERROR: [" + aErrorType + "] = " + aDescription + " !!!!!!!");
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
    if (aTimer == this._eventTimer) {
      // Now create some events inside the test directory:

      // Add event:
      this._addedFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0755);

      // Remove event:
      this._removeFile.remove(false);

      // Changed event:
      var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                       .createInstance(Ci.nsIFileOutputStream);
      foStream.init(this._changeFile, -1, -1, 0);
      var junk = "garbage garbage garbage";
      foStream.write(junk, junk.length);
      foStream.close();
     
      // Setup a timer to wait to receive the events created above.
      this._shutdownTimer = Cc["@mozilla.org/timer;1"]
                              .createInstance(Ci.nsITimer);
      this._shutdownTimer.initWithCallback(this,
                                           3000,  // wait three seconds
                                           Ci.nsITimerCallback.TYPE_ONE_SHOT);
    }
    else {
      // The period to wait for file system events to be received has ended.
      // Stop watching the directory and check the results in 
      // |onWatcherStopped()|.
      this._fsWatcher.stopWatching(false);
    }
  },

  QueryInterface: 
    XPCOMUtils.generateQI( [Ci.sbIFileSystemListener, Ci.nsITimerCallback] )
};

