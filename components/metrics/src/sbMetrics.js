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

const SONGBIRD_METRICS_CONTRACTID = "@songbirdnest.com/Songbird/Metrics;1";
const SONGBIRD_METRICS_CLASSNAME = "Songbird Metrics Service Interface";
const SONGBIRD_METRICS_CID = Components.ID("{1066527d-b135-4e0c-9ea4-f6109ae97d02}");
const SONGBIRD_METRICS_IID = Components.interfaces.sbIMetrics;

const SONGBIRD_POSTMETRICS_PREFKEY = "songbird.url.metrics";

const SONGBIRD_UPLOAD_METRICS_EVERY_NDAYS = 1; // every day

function Metrics() {
    this.prefs = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefBranch);
}

Metrics.prototype = {
  _postreq: null,
  _dbquery: null,
  
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
    
    var updated = this._hasVersionChanged();
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

    
    var metrics = this._getTable();

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
    
    
    var updated = this._hasUpdateOccurred();
    
    var tzo = (new Date()).getTimezoneOffset();
    var neg = (tzo < 0);
    tzo = Math.abs(tzo);
    var tzh = Math.floor(tzo / 60);
    var tzm = tzo - (tzh*60);
    // note: timezone is -XX:XX if the offset is positive, and +XX:00 if the offset is negative !
    // this is because the offset has the reverse sign compared to the timezone, since
    // the offset is what you should add to localtime to get UTC, so if you add -XX:XX, you're
    // subtracting XX:XX, because the timezone is UTC+XX:XX
    var tz = (neg ? "+" : "-") + this.formatDigits(tzh,2) + ":" + this.formatDigits(tzm,2);
    
    // build xml
    
    var xml = "";
    xml += '<metrics schema_version="2.0" guid="' + user_install_uuid 
            + '" version="' + appInfo.version 
            + '" build="' + appInfo.appBuildID
            + '" product="' + appInfo.name
            + '" platform="' + platform
            + '" os="' + user_os 
            + '" updated="' + updated 
            + '" timezone="' + tz 
            + '">\n';

    for (var i = 0; i < metrics.length; i++) 
    {
      var key = metrics[i][0];
      var val = metrics[i][1];
      if ( val > 0 )
      {
        var dot = key.indexOf(".");
        if (dot >= 0) {
          var timestamp = key.substr(0, dot);
          var date = new Date();
          date.setTime(timestamp);
          var hourstart = date.getFullYear() + "-" + 
                          this.formatDigits(date.getMonth()+1,2) + "-" + 
                          this.formatDigits(date.getDate(),2) + " " + 
                          this.formatDigits(date.getHours(),2) + ":" + 
                          this.formatDigits(date.getMinutes(),2) + ":" + 
                          this.formatDigits(date.getSeconds(),2);
          xml += '\t<item hour_start="' + hourstart + '" key="' + encodeURIComponent(key) + '" value="' + val + '"/>\n';
        }
      }
    }
    xml += '</metrics>';

/*
    // Happy little self-contained test display
    var gPrompt = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                          .getService(Components.interfaces.nsIPromptService);
    gPrompt.alert( null, "METRICS XML", xml );
*/

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

    var postURL = this.prefs.getCharPref(SONGBIRD_POSTMETRICS_PREFKEY);
    
    this._postreq = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Components.interfaces.nsIXMLHttpRequest); 
    this._postreq.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest).addEventListener("load", onpostload, false);
    this._postreq.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest).addEventListener("error", onposterror, false);
    this._postreq.open('POST', postURL, true); 
    this._postreq.send(document);
  },
  
  formatDigits: function(str, n) {
    str = str+'';
    while (str.length < n) str = "0" + str;
    return str;
  },

  onPostLoad: function() {
    this.LOG("POST metrics done: "  + this._postreq.status + " - " + this._postreq.responseText);
    
    // POST successful, reset all metrics to 0
    if (this._postreq.status == 200 && this._postreq.responseText == "OK") 
    {
      this._emptyTable();
      var pref = Components.classes["@mozilla.org/preferences-service;1"]
                        .getService(Components.interfaces.nsIPrefBranch);
      var timenow = new Date();
      var now = timenow.getTime();
      
      pref.setCharPref("app.metrics.last_upload", now);
      pref.setCharPref("app.metrics.last_version", this._getCurrentVersion());
      pref.setIntPref("app.metrics.last_update_count", this._getUpdateCount());
      
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
    //if (!enabled) dump("*** METRICS ARE DISABLED ***\n");
    
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
  _hasVersionChanged: function() {
  
    var upgraded = false;
    
    var currentVersion = this._getCurrentVersion();
    var lastVersion = null;
    
    try 
    {
      lastVersion = this.prefs.getCharPref("app.metrics.last_version");
    }
    catch (e) { }    
    
    if (currentVersion != lastVersion) 
    {
        upgraded = true;
    }
    
    return upgraded;
  },
  
  
  /**
   * Has the update manager updated songbird since the last time metrics were submitted
   */
  _hasUpdateOccurred: function() {
  
    var updated = false;
    
    var currentCount = this._getUpdateCount();
    var lastCount = null;
    
    try 
    {
      lastCount = this.prefs.getIntPref("app.metrics.last_update_count");
    }
    catch (e) 
    { 
      // If the pref didn't exist, then we must not have updated
      return false; 
    }    
    
    if (currentCount != lastCount) 
    {
        updated = true;
    }
    
    return updated;
  },
  
 
  
  /**
   * TODO: REPLACE WITH SOMETHING OFFICIAL
   */  
  _getCurrentVersion: function() {
  
    var appInfo = Components.classes["@mozilla.org/xre/app-info;1"].getService(Components.interfaces.nsIXULAppInfo);
    return appInfo.name + " " + appInfo.version + " - " + appInfo.appBuildID;    
  },
  

  /**
   * Find out how many updates have been applied through the update manager
   */  
  _getUpdateCount: function() {
    var updateManager = Components.classes["@mozilla.org/updates/update-manager;1"].getService(Components.interfaces.nsIUpdateManager);
    return updateManager.updateCount;    
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
  
  metricsInc: function( aCategory, aUniqueID, aExtraString ) {
    this.metricsAdd( aCategory, aUniqueID, aExtraString, 1 );
  },
  
  metricsAdd: function( aCategory, aUniqueID, aExtraString, aIntValue ) {
    // timestamps are recorded as UTC !
    var d = new Date();
    var timestamp = (Math.floor(d.getTime() / 3600000) * 3600000) + (d.getTimezoneOffset() * 60000);

    // Cook up the key string
    var key = timestamp + "." + aCategory + "." + aUniqueID;
    if (aExtraString != null && aExtraString != "") key = key + "." + aExtraString;
    
    try {
      // Don't record things if we're disabled.
      if (!this._isEnabled()) return;

      // Make sure it's an actual int.      
      intvalue = parseInt(aIntValue);
    
      // Then add our value to the old value and write it back
      var cur = this._getValue( key );
      var newval = cur + intvalue;
      this._setValue( key, newval );
    } 
    catch(e) { 
      this.LOG("error: metricsAdd( " + 
               aCategory +
               ", " +
               aUniqueID + 
               ", " + 
               aExtraString + 
               ", " + 
               aIntValue + 
               " ) == '" + 
               key + 
               "'\n\n" + 
               e);
    }
  },
  
  _initDB: function() {
    if (!this._dbquery) {
      this._dbquery = Components.classes["@songbirdnest.com/Songbird/DatabaseQuery;1"].
                                 createInstance(Components.interfaces.sbIDatabaseQuery);
      this._dbquery.setAsyncQuery(false);
      this._dbquery.setDatabaseGUID("metrics");
      this._dbquery.resetQuery();
      this._dbquery.addQuery("CREATE TABLE IF NOT EXISTS metrics (keyname TEXT UNIQUE NOT NULL, keyvalue BIGINT DEFAULT 0)");
      this._dbquery.execute();
    }
  },
  
  _getValue: function(key) {
    var retval = 0;

    this._initDB();
    this._dbquery.resetQuery();
  
    this._dbquery.addQuery("SELECT * FROM metrics WHERE keyname = \"" + key + "\"");
    this._dbquery.execute();

    var dbresult = this._dbquery.getResultObject();
    if (dbresult.getRowCount() > 0) {
      retval = parseInt(dbresult.getRowCell(0, 1));
    }

    return retval;
  },
   
  _setValue: function(key, n) {
    this._initDB();
    this._dbquery.resetQuery();
    this._dbquery.addQuery("INSERT OR REPLACE INTO metrics VALUES (\"" + key + "\", " + n + ")");
    this._dbquery.execute();
  },
  
  _getTable: function() {
    var table = new Array();
    this._initDB();
    this._dbquery.resetQuery();
    this._dbquery.addQuery("SELECT * FROM metrics");
    this._dbquery.execute();

    var dbresult = this._dbquery.getResultObject();
    var count = dbresult.getRowCount();

    for (var i=0;i<count;i++) {
      var key = dbresult.getRowCell(i, 0);
      var val = parseInt(dbresult.getRowCell(i, 1));
      table.push([key, val]);
    }
    
    return table;
  },
  
  _emptyTable: function() {
    this._initDB();
    this._dbquery.resetQuery();
    this._dbquery.addQuery("DELETE FROM metrics");
    this._dbquery.execute();
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
