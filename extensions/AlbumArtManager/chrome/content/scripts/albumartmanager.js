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
//  Steven Bengtson <steven@songbirdnest.com>
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

const STRING_BUNDLE = "chrome://albumartmanager/locale/albumartmanager.properties";
const SEARCH_TIMER_INTERVAL = 500;
const KEY_TIMER_INTERVAL    = 200;

const FETCH_BUTTON_ENABLED  = 1;
const FETCH_BUTTON_DISABLED = 2;
const FETCH_BUTTON_HIDDEN   = 3;

Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

var AlbumArtManager = {
  className: "AlbumArtManager",
  
  _AlbumArtBox: null,       // Container for all the albums

  _albumArtService: null,   // Album Art Service
  _bundle: null,            // String bundle
  _preferences: null,       // Preferences Service
  
  _fetchers: null,          // List of fetchers available.
  
  _keyTimer: null,          // Timer for Key Events
  
  // set up a timer to do work on
  _searchTimer: null,
  _searchNodes: [],         // Array of guids to search for covers
  _searchNodeLength: 0,     // Number of nodes we searched for
  
  _rdfService: null,        // RDF Service component
  _dataSource: null,        // Datasource for our list of albums

  DEBUG: false,             // Set to true for debug output on console
  
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
   * AlbumArtManager functions
   */
  init: function() {
    this._mainLibrary = LibraryUtils.mainLibrary;

    this._preferences = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);

    this._consoleService = Cc["@mozilla.org/consoleservice;1"]
                            .getService(Ci.nsIConsoleService);

    try {
      this.DEBUG = this._preferences.getBoolPref("songbird.albumartmanager.debug");
    } catch (err) {}
    
    this._AlbumArtBox = document.getElementById("AlbumArtManager_AlbumList");

    var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                              .getService(Ci.nsIStringBundleService);
    this._bundle = stringBundleService.createBundle(STRING_BUNDLE);

    this._albumArtService = Cc["@songbirdnest.com/songbird-album-art-service;1"]
                            .getService(Ci.sbIAlbumArtService);

    this._fetchers = this._albumArtService.getFetcherList();

    // Set up the datasource for albums
    this._rdfService = Cc["@mozilla.org/rdf/rdf-service;1"]
                         .getService(Ci.nsIRDFService);
    this._dataSource = Cc["@mozilla.org/rdf/datasource;1?name=in-memory-datasource"]
                         .createInstance(Ci.nsIRDFDataSource);
    
    var mAlbumBox = document.getElementById("AlbumArtManager_AlbumList");
    mAlbumBox.database.AddDataSource(this._dataSource);
    mAlbumBox.setAttribute("ref", "urn:root");

    this._searchTimer = Cc['@mozilla.org/timer;1'].createInstance(Ci.nsITimer);
    this._keyTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

    // Load up the albums
    this.getAlbums();
  },
  
  deinit: function() {
    // Unload listeners and services
    this._timer.cancel();
    this._timer = null;
    this._bundle = null;
    this._preferences = null;
    this._dataSource = null;
    this._rdfService = null;
    this._albumArtService = null;
    this._fetchers = null;
  },

  openPrefs: function() {
    if (window && window.SBOpenPreferences) {
      window.SBOpenPreferences("library_albumartmanager_pref_pane");
    } else {
      var windowMediator;
      var sbWindow;

      /* Get the main Songbird window. */
      windowMediator = Components.classes
                          ["@mozilla.org/appshell/window-mediator;1"].
                          getService(Components.interfaces.nsIWindowMediator);
      sbWindow = windowMediator.getMostRecentWindow("Songbird:Main");

      /* Open the preferences pane. */
      sbWindow.SBOpenPreferences("library_albumartmanager_pref_pane");
    }
  },

  closeWindow: function() {
    window.close();
  },

  updateStatus: function(aMessage, aIndex, aTotalCount) {
    var mStatusLabel = document.getElementById("AlbumArtManager_StatusMessage");
    var mStatusProgress = document.getElementById("AlbumArtManager_StatusProgress");
    var mStatusProgressLabel = document.getElementById("AlbumArtManager_StatusProgressLabel");
    
    mStatusLabel.setAttribute("label", aMessage);
    
    if (aIndex == 0 && aTotalCount == 0) {
      mStatusProgress.setAttribute("hidden", "true");
      mStatusProgressLabel.setAttribute("hidden", "true");
    } else {
      var percentage = Math.round(((aIndex / aTotalCount) * 100));
      mStatusProgress.setAttribute("value", percentage);
      mStatusProgressLabel.setAttribute("label",
                                        SBFormattedString("albumartmanager.status.searchingpercent",
                                                          [aIndex, aTotalCount, percentage],
                                                          this._bundle));
    }
  },

  updateFetchButton: function(aState) {
    var fetchButton = document.getElementById("toolbar-missing-covers");
    var cancelButton = document.getElementById("toolbar-missing-covers-cancel");
    
    switch (aState) {
      case FETCH_BUTTON_ENABLED:
        this._debug("Updating fetch button: [ENABLED] " + aState);
        cancelButton.setAttribute("hidden", true);
        fetchButton.removeAttribute("disabled");
        fetchButton.removeAttribute("hidden");
      break;
      case FETCH_BUTTON_DISABLED:
        this._debug("Updating fetch button: [ENABLED] " + aState);
        cancelButton.setAttribute("hidden", true);
        fetchButton.setAttribute("disabled", true);
        fetchButton.removeAttribute("hidden");
      break;
      case FETCH_BUTTON_HIDDEN:
        this._debug("Updating fetch button: [ENABLED] " + aState);
        cancelButton.removeAttribute("hidden");
        fetchButton.setAttribute("disabled", true);
        fetchButton.setAttribute("hidden", true);
      break;
      default:
        this._debug("Unkown state for fetch button " + aState);
        this.updateFetchButton(FETCH_BUTTON_ENABLED);
      break;
    }
  },
  
  clearAlbumArt: function(aMediaItem) {
    var mAlbumName = aMediaItem.getProperty(SBProperties.albumName);
    this._toggleSearching(aMediaItem, true);
    
    // Lets ask the user if they want to delete the cover file :)
    var mAlbumArt = aMediaItem.getProperty(SBProperties.primaryImageURL);
    try {
      uri = this._ioService.newURI(mAlbumArt, null, null);
      if (uri.scheme == "file") {
        var prompts = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                        .getService(Ci.nsIPromptService);
        var neverPromptAgain = { value: false };
        var result = prompts.confirmEx(null,
                                       SBString("albumartmanager.clear.confirm_title", null, this._bundle),
                                       SBString("albumartmanager.clear.confirm_text", null, this._bundle),
                                       Ci.nsIPromptService.STD_YES_NO_BUTTONS,
                                       null, null, null,
                                       null,
                                       neverPromptAgain);
        if (result == 0) {
          try {
            var fph = this._ioService.getProtocolHandler("file")
                          .QueryInterface(Ci.nsIFileProtocolHandler);
            var file = fph.getFileFromURLSpec(mAlbumArt);
            this._debug("Removing file " + file.path);
            file.remove(false);
          } catch (err) {
            this._logError("Unable to remove file " + mAlbumArt + " - " + err);
          }
        }
      }
    } catch (err) {
      this._logError("Unable to convert " + mAlbumArt + " to URI");
    }

    this._albumArtService.updateAlbumCover(mAlbumName,
                                           this._albumArtService.defaultCover);
    this.updateAlbumNode(aMediaItem,
                       "coverUrl",
                       this._albumArtService.defaultCover);

    this._toggleSearching(aMediaItem, false);
  },

  /**
   * \brief Uses a sbICoverFetcher to search for a cover.
   * \param aMediaItem Media Item to search a cover for.
   * \param aUseFetcher the CID of a Fetcher to use.
   * \param aWindow [optional] indicates we have user interaction for some
   *                fetchers.
   */
  getCover: function(aMediaItem, aUseFetcher, aWindow) {
    this.toggleSearching(aMediaItem, true);
    var mAlbumName = aMediaItem.getProperty(SBProperties.albumName);
    this.updateStatus("Searching for cover for " + mAlbumName, 0, 0);
    if (!aUseFetcher) {
      // We need to scan all fetchers for the cover
      this._debug("Starting cover scanner for " + aMediaItem);
      var coverScanner = Cc["@songbirdnest.com/Songbird/cover/cover-scanner;1"]
                           .createInstance(Ci.sbICoverScanner);
      coverScanner.fetchCover(aMediaItem, this);
    } else {
      this._debug("Starting the fetcher [" + aUseFetcher + "]...");
      try {
        var fetcher = Cc[aUseFetcher].getService(Ci.sbICoverFetcher);
        fetcher.fetchCoverForMediaItem(aMediaItem, this, aWindow);
      } catch (err) {
        this._toggleSearching(aMediaItem, false);
        this._logError("Unable to get fetcher: " + aUseFetcher);
      }
    }
  },

  getAlbumArt: function(aMediaItem, aFetcherCid) {
    this.getCover(aMediaItem, aFetcherCid, window);
    return true;
  },

  getMissingCovers: function() {
    if (this._searchNodeLength == 0) {
      // Get a list of nodes to search
      if (this._AlbumArtBox.hasChildNodes()) {
        var defaultCover = this._albumArtService.defaultCover;
        var currentNode = this._AlbumArtBox.firstChild;
        while (currentNode != null) {
          var mMediaItemGuid = currentNode.getAttribute("mediaItemGuid");
          var mAlbumArt = currentNode.getAttribute("src");
          if ( (mAlbumArt == defaultCover || mAlbumArt == "") &&
               (mMediaItemGuid != "") ) {
            this._searchNodes.push(mMediaItemGuid);
          }
          currentNode = currentNode.nextSibling;
        }
        
        if (this._searchNodes.length > 0) {
          this.updateFetchButton(FETCH_BUTTON_HIDDEN);
          this._searchNodeLength = this._searchNodes.length;
          this._searchTimer.initWithCallback(this,
                                             SEARCH_TIMER_INTERVAL, 
                                             Ci.nsITimer.TYPE_REPEATING_SLACK);
        }
      }
    }
  },

  cancelFetching: function() {
    this.updateFetchButton(FETCH_BUTTON_ENABLED);
    this._searchTimer.cancel();
    this._searchNodeLength = 0;
    this._searchNodes = [];
  },
  
  createMenuPopup: function(aMenuParent) {
    var aFirstChild = aMenuParent.firstChild;
    for (var fIndex = 0; fIndex < this._fetchers.length; fIndex++) {
      // Add a menu entry for each one
      var fetcherCid = this._fetchers.queryElementAt(fIndex, Ci.nsISupportsString).toString();
      var cFetcher = Cc[fetcherCid].createInstance(Ci.sbICoverFetcher);
      
      if (cFetcher.userFetcher) {
        var menuLabel = SBFormattedString("albumartmanager.menu.label",
                                          [cFetcher.name],
                                          this._bundle);
        
        var menuItem = document.createElement("menuitem");
        menuItem.setAttribute("label", menuLabel);
        menuItem.setAttribute("oncommand",
                              "this.parentNode.parentNode.getCover('" +
                                fetcherCid + "');");
        menuItem.setAttribute("sbid", "menu-" + fetcherCid);
        aMenuParent.insertBefore(menuItem, aFirstChild);
      }
    }
  },

  onKeyHandler: function(aEvent) {
    if (this._keyTimer) {
      this._keyTimer.cancel();
    }
    this._keyTimer.initWithCallback(this,
                                    KEY_TIMER_INTERVAL,
                                    Ci.nsITimer.TYPE_ONE_SHOT);
  },
  
  notify: function(aTimer) {
    if (aTimer == this._searchTimer) {
      if (this._searchNodes.length > 0) {
        // We still have some nodes to search
        var mMediaItemGuid = this._searchNodes[0];
        this._searchNodes.splice(0,1); // Remove the guid
        var mMediaItem = this._mainLibrary.getMediaItem(mMediaItemGuid);
        var albumName = mMediaItem.getProperty(SBProperties.albumName);
        this.updateStatus(SBFormattedString("albumartmanager.status.searchingforCover",
                                            [albumName],
                                            this._bundle),
                          (this._searchNodeLength -this._searchNodes.length),
                          this._searchNodeLength);
        this.getCover(mMediaItem, null, null);
      } else if (this._searchNodeLength > 0) {
        // All done searching
        this._searchTimer.cancel();
        this._searchNodeLength = 0;
        this._searchNodes = [];
        this.updateStatus(SBString("", null, this._bundle), 0, 0);
        this.updateFetchButton(FETCH_BUTTON_ENABLED);
      }
    }

    if (aTimer == this._keyTimer) {
      this._keyTimer.cancel();
      
      // Filter albums
      var filterBox  = document.getElementById("filterBox");
      var albumWhere = document.getElementById("albumMatch");
      albumWhere.setAttribute("value", filterBox.value);
      this._AlbumArtBox.builder.rebuild();
    }
  },

  getAlbums: function() {
    // NOTE this is not really the way you SHOULD do this, but I am because
    // it is so much faster :).
    var mainLocalLibrary = LibraryUtils.mainLibrary.QueryInterface(Ci.sbILocalDatabaseLibrary);
    this._array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/AsyncGUIDArray;1"]
                   .createInstance(Ci.sbILocalDatabaseAsyncGUIDArray);
    this._array.databaseGUID = mainLocalLibrary.databaseGuid;
    this._array.propertyCache = mainLocalLibrary.propertyCache;
    this._array.baseTable = 'media_items';
    this._array.isDistinct = true;
    this._array.addSort(SBProperties.albumName, true);
    this._array.addAsyncListener(this);
    this._array.invalidate();
    this._array.getLengthAsync();
  },

  addAlbum: function(aMediaItem) {
    var mAlbum = aMediaItem.getProperty(SBProperties.albumName);
    if (mAlbum == null || mAlbum == "") {
      return;
    }

    var mLocation = aMediaItem.getProperty(SBProperties.contentURL);
    if (mLocation.indexOf("file://") != 0) {
      return;
    }

    var mArtist = aMediaItem.getProperty(SBProperties.artistName);
    var mAlbumArtUrl = this._albumArtService.getAlbumArtWork(aMediaItem, false);
    var mGuid = aMediaItem.guid;

    this._dataSource.Assert(this._rdfService.GetResource("urn:root"),
                            this._rdfService.GetResource("http://songbirdnest.com/SB-rdf#album"),
                            this._rdfService.GetResource(mGuid),
                            true);

    this._dataSource.Assert(this._rdfService.GetResource(mGuid),
                            this._rdfService.GetResource("http://songbirdnest.com/SB-rdf#albumTitle"),
                            this._rdfService.GetLiteral(mAlbum),
                            true);
    if (mArtist) {
      this._dataSource.Assert(this._rdfService.GetResource(mGuid),
                              this._rdfService.GetResource("http://songbirdnest.com/SB-rdf#artistName"),
                              this._rdfService.GetLiteral(mArtist),
                              true);
    }
    this._dataSource.Assert(this._rdfService.GetResource(mGuid),
                            this._rdfService.GetResource("http://songbirdnest.com/SB-rdf#coverUrl"),
                            this._rdfService.GetLiteral(mAlbumArtUrl),
                            true);
    this._dataSource.Assert(this._rdfService.GetResource(mGuid),
                            this._rdfService.GetResource("http://songbirdnest.com/SB-rdf#searching"),
                            this._rdfService.GetLiteral("false"),
                            true);
    this._dataSource.Assert(this._rdfService.GetResource(mGuid),
                            this._rdfService.GetResource("http://songbirdnest.com/SB-rdf#playing"),
                            this._rdfService.GetLiteral("false"),
                            true);
  },

  // RDF maintenance
  updateAlbumNode: function(aMediaItem, aParam, aNewValue) {
    this._debug("Updating node: " + aMediaItem.guid + " " + aParam + " = " + aNewValue);
    var root = this._rdfService.GetResource("urn:root");
    var albumNode = this._rdfService.GetResource("http://songbirdnest.com/SB-rdf#album");
    var albumGuid = this._rdfService.GetResource(aMediaItem.guid);
    
    // Only change it if the album exists with this mediaitem guid
    if (this._dataSource.HasAssertion(root, albumNode, albumGuid, true)) {
      var property = this._rdfService.GetResource("http://songbirdnest.com/SB-rdf#" + aParam);
      var oldTarget = this._dataSource.GetTarget(albumGuid, property, true);
      var newTarget = this._rdfService.GetLiteral(aNewValue);
    
      // Only update if newTarget is different
      if (oldTarget) {
        this._dataSource.Change(albumGuid, property, oldTarget, newTarget);
      } else {
        // No existing target so add one.
        this._dataSource.Assert(albumGuid, property, newTarget, true);
      }
    }
  },

  toggleSearching: function(aMediaItem, aTurnOn) {
    this.updateAlbumNode(aMediaItem, "searching", aTurnOn);
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
    this.toggleSearching(aMediaItem, false);
    var mAlbumName = aMediaItem.getProperty(SBProperties.albumName);
    this.updateStatus(SBFormattedString("albumartmanager.status.searchfound",
                                       [mAlbumName],
                                       this._bundle),
                      0,
                      0);
    
    this._albumArtService.updateAlbumCover(mAlbumName, aURI);
    this.updateAlbumNode(aMediaItem, "coverUrl", aURI);
    
    if (this._searchNodeLength > 0) {
      this._debug("Trying next album");
      this.getMissingCovers();
    }
  },

  /* \brief either the fetch from a service failed or the download failed.
   * \param aMediaItem the item that cover art was fetched for
   * \param aScope null, or a set of properties and values describing all 
   *        the media items for which the result are valid.
   */
  coverFetchFailed: function(aMediaItem, aScope) {
    // If the property is blank then set to DEFAULT_COVER
    var currentCover = aMediaItem.getProperty(SBProperties.primaryImageURL);
    var mAlbumName = aMediaItem.getProperty(SBProperties.albumName);
    this.updateStatus(SBFormattedString("albumartmanager.status.searchfailed",
                                       [mAlbumName],
                                       this._bundle),
                      0,
                      0);

    if (currentCover == "" || currentCover == null) {
      this._albumArtService.updateAlbumCover(mAlbumName,
                                             this._albumArtService.defaultCover);
      this.updateAlbumNode(aMediaItem,
                         "coverUrl",
                         this._albumArtService.defaultCover);
    }
    this.toggleSearching(aMediaItem, false);

    if (this._searchNodeLength > 0) {
      this._debug("Trying next album");
      this.getMissingCovers();
    }
  },
  
  /*********************************
   * sbILocalDatabaseAsyncGUIArrayListener
   ********************************/
  onGetGuidByIndex: function(aIndex, aGuid, aResult) {
    if (aGuid) {
      var item = LibraryUtils.mainLibrary.getMediaItem(aGuid);
      var albumName = item.getProperty(SBProperties.albumName);
      this.updateStatus(SBFormattedString("albumartmanager.status.searchingforAlbum",
                                         [aIndex, this._length, albumName],
                                         this._bundle),
                        aIndex,
                        this._length);
      this.addAlbum(item);
    }

    if (this._length > aIndex) {
      this._array.getGuidByIndexAsync(++aIndex);
    } else {
      this.updateFetchButton(FETCH_BUTTON_ENABLED);
      this.updateStatus(SBFormattedString("albumartmanager.status.searchcomplete",
                                         [this._length], this._bundle),
                        this._length,
                        this._length);
    }
  },

  onGetLength: function(aLength, aResult) {
    this._length = (aLength - 1);

    if (this._length > 0) {
      this.updateStatus(SBFormattedString("albumartmanager.status.searching",
                                          null,
                                          this._bundle),
                        0,
                        this._length);
      this._array.getGuidByIndexAsync(0);
    }
  },

  onGetSortPropertyValueByIndex: function(aIndex, aPropertySortValue, aResult) { },
  onGetMediaItemIdByIndex: function(aIndex, aMediaItemId, aResult) { },
  onStateChange: function(aState) { },

  /*********************************
   * nsISupports
   ********************************/
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports,
                                         Ci.sbICoverListener,
                                         Ci.nsITimerCallback,
                                         Ci.sbILocalDatabaseAsyncGUIDArrayListener,
                                         Ci.nsISupportsWeakReference])

};
