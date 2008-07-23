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

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

Cu.import('resource://app/jsmodules/sbProperties.jsm');
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

/**
 * sbLibraryScanner Component
 */
function sbLibraryScanner() {
  this.PROP_LAST_COVER_SCAN = SBProperties.base + 'lastCoverScan';
  this.TIMER_INTERVAL = (10 * 1000);              // perform a task every Xs
  this.LONG_TIMER_INTERVAL = (5 * 60 * 1000);     // long interval at X minutes
  this.DEBUG = false;                   // debug flag default to false
  
  this._shutdown = false;               // Flag to shutdown the scanner
  this._scanComplete = true;            // for isScanComplete attribute
  this._currentItem = null;             // Current item we are scanning
  this._listener = null;                // CoverListener
  this._array = null;                   // Array of albums we are scanning
  this._length = 0;                     // Number of items found in the db
  this._curIndex = 0;                   // Current index in the array to scan
  this._doRestart = false;              // Flag to restart the scanner
  this._currTimeScan = 0;               // What interval are we waiting with?

  // set up a timer to do work on
  this._timer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
  this._timerRunning = false;           // is the timer running?

  // See if we are in debug mode
  try {
    this._consoleService = Cc["@mozilla.org/consoleservice;1"]
                            .getService(Ci.nsIConsoleService);
    var prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);
    this.DEBUG = prefService.getBoolPref("songbird.albumartmanager.debug");
  } catch (err) {}
  
  try {
    var albumArtService = Cc['@songbirdnest.com/songbird-album-art-service;1']
                              .getService(Ci.sbIAlbumArtService);
    this._defaultCover = albumArtService.defaultCover;
  } catch (err) {
    this._defaultCover = null;
  }
  
  this._ioService =  Cc['@mozilla.org/network/io-service;1']
                       .getService(Ci.nsIIOService);

  this._playListPlaybackService = 
                           Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                           .getService(Ci.sbIPlaylistPlayback);

  // Setup a listener to the main library for added/removed items
  LibraryUtils.mainLibrary.addListener(this,
                                       true,
                                       Ci.sbIMediaList.LISTENER_FLAGS_ITEMADDED |
                                       Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED,
                                       null);

  this._debug("Initialized");
}
sbLibraryScanner.prototype = {
  className: 'sbLibraryScanner',
  classDescription: 'Songbird Library Scanner',
  classID: Components.ID('{6e29093f-5bd0-4006-9016-57a5581d9979}'),
  contractID: '@songbirdnest.com/Songbird/cover/library-scanner;1'
}

// nsISupports
sbLibraryScanner.prototype.QueryInterface =
XPCOMUtils.generateQI([Ci.sbILibraryScanner,
                       Ci.nsITimerCallback,
                       Ci.nsISupportsWeakReference,
                       Ci.sbICoverListener,
                       Ci.sbIMediaListListener,
                       Ci.sbILocalDatabaseAsyncGUIDArrayListener]);

// Internal
/**
 * \brief Dumps out a message if the DEBUG flag is enabled with
 *        the className pre-appended.
 * \param message String to print out.
 */
sbLibraryScanner.prototype._debug =
function sbLibraryScanner__debug(message) {
  if (!this.DEBUG) return;
  dump(this.className + ": " + message + "\n");
  this._consoleService.logStringMessage(this.className + ": " + message);
}

/**
 * \brief Dumps out an error message with "the className and "[ERROR]"
 *        pre-appended, and will report the error so it will appear in the
 *        error console.
 * \param message String to print out.
 */
sbLibraryScanner.prototype._logError =
function sbLibraryScanner__logError(message) {
  dump(this.className + ": [ERROR] - " + message + "\n");
  Cu.reportError(this.className + ": [ERROR] - " + message);
}

sbLibraryScanner.prototype._getAlbums =
function sbLibraryScanner__getAlbums() {
  // NOTE this is not really the way you SHOULD do this, but I am because
  // it is so much faster :).
  this._array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/AsyncGUIDArray;1"]
                 .createInstance(Ci.sbILocalDatabaseAsyncGUIDArray);
  this._array.databaseGUID = this._library.databaseGuid;
  this._array.propertyCache = this._library.propertyCache;
  this._array.baseTable = 'media_items';
  this._array.isDistinct = true;
  this._array.addSort(SBProperties.albumName, true);
  this._array.addAsyncListener(this);
  this._array.invalidate();
  this._array.getLengthAsync();
}

