/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2006 POTI, Inc.
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
const SONGBIRD_FACEPLATEREGISTRATION_CONTRACTID = "@songbirdnest.com/Songbird/FaceplateRegistration;1";
const SONGBIRD_FACEPLATEREGISTRATION_CLASSNAME = "Songbird Faceplate Registration Service Interface";
const SONGBIRD_FACEPLATEREGISTRATION_CID = Components.ID("{0d3c6e4e-6f94-11db-968e-00e08161165f}");
const SONGBIRD_FACEPLATEREGISTRATION_IID = Components.interfaces.sbIFaceplateRegistration;
const SONGBIRD_DATAREMOTE_CONTRACTID = "@songbirdnest.com/Songbird/DataRemote;1";
const SONGBIRD_DATAREMOTE_IID = Components.interfaces.sbIDataRemote;

var gOS = null;

function FaceplateRegistration() {
  try {
    gOS      = Components.classes["@mozilla.org/observer-service;1"]
                        .getService(Components.interfaces.nsIObserverService);
    
    if (gOS.addObserver) {
      // We should wait until the profile has been loaded to start
      gOS.addObserver(this, "profile-after-change", false);
      // We need to unhook things on shutdown
      gOS.addObserver(this, "xpcom-shutdown", false);
    }
  } catch (e) { }
}

FaceplateRegistration.prototype.constructor = FaceplateRegistration;

FaceplateRegistration.prototype = {

  remote_npages: null,  
  createDataRemote: null,
  
  _init: function() {
    this._panes = new Array();
    this.createDataRemote = new Components.Constructor( SONGBIRD_DATAREMOTE_CONTRACTID, SONGBIRD_DATAREMOTE_IID, "init");
    this.remote_npages = this.createDataRemote("faceplate.panes", null);
    this.remote_npages.intValue = 0;
  },
  
  _deinit: function() {
    this.remote_npages.unbind();
  },

  registerPane: function (paneid, panename) {
    
    for (var i=0;i<this._panes.length;i++) {
      if (this._panes[i].getPaneId() == paneid) {
        this._panes[i]._incRefCount();
        return this._panes[i];
      }
    }
    
    var pane = {
      id                             : "",
      name                           : "",
      refcount                       : 1,
      manager                        : null,
      createDataRemote               : null,
      remote_label1                  : null,
      remote_label1_hidden           : null,
      remote_label2                  : null,
      remote_label2_hidden           : null,
      remote_progressmeter           : null,
      remote_progressmeter_hidden    : null,
      
      _init: function(createDataRemote) {
        this.remote_label1 = this.createDataRemote("faceplate." + this.id + ".label1", null);
        this.remote_label1_hidden = this.createDataRemote("faceplate." + this.id + ".label1.hidden", null);
        this.remote_label2 = this.createDataRemote("faceplate." + this.id + ".label2", null);
        this.remote_label2_hidden = this.createDataRemote("faceplate." + this.id + ".label2.hidden", null);
        this.remote_progressmeter = this.createDataRemote("faceplate." + this.id + ".progressmeter", null);
        this.remote_progressmeter_hidden = this.createDataRemote("faceplate." + this.id + ".progressmeter.hidden", null);
        this.remote_label1.stringValue = "";
        this.remote_label1_hidden.boolValue = false;
        this.remote_label2.stringValue = "";
        this.remote_label2_hidden.boolValue = false;
        this.remote_progressmeter.intValue = 0;
        this.remote_progressmeter_hidden.boolValue = false;
      },
      
      // sbIFaceplatePane
      
      getPaneId: function() { return this.id; },
      setLabel1: function(text) {
        this.remote_label1.stringValue = text;
      },
      getPaneName: function() { return this.name; },
      showLabel1: function() {
        this.remote_label1_hidden.boolValue = false;
      },
      hideLabel1: function() {
        this.remote_label1_hidden.boolValue = true;
      },
      setLabel2: function(text) {
        this.remote_label2.stringValue = text;
      },
      showLabel2: function() {
        this.remote_label2_hidden.boolValue = false;
      },
      hideLabel2: function() {
        this.remote_label2_hidden.boolValue = true;
      },
      setProgressMeter: function(value) {
        this.remote_progressmeter.intValue = value;
      },
      showProgressMeter: function() {
        this.remote_progressmeter_hidden.boolValue = false;
      },
      hideProgressMeter: function() {
        this.remote_progressmeter_hidden.boolValue = true;
      },
      
      // private
      
      _setPaneId: function(id) { this.id = id; },
      _setPaneName: function(name) { this.name = name; },
      _incRefCount: function() { return this.refcount++; },
      _decRefCount: function() { return --this.refcount; },
      _setManager: function(manager) { this.manager = manager; },
      
      // xpcom
      
      QueryInterface: function(iid) {
        if (!iid.equals(Components.interfaces.sbIFaceplatePane) &&
            !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
            !iid.equals(Components.interfaces.nsISupports))
          throw Components.results.NS_ERROR_NO_INTERFACE;
        return this;
      }
    };
    pane.createDataRemote = this.createDataRemote;
    pane._setManager(this);
    pane._setPaneId(paneid);
    pane._setPaneName(panename);
    pane._init(this.createDataRemote);
    this._panes.push(pane);
    this.remote_npages.intValue = this._panes.length; // creates ui panes
    return pane;
  },
  
  unregisterPane: function(paneid) {
    for (var i=0;i<this._panes.length;i++) {
      if (this._panes[i].getPaneId() == paneid) {
        if (this._panes[i]._decRefCount() == 0) {
          this._panes.splice(i, 1);
        }
      }
    }
    this.remote_npages.intValue = this._panes.length;
  },
  
  getNumPanes: function() {
    return this._panes.length;
  },
  
  enumPane: function(pane) {
    return this._panes[pane];
  },
  
  // watch for XRE startup and shutdown messages 
  observe: function(subject, topic, data) {
    switch (topic) {
    case "profile-after-change":
      gOS.removeObserver(this, "profile-after-change");
      
      // Preferences are initialized, ready to start the service
      this._init();
      break;
    case "xpcom-shutdown":
      gOS.removeObserver(this, "xpcom-shutdown");
      this._deinit();
      
      // Release Services to avoid memory leaks
      gOS       = null;
      break;
    }
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(SONGBIRD_FACEPLATEREGISTRATION_IID) &&
        !iid.equals(Components.interfaces.nsIObserver) && 
        !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // FaceplateRegistration.prototype

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
    categoryManager.addCategoryEntry("app-startup", this._objects.faceplateregistration.className,
                                    "service," + this._objects.faceplateregistration.contractID, 
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
    // The FaceplateRegistration Component
    faceplateregistration:     { CID        : SONGBIRD_FACEPLATEREGISTRATION_CID,
                                 contractID : SONGBIRD_FACEPLATEREGISTRATION_CONTRACTID,
                                 className  : SONGBIRD_FACEPLATEREGISTRATION_CLASSNAME,
                                 factory    : #1#(FaceplateRegistration)
                               },
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule


