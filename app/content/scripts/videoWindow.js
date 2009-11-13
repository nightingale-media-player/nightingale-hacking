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

/**
 * \file videoWindow.js
 * \brief Video window controller.
 * \internal
 */

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/DOMUtils.jsm");
Cu.import("resource://app/jsmodules/SBDataRemoteUtils.jsm");
Cu.import("resource://app/jsmodules/SBUtils.jsm");
  
var videoWindowController = {
  //////////////////////////////////////////////////////////////////////////////
  // Internal data
  //////////////////////////////////////////////////////////////////////////////

  _mediacoreManager: null,
  _shouldDismissSelf: false,
  _ssp: null,
  
  _actualSizeDataRemote: null,
  _windowNeedsResize: false,
  
  _contextMenu: null,
  _contextMenuListener: null,
  
  _ignoreResize: false,
  _resizeListener: null,
  _showing: false,
  
  _videoBox: null,
  _videoElement: null,

  _osdService: null,
  
  //////////////////////////////////////////////////////////////////////////////
  // Getters/Setters
  //////////////////////////////////////////////////////////////////////////////

  get ACTUAL_SIZE_DR_KEY() {
    const dataRemoteKey = "videowindow.actualsize";
    return dataRemoteKey;
  },
  
  //////////////////////////////////////////////////////////////////////////////
  // sbIMediacoreEventListener
  //////////////////////////////////////////////////////////////////////////////

  onMediacoreEvent: function vwc_onMediacoreEvent(aEvent) {
    switch(aEvent.type) {
      case Ci.sbIMediacoreEvent.BEFORE_TRACK_CHANGE: {
        this._handleBeforeTrackChange(aEvent);
      }
      break;
      
      case Ci.sbIMediacoreEvent.TRACK_CHANGE: {
        this._handleTrackChange(aEvent);
      }
      break;
      
      case Ci.sbIMediacoreEvent.SEQUENCE_END: {
        this._handleSequenceEnd(aEvent);
      }
      break;
      
      case Ci.sbIMediacoreEvent.VIDEO_SIZE_CHANGED: {
        this._handleVideoSizeChanged(aEvent);
      }
      break;
    }
  },
  
  //////////////////////////////////////////////////////////////////////////////
  // nsIObserver
  //////////////////////////////////////////////////////////////////////////////
  
  observe: function vwc_observe(aSubject, aTopic, aData) {
    if(aTopic == this.ACTUAL_SIZE_DR_KEY &&
       this._actualSizeDataRemote.boolValue == true &&
       this._videoBox) {
      this._resizeFromVideoBox(this._videoBox);
    }
  },
  
  //////////////////////////////////////////////////////////////////////////////
  // Init/Shutdown
  //////////////////////////////////////////////////////////////////////////////

  _initialize: function vwc__initialize() {
    this._mediacoreManager = 
      Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
        .getService(Ci.sbIMediacoreManager);

    // Inform the OSD control service that a video window is opening.
    this._osdService = Cc["@songbirdnest.com/mediacore/osd-control-service;1"]
                         .getService(Ci.sbIOSDControlService);
    this._osdService.onVideoWindowOpened(window);

    this._mediacoreManager.addListener(this);
    
    try {
      this._ssp = Cc["@songbirdnest.com/Songbird/ScreenSaverSuppressor;1"]
                    .getService(Ci.sbIScreenSaverSuppressor);
    } catch(e) {
      // No SSP on this platform.
    }

    if (this._ssp)
      this._ssp.suppress(true);
    
    this._actualSizeDataRemote = SBNewDataRemote(this.ACTUAL_SIZE_DR_KEY);
    
    // Catch un-initialized actual size data remote value and default
    // to true in the case where it has no value yet.
    if(this._actualSizeDataRemote.stringValue == null ||
       this._actualSizeDataRemote.stringValue == "") {
      this._actualSizeDataRemote.boolValue = true;
    }

    this._actualSizeDataRemote.bindObserver(this);
    
    // We need to ignore the first resize.
    this._ignoreResize = true;
    
    var self = this;
    this._resizeListener = function(aEvent) {
      self._onResize(aEvent);
    };
    
    window.addEventListener("resize", this._resizeListener, false);
    
    this._contextMenuListener = function(aEvent) {
    //  alert("Context menu requested");
      return self._onContextMenu(aEvent);
    };
    
    window.addEventListener("contextmenu", this._contextMenuListener, false);
    
    this._contextMenu = document.getElementById("video-context-menu");
    this._videoElement = document.getElementById("video-box");
  },
  
  _close: function vwc__close() {
    this._mediacoreManager.sequencer.stop();
    return true;
  },
  
  _shutdown: function vwc__shutdown() {
    window.removeEventListener("resize", this._resizeListener, false);
    this._resizeListener = null;
    
    window.removeEventListener("contextmenu", this._contextMenuListener, false);
    this._contextMenuListener = null;
    
    this._actualSizeDataRemote.unbind();

    this._osdService.onVideoWindowWillClose();
    this._osdService = null;
    
    this._mediacoreManager.removeListener(this);
    this._mediacoreManager = null;
    this._ssp = null;
    this._actualSizeDataRemote = null;
    this._videoBox = null;
    this._videoElement = null;
  },

  //////////////////////////////////////////////////////////////////////////////
  // Actual Size Resizing Methods
  //////////////////////////////////////////////////////////////////////////////
  _resizeFromVideoBox: function vwc__resizeFromVideoBox(aVideoBox) {
    
    var actualHeight = aVideoBox.height;
    var actualWidth = aVideoBox.width * aVideoBox.parNumerator / 
                      aVideoBox.parDenominator;
    
    this._resizeFromWidthAndHeight(actualWidth, 
                                   actualHeight, 
                                   aVideoBox.parNumerator, 
                                   aVideoBox.parDenominator);
  },
  
  _resizeFromWidthAndHeight: function vwc__resizeFromWidthAndHeight(aWidth, 
                                                                    aHeight,
                                                                    aPARNum,
                                                                    aPARDen) {
    function log(str) {
      //dump("_resizeFromWidthAndHeight: " + str + "\n");
    }

    log("Width: " + aWidth);
    log("Height: " + aHeight);
    
    var screen = window.screen;
    var availHeight = screen.availHeight;
    var availWidth = screen.availWidth;
    
    log("Screen: " + screen);
    log("Available Width: " + availWidth);
    log("Available Height: " + availHeight);
    log("Window X: " + window.screenX);
    log("Window Y: " + window.screenY);
    
    var fullWidth = false;
    var deltaWidth = 0;
    
    var fullHeight = false;
    var deltaHeight = 0;
    
    var boxObject = this._videoElement.boxObject;
    var orient = (aWidth < aHeight) ? "portrait" : "landscape";
    
    log("Video Orientation: " + orient);
    
    log("Video Element Width: " + boxObject.width);
    log("Video Element Height: " + boxObject.height);
    
    var decorationsWidth = window.outerWidth - boxObject.width;
    var decorationsHeight = window.outerHeight - boxObject.height;
    
    log("Decorations Width: " + decorationsWidth);
    log("Decorations Height: " + decorationsHeight);
    
    // Doesn't fit width wise, get the ideal width.
    if(aWidth > boxObject.width) {
      let delta = aWidth - boxObject.width;
      
      log("Initial Delta Width: " + delta);
      
      // We're making the window bigger. Make sure it fits on the screen
      // width wise.
      let available = availWidth - window.outerWidth - window.screenX;
      
      log("Current Available Width: " + available);
      
      if(available > delta) {
        // Looks like we have room.
        deltaWidth = delta;
        fullWidth = true;
      }
      else {
        // Not enough room for the size we want, just use the available size.
        deltaWidth = available;
      }
    }
    else {
      // No need to cap anything in this case since the 
      // window is already bigger.
      deltaWidth = aWidth - boxObject.width;
      fullWidth = true;
    }

    
    // Doesn't fit height wise, get the ideal height.
    if(aHeight > boxObject.height) {
      let delta = aHeight - boxObject.height;
      
      log("Initial Delta Height: " + delta);
      
      // We're making the window bigger. Make sure it fits on the screen
      // height wise.
      let available = availHeight - window.outerHeight - window.screenY;
      
      log("Current Available Height: " + available);
      
      if(available >= delta) {
        // Looks like we have room.
        deltaHeight = delta;
        fullHeight = true;
      }
      else {
        // Not enough room for the size we want, just use the available size.
        deltaHeight = available;
      }
    }
    else {
      // Negative delta, we're resizing smaller.
      deltaHeight = aHeight - boxObject.height;
      fullHeight = true;
    }
    
    log("Final Delta Width: " + deltaWidth);
    log("Final Delta Height: " + deltaHeight);
    log("Full Width: " + fullWidth);
    log("Full Height: " + fullHeight);
    
    // Try and resize in a somewhat proportional manner. 
    if(orient == "landscape" && deltaHeight > deltaWidth && !fullWidth) {
      let mul = aPARDen / aPARNum;
      if((aPARDen / aPARNum) == 1) {
        mul = aHeight / aWidth;
      }
      deltaHeight = Math.round(deltaWidth * mul);
    }
    else if(orient == "portrait" && deltaWidth > deltaHeight && !fullHeight) {
      let mul = aPARNum / aPARDen;
      if((aPARNum / aPARDen) == 1) {
        mul = aWidth / aHeight;
      }
      deltaWidth = Math.round(deltaHeight * mul);
    }
    
    // We have to ignore this resize so that we don't disable actual size.
    // This doesn't actually prevent the window from getting resized, it just
    // prevents the actual size data remote from being set to false.
    this._ignoreResize = true;

    log("Final Delta Width (With aspect ratio compensation): " + deltaWidth);
    log("Final Delta Height (With aspect ratio compensation): " + deltaHeight);

    // Resize it!
    window.resizeBy(deltaWidth, deltaHeight);

    log("New Video Element Width: " + boxObject.width);
    log("New Video Element Height: " + boxObject.height);
  },
  
  _moveToCenter: function vwc__moveToCenter() {
    var posX = (window.screen.availWidth - window.outerWidth) / 2;
    var posY = (window.screen.availHeight - window.outerHeight) / 2;
    
    window.moveTo(posX, posY);
  },
  
  //////////////////////////////////////////////////////////////////////////////
  // Mediacore Event Handling
  //////////////////////////////////////////////////////////////////////////////
  
  _handleBeforeTrackChange: function vwc__handleBeforeTrackChange(aEvent) {
    var mediaItem = aEvent.data.QueryInterface(Ci.sbIMediaItem);
    
    dump("_handleBeforeTrackChange: " + mediaItem + "\n");
    dump("_handleBeforeTrackChange: " + mediaItem.contentType + "\n");
    
    // If the next item is not video, we will dismiss 
    // the window on track change.
    if(mediaItem.contentType != "video") {
      //this._shouldDismiss = true;
      this._dismissSelf();
    }
    
    // Clear cached video box.
    this._videoBox = null;
  },
  
  _handleTrackChange: function vwc__handleTrackChange(aEvent) {
    if(this._shouldDismiss) {
      this._dismissSelf();
      this._shouldDismiss = false;
    }
  },
  
  _handleSequenceEnd: function vwc__handleSequenceEnd(aEvent) {
    this._dismissSelf();
  },
  
  _handleVideoSizeChanged: function vwc__handleVideoSizeChanged(aEvent) {
    var videoBox = aEvent.data.QueryInterface(Ci.sbIVideoBox);

    // If actual size is enabled and we are not in fullscreen we can
    // go ahead and 'actual size' the video.    
    if(this._actualSizeDataRemote.boolValue == true &&
       this._mediacoreManager.video.fullscreen == false) {
      // Size it!
      this._resizeFromVideoBox(videoBox);
      
      // Window needs to be centered.
      if(this._needsMove) {
        this._moveToCenter();
        this._needsMove = false;
      }
    }
    
    // We also probably always want to save the last one so that if the user 
    // turns on actual size, we can resize to the right thing.
    this._videoBox = videoBox;
  },

  //////////////////////////////////////////////////////////////////////////////
  // UI Event Handling
  //////////////////////////////////////////////////////////////////////////////
  
  _onContextMenu: function vwc__onContextMenu(aEvent) {
    for each (var node in this._contextMenu.childNodes) {
      switch(node.id) {
        case "actualsize": {
          node.setAttribute("checked", 
                            this._actualSizeDataRemote.boolValue);
        }
        break;
        
        case "fullscreen": {
          node.setAttribute("checked",
                            this._mediacoreManager.video.fullscreen);
        }
        break;
      }
    }
    
    this._contextMenu.openPopupAtScreen(aEvent.screenX, aEvent.screenY, true);
    
    return true;
  },
  
  _onResize: function vwc__onResize(aEvent) {
    // Inform the OSD service that we are resizing.
    this._osdService.onVideoWindowResized();

    // Any resize by the user disables actual size except when the resize event
    // is sent because the window is being shown for the first time, or we are
    // attempting to size it using the sizing hint.
    if(this._ignoreResize) {
      this._ignoreResize = false;
      
      // First time window is being shown, it will be moved to the center
      // of the screen.
      if(!this._showing) {
        this._needsMove = true;
        this._showing = true;
      }
      
      return;
    }
    
    // Actual size is now disabled.
    this._actualSizeDataRemote.boolValue = false;
  },
  
  _onToggleActualSize: function vwc__onToggleActualSize() {
    var toggle = !this._actualSizeDataRemote.boolValue;
    this._actualSizeDataRemote.boolValue = toggle;
  },
  
  _onToggleFullscreen: function vwc__onToggleFullscreen() {
    var video = this._mediacoreManager.video;
    video.fullscreen = !video.fullscreen;
  },
  
  _dismissSelf: function vwc__dismissSelf() {
    if (this._ssp) 
      this._ssp.suppress(false);
    
    setTimeout(function() { window.close(); }, 0);
  },
};

