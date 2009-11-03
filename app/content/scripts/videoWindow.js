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
  // Internal data
  _mediacoreManager: null,
  _shouldDismissSelf: false,
  _ssp: null,
  
  // sbIMediacoreEventListener
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
  
  // Internal functions
  _initialize: function vwc__initialize() {
    this._mediacoreManager = 
      Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
        .getService(Ci.sbIMediacoreManager);
    
    this._mediacoreManager.addListener(this);
    
    this._ssp = Cc["@songbirdnest.com/Songbird/ScreenSaverSuppressor;1"]
                  .getService(Ci.sbIScreenSaverSuppressor);
    this._ssp.suppress(true);
  },
  
  _shutdown: function vwc__shutdown() {
    this._mediacoreManager.removeListener(this);
    this._mediacoreManager = null;
    this._ssp = null;
  },
  
  _handleBeforeTrackChange: function vwc__handleBeforeTrackChange(aEvent) {
    var mediaItem = aEvent.data;
    
    // If the next item is not video, we will dismiss 
    // the window on track change.
    if(mediaItem.contentType != "video") {
      this._shouldDismiss = true;
    }
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
    // XXXAus: This will be implemented when we add support for 
    //         'actual size'. See bug 18056.
  },
  
  _dismissSelf: function vwc__dismissSelf() {
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
