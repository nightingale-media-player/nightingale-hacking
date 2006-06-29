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

const SONGBIRD_METRICS_CONTRACTID = "@songbirdnest.com/Songbird/Metrics;1";
const SONGBIRD_METRICS_CLASSNAME = "Songbird Metrics Service Interface";
const SONGBIRD_METRICS_CID = Components.ID("{1066527d-b135-4e0c-9ea4-f6109ae97d02}");
const SONGBIRD_METRICS_IID = Components.interfaces.sbIMetrics;

const SONGBIRD_POSTMETRICS_URL = 'http://metrics.songbirdnest.com/post';
//const SONGBIRD_POSTMETRICS_URL = 'http://192.168.239.128:3000/metrics/post';

const SONGBIRD_UPLOAD_METRICS_EVERY_NDAYS = 7; // every week

function Metrics() {
    this.prefs = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefBranch);
}

Metrics.prototype = {
  _postreq: null,
  
  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },


  /**
   * Check to see if metrics should be submitted.
   */
  checkUploadMetrics: function()
  {
    if (!this._isEnabled()) return;
    
    var updated = this._checkUpgradeOccurred();
    var timeUp = this._isWaitPeriodUp();
    
    if (timeUp || updated)
    {
      this.uploadMetrics();
    } 
  },
  
  
  
  /**
   * Bundle all metrics info and send it to the server.
   *
   * TODO: Rethink version and OS strings
   */
  uploadMetrics: function()
  {
    dump("*** UPLOADING METRICS ***");
    var user_install_uuid = this._getPlayerUUID();
    
    var xulRuntime = Components.classes["@mozilla.org/xre/app-info;1"].getService(Components.interfaces.nsIXULRuntime);    
    var user_os = xulRuntime.OS;

    
    var ps = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefService);   
                                      
    var branch = ps.getBranch("metrics.");
    var metrics = branch.getChildList("", { value: 0 });

    
    var appInfo = Components.classes["@mozilla.org/xre/app-info;1"].getService(Components.interfaces.nsIXULAppInfo);
    // appInfo.name + " " + appInfo.version + " - " + appInfo.appBuildID;    

    var abi = "Unknown";
    // Not all builds have a known ABI
    try {
      abi = appInfo.XPCOMABI;
      
      // TODO: Throwing an exception every time is bad.. should probably detect os x
      
      // Mac universal build should report a different ABI than either macppc
      // or mactel.
      var macutils = Components.classes["@mozilla.org/xpcom/mac-utils;1"]
                               .getService(Components.interfaces.nsIMacUtils); 
      if (macutils.isUniversalBinary)  abi = "Universal-gcc3";
    }
    catch (e) {}

    var platform = appInfo.OS + "_" + abi;
    
    
    // build xml
    
    var xml = "";
    xml += '<metrics schema_version="1.0" guid="' + user_install_uuid 
            + '" version="' + appInfo.version 
            + '" build="' + appInfo.appBuildID
            + '" product="' + appInfo.name
            + '" platform="' + platform
            + '" os="' + user_os 
            + '">';
    for (var i = 0; i < metrics.length; i++) 
    {
      var val = branch.getCharPref(metrics[i]);
      xml += '<item key="' + metrics[i] + '" value="' + val + '"/>';
    }
    xml += '</metrics>';
    
    
    // upload xml

    var domparser = Components.classes["@mozilla.org/xmlextras/domparser;1"]
                      .getService(Components.interfaces.nsIDOMParser);   
    var document = domparser.parseFromString(xml, "text/xml");

    var onpostload = { 
      _that: null, 
      handleEvent: function( event ) { this._that.onPostLoad(); } 
    } onpostload._that = this;
    
    var onposterror = { 
      _that: null, 
      handleEvent: function( event ) { this._that.onPostError(); } 
    } onposterror._that = this;

    this._postreq = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Components.interfaces.nsIXMLHttpRequest); 
    this._postreq.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest).addEventListener("load", onpostload, false);
    this._postreq.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest).addEventListener("error", onposterror, false);
    this._postreq.open('POST', SONGBIRD_POSTMETRICS_URL, true); 
    this._postreq.send(document);
  },

  onPostLoad: function() {
    this.LOG("POST metrics done: "  + this._postreq.status + " - " + this._postreq.responseText);
    
    // POST successful, reset all metrics to 0
    if (this._postreq.status == 200 && this._postreq.responseText == "OK") 
    {
        var ps = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefService);   
        var branch = ps.getBranch("metrics.");
        var metrics = branch.getChildList("", { value: 0 });
        for (var i = 0; i < metrics.length; i++) 
        {
          branch.setCharPref(metrics[i], "0");
        }
        var pref = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch);
        var timenow = new Date();
        var now = timenow.getTime();
        
        pref.setCharPref("app.metrics.last_upload", now);
        pref.setCharPref("app.metrics.last_version", this._getCurrentVersion());
        
        this.LOG("metrics reset");
    }    
    else 
    {
        this.LOG("POST metrics failed: " + this._postreq.responseText);
    }
  },

  onPostError: function() {
    this.LOG("POST metrics error");
  },
  
  
  
  /**
   * Return true unless metrics have been 
   * explicitly disabled.
   */
  _isEnabled: function() {
  
    // Make sure we are allowed to send metrics
    var enabled = 0;
    try {
      enabled = parseInt(this.prefs.getCharPref("app.metrics.enabled"));
    }
    catch (e) { }
    if (!enabled) dump("*** METRICS ARE DISABLED ***\n");
    
    return enabled;
  },
  
  
  /**
   * Return true if SONGBIRD_UPLOAD_METRICS_EVERY_NDAYS days have passed
   * since last submission
   */
  _isWaitPeriodUp: function() { 
                  
    var timenow = new Date();
    var now = timenow.getTime();
    var last = 0;
    try 
    {
      last = parseInt(this.prefs.getCharPref("app.metrics.last_upload"));
    }
    catch (e)
    {
      // first start, pretend we just uploaded so we'll trigger the next upload in n days
      this.prefs.setCharPref("app.metrics.last_upload", now);
      last = now;
    }
    
    var diff = now - last;
    
    return (diff > (1000 /*one second*/ * 60 /*one minute*/ * 60 /*one hour*/ * 24 /*one day*/ * SONGBIRD_UPLOAD_METRICS_EVERY_NDAYS))
  },


  /**
   * Has the version changed since last metrics submission
   */
  _checkUpgradeOccurred: function() {
  
    var upgraded = false;
    
    var currentVersion = this._getCurrentVersion();
    var lastVersion = null;
    
    try 
    {
      lastVersion = this.prefs.getCharPref("app.metrics.last_version");
    }
    catch (e) { }    
    
    if (lastVersion && currentVersion != lastVersion) 
    {
        upgraded = true;
    }
    
    return upgraded;
  },
  
 
  
  /**
   * TODO: REPLACE WITH SOMETHING OFFICIAL
   */  
  _getCurrentVersion: function() {
  
    var appInfo = Components.classes["@mozilla.org/xre/app-info;1"].getService(Components.interfaces.nsIXULAppInfo);
    return appInfo.name + " " + appInfo.version + " - " + appInfo.appBuildID;    
  },
  
  
  /**
   * TODO: REPLACE WITH SOMETHING OFFICIAL
   */  
  _getPlayerUUID: function() {
  
    var uuid = "";
   
    try 
    {
      uuid = this.prefs.getCharPref("app.player_uuid");
    }
    catch (e)
    {
      uuid = "";
    }   
    
    if (uuid == "")
    {
        var aUUIDGenerator = Components.classes["@mozilla.org/uuid-generator;1"].createInstance(Components.interfaces.nsIUUIDGenerator);
        uuid = aUUIDGenerator.generateUUID();
        this.prefs.setCharPref("app.player_uuid", uuid);
    }
    
    return uuid;     
  },  
  
  
  
  
  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(SONGBIRD_METRICS_IID) &&
        !iid.equals(Components.interfaces.nsIWebProgressListener) &&
        !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // Metrics.prototype

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
    // The Metrics Component
    metrics:     { CID       : SONGBIRD_METRICS_CID,
                  contractID : SONGBIRD_METRICS_CONTRACTID,
                  className  : SONGBIRD_METRICS_CLASSNAME,
                  factory    : #1#(Metrics)
                },
  },

  canUnload: function(componentManager) { 
    return true; 
  }
}; // gModule

function NSGetModule(comMgr, fileSpec) {
  return gModule;
} // NSGetModule
