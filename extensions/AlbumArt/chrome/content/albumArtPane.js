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

Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbCoverHelper.jsm");
Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
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
      
      SBOpenModalDialog("chrome://albumart/content/coverPreview.xul",
                   "coverPreview",
                   "all,chrome,centerscreen",
                   passImageParam,
                   null);
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
  },

  /**
   * \brief resizeImage - This function will take care of properly resizing an
   *        image to keep the proper aspect ratio when our display pane resizes.
   *        We use a cache because currently <image> tags do not hold the size
   *        information as we need it.
   * \param aCacheIndex - An index to which image to resize, this will generally
   *        be linked to the deck index, 0 = Selected for example.
   */
  resizeImage: function AlbumArt_resizeImage(aCacheIndex) {
    var imageCache = AlbumArt._stateInfo[aCacheIndex];
    var mImage = document.getElementById(imageCache.imageID);
    var currentSrc = mImage.src;
    
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
  changeNowSelected: function AlbumArt_changeNowSelected(aNewURL) {
    // Load up our elements
    var albumArtSelectedImage = document.getElementById('sb-albumart-selected');
    var albumArtNotSelectedBox = document.getElementById('sb-albumart-not-selected');
    var albumArtSelectedDragBox = document.getElementById('sb-albumart-select-drag');


    // Configure the display pane
    var itemImage = aNewURL;
    if (!AlbumArt._nowSelectedMediaItem) {
      // Show the not playing message.
      albumArtNotSelectedBox.removeAttribute("hidden");
      // Hide the Drag Here box
      albumArtSelectedDragBox.setAttribute("hidden", true);
      // Set the image to the default cover and clear the item image
      aNewURL = DROP_TARGET_IMAGE;
      itemImage = null;
    } else if (!aNewURL) {
     // Show the Drag Here box
      albumArtSelectedDragBox.removeAttribute("hidden");
      // Hide the not selected message.
      albumArtNotSelectedBox.setAttribute("hidden", true);
      // Set the image to the default cover and clear the item image
      aNewURL = DROP_TARGET_IMAGE;
      itemImage = null;
    } else {
      // Hide the not selected message.
      albumArtNotSelectedBox.setAttribute("hidden", true);
      // Hide the Drag Here box
      albumArtSelectedDragBox.setAttribute("hidden", true);
    }
    albumArtSelectedImage.src = aNewURL;
    if (itemImage)
      albumArtSelectedImage.setAttribute("itemimage", itemImage);
    else
      albumArtSelectedImage.removeAttribute("itemimage");
    // Call the onResize so we display the image correctly.
    AlbumArt.onResize();
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

    // Configure the display pane
    var itemImage = aNewURL;
    if (!AlbumArt.getNowPlayingItem()) {
      // Show the not playing message.
      albumArtNotPlayingBox.removeAttribute("hidden");
      // Set the image to the default cover and clear the item image
      aNewURL = DROP_TARGET_IMAGE;
      itemImage = null;
    } else if (!aNewURL) {
      // Show the Drag Here box
      albumArtPlayingDragBox.removeAttribute("hidden");
      // Hide the not playing message.
      albumArtNotPlayingBox.setAttribute("hidden", true);
      // Set the image to the default cover and clear the item image
      aNewURL = DROP_TARGET_IMAGE;
      itemImage = null;
    } else {
      // Hide the not playing message.
      albumArtNotPlayingBox.setAttribute("hidden", true);
      // Hide the Drag Here box
      albumArtPlayingDragBox.setAttribute("hidden", true);
    }
    albumArtPlayingImage.src = aNewURL;
    if (itemImage)
      albumArtPlayingImage.setAttribute("itemimage", itemImage);
    else
      albumArtPlayingImage.removeAttribute("itemimage");
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

    // Do an initial load of the default cover since it is a big image, and then
    // set the width and height of the images to the width and height of the
    // window so the image does not stretch out of bounds on the first load.
    var newImg = new Image();
    newImg.src = DROP_TARGET_IMAGE;
    var windowWidth = window.innerWidth;
    var windowHeight = window.innerHeight;
    for (var cIndex = 0; cIndex < AlbumArt._stateInfo.length; cIndex++) {
      var imageCache = AlbumArt._stateInfo[cIndex];
      
      // Listen to when an image loads so we can keep the aspect ratio.
      var mImage = document.getElementById(imageCache.imageID);
      if (mImage) {
        mImage.addEventListener("load", AlbumArt.onResize, false);
        mImage.style.width = windowWidth + "px";
        mImage.style.height = windowHeight + "px";
      }
    }

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
      AlbumArt.onSelectionChanged();
    }
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
    
    var propArray = ArrayConverter.stringEnumerator([SBProperties.primaryImageURL]);
    var metadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                            .getService(Ci.sbIFileMetadataService);      
    try {
      var job = metadataService.write([aMediaItem], propArray);
    
      SBJobUtils.showProgressDialog(job, window);
    } catch (e) {
      // Job will fail if writing is disabled by the pref
      Components.utils.reportError(e);
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
    var itemEnum = selection.selectedIndexedMediaItems;
    var oldAlbumName = null;
    var oldAlbumArtist = null;
    while (itemEnum.hasMoreElements()) {
      var item = itemEnum.getNext().mediaItem;
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
    itemEnum = selection.selectedIndexedMediaItems;
    while (itemEnum.hasMoreElements()) {
      var item = itemEnum.getNext().mediaItem;
      var oldImage = item.getProperty(SBProperties.primaryImageURL);
      if (oldImage != aNewImageUrl) {
        item.setProperty(SBProperties.primaryImageURL, aNewImageUrl);
        mediaItemArray.appendElement(item, false);
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
    if (AlbumArt._currentState == STATE_SELECTED) {
      var albumArtSelectedImage = document.getElementById('sb-albumart-selected');
      return albumArtSelectedImage.getAttribute("itemimage");
    } else {
      var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
      return albumArtPlayingImage.getAttribute("itemimage");
    }
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
    var imageData = sbClipboard.copyImageFromClipboard(mimeType, {});
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
    if (curImageUrl) {
      cutElem.removeAttribute("disabled");
      copyElem.removeAttribute("disabled");
      clearElem.removeAttribute("disabled");
    } else {
      cutElem.setAttribute("disabled", true);
      copyElem.setAttribute("disabled", true);
      clearElem.setAttribute("disabled", true);
    }

    // Check if the clipboard contains valid album art.
    var validAlbumArt = {};
    AlbumArt.getClipboardAlbumArt({}, {}, validAlbumArt);
    validAlbumArt = validAlbumArt.value;

    // Update the popup menu for the clipboard contents.
    var pasteElem = document.getElementById("pasteMenuItem");
    if (validAlbumArt)
      pasteElem.removeAttribute("disabled");
    else
      pasteElem.setAttribute("disabled", true);
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

      var newFile = artService.cacheImage(mimeType,
                                          imageData,
                                          imageData.length);
      if (newFile) {
        AlbumArt.setCurrentStateItemImage(newFile.spec);
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
      var imageData, mimetype;
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
    var item = null;
    var itemEnum = selection.selectedIndexedMediaItems;
    if (itemEnum.hasMoreElements())
      item = itemEnum.getNext().mediaItem;

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
      
      case Ci.sbIMediacoreEvent.BEFORE_TRACK_CHANGE:
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
