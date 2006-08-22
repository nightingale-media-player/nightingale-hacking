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

/**
 * \file sbDataRemote.js
 * \brief Implementation of the interface sbIDataRemote
 * This implementation of sbIDataRemote uses the mozilla pref system as a
 *   backend for storing key-value pairs.
 * \sa sbIDataRemote.idl  sbIDataRemote.js
 */

const SONGBIRD_DATAREMOTE_CONTRACTID = "@songbirdnest.com/Songbird/DataRemote;1";
const SONGBIRD_DATAREMOTE_CLASSNAME = "Songbird Data Remote Service Interface";
const SONGBIRD_DATAREMOTE_CID = Components.ID("{9e6c2f00-b727-443f-88a6-1f4e2beb65cd}");
const SONGBIRD_DATAREMOTE_IID = Components.interfaces.sbIDataRemote;

function DataRemote() {
  // Nothing here...
}

// Define the prototype block before adding to the prototype object or the
//   additions will get blown away (at least the setters/getters were).
DataRemote.prototype = {
  _initialized: false,     // has init been called
  _observing: false,       // are we hooked up to the pref branch as a listener
  _prefBranch: null,       // the pref branch associated with the root
  _root: null,             // the root used to retrieve the pref branch
  _key: null,              // the section of the branch we care about ("Domain")
  _boundObserver: null,    // the object observing the change
  _boundElement: null,     // the element containing the linked prop/attr.
  _boundProperty: null,    // the property linked to the data (of boundElement)
  _boundAttribute: null,   // the attribute linked to the data (of boundElement)
  _isBool: false,          // Is the data a yes/no true/false chunk of data?
  _isNot: false,           // Is the linked data state opposite of target data?
  _evalString: "",         // a string of js to evaluate when the data changes

  init: function(aKey, aRoot) {
    // Only allow initialization once per object
    if (this._initialized)
      throw Components.results.NS_ERROR_UNEXPECTED;
      
    // Set the strings
    if (aRoot == null) {
      // The prefapi hashes fully qualified prefs, so using a simple root does not
      //   hurt us. Callbacks are in a (BIG) linked-list (ew), which sucks. Having
      //   a shorter root saves some strncmp() time.
      this._root = "songbird.";
      this._key = aKey;
    } else {
      // If a root is specified use that.
      this._root = aRoot;
      this._key = aKey;
    }

    // get the prefbranch for our root from the pref service
    var prefsService = Components.classes["@mozilla.org/preferences-service;1"]
                       .getService(Components.interfaces.nsIPrefService);
    this._prefBranch = prefsService.getBranch(this._root)
                  .QueryInterface(Components.interfaces.nsIPrefBranch2);
    if (!this._prefBranch)
      throw Components.results.NS_ERROR_FAILURE;

    this._initialized = true;
  },

  // only needs to be called if we have bound an attribute, property or observer
  unbind: function() {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    if (this._prefBranch && this._observing)
      this._prefBranch.removeObserver( this._key, this );

    // clear the decks
    this._observing = false;
    this._boundObserver = null;
    this._boundAttribute = null;
    this._boundProperty = null;
    this._boundElement = null;
  },
      
  bindObserver: function(aObserver, aSuppressFirst) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    // Clear and reinsert ourselves as an observer.
    if ( this._observing )
      this._prefBranch.removeObserver(this._key, this);

    // Temporary change in how we attach as observers
    //this._prefBranch.addObserver(this._key, this, true);
    this._prefBranch.addObserver(this._key, this, false);
    this._observing = true;

    // Now we are linked to an nsIObserver object
    this._boundObserver = aObserver;
    this._boundElement = null;
    this._boundProperty = null;
    this._boundAttribute = null;
    this._isBool = false;
    this._isNot = false;
    this._evalString = "";
    
    // If the caller wants to be notified, fire on attachment
    if (!aSuppressFirst)
      this.observe(null, null, this._key);
  },
     
  bindProperty: function(aElement, aProperty, aIsBool, aIsNot, aEvalString) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    if (!aIsBool)
      aIsBool = false;
    if (!aIsNot)
      aIsNot = false;
    if (!aEvalString)
      aEvalString = "";

    // Clear and reinsert ourselves as an observer.
    if ( this._observing )
      this._prefBranch.removeObserver(this._key, this);

    // Temporary change in how we attach as observers
    //this._prefBranch.addObserver(this._key, this, true);
    this._prefBranch.addObserver(this._key, this, false);
    this._observing = true;

    // Now we are linked to property on an element
    this._boundObserver = null;
    this._boundElement = aElement;
    this._boundProperty = aProperty;
    this._boundAttribute = null;
    this._isBool = aIsBool;
    this._isNot = aIsNot;
    this._evalString = aEvalString;
    
    // Set the value once
    this.observe(null, null, this._key);
  },
        
  bindAttribute: function(aElement, aAttribute, aIsBool, aIsNot, aEvalString) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    if (!aIsBool)
      aIsBool = false;
    if (!aIsNot)
      aIsNot = false;
    if (!aEvalString)
      aEvalString = "";

    // Clear and reinsert ourselves as an observer.
    if ( this._observing )
      this._prefBranch.removeObserver(this._key, this);

    // Temporary change in how we attach as observers
    //this._prefBranch.addObserver(this._key, this, true);
    this._prefBranch.addObserver(this._key, this, false);
    this._observing = true;
    
    // Now we are linked to an attribute on an element
    this._boundObserver = null;
    this._boundElement = aElement;
    this._boundProperty = null;
    this._boundAttribute = aAttribute;
    this._isBool = aIsBool;
    this._isNot = aIsNot;
    this._evalString = aEvalString;
    
    // Set the value once
    this.observe(null, null, this._key);
  },

  get stringValue() {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    return this._getValue();
  },

  set stringValue(aStringValue) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    // Make sure there is a string object to pass.
    if (aStringValue == null)
      aStringValue = "";
    this._setValue(aStringValue);
  },

  get boolValue() {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    return (this._makeIntValue(this._getValue()) != 0);
  },

  set boolValue(aBoolValue) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    // convert the bool to a numeric string for easy getBoolValue calls.
    aBoolValue = aBoolValue ? "1" : "0";
    this._setValue(aBoolValue);
  },

  get intValue() {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    return this._makeIntValue(this._getValue());
  },

  set intValue(aIntValue) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;

    this._setValue(aIntValue + "");
  },

  // internal helper function - all setters ultimately call this
  _setValue: function(aValueStr) {
    // assume we are being called after the init check in another method.

    // Make a unicode string, assign the value, set it into the preferences.
    var sString = Components.classes["@mozilla.org/supports-string;1"]
                            .createInstance(Components.interfaces.nsISupportsString);
    sString.data = aValueStr;
    this._prefBranch.setComplexValue(this._key,
                                Components.interfaces.nsISupportsString,
                                sString);
  },

  // internal helper function - all getters ultimately call this
  _getValue: function() {
    // assume we are being called after the init check in another method.

    var retval = "";
    if (this._prefBranch.prefHasUserValue(this._key)) {
      // need complexValue for unicode strings
      var prefValue = this._prefBranch.getComplexValue(this._key, Components.interfaces.nsISupportsString);
      if (prefValue != "") {
        retval = prefValue.data;
      }
    }
    return retval;
  },
      
  // internal function for converting to an integer.
  _makeIntValue: function(aValueStr) {
    var retval = 0;
    if (aValueStr && aValueStr.length)
      retval = parseInt(aValueStr);
    return retval;
  },

  // observe - Called when someone updates the remote data
  // aSubject: The prefbranch object
  // aTopic:   NS_PREFBRANCH_PREFCHANGE_TOPIC_ID
  // aData:    the domain (key)
  observe: function(aSubject, aTopic, aData) {
    if (!this._initialized)
      throw Components.results.NS_ERROR_NOT_INITIALIZED;
    
    // Early bail conditions
    if (aData != this._key)
      return;
    
    // Get the value as a string - this must be called value to not break
    //    evalStrings that do an eval of the value.
    var value = this._getValue();
    
    // Run the optional evaluation
    if (this._evalString.length)
      value = eval(this._evalString);
    
    // Handle boolean and not states
    if (this._isBool) {
      // If we were not a bool, make us one (_evalString could change value)
      if (typeof(value) != "boolean") {
        if (value == "true")
          value = true;
        else if (value == "false")
          value = false;
        else
          value = (this._makeIntValue(value) != 0);
      }
      // reverse ourself if neccessary
      if (this._isNot)
        value = !value;
    }

    // Handle callback states
    if (this._boundObserver) {
      try {
        // pass useful information to the observer.
        this._boundObserver.observe( this, "", value );
      }
      catch (err) {
        dump("ERROR! Could not call boundObserver.observe(). Key = " + this._key + "\n" + err + "\n");
      }
    }
    else if (this._boundElement && this._boundProperty) {
      // Set the property of the callback object
      this._boundElement[this._boundProperty] = value;
    }
    else if (this._boundElement && this._boundAttribute) {
      // Set the attribute of the callback object
      var valStr = value;
      // If bool-type, convert to string.
      if (this._isBool) {
        if (value)
          valStr = "true";
        else
          valStr = "false";
      }
      try {
        this._boundElement.setAttribute(this._boundAttribute, valStr);
      }
      catch (err) {
        dump("ERROR! Could not setAttribute in sbDataRemote.js\n " + err + "\n");
      }
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

// be specific
DataRemote.prototype.constructor = DataRemote;

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

