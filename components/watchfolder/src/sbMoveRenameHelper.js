/**
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
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");

/******************************************************************************
 * Object implementing sbIJobProgress, responsible for taking a list of
 * removed paths, and a list of added paths, and figuring out if 
 * media items have been moved or renamed as a result.  
 *
 * See sbIWFMoveRenameHelper9000 for further explanation.
 *
 * Call begin() to start the job.
 *****************************************************************************/
function MoveRenameJob(aRemovedPaths, 
                       aAddedPaths 
                       aService) {
  // TODO verify inputs

  // Call super constructor
  SBJobUtils.JobBase.call(this);

  this._owner = aService;
    
  // TODO these strings probably need updating
  this._titleText = SBString("watchfolders.moverename.title");
  this._statusText =  SBString("watchfolders.moverename.processing");
  
  // Initially cancelable
  this._canCancel = true;  
}
MoveRenameJob.prototype = {
  __proto__: SBJobUtils.JobBase.prototype,
  
  QueryInterface: XPCOMUtils.generateQI(
    [Ci.sbIJobProgress, Ci.sbIJobProgressListener,
     Ci.sbIJobCancelable, Ci.nsIObserver, Ci.nsIClassInfo]),
  
  /** For nsIClassInfo **/
  getInterfaces: function(count) {
    var interfaces = [Ci.sbIJobProgress, Ci.sbIJobProgressListener,
                      Ci.sbIJobCancelable, Ci.nsIClassInfo, 
                      Ci.nsIObserver, Ci.nsISupports];
    count.value = interfaces.length;
    return interfaces;
  },

  // MoveRenameHelperService
  _owner: null,
  
  
  /**
   * Begin the import job
   */
  begin: function MoveRenameJob_begin() {
    // TODO
    
    /*
    Algorithm:

    use leafname and filesize to update moved items.
    this should work most of the time, and worst case you point to the wrong instance of the same track.

    for each file in addedList
      push file into map[filesize][leafname]

    get all items for removedFiles as removedItems

    for each item in removedItems
      if map[filesize][leafname]
        item.contentSrc = unshift map[filesize][leafname]
      else
        push item into remainingItems

    remainingItems are either items that were removed, or items that were renamed.
    get desperate and use filesize only to try to resolve renames.

    for each item in remainingItems
      if map[filesize]
        item.contentSrc = map[filesize][whateverleafname]  // should be only one
        delete map[filesize]
      else
        push item into itemsToRemove

    at this point we've done all we can do.  whatever is left needs to be removed/added

    delete itemsToRemove from the library

    newFiles = []
    for keys in map
      push path into newFiles

    import newFiles into library
    */
    
    
    this.complete();
  },
  

  /**
   * Called when a sub-job (set via Job.delegateJobProgress) completes.
   */
  onJobDelegateCompleted: function MoveRenameJob_onJobDelegateCompleted() {

    // Track overall progress, so that the progress bar reflects
    // the total number of items to process, not the individual 
    // batches.
    this._progress += this._jobProgressDelegate.progress;
    
    // Stop delegating
    this.delegateJobProgress(null);
    
    // XXX TODO Needed?
  },

  /**
   * Override JobBase.progress, to make sure the metadata scan progress
   * bar reflects the total item count, not the current batch
   */
  get progress() {
    return (this._jobProgressDelegate) ? 
      this._jobProgressDelegate.progress + this._progress : this._progress;
  },
  
  get total() {
    return this._total;
  },
  
  /**
   * Override JobBase.titleText, since we want to maintain
   * the same title throughout the import process
   */
  get titleText() {
    return this._titleText;
  },

  /** sbIJobCancelable **/
  cancel: function MoveRenameJob_cancel() {
    if (!this.canCancel) {
      throw new Error("MoveRenameJob not currently cancelable")
    }
    
    if (this._jobProgressDelegate) {
      // Cancelling the sub-job will trigger onJobDelegateCompleted
      this._jobProgressDelegate.cancel();
    } else {
      this.complete();
    }
  },
  
  /**
   * Stop everything, complete the job.
   */
  complete: function MoveRenameJob_complete() {
        
    this._status = Ci.sbIJobProgress.STATUS_SUCCEEDED;
    this._statusText = SBString("watchfolders.moverename.complete"); // TODO
    this.notifyJobProgressListeners();
    
    this._owner.onJobComplete();
  },
  
  /**
   * nsIObserver, for nsITimer
   */
  observe: function MoveRenameJob_observe(aSubject, aTopic, aData) {
    // TODO
  }
}






/******************************************************************************
 * Object implementing sbIWFMoveRenameHelper9000. Used to start a 
 * new processing job.
 *****************************************************************************/
function MoveRenameHelper() {  
  this._jobs = [];
}

MoveRenameHelper.prototype = {
  classDescription: "Songbird Watch Folder Move/Rename Helper Service",
  classID:          Components.ID("{02ba1ba0-fee5-11dd-87af-0800200c9a66}"),
  contractID:       "@songbirdnest.com/Songbird/MoveRenameHelper;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIWFMoveRenameHelper9000]),
  
  // List of pending jobs.  We only want to allow one to run at a time, 
  // since this can lock up the UI.
  _jobs: null,
  
  /**
   * \brief Start a job for the given added/removed paths
   */
  processPaths: function MoveRenameHelper_processPaths(aRemovedPaths, aAddedPaths) {

    var job = new MoveRenameJob(aRemovedPaths, aAddedPaths, this);
    
    this._jobs.push(job);
    
    // If this is the only job, just start immediately.
    // Otherwise it will be started when the current job finishes.
    if (this._jobs.length == 1) {
      job.begin();
    }
    
    return job;
  },
  
  /**
   * \brief Called by the active job when it completes
   */
  onJobComplete: function MoveRenameHelper_onJobComplete() {
    this._jobs.shift();
    if (this._jobs.length > 0) {
      this._jobs[0].begin();
    }
  }

} // MoveRenameHelper.prototype


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([MoveRenameHelper]);
}