///////////////////////////////////////////////////////////////////////////////
//
// XXXAus: All of the code below will get blown away. I'm just keeping it
//         here for now because it's useful reference. It doesn't actually
//         get used as you can see. :)
//
///////////////////////////////////////////////////////////////////////////////

var SBVideoMinMaxCB =
{
  // Shrink until the box doesn't match the window, then stop.
  GetMinWidth: function()
  {
    // What we'd like it to be
    var outerframe = window.gOuterFrame;
    var retval = 720;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerWidth != outerframe.boxObject.width)
    {
      // That means we found the document's min width.  Because you can't query it directly.
      retval = outerframe.boxObject.width - 1;
    }
    return retval;
  },

  GetMinHeight: function()
  {
    // What we'd like it to be
    var outerframe = window.gOuterFrame;
    var retval = 450;
    // However, if in resizing the window size is different from the document's box object
    if (window.innerHeight != outerframe.boxObject.height)
    {
      // That means we found the document's min height.  Because you can't query it directly.
      retval = outerframe.boxObject.height - 1;
    }
    return retval;
  },

  GetMaxWidth: function()
  {
    return -1;
  },

  GetMaxHeight: function()
  {
    return -1;
  },

  OnWindowClose: function()
  {
    setTimeout(onHideButtonClick, 0);
  },

  QueryInterface : function(aIID)
  {
    if (!aIID.equals(Components.interfaces.sbIWindowMinMaxCallback) &&
        !aIID.equals(Components.interfaces.nsISupports))
    {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }

    return this;
  }
}

/**
 * \brief Set Video window Min/Max width and height window listener.
 * \internal
 */
function setVideoMinMaxCallback()
{
  try {
    var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
    if (windowMinMax) {
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
      if (service)
        service.setCallback(document, SBVideoMinMaxCB);
    }
  }
  catch (err) {
    // No component
    dump("Error. No WindowMinMax component available." + err + "\n");
  }
}

/**
 * \brief Reset the Video window Min/Max width and height window listener to the default listener.
 * \internal
 */
function resetVideoMinMaxCallback() {
  try {
    var windowMinMax = Components.classes["@songbirdnest.com/Songbird/WindowMinMax;1"];
    if (windowMinMax) {
      var service = windowMinMax.getService(Components.interfaces.sbIWindowMinMax);
      if (service)
        service.resetCallback(document);
    }
  }
  catch (err) {
    // No component
    dump("Error. No WindowMinMax component available." + err + "\n");
  }
}
