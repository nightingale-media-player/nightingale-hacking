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

// used to identify directory import profiling runs
var gCounter = 0;

/******************************************************************************
 * Object implementing sbIDirectoryImportJob, responsible for finding media
 * items on disk, adding them to the library and performing a metadata scan.
 *
 * Call begin() to start the job.
 *****************************************************************************/
function DirectoryImportJob(aDirectoryArray, 
                            aTargetMediaList, 
                            aTargetIndex, 
                            aImportService) {
  if (!(aDirectoryArray instanceof Ci.nsIArray && 
        aDirectoryArray.length > 0) ||
      !(aTargetMediaList instanceof Ci.sbIMediaList)) {
    throw Components.results.NS_ERROR_INVALID_ARG;
  }
  // Call super constructor
  SBJobUtils.JobBase.call(this);

  this._directories = ArrayConverter.JSArray(aDirectoryArray);
  this.targetMediaList = aTargetMediaList;
  this.targetIndex = aTargetIndex;

  this._importService = aImportService;
  
  this._itemURIs = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                     .createInstance(Components.interfaces.nsIMutableArray);
  this._itemInitialProperties = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                                  .createInstance(Components.interfaces.nsIMutableArray);

  // TODO these strings probably need updating
  this._titleText = SBString("media_scan.scanning");
  this._statusText =  SBString("media_scan.adding");
  
  // Initially cancelable
  this._canCancel = true;
  
  if ("@songbirdnest.com/Songbird/TimingService;1" in Cc) {
    this._timingService = Cc["@songbirdnest.com/Songbird/TimingService;1"]
                            .getService(Ci.sbITimingService);
    this._timingIdentifier = "DirImport" + gCounter++;
  }
}
DirectoryImportJob.prototype = {
  __proto__: SBJobUtils.JobBase.prototype,
  
  QueryInterface: XPCOMUtils.generateQI(
    [Ci.sbIDirectoryImportJob, Ci.sbIJobProgress, Ci.sbIJobProgressListener,
     Ci.sbIJobCancelable, Ci.nsIObserver, Ci.nsIClassInfo]),
  
  /** For nsIClassInfo **/
  getInterfaces: function(count) {
    var interfaces = [Ci.sbIDirectoryImportJob, Ci.sbIJobProgress, Ci.sbIJobProgressListener,
                      Ci.sbIJobCancelable, Ci.nsIClassInfo, Ci.nsISupports];
    count.value = interfaces.length;
    return interfaces;
  },

  totalAddedToMediaList     : 0,
  totalAddedToLibrary       : 0,
  totalDuplicates           : 0,  
  targetMediaList           : null,
  targetIndex               : null,
  
  // nsITimer used to poll the filescanner.  *sigh*
  _pollingTimer             : null,
  FILESCAN_POLL_INTERVAL    : 33,
  
  // sbIFileScan used to find media files in directories
  _fileScanner              : null,
  _fileScanQuery            : null,
  
  // Array of nsIFile directory paths
  _directories              : null,
  _fileExtensions           : null,
  
  // The sbIDirectoryImportService.  Called back on job completion.
  _importService            : null,
  
  // nsIMutableArray of nsIURI items for all found media items
  _itemURIs                 : null,
  
  // nsIMutableArray of sbIMutablePropertyArrays containing default
  // properties (e.g. track name), with one for every URI in _itemURIs
  _itemInitialProperties    : null,
  
  // nsIArray of previously unknown sbIMediaItems that were created during 
  // this job. Set in _onItemCreationProgress.
  _newMediaItems            : null,
  
  // JS array of all sbIMediaItems that were found during 
  // this job. This is a cache created by enumerateAllItems(),
  // and should not be used directly.
  _allMediaItems            : null,
  
  // True if we've forced the library into a batch state for
  // performance reasons
  _inLibraryBatch           : false,
  
  // Used to track performance
  _timingService            : null,
  _timingIdentifier         : null,
  
  /**
   * Get an enumerator over the previously unknown media items that were
   * discovered during this job.  Will return an empty enumerator
   * until the batch creation phase is complete.  Best called when
   * the entire job is done.
   */
  enumerateNewItemsOnly: function DirectoryImportJob_enumerateNewItemsOnly() {
    if (!this._newMediaItems) {
      this._newMediaItems = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                              .createInstance(Ci.nsIArray);      
    }
    return this._newMediaItems.enumerate();
  },
  
  /**
   * Get an enumerator over all media items found in this job.  
   * Will return an empty enumerator until the batch creation 
   * phase is complete.  Best called when the entire job is done.
   */
  enumerateAllItems: function DirectoryImportJob_enumerateAllItems() {
    // Ultimately the batch create job would give us this list, 
    // but at the moment we have to recreate it ourselves
    // using the list of URIs from the file scan.
    
    // If no URIs, just return the empty enumerator
    if (this._itemURIs.length == 0) {
      return this._itemURIs.enumerate();
    }
    
    // If all the URIs resulted in new items, we can 
    // cheat and just return that list
    if (this._newMediaItems && 
        this._itemURIs.length == this._newMediaItems.length) {
      return this.enumerateNewItemsOnly();
    }
    
    // Computing the list of media items can be expensive, so keep
    // the results cached just in case the function is called 
    // multiple times
    if (!this._allMediaItems || 
        this._allMediaItems.length != this._itemURIs.length) {
      
      // Make an array of contentURL properties, so we can search 
      // for all tracks in one go
      var propertyArray = SBProperties.createArray();
      var enumerator = this._itemURIs.enumerate();
      while (enumerator.hasMoreElements()) {
        var itemURI = enumerator.getNext().QueryInterface(Ci.nsIURI);
        propertyArray.appendProperty(SBProperties.contentURL, itemURI.spec);
      } 

      // Get media items for the URI list
      this._allMediaItems = [];
      var listener = {
        mediaItems: this._allMediaItems,
        onEnumerationBegin: function() {},
        onEnumeratedItem: function(list, item) {
          this.mediaItems.push(item);
        },
        onEnumerationEnd: function() {},
        QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaListEnumerationListener]),
      };

      this.targetMediaList.library.
        enumerateItemsByProperties(propertyArray,
                                   listener);
    }
    
    return ArrayConverter.enumerator(this._allMediaItems);
  },
    
  
  /**
   * Begin the import job
   */
  begin: function DirectoryImportJob_begin() {
    if (this._timingService) {
      this._timingService.startPerfTimer(this._timingIdentifier); 
    }
    
    // Start by finding all the files in the given directories
    
    this._fileScanner = Cc["@songbirdnest.com/Songbird/FileScan;1"]
                          .createInstance(Components.interfaces.sbIFileScan);

    // Figure out what files we are looking for.
    var typeSniffer = Cc["@songbirdnest.com/Songbird/Mediacore/TypeSniffer;1"]
                        .createInstance(Ci.sbIMediacoreTypeSniffer);
    try {
      var extensions = typeSniffer.mediaFileExtensions;
      this._fileExtensions = [];
      while (extensions.hasMore()) {
        this._fileExtensions.push(extensions.getNext());
      }
    } catch (e) {
      dump("WARNING: DirectoryImportJob_begin could not find supported file extensions.  " +
           "Assuming test mode, and using a hardcoded list.\n");
      this._fileExtensions = ["mp3", "ogg", "flac"];
    }
    
    // XXX If possible, wrap the entire operation in an update batch
    // so that onbatchend listeners dont go to work between the 
    // end of the batchcreate and the start of the metadata scan.
    // This is an ugly hack, but it prevents a few second hang
    // when importing 100k tracks.
    var library = this.targetMediaList.library;
    if (library instanceof Ci.sbILocalDatabaseLibrary) {
      this._inLibraryBatch = true;
      library.forceBeginUpdateBatch();
    }
    
    this._startNextDirectoryScan();
    
    // Now poll the file scan, since it apparently perf is much better this way
    this._pollingTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
    this._pollingTimer.init(this, this.FILESCAN_POLL_INTERVAL, Ci.nsITimer.TYPE_REPEATING_SLACK);
  },
  
  /**
   * Kick off a FileScanQuery for the next directory in _directories.
   * Assumes there are directories left to scan.
   */
  _startNextDirectoryScan: function DirectoryImportJob__startNextDirectoryScan() {
    // Just report the error rather then throwing, since this method
    // is called by the timer.
    if (this._fileScanQuery && this._fileScanQuery.isScanning()) {
      Cu.reportError(
        "DirectoryImportJob__startDirectoryScan called with a scan in progress?");
      return;
    }
    
    var directory = this._directories.shift(); // Process directories in the order provided
    
    // If something is messed up, just report and wait. The poll function will
    // move on to the next step.
    if (!directory || !(directory instanceof Ci.nsIFile)) {
      Cu.reportError("DirectoryImportJob__startNextDirectoryScan: invalid directory");
      return;
    }
    
    var fileScanQuery = Cc["@songbirdnest.com/Songbird/FileScanQuery;1"]
                          .createInstance(Components.interfaces.sbIFileScanQuery);
    fileScanQuery.setDirectory(directory.path);
    fileScanQuery.setRecurse(true);
    this._fileExtensions.forEach(function(ext) { fileScanQuery.addFileExtension(ext) });
    this._fileScanQuery = fileScanQuery;
    this._fileScanner.submitQuery(this._fileScanQuery);
  },
  
  /** 
   * Called by _pollingTimer. Checks on the current fileScanQuery,
   * reporting progress and checking for completion.
   */
  _onPollFileScan: function DirectoryImportJob__onPollFileScan() {
    
    // If the current query is finished, collect the results and move on to
    // the next directory
    if (!this._fileScanQuery.isScanning()) {
      
      var fileCount = this._fileScanQuery.getFileCount();
      if (fileCount > 0 ) {
        var files = this._fileScanQuery.getResultRangeAsURIs(0, fileCount - 1);
        var fileURI;
        for (var i = 0; i < files.length; i++) {
          fileURI = files.queryElementAt(i, Components.interfaces.nsIURI);
          this._itemURIs.appendElement(fileURI, false);
        
          // Create a temporary track name for each of the found items
          var props =
            Components.classes["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                      .createInstance(Components.interfaces.sbIMutablePropertyArray);
          props.appendProperty(SBProperties.trackName, this._createDefaultTitle(fileURI));
          this._itemInitialProperties.appendElement(props, false);
        }
      }

      // If there are more directories to scan, start the next one
      if (this._directories.length > 0) {
        this._startNextDirectoryScan();
      } else {
        // Otherwise, we're done scanning directories, and it is time 
        // to create media items
        this._finishFileScan();
        this._startMediaItemCreation();
      }

    // If the file scan query is still running just update the UI
    } else {
      var text = this._fileScanQuery.getLastFileFound();
      if (text.length > 60) {
        text = text.substring(0, 20) + "..." + text.substring(text.length - 20);
      }
      this._statusText = text;
      this.notifyJobProgressListeners();
    }
  },
  
  /** 
   * Called when the file scanner finishes with the target directories.
   * Responsible for shutting down the scanner.
   */
  _finishFileScan: function DirectoryImportJob__finishFileScan() {
    if (this._fileScanner) {
      this._fileScanner = null;
    }
    if (this._fileScanQuery) {
      this._fileScanQuery.cancel();
      this._fileScanQuery = null;   
    }
    if (this._pollingTimer) {
      this._pollingTimer.cancel();
      this._pollingTimer = null;
    }
  },
  
  /** 
   * Begin creating sbIMediaItems for all found media URIs
   */
  _startMediaItemCreation: 
  function DirectoryImportJob__startMediaItemCreation() {
    if (!this._itemURIs || !this._itemInitialProperties || 
        this._itemURIs.length != this._itemInitialProperties.length) {
      Cu.reportError(
        "DirectoryImportJob__startMediaItemCreation called with invalid state");
      this.complete();      
    }
    if (this._itemURIs.length == 0) {
      this.complete();
    }
    
    // At the moment batchmediaitemcreate is not cancelable
    this._canCancel = false;
    
    // Update status
    this._statusText = SBString("media_scan.adding");    
    this.notifyJobProgressListeners();
    
    // Bug 10228 - this needs to be replaced with an sbIJobProgress interface
    var thisJob = this;
    var batchCreateListener = {
      onProgress: function(aIndex) {},
      onComplete: function(aMediaItems, aResult) {
        thisJob._onItemCreationProgress(aMediaItems, aResult);
      }
    }
    
    if (this._timingService) {
      this._timingService.startPerfTimer(this._timingIdentifier + "-ItemCreation");
    }
    
    // Bug 10228 -  batch item creation is not cancelable!
    this._canCancel = false;
    
    // BatchCreateMediaItems needs to return an sbIJobProgress
    this.targetMediaList.library.batchCreateMediaItemsAsync(
        batchCreateListener, this._itemURIs, this._itemInitialProperties);
  },
  
  /** 
   * Called by sbILibrary.batchCreateMediaItemsAsync. 
   * BatchCreateMediaItemsAsync needs to be updated to actually send progress.
   * At the moment it only notifies when the process is over.
   */
  _onItemCreationProgress: function DirectoryImportJob__onItemCreationProgress(aMediaItems, aResult) {
    // Get the completed item array.  Don't use the given item array on error.
    // Use an empty one instead.
    if (Components.isSuccessCode(aResult)) {
      this._newMediaItems = aMediaItems;
    } else {
      Cu.reportError("DirectoryImportJob__onItemCreationProgress: aResult == " + aResult);
      this._newMediaItems = Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                                      .createInstance(Components.interfaces.nsIArray);
    }
    
    this.totalAddedToLibrary = this._newMediaItems.length;
    this.totalDuplicates = this._itemURIs.length - this._newMediaItems.length;
    
    // TODO update UI?
    
    // Handle inserting into a media list at a specific index
    this._insertFoundItemsIntoTarget();
    
    if (this._timingService) {
      this._timingService.stopPerfTimer(this._timingIdentifier + "-ItemCreation");
    }
    
    // Make sure we have metadata for all the added items
    this._startMetadataScan();
  },
  
  /** 
   * Insert found media items into the specified target location
   */
  _insertFoundItemsIntoTarget: function DirectoryImportJob__insertFoundItemsIntoTarget() {
    // If we are inserting into a list, there is more to do than just importing 
    // the tracks into its library, we also need to insert all the items (even
    // the ones that previously existed) into the list at the requested position
    if (!(this.targetMediaList instanceof Ci.sbILibrary)) {
      try {
        if (this._itemURIs.length > 0) {
          // If we need to insert, then do so
          if ((this.targetMediaList  instanceof Ci.sbIOrderableMediaList) && 
              (this.targetIndex >= 0) && 
              (this.targetIndex < this.targetMediaList.length)) {
            this.targetMediaList.insertSomeBefore(this.targetIndex, this.enumerateAllItems());
          } else {
            // Otherwise, just add
            this.targetMediaList.addSome(this.enumerateAllItems());
          }
          this.totalAddedToMediaList = this._allMediaItems.length;
        }
      } catch (e) {
        Cu.reportError(e); 
      }
    }
  },
  
  /** 
   * Begin finding the metadata for all imported items. 
   * Forward sbIJobProgress through to the metadata job
   */
  _startMetadataScan: function DirectoryImportJob__startMetadataScan() {
    if (this._newMediaItems && this._newMediaItems.length > 0) {
      this._canCancel = true;
      
      var metadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);
      var metadataJob = metadataService.read(this._newMediaItems);
    
      // Metadata job is cancelable
      this._canCancel = true;
    
      if (this._timingService) {
        this._timingService.startPerfTimer(this._timingIdentifier + "-Metadata");
      }
      
      // Pump metadata job progress to the UI
      this.delegateJobProgress(metadataJob);
    } else {
      // Nothing to do. 
      this.complete();
    }
  },

  /**
   * Called when a sub-job (set via Job.delegateJobProgress) completes.
   */
  onJobDelegateCompleted: function DirectoryImportJob_onJobDelegateCompleted() {
    // Stop delegating
    this.delegateJobProgress(null);
    
    if (this._timingService) {
      this._timingService.stopPerfTimer(this._timingIdentifier + "-Metadata");
    }
    
    // For now the only job we delegate to is metadata... so when 
    // it completes, we're done
    this.complete();
  },

  /** sbIJobCancelable **/
  cancel: function DirectoryImportJob_cancel() {
    if (!this.canCancel) {
      throw new Error("DirectoryImportJob not currently cancelable")
    }
    
    if (this._fileScanner) {
      this._finishFileScan();
      this.complete();
    } else if (this._jobProgressDelegate) {
      // Cancelling the sub-job will trigger onJobDelegateCompleted
      this._jobProgressDelegate.cancel();
    }
    
    // Remove anything that we've added.
    if (this._newMediaItems && this._newMediaItems.length > 0) {
      this.targetMediaList.library.removeSome(this._newMediaItems.enumerate());
      this.totalAddedToMediaList = 0;
      this.totalAddedToLibrary = 0;
      this.totalDuplicates = 0;
    }
  },
  
  /**
   * Stop everything, complete the job.
   */
  complete: function DirectoryImportJob_complete() {
    
    this._status = Ci.sbIJobProgress.STATUS_SUCCEEDED;
    this._statusText = SBString("media_scan.complete");
    this.notifyJobProgressListeners();
    
    this._importService.onJobComplete();
    
    // XXX If we forced the library into a batch mode
    // in order to improve performance, make sure 
    // we end the batch (if we fail to do this
    // the tree view will never update)
    var library = this.targetMediaList.library;
    if (library instanceof Ci.sbILocalDatabaseLibrary &&
        this._inLibraryBatch) 
    {
      this._inLibraryBatch = false;
      library.forceEndUpdateBatch();
    }
    
    if (this._timingService) {
      this._timingService.stopPerfTimer(this._timingIdentifier);
    }
  },
  
  /**
   * nsIObserver, for nsITimer
   */
  observe: function DirectoryImportJob_observe(aSubject, aTopic, aData) {
    this._onPollFileScan();
  },
  
  /**
   * Convert an nsIURI into a string filename
   */
  _createDefaultTitle: function DirectoryImportJob__createDefaultTitle(aURI) {

    var netutil = Cc["@mozilla.org/network/util;1"]
                    .getService(Ci.nsINetUtil);

    var s = netutil.unescapeString(aURI.spec,
                                   Ci.nsINetUtil.ESCAPE_URL_SKIP_CONTROL);

    var unicodeConverter = 
      Cc["@mozilla.org/intl/scriptableunicodeconverter"]
        .createInstance(Ci.nsIScriptableUnicodeConverter);
    unicodeConverter.charset = "UTF-8";

    try {
      s = unicodeConverter.ConvertToUnicode(s);
    } catch (ex) {
      // conversion failed, use encoded string
    }
    return s.split("/").pop();
  }
}






