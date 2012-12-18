/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const SONGBIRD_METRICS_CONTRACTID = "@songbirdnest.com/Songbird/Metrics;1";
const SONGBIRD_METRICS_CLASSNAME = "Songbird Metrics Service Interface";
const SONGBIRD_METRICS_CID = Components.ID("{1066527d-b135-4e0c-9ea4-f6109ae97d02}");
const SONGBIRD_METRICS_IID = Components.interfaces.sbIMetrics;

const SONGBIRD_METRICS_GA_PROPERTYKEY = "app.metrics.ga.property";
const SONGBIRD_METRICS_GA_PROPERTYDEF = "UA-114360-23";

const SONGBIRD_UPLOAD_METRICS_EVERY_NDAYS = 1; // every day

function Metrics() {
    this.prefs = Components.classes["@mozilla.org/preferences-service;1"]
                      .getService(Components.interfaces.nsIPrefBranch);
}

Metrics.prototype = {
  classDescription: SONGBIRD_METRICS_CLASSNAME,
  classID:          SONGBIRD_METRICS_CID,
  contractID:       SONGBIRD_METRICS_CONTRACTID,
  QueryInterface:   XPCOMUtils.generateQI([
    SONGBIRD_METRICS_IID,
    Components.interfaces.nsIWebProgressListener,
    Components.interfaces.nsISupportsWeakReference 
  ]),

  _getreq: null,
  _dbquery: null,

  _eventMatchArray: [
    [new RegExp(/^firstrun\.mediaimport\.(.+)$/g),
      "install","first run media import"],
    [new RegExp(/^app\.appstart$/g),
      "usage","start"],
    [new RegExp(/^mediacore\.play\.attempt\.(.+)$/g),
      "usage","media playback"],
  ],
  
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
    
    var timeUp = this._isWaitPeriodUp();
    
    if (timeUp)
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
    var user_install_uuid = this._getPlayerUUID();
    
    var xulRuntime = Components.classes["@mozilla.org/xre/runtime;1"].getService(Components.interfaces.nsIXULRuntime);    
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
    
    for (var i = 0; i < metrics.length; i++) 
    {
      var key = metrics[i][0];
      var val = metrics[i][1];
      if ( val > 0 )
      {
        var dot = key.indexOf(".");
        if (dot >= 0) {
          var timestamp = key.substr(0, dot);
          var cleanKey = key.substr(dot + 1);
          
          // TODO ???
        }
      }
    }

    var ongetload = { 
      _that: null, 
      handleEvent: function( event ) { this._that.onGetLoad(); } 
    };
    ongetload._that = this;
    
    var ongeterror = { 
      _that: null, 
      handleEvent: function( event ) { this._that.onGetError(); } 
    };
    ongeterror._that = this;

    var getURL = "http://www.google-analytics.com/__ga.gif?";
    // todo
    
//    this._getreq = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Components.interfaces.nsIXMLHttpRequest); 
//    this._getreq.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest).addEventListener("load", ongetload, false);
//    this._getreq.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest).addEventListener("error", ongeterror, false);
//    this._getreq.open('GET', getURL, true); 
//    this._getreq.send(null);
  },
  
  formatDigits: function(str, n) {
    str = str+'';
    while (str.length < n) str = "0" + str;
    return str;
  },

  onGetLoad: function() {
    this.LOG("GET metrics done: "  + this._getreq.status + " - " + this._getreq.responseText);
    
    // GET successful, reset all metrics to 0
    if (this._getreq.status == 200 && this._getreq.responseText == "OK") 
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
      this.LOG("GET metrics failed: " + this._getreq.responseText);
    }
  },

  onGetError: function() {
    this.LOG("GET metrics error");
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
        uuid = this.prefs.getCharPref("app.player_uuid"); // force string type
    }
    
    return uuid;
  },
  
  metricsInc: function( aCategory, aUniqueID, aExtraString ) {
    this.metricsAdd( aCategory, aUniqueID, aExtraString, 1 );
  },
  
  metricsAdd: function( aCategory, aUniqueID, aExtraString, aIntValue ) {
    var gPrompt = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
    .getService(Components.interfaces.nsIPromptService);
      
    // Cook up the key string
    var key = aCategory + "." + aUniqueID;    
    if (aExtraString != null && aExtraString != "") key = key + "." + aExtraString;

    var eventCode = null;
    for(var iPattern = 0; iPattern < this._eventMatchArray.length; iPattern++ ) {
      var match = this._eventMatchArray[iPattern][0].exec(key);      
      if(match && (match.length > 0) && (match[0] == key)) {
        eventCode = "5({0}*{1}*{2})(1)";
        eventCode = eventCode.replace("{0}", this._eventMatchArray[iPattern][1]);
        eventCode = eventCode.replace("{1}", this._eventMatchArray[iPattern][2]);
        eventCode = eventCode.replace("{2}", (match.length > 1) ? match[1] : "");        
        break;
      }
    }
    if(!eventCode) {
      return;
    }

//  gPrompt.alert(null,key,eventCode);

    // timestamps are recorded as UTC !
    var d = new Date();
    /// getTime() is in ms, round to nearest second
    var timestamp = (Math.floor(d.getTime() / 3600000) * 3600) + (d.getTimezoneOffset() * 60);
    /// GA hashing algorithm is intended for URLs and doesn't like certain characters
    var gaCompliantUUID = this._getPlayerUUID().replace(/[{}-]/g,"");
    // GA cookie
    var cookie = [
      this._gaHash("songbirdnest.com"),
      this._gaHash(gaCompliantUUID),
      timestamp,timestamp,timestamp,
      1
      ].join(".");
    var rand = Math.floor(Math.random()*1000000000);
//    var gaPropKey = this.prefs.getCharPref(SONGBIRD_METRICS_GA_PROPERTYKEY);
  	var gaPropKey = SONGBIRD_METRICS_GA_PROPERTYDEF;
    var getParams = [
      "utmwv=5.3.8",
      "utmac=" + gaPropKey,
      "utmcc=__utma%3D" + cookie,
      "utmp=index.html",
      "utmt=event",
      "utme=" + eventCode,
      "utmn=" + rand
      ].join("&"); 

//    gPrompt.alert(null,key,getParams);

    var getURL = "http://www.google-analytics.com/__utm.gif?"
      + getParams;

    this._getreq = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"]
      .createInstance(Components.interfaces.nsIXMLHttpRequest); 
    this._getreq.open('GET', getURL, false); 
    this._getreq.send(null);  
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
  
  _gaHash : function(aString) {
    var result = 1;
    var partial = 0;
    var charIndex = 0;
    var charCode = 0;
  if( aString ) {
      result = 0;
    for( charIndex = aString["length"]-1; charIndex>=0; charIndex--) {
      charCode = aString.charCodeAt(charIndex);
      result = (result<<6&268435455)+charCode+(charCode<<14);
      partial = result&266338304;
      result = (partial!=0) ? result^partial>>21 : result;
    }
  }
  return result;
  },    
} // Metrics.prototype

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([Metrics]);
}

