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
Cu.import("resource://app/jsmodules/GeneratorThread.jsm");

function LOG(aMessage) {
  dump("MoveRenameJob: " + aMessage + "\n");
}

/******************************************************************************
 * Object implementing sbIJobProgress, responsible for taking a list of
 * removed paths, and a list of added paths, and figuring out if 
 * media items have been moved or renamed as a result.  
 *
 * See sbIWFMoveRenameHelper9000 for further explanation.
 *
 * Call begin() to start the job.
 *****************************************************************************/
function MoveRenameJob(aItems, 
                       aPaths, 
                       aService,
                       aListener)
{
  if (!aItems || !aPaths) {
    throw Components.results.NS_ERROR_INVALID_ARG;
  }

  // Call super constructor
  SBJobUtils.JobBase.call(this);

  this._owner = aService;
  this._listener = aListener;
  this._mediaItems = ArrayConverter.JSArray(aItems);
  this._paths = aPaths;
  
  this._map = {};
    
  this._titleText = SBString("watchfolders.moverename.title");
  this._statusText =  SBString("watchfolders.moverename.processing");

  this._canCancel = true;  
}
MoveRenameJob.prototype = {
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

  // MoveRenameHelperService
  _owner: null,
  _listener: null,
  
  _thread: null,
  _mediaItems: null,
  _paths: null,
  
  // map[filesize][leafname] = path, based on _paths, built by _buildMap
  // used as a lookup table when figuring out which media item
  // to associate with which new path
  _map: null,
  
  /**
   * Begin the import job
   */
  begin: function MoveRenameJob_begin() {
    var job = this;

    // Add the specified job listener to the super class.
    this.addJobProgressListener(this._listener);

    this._thread = new GeneratorThread(this._process());
    this._thread.period = 33;
    this._thread.maxPctCPU = 60;
    this._thread.start();
  },
  
  /**
   * Main run method, driven by _thread
   */
  _process: function MoveRenameJob_process() {
    try { 
      yield this._buildMap();
      yield this._processMovedItems();
      yield this._processRenamedItems();
      yield this._processRemaining();
    } catch (e) {
      dump(e);
    }
    
    // Complete, unless we've started a sub-job, 
    // in which case it will call complete
    if (!this._jobProgressDelegate) {
      this.complete(); 
    }
  },
  
  /**
   * Map all new paths by leafname and file size
   */
  _buildMap: function MoveRenameJob__buildMap() {
    for (var path in ArrayConverter.JSEnum(this._paths)) {
      var file = path.QueryInterface(Ci.nsIFileURL).file;
      var size = "" + file.fileSize; 
      var name = file.leafName;
      if (!(size in this._map)) this._map[size] = {};
      if (!(name in this._map[size])) this._map[size][name] = [];
      this._map[size][name].push(path);
      yield this._yieldIfShouldWithStatusUpdate();
    }
    this._paths = null;
    //LOG(uneval(this._map));
  },
  
  /**
   * Use the name/filesize map to guess moved files
   */
  _processMovedItems: function MoveRenameJob__processMovedItems() {
    this._total = this._mediaItems.length;
       
    var remainingItems = [];
    for each (var item in this._mediaItems) {
      var file = item.contentSrc.QueryInterface(Ci.nsIFileURL).file;
      var name = file.leafName;
      this._progress++;
      var size = item.getProperty(SBProperties.contentLength);
      // If we have exactly one file with the same
      // name and size, it is probably the same file
      if (this._map[size] && this._map[size][name] && 
          this._map[size][name].length == 1) {
        //LOG("found moved item " + item.contentSrc.spec);
        item.contentSrc = this._map[size][name][0];
        delete this._map[size][name];
      } else {
        //LOG(item.contentSrc.spec + " was not moved");
        remainingItems.push(item);
      }
      
      yield this._yieldIfShouldWithStatusUpdate();
    }
    
    this._mediaItems = remainingItems;
  },

  /**
   * Get desperate and use file size only to guess renames
   * in the remaining files (could go wrong, but unlikely)
   */
  _processRenamedItems: function MoveRenameJob__processRenamedItems() {
    this._total += this._mediaItems.length;

    // What is left in _mediaItems are either items that were removed, 
    // or items that were renamed. 
    var remainingItems = [];
    for each (var item in this._mediaItems) {
      this._progress++;
      var size = item.getProperty(SBProperties.contentLength);
      if (this._map[size]) {
        var count = 0;
        var name;
        for (name in this._map[size]) count++;
        if (count == 1 && this._map[size][name].length == 1) {
          // If there is only one file name with this size, then 
          // it is probably a rename
          //LOG("found renamed item " + item.contentSrc.spec);
          item.contentSrc = this._map[size][name].shift();
        } else if (count == 0) {
          // No remaining file names for this size. Ignore.
        } else {
          // Hmm, multiple files with the same size.
          // We can't reliably rename, so just warn
          dump("Watch Folders: unable to resolve ambiguous file renames\n");
        }
        delete this._map[size];
      } else {
        //LOG(item.contentSrc.spec + " was not renamed");
        remainingItems.push(item);
      }
      
      yield this._yieldIfShouldWithStatusUpdate();
    }
    
    this._mediaItems = remainingItems;
  },

  /**
   * At this point we've done all we can do.  
   * Whatever is left needs to be removed/added
   */
  _processRemaining: function MoveRenameJob__processRemaining() {
    // Delete all the remaining media items.  We 
    // weren't able to salvage them.
    if (this._mediaItems.length > 0) {
      var library = this._mediaItems[0].library;
      //LOG("deleting " + this._mediaItems.length + " remaining items");
      library.removeSome(ArrayConverter.enumerator(this._mediaItems));
    }
    
    // Find all the new paths that weren't mapped old items
    var newPaths = [];
    for (var size in this._map) {
      for (var name in this._map[size]) {
        newPaths = newPaths.concat(this._map[size][name]);
      }
    }
    this._map = null; 
    
    if (newPaths.length > 0) {
      //LOG("adding " + newPaths.length + " new items");
      // Add them as new media items
      var importer = Cc['@songbirdnest.com/Songbird/DirectoryImportService;1']
                       .getService(Ci.sbIDirectoryImportService);
      var importJob = importer.import(ArrayConverter.nsIArray(newPaths));
      this.delegateJobProgress(importJob);
    }
  },

  /**
   * Yield the generator thread if needed, and 
   * update the job progress
   */
  _yieldIfShouldWithStatusUpdate: 
  function MoveRenameJob__yieldIfShouldWithStatusUpdate()  {
    if (GeneratorThread.shouldYield())
      yield this._yieldWithStatusUpdate();
  },

  /**
   * Update job progress
   */
  _yieldWithStatusUpdate: 
  function MoveRenameJob_yieldIfShouldWithStatusUpdate()  {
    this.notifyJobProgressListeners();
    yield;
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
    
    this.complete();
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
      throw new Error("MoveRenameJob not currently cancelable");
    }

    if (this._thread) {
      this._thread.terminate();
      this._thread = null;
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
    this._statusText = SBString("watchfolders.moverename.complete"); 
    this.notifyJobProgressListeners();

    // Remove the job listener from the super class.
    this.removeJobProgressListener(this._listener);
    this._listener = null;
    
    this._owner.onJobComplete();
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
  process: function MoveRenameHelper_process(aItems, aPaths, aListener) {
    dump("WatchFolders MoveRenameHelper_process\n");
    var job = new MoveRenameJob(aItems, aPaths, this, aListener);
    
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
        SBJobUtils.showProgressDialog(sip.data, null, 100);
      });
    }
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


var NSGetFactory = XPCOMUtils.generateNSGetFactory([MoveRenameHelper]);
