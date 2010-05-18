/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
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
 *=END SONGBIRD GPL
 */

if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbCoverHelper.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/sbMetadataUtils.jsm");
Cu.import("resource://app/jsmodules/SBUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

// Display pane constants
const DISPLAY_PANE_CONTENTURL = "chrome://albumart/content/albumArtPane.xul";
const DISPLAY_PANE_ICON       = "chrome://albumart/skin/icon-albumart.png";

// Default cover for items missing the cover
const DROP_TARGET_IMAGE = "chrome://songbird/skin/album-art/drop-target.png";

// Constants for toggling between Selected and Playing states
// These releate to the deck
const STATE_SELECTED = 0;
const STATE_PLAYING  = 1;

// Preferences
const PREF_STATE = "songbird.albumart.displaypane.view";

// Namespace defs.
if (typeof(XLINK_NS) == "undefined")
  const XLINK_NS = "http://www.w3.org/1999/xlink";

/*
 * Function invoked by the display pane manager to populate the display pane
 * menu.
 */
function onDisplayPaneMenuPopup(command, menupopup, doc) {
  switch (command) {
    case "create":
      AlbumArt._populateMenuControl(menupopup, doc);
      break;
    case "destroy":
      AlbumArt._destroyMenuControl();
      break;
    default:
      dump("AlbumArt displaypane menu: unknown command: " + command + "\n");
  }
}

/******************************************************************************
 *
 * \class AlbumArt 
 * \brief Controller for the Album Art Pane.
 *
 *****************************************************************************/
var AlbumArt = {
  _coverBind: null,                   // Data remote for the now playing image.
  _mediacoreManager: null,            // Get notifications of track changes.
  _mediaListView: null,               // Current active mediaListView.
  _browser: null,                     // Handle to browser for tab changes.
  _displayPane: null,                 // Display pane we are in.
  _nowSelectedMediaItem: null,        // Now selected media item.
  _nowSelectedMediaItemWatcher: null, // Now selected media item watcher.
  _prevAutoFetchArtworkItem: null,    // Previous item for which auto artwork
                                      // fetch was attempted.
  _autoFetchTimeoutID: null,          // ID of auto-fetch timeout.

  // Array of state information for each display (selected/playing/etc)
  _stateInfo: [ { imageID: "sb-albumart-selected",
                  titleID: "selected",
                  menuID: "selected",
                  defaultTitle: "Now Selected"
                },
                { imageID: "sb-albumart-playing",
                  titleID: "playing",
                  menuID: "playing",
                  defaultTitle: "Now Playing"
                }
              ],
  _currentState: STATE_SELECTED, // Default to now selected state (display)

  _switchToNowPlaying: function() {
    AlbumArt.switchState(STATE_PLAYING);
  },
  _switchToNowSelected: function() {
    AlbumArt.switchState(STATE_SELECTED);
  },
  _populateMenuControl: function AlbumArt_populateMenuControl(menupopup, doc) {
    // now playing
    var nowPlayingMenu = doc.createElement("menuitem");
    var titleInfo = AlbumArt._stateInfo[STATE_PLAYING];
    var npString = SBString("albumart.displaypane.menu.show" +
                            titleInfo.titleID, titleInfo.defaultTitle);
    nowPlayingMenu.setAttribute("label", npString);
    nowPlayingMenu.setAttribute("type", "radio");
    nowPlayingMenu.addEventListener("command", AlbumArt._switchToNowPlaying,
                                    false);
    menupopup.appendChild(nowPlayingMenu);
    
    // now selected
    var nowSelectedMenu = doc.createElement("menuitem");
    titleInfo = AlbumArt._stateInfo[STATE_SELECTED];
    var nsString = SBString("albumart.displaypane.menu.show" +
                         titleInfo.titleID, titleInfo.defaultTitle);
    nowSelectedMenu.setAttribute("label", nsString);
    nowSelectedMenu.setAttribute("type", "radio");
    nowSelectedMenu.addEventListener("command",
                                                  AlbumArt._switchToNowSelected,
                                                  false);
    menupopup.appendChild(nowSelectedMenu);
   
    if (AlbumArt._currentState == STATE_PLAYING)
      nowPlayingMenu.setAttribute("selected", "true");
    else
      nowSelectedMenu.setAttribute("selected", "true");

    AlbumArt._menuPopup = menupopup;
  },

  _destroyMenuControl: function AlbumArt_destroyMenuControl() {
    while (AlbumArt._menuPopup.firstChild) {
      AlbumArt._menuPopup.removeChild(AlbumArt._menuPopup.firstChild);
    }
  },

  /**
   * \brief onImageDblClick - This function is called when the user double
   *        clicks the image.
   * \param aEvent - event object of the current event.
   */
  onImageDblClick: function AlbumArt_onImageDblClick(aEvent) {
    // Only respond to primary button double clicks.
    var passImageParam = null;
    if (aEvent.button == 0) {
      // If the user is clicking on the "Now Selected" image we are going to
      // display that one, otherwise the window will display the currently
      // playing image.
      if (aEvent.target.id == 'sb-albumart-selected') {
        passImageParam = this.getCurrentStateItemImage();
      }

      var winMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                          .getService(Ci.nsIWindowMediator);
      var mainWin = winMediator.getMostRecentWindow("Songbird:Main");
      if (mainWin && mainWin.window) {
        mainWin.openDialog("chrome://albumart/content/coverPreview.xul",
                           "coverPreview",
                           "chrome,centerscreen,resizeable=no",
                           passImageParam);
      }
    }
  },

  /**
   * \brief setPaneTitle - This function updates the pane title depending on
   *        the current state.
   */
  setPaneTitle: function AlbumArt_setPaneTitle() {
    // Sanity check
    if (AlbumArt._currentState >= AlbumArt._stateInfo.length ||
        AlbumArt._currentState < 0) {
      return;
    }
    
    var titleInfo = AlbumArt._stateInfo[AlbumArt._currentState];
    var titleString = SBString("albumart.displaypane.title." + titleInfo.titleID,
                               titleInfo.defaultTitle);
    
    // We set the title with contentTitle instead of using the paneMgr because
    // we do not want to change the name for this display pane anywhere else.
    if (AlbumArt._displayPane) {
      AlbumArt._displayPane.contentTitle = titleString;
    }
  },

  /**
   * \brief switchState - This function will switch the deck to the desired
   *        display, this allows us to easily switch between the "Selected" and
   *        "Playing" displays of the album art.
   * \param aNewState - is the index of the deck to make active.
   */
  switchState: function AlbumArt_switchState(aNewState) {
    AlbumArt._currentState = aNewState;
    var albumArtDeck = document.getElementById('sb-albumart-deck');
    albumArtDeck.selectedIndex = AlbumArt._currentState;
    AlbumArt.setPaneTitle();

    if (AlbumArt._currentState == STATE_PLAYING) {
      document.getElementById("showNowSelectedMenuItem")
              .setAttribute("checked", "false");
      document.getElementById("showNowPlayingMenuItem")
              .setAttribute("checked", "true");
    } else {
      document.getElementById("showNowPlayingMenuItem")
              .setAttribute("checked", "false");
      document.getElementById("showNowSelectedMenuItem")
              .setAttribute("checked", "true");
    }
  },

  /**
   * \brief canEditItems - This checks that all items in aItemArray can have
   *        their metadata edited. It will return true only if all items are
   *        editable.
   * \param aItemArray - Array of sbIMediaItem.
   * \returns true if all items in the array are editable, false otherwise.
   */
  canEditItems: function AlbumArt_canEditItems(aItemArray) {
    for each (var item in aItemArray) {
      if (!LibraryUtils.canEditMetadata(item)) {
        return false;
      }
    }
    return true;
  },

  /**
   * \brief - changeImage updates the image information, menus, and such for the required display.
   *          The display is either the selected or now playing decks.
   * \param aNewURL - URL to set image to
   * \param aImageElement - Image element we are changing.
   * \param aNotBox - the not box related to this image.
   * \param aDragBox - the drag box related to this image.
   * \param isPlayingOrSelected - Indicates if there are items selected or one is playing.
   */
  changeImage: function AlbumArt_changeImage(aNewURL,
                                             aImageElement,
                                             aNotBox,
                                             aDragBox,
                                             aStack,
                                             isPlayingOrSelected) {
    // Determine what to display
    if (!isPlayingOrSelected) {
      // No currently playing or selected items

      // Show the not selected/playing message.
      aNotBox.hidden = false;

      // Hide the Drag Here box
      aDragBox.hidden = true;

      // Set the image to the default cover
      aNewURL = DROP_TARGET_IMAGE;
    
      aStack.className = "artwork-none";
    } else if (!aNewURL) {
      // We are playing or have selected items, but the image is not available

      // Show the Drag Here box
      aDragBox.hidden = false;

      // Hide the not selected message.
      aNotBox.hidden = true;

      // Set the image to the default cover
      aNewURL = DROP_TARGET_IMAGE;
      
      aStack.className = "artwork-none";
    } else {
      // We are playing or have selected items, and we have a valid image

      // Hide the not selected/playing message.
      aNotBox.hidden = true;

      // Hide the Drag Here box
      aDragBox.hidden = true;
      
      aStack.className = "artwork-found";
    }

    if (!Application.prefs.getValue("songbird.albumart.autofetch.disabled",
                                    false)) {
      // Auto-fetch artwork.
      AlbumArt.autoFetchArtwork();
    }

    var princely = Application.prefs.getValue("songbird.purplerain.prince",
                                              false);
    if ((princely == "1") && aNotBox.hidden)
      aNewURL = "chrome://songbird/skin/album-art/princeaa.jpg";

    /* Set the image element URL */
    if (aNewURL) {
      aImageElement.setAttributeNS(XLINK_NS, "href", aNewURL);

      // Attempt to determine the image's height & width so we can use it
      // for aspect ratio scaling calculations later (to adjust the display
      // pane height)
      var img = new Image();
      img.addEventListener("load", function(e) {
          AlbumArt.imageDimensions = {
            width: img.width,
            height: img.height,
            aspectRatio: (img.height/img.width)
          }
          img.removeEventListener("load", arguments.callee, false);
          delete img;
      }, false);
      img.src = aNewURL;
    } else {
      aImageElement.removeAttributeNS(XLINK_NS, "href");
    }
  },

  /**
   * \brief changeNowSelected - This function changes the Now Selected display
   *        to what is suppose to be displayed based on the image.
   */
  changeNowSelected: function AlbumArt_changeNowSelected(aNewURL) {
    // Load up our elements
    var albumArtSelectedImage = document.getElementById('sb-albumart-selected');
    var albumArtNotSelectedBox = document.getElementById('sb-albumart-not-selected');
    var albumArtSelectedDragBox = document.getElementById('sb-albumart-select-drag');
    var stack = document.getElementById('sb-albumart-nowselected-stack');
    
    var isSelected = false;
    if (AlbumArt._mediaListView)
      isSelected = AlbumArt._mediaListView.selection.count > 0;
    this.changeImage(aNewURL,
                     albumArtSelectedImage,
                     albumArtNotSelectedBox,
                     albumArtSelectedDragBox,
                     stack,
                     isSelected);
  },
  
  /**
   * \brief changeNowPlaying - This function changes the Now Playing display
   *        to what is suppose to be displayed based on the image.
   */
  changeNowPlaying: function AlbumArt_changeNowPlaying(aNewURL) {
    // Load up our elements
    var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
    var albumArtNotPlayingBox = document.getElementById('sb-albumart-not-playing');
    var albumArtPlayingDragBox = document.getElementById('sb-albumart-playing-drag');
    var stack = document.getElementById('sb-albumart-nowplaying-stack');
    
    this.changeImage(aNewURL,
                     albumArtPlayingImage,
                     albumArtNotPlayingBox,
                     albumArtPlayingDragBox,
                     stack,
                     (this.getNowPlayingItem() != null));
  },

  /**
   * \brief observe - This is called when the dataremote value changes. We watch
   *        this so that we can respond to when the image is updated for an
   *        item even when already playing.
   * \param aSubject - not used.
   * \param aTopic   - key of data remote that changed.
   * \param aData    - new value of the data remote.
   */
  observe: function AlbumArt_observe(aSubject, aTopic, aData) {
    // Ignore any other topics (we should not normally get anything else)
    if (aTopic != "metadata.imageURL") {
      return;
    }
    
    if (aData == DROP_TARGET_IMAGE) {
      aData = "";
    }
    AlbumArt.changeNowPlaying(aData);
  },
  
  /**
   * \brief toggleView- This function will switch to the next deck, and if it is
   *        already at the last deck then it will switch to the first.
   */
  toggleView: function AlbumArt_toggleView() {
    var newState = (AlbumArt._currentState + 1);
    if (newState >= AlbumArt._stateInfo.length) {
      // Wrap around
      newState = 0;
    }
    AlbumArt.switchState(newState);
  },
  
  /**
   * \brief dpStateListener - Called when the display pane changes state
   */
  dpStateListener: function AlbumArt_onDisplayPaneStateListener(e) {
    if (!AlbumArt._displayPane)
      return;

    // the state is the state *before* it's changed
    if (e.target.getAttribute("state") == "open") {
      // it's collapsing, reset the contentTitle
      AlbumArt._displayPane.contentTitle = "";
    } else {
      AlbumArt.setPaneTitle();
    }
  },

  /**
   * \brief onServicepaneResize - Called when the servicepane is resized
   */
  onServicepaneResize: function AlbumArt_onServicepaneResize(e) {
    var imgEl;
    if (AlbumArt._currentState == STATE_PLAYING) {
      imgEl = AlbumArt._albumArtPlayingImage;
    } else {
      imgEl = AlbumArt._albumArtSelectedImage;
    }

    // Determine the new actual image dimensions
    var svgWidth = imgEl.width.baseVal.value;
    var svgHeight = parseInt(AlbumArt.imageDimensions.aspectRatio * svgWidth);
    if (Math.abs(svgHeight - AlbumArt._displayPane.height) > 10) {
      AlbumArt._animating = true;
      AlbumArt._finalAnimatedHeight = svgHeight;
      AlbumArt.animateHeight(AlbumArt._displayPane.height);
    } else {
      if (!AlbumArt._animating)
        AlbumArt._displayPane.height = svgHeight;
      else
        AlbumArt._finalAnimatedHeight = svgHeight;
    }
  },

  animateHeight: function AlbumArt_animateHeight(destHeight) {
    destHeight = parseInt(destHeight);
    AlbumArt._displayPane.height = destHeight;

    if (destHeight == AlbumArt._finalAnimatedHeight) {
      AlbumArt._animating = false;
    } else {
      var newHeight;
      var delta = Math.abs(AlbumArt._finalAnimatedHeight - destHeight) / 2;
      if (delta < 5)
        newHeight = AlbumArt._finalAnimatedHeight;
      else {
        if (AlbumArt._finalAnimatedHeight > destHeight)
          newHeight = destHeight + delta;
        else
          newHeight = destHeight - delta;
      }
      setTimeout(AlbumArt.animateHeight, 0, newHeight);
    }
  },
  /**
   * \brief onLoad - Called when the display pane loads, here we make sure that
   *        we have the correct image loaded.
   */
  onLoad: function AlbumArt_onLoad() {
    // Remove our loaded listener so we do not leak
    window.removeEventListener("DOMContentLoaded", AlbumArt.onLoad, false);

    // Get our displayPane
    var displayPaneManager = Cc["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
                                .getService(Ci.sbIDisplayPaneManager);
    var dpInstantiator = displayPaneManager.getInstantiatorForWindow(window);
 
    if (dpInstantiator) {
      AlbumArt._displayPane = dpInstantiator.displayPane;
    }
    var splitter = AlbumArt._displayPane._splitter;
    // Also remove the ability for this slider to be manually resized
    splitter.setAttribute("disabled", "true");
    splitter.addEventListener("displaypane-state", AlbumArt.dpStateListener,
                              false);

    // Get the mediacoreManager.
    AlbumArt._mediacoreManager =
      Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
        .getService(Ci.sbIMediacoreManager);

    // Load the previous selected display the user shutdown with
    AlbumArt._currentState = Application.prefs.getValue(PREF_STATE,
                                                        AlbumArt._currentState);
    if (AlbumArt._currentState == STATE_PLAYING) {
      document.getElementById("showNowPlayingMenuItem")
              .setAttribute("checked", "true");
    } else {
      document.getElementById("showNowSelectedMenuItem")
              .setAttribute("checked", "true");
    }
    
    // Ensure we have the correct title and deck displayed.
    AlbumArt.switchState(AlbumArt._currentState);
    
    // Setup the dataremote for the now playing image.
    var createDataRemote =  new Components.Constructor(
                                  "@songbirdnest.com/Songbird/DataRemote;1",
                                  Components.interfaces.sbIDataRemote,
                                  "init");
    AlbumArt._coverBind = createDataRemote("metadata.imageURL", null);
    AlbumArt._coverBind.bindObserver(AlbumArt, false);

    // Set the drag description contents
    document.getElementById('sb-albumart-select-drag').firstChild
            .textContent = SBString("albumart.displaypane.drag_artwork_here");
    document.getElementById('sb-albumart-playing-drag').firstChild
            .textContent = SBString("albumart.displaypane.drag_artwork_here");

    // Monitor track changes for faster image changing.
    AlbumArt._mediacoreManager.addListener(AlbumArt);

    var currentStatus = AlbumArt._mediacoreManager.status;
    var stopped = (currentStatus.state == currentStatus.STATUS_STOPPED ||
                   currentStatus.state == currentStatus.STATUS_UNKNOWN);
    if (stopped) {
      AlbumArt.changeNowPlaying(null);
      AlbumArt.changeNowSelected(null);
    }

    // Setup the Now Selected display
    AlbumArt.onTabContentChange();

    AlbumArt._albumArtSelectedImage =
            document.getElementById('sb-albumart-selected');
    AlbumArt._albumArtPlayingImage =
            document.getElementById('sb-albumart-playing');
    var mainWindow = window.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebNavigation)
                   .QueryInterface(Ci.nsIDocShellTreeItem)
                   .rootTreeItem
                   .QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindow);

    AlbumArt._servicePaneSplitter =
            mainWindow.document.getElementById("servicepane_splitter");
    AlbumArt._servicePaneSplitter.addEventListener("dragging",
                                                   AlbumArt.onServicepaneResize,
                                                   false);
  },
  
  /**
   * \brief onUnload - This is called when the display pane is closed. Here we
   *        can shut every thing down.
   */
  onUnload: function AlbumArt_onUnload() {
    // Remove our unload event listener so we do not leak
    window.removeEventListener("unload", AlbumArt.onUnload, false);
    AlbumArt._displayPane._splitter.removeEventListener("displaypane-state",
                                 AlbumArt.dpStateListener, false);

    // Remove servicepane resize listener
    AlbumArt._servicePaneSplitter.removeEventListener("dragging",
                                                   AlbumArt.onServicepaneResize,
                                                   false);

    // Undo our modifications to the spitter
    var splitter = AlbumArt._displayPane._splitter;
    splitter.removeAttribute("hovergrippy");
    splitter.removeAttribute("disabled");

    // Clear any auto-fetch timeouts.
    if (this._autoFetchTimeoutID) {
      clearTimeout(this._autoFetchTimeoutID)
      this._autoFetchTimeoutID = null;
    }

    // Clear the now selected media item
    AlbumArt.clearNowSelectedMediaItem();

    // Save the current display state for when the user starts again
    Application.prefs.setValue(PREF_STATE, AlbumArt._currentState);
                               
    AlbumArt._mediacoreManager.removeListener(AlbumArt);
    AlbumArt._mediacoreManager = null;
    
    AlbumArt._coverBind.unbind();
    AlbumArt._coverBind = null;

    // Remove selection listeners
    if(AlbumArt._mediaListView) {
      AlbumArt._mediaListView.selection.removeListener(AlbumArt);
    }
    if (AlbumArt._browser) {
      AlbumArt._browser.removeEventListener("TabContentChange", 
                                            AlbumArt.onTabContentChange,
                                            false);
    }
  },
  
  /**
   * \brief getMainBrowser - This function sets up the listener on the brower
   *        so we can be notified when the tab changes, then we can put another
   *        listener to the media list in view so we can be notified of the
   *        selection changes.
   */
  getMainBrowser: function AlbumArt_getMainBrowser() {
    // Get the main window, we have to do this because we are in a display pane
    // and our window is the content of the display pane.
    var windowMediator = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                          .getService(Components.interfaces.nsIWindowMediator);

    var songbirdWindow = windowMediator.getMostRecentWindow("Songbird:Main"); 
    // Store this so we don't have to get it every time.
    AlbumArt._browser = songbirdWindow.gBrowser;
    if (AlbumArt._browser) {
      AlbumArt._browser.addEventListener("TabContentChange", 
                                         AlbumArt.onTabContentChange,
                                         false);
    }
  },

  /**
   * \brief onTabContentChange - When the tab changes in the browser we need to
   *        grab the new mediaListView if there is one and attach a listener on
   *        it for selection. We must also be sure to remove any existing
   *        listeners so we do not leak.
   */
  onTabContentChange: function AlbumArt_onTabContentChange() {
    // Remove any existing listeners
    if(AlbumArt._mediaListView) {
      AlbumArt._mediaListView.selection.removeListener(AlbumArt);
    }

    // Make sure we have the browser
    if (!AlbumArt._browser) {
      AlbumArt.getMainBrowser();
    }
    AlbumArt._mediaListView = AlbumArt._browser.currentMediaListView;

    // Now attach a new listener for the selection changes
    if (AlbumArt._mediaListView) {
      AlbumArt._mediaListView.selection.addListener(AlbumArt);
      // Make an initial call so we can change our image based on selection
      // (but wait for the tab to stabilize)
      setTimeout(function() {
        AlbumArt.onSelectionChanged();
      }, 0);
    }
  },

  /**
   * \brief Check if the media item is a local file or not.
   * \param aMediaItem - sbIMediaItem to check
   * \return true if the media item is a local file.
   */
  checkIsLocal: function AlbumArt_checkIsLocal(aMediaItem) {
    var contentURL = aMediaItem.getProperty(SBProperties.contentURL);
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var uri = null;
    try {
      uri = ioService.newURI(contentURL, null, null);
    } catch (err) {
      Cu.reportError("AlbumArt: Unable to convert to URI: [" +
                     contentURL + "] " + err);
      return false;
    }
    
    return (uri instanceof Ci.nsIFileURL);
  },

  /**
   * \brief This will set the currently playing items image.
   * \param aNewImageUrl - new string url of file to set cover to on the item.
   */
  setNowPlayingCover: function AlbumArt_setNowPlayingCover(aNewImageUrl) {
    var aMediaItem = AlbumArt.getNowPlayingItem();
    if (!aMediaItem)
      return;
     
    aMediaItem.setProperty(SBProperties.primaryImageURL, aNewImageUrl);
    AlbumArt.changeNowPlaying(aNewImageUrl);
    
    // Only write the image if it is local
    if (this.checkIsLocal(aMediaItem)) {
      sbMetadataUtils.writeMetadata([aMediaItem],
                                    [SBProperties.primaryImageURL],
                                    window);
    }
  },

  /**
   * \brief This will set each items cover in the selection to the one supplied.
   * \param aNewImageUrl - new string url of file to set cover to on each item.
   */
  setSelectionsCover: function AlbumArt_setSelectionsCover(aNewImageUrl) {
    var selection = AlbumArt._mediaListView.selection;
    var multipleItems = false;
    
    // First check through all the items to see if they are the same or not
    // We determine that items are the same if they belong to the same album
    // and have the same AlbumArtist or ArtistName.
    var itemEnum = selection.selectedMediaItems;
    var oldAlbumName = null;
    var oldAlbumArtist = null;
    while (itemEnum.hasMoreElements()) {
      var item = itemEnum.getNext();
      var albumName = item.getProperty(SBProperties.albumName);
      var albumArtist = item.getProperty(SBProperties.albumArtistName);
      if (!albumArtist || albumArtist == "") {
        albumArtist = item.getProperty(SBProperties.artistName);
      }
      
      if (oldAlbumArtist == null &&
          oldAlbumName == null) {
        oldAlbumName = albumName;
        oldAlbumArtist = albumArtist;
      } else if (albumName != oldAlbumName ||
                 albumArtist != oldAlbumArtist) {
        // Not the same so flag and break
        multipleItems = true;
        break;
      }
    }
    
    if (multipleItems) {
      // Ask the user if they are sure they want to change the items selected
      // since they are different.
      const BYPASSKEY = "albumart.multiplewarning.bypass";
      const STRINGROOT = "albumart.multiplewarning.";
      if (!Application.prefs.getValue(BYPASSKEY, false)) {
        var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                              .getService(Ci.nsIPromptService);
        var check = { value: false };
                
        var strTitle = SBString(STRINGROOT + "title");
        var strMsg = SBString(STRINGROOT + "message");
        var strCheck = SBString(STRINGROOT + "check");
        
        var confirmOk = promptService.confirmCheck(window, 
                                                   strTitle, 
                                                   strMsg, 
                                                   strCheck, 
                                                   check);
        if (!confirmOk) {
          return;
        }
        if (check.value == true) {
          Application.prefs.setValue(BYPASSKEY, true);
        }
      }
    }
    
    // Now actually set the properties.  This will trigger a notification that
    // will update the currently selected display.
    var mediaItemArray = [];
    itemEnum = selection.selectedMediaItems;
    while (itemEnum.hasMoreElements()) {
      var item = itemEnum.getNext();
      var oldImage = item.getProperty(SBProperties.primaryImageURL);
      if (oldImage != aNewImageUrl) {
        item.setProperty(SBProperties.primaryImageURL, aNewImageUrl);
        if (this.checkIsLocal(item)) {
          mediaItemArray.push(item);
        }
      }
    }
    
    // Write the images to metadata
    if (mediaItemArray.length > 0) {
      sbMetadataUtils.writeMetadata(mediaItemArray,
                                    [SBProperties.primaryImageURL],
                                    window);
    }
  },

  /**
   * \brief This will set the appropriate items images to aNewImageUrl.
   * \param aNewImageUrl - new string url of file to set cover to.
   */
  setCurrentStateItemImage:  function AlbumArt_setCurrentSateItemImage(aNewImageUrl) {
    if (AlbumArt._currentState == STATE_SELECTED) {
      AlbumArt.setSelectionsCover(aNewImageUrl);
    } else {
      AlbumArt.setNowPlayingCover(aNewImageUrl);
    }
  },

  /**
   * \brief This will get the appropriate item image depending on current state.
   * \returns imageURL for currently displayed image, or null if none.
   */
  getCurrentStateItemImage: function AlbumArt_getCurrentStateItemImage() {
    var imageNode;
    if (AlbumArt._currentState == STATE_SELECTED) {
      imageNode = document.getElementById("sb-albumart-selected");
    } else {
      imageNode = document.getElementById("sb-albumart-playing");
    }
    return imageNode.getAttributeNS(XLINK_NS, "href");
  },

  /**
   * \brief This will clear the now selected media item and stop watching it for
   *        changes.
   */
  clearNowSelectedMediaItem: function AlbumArt_clearNowSelectedMediaItem() {
    // Stop watching the now selected media item
    if (AlbumArt._nowSelectedMediaItemWatcher) {
      AlbumArt._nowSelectedMediaItemWatcher.cancel();
      AlbumArt._nowSelectedMediaItemWatcher = null;
    }

    // Clear the now selected media item.
    AlbumArt._nowSelectedMediaItem = null;
  },

  /**
   * \brief This will get the now playing media item.
   * \return Now playing media item.
   */
  getNowPlayingItem: function AlbumArt_getNowPlayingItem() {
    // No playing item if the media core is stopped.
    var currentStatus = AlbumArt._mediacoreManager.status;
    var stopped = (currentStatus.state == currentStatus.STATUS_STOPPED ||
                   currentStatus.state == currentStatus.STATUS_UNKNOWN);
    if (stopped)
      return null;

    // Return the currently playing item.
    return AlbumArt._mediacoreManager.sequencer.currentItem;
  },

  /**
   * \brief Get album art from the clipboard.
   * \param aMimeType - Returned album art MIME type.  Null if clipboard does
   *                    not contain valid album art.
   * \param aImageData - Returned album art image data.  Null if clipboard does
   *                     not contain valid album art.
   * \param aIsValidAlbumArt - Returned true if clipboard contains valid album
   *                           art.
   */
  getClipboardAlbumArt:
    function AlbumArt_getClipboardAlbumArt(aMimeType,
                                           aImageData,
                                           aIsValidAlbumArt) {
    // Get the clipboard image.
    var sbClipboard = Cc["@songbirdnest.com/moz/clipboard/helper;1"]
                        .createInstance(Ci.sbIClipboardHelper);
    var mimeType = {};
    var imageData = null;
    try {
      imageData = sbClipboard.copyImageFromClipboard(mimeType, {});
    } catch (err) {
      Cu.reportError("Error checking for image data on the clipboard: " + err);
      aMimeType.value = null;
      aImageData.value = null;
      aIsValidAlbumArt.value = false;
      return;
    }
    mimeType = mimeType.value;

    // Validate image as valid album art.
    var isValidAlbumArt = false;
    if (imageData && (imageData.length > 0)) {
      var artService = Cc["@songbirdnest.com/Songbird/album-art-service;1"]
                         .getService(Ci.sbIAlbumArtService);
      isValidAlbumArt = artService.imageIsValidAlbumArt(mimeType,
                                                        imageData,
                                                        imageData.length);
    }
    if (!isValidAlbumArt) {
      mimeType = null;
      imageData = null;
    }

    // Return results.
    aMimeType.value = mimeType;
    aImageData.value = imageData;
    aIsValidAlbumArt.value = isValidAlbumArt;
  },

  /**
   * \brief Handle showing of the context menu popup.
   */
  onPopupShowing: function AlbumArt_onPopupShowing() {
    // Get the current state item image.
    var curImageUrl = AlbumArt.getCurrentStateItemImage();

    // Update the popup menu for the current state item image.
    var cutElem = document.getElementById("cutMenuItem");
    var copyElem = document.getElementById("copyMenuItem");
    var clearElem = document.getElementById("clearMenuItem");
    var pasteElem = document.getElementById("pasteMenuItem");
    var getArtworkElem = document.getElementById("getArtworkMenuItem");
    var getArtworkSeparatorElem = document.getElementById("getArtworkSeparator");

    // Hide the Get Artwork command if no "audio" is selected.
    var shouldHideGetArtwork = AlbumArt.shouldHideGetArtworkCommand();
    getArtworkSeparatorElem.hidden = shouldHideGetArtwork;
    getArtworkElem.hidden = shouldHideGetArtwork;

    // Always enable get artwork
    getArtworkElem.disabled = false;

    // Check if the clipboard contains valid album art.
    var validAlbumArt = {};
    AlbumArt.getClipboardAlbumArt({}, {}, validAlbumArt);
    validAlbumArt = validAlbumArt.value;

    // Do not allow copying the drop target image
    if (curImageUrl == DROP_TARGET_IMAGE) {
      cutElem.disabled = true;
      copyElem.disabled = true
      clearElem.disabled = true;
    } else {
      // Only allow valid images to be modified.
      cutElem.disabled = !curImageUrl;
      copyElem.disabled = !curImageUrl;
      clearElem.disabled = !curImageUrl;
    }

    // Enable the paste if a valid image is on the clipboard
    // Disable the paste if no "audio" is selected.
    pasteElem.disabled = !validAlbumArt || shouldHideGetArtwork;

  },
  
  shouldHideGetArtworkCommand: function AlbumArt_shouldHideGetArtworkCommand() {
    if (AlbumArt._currentState == STATE_SELECTED) {
      // Now Selected
      var items = AlbumArt._mediaListView.selection.selectedMediaItems;
      while (items.hasMoreElements()) {
        var item = items.getNext().QueryInterface(Ci.sbIMediaItem);
        if (item.getProperty(SBProperties.contentType) == "audio") {
          return false;
        }
      }
      return true;
    }
    else {
      // Now playing
      var item = AlbumArt.getNowPlayingItem();
      if (item && item.getProperty(SBProperties.contentType) == "audio") {
        return false;
      }
      return true;
    }
  },

  /**
   * \brief Automatically fetch artwork for the currently displayed item if
   *        needed.
   */
  autoFetchArtwork: function AlbumArt_autoFetchArtwork() {
    // Ensure no auto-fetch timeout is pending.
    if (this._autoFetchTimeoutID) {
      clearTimeout(this._autoFetchTimeoutID)
      this._autoFetchTimeoutID = null;
    }

    // Delay the start of auto-fetching for a second.
    var _this = this;
    var func = function() {
      _this._autoFetchTimeoutID = null;
      _this._autoFetchArtwork();
    }
    this._autoFetchTimeoutID = setTimeout(func, 1000);
  },

  _autoFetchArtwork: function AlbumArt__autoFetchArtwork() {
    // Determine the item for which artwork should be fetched.
    var item = null;
    if (AlbumArt._currentState == STATE_PLAYING) {
      item = this.getNowPlayingItem();
    }
    else {
      if (AlbumArt._mediaListView)
        item = AlbumArt._mediaListView.selection.currentMediaItem;
    }

    // Do nothing if item has not changed.
    if (item == AlbumArt._prevAutoFetchArtworkItem)
      return;
    AlbumArt.prevAutoFetchArtworkItem = item;

    // Do nothing more if no item.
    if (!item)
      return;

    // Auto-fetch if item does not have an image and a remote artwork fetch has
    // not yet been attempted.
    var primaryImageURL = item.getProperty(SBProperties.primaryImageURL);
    var attemptedRemoteArtFetch =
          item.getProperty(SBProperties.attemptedRemoteArtFetch);
    if (!primaryImageURL && !attemptedRemoteArtFetch) {
      var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                  .createInstance(Ci.nsISupportsInterfacePointer);
      sip.data = ArrayConverter.nsIArray([item]);
      sbCoverHelper.getArtworkForItems(sip.data, window, item.library, true);
    }
  },

  /*********************************
   * Drag and Drop
   ********************************/
  /**
   * \brief Gets the flavours we support for drag and drop. This is called from
   *        the onDragOver to determine if we support what is being dragged.
   * \returns FlavourSet of flavours supported.
   */
  getSupportedFlavours : function AlbumArt_getSupportedFlavours() {
    // Disable drag and drop if artwork is not supported
    var shouldHideGetArtwork = AlbumArt.shouldHideGetArtworkCommand();
    if (shouldHideGetArtwork)
      return null;

    var flavours = new FlavourSet();
    return sbCoverHelper.getFlavours(flavours);
  },
  
  /**
   * \brief handles something being dropped on our display pane.
   * \param aEvent    - Event of drag and drop session
   * \param aDropata  - Data of what has been dropped.
   * \param aSesssion - Drag and drop session.
   */
  onDrop: function AlbumArt_onDrop(aEvent, aDropData, aSession) {
    var self = this;
    sbCoverHelper.handleDrop(function (newFile) {
      if (newFile) {
        self.setCurrentStateItemImage(newFile);
      }
    }, aDropData);
  },

  /**
   * \brief this is here to satisfy the nsDragAndDrop.onDragOver function.
   * \param aEvent    - Event of drag and drop session
   * \param aFlavour  - Flavour of what is being dragged over us.
   * \param aSesssion - Drag and drop session.
   */
  onDragOver: function AlbumArt_onDragOver(aEvent, aFlavour, aSesssion) {
    // No need to do anything here, for UI we should set the
    // #sb-albumart-selected:-moz-drag-over style.
  },
  
  /**
   * \brief Handles setting up the data for a drag session..
   * \param aEvent        - Event of drag and drop session
   * \param aTransferData - Our data to as a nsITransfer.
   * \param aAction       - The type of drag action (Copy, Move or Link).
   */
  onDragStart: function AlbumArt_onDragStart(aEvent, aTransferData, aAction) {
    var imageURL = AlbumArt.getCurrentStateItemImage();
    aTransferData.data = new TransferData();
    sbCoverHelper.setupDragTransferData(aTransferData, imageURL);
  },


  /*********************************
   * Copy, Cut, Paste, Clear menu
   ********************************/
  /**
   * \brief Paste an image to the item that is either selected or playing.
   */
  onPaste: function AlbumArt_onPaste() {
    var mimeType = {};
    var imageData = {};
    AlbumArt.getClipboardAlbumArt(mimeType, imageData, {});
    mimeType = mimeType.value;
    imageData = imageData.value;
    if (imageData && sbCoverHelper.isImageSizeValid(null, imageData.length)) {
      var artService = Cc["@songbirdnest.com/Songbird/album-art-service;1"]
                         .getService(Ci.sbIAlbumArtService);

      var newURI = artService.cacheImage(mimeType,
                                         imageData,
                                         imageData.length);
      if (newURI) {
        AlbumArt.setCurrentStateItemImage(newURI.spec);
      }
    }
  },
  
  /**
   * \brief Copy the currently displayed image to the clipboard.
   */
  onCopy: function AlbumArt_onCopy() {
    var sbClipboard = Cc["@songbirdnest.com/moz/clipboard/helper;1"]
                        .createInstance(Ci.sbIClipboardHelper);
    var aImageURL = AlbumArt.getCurrentStateItemImage();
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var imageURI = null;
    try {
      imageURI = ioService.newURI(aImageURL, null, null);
    } catch (err) {
      Cu.reportError("albumArtPane: Unable to convert to URI: [" + aImageURL +
                     "] " + err);
      return false;
    }
    
    var copyOk = false;
    if (imageURI instanceof Ci.nsIFileURL) {
      var imageFile = imageURI.file.QueryInterface(Ci.nsILocalFile);
      var imageData, mimeType;
      [imageData, mimeType] = sbCoverHelper.readImageData(imageFile);
    
      try {
        sbClipboard.pasteImageToClipboard(mimeType, imageData, imageData.length);
        copyOk = true;
      } catch (err) {
        Cu.reportError("albumArtPane: Unable to paste to the clipboard " + err);
      }
    }
    
    return copyOk;
  },
  
  /**
   * \brief Copy the currently displayed image to the clipboard and if
   *        sucessful then remove the image from the playing or selected items.
   */
  onCut: function AlbumArt_onCut() {
    if (this.onCopy()) {
      this.onClear();
    }
  },

  /**
   * \brief Scan items for missing artwork.
   */
  onGetArtwork: function AlbumArt_onGetArtwork() {
    if (AlbumArt._currentState == STATE_SELECTED) {
      // Now Selected
      var library = AlbumArt._mediaListView.mediaList.library;

      // Only look up artwork for audio items.
      var isAudioItem = function(aElement) {
        return aElement.getProperty(SBProperties.contentType) == "audio";
      };
      var selectedAudioItems = new SBFilteredEnumerator(
          AlbumArt._mediaListView.selection.selectedMediaItems, 
          isAudioItem);
      
      // We need to convert our JS object into an XPCOM object.
      // Mook claims this is actually the best way to do this.
      var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                      .createInstance(Ci.nsISupportsInterfacePointer);
      sip.data = selectedAudioItems;
      selectedAudioItems = sip.data;

      sbCoverHelper.getArtworkForItems(selectedAudioItems,
                                       window,
                                       library);
    } else {
      // Now playing
      var item = AlbumArt.getNowPlayingItem();
      if (item.getProperty(SBProperties.contentType != "audio")) {
        Components.utils.reportError("albumArtPane.js::onGetArtwork: \n"+
         "shouldn't try to get album art for non-audio items.");
        return;
      }
      if (item) {
        // damn it I hate this whole stupid thing where each JS global gets a
        // completely _different_ set of global classes
        var sip = Cc["@mozilla.org/supports-interface-pointer;1"]
                    .createInstance(Ci.nsISupportsInterfacePointer);
        sip.data = ArrayConverter.nsIArray([item]);
        sbCoverHelper.getArtworkForItems(sip.data, window, item.library);
      }
    }
  },


  /**
    * \brief Clear the image from the playing or selected items.
   */
  onClear: function AlbumArt_onClear() {
    AlbumArt.setCurrentStateItemImage("");
  },

  /*********************************
   * sbIMediaListViewSelectionListener
   ********************************/
  /**
   * \brief The current selection has changed so we need to update the now
   *        selected image.
   */
  onSelectionChanged: function AlbumArt_onSelectionChanged() {
    // Get the new now selected media item
    var selection = AlbumArt._mediaListView.selection;
    var curImageUrl = null;
    var item = selection.currentMediaItem;
    
    // Clear the old now selected media item
    AlbumArt.clearNowSelectedMediaItem();

    // Watch new now selected media item for primary image URL changes
    if (item) {
      AlbumArt._nowSelectedMediaItem = item;
      AlbumArt._nowSelectedMediaItemWatcher =
                 Cc["@songbirdnest.com/Songbird/Library/MediaItemWatcher;1"]
                   .createInstance(Ci.sbIMediaItemWatcher);
      var filter = SBProperties.createArray([ [ SBProperties.primaryImageURL,
                                                null ] ]);
      AlbumArt._nowSelectedMediaItemWatcher.watch(item, this, filter);
    }

    // Get the now selected media item image.  Do this after adding watcher to
    // ensure changes are always picked up.
    if (item) {
      curImageUrl = item.getProperty(SBProperties.primaryImageURL);
    }

    AlbumArt.changeNowSelected(curImageUrl);
  },

  /**
   * \brief The current index has changed in the selection, we do not do
   *        anything with this.
   */
  onCurrentIndexChanged: function AlbumArt_onCurrentIndexChanged() {
  },

  /*********************************
   * sbIMediaItemListener
   ********************************/
  /**
   * \brief Called when a media item is removed from its library.
   * \param aMediaItem The removed media item.
   */
  onItemRemoved: function AlbumArt_onItemRemoved(aMediaItem) {
  },

  /**
   * \brief Called when a media item is changed.
   * \param aMediaItem The item that has changed.
   */
  onItemUpdated: function AlbumArt_onItemUpdated(aMediaItem) {
    // Update now selected media item image
    var curImageUrl =
          this._nowSelectedMediaItem.getProperty(SBProperties.primaryImageURL);
    AlbumArt.changeNowSelected(curImageUrl);
  },

  /*********************************
   * sbIMediacoreEventListener and Event Handlers
   ********************************/
  onMediacoreEvent: function AlbumArt_onMediacoreEvent(aEvent) {
    switch(aEvent.type) {
      case Ci.sbIMediacoreEvent.STREAM_END:
      case Ci.sbIMediacoreEvent.STREAM_STOP:
        AlbumArt.onStop();
      break;
      
      case Ci.sbIMediacoreEvent.TRACK_CHANGE:
        AlbumArt.onBeforeTrackChange(aEvent.data);
      break;
    }
  },
  /**
   * \brief The playback has stopped so clear the now playing.
   */
  onStop: function AlbumArt_onStop() {
    // Basicly clear the now playing image
    AlbumArt.changeNowPlaying(null);
  },
  /**
   * \brief A new track is going to play so update the now playing.
   * \param aItem - Item that is going to play.
   */
  onBeforeTrackChange: function AlbumArt_onBeforeTrackChange(aItem) {
    var newImageURL = aItem.getProperty(SBProperties.primaryImageURL);
    AlbumArt.changeNowPlaying(newImageURL);
  },
  /**
   * \brief The index of the playing track has changed.
   */
  onTrackIndexChange: function AlbumArt_onTrackIndexChange(aItem, aView, aIndex) { },
  /**
   * \brief We are changing to a new view.
   */
  onBeforeViewChange: function AlbumArt_onBeforeViewChange(aView) { },
  /**
   * \brief We have changed views.
   */
  onViewChange: function AlbumArt_onViewChange(aView) { },
  /**
   * \brief A track has started to play.
   */
  onTrackChange: function AlbumArt_onTrackChange(aItem, aView, aIndex) { },

  /*********************************
   * nsISupports
   ********************************/
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediacoreEventListener,
                                         Ci.sbIMediaListViewSelectionListener])
};

// We need to use DOMContentLoaded instead of load here because of the
// Mozilla Bug #420815
window.addEventListener("DOMContentLoaded", AlbumArt.onLoad, false);
window.addEventListener("unload", AlbumArt.onUnload, false);
