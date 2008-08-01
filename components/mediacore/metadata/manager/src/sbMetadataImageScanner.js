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

// Variables for convience
var Cc = Components.classes;
var Ci = Components.interfaces;
var Cu = Components.utils;
var Cr = Components.results;

// Import helper modules
Cu.import('resource://app/jsmodules/sbProperties.jsm');
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

/**
 * Since we can't use the FUEL components until after all other components have
 * been loaded we define a lazy getter here for when we need it.
 */
__defineGetter__("Application", function() {
  delete this.Application;
  return this.Application = Cc["@mozilla.org/fuel/application;1"]
                              .getService(Ci.fuelIApplication);
});

// Constanst for convinence
const PROP_LAST_COVER_SCAN = SBProperties.base + 'lastCoverScan';
const RESCAN_INTERVAL = (60*60*24*7);      // Rescan for missing art every week
const TIMER_INTERVAL = (5 * 1000);         // perform a task every Xs

// Constants for topics to observe
const SB_LIBRARY_MANAGER_READY_TOPIC = "songbird-library-manager-ready";
const SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC = "songbird-library-manager-before-shutdown";

// Constants for preferences
const PREF_METADATA_IMAGESCANNER_DEBUG = "songbird.metadataimagescanner.debug";
const PREF_METADATA_DEFAULT_COVER      = "songbird.metadataimagescanner.defaultCover";
const PREF_METADATA_RESCAN_INTERVAL    = "songbird.metadataimagescanner.rescan";
const PREF_METADATA_TIMER_INTERVAL     = "songbird.metadataimagescanner.interval";

/**
 * sbMetadataImageScanner
 * \brief A service that will scan through the main library searching the
 *        metadata for a cover in each media item.
 */
