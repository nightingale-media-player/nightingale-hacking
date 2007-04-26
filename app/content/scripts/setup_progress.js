/*
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

var label;
var progressmeter;
var bundle,pbundle;
var n_ext;
var cur_ext;
var destFile;
var afailure = false;

const BUNDLE_ERROR_XPIINSTALLFAILED = -1;
const BUNDLE_ERROR_DOWNLOADFAILED   = -2;

const VK_ENTER = 13;
const VK_ESCAPE = 27;


function init()
{
  label = document.getElementById("setupprogress_label");
  progressmeter = document.getElementById("setupprogress_progress");
  bundle = window.arguments[0];
  pbundle = bundle.QueryInterface(Components.interfaces.sbPIBundle);
  pbundle.setNeedRestart(false);
  bundle = bundle.QueryInterface(Components.interfaces.sbIBundle);
  n_ext = bundle.bundleExtensionCount;
  cur_ext = -1;
  setTimeout(installNextXPI, 0);
}

function installNextXPI()
{
  cur_ext++;
  if (cur_ext+1 > n_ext) { 
    pbundle.setInstallResult(afailure ? 0 : 1); 
    for (var i=0;i<pbundle.installListenerCount;i++) 
      pbundle.getInstallListener(i).onComplete(bundle);
    window.close(); 
    return; 
  }
  
  
  if (!bundle.getExtensionInstallFlag(cur_ext)) {
    setTimeout(installNextXPI, 0);
    return;
  }

  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"].getService(Components.interfaces.nsIStringBundleService);
  var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
  var downloading = "Downloading";
  try {
    downloading = songbirdStrings.GetStringFromName("setupprogress.downloading");
  } catch (e) {}
  label.setAttribute("value", downloading + " " + bundle.getExtensionAttribute(cur_ext, "name"));
  progressmeter.setAttribute("value", 0);
  
  for (var i=0;i<pbundle.installListenerCount;i++) 
    pbundle.getInstallListener(i).onExtensionDownloadProgress(bundle, cur_ext, 0, 1);
  
  destFile = downloadFile(bundle.getExtensionAttribute(cur_ext, "url"));
}

function onExtensionDownloadProgress(aCurrentProgress, aMaxProgress) {
  progressmeter.setAttribute("value", aCurrentProgress/aMaxProgress*100);
  for (var i=0; i < pbundle.installListenerCount; i++) 
    pbundle.getInstallListener(i).onExtensionDownloadProgress(pbundle, cur_ext, aCurrentProgress, aMaxProgress);
}
  
function onExtensionDownloadComplete() {
  for (var i=0;i<pbundle.installListenerCount;i++) 
    pbundle.getInstallListener(i).onDownloadComplete(pbundle, cur_ext);
  var r = installXPI(destFile);
  if (r == 0) {
    gPrompt.alert( window, "Error",
                  SBString( "setupprogress.couldnotinstall", "Could not install" ) + " " +
                  bundle.getExtensionAttribute(cur_ext, "name") );
    for (var i=0;i<pbundle.installListenerCount;i++) 
      pbundle.getInstallListener(i).onInstallError(pbundle, cur_ext);
  } else pbundle.setNeedRestart(true);
  deleteLastDownloadedFile();
  for (var i=0;i<pbundle.installListenerCount;i++) 
    pbundle.getInstallListener(i).onInstallComplete(pbundle, cur_ext);
  setTimeout(installNextXPI, 0);
}

function onExtensionDownloadError() {
  gPrompt.alert( window, "Error",
                SBString( "setupprogress.couldnotdownload", "Downloading" ) + " " +
                bundle.getExtensionAttribute(cur_ext, "name") );
  sbMessageBox("Error", couldnotdownload + " " + bundle.getExtensionAttribute(cur_ext, "name"), false);
  afailure = true;
  deleteLastDownloadedFile();
  for (var i=0;i<pbundle.installListenerCount;i++) 
    pbundle.getInstallListener(i).onDownloadError(pbundle, cur_ext);
  setTimeout(installNextXPI, 0);
}

function handleKeyDown(event) 
{
  if (event.keyCode == VK_ESCAPE ||
      event.keyCode == VK_ENTER) {
    event.preventDefault();
  }
}

// -------------------

var _downloadListener = {
  onLocationChange: function(aWebProgress, aRequest, aLocation)
  {
  },

  onProgressChange: function(aWebProgress, aRequest, curSelfProgress, maxSelfProgress, curTotalProgress, maxTotalProgress)
  {
    onExtensionDownloadProgress(curSelfProgress, maxSelfProgress);
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
        file.initWithPath(_filename);
        if (file.exists())
          onExtensionDownloadComplete();
        else
          onExtensionDownloadError();
      } catch (e) {
        dump(e + "\n");
        onExtensionDownloadError();
      }
    }
  },
  
  onStatusChange: function(aWebProgress, aRequest, aStateFlags, strStateMessage)
  {
  },

  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.nsIWebProgressListener) &&
        !iid.equals(Components.interfaces.nsISupportsWeakReference) &&
        !iid.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}

var _url;
var _filename;
var _browser;
var _file;

function downloadFile(url) {
  _url = url;

  var destFile = getTempFilename() + ".xpi";
  _filename = destFile;
  _browser = (Components.classes["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]).createInstance(Components.interfaces.nsIWebBrowserPersist);

  if (!_browser) return null;
  _browser.progressListener = _downloadListener;
  
  var aLocalFile = (Components.classes["@mozilla.org/file/local;1"]).createInstance(Components.interfaces.nsILocalFile);
  aLocalFile.initWithPath(destFile);
  _file = aLocalFile;
  
  var aLocalURI = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService).newURI(url + getRandomParameter(), null, null);

  const nsIWBP = Components.interfaces.nsIWebBrowserPersist;
  var flags = nsIWBP.PERSIST_FLAGS_NO_CONVERSION |
              nsIWBP.PERSIST_FLAGS_REPLACE_EXISTING_FILES |
              nsIWBP.PERSIST_FLAGS_BYPASS_CACHE;
  _browser.persistFlags = flags;

  _browser.saveURI(aLocalURI, null, null, null, "", aLocalFile);

  return destFile;
}

function deleteLastDownloadedFile() {
  if (_file) {
    try {
      _file.remove(true);
    } catch (e) {}
    _file = null;
    _filename = null;
  }
}

function getTempFilename() {
  var strTempFile = "";
  
  var aDirectoryService = Components.classes["@mozilla.org/file/directory_service;1"].createInstance();
  aDirectoryService = aDirectoryService.QueryInterface(Components.interfaces.nsIProperties);
  
  var aUUID = generateUUID();
  
  var bResult = new Object;
  var aTempFolder = aDirectoryService.get("DefProfLRt", Components.interfaces.nsIFile, bResult);
  
  aTempFolder.append(aUUID);
  
  return aTempFolder.path;
}

function generateUUID() {
  var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
  aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);
  return aUUIDGenerator.generateUUID();
}

function getRandomParameter() {
  return "?randomguid=" + escape(generateUUID());
}

function installXPI(localFilename)
{
  var file = Components.classes["@mozilla.org/file/local;1"]
                    .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath(localFilename);
  
  var em = Components.classes["@mozilla.org/extensions/manager;1"]
                      .getService(Components.interfaces.nsIExtensionManager);
  var r = 0;
  try {
    em.installItemFromFile(file, "app-profile");
    r = 1;
  } catch (e) {}
  return r;
}