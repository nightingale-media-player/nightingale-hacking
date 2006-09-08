/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the “GPL”).
// 
// Software distributed under the License is distributed 
// on an “AS IS” basis, WITHOUT WARRANTY OF ANY KIND, either 
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
const SONGBIRD_HOTKEYACTIONS_CONTRACTID = "@songbirdnest.com/Songbird/HotkeyActions;1";
const SONGBIRD_HOTKEYACTIONS_CLASSNAME = "Songbird Hotkey Actions Service Interface";
const SONGBIRD_HOTKEYACTIONS_CID = Components.ID("{0BB80965-77C8-4212-866C-F22677F75A2C}");
const SONGBIRD_HOTKEYACTIONS_IID = Components.interfaces.sbIHotkeyActions;

function HotkeyActions() {
  this._bundles = new Array();
}

HotkeyActions.prototype.constructor = HotkeyActions;

HotkeyActions.prototype = {
  
  registerHotkeyActionBundle: function (bundle) {
    this._bundles.push(bundle);
  },
  
  unregisterHotkeyActionBundle: function (bundle) {
    var newarray = [];
    var n = this._bundles.length;
    for (var i=0;i<n;i++) {
      if (this._bundles[i] != bundle) newarray.push(this._bundles[i]);
    }
    this._bundles = newarray;
  },
  
  get_bundleCount: function () {
    return this._bundles.length;
  },
  
  enumBundle: function (idx) {
    return this._bundles[idx];
  },
 
  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(SONGBIRD_HOTKEYACTIONS_IID) &&
        !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
        !iid.equals(Components.interfaces.nsISupports)) {
      throw Components.results.NS_ERROR_NO_INTERFACE;
    }
    return this;
  }
}; // HotkeyActions.prototype

HotkeyActions.prototype.__defineGetter__( "bundleCount", HotkeyActions.prototype.get_bundleCount);

/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 * Adapted from nsBundleService.js
 */
var gModule = {
  registerSelf: function(componentManager, fileSpec, location, type) {
    componentManager = componentManager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    for (var key in this._objects) {
      var obj = this._objects[key];
      componentManager.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                               fileSpec, location, type);
    }
  },

  getClassObject: function(componentManager, cid, iid) {
    if (!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this._objects) {
      if (cid.equals(this._objects[key].CID))
        return this._objects[key].factory;
    }
    
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  _makeFactory: #1= function(ctor) {
    function ci(outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return (new ctor()).QueryInterface(iid);
    } 
    return { createInstance: ci };
  },
  
  _objects: {
    // The HotkeyActions Component
    hotkeyactions:     { CID        : SONGBIRD_HOTKEYACTIONS_CID,
                         contractID : SONGBIRD_HOTKEYACTIONS_CONTRACTID,
                         className  : SONGBIRD_HOTKEYACTIONS_CLASSNAME,
                         factory    : #1#(HotkeyActions)
                       },
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule