/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \file videoWindowControls.js
 * \brief Video Controls controller.
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
  
var videoControlsController = {
  //////////////////////////////////////////////////////////////////////////////
  // Internal data
  //////////////////////////////////////////////////////////////////////////////
  _mediacoreManager: null,

  _videoFullscreenDataRemote: null,

  //////////////////////////////////////////////////////////////////////////////
  // Getter
  //////////////////////////////////////////////////////////////////////////////

  get VIDEO_FULLSCREEN_DR_KEY() {
    const dataRemoteKey = "video.fullscreen";
    return dataRemoteKey;
  },
  
  //////////////////////////////////////////////////////////////////////////////
  // Public Methods
  //////////////////////////////////////////////////////////////////////////////
  
  toggleFullscreen: function vcc_toggleFullscreen() {
    this._videoFullscreenDataRemote.boolValue =
      !this._videoFullscreenDataRemote.boolValue;
  },

  //////////////////////////////////////////////////////////////////////////////
  // Init/Shutdown
  //////////////////////////////////////////////////////////////////////////////
  
  _initialize: function vcc__initialize() {
    this._mediacoreManager =
           Cc["@getnightingale.com/Nightingale/Mediacore/Manager;1"]
             .getService(Ci.sbIMediacoreManager);
    this._videoFullscreenDataRemote =
           SBNewDataRemote(this.VIDEO_FULLSCREEN_DR_KEY);

    var useTransparentGraphics = true;
    if ("@getnightingale.com/Nightingale/WindowChromeService;1" in Cc) {
      var winChromeService =
        Cc["@getnightingale.com/Nightingale/WindowChromeService;1"]
          .getService(Ci.sbIWindowChromeService);
      useTransparentGraphics = winChromeService.isCompositionEnabled;
    }

    if (useTransparentGraphics) {
      var osdWindow = document.getElementById("video_osd_controls_win");
      osdWindow.setAttribute("transparent", true);
    }
  },
  
  _shutdown: function vcc__shutdown() {
    this._mediacoreManager = null;
    this._videoFullscreenDataRemote = null;
  }
};
