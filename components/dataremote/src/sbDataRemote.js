/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright© 2006 Pioneers of the Inevitable LLC
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
const SONGBIRD_DATAREMOTE_CONTRACTID = "@songbird.org/Songbird/DataRemote;1";
const SONGBIRD_DATAREMOTE_CLASSNAME = "Songbird Data Remote Service Interface";
const SONGBIRD_DATAREMOTE_CID = Components.ID("{79eece3f-d661-4f03-83dd-5764a0f77088}");
const SONGBIRD_DATAREMOTE_IID = Components.interfaces.sbIDataRemote;

function DataRemote() {
  // Nothing here...
}
DataRemote.prototype.constructor = DataRemote;

DataRemote.prototype = {
  _prefs: null,
  _initialized: false,
  _root: null,
  _key: null,
  _callbackFunction: null,
  _callbackObject: null,
  _callbackPropery: null,
  _callbackAttribute: null,
  _callbackBool: false,
  _callbackNot: false,
  _callbackEval: "",
  _suppressFirst: false,
  _observing: false,

  init: function(key, root) {
    // Only allow initialization once per object
    if (this._initialized)
      throw Components.results.NS_ERROR_UNEXPECTED;
      
    // Set the strings
    if (root == null) {
    
      // Use key in root string, makes unique observer lists per key (big but fast?).
      this._root = "songbird.dataremotes." + key;
      this._key = key;
/*        

  AUGH!
  
  So, this code here doesn't actually work in the script version.
  
  So, to be compatible, we can't do this.

      this._root = "";
      this._key = key;
*/      
    } else {
      // If we're specifying a root, just obey what's asked for.
      this._root = root;
      this._key = key;
    }
    var prefsService = Components.classes["@mozilla.org/preferences-service;1"]
                                 .getService(Components.interfaces.nsIPrefService);
    // Ask for the branch.
    this._prefs = prefsService.getBranch(this._root)
                              .QueryInterface(Components.interfaces.nsIPrefBranch2);
    if (!this._prefs)
      throw Components.results.NS_ERROR_FAILURE;
      
    this._initialized = true;
  },
        
  unbind: function() {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    if (this._prefs && this._observing)
      this._prefs.removeObserver( this._key, this );
    this._observing = false;
  },
      
  bindEventFunction: function(func) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    this.bindCallbackFunction(func, true);
  },
      
  bindCallbackFunction: function(func, suppressFirst) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    // Don't call the function the first time it is bound
    this._suppressFirst = suppressFirst;
  
    // Clear and reinsert ourselves as an observer.
    if ( this._observing )
      this._prefs.removeObserver(this._key, this);
    this._prefs.addObserver(this._key, this, true);
    this._observing = true;

    // Now we're observing for a function.        
    this._callbackFunction = func;
    this._callbackObject = null;
    this._callbackPropery = null;
    this._callbackAttribute = null;
    this._callbackBool = false;
    this._callbackNot = false;
    this._callbackEval = "";
    
    // Set the value once
    this.observe(null, null, this._key);
  },
      
  bindCallbackProperty: function(obj, prop, bool, not, eval) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    if (!bool)
      bool = false;
    if (!not)
      not = false;
    if (!eval)
      eval = "";

    // Clear and reinsert ourselves as an observer.
    if ( this._observing )
      this._prefs.removeObserver(this._key, this);
    this._prefs.addObserver(this._key, this, true);
    this._observing = true;

    // Now we're observing for an object's property.        
    this._callbackFunction = null;
    this._callbackObject = obj;
    this._callbackPropery = prop;
    this._callbackAttribute = null;
    this._callbackBool = bool;
    this._callbackNot = not;
    this._callbackEval = eval;
    
    // Set the value once
    this.observe(null, null, this._key);
  },
        
  bindCallbackAttribute: function(obj, attr, bool, not, eval) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    if (!bool)
      bool = false;
    if (!not)
      not = false;
    if (!eval)
      eval = "";

    // Clear and reinsert ourselves as an observer.
    if ( this._observing )
      this._prefs.removeObserver(this._key, this);
    this._prefs.addObserver(this._key, this, true);
    this._observing = true;
    
    // Now we're observing for an object's attribute.        
    this._callbackFunction = null;
    this._callbackObject = obj;
    this._callbackPropery = null;
    this._callbackAttribute = attr;
    this._callbackBool = bool;
    this._callbackNot = not;
    this._callbackEval = eval;
    
    // Set the value once
    this.observe(null, null, this._key);
  },

      // SetValue - Put the value into the data store, alert everyone watching this data    
  setValue: function(value) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    
    // Clear the value
    if (value == null)
      value = "";

    // Make a unicode string, assign the value, set it into the preferences.
    var sString = Components.classes["@mozilla.org/supports-string;1"]
                            .createInstance(Components.interfaces.nsISupportsString);
    sString.data = value;
    this._prefs.setComplexValue(this._key,
                                Components.interfaces.nsISupportsString,
                                sString);
  },

      // setBoolValue - Set the value because the js turns true into "true"
  setBoolValue: function(value) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    if (value)
      value = "1";
    else
      value = "0";
    return this.setValue(value);
  },    
  
      // GetValue - Get the value from the data store
  getValue: function() {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
      
    var retval = "";
    if (this._prefs.prefHasUserValue(this._key)) {
      var prefValue = this._prefs.getComplexValue(this._key, Components.interfaces.nsISupportsString);
      if (prefValue)
        retval = prefValue.data;
    }
    return retval;
  },
      
      // GetIntValue - Get the value from the data store as an int
  getIntValue: function() {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return this.makeIntValue(this.getValue());
  },
      
      // GetBoolValue - Get the value from the data store as an int
  getBoolValue: function() {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    return this.makeIntValue(this.getValue()) != 0;
  },
      
      // MakeIntValue - Get the value from the data store as an int
  makeIntValue: function(value) {
    var retval = 0;
    if (value && value.length)
      retval = parseInt(value);
    return retval;
  },

  // observe - Called when someone updates the remote data
  observe: function(subject, topic, data) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    
    // Early bail conditions
    if (data != this._key)
      return;
    if (this._suppressFirst) {
      this._suppressFirst = false;
      return;
    }
    
    // Get the value (why isn't this a param?)
    var value = this.getValue();
    
    // Run the optional evaluation
    if (this._callbackEval.length)
      value = eval(this._callbackEval);
    
    // Handle boolean and not states
    if (this.callbackBool) {
      // If we're not bool before,
      if(typeof(value) != "boolean") {
        if (value == "true")
          value = true;
        else if (value == "false")
          value = false;
        else
          value = (this.makeIntValue(value) != 0);
      }
      // ...we are now!
      if (this._callbackNot)
        value = !value;
    }
    
    // Handle callback states
    if (this._callbackFunction) {
      // Call the callback function
      this._callbackFunction(value);
    }
    else if (this._callbackObject && this._callbackPropery) {
      // Set the callback object's property
      this._callbackObject[this._callbackPropery] = value;
    }
    else if (this._callbackObject && this._callbackAttribute) {
      var valStr = value;
      // If bool-type, convert to string.
      if (this._callbackBool) {
        if (value)
          valStr = "true";
        else
          valStr = "false";
      }
      // Set the callback object's attribute
      this._callbackObject.setAttribute(this._callbackAttribute, valStr);
    }
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(SONGBIRD_DATAREMOTE_IID) &&
        !iid.equals(Components.interfaces.nsIObserver) && 
        !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // DataRemote.prototype

/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 * Adapted from nsUpdateService.js
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
    // The DataRemote Component
    dataremote: { CID        : SONGBIRD_DATAREMOTE_CID,
                  contractID : SONGBIRD_DATAREMOTE_CONTRACTID,
                  className  : SONGBIRD_DATAREMOTE_CLASSNAME,
                  factory    : #1#(DataRemote)
                },
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule