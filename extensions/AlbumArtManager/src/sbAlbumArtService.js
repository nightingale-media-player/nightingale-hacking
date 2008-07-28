/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
//
// The Initial Developer of the Original Code is
// Steven Bengtson.
// Portions created by the Initial Developer are Copyright (C) 2008
// the Initial Developer. All Rights Reserved.
//
// Contributor(s):
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

// Constants for convience
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

const DEFAULT_COVER = "chrome://albumartmanager/skin/no-cover.png";
const STRING_BUNDLE = "chrome://albumartmanager/locale/albumartmanager.properties";
const DISPLAY_PANE  = "chrome://albumartmanager/content/xul/servicePane.xul";

Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

/**
 * \brief AlbumArtService is the main service that maintains album art
 * for the albums in the library, it also handles notification of currently
 * playing item and notifications on changes to album art.
 */
function sbAlbumArtService () {
  this._osService = Cc["@mozilla.org/observer-service;1"]
                    .getService(Ci.nsIObserverService);

  this.DEBUG = false;
  try {
    this._consoleService = Cc["@mozilla.org/consoleservice;1"]
                            .getService(Ci.nsIConsoleService);
    this._prefService = Cc['@mozilla.org/preferences-service;1']
                          .getService(Ci.nsIPrefBranch);
    this.DEBUG = prefService.getBoolPref("songbird.albumartmanager.debug");
  } catch (err) {}

  // We want to wait until profile-after-change to initialize
  this._osService.addObserver(this, 'profile-after-change', false);

  // We need to unhook things on shutdown
  this._osService.addObserver(this, "quit-application", false);
};

sbAlbumArtService.prototype = {
  className: "sbAlbumArtService",
  constructor: sbAlbumArtService,       // Constructor to this object
  classDescription: "Songbird Album Art Service",
  classID: Components.ID("{68b3c6f2-b5c2-49c5-a0dc-cf224bd27adf}"),
  contractID: "@songbirdnest.com/songbird-album-art-service;1",

  // Variables
  DEBUG: false,                       // Debug flag for _debug function
  _fetchers: [],                      // List of available fetchers
  
  // Services
  _osService: null,                   // Observer Service
  _prefService: null,                 // Perferences Service
  _ioService: null,                   // IO Service
  _alertService: null,                // Alert notification Service
  _playListPlaybackService: null,     // PlaylistPlayback Service
  _bundle: null,                      // String bundle
  _paneMgr: null,                     // Display pane manager
  
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
      // We do not want to throw exception here
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
      // We do not want to thow exceptions here
    }
  },

  /**
   * Internal functions
   */
  /**
   * \brief Initalize the AlbumArtService, grab the services we need.
   */
  _init: function ()
  {
    this._debug("Init called");
    
    this._debug("Loading preferences");
    var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                               .getService(Ci.nsIStringBundleService);
    this._bundle = stringBundleService.createBundle(STRING_BUNDLE);

    this._debug("Loading playListListener");
    this._playListPlaybackService = 
                             Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                             .getService(Ci.sbIPlaylistPlayback);
    this._playListPlaybackService.addListener(this);

    this._debug("Loading alert service");
    this._alertService = Cc["@mozilla.org/alerts-service;1"]
                          .getService(Ci.nsIAlertsService);

    this._debug("Creating dataremote for now playing cover");
    // Create a dataremote for the current album cover art
    this._currentArtDataRemote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                                   .createInstance(Ci.sbIDataRemote);
    this._currentArtDataRemote.init("albumartmanager.currentCoverUrl", null);
    this._currentArtDataRemote.stringValue = DEFAULT_COVER;

    this._debug("Loading ioService");
    // Used for converting string urls to nsIURIs
    this._ioService =  Cc['@mozilla.org/network/io-service;1']
                         .getService(Ci.nsIIOService);

    this._debug("Registering now playing service display pane");
    // Add a pane to the service pane for current album art
    this._paneMgr = Cc["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
                   .getService(Ci.sbIDisplayPaneManager);
    var panelLabel = SBString("albumartmanager.panel.label",
                              null,
                              this._bundle);
    this._paneMgr.registerContent(DISPLAY_PANE, 
                                  panelLabel,      // Label of panel
                                  "",              // TODO: Icon of panel
                                  175,             // Random width
                                  192,             // Random height
                                  "servicepane",   // which pane we are attaching to
                                  true);           // Auto show panel
    this._paneMgr.showPane(DISPLAY_PANE);

    this.loadFetcherList();

    if (this._prefService.getBoolPref("songbird.albumartmanager.scan")) {
      this.toggleScanner(true);
    }
  },

  /**
   * \brief Shutdown the AlbumArtService
   */
  _deinit: function ()
  {
    this._debug("Shutdown called");
    if (this._libraryScanner) {
      this._libraryScanner.shutdown();
    }
    this._playListPlaybackService.removeListener(this);
    this._paneMgr.unregisterContent(DISPLAY_PANE);
  },

  _addFetcher: function(aContractID) {
    try {
      this._debug("Trying to add Fetcher " + aContractID);
      var fetcherService = Cc[aContractID].getService(Ci.sbICoverFetcher);
      // Get the priority of this one and then put it in the proper spot in the
      // array
      var curPriority = 99;
      try {
        curPriority = this._prefService.getIntPref("songbird.albumartmanager.fetcher." +
                                                   fetcherService.shortName +
                                                   ".priority");
      } catch (err) {}
      
      var fIndex = 0;
      for (fIndex = 0; fIndex < this._fetchers.length; fIndex++) {
        var fetcherCid = this._fetchers
                     .queryElementAt(fIndex, Ci.nsISupportsString)
                     .toString();
        var nFetcherService = Cc[fetcherCid].getService(Ci.sbICoverFetcher);

        var fPriority = 99;
        try {
          fPriority = this._prefService.getIntPref("songbird.albumartmanager.fetcher." +
                                                   nFetcherService.shortName +
                                                   ".priority");
        } catch (err) {}

        if (fPriority > curPriority) {
          break; // jump out so we can insert
        }
      }

      var cidString =  Cc["@mozilla.org/supports-string;1"]
                         .createInstance(Ci.nsISupportsString);
      cidString.data = aContractID;
      if (fIndex < this._fetchers.length) {
        this._debug("Inserting fetcher [" + aContractID + "] as position " + fIndex);
        this._fetchers.insertElementAt(cidString, fIndex, false);
      } else {
        this._debug("Appending fetcher [" + aContractID + "]");
        // Didn't find a place for it so just append
        this._fetchers.appendElement(cidString, false);
      }
    } catch (err) {
      // Bad service just ignore
      this._logError("Unable to add Fetcher: " + aContractID);
    }
  },

  /**
   * \brief Helper function for getting a property from a media item with a
   *        default value if none found.
   * \param aMediaItem item to get property from
   * \param aPropertyName property to get from item
   * \param aDefaultValue default value to use if property had no value
   * \return property value or default value passed in if not available
   */
  _getProperty: function(aMediaItem, aPropertyName, aDefaultValue) {
    var propertyValue = aMediaItem.getProperty(aPropertyName);
    if ( (propertyValue == null) ||
         (propertyValue == "") ) {
      return aDefaultValue;
    } else {
      return propertyValue;
    }
  },
 
  /**
   * \brief Display notification for track change, OS X will use growl if
   *        available, Windows and Linux will use a xul popup, and change the
   *        dataremote for the current album art.
   * \param aMediaItem Media item to use for notification.
   */
  _notify: function(aMediaItem)
  {
    this._debug("Notify called.");

    if (!aMediaItem) {
      this._currentArtDataRemote.stringValue = DEFAULT_COVER;
      return;
    }

    var mAlbumArt = this.getAlbumArtWork(aMediaItem, true);

    if (this._playListPlaybackService.currentGUID == aMediaItem.guid) {
      // Only update the dataRemote if this is the current track playing
      this._debug("Setting dataremote from notify to " + mAlbumArt);
      this._currentArtDataRemote.stringValue = mAlbumArt;
    }

    if ( !this._playListPlaybackService.playing ||
         this._playListPlaybackService.paused ||
         (this._playListPlaybackService.currentGUID != aMediaItem.guid) ) {
      // Do not show notification if the track is not playing or is paused or
      // the current track playing is not aMediaItem
      this._debug("Item is not playing so not showing notification.");
      return;
    }

    // Check if the user want to show notifications
    try {
      if (!this._prefService
            .getBoolPref("songbird.albumartmanager.display_track_notification")) {
        return;
      }
    } catch (err) {}

    var mTitle = this._getProperty(aMediaItem,
                                   SBProperties.trackName,
                                   "Unknown");
    var mArtist = this._getProperty(aMediaItem,
                                    SBProperties.artistName,
                                    "Unknown");
    var mAlbum = this._getProperty(aMediaItem,
                                   SBProperties.albumName,
                                   "Unknown");

    var textTitle = mTitle;
    var textOutput = mArtist + "\n" + mAlbum; // This only works with my changes
                                              // to alert.js and alert.xul

    // We put a try here because on OS X if there is no Growl installed then
    // it will throw an exception (OS X doesn't work well with the xul popup)
    // TODO: Make a libnotify version of the alertService for Linux?
    // TODO: Make a Snarl version of the alertService for Windows?
    try {
      this._alertService.showAlertNotification(mAlbumArt,
                                               textTitle,
                                               textOutput);
    } catch (err) {
      var alertError = SBString("albumartmanager.alertError",
                              "Unable to alert check settings.",
                              this._bundle);
      this._logError(alertError);
    }
  },

  /**
   * \brief If a new track has started playing then we need to first see if
   *        the item has a cover, if not then serch for it. If it does have a
   *        cover then display the notification.
   * \param aMediaItem currently playing item.
   */
  _trackChanged: function (aMediaItem)
  {
    this._debug("Track Changed called.");
    
    if (aMediaItem == null) {
      return;
    }

    // Change the dataremote so that displays are updated (this may change
    // again if we need to search for covers, see _notify)
    var mAlbumArt = this.getAlbumArtWork(aMediaItem, true);
    this._debug("Got album art of [" + mAlbumArt + "] on trackChanged");
    this._currentArtDataRemote.stringValue = mAlbumArt;
    
    // Fire off the scanners to find album art if needed
    if (mAlbumArt == DEFAULT_COVER) {
      this._debug("Starting search of covers for guid: " + aMediaItem.guid);
      var coverScanner = Cc["@songbirdnest.com/Songbird/cover/cover-scanner;1"]
                           .createInstance(Ci.sbICoverScanner);
      coverScanner.fetchCover(aMediaItem, this);
    } else {
      this._notify(aMediaItem);
    }
  },

  /**
   * \brief Fixes filenames for some of the file systems (Fat, NTFS, ext, hpfs)
   * \param aFileName the string version of the filename to fix
   * \return a string of the fixed filename
   */
  _makeFileNameSafe: function(aFileName) {
    var newFileName;
    
    // Replace all known bad characters (mostly from windows filesystems)
    newFileName = aFileName.replace(/[*|\"|\/|\\|\||:|\?|<|>]/g, "_");
    
    // We also do not allow the filename to start with a .
    // TODO: Change this to check each portion of the filename so we don't get
    //       a value like /home/joe/Music/Covers/.test.jpg
    if (newFileName.indexOf(".") == 0) {
      newFileName = newFileName.slice(1);
    }
    
    // Return what we have
    return newFileName;
  },


  /*********************************
   * sbIAlbumArtService
   ********************************/
  get defaultCover() {
    return DEFAULT_COVER;
  },

  /**
   * \brief Determines the download location based on preferences.
   *
   * \param aMediaItem item to generate download url for
   * \param aExtension optional extension to add on to the file
   * \return a nsIFile of the location to download to minus an extension if
   *         aExtension is null, otherwise full location.
   */
  getCoverDownloadLocation: function(aMediaItem, aExtension)
  {
    var albumName = this._getProperty(aMediaItem, SBProperties.albumName, "");
    var albumArtist = this._getProperty(aMediaItem, SBProperties.artistName, "");
    var trackName = this._getProperty(aMediaItem, SBProperties.trackName, "");
    var contentURL = aMediaItem.getProperty(SBProperties.contentURL);
    var itemGuid = aMediaItem.guid;
    
    var toAlbumLocation = true;
    try {
      toAlbumLocation = this._prefService
              .getBoolPref("songbird.albumartmanager.cover.usealbumlocation");
    } catch (err) {
      this._prefService
        .setBoolPref("songbird.albumartmanager.cover.usealbumlocation", true);
    }

    var downloadLocation = null;
    if (toAlbumLocation) {
      var uri;
      try {
        var fph = this._ioService.getProtocolHandler("file")
                      .QueryInterface(Ci.nsIFileProtocolHandler);
        var file = fph.getFileFromURLSpec(contentURL);
        if (file.parent) {
          downloadLocation = file.parent;
        }
      } catch (err) {
        this._logError("Error getting content location: " + err);
        downloadLocation = null;
      }
    } else {
      try {
        var otherLocation = this._prefService.getComplexValue(
                                        "songbird.albumartmanager.cover.folder",
                                        Ci.nsILocalFile);
        if (otherLocation) {
          downloadLocation = otherLocation.QueryInterface(Ci.nsIFile);
        }
      } catch (err) {
        this._logError("Unable to get download location: " + err);
        downloadLocation = null;
      }
    }
    
    if (downloadLocation != null) {
      var coverFormat = this._prefService.getCharPref(
                                    "songbird.albumartmanager.cover.format");
      if (coverFormat == "" || coverFormat == null) {
        coverFormat = "%artist% - %album%";
      }
      
      coverFormat = coverFormat.replace(/%album%/gi, this._makeFileNameSafe(albumName));
      coverFormat = coverFormat.replace(/%artist%/gi, this._makeFileNameSafe(albumArtist));
      coverFormat = coverFormat.replace(/%track%/gi, this._makeFileNameSafe(trackName));
      coverFormat = coverFormat.replace(/%guid%/gi, this._makeFileNameSafe(itemGuid));
  
      if (aExtension) {
        coverFormat = coverFormat + aExtension;
      }
  
      // Have to append each folder if the user is using \ or / in the format
      // we can't seem to append something like "folder/folder/folder" :P
      try {
        if ( (coverFormat.indexOf("/") > 0) ||
             (coverFormat.indexOf("\\") > 0) ) {
          this._debug("Splitting " + coverFormat);
          var sections = coverFormat.split("/");
          if (sections.length == 1) {
            this._debug("/ didn't work so trying \\");
            sections = coverFormat.split("\\");
          }
          this._debug("Split into " + sections.length + " sections");
          for (var i = 0; i < sections.length; i++) {
            this._debug("Appending " + sections[i]);
            sections[i] = this._makeFileNameSafe(sections[i]);
            downloadLocation.append(sections[i]);
          }
        } else {
          this._debug("No need to split " + coverFormat);
          coverFormat = this._makeFileNameSafe(coverFormat);
          downloadLocation.append(coverFormat);
        }
      } catch (err) {
        this._logError("Unable to create downloadLocation with " + coverFormat);
        downloadLocation = null;
      }
    }

    return downloadLocation;
  },

  /**
   * \brief Tries to get the album art for a mediaItem.
   * \param aMediaItem Media item to use
   * \param aCheckFileExists for file: schemas check if it exists first.
   * \return string URI of album art or DEFAULT_COVER if not found.
   */
  getAlbumArtWork: function(aMediaItem, aCheckFileExists) {
    if (!aMediaItem) {
      this._logError("no valid media item passed to getAlbumArtWork");
      return DEFAULT_COVER;
    }
    
    var mAlbumArt = this._getProperty(aMediaItem,
                                      SBProperties.primaryImageURL,
                                      DEFAULT_COVER);
    if (!mAlbumArt || mAlbumArt == "") {
      return DEFAULT_COVER;
    }

    try {
      uri = this._ioService.newURI(mAlbumArt, null, null);
    } catch (err) {
      this._logError("Unable to convert " + mAlbumArt + " to URI - " + err);
      return mAlbumArt;
    }
    
    var foundCover = DEFAULT_COVER;
    switch (uri.scheme) {
      case 'file':
        if (aCheckFileExists) {
          // now check that it exists
          try {
            var fph = this._ioService.getProtocolHandler("file")
                          .QueryInterface(Ci.nsIFileProtocolHandler);
            var file = fph.getFileFromURLSpec(mAlbumArt);
            this._debug("Check that file [" + file.path + "] exists");
            if(file.exists()) {
              foundCover = mAlbumArt;
            }
          } catch (err) {
            this._logError("Error with file.initWithPath on " + fullPath + " - " + err);
            // Bad file
          }
        } else {
          foundCover = mAlbumArt;
        }
      break;
    
      case 'http':
      case 'https':
      case 'data':
        foundCover = mAlbumArt;
      break;
    }

    // Return the default for now until we find one
    return foundCover;
  },
  
  /**
   * \brief Update the album cover for all tracks in an album
   * \param aAlbumTitle Title of Album to update
   * \param aNewCover String URI of new cover
   */
  updateAlbumCover: function(aAlbumTitle, aNewCover) {
    var self = this;
    var updateListener = {
      onEnumerationBegin: function() {
      self._debug("updateAlbumArt enumeration begins...")
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumeratedItem: function(list, item) {
        var oldArtLocation = item.getProperty(SBProperties.primaryImageURL);
        if (oldArtLocation != aNewCover) {
          self._debug("Setting item cover to [" + aNewCover + "]");
          item.setProperty(SBProperties.primaryImageURL, aNewCover);
          if (self._playListPlaybackService.currentGUID == item.guid) {
            self._notify(item);
          }
        }
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumerationEnd: function() {
        self._debug("Done setting albumart, notifying of album change.");
      }
    };

    var propertyArray =
      Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
      .createInstance(Ci.sbIMutablePropertyArray);
    propertyArray.appendProperty(SBProperties.albumName, aAlbumTitle);
    //propertyArray.appendProperty(SBProperties.artistName, mArtist);

/*    var mediaList = null;
    if (this._playListPlaybackService.currentGUID == aMediaItem.guid) {
      mediaList = this._playListPlaybackService.playingView.mediaList;
    } else {*/
      mediaList = LibraryUtils.mainLibrary;
    //}
    mediaList.enumerateItemsByProperties(propertyArray, updateListener,
                                      Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
  },

  /**
   * \brief creats a list of fetchers and orders them based on priority
   */
  loadFetcherList: function() {
    this._debug("Creating list of fetcher components");
    var catManager = Cc['@mozilla.org/categorymanager;1']
                      .getService(Ci.nsICategoryManager);
    var catEnumerator = catManager.enumerateCategory('sbCoverFetcher');
    this._fetchers = Cc["@mozilla.org/array;1"]
                      .createInstance(Ci.nsIMutableArray);
    while (catEnumerator.hasMoreElements()) {
      try {
        var entry =catEnumerator
                    .getNext()
                    .QueryInterface(Ci.nsISupportsCString);
        var contractID = catManager.getCategoryEntry('sbCoverFetcher', entry);
        // Now we need to determine the priority of this fetcher
        this._addFetcher(contractID);
      } catch (err) {
        this._logError("Failed to get fetcher: " + err);
      }
    }
  },

  /**
   * \brief returns a list of fetcher CIDs as an array of nsISupportsString
   * \return nsIArray of nsISupportsString
   */
  getFetcherList: function() {
    return this._fetchers;
  },
  
  /**
   * \brief turns on or off the library scanner for the main library
   * \param aTurnOn to turn on pass true, to turn off pass false
   */
  toggleScanner: function(aTurnOn) {
    // Make sure we have a library scanner
    if (!this._libraryScanner) {
      this._libraryScanner = Cc["@songbirdnest.com/Songbird/cover/library-scanner;1"]
                               .createInstance(Ci.sbILibraryScanner);
    }

    if (aTurnOn) {
      this._debug("Setting up the Library scanner for main library");
      // Setup the library scanner to do an automatic scan of all the items in the
      // main library.
      this._libraryScanner.scanLibrary(LibraryUtils.mainLibrary, this);
    } else {
      if (this._libraryScanner) {
        this._debug("Shutting down the Library scanner for main library");
        this._libraryScanner.shutdown();
        this._libraryScanner = null;
      }
    }

  },

  /*********************************
   * sbIPlaylistPlaybackListener
   ********************************/
  onStop: function() { },
  onBeforeTrackChange: function(aItem, aView, aIndex) { },
  onTrackIndexChange: function(aItem, aView, aIndex) { },

  /**
   * \brief Called when a track starts playing, this could be the same track
   */
  onTrackChange: function(aItem, aView, aIndex) {
    // The track has changed, this could be called more than once for the
    // same item, we will check that in the function _trackChanged
    this._trackChanged(aItem);
  },

  /*********************************
   * sbICoverListener
   ********************************/
  /**
   * \brief called when either a cover has been found on a service or when
   *        a cover image has been downloaded to the local system.
   * \param aMediaItem the item that cover art was fetched for
   * \param aScope null, or a set of properties and values describing all 
   *        the media items for which the result are valid.
   * \param aURI url for cover found
   */
  coverFetchSucceeded: function(aMediaItem, aScope, aURI) {
    this._debug("Cover fetch succeeded, URI = " + aURI);
    aMediaItem.setProperty(SBProperties.primaryImageURL, aURI);
    this._notify(aMediaItem);
  },

  /* \brief either the fetch from a service failed or the download failed.
   * \param aMediaItem the item that cover art was fetched for
   * \param aScope null, or a set of properties and values describing all 
   *        the media items for which the result are valid.
   */
  coverFetchFailed: function(aMediaItem, aScope) {
    this._debug("coverFetchFailed");
    // If the property is blank then set to DEFAULT_COVER
    var currentCover = aMediaItem.getProperty(SBProperties.primaryImageURL);
    if (currentCover == "" || currentCover == null) {
      aMediaItem.setProperty(SBProperties.primaryImageURL, DEFAULT_COVER);
    }
    this._notify(aMediaItem);
  },

  /*********************************
   * nsIObserver
   ********************************/
  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "profile-after-change":
        this._osService.removeObserver(this, "profile-after-change");
        this._init();
        break;
      case "quit-application":
        this._osService.removeObserver(this, "quit-application");
        this._deinit();
        break;
    }
  },

  /*********************************
   * nsISupports
   ********************************/
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.sbIAlbumArtService,
                                         Ci.sbIPlaylistPlaybackListener,
                                         Ci.nsITimerCallback,
                                         Ci.nsIObserver,
                                         Ci.sbICoverListener,
                                         Ci.sbILocalDatabaseAsyncGUIDArrayListener,
                                         Ci.nsISupportsWeakReference])

};

/**
 * XPCOM Implementation
 */
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbAlbumArtService,
  ],
  function(aCompMgr, aFileSpec, aLocation) {
    XPCOMUtils.categoryManager.addCategoryEntry(
      "app-startup",
      sbAlbumArtService.prototype.classDescription,
      "service," + sbAlbumArtService.prototype.contractID,
      true,
      true);
  });
}
