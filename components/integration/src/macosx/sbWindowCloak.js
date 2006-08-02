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

const SONGBIRD_WINDOWCLOAK_CONTRACTID = "@songbirdnest.com/Songbird/WindowCloak;1";
const SONGBIRD_WINDOWCLOAK_CLASSNAME = "Songbird Window Cloak Service Interface";
const SONGBIRD_WINDOWCLOAK_CID = Components.ID("{F1387DC0-4F82-4d8d-8671-C8EB03D27271}");
const SONGBIRD_WINDOWCLOAK_IID = Components.interfaces.sbIWindowCloak;

// THIS IS THE MAC VERSION.  IT MOVES THE WINDOW OFFSCREEN.

function WindowCloak() {
}

WindowCloak.prototype = {

  cloak: function(doc) {
	dump("sbWindowCloak: cloaking..\n");

	var win = this._getWindowFromDocument(doc);
	
	if (!win.cloaked) {
		win.preCloakScreenX = win.screenX;
		win.preCloakScreenY = win.screenY;	
		win.cloaked = true;	
		win.screenY = 5000;		
	}
  },


  uncloak: function(doc) {
	dump("sbWindowCloak: uncloaking..\n");

	var win = this._getWindowFromDocument(doc);
	win.screenX = win.preCloakScreenX;
	win.screenY = win.preCloakScreenY;
	win.cloaked = false;
  },


  _getWindowFromDocument: function(doc) {
	var acc = Components.classes["@mozilla.org/accessibilityService;1"]
                      .getService(Components.interfaces.nsIAccessibilityService);

	acc = acc.getAccessibleFor(doc).QueryInterface(Components.interfaces.nsIAccessNode).accessibleDocument;

    return acc.window.QueryInterface(Components.interfaces.nsIDOMJSWindow);
  },


  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(SONGBIRD_WINDOWCLOAK_IID) &&
        !iid.equals(Components.interfaces.nsIWebProgressListener) &&
        !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // WindowCloak.prototype


/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 * Adapted from nsMetricsService.js
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
    // The WindowCloak Component
    windowCloak:  { CID       : SONGBIRD_WINDOWCLOAK_CID,
                  contractID : SONGBIRD_WINDOWCLOAK_CONTRACTID,
                  className  : SONGBIRD_WINDOWCLOAK_CLASSNAME,
                  factory    : #1#(WindowCloak)
                },
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule
