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
    this._mediacoreManager = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                               .getService(Ci.sbIMediacoreManager);
    this._videoFullscreenDataRemote = SBNewDataRemote(this.VIDEO_FULLSCREEN_DR_KEY);
  },
  
  _shutdown: function vcc__shutdown() {
    this._mediacoreManager = null;
    this._videoFullscreenDataRemote = null;
  }
};
