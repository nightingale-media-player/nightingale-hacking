/*
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

/**
 * \file setupProgress.js
 * \brief This file contains support functions and objects for setupProgress.xul 
 *        which downloads XPIs during first run.
 *
 * setupProgress.xul and setupProgress.js are responsible for downloading XPIs during first run.
 * setupProgress.js contains all the support code required to download XPIs and install them.
 *
 * \internal
 */

var label;
var progressmeter;
var cancelButton;
var bundle,pbundle;
var n_ext;
var cur_ext;
var destFile;
var afailure = false;
var userCancel = false;

const BUNDLE_ERROR_XPIINSTALLFAILED = -1;
const BUNDLE_ERROR_DOWNLOADFAILED   = -2;

const VK_ENTER = 13;
const VK_ESCAPE = 27;

const NS_BINDING_ABORTED = 0x804B0002;

/**
 * \brief Initialize the setupProgress dialog.
 * \internal
 */
function init()
{
  label = document.getElementById("setupprogress_label");
  progressmeter = document.getElementById("setupprogress_progress");
  cancelButton = document.getElementById("setupprogress_cancel");
  bundle = window.arguments[0];
  pbundle = bundle.QueryInterface(Components.interfaces.sbPIBundle);
  pbundle.setNeedRestart(false);
  bundle = bundle.QueryInterface(Components.interfaces.sbIBundle);
  n_ext = bundle.bundleExtensionCount;
  cur_ext = -1;
  setTimeout(installNextXPI, 0);
}

/**
 * \brief Install the next XPI in the list of XPIs to install.
 * \internal
 */
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
  cancelButton.removeAttribute("disabled");
  userCancel = false;
  progressmeter.setAttribute("value", 0);
  
  for (var i=0;i<pbundle.installListenerCount;i++) 
    pbundle.getInstallListener(i).onExtensionDownloadProgress(bundle, cur_ext, 0, 1);
  
  destFile = downloadFile(bundle.getExtensionAttribute(cur_ext, "url"));
}

/**
 * \brief Listener function used to track progress of XPIs being downloaded.
 * \internal
 */
function onExtensionDownloadProgress(aCurrentProgress, aMaxProgress) {
  progressmeter.setAttribute("value", aCurrentProgress/aMaxProgress*100);
  for (var i=0; i < pbundle.installListenerCount; i++) 
    pbundle.getInstallListener(i).onExtensionDownloadProgress(pbundle, cur_ext, aCurrentProgress, aMaxProgress);
}

/**
 * \brief Listener function used to trigger installation of XPI when download is complete.
 * \internal
 */  
function onExtensionDownloadComplete() {
  for (var i=0;i<pbundle.installListenerCount;i++) 
    pbundle.getInstallListener(i).onDownloadComplete(pbundle, cur_ext);
  var r = forceInstallXPI(destFile);
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

/**
 * \brief Listener function used to recover from any errors that occur during the download of an XPI.
 * \internal
 */
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

/**
 * \brief Handler function used to prevent bubbling of escape 
 *        and enter key events while the progressSetup dialog is active.
 * \internal
 */
function handleKeyDown(event) 
{
  if (event.keyCode == VK_ESCAPE ||
      event.keyCode == VK_ENTER) {
    event.preventDefault();
  }
}

// -------------------

/**
 * \brief Download listener object used to track progress of XPI downloads.
 * \internal
 */
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
      if ( (aStatus == NS_BINDING_ABORTED) && userCancel ) {
        // User canceled so delete the downloaded file if any and
        // skip this file and move on to the next one
        deleteLastDownloadedFile();
        // Give the user some time to notice the cancel.
        setTimeout(installNextXPI, 2000);
      } else {
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

/**
 * \brief Download an XPI file from a URL.
 * Downloads an XPI file from a URL to a temporary file on disk.
 * \param url The url of the file to download.
 * \return The destination filename.
 * \internal
 */
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

function cancelDownload() {

  if (!_browser) return;

  var sbs = Components.classes["@mozilla.org/intl/stringbundle;1"]
                      .getService(Components.interfaces.nsIStringBundleService);
  var songbirdStrings = sbs.createBundle("chrome://songbird/locale/songbird.properties");
  var cancelling = "Cancelling" + " " + bundle.getExtensionAttribute(cur_ext, "name");
  try {
    var params = [ bundle.getExtensionAttribute(cur_ext, "name") ];
    cancelling = songbirdStrings.formatStringFromName("setupprogress.cancelling",
                                                      params, params.length);
  } catch (e) {}
  label.setAttribute("value", cancelling);
  cancelButton.disabled = true;
  progressmeter.setAttribute("value", 0);
  userCancel = true;
  
  // Cancel the save, this will cause onStageChange to fire with a status
  // of NS_BINDING_ABORTED
  _browser.cancelSave();
}

/**
 * \brief Delete the last downloaded XPI file.
 * \internal
 */
function deleteLastDownloadedFile() {
  if (_file) {
    try {
      _file.remove(true);
    } catch (e) {}
    _file = null;
    _filename = null;
  }
}

/**
 * \brief Get a temporary file name.
 * \internal
 */
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

/**
 * \brief Generate a UUID.
 * \return The generated UUID.
 * \internal
 */
function generateUUID() {
  var aUUIDGenerator = (Components.classes["@mozilla.org/uuid-generator;1"]).createInstance();
  aUUIDGenerator = aUUIDGenerator.QueryInterface(Components.interfaces.nsIUUIDGenerator);
  return aUUIDGenerator.generateUUID();
}

/**
 * \brief Generate a random URL parameter.
 * \return The generated random URL parameter.
 * \internal
 */
function getRandomParameter() {
  return "?randomguid=" + escape(generateUUID());
}

/**
 * \brief Force installation of an XPI. 
 * This method will install the XPI without asking the user's permission.
 * Use "installXPI()" to involve the user in the install process.
 * \param localFilename The filename of the XPI to install.
 * \return Non-zero on success.
 * \internal
 */
function forceInstallXPI(localFilename)
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