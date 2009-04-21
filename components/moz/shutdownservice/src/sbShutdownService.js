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
  _mListeners:      [],
  _mShouldShutdown: true,
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
    switch (aJobProgress.status) {
      case Ci.sbIJobProgress.STATUS_FAILED:
        // If the job failed - report the errors and continue on to the next
        // shutdown task.
        Cu.reportError("sbShutdownJobService - shutdown job failed!");

      case Ci.sbIJobProgress.STATUS_SUCCEEDED:
        // Clean up ourselves on the current task.
        var curTask = this._mTasks[this._mTaskIndex];
        curTask.removeJobProgressListener(this);

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

  get statusText() {
    return SBFormattedString("shutdownservice.statustext",
                             [(this._mTaskIndex + 1), this._mTasks.length]);
  },

  get titleText() {
    return SBString("shutdownservice.statustitle");
  },

  get progress() {
    return this._mTaskIndex * 10;
  },

  get total() {
    return this._mTasks.length * 10;
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
    this._notifyListeners();

    if (this._mTaskIndex < this._mTasks.length) {
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
    // Notify the listener and attempt to shutdown again.
    this._mStatus = Ci.sbIJobProgress.STATUS_SUCCEEDED;
    this._notifyListeners();

    if (this._mShouldShutdown) {
      var appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                         .getService(Ci.nsIAppStartup);
      appStartup.quit(Ci.nsIAppStartup.eAttemptQuit);
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

  // XCOM Goo
  className: SB_SHUTDOWNSERVICE_CLASSNAME,
  classDescription: SB_SHUTDOWNSERVICE_DESC,
  classID: Components.ID(SB_SHUTDOWNSERVICE_CID),
  contractID: SB_SHUTDOWNSERVICE_CONTRACTID,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, 
                                         Ci.sbIJobProgressListener,
                                         Ci.sbIJobProgress,
                                         Ci.sbIJobCancelable]) 
};

//------------------------------------------------------------------------------
// XPCOM Registration

function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule([sbShutdownJobService],
    function(aCompMgr, aFileSpec, aLocation) {
      XPCOMUtils.categoryManager.addCategoryEntry("app-startup",
                                                  SB_SHUTDOWNSERVICE_DESC,
                                                  "service," +
                                                  SB_SHUTDOWNSERVICE_CONTRACTID,
                                                  true,
                                                  true);
    }
  );
}

