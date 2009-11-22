/*
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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

  // Array of state information for each display (selected/playing/etc)
  _stateInfo: [ { imageID: "sb-albumart-selected",
                  titleID: "selected",
                  defaultTitle: "Now Selected"
                },
                { imageID: "sb-albumart-playing",
                  titleID: "playing",
                  defaultTitle: "Now Playing"
                }
              ],
  _currentState: STATE_SELECTED, // Default to now selected state (display)


  /**
   * \brief makeTitleBarButton - This modifies the display pane title bar so
   *        that it acts like a button to toggle between the different displays.
   */
  makeTitleBarButton: function AlbumArt_makeTitleBarButton() {
    if (AlbumArt._displayPane) {
      // Make the title bar of the display pane act like a button
      AlbumArt._displayPane.makeClickableTitleBar(AlbumArt.titleClick);
    }
  },
  
  /**
   * \brief titleClick - This handles the event when the user clicks the title
   *        bar of the display pane, for primary button clicks we toggle the
   *        view.
   * \param aEvent - is the event of this click.
   */
  titleClick: function AlbumArt_titleClick(aEvent) {
    if (aEvent.button == 0) {
      AlbumArt.toggleView();
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

    // Update the display-pane title button tooltip
    var tooltip = "";
    if (AlbumArt._currentState == STATE_SELECTED) {
      // tooltip should show going to now playing
      tooltip = SBString("albumart.displaypane.tooltip.playing");
    } else {
      // tooltip should show going to selected track
      tooltip = SBString("albumart.displaypane.tooltip.selected");
    }
    AlbumArt._displayPane.titlebarTooltipText = tooltip;
  },

  /**
   * \brief isPaneSquare - This function ensures that the height of the
   *        display pane is equal to the width. This will resize the pane if
   *        it is not square.
   * \return False if the pane is not square, True if it is. 
   */
  isPaneSquare: function AlbumArt_isPaneSquare() {
    // Window is our displaypane, so grab the size
    var windowWidth = window.innerWidth;
    var windowHeight = window.innerHeight;

    if (windowHeight != windowWidth) {
      // First determine if we are in the service pane display pane
      if (AlbumArt._displayPane &&
          AlbumArt._displayPane.contentGroup == "servicepane") {
        // Account for the displayPane header.
        var displayPaneHeaderSize = AlbumArt._displayPane.height - windowHeight;
        AlbumArt._displayPane.height = windowWidth + displayPaneHeaderSize;
        // Indicate that we have resized the pane.
        return false;
      }
    }
    return true;
  },
  
  /**
   * \brief onResize - This function is called when either an image loads or
   *        the display pane is resized.
   */
  onResize: function AlbumArt_onResize() {
    // Ensure the display pane content is square.
    AlbumArt.isPaneSquare();
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
    } else if (!aNewURL) {
      // We are playing or have selected items, but the image is not available

      // Show the Drag Here box
      aDragBox.hidden = false;

      // Hide the not selected message.
      aNotBox.hidden = true;

      // Set the image to the default cover
      aNewURL = DROP_TARGET_IMAGE;
    } else {
      // We are playing or have selected items, and we have a valid image

      // Hide the not selected/playing message.
      aNotBox.hidden = true;

      // Hide the Drag Here box
      aDragBox.hidden = true;
    }

    /* Set the image element URL */
    if (aNewURL)
      aImageElement.setAttributeNS(XLINK_NS, "href", aNewURL);
    else
      aImageElement.removeAttributeNS(XLINK_NS, "href");
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
    
    this.changeImage(aNewURL,
                     albumArtSelectedImage,
                     albumArtNotSelectedBox,
                     albumArtSelectedDragBox,
                     (AlbumArt.getSelection().count > 0));
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
    
    this.changeImage(aNewURL,
                     albumArtPlayingImage,
                     albumArtNotPlayingBox,
                     albumArtPlayingDragBox,
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

    // Get the mediacoreManager.
    AlbumArt._mediacoreManager =
      Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
        .getService(Ci.sbIMediacoreManager);

    // Load the previous selected display the user shutdown with
    AlbumArt._currentState = Application.prefs.getValue(PREF_STATE,
                                                        AlbumArt._currentState);
    
    // Ensure we have the correct title and deck displayed.
    AlbumArt.switchState(AlbumArt._currentState);
    
    // Make sure we are square (this will resize)
    AlbumArt.isPaneSquare();

    // Setup the dataremote for the now playing image.
    var createDataRemote =  new Components.Constructor(
                                  "@songbirdnest.com/Songbird/DataRemote;1",
                                  Components.interfaces.sbIDataRemote,
                                  "init");
    AlbumArt._coverBind = createDataRemote("metadata.imageURL", null);
    AlbumArt._coverBind.bindObserver(AlbumArt, false);

    // Monitor track changes for faster image changing.
    AlbumArt._mediacoreManager.addListener(AlbumArt);

    var currentStatus = AlbumArt._mediacoreManager.status;
    var stopped = (currentStatus.state == currentStatus.STATUS_STOPPED ||
                   currentStatus.state == currentStatus.STATUS_UNKNOWN);
    if (stopped) {
      AlbumArt.changeNowPlaying(null);
    }

    // Listen for resizes of the display pane so that we can keep the aspect
    // ratio of the image.
    window.addEventListener("resize",
                            AlbumArt.onResize,
                            false);
    
    // Make the title bar of the display pane act like a button so we can
    // toggle between Now Playing and Now Selected views.
    AlbumArt.makeTitleBarButton();

    // Setup the Now Selected display
    AlbumArt.onTabContentChange();
  },
  
  /**
   * \brief onUnload - This is called when the display pane is closed. Here we
   *        can shut every thing down.
   */
  onUnload: function AlbumArt_onUnload() {
    // Remove our unload event listener so we do not leak
    window.removeEventListener("unload", AlbumArt.onUnload, false);

    // Clear the now selected media item
    AlbumArt.clearNowSelectedMediaItem();

    // Save the current display state for when the user starts again
    Application.prefs.setValue(PREF_STATE, AlbumArt._currentState);
                               
    AlbumArt._mediacoreManager.removeListener(AlbumArt);
    AlbumArt._mediacoreManager = null;
    
    AlbumArt._coverBind.unbind();
    AlbumArt._coverBind = null;

    window.removeEventListener("resize",
                               AlbumArt.onResize,
                               false);

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
      var propArray = ArrayConverter.stringEnumerator([SBProperties.primaryImageURL]);
      var metadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);      
      try {
        var job = metadataService.write(ArrayConverter.nsIArray([aMediaItem]),
                                        propArray);
      
        SBJobUtils.showProgressDialog(job, window);
      } catch (e) {
        // Job will fail if writing is disabled by the pref
        Components.utils.reportError(e);
      }
    }
  },

  /**
   * \brief This will set each items cover in the selection to the one supplied.
   * \param aNewImageUrl - new string url of file to set cover to on each item.
   */
  setSelectionsCover: function AlbumArt_setSelectionsCover(aNewImageUrl) {
    var selection = AlbumArt.getSelection();
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
    var mediaItemArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                        .createInstance(Ci.nsIMutableArray);
    itemEnum = selection.selectedMediaItems;
    while (itemEnum.hasMoreElements()) {
      var item = itemEnum.getNext();
      var oldImage = item.getProperty(SBProperties.primaryImageURL);
      if (oldImage != aNewImageUrl) {
        item.setProperty(SBProperties.primaryImageURL, aNewImageUrl);
        if (this.checkIsLocal(item)) {
          mediaItemArray.appendElement(item, false);
        }
      }
    }
    
    // Write the images to metadata
    if (mediaItemArray.length > 0) {
      var propArray = ArrayConverter.stringEnumerator([SBProperties.primaryImageURL]);
      var metadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);      
      try {
        var job = metadataService.write(mediaItemArray, propArray);
      
        SBJobUtils.showProgressDialog(job, window);
      } catch (e) {
        // Job will fail if writing is disabled by the pref
        Components.utils.reportError(e);
      }
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
   * \brief This will get the current media item selection.
   * \return Current media item selection.
   */
  getSelection: function AlbumArt_getSelection() {
    // Get the media list view selection.
    var selection = AlbumArt._mediaListView.selection;

    // If nothing is selected, and the current media list is an album media
    // list, return a selection of all media list view items.
    if (selection.count == 0) {
      var mediaList = AlbumArt._mediaListView.mediaList;
      if (mediaList.getProperty(SBProperties.isAlbum) == "1") {
        var mediaListViewClone = AlbumArt._mediaListView.clone();
        mediaListViewClone.selection.selectAll();
        if (mediaListViewClone.selection.count > 0)
          mediaListViewClone.selection.currentIndex = 0;
        return mediaListViewClone.selection;
      }
    }

    return selection;
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
    pasteElem.disabled = !validAlbumArt;

  },
  
  shouldHideGetArtworkCommand: function AlbumArt_shouldHideGetArtworkCommand() {
    if (AlbumArt._currentState == STATE_SELECTED) {
      // Now Selected
      var items = AlbumArt.getSelection().selectedMediaItems;
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
      if (item && item.getProperty(SBProperties.contentType == "audio")) {
        return false;
      }
      return true;
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
          AlbumArt.getSelection().selectedMediaItems, 
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
    var selection = AlbumArt.getSelection();
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
