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
const SONGBIRD_BUNDLE_CONTRACTID = "@songbirdnest.com/Songbird/Bundle;1";
const SONGBIRD_BUNDLE_CLASSNAME = "Songbird Bundle Service Interface";
const SONGBIRD_BUNDLE_CID = Components.ID("{ff29ec35-1294-42ae-a341-63d0303df969}");
const SONGBIRD_BUNDLE_IID = Components.interfaces.sbIBundle;

const SONGBIRD_GETBUNDLE_PREFKEY = "songbird.url.bundles";

function Bundle() {
  this._datalisteners = new Array();
  this._installlisteners = new Array();
}

Bundle.prototype.constructor = Bundle;

Bundle.prototype = {
  _bundleid: null,
  _req: null,
  _datalisteners: null,
  _installlisteners: null,
  _status: 0,
  _extlist: null,
  _browser: null,
  _downloadListener: null,
  _url: null,
  _file: null,
  _filename: null,
  _needrestart: false,
  _bundleversion: 0,
  _simulate_lots_of_entries: false,
  _init: false,
  _onload: null,
  _onerror: null,
  _installresult: -1,
  _timer: null,

  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },
  
  
  get bundleId() {
    return this._bundleid;
  },

  set bundleId(aStringValue) {
    // Make sure there is a string object to pass.
    if (aStringValue == null)
      aStringValue = "";
    this._bundleid = aStringValue;
  },
  
  retrieveBundleData: function(aTimeout) {
  
    if (this._init && this._req) {
      this._req.abort();
      var httpReq = this._req.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest);
      httpReq.removeEventListener("load", this._onload, false);
      httpReq.removeEventListener("error", this._onerror, false);
      this._req = null;
      this._status = SONGBIRD_BUNDLE_IID.BUNDLE_DATA_STATUS_DOWNLOADING;
    }
    
    this._onload = { 
      _that: null, 
      handleEvent: function( event ) { this._that.onLoad(); } 
    }; this._onload._that = this;
    
    this._onerror = { 
      _that: null, 
      handleEvent: function( event ) { this._that.onError(); } 
    }; this._onerror._that = this;
    
    this._req = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Components.interfaces.nsIXMLHttpRequest); 
    var httpReq = this._req.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest);
    httpReq.addEventListener("load", this._onload, false);
    httpReq.addEventListener("error", this._onerror, false);
    
    var prefsService =
        Components.classes["@mozilla.org/preferences-service;1"].
        getService(Components.interfaces.nsIPrefBranch);

    var baseURL = prefsService.getCharPref(SONGBIRD_GETBUNDLE_PREFKEY);
    var url = baseURL + this._bundleid;

    this._req.open('GET', url + this._getRandomParameter(), true); 
    this._req.send(null);
    this._init = true;

    // If specified, set up a callback to enforce request timeout
    if (aTimeout > 0) {
      this._timer = Components.classes["@mozilla.org/timer;1"]
                              .createInstance(Components.interfaces.nsITimer);
      this._timer.initWithCallback(this, aTimeout,
                                   Components.interfaces.nsITimer.TYPE_ONE_SHOT);
    }
  },

  get bundleDataDocument() {
    return this._req ? this._req.responseXML : null;
  },

  get bundleDataText() {
    return this._req ? this._req.responseText : "";
  },
  
  addBundleDataListener: function(aListener) {
    this._datalisteners.push(aListener);
  },
  
  removeBundleDataListener: function (aListener) {
    var r = this.getDataListenerIndex(aListener);
    if (r != -1) this._datalisteners.splice(r, 1);
  },
  
  getNumDataListeners: function() {
    return this._datalisteners.length;
  },
  
  getDataListener: function(aIndex) {
    return this._datalisteners[aIndex];
  },

  getDataListenerIndex: function(aListener) {
    return this._datalisteners.indexOf(aListener);
  },

  addBundleInstallListener: function(aListener) {
    this._installlisteners.push(aListener);
  },
  
  removeBundleInstallListener: function (aListener) {
    var r = this.getInstallListenerIndex(aListener);
    if (r != -1) this._installlisteners.splice(r, 1);
  },
  
  get installListenerCount() {
    return this._installlisteners.length;
  },
  
  getInstallListener: function(aIndex) {
    return this._installlisteners[aIndex];
  },
  
  getInstallaListenerIndex: function(aListener) {
    for (var i=0;i<this._installlisteners.length;i++) if (this._datalisteners[i] == aListener) return i;
    return -1;
  },
  
  get bundleDataStatus() {
    return this._status;
  },
  
  onLoad: function() {
    this._status = SONGBIRD_BUNDLE_IID.BUNDLE_DATA_STATUS_SUCCESS;
    this.getExtensionList();
    for (var i=0;i<this._datalisteners.length;i++) this._datalisteners[i].onDownloadComplete(this);
  },

  onError: function() {
    this._status = SONGBIRD_BUNDLE_IID.BUNDLE_DATA_STATUS_ERROR;
    for (var i=0;i<this._datalisteners.length;i++) this._datalisteners[i].onError(this);
  },
  
  getDataNodes: function(bundledocument) {
    if (!bundledocument) return null;
    var datablocknodes = bundledocument.childNodes;
    for (var i=0;i<datablocknodes.length;i++) {
      if (datablocknodes[i].tagName == "SongbirdInstallBundle") {
        this._bundleversion = datablocknodes[i].getAttribute("version")
        return datablocknodes[i].childNodes;
      }
    }
    return null;
  },

  installFlaggedExtensions: function(aWindow) {
    var windowWatcherService = Components.classes['@mozilla.org/embedcomp/window-watcher;1']
                            .getService(Components.interfaces.nsIWindowWatcher);
                            
    // TODO: do the install !
    this._installresult = "";
    windowWatcherService.openWindow(aWindow, "chrome://songbird/content/xul/setup_progress.xul", "_blank", "chrome,dialog=yes,centerscreen,alwaysRaised,close=no,modal", this);
    return this._installresult;
  },
  
  setInstallResult: function(aResult) {
    this._installresult = aResult;
  },
  
  getExtensionList: function() {
    this._extlist = new Array();
    if (this._status == SONGBIRD_BUNDLE_IID.BUNDLE_DATA_STATUS_SUCCESS) {
      bundledocument = this.bundleDataDocument;
      if (bundledocument) {
        var nodes = this.getDataNodes(bundledocument);
        if (nodes) {
          for (var i=0;i<nodes.length;i++) {
            if (nodes[i].tagName == "XPI") {
              var inst = nodes[i].getAttribute("default");
              this._extlist.push(Array(nodes[i], (inst=="true" || inst=="1")));
            }
          }
        }
      }
    }
  },
  
  get bundleExtensionCount() {
    if (this._status == SONGBIRD_BUNDLE_IID.BUNDLE_DATA_STATUS_SUCCESS) {
      if (this._simulate_lots_of_entries) return this._extlist.length * 20;
      return this._extlist.length;
    }
    return 0;
  },
  
  getExtensionAttribute: function(aIndex, aAttributeName) {
    if (!this._extlist) return "";
    if (this._extlist.length != 0 && this._simulate_lots_of_entries) 
      aIndex = aIndex % this._extlist.length;
    if (this._status == SONGBIRD_BUNDLE_IID.BUNDLE_DATA_STATUS_SUCCESS && aIndex < this.bundleExtensionCount) 
      return this._extlist[aIndex][0].getAttribute(aAttributeName);
    return "";
  },
      
  getExtensionInstallFlag: function(aIndex) {
    if (!this._extlist) return false;
    if (this._extlist.length != 0 && this._simulate_lots_of_entries) 
      aIndex = aIndex % this._extlist.length;
    if (this._status == SONGBIRD_BUNDLE_IID.BUNDLE_DATA_STATUS_SUCCESS && aIndex < this.bundleExtensionCount) 
      return this._extlist[aIndex][1];
    return false;
  },
  
  setExtensionInstallFlag: function(aIndex, aInstallFlag) {
    if (!this._extlist) return;
    if (this._extlist.length != 0 && this._simulate_lots_of_entries) 
      aIndex = aIndex % this._extlist.length;
    if (this._status == SONGBIRD_BUNDLE_IID.BUNDLE_DATA_STATUS_SUCCESS && aIndex < this.bundleExtensionCount) 
      this._extlist[aIndex][1] = aInstallFlag;
  },
  
  _getRandomParameter: function() {
    var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
    aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);
    var aUUID = aUUIDGenerator.generateUUID();
    return "?randomguid=" + escape(aUUID);
  },
  
  setNeedRestart: function(aRequired) {
    this._needrestart = aRequired;
  },

  get restartRequired() {
    return this._needrestart;
  },

  get bundleDataVersion() {
    return this._bundleversion;
  },
  
  // nsITimerCallback
  notify: function(timer)
  {
    if(this._req.readyState != 4) { // 4 = COMPLETED
      // abort() stops the http request so the normal event listeners are never
      // called so we need to call onError() manually.
      this._req.abort();
      this.onError();
    }
    this._timer = null;
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(SONGBIRD_BUNDLE_IID) &&
        !iid.equals(Components.interfaces.sbPIBundle) &&
        !iid.equals(Components.interfaces.nsIWebProgressListener) &&
        !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // Bundle.prototype

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
    // The Bundle Component
    bundle:     { CID        : SONGBIRD_BUNDLE_CID,
                  contractID : SONGBIRD_BUNDLE_CONTRACTID,
                  className  : SONGBIRD_BUNDLE_CLASSNAME,
                  factory    : #1#(Bundle)
                },
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule

