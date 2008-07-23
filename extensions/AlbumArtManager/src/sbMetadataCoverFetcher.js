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


/* This XPCOM service knows how to ask Amazon.com where to find art for 
 * music albums. 
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

function sbMetadataCoverFetcher() {
  this.DEBUG = false;

  this._metadataManager = Cc["@songbirdnest.com/Songbird/MetadataManager;1"]
                            .getService(Ci.sbIMetadataManager);
  this._ioService = Cc['@mozilla.org/network/io-service;1']
    .getService(Ci.nsIIOService);
  var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                             .getService(Ci.nsIStringBundleService);
  this._bundle = stringBundleService.createBundle(STRING_BUNDLE);
  this._shutdown = false;

  this._prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);

  try {
    this._consoleService = Cc["@mozilla.org/consoleservice;1"]
                            .getService(Ci.nsIConsoleService);
    this.DEBUG = prefService.getBoolPref("songbird.albumartmanager.debug");
  } catch (err) {}
}
sbMetadataCoverFetcher.prototype = {
  className: "sbMetadataCoverFetcher",
  classDescription: 'Songbird Metadata Cover Fetcher',
  // Please generate a new classID with uuidgen.
  classID: Components.ID('{6f65689c-1d72-415b-b9e8-2835c65cc6c1}'),
  contractID: '@songbirdnest.com/Songbird/cover/metadata-fetcher;1'
}

/**
 * \brief Dumps out a message if the DEBUG flag is enabled with
 *        the className pre-appended.
 * \param message String to print out.
 */
sbMetadataCoverFetcher.prototype._debug =
function sbMetadataCoverFetcher__debug(message) {
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
sbMetadataCoverFetcher.prototype._logError =
function sbMetadataCoverFetcher__logError(message) {
  try {
    dump(this.className + ": [ERROR] - " + message + "\n");
    Cu.reportError(this.className + ": [ERROR] - " + message);
  } catch (err) {}
}


// nsISupports
sbMetadataCoverFetcher.prototype.QueryInterface =
XPCOMUtils.generateQI([Ci.sbICoverFetcher]);

sbMetadataCoverFetcher.prototype._xpcom_categories = [
  { category: "sbCoverFetcher" }
];

// sbICoverFetcher
sbMetadataCoverFetcher.prototype.__defineGetter__("shortName",
function sbMetadataCoverFetcher_name() {
  return "metadata";
});

sbMetadataCoverFetcher.prototype.__defineGetter__("name",
function sbMetadataCoverFetcher_name() {
  return SBString("albumartmanager.fetcher.metadata.name", null, this._bundle);
});

sbMetadataCoverFetcher.prototype.__defineGetter__("description",
function sbMetadataCoverFetcher_description() {
  return SBString("albumartmanager.fetcher.metadata.description", null, this._bundle);
});

sbMetadataCoverFetcher.prototype.__defineGetter__("userFetcher",
function sbMetadataCoverFetcher_userFetcher() {
  return true;
});

sbMetadataCoverFetcher.prototype.__defineGetter__("isEnabled",
function sbMetadataCoverFetcher_isEnabled() {
  var retVal = false;
  try {
    retVal = this._prefService.getBoolPref("songbird.albumartmanager.fetcher." +
                                            this.shortName + ".isEnabled");
  } catch (err) { }
  return retVal;
});
sbMetadataCoverFetcher.prototype.__defineSetter__("isEnabled",
function sbMetadataCoverFetcher_isEnabled(aNewVal) {
  this._prefService.setBoolPref("songbird.albumartmanager.fetcher." +
                                  this.shortName + ".isEnabled",
                                aNewVal);
});

/**
 * \brief Converts a binary array to hex values with out the 0x for output
 *        to the file system.
 * \param aHashData - Array of bytes to convert to hex.
 * \return string of hex values with out 0x
 */
sbMetadataCoverFetcher.prototype._convertBinaryToHexString = 
function sbMetadataCoverFetcher__convertBinaryToHexString(aHashData) {
  var outString = "";
  for (var hashIndex = 0; hashIndex < aHashData.length; hashIndex++) {
    var hashValue = aHashData.charCodeAt(hashIndex);
    var hexValue = ("0" + hashValue.toString(16)).slice(-2);
    outString += hexValue;
  }
  return outString;
}

/**
 * TODO: Since Songbird does this same thing we should pull it out to a common
 *       place.
 * \brief Saves image data to a file.
 * \param aImageData - Binary array of image data
 * \param aImageDataSize - size of binary array
 * \param aMimeType - mime type of image (image/png, image/jpg, etc)
 * \return location of file created with "file://" pre-appended
 */
sbMetadataCoverFetcher.prototype._saveDataToFile = 
function sbMetadataCoverFetcher__saveDataToFile(aImageData,
                                                aImageDataSize,
                                                aMimeType) {
  // Generate a hash of the imageData for the filename
  var cHash = Cc["@mozilla.org/security/hash;1"]
                .createInstance(Ci.nsICryptoHash);
  cHash.init(Ci.nsICryptoHash.MD5);
  cHash.update(aImageData, aImageDataSize);
  var hash = cHash.finish(false);
  var fileName = this._convertBinaryToHexString(hash);

  // grab the extension from the mimetype
  var ext = aMimeType.toLowerCase();
  if (ext.indexOf("/") > 0) {
    ext = ext.substr(ext.indexOf("/") + 1);
  }

  // Get the profile folder
  var dir = Cc["@mozilla.org/file/directory_service;1"]
                  .createInstance(Ci.nsIProperties);
  var coverFile = dir.get("ProfD", Ci.nsIFile);
  coverFile.append("artwork");
  coverFile.append(fileName + "." + ext);

  var outFilePath = this._ioService.newFileURI(coverFile);

  // save data to the file
  try {
    coverFile.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0655);
  } catch (err) {
    // File already exists
    return outFilePath.spec;
  }

  var outputFileStream =  Cc["@mozilla.org/network/file-output-stream;1"]
                            .createInstance(Ci.nsIFileOutputStream);
  outputFileStream.init(coverFile, -1, -1, 0);
  var outputStream = outputFileStream.QueryInterface(Components.interfaces.nsIOutputStream);

  var binaryOutput = Cc["@mozilla.org/binaryoutputstream;1"]
                       .createInstance(Ci.nsIBinaryOutputStream);
  binaryOutput.setOutputStream(outputStream);
  binaryOutput.writeByteArray(aImageData, aImageDataSize);

  return outFilePath.spec;
}

