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

  _actualSizeDataRemote: null,

  _lastActualSize: null,

  //////////////////////////////////////////////////////////////////////////////
  // Getter
  //////////////////////////////////////////////////////////////////////////////

  get ACTUAL_SIZE_DR_KEY() {
    const dataRemoteKey = "videowindow.actualsize";
    return dataRemoteKey;
  },

  //////////////////////////////////////////////////////////////////////////////
  // Public Methods
  //////////////////////////////////////////////////////////////////////////////
  
  toggleFullscreen: function vcc_toggleFullscreen() {
    var video = this._mediacoreManager.video;
    video.fullscreen = !video.fullscreen;
    if (video.fullscreen) {
      this._lastActualSize = this._actualSizeDataRemote.boolValue;
      this._actualSizeDataRemote.boolValue = false;
    }
    else {
      this._actualSizeDataRemote.boolValue = this._lastActualSize;
    }
  },

  //////////////////////////////////////////////////////////////////////////////
  // Init/Shutdown
  //////////////////////////////////////////////////////////////////////////////
  
  _initialize: function vcc__initialize() {
    this._mediacoreManager = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                               .getService(Ci.sbIMediacoreManager);
    this._actualSizeDataRemote = SBNewDataRemote(this.ACTUAL_SIZE_DR_KEY);
    this._lastActualSize = this._actualSizeDataRemote.boolValue;
  },
  
  _shutdown: function vcc__shutdown() {
    this._mediacoreManager = null;
    this._actualSizeDataRemote = null;
    this._lastActualSize = null;
  }
};
