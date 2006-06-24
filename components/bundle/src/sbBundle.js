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
const SONGBIRD_BUNDLE_CONTRACTID = "@songbirdnest.com/Songbird/Bundle;1";
const SONGBIRD_BUNDLE_CLASSNAME = "Songbird Bundle Service Interface";
const SONGBIRD_BUNDLE_CID = Components.ID("{ff29ec35-1294-42ae-a341-63d0303df969}");
const SONGBIRD_BUNDLE_IID = Components.interfaces.sbIBundle;

const SONGBIRD_GETBUNDLE_URL_WIN32 = 'http://www.songbirdnest.com/bundle/firstrun_win32.xml';
const SONGBIRD_GETBUNDLE_URL_MACOSX = 'http://www.songbirdnest.com/bundle/firstrun_macosx.xml';
const SONGBIRD_GETBUNDLE_URL_LINUX = 'http://www.songbirdnest.com/bundle/firstrun_linux.xml';

function Bundle() {
  this._observers = new Array();
}

Bundle.prototype.constructor = Bundle;

Bundle.prototype = {
  _req: null,
  _observers: null,
  _status: 0,
  _extlist: null,
  _browser: null,
  _downloadObserver: null,
  _url: null,
  _file: null,
  _filename: null,
  _needrestart: false,
  _bundleversion: 0,
  
  LOG: function(str) {
    var consoleService = Components.classes['@mozilla.org/consoleservice;1']
                            .getService(Components.interfaces.nsIConsoleService);
    consoleService.logStringMessage(str);
  },

  retrieveBundleFile: function() {
  
    var onload = { 
      _that: null, 
      handleEvent: function( event ) { this._that.onLoad(); } 
    } onload._that = this;
    
    var onerror = { 
      _that: null, 
      handleEvent: function( event ) { this._that.onError(); } 
    } onerror._that = this;
    
    this._req = Components.classes["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Components.interfaces.nsIXMLHttpRequest); 
    this._req.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest).addEventListener("load", onload, false);
    this._req.QueryInterface(Components.interfaces.nsIJSXMLHttpRequest).addEventListener("error", onerror, false);
    var xulRuntime = Components.classes["@mozilla.org/xre/app-info;1"].getService(Components.interfaces.nsIXULRuntime);
    var url;
    switch (xulRuntime.OS) {
      case 'WINNT': 
      case 'WIN95':
        url = SONGBIRD_GETBUNDLE_URL_WIN32; 
        break;
      case 'Linux': url = SONGBIRD_GETBUNDLE_URL_LINUX; break;
      case 'Darwin': url = SONGBIRD_GETBUNDLE_URL_MACOSX; break;
    }
    //this.LOG(url);
    this._req.open('GET', url, true); 
    this._req.send(null);
  },
        
  getBundleDocument: function() {
    return this._req ? this._req.responseXML : null;
  },

  getTextData: function() {
    return this._req ? this._req.responseText : "";
  },
  
  addBundleObserver: function(obs) {
    this._observers.push(obs);
  },
  
  removeBundleObserver: function (obs) {
    var r = this.getObserverIndex(obs);
    if (r != -1) this._observers.splice(r, 1);
  },
  
  getObserverIndex: function(obs) {
    for (var i=0;i<this._observers.length;i++) if (this._observers[i] == obs) return i;
    return -1;
  },
  
  getStatus: function() {
    return this._status;
  },
  
  onLoad: function() {
    this._status = 1;
    this.getExtensionList();
    for (var i=0;i<this._observers.length;i++) this._observers[i].onLoad(this);
  },

  onError: function() {
    this._status = -1;
    for (var i=0;i<this._observers.length;i++) this._observers[i].onError(this);
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

  installSelectedExtensions: function(window) {
    var windowWatcherService = Components.classes['@mozilla.org/embedcomp/window-watcher;1']
                            .getService(Components.interfaces.nsIWindowWatcher);
                            
    // TODO: do the install !
    windowWatcherService.openWindow(window, "chrome://songbird/content/xul/setup_progress.xul", "_blank", "chrome,dialog=yes,centerscreen,alwaysRaised,close=no,modal", this);
  },
  
  getExtensionList: function() {
    this._extlist = new Array();
    if (this._status == 1) {
      bundledocument = this.getBundleDocument();
      if (bundledocument) {
        var nodes = this.getDataNodes(bundledocument);
        if (nodes) {
          for (var i=0;i<nodes.length;i++) {
            if (nodes[i].tagName == "XPI") {
              var inst = nodes[i].getAttribute("default");
              this._extlist.push(Array(nodes[i].getAttribute("name"), nodes[i].getAttribute("desc"), nodes[i].getAttribute("url"), (inst=="true" || inst=="1"), nodes[i].getAttribute("id")));
            }
          }
        }
      }
    }
  },
  
  getNumExtensions: function() {
    if (this._status == 1) return this._extlist.length;
    return 0;
  },
  
  getExtensionName: function(idx) {
    if (this._status == 1 && idx < this.getNumExtensions()) return this._extlist[idx][0];
    return "";
  },
      
  getExtensionDesc: function(idx) {
    if (this._status == 1 && idx < this.getNumExtensions()) return this._extlist[idx][1];
    return "";
  },
      
  getExtensionURL: function(idx) {
    if (this._status == 1 && idx < this.getNumExtensions()) return this._extlist[idx][2];
    return "";
  },
      
  getExtensionInstallState: function(idx) {
    if (this._status == 1 && idx < this.getNumExtensions()) return this._extlist[idx][3];
    return false;
  },
  
  setExtensionInstallState: function(idx, doinstall) {
    if (this._status == 1 && idx < this.getNumExtensions()) this._extlist[idx][3] = doinstall;
  },
  
  downloadFile: function(url, observer) {
    this._downloadObserver = observer;
    this._url = url;

    //this.LOG("downloading file " + url);

    var destFile = this.getTempFilename() + ".xpi";
    this._filename = destFile;
    this._browser = (Components.classes["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]).createInstance(Components.interfaces.nsIWebBrowserPersist);

    if (!this._browser) return null;
    this._browser.progressListener = this;
    
    var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
    aLocalFile.initWithPath(destFile);
    this._file = aLocalFile;
    
    var aLocalURI = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService).newURI(url, null, null);

    const nsIWBP = Components.interfaces.nsIWebBrowserPersist;
    var flags = nsIWBP.PERSIST_FLAGS_NO_CONVERSION |
                nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
                nsIWBP.PERSIST_FLAGS_BYPASS_CACHE;
    this._browser.persistFlags = flags;

    this._browser.saveURI(aLocalURI, null, null, null, "", aLocalFile);

    return destFile;
  },
  
  deleteLastDownloadedFile: function() {
    if (this._file) {
      try {
        this._file.remove(true);
      } catch (e) {}
      this._file = null;
      this._filename = null;
    }
  },

  getTempFilename: function () {
    var strTempFile = "";
    
    var aDirectoryService = Components.classes["@mozilla.org/file/directory_service;1"].createInstance();
    aDirectoryService = aDirectoryService.QueryInterface(Components.interfaces.nsIProperties);
    
    var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
    aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);
    var aUUID = aUUIDGenerator.generateUUID();
    
    var bResult = new Object;
    var aTempFolder = aDirectoryService.get("DefProfLRt", Components.interfaces.nsIFile, bResult);
    
    aTempFolder.append(aUUID);
    
    return aTempFolder.path;
  },
  
  installXPI: function(localFilename)
  {
    var file = Components.classes["@mozilla.org/file/local;1"]
                     .createInstance(Components.interfaces.nsILocalFile);
    //this.LOG("init file " + localFilename);
    file.initWithPath(localFilename);
    //this.LOG("exists = " + file.exists());
    
    var em = Components.classes["@mozilla.org/extensions/manager;1"]
                       .getService(Components.interfaces.nsIExtensionManager);
    var r = 0;
    try {
      em.installItemFromFile(file, "app-profile");
      r = 1;
    } catch (e) {}
    return r;
  },
  
  setNeedRestart: function(need) {
    this._needrestart = need;
  },

  getNeedRestart: function(need) {
    return this._needrestart;
  },

  onLocationChange: function(aWebProgress, aRequest, aLocation)
  {
  },

  onProgressChange: function(aWebProgress, aRequest, curSelfProgress, maxSelfProgress, curTotalProgress, maxTotalProgress)
  {
    this._downloadObserver.onProgress(this, curSelfProgress/maxSelfProgress*100);
  },

  onSecurityChange: function(aWebProgress, aRequest, aStateFlags)
  {
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    if (aStateFlags & 16 /*this.STATE_STOP*/)
    {
      try {
        var file = Components.classes["@mozilla.org/file/local;1"]
                        .createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath(this._filename);
        if (file.exists())
          this._downloadObserver.onDownloadComplete(this);
        else
          this._downloadObserver.onError(this);
      } catch (e) {
        this._downloadObserver.onError(this);
      }
    }
  },
  
  onStatusChange: function(aWebProgress, aRequest, aStateFlags, strStateMessage)
  {
  },
  
  getBundleVersion: function() {
    return this._bundleversion;
  },
  
  /**
   * See nsISupports.idl
   */
  QueryInterface: function(iid) {
    if (!iid.equals(SONGBIRD_BUNDLE_IID) &&
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