sbMetadataCoverFetcher.prototype.fetchCoverForMediaItem = 
function sbMetadataCoverFetcher_fetchCoverForMediaItem(aMediaItem, aListener, aWindow) {
  if (this._shutdown) {
    return;
  }

  var contentURI = this._ioService.newURI(
                                aMediaItem.getProperty(SBProperties.contentURL),
                                null,
                                null);

  // if the URL isn't valid, fail
  if (!contentURI) {
    aListener.coverFetchFailed(aMediaItem, null);
    return;
  }

  // if the URL isn't local, fail
  var fileURL = contentURI.QueryInterface(Ci.nsIFileURL);
  if (!fileURL) {
    aListener.coverFetchFailed(aMediaItem, null);
    return;
  }

  // the result is scoped to the artist & album
  var scope = Cc['@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1']
               .createInstance(Ci.sbIMutablePropertyArray);
  scope.appendProperty(SBProperties.albumName, 
                       aMediaItem.getProperty(SBProperties.albumName));
  scope.appendProperty(SBProperties.artistName, 
                       aMediaItem.getProperty(SBProperties.artistName));

  this._debug("Getting handler for mediaItem: " + fileURL.spec);
  var handler = this._metadataManager.getHandlerForMediaURL(fileURL.spec);
  
  try {
    this._debug("Reading metadata from item [FRONTCOVER].");
    var mimeTypeOutparam = {};
    var outSize = {};
    var imageData = handler.getImageData(Ci.sbIMetadataHandler
                                          .METADATA_IMAGE_TYPE_FRONTCOVER,
                                          mimeTypeOutparam,
                                          outSize);
    if (outSize.value <= 0) {
      this._debug("Reading metadata from item [OTHER].");
      // Try the OTHER cover
      imageData = handler.getImageData(Ci.sbIMetadataHandler
                                          .METADATA_IMAGE_TYPE_OTHER,
                                          mimeTypeOutparam,
                                          outSize);
    }
    
    if (outSize.value > 0) {
      this._debug("Found an image in the metadata.");
      var outFileLocation = this._saveDataToFile(imageData,
                                                 imageData.length,
                                                 mimeTypeOutparam.value);
      if (outFileLocation) {
        this._debug("Saved data to file: " + outFileLocation);
        aListener.coverFetchSucceeded(aMediaItem, scope, outFileLocation);
      } else {
        this._logError("Unable to save file: [" + outFileLocation + "]");
        aListener.coverFetchFailed(aMediaItem, scope);
      }
    } else {
      this._logError("Unable to find metadata: [" + fileURL.spec + "]");
      aListener.coverFetchFailed(aMediaItem, scope);
    }
  } catch (err) {
    this._logError("Unable to get image from metadata - " + err);
    aListener.coverFetchFailed(aMediaItem, scope);
  }
}

sbMetadataCoverFetcher.prototype.shutdown =
function sbMetadataCoverFetcher_shutdown() {
  this._shutdown = true;
}



function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbMetadataCoverFetcher]);
}




