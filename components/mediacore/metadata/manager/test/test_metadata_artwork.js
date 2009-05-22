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
 * \brief Test reading and writing artwork from various media formats
 */

var gFileLocation = "testharness/metadatamanager/files/";
var gTestLibrary = createNewLibrary( "test_metadatajob" );
var gTestMediaItems = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                        .createInstance(Ci.nsIMutableArray);

// Make a copy of everything in the test file folder
// so that our changes don't interfere with other tests
var testFolder = getCopyOfFolder(newAppRelativeFile(gFileLocation), "_temp_artwork_files");

var gFileMetadataService = Cc["@songbirdnest.com/Songbird/MetadataManager;1"]
                        .getService(Ci.sbIMetadataManager);


function compareArray(aFirstArray, aSecondArray) {
  if (aFirstArray.length != aSecondArray.length) {
    return false;
  }
  
  for (var aIndex = 0; aIndex < aFirstArray.length; aIndex++) {
    if (aFirstArray[aIndex] != aSecondArray[aIndex]) {
      return false;
    }
  }
  
  return true;
}

function getImageData(imageFileName) {
  var imageFile = testFolder.clone();
  imageFile.append(imageFileName);
  
  var newMimeType = Cc["@mozilla.org/mime;1"]
                   .getService(Ci.nsIMIMEService)
                   .getTypeFromFile(imageFile);
  var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(Ci.nsIFileInputStream);
  gFilesToClose.push(inputStream);
  inputStream.init(imageFile, 0x01, 0600, 0);
  var stream = Cc["@mozilla.org/binaryinputstream;1"]
                 .createInstance(Ci.nsIBinaryInputStream);
  stream.setInputStream(inputStream);
  var size = inputStream.available();
  // use a binary input stream to grab the bytes.
  var bis = Cc["@mozilla.org/binaryinputstream;1"].
            createInstance(Ci.nsIBinaryInputStream);
  bis.setInputStream(inputStream);
  var newImageData= bis.readByteArray(size);

  return [newMimeType, newImageData];
}

function testWrite(testFileName, shouldPass) {
  // Grab the test file
  var writeFile = testFolder.clone();
  writeFile.append(testFileName);

  // Load the image data to save to the test file
  var imageFileName = "test.png"
  var imageMimeType;
  var imageData;
  [imageMimeType, imageData] = getImageData(imageFileName);
  assertNotEqual(imageData.length, 0);

  var imageFile = testFolder.clone();
  imageFile.append(imageFileName);

  // Save the image data to the test file
  var localPathURI = newFileURI( writeFile );
  var handler = gFileMetadataService.getHandlerForMediaURL(localPathURI.spec);

  try {
    var newImageFile = newFileURI( imageFile );
    handler.setImageData(Ci.sbIMetadataHandler.METADATA_IMAGE_TYPE_OTHER,
                         newImageFile.spec);
  } catch (err) {
    if (shouldPass) {
      assertEqual(true, false, "Test write:" + err);
    }
  }
  
  try {
    // now grab it from the same file and compare the results
    var testMimeType = {};
    var testDataSize = {};
    var testImageData = handler.getImageData(Ci.sbIMetadataHandler
                                          .METADATA_IMAGE_TYPE_OTHER,
                                          testMimeType,
                                          testDataSize);
    
    if (shouldPass) {
      assertEqual(testMimeType.value,
                  imageMimeType,
                  "Mimetype not equal");
      assertEqual(compareArray(imageData, testImageData),
                  true,
                  "Image data not equal");
    } else {
      assertNotEqual(testMimeType.value,
                     imageMimeType,
                     "Mimetype is equal when it should not be");
      assertNotEqual(compareArray(imageData, testImageData),
                     true,
                     "Image data is equal and should not be");
    }
  } catch(err) {
    if (shouldPass) {
      assertEqual(true, false, "Test read: " + err);
    }
  }
}

function testRead() {
  // Read some artwork
  var artFiles = [ "MP3_ID3v1v22.mp3",
                   "MP3_ID3v1v24.mp3",
                   "MP3_ID3v22.mp3",
                   "MP3_ID3v23.mp3",
                   "MP3_ID3v24.mp3",
                   "MPEG4_Audio_Apple_Lossless.m4a"];
  var noArtFiles = [ "MP3_NoTags.mp3",
                     "MPEG4_Audio_Apple_Lossless_NoArt.m4a",
                     "Ogg_Vorbis.ogg",
                     "TrueAudio.tta",
                     "FLAC.flac"];

  // Read files that should have album art
  for (var index in artFiles) {
    var readFile = testFolder.clone();
    readFile.append(artFiles[index]);
    var localPathURI = newFileURI( readFile );
    var handler = gFileMetadataService.getHandlerForMediaURL(localPathURI.spec);
    // The art for these are stored in OTHER, normally they should be stored in
    // FRONTCOVER
    try {
      var mimeTypeOutparam = {};
      var outSize = {};
      var imageData = handler.getImageData(Ci.sbIMetadataHandler
                                            .METADATA_IMAGE_TYPE_OTHER,
                                            mimeTypeOutparam,
                                            outSize);
      assertEqual(true, (outSize.value > 0));
      assertNotEqual(mimeTypeOutparam.value, null);
      assertNotEqual(imageData, "");
    } catch (err) {
      assertEqual(true, false);
    }

    // Try grabbing some other artwork that is not there
    // but don't do this for .m4a files since there isn't the concept of
    // album art "type"
    if (artFiles[index].match("\.m4a$"))
      continue;
    try {
      var mimeTypeOutparam = {};
      var outSize = {};
      var imageData = handler.getImageData(Ci.sbIMetadataHandler
                                            .METADATA_IMAGE_TYPE_CONDUCTOR,
                                            mimeTypeOutparam,
                                            outSize);
      assertEqual(outSize.value, 0);
      assertEqual(mimeTypeOutparam.value, "");
      assertEqual(imageData, "");
    } catch (err) { }

  }
  // Read files that should NOT have album art
  for (var index in noArtFiles) {
    var readFile = testFolder.clone();
    readFile.append(noArtFiles[index]);
    var localPathURI = newFileURI( readFile );
    var handler = gFileMetadataService.getHandlerForMediaURL(localPathURI.spec);

    try {
      var mimeTypeOutparam = {};
      var outSize = {};
      var imageData = handler.getImageData(Ci.sbIMetadataHandler
                                            .METADATA_IMAGE_TYPE_OTHER,
                                            mimeTypeOutparam,
                                            outSize);
      assertEqual(outSize.value, 0);
      assertEqual(mimeTypeOutparam.value, "");
      assertEqual(imageData, "");
    } catch (err) { }
  }
}

function runTest () {
  // First try to read some metadata
  testRead();

  // Now for writing an image to metadata
  testWrite("MP3_ID3v24.mp3", true);
  testWrite("Ogg_Vorbis.ogg", false);
}
