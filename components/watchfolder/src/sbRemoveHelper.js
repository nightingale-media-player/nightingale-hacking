/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/GeneratorThread.jsm");

/******************************************************************************
 * http://bugzilla.getnightingale.com/show_bug.cgi?id=16517
 * Object implementing sbIJobProgress/sbIWFRemoveHelper9001. Pop up a remove
 * progress dialog.
 *
 * Call begin() to start the job.
 *****************************************************************************/
function RemoveJob(aItems, aHelper) {
  if (!aItems) {
    throw Components.results.NS_ERROR_INVALID_ARG;
  }

  // Call super constructor
  SBJobUtils.JobBase.call(this);

  this._owner = aHelper;
  this._mediaItems = ArrayConverter.JSArray(aItems);
  
  this._titleText = SBString("watchfolders.remove.title");
  this._statusText = SBString("watchfolders.remove.processing");
  this._progress = 0;

  this._canCancel = true;
}

RemoveJob.prototype = {
  __proto__: SBJobUtils.JobBase.prototype,
  
  QueryInterface: XPCOMUtils.generateQI(
    [Ci.sbIJobProgress, Ci.sbIJobProgressListener,
     Ci.sbIJobCancelable, Ci.nsIClassInfo]),
  
  /** For nsIClassInfo **/
  getInterfaces: function(count) {
    var interfaces = [Ci.sbIJobProgress, Ci.sbIJobProgressListener,
                      Ci.sbIJobCancelable, Ci.nsIClassInfo, 
                      Ci.nsISupports];
    count.value = interfaces.length;
    return interfaces;
  },

  // RemoveHelper
  _owner: null,

  // Media items that will be removed
  _mediaItems: null,

  // Index into _mediaItems for the beginning of the next removed batch.
  _nextItemIndex: 0,

  // The size of the current batch
  _currentBatchSize: 0,

  // Split the item removal so that the remvoe progress dialog can
  // be updated. This avoids the block introduced by removeSome.
  BATCHREMOVE_SIZE: 300,

  /**
   * Begin the remove job
   */
  begin: function RemoveJob_begin() {
    try {
      var job = this;
      this._thread = new GeneratorThread(this._process());
      this._thread.start();
    } catch (e) {
      dump(e);
    }
  },

  /**
   * Main run method, driven by _thread
   */
  _process: function RemoveJob_process() {
    this._total = this._mediaItems.length;

    try {
      var library = this._mediaItems[0].library;

      // Process the URIs a slice at a time and update the progress bar
      while (this._nextItemIndex < this._mediaItems.length) {
        yield GeneratorThread.yieldIfShould();
        this._currentBatchSize = Math.min(this.BATCHREMOVE_SIZE,
                                          this._mediaItems.length - this._nextItemIndex);
        var endIndex = this._nextItemIndex + this._currentBatchSize;
        var items = this._mediaItems.slice(this._nextItemIndex, endIndex);
        this._nextItemIndex = endIndex;
        library.removeSome(ArrayConverter.enumerator(items));
        this._progress += this._currentBatchSize;
        this.notifyJobProgressListeners();
       }
    } catch (e) {
      dump(e);
    }

    this.complete();
  },

  /**
   * Override JobBase.progress, to make sure the watch folder remove progress
   * bar reflects the total item count, not the current batch
   */
  get progress() {
    return this._progress;
  },

  get total() {
    return this._total;
  },
  
  /**
   * Override JobBase.titleText, since we want to maintain
   * the same title throughout the remove process
   */
  get titleText() {
    return this._titleText;
  },

  /** sbIJobCancelable **/
  cancel: function RemoveJob_cancel() {
    if (!this.canCancel) {
      throw new Error("RemovedJob not currently cancelable");
    }

    this.complete();
  },
  
  /**
   * Stop everything, complete the job.
   */
  complete: function RemoveJob_complete() {
        
    this._status = Ci.sbIJobProgress.STATUS_SUCCEEDED;
    this._statusText = SBString("watchfolders.remove.complete"); 
    this.notifyJobProgressListeners();

    this._owner.onJobComplete();
  }
}

/******************************************************************************
 * Object implementing sbIWFRemoveHelper9001. Used to start a 
 * new remove job.
 *****************************************************************************/
function RemoveHelper() {  
  this._jobs = [];
}

RemoveHelper.prototype = {
  classDescription: "Nightingale Watch Folder Remove Helper Service",
  classID:          Components.ID("{b44bf03e-fe28-4ad4-ba7e-6112cdced7a4}"),
  contractID:       "@getnightingale.com/Nightingale/RemoveHelper;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIWFRemoveHelper9001]),
  
  // List of pending jobs.  We only want to allow one to run at a time, 
  // since this can lock up the UI.
  _jobs: null,
  
  /**
   * \brief Start a job for the given removed items
   */
  remove: function RemoveHelper_remove(aItems) {
    var job = new RemoveJob(aItems, this);
    
    this._jobs.push(job);
    
    // If this is the only job, just start immediately.
    // Otherwise it will be started when the current job finishes.
    if (this._jobs.length == 1) {

      // Make sure things QI correctly...
      var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                  .createInstance(Ci.nsISupportsInterfacePointer);
      sip.data = job;
      
      // Wrap the job in a modal dialog and library batch
      LibraryUtils.mainLibrary.runInBatchMode(function() {
        job.begin();
        SBJobUtils.showProgressDialog(sip.data, null, 0);
      });
    }
  },
  
  /**
   * \brief Called by the active job when it completes
   */
  onJobComplete: function RemoveHelper_onJobComplete() {
    this._jobs.shift();
    if (this._jobs.length > 0) {
      this._jobs[0].begin();
    }
  }

} // RemoveHelper.prototype


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([RemoveHelper]);
}
