/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;


//------------------------------------------------------------------------------
// Constants

const SB_OSDHIDE_DELAY = 3000;
const SB_BLUREVENT_CHECK_DELAY = 1;

const SB_LAST_BLUR_OSD_WINDOW = 0;
const SB_LAST_BLUR_VIDEO_WINDOW = 1;


//==============================================================================
//
// @interface sbOSDControlService
// @brief Service to provide on-screen-display controls for video playback.
//
//==============================================================================

function sbOSDControlService()
{
  this._cloakService = Cc["@songbirdnest.com/Songbird/WindowCloak;1"]
                         .getService(Ci.sbIWindowCloak);
  this._nativeWinMgr =
    Cc["@songbirdnest.com/integration/native-window-manager;1"]
      .getService(Ci.sbINativeWindowManager);

  this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  this._blurTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
}

sbOSDControlService.prototype =
{
  _videoWindow:           null,
  _videoWindowFullscreen: false,
  _osdWindow:             null,
  _cloakService:          null,
  _nativeWinMgr:          null,
  _timer:                 null,
  _OS:                    null,
  _osdControlsShowing:    false,
  _lastBlurWindow:        -1,
  _videoWindowsLostFocus: false,
  _blurTimer:             null,


  _recalcOSDPosition: function() {
    // This is only necessary when we are not in fullscreen.
    // If we were to do this while in fullscreen, it would re-anchor
    // the controls onto the wrong window.
    if(!this._videoWindowFullscreen) {
      this._osdWindow.moveTo(this._videoWindow.screenX,
                             this._videoWindow.screenY);
      this._osdWindow.resizeTo(this._videoWindow.innerWidth,
                               this._videoWindow.outerHeight);
    }
  },

  //----------------------------------------------------------------------------
  // sbIOSDControlService

  onVideoWindowOpened: function(aVideoWindow) {
    // For now, just open the OSD control pane thingy.
    this._videoWindow = aVideoWindow.QueryInterface(Ci.nsIDOMWindowInternal);

    // Create a OSD overlay window.
    this._osdWindow = this._videoWindow.openDialog(
        "chrome://songbird/content/xul/videoWindowControls.xul",
        "Songbird OSD Control Window",
        "chrome,modal=no,titlebar=no",
        null);
    this._osdWindow.QueryInterface(Ci.nsIDOMWindowInternal);

    // Cloak the window right now.
    this._cloakService.cloak(this._osdWindow);

    this._OS = Cc["@mozilla.org/xre/app-info;1"]
                 .getService(Components.interfaces.nsIXULRuntime)
                 .OS;

    // If we are not on Windows, listen for window move events 
    // on the video window. Windows sends 'resize' events when the window is
    // moved.
    if (this._OS != "WINNT") {
      try {
        // Not all platforms have this service.
        var winMoveService =
          Cc["@songbirdnest.com/integration/window-move-resize-service;1"]
          .getService(Ci.sbIWindowMoveService);

        winMoveService.startWatchingWindow(this._videoWindow, this);
      }
      catch (e) {
        // No window move service on this platform.
      }
    }

    // Listen for blur and focus events for determing when both the video
    // window and the OSD controls loose focus.
    var self = this;
    this._osdWinBlurListener = function(aEvent) {
      self._onOSDWinBlur(aEvent);
    };
    this._videoWinBlurListener = function(aEvent) {
      self._onVideoWinBlur(aEvent);
    };
    this._osdWinFocusListener = function(aEvent) {
      self._onOSDWinFocus(aEvent);
    };
    this._videoWinFocusListener = function(aEvent) {
      self._onVideoWinFocus(aEvent);
    };
    this._osdWindow.addEventListener("blur",
                                     this._osdWinBlurListener,
                                     false);
    this._osdWindow.addEventListener("focus",
                                     this._osdWinFocusListener,
                                     false);
    this._videoWindow.addEventListener("blur",
                                       this._videoWinBlurListener,
                                       false);
    this._videoWindow.addEventListener("focus",
                                       this._videoWinFocusListener,
                                       false);
  },

  onVideoWindowWillClose: function() {
    // We don't use this service on Windows.
    if (this._OS != "WINNT") {
      try {
        // Not all platforms have this service.
        var winMoveService =
          Cc["@songbirdnest.com/integration/window-move-resize-service;1"]
          .getService(Ci.sbIWindowMoveService);

        winMoveService.stopWatchingWindow(this._videoWindow, this);
      }
      catch (e) {
        // No window move service on this platform.
      }
    }
    
    this._timer.cancel();
    this._blurTimer.cancel();
    this._osdWindow.removeEventListener("blur",
                                        this._osdWinBlurListener,
                                        false);
    this._osdWindow.removeEventListener("focus",
                                        this._osdWinFocusListener,
                                        false);
    this._videoWindow.removeEventListener("blur",
                                          this._videoWinBlurListener,
                                          false);
    this._videoWindow.removeEventListener("focus",
                                          this._videoWinFocusListener,
                                          false);
    this._osdWindow.close();
    this._osdWindow = null;
    this._videoWindow = null;
  },

  onVideoWindowResized: function() {
    this._recalcOSDPosition();
  },
  
  onVideoWindowFullscreenChanged: function(aFullscreen) {
    this._videoWindowFullscreen = aFullscreen;
    this._osdWindow.fullScreen = aFullscreen;

    var outterBox = 
      this._osdWindow.document.getElementById("osd_wrapper_hbox");
    
    if(outterBox) {
      if(aFullscreen) {
        outterBox.setAttribute("fullscreen", true);
      }
      else {
        outterBox.removeAttribute("fullscreen");
      }
    }
  },

  hideOSDControls: function() {
    if (!this._cloakService.isCloaked(this._osdWindow)) {
      this._nativeWinMgr.setOnTop(this._osdWindow, false);
      this._cloakService.cloak(this._osdWindow);
    }

    // The OSD controls are no longer showing
    this._osdControlsShowing = false;
  },

  showOSDControls: function() {
    this._recalcOSDPosition();

    // Show the controls if they are currently hidden.
    if (this._cloakService.isCloaked(this._osdWindow)) {
      this._cloakService.uncloak(this._osdWindow);
      this._nativeWinMgr.setOnTop(this._osdWindow, true);
    }

    // Controls are showing
    this._osdControlsShowing = true;

    // Set the timer for hiding.
    this._timer.initWithCallback(this,
                                 SB_OSDHIDE_DELAY,
                                 Ci.nsITimer.TYPE_ONE_SHOT);
  },

  _onOSDWinBlur: function(aEvent) {
    // The OSD controls have blurred, set a timer to ensure that the focused
    // window is the video window before hiding the OSD controls.
    this._lastBlurWindow = SB_LAST_BLUR_OSD_WINDOW;
    this._videoWindowsLostFocus = true;
    this._blurTimer.initWithCallback(this,
                                     SB_BLUREVENT_CHECK_DELAY,
                                     Ci.nsITimer.TYPE_ONE_SHOT);
  },

  _onOSDWinFocus: function(aEvent) {
    if (this._lastBlurWindow == SB_LAST_BLUR_VIDEO_WINDOW) {
      // The OSD window gained focus when the video window blurred, toggle the
      // flag to prevent the OSD controls from being hidden.
      this._videoWindowsLostFocus = false;
    }
  },

  _onVideoWinBlur: function(aEvent) {
    // The video window has blurred, set a timer to ensure that the focused
    // window is the OSD controls window before hiding the OSD controls.
    this._lastBlurWindow = SB_LAST_BLUR_VIDEO_WINDOW;
    this._videoWindowsLostFocus = true;
    this._blurTimer.initWithCallback(this,
                                     SB_BLUREVENT_CHECK_DELAY,
                                     Ci.nsITimer.TYPE_ONE_SHOT);
  },

  _onVideoWinFocus: function(aEvent) {
    if (this._lastBlurWindow == SB_LAST_BLUR_OSD_WINDOW) {
      // The video window gained focus when the OSD controls blurred, toggle
      // the flag to prevent the OSD controls from being hidden.
      this._videoWindowsLostFocus = false;
    }
  },

  //----------------------------------------------------------------------------
  // sbIWindowMoveListener

  onMoveStarted: function() {
    this.hideOSDControls();
  },

  onMoveStopped: function() {
    // XXX KREEGER - Use a timeout here instead of showing right away! //
    // See bug 18150.

    this._recalcOSDPosition();
    this.showOSDControls();
  },

  //----------------------------------------------------------------------------
  // nsITimerCallback

  notify: function(aTimer) {
    if (aTimer == this._timer) {
      this.hideOSDControls();
    }
    else if (aTimer == this._blurTimer) {
      if (this._videoWindowsLostFocus && !this._videoWindowFullscreen) {
        //
        // When one of the video window elements (osd or video window) lost
        // focus, the other window element did not gain focus. This means that
        // another window in the OS gained focus. To prevent weird Z-ordering
        // issues here, just hide the OSD controls now to prevent it from
        // hovering above other windows in the OS.
        // 
        // We only do this when we are not in fullscreen since the blur
        // tracking works on the video window, not the native fullscreen
        // window used by the mediacores.
        //
        this.hideOSDControls();
      }

      // Cleanup blur handling
      this._videoWindowsLostFocus = false;
      this._lastBlurWindow = -1;
    }
  },
};

//------------------------------------------------------------------------------
// XPCOM Registration

sbOSDControlService.prototype.classDescription =
  "Songbird OSD Control Service";
sbOSDControlService.prototype.className =
  "sbOSDControlService";
sbOSDControlService.prototype.classID =
  Components.ID("{03F78779-FCB7-4442-9A0C-E8547B4F1368}");
sbOSDControlService.prototype.contractID =
  "@songbirdnest.com/mediacore/osd-control-service;1";
sbOSDControlService.prototype.QueryInterface =
  XPCOMUtils.generateQI([Ci.sbIOSDControlService,
                         Ci.sbIWindowMoveListener,
                         Ci.nsITimerCallback]);

function NSGetModule(compMgr, fileSpec)
{
  return XPCOMUtils.generateModule([sbOSDControlService]);
}

