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

/**
 * \brief Test file
 */

var testWindow;
var testWindowFailed;
var testServer;
var testBrowserWindow;
var testBrowser;
var testListener;

function beginWindowTest(url, continueFunction) {
  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"]
             .getService(Ci.nsIWindowWatcher);

  testWindow = ww.openWindow(null, url, null, null, null);
  testWindow.addEventListener("load", function() {
    continueFunction.apply(this);
  }, false);
  testWindowFailed = false;
  testPending();
}

function setAllAccess () {
  log("");
  log("--------------------------------------------------------------------------------");
  log("*********************** Setting All Access for RemoteAPI ***********************");
  log("--------------------------------------------------------------------------------");
  log("");
  setRapiPref("playback_control_disable", false);
  setRapiPref("playback_read_disable", false);
  setRapiPref("library_read_disable", false);
  setRapiPref("library_write_disable", false);
  setRapiPref("library_create_disable", false);
}

function setNoAccess() {
  log("");
  log("--------------------------------------------------------------------------------");
  log("*********************** Setting No Access for RemoteAPI ************************");
  log("--------------------------------------------------------------------------------");
  log("");
  setRapiPref("playback_control_disable", true);
  setRapiPref("playback_read_disable", true);
  setRapiPref("library_read_disable", true);
  setRapiPref("library_write_disable", true);
  setRapiPref("library_create_disable", true);
}

function cleanup() {
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                          .getService(Ci.sbILibraryManager);
  libraryManager.mainLibrary.clear();
}

function endWindowTest(e) {
  cleanup();
  if (!testWindowFailed) {
    testWindowFailed = true;
    if (testBrowser && testListener) {
      testBrowser.webProgress.removeProgressListener(testListener);
      testBrowser = null;
      testListener = null;
    }
    if (testWindow) {
      testWindow.close();
      testWindow = null;
    }
    if (e) {
      fail(e);
    }
    else {
      testFinished();
    }
  }
}

function continueWindowTest(fn, parameters) {

  try {
    fn.apply(this, parameters);
  }
  catch(e) {
    endWindowTest();
    fail(e);
  }
}

function safeSetTimeout(closure, timeout) {
  testWindow.setTimeout(function() {
    try {
      closure.apply(this);
    }
    catch(e) {
      endWindowTest();
      fail(e);
    }
  }, timeout);

}

function beginRemoteAPITest(page, continueFunction) {

  var port = getTestServerPortNumber();

  testServer = Cc["@mozilla.org/server/jshttp;1"]
                 .createInstance(Ci.nsIHttpServer);
  testServer.start(port);
  testServer.registerDirectory("/", getFile("."));

  var url = "data:application/vnd.mozilla.xul+xml," +
            "<?xml-stylesheet href='chrome://global/skin' type='text/css'?>" +
            "<?xml-stylesheet href='chrome://songbird/content/bindings/bindings.css' type='text/css'?>" +
            "<window xmlns='http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul'/>";

  beginWindowTest(url, function() { setupBrowser(page, port, continueFunction); })
}

function setupBrowser(page, port, continueFunction) {

  testWindow.resizeTo(200, 200);

  const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

  var document = testWindow.document;

  testBrowser = document.createElementNS(XUL_NS, "browser");
  testBrowser.setAttribute("flex", "1");
  document.documentElement.appendChild(testBrowser);

  var url = "http://127.0.0.1:" + port + "/" + page + "?" + Math.random();
  testListener = new ContinuingWebProgressListener(url, continueFunction);
  testBrowser.webProgress.addProgressListener(testListener, Ci.nsIWebProgress.NOTIFY_STATE_REQUEST);
  testBrowser.loadURI(url);
  testBrowserWindow = testBrowser.contentWindow;
}

function endRemoteAPITest(e) {
  testServer.stop();
  endWindowTest(e);
}

function getFile(fileName) {
  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("resource:app", Ci.nsIFile);
  file = file.clone();
  file.append("testharness");
  file.append("remoteapi");
  file.append(fileName);
  return file;
}

function ContinuingWebProgressListener(url, func) {
  this._url = url;
  this._func = func;
}

ContinuingWebProgressListener.prototype.onStateChange =
function(aProgress, aRequest, aFlag, aStatus)
{
  if(aFlag & Ci.nsIWebProgressListener.STATE_STOP && aRequest.name == this._url) {
    this._func.apply();
  }
}

ContinuingWebProgressListener.prototype.onLocationChange =
function(aProgress, aRequest, aURI) {}
ContinuingWebProgressListener.prototype.onProgressChange = function() {}
ContinuingWebProgressListener.prototype.onStatusChange = function() {}
ContinuingWebProgressListener.prototype.onSecurityChange = function() {}
ContinuingWebProgressListener.prototype.onLinkIconAvailable = function() {}

ContinuingWebProgressListener.prototype.QueryInterface =
function(aIID)
{
  if (aIID.equals(Ci.nsIWebProgressListener) ||
      aIID.equals(Ci.nsISupportsWeakReference) ||
      aIID.equals(Ci.nsISupports))
    return this;
  throw Cr.NS_NOINTERFACE;
}

function setRapiPref(name, value) {
  var prefs = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefService);
  prefs = prefs.getBranch("songbird.rapi.");
  prefs.setBoolPref(name, value);
}

var dest;
function setTempDownloadDir() {
  dest = Cc["@mozilla.org/file/directory_service;1"]
             .getService(Ci.nsIProperties)
             .get("TmpD", Ci.nsIFile);

  dest.append("remoteapi_test");

  var drCtor = new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1", "sbIDataRemote", "init");
  var dlFolder = new drCtor("download.folder", null);
  var dlAlways = new drCtor("download.always", null);
  dlFolder.stringValue = dest.path;
  dlAlways.boolValue = true;
}

initMockCore();
