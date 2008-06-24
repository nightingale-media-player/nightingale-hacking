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

function runTest () {
  var gTestLibrary = createNewLibrary( "test_metadatajob" );
  var gTestMediaItems = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                          .createInstance(Ci.nsIMutableArray);

  // Make a copy of everything in the test file folder
  // so that our changes don't interfere with other tests
  var testFolder = getCopyOfFolder(newAppRelativeFile(gFileLocation), "_temp_reading_files");
  
  // Read some artwork
  var artFiles = [ "MP3_ID3v1v22.mp3",
                   "MP3_ID3v1v24.mp3",
                   "MP3_ID3v22.mp3",
                   "MP3_ID3v23.mp3",
                   "MP3_ID3v24.mp3" ];
  var noArtFiles = [ "MP3_NoTags.mp3",
                     "MPEG4_Audio_Apple_Lossless.m4a",
                     "Ogg_Vorbis.ogg",
                     "TrueAudio.tta",
                     "FLAC.flac"];
  var metadataManager = Cc["@songbirdnest.com/Songbird/MetadataManager;1"]
                          .getService(Ci.sbIMetadataManager);

  // Read files that should have album art
  for (var index in artFiles) {
    var readFile = testFolder.clone();
    readFile.append(artFiles[index]);
    var localPathURI = newFileURI( readFile );
    var handler = metadataManager.getHandlerForMediaURL(localPathURI.spec);
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
    var handler = metadataManager.getHandlerForMediaURL(localPathURI.spec);

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
