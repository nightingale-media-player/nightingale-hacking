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

if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/sbCoverHelper.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

// Display pane constants
const DISPLAY_PANE_CONTENTURL = "chrome://albumart/content/albumArtPane.xul";
const DISPLAY_PANE_ICON       = "chrome://albumart/skin/icon-albumart.png";

// Default cover for items missing the cover
const DEFAULT_COVER = "chrome://songbird/skin/album-art/default-cover.png";

// Constants for toggling between Selected and Playing states
// These releate to the deck
const STATE_SELECTED = 0;
const STATE_PLAYING  = 1;

// Preferences
const PREF_STATE = "songbird.albumart.displaypane.view";

/******************************************************************************
 *
 * \class AlbumArt 
 * \brief Controller for the Album Art Pane.
 *
 *****************************************************************************/
var AlbumArt = {
  _coverBind: null,                 // Data remote for the now playing image.
  _playListPlaybackService: null,   // Get notifications of track changes.
  _mediaListView: null,             // Current active mediaListView.
  _browser: null,                   // Handle to browser for tab changes.
  _displayPane: null,               // Display pane we are in.

  // Array of state information for each display (selected/playing/etc)
  _stateInfo: [ { imageID: "sb-albumart-selected",
                  titleID: "selected",
                  defaultTitle: "Now Selected",
                  lastImageSrc: "",
                  imageWidth: 0,
                  imageHeight: 0
                },
                { imageID: "sb-albumart-playing",
                  titleID: "playing",
                  defaultTitle: "Now Playing",
                  lastImageSrc: "",
                  imageWidth: 0,
                  imageHieght: 0
                }
              ],
  _currentState: STATE_SELECTED, // Default to now selected state (display)


  /**
   * \brief makeTitleBarButton - This modifies the display pane title bar so
   *        that it acts like a button to toggle between the different displays.
   */
  makeTitleBarButton: function() {
    if (AlbumArt._displayPane) {
      // Make the title bar of the display pane act like a button
      AlbumArt._displayPane.makeClickableTitleBar(AlbumArt.titleClick);
    }
  },
  
  /**
   * \brief titleClick - The handles the event when the user clicks the title
   *        bar of the display pane, for primary button clicks we toggle the
   *        view.
   */
  titleClick: function(event) {
    if (event.button == 0) {
      AlbumArt.toggleView();
    }
  },
  
  /**
   * \brief onImageDblClick - This function is called when the user double
   *        clicks the image.
   * \param aEvent event object of the current event.
   */
  onImageDblClick: function (aEvent) {
    // Only respond to primary button double clicks.
    if (aEvent.button == 0) {
      // This will load the songbird.metadata.imageURL preference to
      // determine which image to load.
      SBOpenModalDialog("chrome://albumart/content/coverPreview.xul",
                   "coverPreview",
                   "all,chrome,resizable=no,centerscreen",
                   null,
                   null);
    }
  },

  /**
   * \brief setPanetitle - This function updates the pane title depending on
   *        the current state.
   */
  setPaneTitle: function() {
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
   * \param aNewState is the index of the deck to make active.
   */
  switchState: function (aNewState) {
    AlbumArt._currentState = aNewState;
    var albumArtDeck = document.getElementById('sb-albumart-deck');
    albumArtDeck.selectedIndex = AlbumArt._currentState;
    AlbumArt.setPaneTitle();
  },

  /**
   * \brief resizeImage - This function will take care of properly resizing an
   *        image to keep the proper aspect ratio when our display pane resizes.
   *        We use a cache because currently <image> tags do not hold the size
   *        information as we need it.
   * \param aCacheIndex - An index to which image to resize, this will generally
   *        be linked to the deck index, 0 = Selected for example.
   */
  resizeImage: function (aCacheIndex) {
    var imageCache = AlbumArt._stateInfo[aCacheIndex];
    var mImage = document.getElementById(imageCache.imageID);
    var currentSrc = mImage.getAttribute("src");
    
    // Do not do anything if we don't have an image to resize
    if (currentSrc == "") {
      imageCache.lastImageSrc = "";
      return;
    }

    // See if we have already cached this information
    if (currentSrc != imageCache.lastImageSrc) {
      // We do this to get the proper height and width, since it will not be
      // resized when loaded with the Image() object. The <image> elements
      // change the height and width when the image is resized to fit the
      // content.
      var newImg = new Image();
      newImg.src = currentSrc;
      imageCache.imageHeight = newImg.height;
      imageCache.imageWidth = newImg.width;
    }

    // Do not do anything with invalid images.
    if (imageCache.imageWidth <= 0 || imageCache.imageHeight <= 0) {
      return;
    }
    
    // Window is our displaypane, so grab the size
    var windowWidth = window.innerWidth;
    var windowHeight = window.innerHeight;
    
    // Default to the size of the image
    var newWidth = imageCache.imageWidth;
    var newHeight = imageCache.imageHeight;
    
    // Only resize if the image is bigger than the display pane (window)
    if ( (newWidth > windowWidth) ||
         (newHeight > windowHeight) ) {
      // Figure out the ratios of the image and window.
      var windowRatio = windowWidth / windowHeight;
      var imageRatio = imageCache.imageWidth / imageCache.imageHeight;
      
      // If windowRatio is greater than the imageRatio then we want to resize
      // the height, otherwize we resize the width to keep the aspect ratio.
      // We also do not want to make the image bigger than it actually is.
      if (imageRatio >= windowRatio) {
        newWidth = (windowWidth <= imageCache.imageWidth ?
                      windowWidth : imageCache.imageWidth);
        newHeight = (imageCache.imageHeight * newWidth / imageCache.imageWidth);
      } else {
        newHeight = (windowHeight <= imageCache.imageHeight ?
                      windowHeight : imageCache.imageHeight);
        newWidth = (imageCache.imageWidth * newHeight / imageCache.imageHeight);
      }
    }

    mImage.style.width = newWidth + "px";
    mImage.style.height = newHeight + "px";
  },

  /**
   * \brief isPaneSquare - This function ensures that the height of the
   *        display pane is equal to the width. This will resize the pane if
   *        it is not square.
   * \return False if the pane is not square, True if it is. 
   */
  isPaneSquare: function () {
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
  onResize: function () {
    if (!AlbumArt.isPaneSquare()) {
      // Abort because this function will be called again.
      return;
    }

    // Resize each image in our deck even if not shown so that we don't have
    // to resize every time the user switches the display.
    for (var cIndex = 0; cIndex < AlbumArt._stateInfo.length; cIndex++) {
      AlbumArt.resizeImage(cIndex);
    }
  },

  /**
   * \brief changeNowSelected - This function changes the Now Selected display
   *        to what is suppose to be displayed based on the image.
   */
  changeNowSelected: function(aNewURL) {
    // Load up our elements
    var albumArtSelectedImage = document.getElementById('sb-albumart-selected');
    var albumArtNotSelectedBox = document.getElementById('sb-albumart-not-selected');

    if (aNewURL == albumArtSelectedImage.getAttribute("src")) {
      // Nothing changed so leave as is.
      return;
    }

    // Configure the display pane
    if (!aNewURL) {
      // Show the not playing message.
      albumArtNotSelectedBox.removeAttribute("hidden");
    } else {
      // Hide the not playing message.
      albumArtNotSelectedBox.setAttribute("hidden", true);
    }
    albumArtSelectedImage.setAttribute("src", aNewURL);
    // Call the onResize so we display the image correctly.
    AlbumArt.onResize();
  },

  /**
   * \brief observe - This is called when the dataremote value changes. We watch
   *        this so that we can respond to when the image is updated for an
   *        item even when already playing.
   * \param aSubject - not used.
   * \param aTopic   - key of data remote that changed.
   * \param aData    - new value of the data remote.
   */
  observe: function ( aSubject, aTopic, aData ) {
    // Ignore any other topics (we should not normally get anything else)
    if (aTopic != "metadata.imageURL") {
      return;
    }
    
    // Load up our elements
    var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
    var albumArtNotPlayingBox = document.getElementById('sb-albumart-not-playing');
    
    // This function can be called several times so check that we changed.
    if (albumArtPlayingImage.getAttribute("src") == aData) {
      return;
    }

    // Configure the display pane
    if (!aData || aData == "") {
      // Show the not playing message.
      albumArtNotPlayingBox.removeAttribute("hidden");
    } else {
      // Hide the not playing message.
      albumArtNotPlayingBox.setAttribute("hidden", true);
    }
    albumArtPlayingImage.setAttribute("src", aData);
    AlbumArt.onResize();
  },
  
  /**
   * \brief toggleView- This function will switch to the next deck, and if it is
   *        already at the last deck then it will switch to the first.
   */
  toggleView: function () {
    var newState = (AlbumArt._currentState + 1);
    if (newState >= AlbumArt._stateInfo.length) {
      // Wrap around
      newState = 0;
    }
    AlbumArt.switchState(newState);
  },
  
  onTabSelect: function(aTabIndex) {
    AlbumArt.switchState(aTabIndex);
  },
  
  /**
   * \brief onLoad - Called when the display pane loads, here we make sure that
   *        we have the correct image loaded or we display the Nothing Playing
   *        message.
   */
  onLoad: function () {
    // Remove our loaded listener so we do not leak
    window.removeEventListener("DOMContentLoaded", AlbumArt.onLoad, false);

    // Get our displayPane
    var displayPaneManager = Cc["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
                                .getService(Ci.sbIDisplayPaneManager);
    var dpInstantiator = displayPaneManager.getInstantiatorForWindow(window);
 
    if (dpInstantiator) {
      AlbumArt._displayPane = dpInstantiator.displayPane;
    }

 
    // Load the previous selected display the user shutdown with
    AlbumArt._currentState = Application.prefs.getValue(PREF_STATE,
                                                        AlbumArt._currentState);
    
    // Ensure we have the correct title and deck displayed.
    AlbumArt.switchState(AlbumArt._currentState);
    
    // Do an initial load of the default cover since it is a big image, and then
    // set the width and height of the images to the width and height of the
    // window so the image does not stretch out of bounds on the first load.
    AlbumArt.isPaneSquare(); // Make sure we are square (this will resize)
    var newImg = new Image();
    newImg.src = DEFAULT_COVER;
    var windowWidth = window.innerWidth;
    var windowHeight = window.innerHeight;
    for (var cIndex = 0; cIndex < AlbumArt._stateInfo.length; cIndex++) {
      var imageCache = AlbumArt._stateInfo[cIndex];
      
      // Listen to when an image loads so we can keep the aspect ratio.
      var mImage = document.getElementById(imageCache.imageID);
      mImage.addEventListener("load", AlbumArt.onResize, false);
      mImage.style.width = windowWidth + "px";
      mImage.style.height = windowHeight + "px";
    }

    // Setup the dataremote for the now playing image.
    var createDataRemote =  new Components.Constructor(
                                  "@songbirdnest.com/Songbird/DataRemote;1",
                                  Components.interfaces.sbIDataRemote,
                                  "init");
    AlbumArt._coverBind = createDataRemote("metadata.imageURL", null);
    AlbumArt._coverBind.bindObserver(AlbumArt, false);

    // Load the playListPlaybackService so we can monitor track changes for
    // faster image changing.
    AlbumArt._playListPlaybackService = 
                          Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                            .getService(Ci.sbIPlaylistPlayback);
    AlbumArt._playListPlaybackService.addListener(AlbumArt);

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
  onUnload: function () {
    // Remove our unload event listener so we do not leak
    window.removeEventListener("unload", AlbumArt.onUnload, false);

    // Save the current display state for when the user starts again
    Application.prefs.setValue(PREF_STATE, AlbumArt._currentState);
                               
    AlbumArt._playListPlaybackService.removeListener(AlbumArt);
    AlbumArt._playListPlaybackService = null;
    
    AlbumArt._coverBind.unbind();
    AlbumArt._coverBind = null;
    
    // Remove the load listeners
    for (var cIndex = 0; cIndex < AlbumArt._stateInfo.length; cIndex++) {
      var imageCache = AlbumArt._stateInfo[cIndex];
      var mImage = document.getElementById(imageCache.imageID);
      mImage.removeEventListener("load", AlbumArt.onResize, false);
    }
    
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
  getMainBrowser: function() {
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
  onTabContentChange: function() {
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
      AlbumArt.onSelectionChanged();
    }
  },

  /**
   * \brief This will set each items cover in the selection to the one supplied.
   * \param aImageFile - new string url of file to set cover to on each item.
   */
  setSelectionsCover: function(aImageFile) {
    var selection = AlbumArt._mediaListView.selection;
    var multipleItems = false;
    
    // First check through all the items to see if they are the same or not
    // We determine that items are the same if they belong to the same album
    // and have the same AlbumArtist or ArtistName.
    var itemEnum = selection.selectedIndexedMediaItems;
    var oldAlbumName = null;
    var oldAlbumArtist = null;
    while (itemEnum.hasMoreElements()) {
      var item = itemEnum.getNext().mediaItem;
      var albumName = item.getProperty(SBProperties.albumName);
      var albumArtist = item.getProperty(SBProperties.albumArtist);
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
        
        var sbs = Cc["@mozilla.org/intl/stringbundle;1"]
                    .getService(Ci.nsIStringBundleService);
        var albumartStrings = sbs.createBundle("chrome://albumart/locale/albumart.properties");
        
        var strTitle = SBString(STRINGROOT + "title", null, albumartStrings);
        var strMsg = SBString(STRINGROOT + "message", null, albumartStrings);
        var strCheck = SBString(STRINGROOT + "check", null, albumartStrings);
        
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
    
    // Now actually set the properties
    var mediaItemArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                        .createInstance(Ci.nsIMutableArray);
    itemEnum = selection.selectedIndexedMediaItems;
    while (itemEnum.hasMoreElements()) {
      var item = itemEnum.getNext().mediaItem;
      var oldImage = item.getProperty(SBProperties.primaryImageURL);
      if (oldImage != aImageFile) {
        item.setProperty(SBProperties.primaryImageURL, aImageFile);
        mediaItemArray.appendElement(item, false);
      }
    }
    
    // Write the images to metadata
    if (mediaItemArray.length > 0) {
      var metadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);      
      try {
        var job = metadataService.write(mediaItemArray);
      
        SBJobUtils.showProgressDialog(job, window);
      } catch (e) {
        // Job will fail if writing is disabled by the pref
        Components.utils.reportError(e);
      }
    }
    
    // Update the currently selected display.
    AlbumArt.onSelectionChanged();
  },

  /*********************************
   * Drag and Drop
   ********************************/
  getSupportedFlavours : function () {
    var flavours = new FlavourSet();
    return sbCoverHelper.getFlavours(flavours);
  },
  
  onDrop: function (aEvent, aDropData, aSession) {
    var self = this;
    sbCoverHelper.handleDrop(function (newFile) {
      if (newFile && newFile != "") {
        self.setSelectionsCover(newFile);
      }
    }, aDropData);
  },

  onDragOver: function(event, flavour, session) {
    // No need to do anything here, for UI we should set the
    // #sb-albumart-selected:-moz-drag-over style.
  },
  
  onDragStart: function TrackEditorArtwork_onDragStart(aEvent, 
                                                       aTransferData,
                                                       aAction) {
    var albumArtSelectedImage = document.getElementById('sb-albumart-selected');
    var imageURL  = albumArtSelectedImage.getAttribute("src");
    aTransferData.data = new TransferData();
    sbCoverHelper.setupDragTransferData(aTransferData, imageURL);
  },
  
  /*********************************
   * sbIMediaListViewSelectionListener
   ********************************/
  onSelectionChanged: function() {
    var selection = AlbumArt._mediaListView.selection;
    var curImageUrl = null;
    var itemEnum = selection.selectedIndexedMediaItems;
    if (itemEnum.hasMoreElements()) {
      var item = itemEnum.getNext().mediaItem;
      curImageUrl = item.getProperty(SBProperties.primaryImageURL);
    }
    
    if (curImageUrl == "") {
      // Change empty urls to the default cover
      curImageUrl = DEFAULT_COVER;
    }

    AlbumArt.changeNowSelected(curImageUrl);
  },

  onCurrentIndexChanged: function() {
  },

  /*********************************
   * sbIPlaylistPlaybackListener
   ********************************/
  onStop: function() {
    // Basicly clear the now playing image
    AlbumArt.observe("", "metadata.imageURL", "");
  },
  onBeforeTrackChange: function(aItem, aView, aIndex) {
    var newImageURL = aItem.getProperty(SBProperties.primaryImageURL);
    if (!newImageURL || newImageURL == "") {
      newImageURL = DEFAULT_COVER;
    }
    AlbumArt.observe("", "metadata.imageURL", newImageURL);
  },
  onTrackIndexChange: function(aItem, aView, aIndex) { },
  onBeforeViewChange: function(aView) { },
  onViewChange: function(aView) { },
  onTrackChange: function(aItem, aView, aIndex) { },

  /*********************************
   * nsISupports
   ********************************/
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIPlaylistPlaybackListener,
                                         Ci.sbIMediaListViewSelectionListener])
};

// We need to use DOMContentLoaded instead of load here because of the
// Mozilla Bug #420815
window.addEventListener("DOMContentLoaded", AlbumArt.onLoad, false);
window.addEventListener("unload", AlbumArt.onUnload, false);
