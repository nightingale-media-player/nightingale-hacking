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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const SB_SHUTDOWNSERVICE_CLASSNAME  = "sbShutdownService";
const SB_SHUTDOWNSERVICE_DESC       = "Songbird Shutdown Service";
const SB_SHUTDOWNSERVICE_CONTRACTID = "@songbirdnest.com/shutdown-service;1";
const SB_SHUTDOWNSERVICE_CID        = "{594137ED-DB4A-4530-8635-2573C018B4FB}";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/WindowUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");

//------------------------------------------------------------------------------
// Songbird Shutdown Job Service

var gTestListeners = [];

function sbShutdownJobService()
{
  var observerService = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
  observerService.addObserver(this, "quit-application-requested", false);
}

sbShutdownJobService.prototype = 
{
  _mTasks:          [], 
  _mTaskIndex:      0,
  _mTotal:          0,
  _mProgress:       0,
  _mListeners:      [],
  _mShouldShutdown: true,
  _mShutdownFlags:  Ci.nsIAppStartup.eAttemptQuit,
  _mStatus:         Ci.sbIJobProgress.STATUS_RUNNING,

  // nsIObserver
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "quit-application-requested") {

      var observerService = Cc["@mozilla.org/observer-service;1"]
                              .getService(Ci.nsIObserverService);
      // Only hault shutdown if there are tasks to be processed.
      var listenerEnum = 
        observerService.enumerateObservers("songbird-shutdown");
      while (listenerEnum.hasMoreElements()) {
        try { 
          var curTask = 
            listenerEnum.getNext().QueryInterface(Ci.sbIShutdownJob);
          
          if (curTask.needsToRunTask) {
            this._mTasks.push(curTask);
          }
        }
        catch (e) {
          Cu.reportError(e);
        }
      }

      if (this._mTasks.length > 0) {
        // There are tasks to run, hault shutdown for now.
        var stopShutdown = aSubject.QueryInterface(Ci.nsISupportsPRBool);
        stopShutdown.data = true;

        // If the |aData| flag indicates that this is going to be a restart,
        // append the restart flag for when we do shutdown.
        if (aData == "restart") {
          this._mShutdownFlags |= Ci.nsIAppStartup.eRestart;
        }

        // If this notice was made by the unit test, don't shutdown once
        // all the tasks have been processed.
        if (aData && aData == "is-unit-test") {
          this._mShouldShutdown = false;
        }
      
        this._startProcessingTasks();
      }
    }
  },

  // sbIJobProgressListener
  onJobProgress: function(aJobProgress) {
    this._notifyListeners();

    switch (aJobProgress.status) {
      case Ci.sbIJobProgress.STATUS_FAILED:
        // If the job failed - report the errors and continue on to the next
        // shutdown task.
        Cu.reportError("sbShutdownJobService - shutdown job failed!");

      case Ci.sbIJobProgress.STATUS_SUCCEEDED:
        // Clean up ourselves on the current task.
        var curTask = this._mTasks[this._mTaskIndex];
        curTask.removeJobProgressListener(this);

        // Update the cumulative progress count
        this._mProgress += this._mTasks[this._mTaskIndex].total;

        // Process the next task.
        this._mTaskIndex++;
        this._processNextTask();
        break;
    }
  },

  // sbIJobProgress
  get status() {
    return this._mStatus;
  },

  get blocked() {
    return false;
  },

  get statusText() {
    var statusText = this._mTasks[this._mTaskIndex].statusText;
    var remainingTasks = (this._mTasks.length - this._mTaskIndex) -1;

    if (statusText == "") {
      // If the shutdown task does not provide a string, use a generic one.
      statusText = SBString("shutdownservice.defaultstatustext"); 
    }

    if (remainingTasks == 0) {
      return statusText; 
    }
    else {
      return SBFormattedString("shutdownservice.statustext",
                               [statusText, remainingTasks]);
    }
  },

  get titleText() {
    return SBString("shutdownservice.statustitle");
  },

  get progress() {
    return this._mProgress + this._mTasks[this._mTaskIndex].progress;
  },

  get total() {
    return this._mTotal;
  },

  get errorCount() {
    return 0;
  },

  getErrorMessages: function() {
    // Don't report any error messages for now.
    return null;
  },

  addJobProgressListener: function(aJobListener) {
    this._mListeners.push(aJobListener);
  },

  removeJobProgressListener: function(aJobListener) {
    var listenerIndex = this._mListeners.indexOf(aJobListener);
    if (listenerIndex > -1) {
      this._mListeners.splice(listenerIndex, 1);
    }
  },

  // sbIJobCancelable
  get canCancel() {
    return true;
  },

  cancel: function() {
    this._doShutdown();
  },

  _startProcessingTasks: function() {
    // Calculate the total metric for the job progress interface.
    for (var i = 0; i < this._mTasks.length; i++) {
      this._mTotal += this._mTasks[i].total;
    }

    // The only open window during the shutdown service should be the Songbird
    // window. First, close down the main Songbird window.
    var winMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                        .getService(Ci.nsIWindowMediator);
    var songbirdWin = winMediator.getMostRecentWindow("Songbird:Main");
    if (songbirdWin) {
      songbirdWin.close();
    }

    var args = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    args.appendElement(this, false);

    // Open the progress dialog using the hidden window since the shutdown
    // progress dialog should be the only open Songbird window.
    var appShellService = Cc["@mozilla.org/appshell/appShellService;1"]
                            .getService(Ci.nsIAppShellService);

    var parentWindow = appShellService.hiddenDOMWindow;
    parentWindow.openDialog("chrome://songbird/content/xul/jobProgress.xul",
                            "job_progress_dialog",
                            "chrome,centerscreen",
                            args);

    this._processNextTask();
  },

  _processNextTask: function() {
    if (this._mTaskIndex < this._mTasks.length) {
      this._notifyListeners();
      
      var nextTask = this._mTasks[this._mTaskIndex];
      try {
        nextTask.addJobProgressListener(this);
        nextTask.startTask(this);
      }
      catch (e) {
        Cu.reportError("Error processing shutdown task!");
        this._mTaskIndex++;
        this._processNextTask();
      }
    }
    else {
      this._doShutdown();
    }
  },

  _doShutdown: function() {
    // Sleep for 100ms to allow the dialog to draw the complete progressbar
    // and not close the dialog with a large chunk of "unfinished" progress.
    this._mTaskIndex--;  // make sure to show the last task
    this._notifyListeners();
    this._sleep(100);
    
    // Notify the listener (this will close the progress dialog)
    this._mStatus = Ci.sbIJobProgress.STATUS_SUCCEEDED;
    this._notifyListeners();

    // drop the references to the tasks
    this._mTasks.splice(0);

    // Attempt to shutdown again.
    if (this._mShouldShutdown) {
      var appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                         .getService(Ci.nsIAppStartup);
      appStartup.quit(this._mShutdownFlags);
    }
  },

  _notifyListeners: function() {
    // Clone the listeners, otherwise removing one in the middle can 
    // cause issues.
    var listeners = [].concat(this._mListeners); 
    for (var i = 0; i < listeners.length; i++) {
      try {
        listeners[i].onJobProgress(this);
      }
      catch (e) {
        Cu.reportError(e);
      }
    }
  },

  _sleep: function(ms) {
    var threadManager = Cc["@mozilla.org/thread-manager;1"]
                          .getService(Ci.nsIThreadManager);
    var mainThread = threadManager.mainThread;
    var then = new Date().getTime(), now = then;
    for (; now - then < ms; now = new Date().getTime()) {
      mainThread.processNextEvent(true);
    }
  },

  // XCOM Goo
  className: SB_SHUTDOWNSERVICE_CLASSNAME,
  classDescription: SB_SHUTDOWNSERVICE_DESC,
  classID: Components.ID(SB_SHUTDOWNSERVICE_CID),
  contractID: SB_SHUTDOWNSERVICE_CONTRACTID,
  _xpcom_categories: [{
	 category: "app-startup",
	 service: true
  }],
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, 
                                         Ci.sbIJobProgressListener,
                                         Ci.sbIJobProgress,
                                         Ci.sbIJobCancelable]) 
};

//------------------------------------------------------------------------------
// XPCOM Registration

var NSGetModule = XPCOMUtils.generateNSGetFactory([sbShutdownJobService]);

//    function(aCompMgr, aFileSpec, aLocation) {
//      XPCOMUtils.categoryManager.addCategoryEntry("app-startup",
//                                                  SB_SHUTDOWNSERVICE_DESC,
//                                                  "service," +
//                                                  SB_SHUTDOWNSERVICE_CONTRACTID,
//                                                  true,
//                                                  true);
//    }
//  );
//}
