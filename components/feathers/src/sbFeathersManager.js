/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
 * \file sbFeathersManager.js
 * \brief Coordinates the loading of feathers (combination of skin and XUL window layout)
 */ 
 
 
//
// TODO:
//  * Add doxygen comments
//  * Make skin switching actually do something
//  * Add function arg assertions
//  * add onSwitchCompleted, change onSwitchRequested to allow feedback
// 
 
const CONTRACTID = "@songbirdnest.com/songbird/feathersmanager;1";
const CLASSNAME = "Songbird Feathers Manager Service Interface";
const CID = Components.ID("{99f24350-a67f-11db-befa-0800200c9a66}");
const IID = Components.interfaces.sbIFeathersManager;


/**
 * /class ArrayEnumerator
 * /brief Converts a js array into an nsISimpleEnumerator
 */
function ArrayEnumerator(array) 
{
  this.data = array;
}
ArrayEnumerator.prototype = {

  index: 0,
  
  getNext: function() {
    return this.data[this.index++];
  },
  
  hasMoreElements: function() {            
    if (this.index < this.data.length)
      return true;
    else
      return false;
  },
  
  QueryInterface: function(iid)
  {
    if (!iid.equals(Components.interfaces.nsISimpleEnumerator) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}




/**
 * /class FeathersManager
 * /brief Coordinates the loading of feathers
 *
 * Acts as a registry for skins and layout (known as feathers)
 * and manages compatibility and selection.
 */
function FeathersManager() {

  os      = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
  // We should wait until the profile has been loaded to start
  os.addObserver(this, "profile-after-change", false);
  // We need to unhook things on shutdown
  os.addObserver(this, "xpcom-shutdown", false);
  
  this._skins = {};
  this._layouts = {};
  this._mappings = {};
  this._listeners = [];
};
FeathersManager.prototype = {
  constructor: FeathersManager,
  
  _currentSkinProvider: null,
  _currentLayoutURL: null,
  
  // Hash of skin descriptions keyed by provider (e.g. classic/1.0)
  _skins: null,
  
  // Hash of layout descriptions keyed by URL
  _layouts: null,
  
  
  // Hash of layout URL to hash of compatible skin providers, pointing to 
  // showChrome value.  
  //
  // eg
  // {  
  //     mainwin.xul: {
  //       blueskin: true,
  //       redskin: false,
  //     }
  // }
  //
  // Compatibility is determined by whether or not a skinProvider
  // key is *defined* in the hash, not the actual value it points to.
  // In the above example false means "don't show chrome"
  _mappings: null,
  
  
  // Array of sbIFeathersChangeListeners
  _listeners: null,

  _layoutCount: 0,
  _skinCount: 0,
  

  _init: function FeathersManager_init() {
    // Hmm, don't care?
  },
  
  _deinit: function FeathersManager_deinit() {
      this._skins = null;
      this._layouts = null;
      this._mappings = null;
      this._listeners = null;
  },
  
  
  
  get currentSkinProvider() {
    return this._currentSkinProvider;
  },
  
  get currentLayoutURL() {
    return this._currentLayoutURL;
  },
  
  get skinCount() {
    return this._skinCount;
  },
  
  get layoutCount() {
    return this._layoutCount;
  },

  getSkinDescriptions: function FeathersManager_getSkinDescriptions() {
    // Copy all the descriptions into an array, and then return an enumerator
    return new ArrayEnumerator( [this._skins[key] for (key in this._skins)] );
  },

  getLayoutDescriptions: function FeathersManager_getLayoutDescriptions() {
    // Copy all the descriptions into an array, and then return an enumerator
    return new ArrayEnumerator( [this._layouts[key] for (key in this._layouts)] );
  },
  
  
  
  registerSkin: function FeathersManager_registerSkin(skinDesc) {
    // TODO: Verify desc
    
    if (this._skins[skinDesc.provider] == null) {
      this._skinCount++;
    }
    this._skins[skinDesc.provider] = skinDesc;
    
    // Notify observers
    this._onUpdate();
  },

  unregisterSkin: function FeathersManager_unregisterSkin(skinDesc) {
    if (this._skins[skinDesc.provider]) {
      delete this._skins[skinDesc.provider];
      this._skinCount--;
      
      // Notify observers
      this._onUpdate();
    }
  },

  getSkinDescription: function FeathersManager_getSkinDescription(provider) {
    return this._skins[provider];
  },
  
  
  
  registerLayout: function FeathersManager_registerLayout(layoutDesc) {
    // TODO: Verify desc
   
    if (this._layouts[layoutDesc.url] == null) {
      this._layoutCount++;
    }
    this._layouts[layoutDesc.url] = layoutDesc;
    
    // Notify observers
    this._onUpdate();
  },

  unregisterLayout: function FeathersManager_unregisterLayout(layoutDesc) {
    if (this._layouts[layoutDesc.url]) {
      delete this._layouts[layoutDesc.url];
      this._layoutCount--;
      
      // Notify observers
      this._onUpdate();  
    }  
  },
    
  getLayoutDescription: function FeathersManager_getLayoutDescription(url) {
    return this._layouts[url];
  }, 
  


  assertCompatibility: function FeathersManager_assertCompatibility(layoutURL, skinProvider, showChrome) {
    // TODO verify
    
    if (this._mappings[layoutURL] == null) {
      this._mappings[layoutURL] = {};
    }
    this._mappings[layoutURL][skinProvider] = showChrome;
    
    // Notify observers
    this._onUpdate();    
  },

  unassertCompatibility: function FeathersManager_unassertCompatibility(layoutURL, skinProvider) {
    if (this._mappings[layoutURL]) {
      delete this._mappings[layoutURL][skinProvider];
      
      // Notify observers
      this._onUpdate();  
    }  
  },
  


  isChromeEnabled: function FeathersManager_isChromeEnabled(layoutURL, skinProvider) {
    if (this._mappings[layoutURL]) {
      return this._mappings[layoutURL][skinProvider] == true;
    }
   
    return false; 
  },



  getSkinsForLayout: function FeathersManager_getSkinsForLayout(layoutURL) {
    var skins = [];
    
    // Find skin descriptions that are compatible with the given layout.
    if (this._mappings[layoutURL]) {
      for (skinProvider in this._mappings[layoutURL]) {
        var desc = this.getSkinDescription(skinProvider);
        if (desc) {
          skins.push(desc);
        }
      }
    }   
    return new ArrayEnumerator( skins );
  },



  switchFeathers: function FeathersManager_switchFeathers(layoutURL, skinProvider) {
    layoutDescription = this.getLayoutDescription(layoutURL);
    skinDescription = this.getSkinDescription(skinProvider);
    
    // Make sure we know about the requested skin and layout
    if (layoutDescription == null || skinDescription == null) {
      throw("Unknown layout/skin passed to switchFeathers");
    }
    
    // Check compatibility.
    // True/false refer to the showChrome value, so check for undefined
    // to determine compatibility.
    if (this._mappings[layoutURL][skinProvider] === undefined) {
      throw("Skin [" + skinProvider + "] and Layout [" + layoutURL +
            " are not compatible");
    } 
    
    // Notify that a select is about to occur
    this._onSelect(layoutDescription, skinDescription);
    
    // TODO!
    // Can we get away with killing the main window, or do we have to completely restart?
    // Modify dataremotes and keep a copy of the previous values.
  },



  addListener: function FeathersManager_addListener(listener) {
    this._listeners.push(listener);
  },
  
  
  removeListener: function FeathersManager_removeListener(listener) {
    var index = this._listeners.indexOf(listener);
    if (index > -1) {
      this._listeners.splice(index,1);
    }
  },


  _onUpdate: function FeathersManager_onUpdate() {
    this._listeners.forEach( function (listener) {
      listener.onFeathersUpdate();
    });
  },


  _onSelect: function FeathersManager_onSelect(layoutDesc, skinDesc) {
    // Verify args
    layoutDesc = layoutDesc.QueryInterface(Components.interfaces.sbILayoutDescription);
    skinDesc = skinDesc.QueryInterface(Components.interfaces.sbISkinDescription);
    
    // Broadcast notification
    this._listeners.forEach( function (listener) {
      listener.onFeathersSelectRequest(layoutDesc, skinDesc);
    });
  },

      

  // watch for XRE startup and shutdown messages 
  observe: function(subject, topic, data) {
    os      = Components.classes["@mozilla.org/observer-service;1"]
                      .getService(Components.interfaces.nsIObserverService);
    switch (topic) {
    case "profile-after-change":
      os.removeObserver(this, "profile-after-change");
      
      // Preferences are initialized, ready to start the service
      this._init();
      break;
    case "xpcom-shutdown":
      os.removeObserver(this, "xpcom-shutdown");
      this._deinit();
      break;
    }
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(IID) &&
        !iid.equals(Components.interfaces.nsIObserver) && 
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // FeathersManager.prototype





/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 */
var gModule = {
  registerSelf: function(componentManager, fileSpec, location, type) {
    componentManager = componentManager.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    for (var key in this._objects) {
      var obj = this._objects[key];
      componentManager.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                               fileSpec, location, type);
    }
    var categoryManager = Components.classes["@mozilla.org/categorymanager;1"]
                                    .getService(Components.interfaces.nsICategoryManager);
    categoryManager.addCategoryEntry("app-startup", this._objects.feathersmanager.className,
                                    "service," + this._objects.feathersmanager.contractID, 
                                    true, true, null);
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
    // The FeathersManager Component
    feathersmanager:     { CID        : CID,
                           contractID : CONTRACTID,
                           className  : CLASSNAME,
                           factory    : #1#(FeathersManager)
                         },
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule


