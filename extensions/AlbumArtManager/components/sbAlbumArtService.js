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

// Constants for the AlbumArtService component
const CLASSNAME = "SongbirdAlbumArtService";
const CONTRACTID = "@songbirdnest.com/songbird-album-art-service;1";
const CID = Components.ID("{B61BE8F3-7786-425D-A44F-6B1C00FA0B08}");

// Constants for convience
const Cc = Components.classes;
const Ci = Components.interfaces;

const DEFAULT_COVER = "chrome://albumartmanager/skin/no-cover.png";

Components.utils.import("resource://app/components/sbProperties.jsm");

/**
 * \brief AlbumArtService is the main service that maintains album art
 * for the albums in the library, it also handles notification of currently
 * playing item and notifications on changes to album art.
 */
function albumArtService () {
  this._osService = Cc["@mozilla.org/observer-service;1"]
                    .getService(Ci.nsIObserverService);

  // We want to wait untill profile-after-change to initialize
  this._osService.addObserver(this, 'profile-after-change', false);

  // We need to unhook things on shutdown
  this._osService.addObserver(this, "quit-application", false);
};
albumArtService.prototype = {
  constructor: albumArtService,       // Constructor to this object
  
  // Variables
  DEBUG: true,                        // Debug flag for _debug function
  _isWindows: false,                  // Boolean true = windows os
  _mainLibrary: null,                 // Main Library reference
  _listeners: [],                     // Array of listeners to call
  _lastGUID: null,                    // Guid of last media item played
  _lastArtist: null,                  // String of last Artist name played
  _sbVersion: null,                   // Version of Songbird
  
  // Services
  _osService: null,                   // Observer Service
  _prefService: null,                 // Perferences Service
  _libraryManager: null,              // Songbird Library Manager Service
  _alertService: null,                // Alert notification Service
  _playListPlaybackService: null,     // PlaylistPlayback Service
  _bundleServiceService: null,        // String Bundle Service
  
  
  /**
   * Internal debugging functions
   */
   
  /**
   * \brief Dumps out a message if the DEBUG flag is enabled with
   *        "AlbumArtService:" pre-appended.
   * \param message String to print out, the function will separate the string
   *                on (\n) new line characters.
   */
  _debug: function (message)
  {
    if (!this.DEBUG) return;
  
    var messageArray = message.split("\n");
    for (msg in messageArray)
    {
      dump("AlbumArtService: " + messageArray[msg] + "\n");
    }
  },
  
  /**
   * \brief Dumps out an error message with "AlbumArtService: [ERROR]"
   *        pre-appended.
   * \param message String to print out, the function will separate the string
   *                on (\n) new line characters.
   */
  _logError: function (message)
  {
    var messageArray = message.split("\n");
    for (msg in messageArray)
    {
      dump("AlbumArtService: [ERROR] - " + messageArray[msg] + "\n");
    }    
  },

  // nsISupports
  QueryInterface: function (aIID)
  {
    if(!aIID.equals(Ci.nsISupports) &&
       !aIID.equals(Ci.sbIAlbumArtService) &&
       !aIID.equals(Ci.sbIPlaylistPlaybackListener) &&
       !aIID.equals(Ci.nsIObserver))
      throw Components.results.NS_ERROR_NO_INTERFACE;

    return this;
  },

  // nsIObserver
  observe: function (aSubject, aTopic, aData) {
    this._debug("observer\nTopic :" + aTopic + "\nData :" + aData);

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
  
  // sbIPlaylistPlaybackListener
  /**
   * \brief Called when the media stops playing
   */
  onStop: function() {
    // Reset some variables so we can show notification when we start playing
    // again.
    this._lastArtist = null;
    this._lastGUID = null;
  },
  /**
   * \brief Called when a track starts playing, this could be the same track
   */
  onTrackChange: function(aItem, aView, aIndex) {
    // The track has changed, this could be called more than once for the
    // same item, we will check that in the function _trackChanged
    this._trackChanged(aItem);
  },

  // Internal Functions
  /**
   * \brief Initalize the AlbumArtService, grab the services we need and
   *        load up the initial set of albums
   */
  _init: function ()
  {
    this._debug("Init called");
    
    // Determine if we are on windows or not, we do some special handling for
    // file operations.
    var sysInfo = Cc["@mozilla.org/system-info;1"]
                  .getService(Ci.nsIPropertyBag2);
    if (sysInfo.getProperty("name").match(/Win/) == 'Win') {
      this._isWindows = true;
    }
    
    this._prefService = Cc['@mozilla.org/preferences-service;1']
                          .getService(Ci.nsIPrefBranch);
    this._sbVersion = this._prefService
                          .getCharPref("extensions.lastAppVersion");

    this._libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                           .getService(Ci.sbILibraryManager);

    this._playListPlaybackService = 
                             Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                             .getService(Ci.sbIPlaylistPlayback);
    this._playListPlaybackService.addListener(this);

    this._alertService = Cc["@mozilla.org/alerts-service;1"]
                          .getService(Ci.nsIAlertsService);

    var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                   .getService(Ci.nsIStringBundleService);
    this._bundleService = stringBundleService
    .createBundle("chrome://albumartmanager/locale/albumartmanager.properties");

    // Load the helper objects for getting album cover art from other services.
    var jsLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
                    .getService(Ci.mozIJSSubScriptLoader);
    jsLoader.loadSubScript(
                        "chrome://albumartmanager/content/amazonCoverGetter.js",
                        null);

    // Create a dataremote for the current album cover art
    this._currentArtDataRemote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                                   .createInstance(Ci.sbIDataRemote);
    this._currentArtDataRemote.init("albumartmanager.currentCoverUrl", null);
    this._currentArtDataRemote.stringValue = DEFAULT_COVER;

    // Add a pane to the service pane for current album art
    var paneMgr = Cc["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
              .getService(Ci.sbIDisplayPaneManager);
    var panelLabel = this._bundleService.GetStringFromName(
                                          "albumartmanager.nowplaying.label");
    paneMgr.registerContent("chrome://albumartmanager/content/servicePane.xul", 
                            panelLabel,
                           "",
                           175,
                           192,
                           "servicepane",
                           true);
  },

  /**
   * \brief Shutdown the AlbumArtService
   */
  _deinit: function ()
  {
    this._debug("Shutdown called");
    this._titleDataremote.unbind();
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
    if (!aMediaItem) {
      return;
    }

    var mAlbumArt = this._getProperty(aMediaItem,
                                      SBProperties.albumArtURL,
                                      DEFAULT_COVER);
    this._currentArtDataRemote.stringValue = mAlbumArt;

    if ( !this._playListPlaybackService.playing ||
         this._playListPlaybackService.paused ) {
      // Do not show notification if the track is not playing or is paused.
      return;
    }

    // Check if the user want to show notifications
    if (!this._prefService
          .getBoolPref("songbird.albumartmanager.display_track_notification")) {
      return;
    }

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
    try {
      this._alertService.showAlertNotification(mAlbumArt,
                                               textTitle,
                                               textOutput);
    } catch (err) {
      // Should change this to a string in the bundle and indicate Growl
      var alertError = this._bundleService.GetStringFromName(
                                          "albumartmanager.alertError");
      this._logError(alertError);
    }
  },

  /**
   * \brief Checks if a file exists removing the file:// if necessary.
   * \param aFileName string of file to check
   * \param aExtArray optional array of extensions to use
   * \return file path if file exists, or null if it does not
   */
  _confirmFileExists: function(aFileName, aExtArray) {
    try {
      var attachScheme = "file://";
      var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
      var uri;
      try {
        uri = ioService.newURI(aFileName, null, null);
      }
      catch(e) {
      }
      
      if (uri) {
        attachScheme = uri.scheme;
      }
      
      if (aFileName.indexOf(attachScheme) == 0) {
        aFileName = aFileName.replace(attachScheme, "");
      }

      if (this._isWindows) {
        aFileName = aFileName.replace(/\//g, "\\"); // we have to switch / to \
        // we may have to remove a \ at the begining...
      }
      
      var file = Cc["@mozilla.org/file/local;1"]
                 .createInstance(Ci.nsILocalFile);
      
      if (aExtArray && aExtArray.length > 0) {
        for (ext in aExtArray) {
          this._debug("Checking if [" + aFileName + "." + aExtArray[ext] + 
                      "] exists...");
          file.initWithPath(aFileName + "." + aExtArray[ext]);
          if (file.exists()) {
            return attachScheme + aFileName + "." + aExtArray[ext];
          }
        }
      } else {
        this._debug("Checking if [" + aFileName + "] exists...");
        file.initWithPath(aFileName);
        if(file.exists()) {
          return attachScheme + aFileName;
        }
      }
      
    } catch (err) { }
    
    return null;
  },
  
  /**
   * \brief Tries to get the album art for the currently playing track.
   *  For items in the main library it will update the library if album art
   *  has been found. For items else where we will not update the library.
   * \param aMediaItem Media item to use
   * \param aDoNotAutoRetrieve true = do no go grab artwork from amazon if it
   *                           does not exist
   * \return URI of album art or DEFAULT_COVER if not found.
   */
  getAlbumArtWork: function(aMediaItem, aDoNotAutoRetrieve) {
    // Use the media item to determine the cover location
    var mAlbumArt = this._getProperty(aMediaItem,
                                      SBProperties.albumArtURL,
                                      DEFAULT_COVER);
    if (mAlbumArt.indexOf("file://") == 0) {
      // now check that it exists
      if (this._confirmFileExists(mAlbumArt)) {
        return mAlbumArt;
      }
    } else if (mAlbumArt.indexOf("http") == 0) {
      // Assume all http links are valid
      return mAlbumArt;
    }

    // Check where the user normally stores the file
    var mExtArray = ["jpg", "png", "gif", "tiff", "bmp"];
    var coverDownloadLocation = this.getCoverDownloadLocation(aMediaItem);
    if (coverDownloadLocation != null) {
      this._debug("Trying location [" + coverDownloadLocation + "]");
      mAlbumArt = this._confirmFileExists(coverDownloadLocation, mExtArray);
      if (mAlbumArt) {
        this._debug("Found album art [" + mAlbumArt + "]");
        var mArtist = aMediaItem.getProperty(SBProperties.artistName);
        var mAlbum = aMediaItem.getProperty(SBProperties.albumName);
        aMediaItem.setProperty(SBProperties.albumArtURL, mAlbumArt);
        this._updateAlbumArt(aMediaItem, mAlbumArt, true, false);
        return mAlbumArt;
      }
    }
    this._debug("No art found using [" + DEFAULT_COVER + "]");

    if ( !aDoNotAutoRetrieve ) {
      this.getAlbumCoverFromAmazon(aMediaItem,
                                   true,
                                   true);
    }
    
    if (mAlbumArt != DEFAULT_COVER) {
      aMediaItem.setProperty(SBProperties.albumArtURL, DEFAULT_COVER);
    }
    return DEFAULT_COVER;
  },

  _showCurrentTrack: function()
  {
    // Check if the user want to show notifications
    if (!this._prefService
          .getBoolPref("songbird.albumartmanager.jumpToTrack")) {
      return;
    }

    var currentIndex = this._playListPlaybackService.currentIndex;
    if ( currentIndex > -1 ) {
      try {
        this._playListPlaybackService.playingView.treeView.selection
                                     .clearSelection();
        this._playListPlaybackService.playingView.treeView.selection
                                     .select( currentIndex );
        this._playListPlaybackService.playingView.treeView.selection.tree
                                     .ensureRowIsVisible( currentIndex );
      } catch ( e ) { 
      }
    }
  },

  /**
   * \brief If a new track has started playing then notify the user.
   * \param aMediaItem currently playing item.
   */
  _trackChanged: function (aMediaItem)
  {
    this._debug("Track Changed called.");
    
    if (aMediaItem == null) {
      return;
    } else if (aMediaItem.guid == this._lastGUID) {
      return;
    }
    this._lastGUID = aMediaItem.guid;

    var mAlbumArt = this.getAlbumArtWork(aMediaItem);
    if (mAlbumArt != DEFAULT_COVER) {
      this._currentArtDataRemote.stringValue = mAlbumArt;
      this._notify(aMediaItem);
    }
    this._showCurrentTrack();
  },

  /**
   * \brief Songbird versions before .4 used true to continue enumerators
   *        from the library.enumerateItemsBy* functions. So this detects
   *        what version it is and returns the proper value to use. Once
   *        0.5 comes out I will take this out and make the extension only
   *        work with 0.5 or greater
   * \return proper return value for enumerate functions
   */
  _getEnumerateContinue: function () {
    if (this._sbVersion == null) {
      return true;
    }
    
    if (this._sbVersion.indexOf("0.4") == 0) {
      return true;
    } else {
      return Ci.sbIMediaListEnumerationListener.CONTINUE
    }
  },

  /**
   * \brief Updates the album art location for a album.
   * \param aAlbumName name of the album to update
   * \param aAlbumArtist name of the artist for the album to update
   * \param aCoverUrl new url of the cover to use.
   * \param aForPlayingItem is this for the currently playing item? If so we use
   *                        the media list from the playingView.
   * \param aNotifyOnChange should we notify the user when we update the art
   *                        for the currently playing item?
   */
  _updateAlbumArt: function(aMediaItem,
                            aCoverUrl,
                            aForPlayingItem,
                            aNotifyOnChange) {
    var me = this;
    var updateListener = {
      onEnumerationBegin: function() {
        me._debug("updateAlbumArt enumeration begins...")
        return me._getEnumerateContinue();
      },
      onEnumeratedItem: function(list, item) {
        var oldArtLocation = item.getProperty(SBProperties.albumArtURL);
        if (oldArtLocation != aCoverUrl) {
          me._debug("Setting item cover to [" + aCoverUrl + "]");
          item.setProperty(SBProperties.albumArtURL, aCoverUrl);
        
          // Notifiy listeners that the current tracks cover just updated.
          // This is good for Now Playing type elements.
          if (me._playListPlaybackService.currentGUID == item.guid &&
              aNotifyOnChange) {
            me._debug("Notification for currently playing item.");
            me._notify(item);
          }
        }
        return me._getEnumerateContinue();
      },
      onEnumerationEnd: function() {
        me._debug("Done setting albumart, notifying of album change.");
        me._postChange(aCoverUrl, aMediaItem);
      }
    };

    var mArtist = aMediaItem.getProperty(SBProperties.artistName);
    var mAlbum = aMediaItem.getProperty(SBProperties.albumName);
    var propertyArray =
      Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"].
      createInstance(Ci.sbIMutablePropertyArray);
    propertyArray.appendProperty(SBProperties.albumName, mAlbum);
    propertyArray.appendProperty(SBProperties.artistName, mArtist);

    var mediaList = null;
    if (aForPlayingItem) {
      mediaList = this._playListPlaybackService.playingView.mediaList;
    } else {
      mediaList = this._libraryManager.mainLibrary;
    }
    mediaList.enumerateItemsByProperties(propertyArray, updateListener,
                                      Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
  },

  // Short cuts for notifing listeners
  _postError: function(aMediaItem) {
    var me = this;
    var mArtist = aMediaItem.getProperty(SBProperties.artistName);
    var mAlbum = aMediaItem.getProperty(SBProperties.albumName);
    this._listeners.forEach( function (listener) {
      if (listener.onAlbumArtChangeError) {
        listener.onAlbumArtChangeError(mAlbum, mArtist);
      } else {
        me._logError("Missing onAlbumArtChangeError on sbIAlbumArtListener");
      }
    });
  },
  
  _postChange: function(aCoverLocation, aMediaItem) {
    var me = this;
    var mArtist = aMediaItem.getProperty(SBProperties.artistName);
    var mAlbum = aMediaItem.getProperty(SBProperties.albumName);
    this._listeners.forEach( function (listener) {
      if (listener.onAlbumArtChange) {
        listener.onAlbumArtChange(aCoverLocation, mAlbum, mArtist);
      } else {
        me._logError("Missing onAlbumArtChange on sbIAlbumArtListener");
      }
    });
  },
  
  // nsIAlbumArtService
  get defaultCover() {
    return DEFAULT_COVER;
  },

  getCoverDownloadLocation: function(aMediaItem)
  {
    var albumName = aMediaItem.getProperty(SBProperties.albumName);
    var albumArtist = aMediaItem.getProperty(SBProperties.artistName);
    var trackName = aMediaItem.getProperty(SBProperties.trackName);
    var downloadLocation = aMediaItem.getProperty(SBProperties.contentURL);
    
    if (downloadLocation.indexOf("file://") != 0) {
      return null;
    }
    
    var toAlbumLocation = true;
    try {
      toAlbumLocation = this._prefService
              .getBoolPref("songbird.albumartmanager.cover.usealbumlocation");
    } catch (err) {
      this._prefService
        .setBoolPref("songbird.albumartmanager.cover.usealbumlocation", true);
    }

    if (toAlbumLocation) {
      downloadLocation = downloadLocation.substr(0, 
                                        downloadLocation.lastIndexOf("/") + 1);
      downloadLocation = downloadLocation.replace("file://", "");
      if (this._isWindows) {
        if (downloadLocation.indexOf("/") == 0) {
          downloadLocation = downloadLocation.substr(1);
        }
        downloadLocation = downloadLocation.replace(/\//g, "\\");
      }
      downloadLocation = downloadLocation + "cover";
    } else {
      try {
        var otherLocation = this._prefService.getComplexValue(
                                        "songbird.albumartmanager.cover.folder",
                                        Ci.nsILocalFile);
        downloadLocation = otherLocation.path;

        var coverFormat = this._prefService.getCharPref(
                                      "songbird.albumartmanager.cover.format");
        if (coverFormat == "" || coverFormat == null) {
          coverFormat = "cover";
        }
        downloadLocation = downloadLocation + "/" + coverFormat;          
        if (this._isWindows) {
          downloadLocation = downloadLocation.replace(/\//g, "\\");
        }

        if ( (coverFormat.indexOf("%album%") >= 0 && albumName == null) ||
             (coverFormat.indexOf("%artist%") >= 0 && albumArtist == null) ||
             (coverFormat.indexOf("%track%") >= 0 && trackName == null) ) {
          // we are going to have nulls in the filename so abort
          return null;
        }

        downloadLocation = downloadLocation.replace("%album%", albumName);
        downloadLocation = downloadLocation.replace("%artist%", albumArtist);
        downloadLocation = downloadLocation.replace("%track%", trackName);
      } catch (err) {
        this._logError("Unable to get download location: " + err);
      }
    }

    // Make sure this is a valid filename no special characters.
    downloadLocation = decodeURI(downloadLocation); // remove the %20 and such
    //downloadLocation = downloadLocation.replace(":", "_");

    return downloadLocation;
  },

  getAlbumCoverFromAmazon: function (aMediaItem,
                                     aForPlayingItem,
                                     aNotifyOnChange) {
    var me = this;
    var gAlbumCoverReceiver = {
     onSuccess : function(aMediaItem, albumCoverUrl) {
       me._updateAlbumArt(aMediaItem,
                          albumCoverUrl,
                          aForPlayingItem,
                          aNotifyOnChange);
     },
     onError: function() {
      if (aNotifyOnChange) {
        me._notify(aMediaItem);
      }
       me._postError(aMediaItem);
     }
    }

    var aDownloadLocation = this.getCoverDownloadLocation(aMediaItem);
    if (aDownloadLocation == null) {
      if (aNotifyOnChange) {
        this._notify(aMediaItem);
      }
      return;
    }
    
    var coverGetter = new AmazonAlbumCoverGetter(aMediaItem,
                                                 aDownloadLocation,
                                                 gAlbumCoverReceiver);
    if (coverGetter.init()) {
      this._debug("Calling AmazonAlbumCoverGetter.get");
      coverGetter.get();
    } else {
      this._logError("Failed to create AmazonAlbumCoverGetter");
      if (aNotifyOnChange) {
        this._notify(aMediaItem);
      }
      this._postError(aMediaItem);
    }
  },

  getAlbumCoverFromFile: function (aWindow, aMediaItem) {
    // Open the file picker
    var filePicker = Cc["@mozilla.org/filepicker;1"]
                        .createInstance(Ci.nsIFilePicker);
    var windowTitle = this._bundleService
                  .GetStringFromName("albumartmanager.filepicker.windowtitle");
    filePicker.init( aWindow, windowTitle,
                    Ci.nsIFilePicker.modeOpen);
    var fileResult = filePicker.show();
    if (fileResult == Ci.nsIFilePicker.returnOK) {
      this._updateAlbumArt(aMediaItem, "file://" + filePicker.file.path);
    } else {
      this._postError(aMediaItem);
    }
  },

  clearAlbumCover: function (aMediaItem) {
    this._updateAlbumArt(aMediaItem, DEFAULT_COVER);
  },
  
  addListener: function (aListener)
  {
    if (! (aListener instanceof Ci.sbIAlbumArtListener)) {
      throw Components.results.NS_ERROR_INVALID_ARG;
    }
    if (this._listeners.indexOf(aListener) == -1) {
      this._debug("Adding listener");
      this._listeners.push(aListener);
    }
  },

  removeListener: function (aListener)
  {
    var index = this._listeners.indexOf(aListener);
    if (index > -1) {
      this._debug("Removing listener");
      this._listeners.splice(index,1);
    }    
  }

};



/**
 * XPCOM Registration
 */
var Module = new Object();

Module.registerSelf =
function(compMgr, fileSpec, location, type)
{
  compMgr = compMgr.QueryInterface(Ci.nsIComponentRegistrar);
  compMgr.registerFactoryLocation(CID,
                                  CLASSNAME,
                                  CONTRACTID,
                                  fileSpec,
                                  location,
                                  type);

  var catMgr = Cc["@mozilla.org/categorymanager;1"]
                 .getService(Ci.nsICategoryManager);

  catMgr.addCategoryEntry("app-startup",
                          CLASSNAME,
                          "service," + CONTRACTID,
                          true,
                          true);
}

Module.getClassObject =
function(compMgr, cid, iid)
{
  if(!cid.equals(CID)) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  }

  if(!iid.equals(Ci.nsIFactory)) {
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }

  return Factory;
}

Module.unregisterSelf = 
function(compMgr, location, type)
{
  compMgr.QueryInterface(Ci.nsIComponentRegistrar);
  compMgr.unregisterFactory(CID, location);
  var catman = Componnets.classes["@mozilla.org/categorymanager;1"]
                 .getService(Ci.nsICategoryManager);
  catman.deleteCategoryEntry("app-startup",
                             CLASSNAME,
                             "service," + CONTRACTID,true);
}

Module.canUnload =
function(compMgr)
{
  return true;
}

var Factory = {};

Factory.createInstance =
function(outer, iid)
{
  if(outer != null) {
    throw Components.results.NS_ERROR_NO_AGGREGATION;
  }

  return (new albumArtService()).QueryInterface(iid);
}

function NSGetModule(compMgr, fileSpec)
{
  return Module;
}

