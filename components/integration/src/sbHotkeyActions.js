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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const SONGBIRD_HOTKEYACTIONS_CONTRACTID = "@songbirdnest.com/Songbird/HotkeyActions;1";
const SONGBIRD_HOTKEYACTIONS_CLASSNAME = "Songbird Hotkey Actions Service Interface";
const SONGBIRD_HOTKEYACTIONS_CID = Components.ID("{0bb80965-77c8-4212-866c-f22677f75a2c}");

function HotkeyActions() {
  this._bundles = new Array();
  Cc["@mozilla.org/observer-service;1"]
    .getService(Ci.nsIObserverService)
    .addObserver(this, "quit-application", true);
}

HotkeyActions.prototype = {
  
  registerHotkeyActionBundle: function (bundle) {
    this._bundles.push(bundle);
  },
  
  unregisterHotkeyActionBundle: function (bundle) {
    var idx = this._bundles.indexOf(bundle);
    while (idx != -1) {
      this._bundles.splice(idx, 1);
      idx = this._bundles.indexOf(bundle, idx);
    }
  },
  
  get bundleCount () {
    return this._bundles.length;
  },
  
  enumBundle: function (idx) {
    return this._bundles[idx];
  },

  /**
   * nsIObserver
   */
  observe: function HotkeyActions_observe (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "quit-application": {
        this._bundles = [];
        Cc["@mozilla.org/observer-service;1"]
          .getService(Ci.nsIObserverService)
          .removeObserver(this, aTopic);
      }
    }
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIHotkeyActions,
                                         Ci.nsIObserver,
                                         Ci.nsISupportsWeakReference]),
  
  classDescription: SONGBIRD_HOTKEYACTIONS_CLASSNAME,
  classID: SONGBIRD_HOTKEYACTIONS_CID,
  contractID: SONGBIRD_HOTKEYACTIONS_CONTRACTID
}; // HotkeyActions.prototype

var NSGetFactory = XPCOMUtils.generateNSGetFactory([HotkeyActions]);
