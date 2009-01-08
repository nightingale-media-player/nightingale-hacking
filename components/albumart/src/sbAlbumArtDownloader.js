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
 * \brief sbAlbumArtDownloader downloads cover art using Mozillas
 *        nsIWebBrowserPersist and then returns the downloaded file to the
 *        sbIAlbumArtListener.
 */
function sbAlbumArtDownloader() {
}
sbAlbumArtDownloader.prototype = {
  className: "sbAlbumArtDownloader",
  constructor: sbAlbumArtDownloader,       // Constructor to this object
  classDescription: "Songbird Album Cover Downloader",
  classID: Components.ID("{dba6ac7a-f686-48fc-8a90-9b639cd6f6d9}"),
  contractID: "@songbirdnest.com/Songbird/album-art/downloader;1",
  
  // Global variables for when we are downloading.  
  _outputFile     : null,     // nsILocalFile we are saving to.
  _mediaItem      : null      // Media Item we are downloding for
};

/* nsISupports */
sbAlbumArtDownloader.prototype.QueryInterface =
XPCOMUtils.generateQI([Ci.nsIWebProgressListener, // For downloading images
                       Ci.sbIAlbumArtDownloader]);

/* nsIWebProgressListener methods */
// No need to implement anything in these functions
sbAlbumArtDownloader.prototype.onLocationChange = function (a, b, c) { }
sbAlbumArtDownloader.prototype.onProgressChange = function (a, b, c, d, e, f) { }
sbAlbumArtDownloader.prototype.onSecurityChange = function (a, b, c) { }
sbAlbumArtDownloader.prototype.onStatusChange = function (a, b, c, d) { }

/**
 * \brief onStateChange is fired when the state of the download changes,
 *        we are only looking for the STATE_STOP state that indicates the
 *        download is done.
 */
sbAlbumArtDownloader.prototype.onStateChange =
function sbAlbumArtDownloader_onStateChange(aWebProgress,
                                            aRequest,
                                            aStateFlags, 
                                            aStatus) {
  // when the transfer is complete...
  if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
    if (aStatus == 0) {
      this._downloadFinished();
    } else {
      this._listener.onResult(null, this._mediaItem);
    }
  }
}

/**
 * \brief DownloadFinished completes the download operation and notifies the
 *        listener that it has completed.
 */
sbAlbumArtDownloader.prototype._downloadFinished =
function sbAlbumArtDownloader__downloadFinished() {
  // Get the image data from the temporary file
  var imageMimeType;
  var imageData;
  [imageMimeType, imageData] = this._getImageData(this._outputFile.path);
  
  // Use the album art service to check if this is a valid image and if
  // so cache it to the profile artwork folder.
  var albumArtService =
                   Cc["@songbirdnest.com/Songbird/album-art-service;1"]
                     .getService(Ci.sbIAlbumArtService);

  var newFile = null;
  if (albumArtService.imageIsValidAlbumArt(imageMimeType,
                                           imageData,
                                           imageData.length)) {
    newFile = albumArtService.cacheImage(imageMimeType,
                                         imageData,
                                         imageData.length);
  }

  this._listener.onResult(newFile, this._mediaItem);
}

/**
 * \brief GetImageData reads the mime type and data from the temporary file
 *        into a byte array.
 */
sbAlbumArtDownloader.prototype._getImageData =
function sbAlbumArtDownloader__getImageData(imageFilePath) {
  var imageFile = Cc["@mozilla.org/file/local;1"]
                    .createInstance(Ci.nsILocalFile);
  imageFile.initWithPath(imageFilePath);
  
  var newMimeType = Cc["@mozilla.org/mime;1"]
                      .getService(Ci.nsIMIMEService)
                      .getTypeFromFile(imageFile);
  var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(Ci.nsIFileInputStream);
  inputStream.init(imageFile, 0x01, 0600, 0);
  var stream = Cc["@mozilla.org/binaryinputstream;1"]
                 .createInstance(Ci.nsIBinaryInputStream);
  stream.setInputStream(inputStream);
  var size = inputStream.available();

  // use a binary input stream to grab the bytes.
  var bis = Cc["@mozilla.org/binaryinputstream;1"].
            createInstance(Ci.nsIBinaryInputStream);
  bis.setInputStream(inputStream);
  var newImageData = bis.readByteArray(size);

  return [newMimeType, newImageData];
}

/* sbIAlbumArtDownloader methods */
sbAlbumArtDownloader.prototype.downloadImage =
function sbAlbumArtDownloader_downloadImage(aURL, aMediaItem, aListener) {
  // Check if we already have this item in the correct location
  var currentLocation = aMediaItem.getProperty(SBProperties.primaryImageURL);
  if (currentLocation == aURL) {
    aListener.onResult(uri, aMediaItem);
    return;
  }

  // Check if this is a web file to download
  var uri;
  var uri = Cc["@mozilla.org/network/standard-url;1"]
              .createInstance(Ci.nsIURI);
  uri.spec = aURL;

  switch(uri.scheme) {
    case "http":
    case "https":
    case "ftp":
    case "sftp":
      // These are ok to try and download so we will continue
    break;
    
    case "file":
    case "chrome":
      // These are already local files so no need to download
      aListener.onResult(uri, aMediaItem);
      return;
    break;
  
    default:
      // Everything else is probably not downloadable like file, chrome, data,
      // irc, etc so fail
      aListener.onResult(null, aMediaItem);
      return;
    break;
  }

  // default to .jpg
  var extension = '.jpg';
  try {
    // try to extract the extension from download URL
    extension = aURL.match(/\.[a-zA-Z0-9]+$/)[0];
  } catch(e) { }

  // Here we need to create a tempFile to save to which will be cached to the
  // proper location in the profile folder using the album art service.
  this._outputFile = Cc["@mozilla.org/file/directory_service;1"]
                       .getService(Ci.nsIProperties)
                       .get("TmpD", Ci.nsIFile);
  // Create a unique name
  var uuidGenerator = Cc["@mozilla.org/uuid-generator;1"]
                        .getService(Ci.nsIUUIDGenerator);
  var uuid = uuidGenerator.generateUUID();
  var uuidFileName = uuid.toString() + extension;
  
  // Create our temp file
  this._outputFile.append(uuidFileName);
  this._outputFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0777);
  if (!this._outputFile.isWritable) {
    aListener.onResult(null, aMediaItem);
    return;
  }

  // Make sure it goes away on exit
  var registerFileForDelete = Cc["@mozilla.org/uriloader/external-helper-app-service;1"]
                               .getService(Ci.nsPIExternalAppLauncher);
  registerFileForDelete.deleteTemporaryFileOnExit(this._outputFile);
  
  // Save this for when we have finished downloading
  this._listener = aListener;
  this._mediaItem = aMediaItem;
  
  // Use the nsIWebBrowserPersist to download the file.
  this._wbp = Cc['@mozilla.org/embedding/browser/nsWebBrowserPersist;1']
                .createInstance(Ci.nsIWebBrowserPersist);
  this._wbp.persistFlags = Ci.nsIWebBrowserPersist.PERSIST_FLAGS_NONE;
  this._wbp.progressListener = this;
  try {
    this._wbp.saveURI(uri,                // URI to save to file.
                      null,               // CacheKey (not used)
                      null,               // Referrer (not used)
                      null,               // PostData (not used)
                      null,               // ExtraHeaders (not used)
                      this._outputFile);  // Target File.
  } catch (err) {
    aListener.onResult(null, aMediaItem);
  }
}

/**
 * XPCOM Implementation
 */
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbAlbumArtDownloader]);
}
