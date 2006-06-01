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

const SONGBIRD_METRICS_CONTRACTID = "@songbird.org/Songbird/Metrics;1";
const SONGBIRD_METRICS_CLASSNAME = "Songbird Metrics Service Interface";
const SONGBIRD_METRICS_CID = Components.ID("{F64283C0-CDCF-48ec-8502-735B7282981E}");
const SONGBIRD_METRICS_IID = Components.interfaces.sbIMetrics;

const SONGBIRD_POSTMETRICS_URL = 'http://www.songbirdnest.com/metrics/post.php';
const SONGBIRD_UPLOAD_METRICS_EVERY_NDAYS = 7; // every week

function Metrics() {
}

Metrics.prototype = {
  _postreq: null,
  
  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },

  checkUploadMetrics: function()
  {
    var ps = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefBranch);
    var disabled = 0;
    try 
    {
      disabled = parseInt(ps.getCharPref("metrics_disabled"));
    }
    catch (e) { }
    if (disabled) return;
    var timenow = new Date();
    var now = timenow.getTime();
    var last = 0;
    try 
    {
      last = parseInt(ps.getCharPref("last_metrics_upload"));
    }
    catch (e)
    {
      // first start, pretend we just uploaded so we'll trigger the next upload in n days
      ps.setCharPref("last_metrics_upload", now);
      last = now;
    }
    var diff = now - last;
    if (diff > (1000 /*one second*/ * 60 /*one minute*/ * 60 /*one hour*/ * 24 /*one day*/ * SONGBIRD_UPLOAD_METRICS_EVERY_NDAYS))
    {
      this.uploadMetrics();
    } 
  },
  
  uploadMetrics: function()
  {
    var appInfo = Components.classes["@mozilla.org/xre/app-info;1"].getService(Components.interfaces.nsIXULAppInfo);
    var xulRuntime = Components.classes["@mozilla.org/xre/app-info;1"].getService(Components.interfaces.nsIXULRuntime);
    var user_install_uuid = appInfo.ID;
    var user_agent_version = appInfo.name + " " + appInfo.version + " - " + appInfo.appBuildID;
    var user_os = xulRuntime.OS;
    
    var ps = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefService);   
    var branch = ps.getBranch("metrics.");
    var metrics = branch.getChildList("", { value: 0 });
    var xml = "";
    xml += '<metrics schema_version="1.0" guid="' + user_install_uuid + '" user_agent="' + user_agent_version + '" user_os="' + user_os + '">';
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
    // POST successful, reset all metrics to 0
    this.LOG("POST metrics done");
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
    pref.setCharPref("last_metrics_upload", now);
  },

  onPostError: function() {
    this.LOG("POST metrics error");
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
