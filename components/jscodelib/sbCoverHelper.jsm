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

EXPORTED_SYMBOLS = [ "sbCoverHelper" ];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

// File operation constants for init (-1 is default mode)
const FLAGS_DEFAULT = -1;

var sbCoverHelper = {
  /**
   * \brief Reads the image data from a local file.
   * \param aInputFile the nsIFile to read the image data from.
   * \return array of imageData and the mimeType or null on failure.
   */
  readImageData: function (aInputFile) {
    if(!aInputFile.exists()) {
      return null;
    }

    try {
      // Try and read the mime type from the file
      var newMimeType = Cc["@mozilla.org/mime;1"]
                          .getService(Ci.nsIMIMEService)
                          .getTypeFromFile(aInputFile);
      
      // Read in the data
      var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                          .createInstance(Ci.nsIFileInputStream);
      inputStream.init(aInputFile, FLAGS_DEFAULT, FLAGS_DEFAULT, 0);

      // Read from a binaryStream so we can get primitive data (bytes)
      var binaryStream = Cc["@mozilla.org/binaryinputstream;1"]
                           .createInstance(Ci.nsIBinaryInputStream);
      binaryStream.setInputStream(inputStream);
      var size = binaryStream.available();
      var newImageData = binaryStream.readByteArray(size);
      binaryStream.close();
      
      // Make sure we read as many bytes as we expected to.
      if (newImageData.length != size) {
        return null;
      }
      
      return [newImageData, newMimeType];
    } catch (err) {
      Cu.reportError("sbCoverHelper: Unable to read file image data: " + err);
    }
    
    return null;
  },

  /**
   * \brief Reads a file and saves that to our ProfLD/artwork folder.
   * \param aFromFile - File to read image data from.
   * \return String version of the filename to the new image saved to the
   *         artwork folder, or null if an error occurs.
   */
  saveFileToArtworkFolder: function (aFromFile) {
    try {
      var imageData;
      var mimeType;
      [imageData, mimeType] = this.readImageData(aFromFile);

      // Save the image data out to the new file in the artwork folder
      var metadataImageScannerService =
                        Cc["@songbirdnest.com/Songbird/Metadata/ImageScanner;1"]
                          .getService(Ci.sbIMetadataImageScanner);
      var newFile = metadataImageScannerService
                                          .saveImageDataToFile(imageData,
                                                               imageData.length,
                                                               mimeType);
      return newFile;
    } catch (err) {
      Cu.reportError("sbCoverHelper: Unable to save file image data: " + err);
    }
    
    return null;
  },

  /**
   * \brief Downloads a file from the web and then saves it to the artwork
   *        folder using saveFileToArtworkFolder.
   * \param aWebURL string of a web url to download file from.
   * \param aCallback function to call back with the new image file name.
   */
  downloadFile: function (aWebURL, aCallback) {
    // The tempFile we are saving to.
    var tempFile = Cc["@mozilla.org/file/directory_service;1"]
                     .getService(Ci.nsIProperties)
                     .get("TmpD", Ci.nsIFile);
    
    var self = this;
    var webProgressListener = {
      QueryInterface: XPCOMUtils.generateQI([Ci.nsIWebProgressListener]),

      /* nsIWebProgressListener methods */
      // No need to implement anything in these functions
      onLocationChange : function (a, b, c) { },
      onProgressChange : function (a, b, c, d, e, f) { },
      onSecurityChange : function (a, b, c) { },
      onStatusChange : function (a, b, c, d) { },
      onStateChange : function (aWebProgress, aRequest, aStateFlags, aStatus) {
        // when the transfer is complete...
        if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP) {
          if (aStatus == 0) {
            try {
              var fileName = self.saveFileToArtworkFolder(tempFile);
              aCallback(fileName);
            } catch (err) { }
          } else { }
        }
      }
    };
    
    // Create the temp file to download to
    var extension = null;
    try {
      // try to extract the extension from download URL
      // If this fails then the saveFileToArtworkFolder call will get it from
      // the contents.
      extension = aWebURL.match(/\.[a-zA-Z0-9]+$/)[0];
    } catch(e) { }

    var uuidGenerator = Cc["@mozilla.org/uuid-generator;1"]
                          .getService(Ci.nsIUUIDGenerator);
    var uuid = uuidGenerator.generateUUID();
    var uuidFileName = uuid.toString();
    
    uuidFileName = uuidFileName + extension;
    tempFile.append(uuidFileName);
    tempFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0644);

    // Make sure it is deleted when we shutdown
    var registerFileForDelete = Cc["@mozilla.org/uriloader/external-helper-app-service;1"]
                                 .getService(Ci.nsPIExternalAppLauncher);
    registerFileForDelete.deleteTemporaryFileOnExit(tempFile);
    
    // Download the file
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var webDownloader = Cc['@mozilla.org/embedding/browser/nsWebBrowserPersist;1']
                          .createInstance(Ci.nsIWebBrowserPersist);
    webDownloader.persistFlags = Ci.nsIWebBrowserPersist.PERSIST_FLAGS_NONE;
    webDownloader.progressListener = webProgressListener;
    webDownloader.saveURI(ioService.newURI(aWebURL, null, null), // URL
                          null,
                          null,
                          null, 
                          null,
                          tempFile);  // File to save to
  },

  /**
   * Drag and Drop helper functions
   */
  getFlavours: function(flavours) {
    flavours.appendFlavour("application/x-moz-file", "nsIFile");
    flavours.appendFlavour("text/x-moz-url");
    flavours.appendFlavour("text/uri-list");
    flavours.appendFlavour("text/unicode");
    flavours.appendFlavour("text/plain");
    return flavours;
  },
  
  /**
   * \brief Called from the onDrop function this will retrieve the image and
   *        save it to the Local Profile folder under artwork.
   * \param aCallback Function to call with the string URL of the newly saved
   *        image.
   * \param aDropData Is the DnD data that has been dropped on us.
   */
  handleDrop: function(aCallback, aDropData) {
    switch(aDropData.flavour.contentType) {
      case "text/x-moz-url":
      case "text/uri-list":
      case "text/unicode":
      case "text/plain":
        // Get the url string
        var url = "";
        if (aDropData.data instanceof Ci.nsISupportsString) {
            url = aDropData.data.toString();
        } else {
          url = aDropData.data;
        }

        // Only take the first url if there are more than one.
        url = url.split("\n")[0];
        
        // Now convert it into an URI so we can determine what to do with it
        var ioService = Cc["@mozilla.org/network/io-service;1"]
                          .getService(Ci.nsIIOService);
        var uri = null;
        try {
          uri = ioService.newURI(url, null, null);
        } catch (err) {
          Cu.reportError("sbCoverHelper: Unable to convert to URI: [" + url +
                         "] " + err);
          return;
        }
        
        switch (uri.scheme) {
          case 'file':
            if (uri instanceof Ci.nsIFileURL) {
              aCallback(this.saveFileToArtworkFolder(uri));
            }
          break; 

          case 'http':
          case 'https':
          case 'ftp':
            this.downloadFile(url, aCallback);
          break;
          
          default:
            Cu.reportError("sbCoverHelper: Unable to handle: " + uri.scheme);
          break;
        }
      break;
      
      case "application/x-moz-file":
        // Files are super easy :) just save it to our artwork folder
        if (aDropData.data instanceof Ci.nsILocalFile) {
          var fileName = this.saveFileToArtworkFolder(aDropData.data);
          aCallback(fileName);
        } else {
          Cu.reportError("sbCoverHelper: Not a local file.");
        }
      break;
    }
  },
  
  /**
   * \brief Called from the onDragStart function this will setup all the
   *        flavours needed for Drag and Drop the image to another place.
   * \param aTransferData Is the DnD transfer data passed into the onDragStart
   *        function call.
   * \param aImageURL Is any image url, file, http, data etc.
   */
  setupDragTransferData: function(aTransferData, aImageURL) {
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var imageURI = null;

    // First try to convert the URL to an URI
    try {
      imageURI = ioService.newURI(aImageURL, null, null);
    } catch (err) {
      Cu.reportError("sbCoverHelper: Unable to convert to URI: [" + aImageURL +
                     "] " + err);
    }
    
    // If we have a local file then put it as a proper image mime type
    // and as a x-moz-file
    if (imageURI instanceof Ci.nsIFileURL) {
      try {
        // Read the mime type for the flavour
        var mimetype = Cc["@mozilla.org/mime;1"]
                         .getService(Ci.nsIMIMEService)
                         .getTypeFromFile(imageURI);

        // Create an input stream for mime type flavour if we can
        var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                            .createInstance(Ci.nsIFileInputStream);
        inputStream.init(imageURI, FLAGS_DEFAULT, FLAGS_DEFAULT, 0);
        
        aTransferData.data.addDataForFlavour(mimetype,
                                             inputStream,
                                             0,
                                             Ci.nsIFileInputStream);
      } catch (err) {
        Cu.reportError("sbCoverHelper: Unable to add image from file: [" +
                       aImageURL + "] " + err);
      }

      // Add a file flavour
      aTransferData.data.addDataForFlavour("application/x-moz-file",
                                           imageURI,
                                           0,
                                           Ci.nsILocalFile);
    }

    // Add a url flavour
    aTransferData.data.addDataForFlavour("text/x-moz-url", aImageURL);

    // Add a uri-list flavour
    aTransferData.data.addDataForFlavour("text/uri-list", aImageURL);

    // Add simple Unicode flavour
    aTransferData.data.addDataForFlavour("text/unicode", aImageURL);

    // Finally a plain text flavour
    aTransferData.data.addDataForFlavour("text/plain", aImageURL);  
  }
}
