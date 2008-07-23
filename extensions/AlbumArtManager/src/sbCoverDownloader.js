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

// Constants for convience
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

// Imports to help with some common tasks
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

/**
 * \brief CoverDownloader downloads cover art using Mozillas
 *        nsIWebBrowserPersist and then returns the downloaded file to the
 *        sbICoverListener.
 */
function sbCoverDownloader() {
  // Debug flag to indicate if we should spew junk on the console
  this.DEBUG = false;

  try {
    this._consoleService = Cc["@mozilla.org/consoleservice;1"]
                            .getService(Ci.nsIConsoleService);
    this._prefService = Cc['@mozilla.org/preferences-service;1']
                          .getService(Ci.nsIPrefBranch);
    this.DEBUG = this._prefService.getBoolPref(
                                            "songbird.albumartmanager.debug." +
                                             this.className);
  } catch (err) {
    // Don't need this anymore since we will not output debug statements
    this._consoleService = null;
  }
}
sbCoverDownloader.prototype = {
  className: "sbCoverDownloader",
  classDescription: "Songbird Album Cover Downloader",
  classID: Components.ID("{1a5f4f10-5578-464b-abfd-6c1bf7973ea1}"),
  contractID: "@songbirdnest.com/Songbird/sbCoverDownloader;1",
  
  _consoleService: null,      // We use this in the debug calls
  _prefService   : null,
  
  _outputFile    : ""         // String of the file we are saving to.
}

/**
 * Internal debugging functions
 */
/**
 * \brief Dumps out a message with this.className pre-appended if the DEBUG flag
 *        is enabled.
 * \param message String to print out.
 */
sbCoverDownloader.prototype._debug =
function sbCoverDownloader__debug(message)
{
  if (!this.DEBUG) return;
  if (!this.className) return;
  if (!this._consoleService) return;
  if (!message) return;
  dump(this.className + ": " + message + "\n");
  this._consoleService.logStringMessage(this.className + ": " + message);
}

/**
 * \brief Dumps out an error message with this.className + ": [ERROR]"
 *        pre-appended, and will report the error so it will appear in the
 *        error console.
 * \param message String to print out.
 */
sbCoverDownloader.prototype._logError =
function sbCoverDownloader__logError(message)
{
  if (!this.className) return;
  if (!message) return;
  dump(this.className + ": [ERROR] - " + message + "\n");
  Cu.reportError(this.className + ": [ERROR] - " + message);
}

/* nsISupports */
sbCoverDownloader.prototype.QueryInterface =
XPCOMUtils.generateQI([Ci.nsIWebProgressListener, // For downloading images
                       Ci.sbICoverDownloader]);

/* nsIWebProgressListener methods */
// No need to implement anything in these functions
sbCoverDownloader.prototype.onLocationChange = function (a, b, c) { }
sbCoverDownloader.prototype.onProgressChange = function (a, b, c, d, e, f) { }
sbCoverDownloader.prototype.onSecurityChange = function (a, b, c) { }
sbCoverDownloader.prototype.onStatusChange = function (a, b, c, d) { }

/**
 * \brief onStateChange is fired when the state of the download changes,
 *        we are only looking for the STATE_STOP state that indicates the
 *        download is done.
 */
sbCoverDownloader.prototype.onStateChange =
function sbCoverDownloader_onStateChange(aWebProgress,
                                         aRequest,
                                         aStateFlags, 
                                         aStatus) {
  this._debug('onStateChange aStatus = ' + aStatus);

  // if we're shutting down, do nothing
  if (this._shutdown) {
    return;
  }

  // when the transfer is complete...
  if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
    if (aStatus == 0) {
      this._debug("Download finished");
      var ioService = Cc["@mozilla.org/network/io-service;1"]
                        .getService(Ci.nsIIOService);
      var outFile = this._ioService.newFileURI(this._outputFile).spec;
      this._listener.coverFetchSucceeded(this._mediaItem,
                                         this._scope,
                                         outFile);
    } else {
      this._debug("Download failed");
      this._listener.coverFetchFailed(this._mediaItem, this._scope);
    }
  }
}

/**
 * \brief Gets th platform we are on, we need a better way of doing this.
 * \returns String version of platform (Windows_NT, Darwin, Linux, SunOS)
 */
