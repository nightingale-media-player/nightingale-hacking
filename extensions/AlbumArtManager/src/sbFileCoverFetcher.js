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


/* This XPCOM service knows how to find album art that's stored on your
 * local disk.
 */

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

const STRING_BUNDLE = "chrome://albumartmanager/locale/albumartmanager.properties";

Cu.import('resource://app/jsmodules/sbProperties.jsm');
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

function sbFileCoverFetcher() {
  this.DEBUG = false;
  this._ioService = Cc['@mozilla.org/network/io-service;1']
    .getService(Ci.nsIIOService);
  this._albumArtService = Cc['@songbirdnest.com/songbird-album-art-service;1']
    .getService(Ci.sbIAlbumArtService);
  var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                             .getService(Ci.nsIStringBundleService);
  this._bundle = stringBundleService.createBundle(STRING_BUNDLE);
  this._shutdown = false;

  this._prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);

  try {
    this._consoleService = Cc["@mozilla.org/consoleservice;1"]
                            .getService(Ci.nsIConsoleService);
    this._prefService = Cc['@mozilla.org/preferences-service;1']
                          .getService(Ci.nsIPrefBranch);
    this.DEBUG = prefService.getBoolPref("songbird.albumartmanager.debug");
  } catch (err) {}
}
sbFileCoverFetcher.prototype = {
  className: "sbFileCoverFetcher",
  classDescription: "Songbird Local File Album Cover Fetcher",
  // Please generate a new classID with uuidgen.
  classID: Components.ID("{ca2da771-c58a-44cf-8b58-139f553d62a1}"),
  contractID: "@songbirdnest.com/Songbird/cover/file-fetcher;1"
}

/**
 * \brief Dumps out a message if the DEBUG flag is enabled with
 *        the className pre-appended.
 * \param message String to print out.
 */
sbFileCoverFetcher.prototype._debug =
function sbFileCoverFetcher__debug(message) {
  if (!this.DEBUG) return;
  try {
    dump(this.className + ": " + message + "\n");
    this._consoleService.logStringMessage(this.className + ": " + message);
  } catch (err) {}
}

/**
 * \brief Dumps out an error message with "the className and "[ERROR]"
 *        pre-appended, and will report the error so it will appear in the
 *        error console.
 * \param message String to print out.
 */
sbFileCoverFetcher.prototype._logError =
function sbFileCoverFetcher__logError(message) {
  try {
    dump(this.className + ": [ERROR] - " + message + "\n");
    Cu.reportError(this.className + ": [ERROR] - " + message);
  } catch (err) {}
}

// nsISupports
sbFileCoverFetcher.prototype.QueryInterface =
XPCOMUtils.generateQI([Ci.sbICoverFetcher]);

sbFileCoverFetcher.prototype._xpcom_categories = [
  { category: "sbCoverFetcher" }
];

// sbICoverFetcher
sbFileCoverFetcher.prototype.__defineGetter__("shortName",
function sbFileCoverFetcher_name() {
  return "file";
});

sbFileCoverFetcher.prototype.__defineGetter__("name",
function sbFileCoverFetcher_name() {
  return SBString("albumartmanager.fetcher.file.name", null, this._bundle);
});

sbFileCoverFetcher.prototype.__defineGetter__("description",
function sbFileCoverFetcher_description() {
  return SBString("albumartmanager.fetcher.file.description", null, this._bundle);
});

sbFileCoverFetcher.prototype.__defineGetter__("userFetcher",
function sbFileCoverFetcher_userFetcher() {
  return true;
});

sbFileCoverFetcher.prototype.__defineGetter__("isEnabled",
function sbFileCoverFetcher_isEnabled() {
  var retVal = false;
  try {
    retVal = this._prefService.getBoolPref("songbird.albumartmanager.fetcher." +
                                            this.shortName + ".isEnabled");
  } catch (err) { }
  return retVal;
});
sbFileCoverFetcher.prototype.__defineSetter__("isEnabled",
function sbFileCoverFetcher_isEnabled(aNewVal) {
  this._prefService.setBoolPref("songbird.albumartmanager.fetcher." +
                                  this.shortName + ".isEnabled",
                                aNewVal);
});

sbFileCoverFetcher.prototype.fetchCoverForMediaItem = 
function sbFileCoverFetcher_fetchCoverForMediaItem(aMediaItem, aListener, aWindow) {
  if (this._shutdown) {
    return;
  }

  if (aWindow) {
    // Open the file picker
    var filePicker = Cc["@mozilla.org/filepicker;1"]
                        .createInstance(Ci.nsIFilePicker);
    var windowTitle = SBString("albumartmanager.filepicker.windowtitle", null, this._bundle);
    filePicker.init( aWindow, windowTitle,
                    Ci.nsIFilePicker.modeOpen);
    var fileResult = filePicker.show();
    if (fileResult == Ci.nsIFilePicker.returnOK) {
      aListener.coverFetchSucceeded(aMediaItem,
                                    null,
                                    "file://" + filePicker.file.path);
    } else {
      this._debug("User canceled file picker dialog");
      aListener.coverFetchFailed(aMediaItem, null);
    }
  } else {
    // where would we store this media item's cover?
    var location = this._albumArtService.getCoverDownloadLocation(aMediaItem);
    var extensions = ['.jpg','.jpeg', '.png', '.gif', '.tif', '.tiff', '.bmp'];
    var extCheck = /.+\.(jpg|jpeg|png|gif|tif|tiff|bmp])$/;
    var fph = this._ioService.getProtocolHandler("file")
                  .QueryInterface(Ci.nsIFileProtocolHandler);

    if (location) {
      var filePath = location.path;
      for (var i = 0; i < extensions.length; i++) {
        var fullFile = filePath + extensions[i];
        var file = fph.getFileFromURLSpec("file://" + fullFile);
        this._debug("Checking for file: " + file.path);
        if (file.exists()) {
          var outFile = this._ioService.newFileURI(file).spec;
          aListener.coverFetchSucceeded(aMediaItem, null, outFile);
          return;
        }
      }
    }

    // If we didn't find a cover that is based on the user preferences
    // Then we should scan through a bunch of defaults filenames in the
    // 
    var contentURL = aMediaItem.getProperty(SBProperties.contentURL);
    try {
      var file = fph.getFileFromURLSpec(contentURL);
      if (file.parent) {
        location = file.parent;
      }
    } catch (err) {
      this._logError("Error getting content location: " + err);
      location = null;
    }

    var dirList = location.directoryEntries;
    while (dirList.hasMoreElements()) {
      var testFile = dirList.getNext().QueryInterface(Ci.nsIFile);
      if (testFile.isFile) {
        var result = testFile.leafName.match(extCheck);
        if (result) {
          var outFile = this._ioService.newFileURI(testFile).spec;
          this._debug("Found possible cover image as: " + outFile);
          aListener.coverFetchSucceeded(aMediaItem, null, outFile);
          return;
        } else {
          this._debug("Bummer " + testFile.leafName + " did not match");
        }
      }
    }
    
    // Failed to find a cover
    this._debug("No covers found on file system.");
    aListener.coverFetchFailed(aMediaItem, null);
  }
}

// sbICoverFetcher
sbFileCoverFetcher.prototype.shutdown =
function sbFileCoverFetcher_shutdown() {
  this._shutdown = true;
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbFileCoverFetcher]);
}