/******************************************************************************
 * Object implementing sbIDirectoryImportService. Used to start a 
 * new media import job.
 *****************************************************************************/
function DirectoryImportService() {  
  this._importJobs = [];
}

DirectoryImportService.prototype = {
  classDescription: "Songbird Directory Import Service",
  classID:          Components.ID("{6e542f90-44a0-11dd-ae16-0800200c9a66}"),
  contractID:       "@songbirdnest.com/Songbird/DirectoryImportService;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIDirectoryImportService]),
  
  // List of pending jobs.  We only want to allow one to run at a time, 
  // since this can lock up the UI.
  _importJobs: null,
  
  /**
   * \brief Import any media files found in the given directories to 
   *        the specified media list.
   */
  import: function DirectoryImportService_import(aDirectoryArray, aTargetMediaList, aTargetIndex) {

    // Default to main library if not target is provided
    if (!aTargetMediaList) {
      aTargetMediaList = LibraryUtils.mainLibrary;
    }
    
    var job = new DirectoryImportJob(aDirectoryArray, aTargetMediaList, aTargetIndex, this);
    
    this._importJobs.push(job);
    
    // If this is the only job, just start immediately.
    // Otherwise it will be started when the current job finishes.
    if (this._importJobs.length == 1) {
      job.begin();
    }
    
    return job;
  },
  
  /**
   * \brief Called by the active import job when it completes
   */
  onJobComplete: function DirectoryImportService_onJobComplete() {
    this._importJobs.shift();
    if (this._importJobs.length > 0) {
      this._importJobs[0].begin();
    }
  }

} // DirectoryImportService.prototype


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([DirectoryImportService]);
}