sbCoverDownloader.prototype._getPlatform =
function sbCoverDownloader__getPlatform() {
  var platform = "";
  try {
    var sysInfo =
      Components.classes["@mozilla.org/system-info;1"]
                .getService(Components.interfaces.nsIPropertyBag2);
    platform = sysInfo.getProperty("name");
  }
  catch (e) {
    dump("System-info not available, trying the user agent string.\n");
    var user_agent = navigator.userAgent;
    if (user_agent.indexOf("Windows") != -1)
      platform = "Windows_NT";
    else if (user_agent.indexOf("Mac OS X") != -1)
      platform = "Darwin";
    else if (user_agent.indexOf("Linux") != -1)
      platform = "Linux";
    else if (user_agent.indexOf("SunOS") != -1)
      platform = "SunOS";
  }
  return platform;
}

/**
 * \brief MakeFileNameSafe takes a file name and changes any invalid characters
 *        into valid ones.
 *        TODO: Check if the ioService will do this for use with newFileURI
 * \param aFileName - file name to make safe
 * \return valid version of aFileName.
 */
sbCoverDownloader.prototype.__makeFileNameSafe =
function sbCoverDownloader__makeFileNameSafe(aFileName) {
  var newFileName;
  var platform = this._getPlatform();
  
  // Convert invalid chars into good ones
  switch (platform) {
    case 'Windows_NT':
      // Windows has so many bad characters
      // * " / \ : | ? < >
      newFileName = aFileName.replace(/[*|\"|\/|\\|\||:|\?|<|>]/g, "_");
    break;
  
    case 'Darwin':
      // :
      newFileName = aFileName.replace(/[:]/g, "_");
      // Dot can not be the first character
      if (newFileName.indexOf(".") == 0) {
        newFileName = newFileName.slice(1);
      }
    break;
    
    case 'Linux':
      // / \
      newFileName = aFileName.replace(/[\/|\\]/g, "_");
    break;
    
    default:
      newFileName = aFileName;
    break;
  }
  
  return newFileName;
}

/**
 * \brief Helper function for getting a property from a media item with a
 *        default value if none found.
 * \param aMediaItem item to get property from
 * \param aPropertyName property to get from item
 * \param aDefaultValue default value to use if property had no value
 * \return property value or default value passed in if not available
 */
sbCoverDownloader.prototype._getProperty =
function sbCoverDownloader__getProperty(aMediaItem,
                                        aPropertyName,
                                        aDefaultValue) {
  var propertyValue = aMediaItem.getProperty(aPropertyName);
  if ( (propertyValue == null) ||
       (propertyValue == "") ) {
    return aDefaultValue;
  } else {
    return propertyValue;
  }
}

/**
 * \brief Determines the download location based on preferences.
 *
 * \param aMediaItem item to generate download url for
 * \param aExtension optional extension to add on to the file
 * \return a nsIFile of the location to download to minus an extension if
 *         aExtension is null, otherwise full location.
 */
sbCoverDownloader.prototype._getCoverDownloadLocation =
function sbCoverDownloader__getCoverDownloadLocation(aMediaItem,
                                                     aExtension)
{
  var albumName = this._getProperty(aMediaItem, SBProperties.albumName, "");
  var albumArtist = this._getProperty(aMediaItem, SBProperties.artistName, "");
  var trackName = this._getProperty(aMediaItem, SBProperties.trackName, "");
  var contentURL = aMediaItem.getProperty(SBProperties.contentURL);
  var itemGuid = aMediaItem.guid;
  
  var toAlbumLocation = true;
  try {
    toAlbumLocation = this._prefService
            .getBoolPref("songbird.albumartmanager.cover.usealbumlocation");
  } catch (err) {
    this._prefService
      .setBoolPref("songbird.albumartmanager.cover.usealbumlocation", true);
  }

  var downloadLocation = null;
  if (toAlbumLocation) {
    var uri;
    try {
      var fph = this._ioService.getProtocolHandler("file")
                    .QueryInterface(Ci.nsIFileProtocolHandler);
      var file = fph.getFileFromURLSpec(contentURL);
      if (file.parent) {
        downloadLocation = file.parent;
      }
    } catch (err) {
      this._logError("Error getting content location: " + err);
      downloadLocation = null;
    }
  } else {
    try {
      var otherLocation = this._prefService.getComplexValue(
                                      "songbird.albumartmanager.cover.folder",
                                      Ci.nsILocalFile);
      if (otherLocation) {
        downloadLocation = otherLocation.QueryInterface(Ci.nsIFile);
      }
    } catch (err) {
      this._logError("Unable to get download location: " + err);
      downloadLocation = null;
    }
  }
  
  if (downloadLocation != null) {
    var coverFormat = this._prefService.getCharPref(
                                  "songbird.albumartmanager.cover.format");
    if (coverFormat == "" || coverFormat == null) {
      coverFormat = "%artist% - %album%";
    }
    
    coverFormat = coverFormat.replace(/%album%/gi, this._makeFileNameSafe(albumName));
    coverFormat = coverFormat.replace(/%artist%/gi, this._makeFileNameSafe(albumArtist));
    coverFormat = coverFormat.replace(/%track%/gi, this._makeFileNameSafe(trackName));
    coverFormat = coverFormat.replace(/%guid%/gi, this._makeFileNameSafe(itemGuid));

    if (aExtension) {
      coverFormat = coverFormat + aExtension;
    }

    // Have to append each folder if the user is using \ or / in the format
    // we can not seem to append something like "folder/folder/folder" :P
    try {
      if ( (coverFormat.indexOf("/") > 0) ||
           (coverFormat.indexOf("\\") > 0) ) {
        this._debug("Splitting " + coverFormat);
        var sections = coverFormat.split("/");
        if (sections.length == 1) {
          this._debug("/ didn't work so trying \\");
          sections = coverFormat.split("\\");
        }
        this._debug("Split into " + sections.length + " sections");
        for (var i = 0; i < sections.length; i++) {
          if (sections[i].indexOf("\\")) {
            var subSections = sections.split("\\");
            for (var iSub = 0; iSub < subSections.length; iSub++) {
              var appendFolder = this._makeFileNameSafe(subSections[iSub]);
              this._debug("Appending " + appendFolder);
              downloadLocation.append(appendFolder);
            }
          } else {
            var appendFolder = this._makeFileNameSafe(sections[i]);
            this._debug("Appending " + appendFolder);
            downloadLocation.append(appendFolder);
          }
        }
      } else {
        this._debug("No need to split " + coverFormat);
        coverFormat = this._makeFileNameSafe(coverFormat);
        downloadLocation.append(coverFormat);
      }
    } catch (err) {
      this._logError("Unable to create downloadLocation with " + coverFormat);
      downloadLocation = null;
    }
  }

  return downloadLocation;
}

/* sbICoverDownloader methods */
sbCoverDownloader.prototype.downloadCover =
function sbCoverDownloader_downloadCover( aURI,
                                          aMediaItem,
                                          aListener,
                                          aScope) {
  this._debug('downloadCover: ' + aURI);
  // we'll need the io service for a couple of things...
  var ioService =  Cc['@mozilla.org/network/io-service;1']
                    .getService(Ci.nsIIOService);

  // track shutdown state
  this._shutdown = false;

  // save off the arguments we'll be needing later...
  this._listener = aListener;
  this._mediaItem = aMediaItem;
  this._scope = aScope;

  // default to .jpg
  var extension = '.jpg';
  try {
    // try to extract the extension from download URL
    extension = aURI.match(/\.[a-zA-Z0-9]+$/)[0];
  } catch(e) {
    this._logError("Failed to extract the extension from :" + aURI + " - " + e);
  }

  // work out where we'll save this
  var file = this._getCoverDownloadLocation(aMediaItem. extension)
  if (file) {
    if (!file.exists()) {
      // Make sure we can create the file
      try {
        file.create(Ci.NORMAL_FILE_TYPE, 0664);
      } catch (err) {
        this._logError("Unable to create file: [" + location + extension + "] - " + err);
        this._listener.coverFetchFailed(aMediaItem, aScope);
        return;
      }
    }

    this._outputFile = file.path;
    this._debug("Downloading image from: [" + aURI + "] to [" +
                 this._outputFile + "]");

    this._wbp = Cc['@mozilla.org/embedding/browser/nsWebBrowserPersist;1']
      .createInstance(Ci.nsIWebBrowserPersist);
    this._wbp.persistFlags = Ci.nsIWebBrowserPersist.PERSIST_FLAGS_NONE;
    this._wbp.progressListener = this;
    this._debug('beginning download');
    this._wbp.saveURI(ioService.newURI(aURI, null, null), null, null, null, 
        null, file);
  } else {
    this._debug("couldn't determine the cover location");
    this._listener.coverFetchFailed(aMediaItem, aScope);
  }
}

sbCoverDownloader.prototype.shutdown =
function sbCoverDownloader_shutdown() {
  this._debug('shutdown');
  this._shutdown = true;
}

/**
 * XPCOM Implementation
 */
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([
    sbCoverDownloader,
  ],
  function(aCompMgr, aFileSpec, aLocation) {
    XPCOMUtils.categoryManager.addCategoryEntry(
      "app-startup",
      sbCoverDownloader.prototype.classDescription,
      sbCoverDownloader.prototype.contractID,
      true,
      true);
  });
}

