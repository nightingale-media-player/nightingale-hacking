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

const DISPLAY_PANE_CONTENTURL = "chrome://albumart/content/albumArtPane.xul";
const DISPLAY_PANE_ICON = "http://songbirdnest.com/favicon.ico";

/******************************************************************************
 *
 * \class AlbumArtPaneControler 
 * \brief Controller for the Album Art Pane.
 *
 *****************************************************************************/
var AlbumArtPaneController = {
  _coverBind: null,       // Data remote for the now playing.

  // Cache some things so we don't have to keep loading the same image
  _lastImageSrc: "",      // keep track of what the last image src was
  _imageWidth: 0,          // hold this for resizing
  _imageHeight: 0,         // and this.

  onImageDblClick: function (aEvent) {
    // Only respond to primary button double clicks.
    if (aEvent.button == 0) {
      SBOpenModalDialog("chrome://albumart/content/coverPreview.xul",
                   "coverPreview",
                   "all,chrome,resizable=no,centerscreen",
                   null,
                   null);
    }
  },

  onResize: function () {
    var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
    var currentSrc = albumArtPlayingImage.getAttribute("src");

    // Do not do anything if we don't have an image to resize
    if (currentSrc == "") {
      this._lastImageSrc = "";
      return;
    }

    // See if we have already cached this information
    if (currentSrc != this._lastImageSrc) {
      var newImg = new Image();
      newImg.src = currentSrc;
      this._imageHeight = newImg.height;
      this._imageWidth = newImg.width;
    }

    // Window is our displaypane, so grab the size
    var windowWidth = window.innerWidth;
    var windowHeight = window.innerHeight;

    // Do not do anything with invalid images.
    if (this._imageWidth <= 0 || this._imageHeight <= 0) {
      return;
    }
    
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

  onImageLoad: function () {
    var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
    var albumArtNotPlayingBox = document.getElementById('sb-albumart-not-playing');
    
    if (albumArtPlayingImage.getAttribute("src") == "") {
      // Show the not playing message.
      albumArtNotPlayingBox.removeAttribute("hidden");
    } else {
      // Hide the not playing message.
      albumArtNotPlayingBox.setAttribute("hidden", true);
      // Resize the image to keep aspect ratio.
      AlbumArtPaneController.onResize();
    }
  },
  
  onLoad: function () {
    // Set the title correctly, localize since we can not from the install.rdf.
    paneMgr = Cc["@songbirdnest.com/Songbird/DisplayPane/Manager;1"]
                .getService(Ci.sbIDisplayPaneManager);
    paneMgr.updateContentInfo(DISPLAY_PANE_CONTENTURL,
                              SBString("albumart.displaypane.title",
                                       "Album Art"),
                              DISPLAY_PANE_ICON);

    // When an image loads we hide the not playing message.
    var albumArtPlayingImage = document.getElementById('sb-albumart-playing');
    albumArtPlayingImage.addEventListener("load",
                                          AlbumArtPaneController.onImageLoad,
                                          false);

    // Setup the dataremote for the now playing image.
    var createDataRemote =  new Components.Constructor(
                                  "@songbirdnest.com/Songbird/DataRemote;1",
                                  Components.interfaces.sbIDataRemote,
                                  "init");
    this._coverBind = createDataRemote("metadata.imageURL", null);
    this._coverBind.bindAttribute(albumArtPlayingImage,
                                  "src",
                                  false,
                                  false,
                                  null);
    
    // Listen for resizes of the display pane so that we can keep the aspect
    // ratio of the image.
    window.addEventListener("resize",
                            AlbumArtPaneController.onResize,
                            false);
  },
  
  onUnload: function () {
    this._coverBind.unbind();
    window.removeEventListener("resize",
                               AlbumArtPaneController.onResize,
                               false);
  }
};
