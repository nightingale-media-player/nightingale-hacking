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

Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

// Display pane constants
const DISPLAY_PANE_CONTENTURL = "chrome://albumart/content/albumArtPane.xul";
const DISPLAY_PANE_ICON       = "http://songbirdnest.com/favicon.ico";

// Default cover for items missing the cover
const DEFAULT_COVER = "chrome://songbird/skin/album-art/default-cover.png";

/******************************************************************************
 *
 * \class AlbumArtPaneControler 
 * \brief Controller for the Album Art Pane.
 *
 *****************************************************************************/
var AlbumArtPaneController = {
  _coverBind: null,                 // Data remote for the now playing image.
  _playListPlaybackService: null,   // Get notifications of track changes

  // Cache some things so we don't have to keep loading the same image
  _lastImageSrc: "",       // keep track of what the last image src was
  _imageWidth: 0,          // hold this for resizing
  _imageHeight: 0,         // and this.

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
      var displayPane = window.top.document.getElementById('displaypane_servicepane_bottom');
      if (displayPane) {
        var displayPaneHeaderSize = displayPane.height - windowHeight;
        displayPane.height = windowWidth + displayPaneHeaderSize;
        
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
    if (!AlbumArtPaneController.isPaneSquare()) {
      // Abort because this function will be called again.
      return;
    }

    var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
    var currentSrc = albumArtPlayingImage.getAttribute("src");

    // Do not do anything if we don't have an image to resize
    if (currentSrc == "") {
      this._lastImageSrc = "";
      return;
    }

    // See if we have already cached this information
    if (currentSrc != this._lastImageSrc) {
      // We do this to get the proper height and width, since it will not be
      // resized when loaded with the Image() object. The <image> elements
      // change the height and width when the image is resized to fit the
      // content.
      var newImg = new Image();
      newImg.src = currentSrc;
      this._imageHeight = newImg.height;
      this._imageWidth = newImg.width;
    }

    // Do not do anything with invalid images.
    if (this._imageWidth <= 0 || this._imageHeight <= 0) {
      return;
    }
    
    // Window is our displaypane, so grab the size
    var windowWidth = window.innerWidth;
    var windowHeight = window.innerHeight;
    
    // Default to the size of the image
    var newWidth = this._imageWidth;
    var newHeight = this._imageHeight;
    
    // Only resize if the image is bigger than the display pane (window)
    if ( (this._imageWidth > windowWidth) ||
         (this._imageHeight > windowHeight) ) {
      // Figure out the ratios of the image and window.
      var windowRatio = windowWidth / windowHeight;
      var imageRatio = this._imageWidth / this._imageHeight;
      
      // If windowRatio is greater than the imageRatio then we want to resize
      // the height, otherwize we resize the width to keep the aspect ratio.
      // We also do not want to make the image bigger than it actually is.
      if (imageRatio >= windowRatio) {
        newWidth = (windowWidth <= this._imageWidth ?
                      windowWidth : this._imageWidth);
        newHeight = (this._imageHeight * newWidth / this._imageWidth);
      } else {
        newHeight = (windowHeight <= this._imageHeight ?
                      windowHeight : this._imageHeight);
        newWidth = (this._imageWidth * newHeight / this._imageHeight);
      }
    }

    albumArtPlayingImage.style.width = newWidth + "px";
    albumArtPlayingImage.style.height = newHeight + "px";
  },

  /**
   * \brief onImageLoad - This is called when the image is done loading, we
   *        watch this so that we do not call the resize until we have a full
   *        image to resize.
   */
  onImageLoad: function () {
    var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
    if (albumArtPlayingImage.getAttribute("src")) {
      // Resize the image to keep the aspect ratio.
      AlbumArtPaneController.onResize();
    }
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
    AlbumArtPaneController.onResize();
  },
  
  /**
   * \brief onLoad - Called when the display pane loads, here we make sure that
   *        we have the correct image loaded or we display the Nothing Playing
   *        message.
   */
  onLoad: function () {
    // Set the title correctly, localize since we can not localize from the
    // install.rdf.
    var paneMgr = Cc["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
                    .getService(Ci.sbIDisplayPaneManager);
    paneMgr.updateContentInfo(DISPLAY_PANE_CONTENTURL,
                              SBString("albumart.displaypane.title",
                                       "Album Art"),
                              DISPLAY_PANE_ICON);

    // Listen to when an image resizes so we can keep the aspect ratio.
    var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
    albumArtPlayingImage.addEventListener("load",
                                          AlbumArtPaneController.onImageLoad,
                                          false);
    
    // Do an initial load of the default cover since it is a big image, and then
    // set the width and height of the image to the width and height of the
    // window so the image does not stretch out of bounds on the first load.
    this.isPaneSquare(); // Make sure we are square (this will resize)
    var newImg = new Image();
    newImg.src = DEFAULT_COVER;
    var windowWidth = window.innerWidth;
    var windowHeight = window.innerHeight;
    albumArtPlayingImage.style.width = windowWidth + "px";
    albumArtPlayingImage.style.height = windowHeight + "px";

    // Setup the dataremote for the now playing image.
    var createDataRemote =  new Components.Constructor(
                                  "@songbirdnest.com/Songbird/DataRemote;1",
                                  Components.interfaces.sbIDataRemote,
                                  "init");
    this._coverBind = createDataRemote("metadata.imageURL", null);
    this._coverBind.bindObserver(AlbumArtPaneController, false);

    // Load the playListPlaybackService so we can monitor track changes for
    // faster image changing.
    this._playListPlaybackService = 
                          Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
                            .getService(Ci.sbIPlaylistPlayback);
    this._playListPlaybackService.addListener(AlbumArtPaneController);

    // Listen for resizes of the display pane so that we can keep the aspect
    // ratio of the image.
    window.addEventListener("resize",
                            AlbumArtPaneController.onResize,
                            false);
  },
  
  /**
   * \brief onUnload - This is called when the display pane is closed. Here we
   *        can shut every thing down.
   */
  onUnload: function () {
    AlbumArtPaneController._playListPlaybackService.removeListener(AlbumArtPaneController);
    AlbumArtPaneController._playListPlaybackService = null;
    
    AlbumArtPaneController._coverBind.unbind();
    AlbumArtPaneController._coverBind = null;
    
    var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
    albumArtPlayingImage.removeEventListener("load",
                                             AlbumArtPaneController.onImageLoad,
                                             false);
    
    window.removeEventListener("resize",
                               AlbumArtPaneController.onResize,
                               false);
  },

  /*********************************
   * sbIPlaylistPlaybackListener
   ********************************/
  onStop: function() {
    AlbumArtPaneController.observe("", "metadata.imageURL", "");
  },
  onBeforeTrackChange: function(aItem, aView, aIndex) {
    var newImageURL = aItem.getProperty(SBProperties.primaryImageURL);
    if (!newImageURL || newImageURL == "") {
      newImageURL = DEFAULT_COVER;
    }
    AlbumArtPaneController.observe("", "metadata.imageURL", newImageURL);
  },
  onTrackIndexChange: function(aItem, aView, aIndex) { },
  onBeforeViewChange: function(aView) { },
  onViewChange: function(aView) { },
  onTrackChange: function(aItem, aView, aIndex) { },

  QueryInterface: XPCOMUtils.generateQI([Ci.sbIPlaylistPlaybackListener])
};
