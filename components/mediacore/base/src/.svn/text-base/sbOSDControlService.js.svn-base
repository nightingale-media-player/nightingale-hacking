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
const MAX_OSD_WIDTH    = 502;
const OSD_HEIGHT       = 70;
const OSD_PADDING      = 18;  // 9 on each side.


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
  this._timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);

  // Ask the window chrome service (if available) if it supports window
  // composition to enable transparent graphics for the OSD.
  this._useTransparentGraphics = true;
  if ("@songbirdnest.com/Songbird/WindowChromeService;1" in Cc) {
    var winChromeService =
      Cc["@songbirdnest.com/Songbird/WindowChromeService;1"]
        .getService(Ci.sbIWindowChromeService);
    this._useTransparentGraphics = winChromeService.isCompositionEnabled;
  }
}

sbOSDControlService.prototype =
{
  _videoWindow:            null,
  _osdWindow:              null,
  _cloakService:           null,
  _timer:                  null,
  _osdControlsShowing:     false,
  _useTransparentGraphics: false,
  
  _videoWinHasFocus:      false,
  _osdWinHasFocus:        false,
  _mouseDownOnOSD:        false,
  
  _fadeInterval: null,
  _fadeContinuation: null,


  _recalcOSDPosition: function() {
    // Compute the position of the OSD controls using the video windows local
    // and screen coordinates.
    var osdY = this._videoWindow.screenY +
      (this._videoWindow.outerHeight * 0.95 - this._osdWindow.outerHeight);

    // Resize the osd window as needed. (after move if needed)
    var newOSDWidth = this._osdWindow.innerWidth;
    if (this._videoWindow.innerWidth + OSD_PADDING > MAX_OSD_WIDTH) {
      newOSDWidth = MAX_OSD_WIDTH;
    }
    else {
      newOSDWidth = this._videoWindow.innerWidth - OSD_PADDING;
    }

    // Now compute X position.
    var osdX = this._videoWindow.screenX +
      (this._videoWindow.innerWidth / 2) - (newOSDWidth / 2);

    // Move to the new (x,y) coordinates.
    this._osdWindow.moveTo(osdX, osdY);

    // Now finally resize the osd window.
    this._osdWindow.resizeTo(newOSDWidth, OSD_HEIGHT);
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
        "chrome,dependent,modal=no,titlebar=no",
        null);
    this._osdWindow.QueryInterface(Ci.nsIDOMWindowInternal);

    // Cloak the window right now.
    this._cloakService.cloak(this._osdWindow);

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
    this._osdWinMousemoveListener = function(aEvent) {
      self._onOSDWinMousemove(aEvent);
    };
    this._osdWinMousedownListener = function(aEvent) {
      self._onOSDWinMousedown(aEvent);
    };
    this._osdWinMouseupListener = function(aEvent) {
      self._onOSDWinMouseup(aEvent);
    };
    this._osdWinKeypressListener = function(aEvent) {
      self._onOSDWinKeypress(aEvent);
    };
    this._osdWindow.addEventListener("blur",
                                     this._osdWinBlurListener,
                                     false);
    this._osdWindow.addEventListener("focus",
                                     this._osdWinFocusListener,
                                     false);
    this._osdWindow.addEventListener("mousemove",
                                     this._osdWinMousemoveListener,
                                     false);
    this._videoWindow.addEventListener("mousemove",
                                       this._osdWinMousemoveListener,
                                       false);
    this._osdWindow.addEventListener("mousedown",
                                     this._osdWinMousedownListener,
                                     false);
    this._osdWindow.addEventListener("mouseup",
                                     this._osdWinMouseupListener,
                                     false);
    this._osdWindow.addEventListener("keypress",
                                     this._osdWinKeypressListener,
                                     false);
    this._videoWindow.addEventListener("blur",
                                       this._videoWinBlurListener,
                                       false);
    this._videoWindow.addEventListener("focus",
                                       this._videoWinFocusListener,
                                       false);
  },

  onVideoWindowWillClose: function() {
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
    
    this._timer.cancel();
    this._osdWindow.removeEventListener("blur",
                                        this._osdWinBlurListener,
                                        false);
    this._osdWindow.removeEventListener("focus",
                                        this._osdWinFocusListener,
                                        false);
    this._osdWindow.removeEventListener("mousemove",
                                        this._osdWinMousemoveListener,
                                        false);
    this._osdWindow.removeEventListener("mousedown",
                                        this._osdWinMousedownListener,
                                        false);
    this._osdWindow.removeEventListener("mouseup",
                                        this._osdWinMouseupListener,
                                        false);
    this._osdWindow.removeEventListener("keypress",
                                        this._osdWinKeypressListener,
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
    var outterBox = 
      this._osdWindow.document.getElementById("osd_wrapper_hbox");
    var fullscreenButton =
      this._osdWindow.document.getElementById("full_screen_button");
    
    if(outterBox) {
      if(aFullscreen) {
        outterBox.setAttribute("fullscreen", true);
        fullscreenButton.setAttribute("fullscreen", true);
      }
      else {
        outterBox.removeAttribute("fullscreen");
        fullscreenButton.removeAttribute("fullscreen");
      }
    }
  },

  hideOSDControls: function(aTransitionType) {
    var self = this;
    // Don't hide the window while the user is dragging
    if (this._mouseDownOnOSD)
      return;

    this._timer.cancel();

    var transition;
    switch (aTransitionType) {
      case Ci.sbIOSDControlService.TRANSITION_FADE:
        transition = this._fadeOut;
        break;

      case Ci.sbIOSDControlService.TRANSITION_NONE:
        transition = this._hideInstantly;
        break;

      default:
        Components.utils.reportError(
            "Invalid transition type passed into hideOSDControls()!");

        // Just fall back to hiding instantly.
        transition = this._hideInstantly; 
    }

    if (!this._useTransparentGraphics) {
      transition = this._hideInstantly;
    }

    transition.call(this, function() {
      // The OSD controls are no longer showing. The order of events
      // here is critical; surprisingly, the cloaking must happen
      // last.
      self._osdControlsShowing = false;

      if (!self._cloakService.isCloaked(self._osdWindow)) {
        if (self._osdWinHasFocus) {
          self._videoWindow.focus();
        }
        self._cloakService.cloak(self._osdWindow);
      }
    });
  },

  showOSDControls: function(aTransitionType) {
    if (!this._videoWinHasFocus &&
        !this._osdWinHasFocus)
    {
      // Don't bother showing the controls if the video and the OSD window have
      // lost focus. This prevents floating the OSD controls ontop of every
      // other window in the OS.
      return;
    }

    if (this._osdSurpressed)
      return;

    this._timer.cancel();
    this._timer.initWithCallback(this,
                                 SB_OSDHIDE_DELAY,
                                 Ci.nsITimer.TYPE_ONE_SHOT);

    // if the osd is already visible, then all we need to do is reset the timer
    // and make sure the controls are focused.
    if (this._osdControlsShowing) {
      this._osdWindow.focus();
      return;
    }
    
    // Controls are showing
    this._osdControlsShowing = true;
    this._recalcOSDPosition();

    // Show the controls if they are currently hidden.
    if (this._cloakService.isCloaked(this._osdWindow)) {
      this._cloakService.uncloak(this._osdWindow);
    }

    var transition;
    switch (aTransitionType) {
      case Ci.sbIOSDControlService.TRANSITION_FADE:
        transition = this._fadeIn;
        break;

      case Ci.sbIOSDControlService.TRANSITION_NONE:
        transition = this._showInstantly;
        break;

      default:
        Components.utils.reportError(
            "Invalid transition type passed into showOSDControls()!");

        // Just fall back to show instantly.
        transition = this._showInstantly;
    }

    if (!this._useTransparentGraphics) {
      transition = this._showInstantly;
    }
    
    transition.call(this);
  },

  _fade: function(start, end, func) {
    var self = this;
    this._fadeCancel();
    this._fadeContinuation = func;
    var node = this._osdWindow.document.getElementById("osd_wrapper_hbox");
    var opacity = start;
    var delta = (end - start) / 10;
    var step = 1;

    self._fadeInterval = self._osdWindow.setInterval(function() {
      opacity += delta;
      node.style.opacity = opacity;

      if (step++ >= 9) {
        self._fadeCancel();
        node = null;
      }
    }, 50);
  },
  
  _fadeCancel: function() {
    this._osdWindow.clearInterval(this._fadeInterval);
    if (this._fadeContinuation) {
       this._fadeContinuation();
    }
    this._fadeContinuation = null;
  },

  _showInstantly: function(func) {
    this._fadeCancel();
    var node = this._osdWindow.document.getElementById("osd_wrapper_hbox");
    if (node) {
      node.style.opacity = 1;
    }
    if (func) {
       func();
    }
  },

  _hideInstantly: function(func) {
    this._fadeCancel();
    if (func) {
       func();
    }
  },

  // fade from 100% opaque to 0% opaque 
  _fadeOut: function(func) {
    this._fade(1, 0, func);
  },

  // fade from 0% opaque to 100% opaque 
  _fadeIn: function(func) {
    this._fade(0, 1, func);
  },

  _onOSDWinBlur: function(aEvent) {
    this._osdWinHasFocus = false;
  },

  _onOSDWinFocus: function(aEvent) {
    this._osdWinHasFocus = true;
  },

  _onVideoWinBlur: function(aEvent) {
    this._videoWinHasFocus = false;
  },

  _onVideoWinFocus: function(aEvent) {
    this._videoWinHasFocus = true;
  },

  _onOSDWinMousemove: function(aEvent) {
    // The user has the mouse over the OSD controls, ensure that the controls
    // are visible.
    this.showOSDControls(Ci.sbIOSDControlService.TRANSITION_NONE);
  },

  _onOSDWinMousedown: function(aEvent) {
    if (aEvent.button == 0) {
      this._mouseDownOnOSD = true;
    }
  },

  _onOSDWinMouseup: function(aEvent) {
    if (aEvent.button == 0) {
      // User released the mouse button, reset the timer
      this._mouseDownOnOSD = false;
      this.showOSDControls(Ci.sbIOSDControlService.TRANSITION_NONE);
    }
  },

  _onOSDWinKeypress: function(aEvent) {
    if (!aEvent.getPreventDefault())
    {
      let event = this._videoWindow.document.createEvent("KeyboardEvent");
      event.initKeyEvent(aEvent.type, aEvent.bubbles, aEvent.cancelable,
                         this._videoWindow, aEvent.ctrlKey, aEvent.altKey,
                         aEvent.shiftKey, aEvent.metaKey, aEvent.keyCode,
                         aEvent.charCode);
      this._videoWindow.dispatchEvent(event);
    }
  },

  //----------------------------------------------------------------------------
  // sbIWindowMoveListener

  onMoveStarted: function() {
    this._osdSurpressed = true;
    this._showOSDControlsOnStop = this._osdControlsShowing;
    this.hideOSDControls(Ci.sbIOSDControlService.TRANSITION_NONE);
  },

  onMoveStopped: function() {
    this._osdSurpressed = false;
    if (this._showOSDControlsOnStop) {
      this.showOSDControls(Ci.sbIOSDControlService.TRANSITION_NONE);
    }
    this._showOSDControlsOnStop = false;
  },

  //----------------------------------------------------------------------------
  // nsITimerCallback

  notify: function(aTimer) {
    if (aTimer == this._timer) {
      this.hideOSDControls(Ci.sbIOSDControlService.TRANSITION_FADE);
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