sbLibraryScanner.prototype._setTimer =
function sbLibraryScanner__setTimer(aInterval) {
  this._stopTimer();
  this._currTimeScan = aInterval;
  this._timer.initWithCallback(this,
                               aInterval, 
                               Ci.nsITimer.TYPE_REPEATING_SLACK);
}

sbLibraryScanner.prototype._stopTimer =
function sbLibraryScanner__stopTimer() {
  if (this._timer) {
    this._timer.cancel();
    this._timerRunning = false;
  }
}

sbLibraryScanner.prototype._getCoversForAlbum =
function sbLibraryScanner__getCoversForAlbum(aMediaItem) {
  var self = this;
  var enumListener = {
    onEnumerationBegin: function() {
      return Ci.sbIMediaListEnumerationListener.CONTINUE;
    },
    onEnumeratedItem: function(list, item) {
      var oldArtLocation = item.getProperty(SBProperties.primaryImageURL);
      if (!oldArtLocation || oldArtLocation == "") {
        var coverScanner = Cc["@songbirdnest.com/Songbird/cover/cover-scanner;1"]
                             .createInstance(Ci.sbICoverScanner);
        coverScanner.fetchCover(item, self);
      }
      return Ci.sbIMediaListEnumerationListener.CONTINUE;
    },
    onEnumerationEnd: function() {
    }
  };

  var mArtist = aMediaItem.getProperty(SBProperties.artistName);
  var mAlbum = aMediaItem.getProperty(SBProperties.albumName);
  var propertyArray =
    Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
    .createInstance(Ci.sbIMutablePropertyArray);
  propertyArray.appendProperty(SBProperties.albumName, mAlbum);
  propertyArray.appendProperty(SBProperties.artistName, mArtist);

  var mediaList = null;
  if (this._playListPlaybackService.currentGUID == aMediaItem.guid) {
    mediaList = this._playListPlaybackService.playingView.mediaList;
  } else {
    mediaList = LibraryUtils.mainLibrary;
  }
  mediaList.enumerateItemsByProperties(propertyArray,
                                       enumListener,
                                       Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
}

// sbILocalDatabaseAsyncGUIArrayListener
sbLibraryScanner.prototype.onGetGuidByIndex =
function sbLibraryScanner_onGetGuidByIndex(aIndex, aGuid, aResult) {
  if (aGuid) {
    this._debug("Getting library item for guid[" + aIndex + "]: " + aGuid);
    var item = null;
    try {
      item = this._library.getMediaItem(aGuid);
    } catch (err) {
      this._debug("Item: " + aGuid + " is no longer in the main library");
      return;
    }

    var album = item.getProperty(SBProperties.albumName);
    var artist = item.getProperty(SBProperties.artistName);
    this._debug("Next item has Artist=" + artist + ", Album " + album);
    var art = item.getProperty(SBProperties.primaryImageURL);
    this._debug("Next item has Art of [" + art + "]");

    if ( !art || art == this._defaultCover ) {
      if (artist && album) {
        this._currentItem = item;
        this._getCoversForAlbum(item);
      }
    } else {
      this._debug("This item already has album art!");
    }
  }
}

sbLibraryScanner.prototype.onGetLength =
function sbLibraryScanner_onGetLength(aLength, aResult) {
  this._debug("onGetLength - length = " + aLength);
  this._length = aLength;
  this._curIndex = 0;
}

sbLibraryScanner.prototype.onGetSortPropertyValueByIndex =
function sbLibraryScanner_(onGetSortPropertyValueByIndexaIndex, aPropertySortValue, aResult) {
  // Not used
}

sbLibraryScanner.prototype.onGetMediaItemIdByIndex =
function sbLibraryScanner_onGetMediaItemIdByIndex(aIndex, aMediaItemId, aResult) {
  // Not used
}

sbLibraryScanner.prototype.onStateChange =
function sbLibraryScanner_onStateChange(aState) {
  // Ignoring
}

// sbILibraryScanner
sbLibraryScanner.prototype.__defineGetter__("isScanComplete",
function sbLibraryScanner_isScanComplete() {
  return this._scanComplete;
});

sbLibraryScanner.prototype.__defineGetter__("currentItemScanning",
function sbLibraryScanner_currentItemScanning() {
  return this._currentItem;
});

sbLibraryScanner.prototype.restartScanner =
function sbLibraryScanner_restartScanner() {
  this._debug("Restarting scanner.");
  this._doRestart = true;
  if (this._currTimeScan == this.LONG_TIMER_INTERVAL) {
    this._debug("Reseting timer to shorter interval.");
    this._setTimer(this.TIMER_INTERVAL);
  }
}

sbLibraryScanner.prototype.scanLibrary =
function sbLibraryScanner_scanLibrary(aLibrary, aListener) {
  if (this._shutdown) {
    this._logError("Unable to start a new scan when we are shutting down!");
    return;
  }

  if (!this._scanComplete) {
    // We are already scanning! only one at a time!
    this._logError("Unable to start a new scan when the previous one is not finished!");
    return;
  }

  // Indicate that we are scanning
  this._scanComplete = false;
  this._shutdown = false;
  this._length = -1;
  this._doRestart = false;

  // Store this for future use
  this._library = aLibrary.QueryInterface(Ci.sbILocalDatabaseLibrary);
  this._listener = aListener;

  // if the timer isn't running, start it
  if (!this._timerRunning) {
    // every second or so...
    this._setTimer(this.TIMER_INTERVAL);
  }

  // Start the search
  this._debug("Starting search for albums");
  this._getAlbums();
}

sbLibraryScanner.prototype.shutdown =
function sbLibraryScanner_shutdown() {
  this._stopTimer();
  this._currentItem = null;
  this._shutdown = true;
  this._scanComplete = true;
  this._listener = null;
  this._array = null;
  this._length = 0;
  this._library = null;
  this._listener = null;
}

// nsITimerCallback
sbLibraryScanner.prototype.notify = 
function sbLibraryScanner_notify(aTimer) {
  if (this._shutdown) {
    this._debug("We are shutting down.");
    return;
  }

  // Restart our scanner if we are requested to, this could be triggered if new
  // items have been added.
  if (this._doRestart) {
    this._debug("Restarting scanner");
    this._doRestart = false;
    this._setTimer(this.TIMER_INTERVAL);
    this._getAlbums();
    return;
  }

  if (this._length == -1) {
    // we're waiting for the length of the result set
    // so do nothing right now
    return;
  }

  // if there are no items to scan, then we wait a bit longer and restart
  if (this._curIndex >= this._length) {
    // Reset the timer to the long interval
    this._debug("There are no items to scan, waiting a bit longer to restart");
    this._setTimer(this.LONG_TIMER_INTERVAL);
    return;
  }

  this._array.getGuidByIndexAsync(this._curIndex);
  this._curIndex++;
}

// sbICoverListener
sbLibraryScanner.prototype.coverFetchSucceeded =
function sbLibraryScanner_coverFetchSucceeded(aMediaItem, aScope, aURI) {
  if (aMediaItem == this._currentItem) {
    this._currentItem = null;
  }
  this._listener.coverFetchSucceeded(aMediaItem, aScope, aURI);
}

sbLibraryScanner.prototype.coverFetchFailed =
function  sbLibraryScanner_coverFetchFailed(aMediaItem, aScope) {
  if (aMediaItem == this._currentItem) {
    this._currentItem = null;
  }
  this._listener.coverFetchFailed(aMediaItem, aScope);
}

// sbIMediaListListener (We only listen to onItemAdded and onAfterItemRemoved
// calls)
sbLibraryScanner.prototype.onItemAdded =
function  sbLibraryScanner_onItemAdded(aMediaList, aMediaItem, aIndex) {
  this._debug("New item added so restarting scanner.");
  this.restartScanner();
  return true;
}

sbLibraryScanner.prototype.onAfterItemRemoved =
function  sbLibraryScanner_onAfterItemRemoved(aMediaList, aMediaItem, aIndex) {
  this._debug("New item added so restarting scanner.");
  this.restartScanner();
  return true;
}


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbLibraryScanner]);
}
