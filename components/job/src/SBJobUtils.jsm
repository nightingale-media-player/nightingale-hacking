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
 
EXPORTED_SYMBOLS = [ "SBJobUtils" ];

// Amount of time to wait before launching a progress dialog
const PROGRESS_DEFAULT_DIALOG_DELAY = 1000;

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/WindowUtils.jsm");
Cu.import("resource://app/jsmodules/SBTimer.jsm");

/******************************************************************************
 * A collection of functions for manipulating sbIJob* interfaces
 *****************************************************************************/
var SBJobUtils = {
 
  /**
   * \brief Display a progress dialog for an object implementing sbIJobProgress.
   *
   * If the job has not completed in a given amount of time, display
   * a modal progress dialog. 
   *
   * \param aJobProgress    sbIJobProgress interface to be displayed.
   * \param aWindow         Parent window for the dialog. Can be null.
   * \param aTimeout        Time to wait before opening a dialog. 
   *                        Defaults to 1 second.  Pass 0 to open 
   *                        the dialog immediately.
   * \param aNonModal       if true the dialog is not modal
   */
  showProgressDialog: function(aJobProgress, aWindow, aTimeout, aNonModal) {
    if (!(aJobProgress instanceof Ci.sbIJobProgress)) {
      throw new Error("showProgressDialog requires an object implementing sbIJobProgress");
    }

    function showDialog() {
      if (timer) {
        timer.cancel();
        timer = null;
      }
      
      // If the job is already complete, skip the dialog
      if (aJobProgress.status == Ci.sbIJobProgress.STATUS_SUCCEEDED) {
        return;
      }
     
      // If the job is blocked and the dialog is modal, skip the dialog to avoid
      // blocking the UI
      if (!aNonModal && aJobProgress.blocked) {
        return;
      }

      // Parent to the main window by default
      if (!aWindow || aWindow.closed) {
        var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                           .getService(Components.interfaces.nsIWindowMediator);
        aWindow = wm.getMostRecentWindow("Songbird:Main");
      }
      WindowUtils.openDialog(
        aWindow,
         "chrome://songbird/content/xul/jobProgress.xul",
         "job_progress_dialog",
         "chrome,centerscreen",
         !aNonModal,
         [ aJobProgress ],
         null,
         null);
    } 
   
    if (aTimeout == null) {
      aTimeout = PROGRESS_DEFAULT_DIALOG_DELAY;
    }
    
    var timer;
    if (aTimeout) {
      // Wait a bit before launching the dialog.  If the job is already
      // complete don't bother launching it.
      // The timer will maintain a ref to this closure during the delay, and vice versa.
      timer = new SBTimer(showDialog, aTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
    } else {
      // If 0 just show immediately
      showDialog();
    }
  }
  
}


/******************************************************************************
 * A base implementation of sbIJobProgress, to be extended
 * when creating a JavaScript sbIJobProgress object.
 *
 * Supports forwarding progress information to a child sbIJobProgress,
 * for cases where a main job needs to aggregate a number of sub-jobs.
 *
 * See DirectoryImportJob for an example.
 *****************************************************************************/
SBJobUtils.JobBase = function() {
  this._errorMessages = [];
  this._listeners = [];
}
SBJobUtils.JobBase.prototype = {
  QueryInterface          : XPCOMUtils.generateQI(
      [Ci.sbIJobProgress, Ci.sbIJobCancelable, 
       Ci.sbIJobProgressListener, Ci.nsIClassInfo]),

  /** nsIClassInfo, so callers don't have to QI from JS **/
  
  classDescription        : 'Songbird Job Progress Implementation',
  classID                 : null,
  contractID              : null,
  flags                   : Ci.nsIClassInfo.MAIN_THREAD_ONLY,
  implementationLanguage  : Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage    : function(aLanguage) { return null; },
  getInterfaces : function(count) {
    var interfaces = [Ci.sbIJobProgress,
                      Ci.sbIJobCancelable,
                      Ci.sbIJobProgressListener,
                      Ci.nsIClassInfo,
                      Ci.nsISupports
                     ];
    count.value = interfaces.length;
    return interfaces;
  },

  
  _status              : Ci.sbIJobProgress.STATUS_RUNNING,
  _blocked             : false,
  _statusText          : "",
  _titleText           : "",
  _progress            : 0,
  _total               : 0,
  
  // Array of error strings
  _errorMessages       : null,
  
  // Array of listeners
  _listeners           : null,
  
  // Another sbIJobProgress. If set, all sbIJobProgress calls other than
  // status and listeners will be forwarded through to the other job.
  // This can be used to provide a single interface that aggregates
  // several sub-jobs.
  _jobProgressDelegate : null,
  
  
  /** sbIJobProgress **/
  
  get status() {
    // Note that status is not delegated, as the main job only completes
    // when all sub-jobs are finished.
    return this._status;
  },
  
  get blocked() {
    return (this._jobProgressDelegate) ?
      this._jobProgressDelegate.blocked : this._blocked;
  },

  get statusText() {
    return (this._jobProgressDelegate) ? 
      this._jobProgressDelegate.statusText : this._statusText;
  },
  
  get titleText() {
    return (this._jobProgressDelegate) ? 
      this._jobProgressDelegate.titleText : this._titleText;
  },
  
  get progress() {
    return (this._jobProgressDelegate) ? 
      this._jobProgressDelegate.progress : this._progress;
  },
  
  get total() {
    return (this._jobProgressDelegate) ? 
      this._jobProgressDelegate.total : this._total;
  },
  
  get errorCount() {
    return (this._jobProgressDelegate) ? 
      this._jobProgressDelegate.errorCount : this._errorMessages.length;
  },
  
  getErrorMessages: function Job_getErrorMessages() {
    return (this._jobProgressDelegate) ? 
      this._jobProgressDelegate.getErrorMessages() :
      ArrayConverter.enumerator(this._errorMessages);
  },
  
  addJobProgressListener: function Job_addJobProgressListener(aListener) {
    aListener.QueryInterface(Ci.sbIJobProgressListener);
    this._listeners.push(aListener);
  },
  
  removeJobProgressListener: function Job_removeJobProgressListener(aListener){
    var index = this._listeners.indexOf(aListener);
    if (index > -1) {
      this._listeners.splice(index,1);
    }
  },
  
  
  /** sbIJobCancelable **/
  
  _canCancel: false,
  get canCancel() {
    return (this._jobProgressDelegate && 
            this._jobProgressDelegate instanceof Ci.sbIJobCancelable) ? 
              this._jobProgressDelegate.canCancel : this._canCancel;
  },
  
  cancel: function Job_cancel() {
    // Override this method
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },
  
  
  /** Non-interface methods **/
  
  /**
   * Delegate most of sbIJobProgress to the given sub-job.
   * Allows this job to wrap/aggregate other jobs.
   * Pass null to stop delegation.
   */
  delegateJobProgress: function Job_delegateJobProgress(aJob)  {
    if (aJob != null && !(aJob.QueryInterface(Ci.sbIJobProgress))) {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    
    if (this._jobProgressDelegate) {
      this._jobProgressDelegate.removeJobProgressListener(this);
    }
    
    this._jobProgressDelegate = aJob;
    
    if (this._jobProgressDelegate) {
      this._jobProgressDelegate.addJobProgressListener(this);
    }
  },


  /**
   * sbIJobProgressListener implementation. Called by 
   * _jobProgressDelegate if set.
   */
  onJobProgress: function Job_onJobProgress(aJob) {
    try {
        if ((!this._jobProgressDelegate) || this._jobProgressDelegate != aJob) {
          // Just report the error. Don't throw, as that would just go to the
          // sub-job, and wouldn't help.
          Cu.reportError("Job_onJobProgress called with invalid _jobProgressDelegate state!");
        }
        
        // Forward the progress notification on to all our listeners
        this.notifyJobProgressListeners();
        
        // If the delegate is done, allow the main job to move on
        if (this._jobProgressDelegate.status != Ci.sbIJobProgress.STATUS_RUNNING) {
          this.onJobDelegateCompleted();
        }
     }
     catch (e) {
       Cu.reportError(e);
     }
  },
  
  
  /**
   * Called when a sub-job (set with delegateJobProgress) completes.
   * Override this method to carry on with other work.
   */
  onJobDelegateCompleted: function Job_onJobDelegateCompleted() {
    // Base behaviour is to just throw away the delegated job
    this.delegateJobProgress(null);
  },


  /**
   * Poke listeners and tell them to check the current progress
   */  
  notifyJobProgressListeners: function Job_notifyJobProgressListeners() {
    var thisJob = this;
    // need to clone the array, otherwise removing one in the middle gets
    // things confused
    var listeners = [].concat(this._listeners);
    listeners.forEach( function (listener) {
      try {
        listener.onJobProgress(thisJob);
      } catch (e) {
        Cu.reportError(e);
      }
    });
  }
}


