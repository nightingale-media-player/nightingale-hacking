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

const SB_OSDHIDE_DELAY = 3000;


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
}

sbOSDControlService.prototype =
{
  _videoWindow:  null,
  _osdWindow:    null,
  _cloakService: null,
  _nativeWinMgr: null,
  _timer:        null,


  _recalcOSDPosition: function() {
    this._osdWindow.moveTo(this._videoWindow.screenX,
                           this._videoWindow.screenY);
    this._osdWindow.resizeTo(this._videoWindow.innerWidth,
                             this._videoWindow.outerHeight);
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

    // XXX KREEGER TODO:
    //  -- Remove on-top status when the video window loses focus!
    this._nativeWinMgr.setOnTop(this._osdWindow, true);

    // Listen for window move events on the video window.
    try {
      // XXXkreeger Until the window move service is implemented on all
      // platforms, wrap this call in a try/catch block.
      var winMoveService =
        Cc["@songbirdnest.com/integration/window-move-resize-service;1"]
        .getService(Ci.sbIWindowMoveService);

      winMoveService.startWatchingWindow(this._videoWindow, this);
    }
    catch (e) {
      Cu.reportError(
        "The sbIWindowMoveService is not implemented on this platform!");
    }
  },

  onVideoWindowWillClose: function() {
    this._timer.cancel();
    this._osdWindow.close();
    this._osdWindow = null;
    this._videoWindow = null;
  },

  onVideoWindowResized: function() {
    this._recalcOSDPosition();
  },

  hideOSDControls: function() {
    if (!this._cloakService.isCloaked(this._osdWindow)) {
      this._cloakService.cloak(this._osdWindow);
    }
  },

  showOSDControls: function() {
    // Show the controls if they are currently hidden.
    if (this._cloakService.isCloaked(this._osdWindow)) {
      this._cloakService.uncloak(this._osdWindow);
    }

    this._recalcOSDPosition();

    // Set the timer for hiding.
    this._timer.initWithCallback(this,
                                 SB_OSDHIDE_DELAY,
                                 Ci.nsITimer.TYPE_ONE_SHOT);
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
    this.hideOSDControls();
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