function sbMetadataImageScanner () {
  this._obsService = Cc["@mozilla.org/observer-service;1"]
                       .getService(Ci.nsIObserverService);

  // We want to wait until the library is ready before starting up.
  this._obsService.addObserver(this,
                               SB_LIBRARY_MANAGER_READY_TOPIC,
                               false);

  // We need to stop scanning before the library is shutdown.
  this._obsService.addObserver(this,
                               SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC,
                               false);
};
sbMetadataImageScanner.prototype = {
  // Component Settings
  className: "sbMetadataImageScanner",
  constructor: sbMetadataImageScanner,       // Constructor to this object
  classDescription: "Songbird Metadata Image Scanner",
  classID: Components.ID("{59a57af8-1b1a-48ad-b81d-42afcf45d4f7}"),
  contractID: "@songbirdnest.com/Songbird/Metadata/ImageScanner;1",
  _xpcom_categories: [{ // Make this a service
    category: "app-startup",
    service: true
  }],

  // Variables
  DEBUG : false,            // debug flag
  _timer: null,             // Timer for scanning the next item
  _mediaListView: null,     // sbIMediaListView we are scanning
  _itemViewIndex: 0,        // Index of next item in view we are to scan
  _batch: null,             // Helper for batch items.
  _mainLibrary: null,       // Hold it here since I have to QI it
  _restartScanning: false,  // Set to true to restart the scanning
  _rescanInterval: RESCAN_INTERVAL, // Time to pass for rescan of art
  _timerInterval: TIMER_INTERVAL,   // Time to pass before next item scan

  
  // Services
  _consoleService: null,    // For output of debug messages to error console
  _obsService: null,         // For observer service to start/stop properly
  _metadataManager: null,   // For getting metadata information from item
  _ioService: null,         // For converting URIs and such

  /**
   * Internal debugging functions
   */
  /**
   * \brief Dumps out a message if the DEBUG flag is enabled with
   *        the className pre-appended.
   * \param message String to print out.
   */
  _debug: function (message)
  {
    if (!this.DEBUG) return;
    try {
      dump(this.className + ": " + message + "\n");
      this._consoleService.logStringMessage(this.className + ": " + message);
    } catch (err) {
      // We do not want to throw an exception here
    }
  },
  
  /**
   * \brief Dumps out an error message with the className + ": [ERROR]"
   *        pre-appended, and will report the error so it will appear in the
   *        error console.
   * \param message String to print out.
   */
  _logError: function (message)
  {
    try {
      dump(this.className + ": [ERROR] - " + message + "\n");
      Cu.reportError(this.className + ": [ERROR] - " + message);
    } catch (err) {
      // We do not want to thow an exception here
    }
  },

  /*********************************
   * Start of metadata specific functions
   ********************************/
  /**
   * \brief Saves image data to a file.
   * \param aImageData - Binary array of image data
   * \param aImageDataSize - size of binary array
   * \param aMimeType - mime type of image (image/png, image/jpg, etc)
   * \return location of file as a fileURI
   */
  _saveDataToFile: function (aImageData, aImageDataSize, aMimeType) {
    // Generate a hash of the imageData for the filename
    var cHash = Cc["@mozilla.org/security/hash;1"]
                  .createInstance(Ci.nsICryptoHash);
    cHash.init(Ci.nsICryptoHash.MD5);
    cHash.update(aImageData, aImageDataSize);
    var hash = cHash.finish(false);
    var fileName = Array.prototype.map.call(hash,
                               function charToHex(aChar) {
                                var charCode = aChar.charCodeAt(0);
                                return ("0" + charCode.toString(16)).slice(-2);
                               }).join("");
    
    // grab the extension from the mimetype
    var mimeService = Cc["@mozilla.org/mime;1"]
                        .getService(Ci.nsIMIMEService);
    var mimeInfo = mimeService.getFromTypeAndExtension(aMimeType, "");
    if (!mimeInfo.getFileExtensions().hasMore()) {
      this._logError("Unable to get extension for image data.");
      return null;
    }
    ext = mimeInfo.primaryExtension;
    this._debug("Got extension of :" + ext);
    
    // Get the profile folder and append "artwork" as destination folder
    var dir = Cc["@mozilla.org/file/directory_service;1"]
                .createInstance(Ci.nsIProperties);
    var coverFile = dir.get("ProfLD", Ci.nsIFile);
    coverFile.append("artwork");
    // TODO: Check that we don't create other types of files like .exe
    coverFile.append(fileName + "." + ext);
  
    // Make sure we have this file or are able to create it.
    var outFilePath = this._ioService.newFileURI(coverFile);
    if (coverFile.exists()) {
      return outFilePath.spec;
    }
    
    // The file does not exist so create it
    try {
      coverFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0655);
    } catch (err) {
      this._logError("Unable to create file: " + err);
      return null;
    }
  
    // Save data to the file
    var outputFileStream =  Cc["@mozilla.org/network/file-output-stream;1"]
                              .createInstance(Ci.nsIFileOutputStream);
    outputFileStream.init(coverFile, -1, -1, 0);
    // Need to write out as binary data
    var binaryOutput = Cc["@mozilla.org/binaryoutputstream;1"]
                         .createInstance(Ci.nsIBinaryOutputStream);
    binaryOutput.setOutputStream(outputFileStream);
    binaryOutput.writeByteArray(aImageData, aImageDataSize);
    // Close the file so we are all clean.
    outputFileStream.close();
  
    return outFilePath.spec;
  },

  /**
   * \brief Searches for an image in the metadata of a file and if it finds one
   *        it saves it to a file and sets the primaryImageURL property to that
   *        file.
   * \param aMediaItem - Item to search for an image in
   * \return True if an image was found, False if the item has no image.
   */
  _findImageForItem: function (aMediaItem) {
    // First check if this is a valid local file.
    var fileURL = null;
    var contentURL = aMediaItem.getProperty(SBProperties.contentURL);
    try {
      var contentURI = this._ioService.newURI(contentURL, null, null);
      // if the URL isn't valid, fail
      if (!contentURI) {
        this._logError("Unable to find image for bad file: " + contentURL);
        return false;
      }

      // if the URL isn't local, fail
      if ( !(contentURI instanceof Ci.nsIFileURL) ) {
        this._logError("Unable to get image from non-local file: " +
                       contentURL);
        return false;
      }
    } catch (err) {
      this._logError("Unable to find image for: " + contentURL + " - " + err);
      return false;
    }
   
    this._debug("Getting handler for mediaItem: " + contentURI.spec);
    var handler = this._metadataManager.getHandlerForMediaURL(contentURI.spec);
    if (!handler) {
      this._debug("Unable to get handler for: " + contentURI.spec);
      return false;
    }

    try {
      this._debug("Reading metadata from item [FRONTCOVER].");
      var mimeTypeOutparam = {};
      var outSize = {};
      var imageData = handler.getImageData(Ci.sbIMetadataHandler
                                             .METADATA_IMAGE_TYPE_FRONTCOVER,
                                           mimeTypeOutparam,
                                           outSize);
      if (outSize.value <= 0) {
        this._debug("Reading metadata from item [OTHER].");
        // Try the OTHER cover
        imageData = handler.getImageData(Ci.sbIMetadataHandler
                                           .METADATA_IMAGE_TYPE_OTHER,
                                         mimeTypeOutparam,
                                         outSize);
      }
      
      if (outSize.value > 0) {
        this._debug("Found an image in the metadata.");
        var outFileLocation = this._saveDataToFile(imageData,
                                                   imageData.length,
                                                   mimeTypeOutparam.value);
        if (outFileLocation) {
          this._debug("Saved data to file: " + outFileLocation);
          aMediaItem.setProperty(SBProperties.primaryImageURL,
                                 outFileLocation);
          return true;
        } else {
          this._logError("Unable to save file: [" + outFileLocation + "]");
        }
      } else {
        this._debug("Unable to find metadata: [" + contentURI.spec + "]");
      }
    } catch (err) {
      this._debug("Unable to get image from metadata - for item at: " +
                  contentURI.spec + " - " + err );
    }
    
    return false;
  },
  
  /**
   * \brief Prepares to searche for an image in the metadata of a file and if it
   *        finds one it saves it to a file and sets the primaryImageURL
   *        property to that file.
   * \param aMediaItem - Item to search for an image in
   * \return True if an image was found, False if the item has no image.
   */
  _getCoverForItem: function (aMediaItem) {
    // Check if there is no current cover
    var primaryImageURL = aMediaItem.getProperty(SBProperties.primaryImageURL);
    this._debug("Next item has primaryImageURL of [" + primaryImageURL + "]");
    if (!primaryImageURL) {    

      // Check if we are ready to rescan this item
      var lastCoverScan = parseInt(aMediaItem.getProperty(PROP_LAST_COVER_SCAN));
      if (isNaN(lastCoverScan)) { lastCoverScan = 0; }
      var timeNow = Date.now();
      this._debug("Next item has lastCoverScan of [" + lastCoverScan + "] " +
                  "Now = " + timeNow);
      if ( (lastCoverScan + this._rescanInterval) < timeNow ) {

        // Check if we have artist and album
        var albumName = aMediaItem.getProperty(SBProperties.albumName);
        var artistName = aMediaItem.getProperty(SBProperties.artistName);
        this._debug("Next item has Artist=" + artistName +
                    ", Album=" + albumName);
        if ( artistName && albumName ) {
          aMediaItem.setProperty(PROP_LAST_COVER_SCAN, timeNow);
          return this._findImageForItem(aMediaItem);
        } else {
          this._debug("Missing Artist or Album from item");
        }
      } else {
        this._debug("Not ready to rescan this item for images.");
      }
    } else {
      // Since we are sorting by primaryImageURL (artwork) if we encounter an
      // item with artwork then we are done.
      this._debug("This item already has album art, this means we are done.");
      this._turnOffTimer();
    }
    
    return false;
  },
  /*********************************
   * End of metadata specific functions
   ********************************/

  /**
   * \brief Grabs the next item from the view to scan.
   */
  _getNextItem: function () {
    // Don't do anything if there is a batch running.
    if (this._batch.isActive()) {
      this._debug("Scanning is currently paused due to a batch running");
      return;
    }
    
    // if there are no items left to scan, then we are done.
    if (this._itemViewIndex >= this._mediaListView.length) {
      this._debug("All done searching");
      this._turnOffTimer();
      return;
    }

    // Since we are sorting by the primaryImageURL (artwork) the items will be
    // reordered every time an items pirmaryImageURL property changes. So here
    // we take the next top item that we have not already scanned.
    var nextItem = this._mediaListView.getItemByIndex(this._itemViewIndex);
    if (nextItem) {
      this._debug("Getting next items cover");
      if (!this._getCoverForItem(nextItem)) {
        // Only increment the index if the item does not have an image
        this._itemViewIndex++;
      }
    }
  },

  /**
   * \brief Starts a query for all items in the main library that do not have
   *        the primaryImageURL set.
   */
  _getItems: function () {
    this._mediaListView = this._mainLibrary.createView();
    // Due to Bug 8489 we can not filter on null or empty values.
    // Currently we are sorting by primaryImageURL so the empty ones are at
    // The top.
    var propArray =
        Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
          .createInstance(Ci.sbIMutablePropertyArray);
    propArray.appendProperty(SBProperties.primaryImageURL, "a");
    this._mediaListView.setSort(propArray);

    // Remove the hidden or list items
    var filter = LibraryUtils.createConstraint([
      [
        [SBProperties.isList, ["0"]]
      ],
      [
        [SBProperties.hidden, ["0"]]
      ]
    ]);

    this._mediaListView.filterConstraint = filter;
    this._debug("Scanning " + this._mediaListView.length + " items.");

    this._itemViewIndex = 0;
    this._turnOnTimer();
  },
  
  /**
   * \brief Turns off the timer and cleans up related stuff
   */
  _turnOffTimer: function () {
    if (this._timer) {
      this._debug("Turning off timer");
      this._timer.cancel();
      this._timer = null;
      this._mediaListView = null;
      this._itemViewIndex = 0;
    }
  },
  
  /**
   * \brief Turns on the timer, if the timer is currently on then it stops it
   *        first
   */
  _turnOnTimer: function () {
    if (this._timer) {
      this._timer.cancel();
    } else {
      this._timer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
    }
    this._timer.initWithCallback(this,
                                 this._timerInterval, 
                                 Ci.nsITimer.TYPE_REPEATING_SLACK);
  },
  
  /**
   * \brief Starts up the scanning of the main library for image in metadata.
   */
  _startup: function () {
    // Check if we need debugging stuff
    this.DEBUG = Application.prefs.getValue(PREF_METADATA_IMAGESCANNER_DEBUG,
                                            false);
    if (this.DEBUG) {
      this._consoleService = Cc["@mozilla.org/consoleservice;1"]
                              .getService(Ci.nsIConsoleService);
    }
    this._debug("Starting metadata image scanner");

    // Get some settings from the preferences
    this._rescanInterval = Application.prefs.getValue(
                                                  PREF_METADATA_RESCAN_INTERVAL,
                                                  RESCAN_INTERVAL);
    this._timerInterval = Application.prefs.getValue(
                                                  PREF_METADATA_TIMER_INTERVAL,
                                                  TIMER_INTERVAL);

    // Load some services and such
    this._metadataManager = Cc["@songbirdnest.com/Songbird/MetadataManager;1"]
                              .getService(Ci.sbIMetadataManager);
    this._ioService = Cc['@mozilla.org/network/io-service;1']
                        .getService(Ci.nsIIOService);
      
    // Store this since we have to QI it.
    this._mainLibrary = LibraryUtils.mainLibrary
                          .QueryInterface(Ci.sbILocalDatabaseLibrary);

    try {
      // Setup a listener to the main library for batches
      this._batch = new LibraryUtils.BatchHelper();
      this._mainLibrary.addListener(this,
                                    true,
                                    Ci.sbIMediaList.LISTENER_FLAGS_BATCHBEGIN |
                                      Ci.sbIMediaList.LISTENER_FLAGS_BATCHEND,
                                    null);
  
    } catch (err) {
      this._logError("Unable to start scanner: " + err);
      return;
    }

    // Start the search
    this._getItems();
  },
  
  /**
   * \brief Shutdown the scanning.
   */
  _shutdown: function () {
    this._debug("Shutting down");
    this._turnOffTimer();
  },
  
  /*********************************
   * nsITimerCallback
   ********************************/
  notify: function (aTimer) {
    // Scan the next item for images
    this._debug("Scanning for next item " + this._itemViewIndex +
                " of " + this._mediaListView.length);
    this._getNextItem();
  },

  /*********************************
   * sbIMediaListListener (requires nsISupportsWeakReference)
   ********************************/
  onBatchBegin: function (aMediaList) {
    this._debug("Batch Begin Called");
    this._batch.begin();
    this._turnOffTimer();
  },
  
  onBatchEnd: function onBatchEnd(aMediaList) {
    this._debug("Batch End Called");
    this._batch.end();
    // If the batch has finished we need to restart the scanner in case any
    // Items have been added or removed.
    if (!this._batch.isActive()) {
      this._debug("Batch is no longer running so restart scanning");
      this._getItems();
    }
  },

  /*********************************
   * nsIObserver
   ********************************/
  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case SB_LIBRARY_MANAGER_READY_TOPIC:
        this._obsService.removeObserver(this,
                                       SB_LIBRARY_MANAGER_READY_TOPIC);
        this._startup();
        break;
      case SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC:
        this._obsService.removeObserver(this,
                                       SB_LIBRARY_MANAGER_BEFORE_SHUTDOWN_TOPIC);
        this._shutdown();
        break;
    }
  },

  /*********************************
   * nsISupports
   ********************************/
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaListListener,
                                         Ci.nsISupportsWeakReference,
                                         Ci.nsITimerCallback])
}


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbMetadataImageScanner]);
}
