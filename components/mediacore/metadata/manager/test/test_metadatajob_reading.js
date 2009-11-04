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
 * \brief Test reading from various media formats
 */

var gFileLocation = "testharness/metadatamanager/files/";

var PORT_NUMBER = getTestServerPortNumber();
var gRemoteURLPrefix = "http://localhost:" + PORT_NUMBER + "/";

var gDefaultMetadata = {};
gDefaultMetadata[SBProperties.artistName] = "Songbird";
gDefaultMetadata[SBProperties.albumName]  = "Unit Test Classics";
gDefaultMetadata[SBProperties.trackName]  = "Sample";

// Map of media files to expected metadata
var gFiles = {};
gFiles["FLAC.flac"] = gDefaultMetadata;
gFiles["MP3_ID3v1.mp3"] = gDefaultMetadata;
gFiles["MP3_ID3v1_and_ID3v2.mp3"] = gDefaultMetadata;
gFiles["MP3_ID3v1_ID3v2_APE2.mp3"] = gDefaultMetadata;
gFiles["MP3_ID3v1v22.mp3"] = gDefaultMetadata;
gFiles["MP3_ID3v1v23.mp3"] = gDefaultMetadata;
gFiles["MP3_ID3v1v24.mp3"] = gDefaultMetadata;
gFiles["MP3_ID3v22.mp3"] = gDefaultMetadata;
gFiles["MP3_ID3v23.mp3"] = gDefaultMetadata;
gFiles["MP3_ID3v24.mp3"] = gDefaultMetadata;
gFiles["MP3_NoTags.mp3"] = {};
gFiles["MPEG4_Audio_Apple_Lossless.m4a"] = gDefaultMetadata;
gFiles["MusePack.mpc"] = gDefaultMetadata;
gFiles["Ogg_Vorbis.ogg"] = gDefaultMetadata;

gFiles["TrueAudio.tta"] = gDefaultMetadata;
gFiles["WavPack.wv"] = gDefaultMetadata;
gFiles["\u2606\u2606\u2606\u2606\u2606\u2606.mp3"] = gDefaultMetadata;

gFiles["MP3_ID3v1_Shift_JIS.mp3"] = {};
gFiles["MP3_ID3v1_Shift_JIS.mp3"][SBProperties.artistName] = "\u7406\u591A";
gFiles["MP3_ID3v1_Shift_JIS.mp3"][SBProperties.albumName]  = "Monologue -\u3082\u306E\u308D\u3049\u3050-";

// TODO Not working at the moment. Filed as bug 8768
// gFiles["Speex.spx"] = gDefaultMetadata;
// gFiles["Ogg_FLAC.oga"] = gDefaultMetadata;

var gLocalMediaItems = [];
var gRemoteMediaItems = [];
var gFileList = [];

var gFileMetadataService = null;

var gServer;

function runTest () {
   
  var gTestLibrary = createNewLibrary( "test_metadatajob" );
  var gTestMediaItems = Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                                  .createInstance(Components.interfaces.nsIMutableArray);

  // Make a copy of everything in the test file folder
  // so that our changes don't interfere with other tests
  var testFolder = getCopyOfFolder(newAppRelativeFile(gFileLocation), "_temp_reading_files");

  sleep(2000);
  
  // Make a file with a unicode filename.  This would be checked in, except
  // the windows build system can't handle unicode filenames.
  var unicodeFile = testFolder.clone();
  unicodeFile.append("MP3_ID3v23.mp3");
  unicodeFile = getCopyOfFile(unicodeFile, "\u2606\u2606\u2606\u2606\u2606\u2606.mp3", testFolder);


  gServer = Cc["@mozilla.org/server/jshttp;1"]
              .createInstance(Ci.nsIHttpServer);

  gServer.start(PORT_NUMBER);

  gServer.registerDirectory("/", testFolder.clone());

  for (var fileName in gFiles) {
    log("MetadataJob_Reading: enqueueing file " + fileName);
  
    gFileList.push(fileName);
  
    // Add gFiles to it
    var localPath = testFolder.clone();
    localPath.append(fileName);
    assertNotEqual( localPath, null );
    var localPathURI = newFileURI( localPath );
    assertNotEqual( localPathURI, null );
    // Allow duplicates, since many of the files have the same hash value
    var localPathMI = gTestLibrary.createMediaItem( localPathURI, null, true );
    assertNotEqual( localPathMI, null );
    gLocalMediaItems.push( localPathMI );
    gTestMediaItems.appendElement( localPathMI, false );

    // Add gRemoteUrls to it
    var remotePathURI = newURI( gRemoteURLPrefix + fileName );
    assertNotEqual( remotePathURI, null );
    var remotePathMI = gTestLibrary.createMediaItem( remotePathURI );
    assertNotEqual( remotePathMI, null );
    gRemoteMediaItems.push( remotePathMI );
    gTestMediaItems.appendElement( remotePathMI, false );
  }
  
  // Request metadata for both local and remote urls at the same time.  Woo!
  gFileMetadataService = Components.classes["@songbirdnest.com/Songbird/FileMetadataService;1"]
                                .getService(Components.interfaces.sbIFileMetadataService);
  var job = gFileMetadataService.read( gTestMediaItems);
  
  if (job.status == Components.interfaces.sbIJobProgress.STATUS_SUCCEEDED) {
    jobFinished();
  } else {
    // Set an observer to know when we complete
    job.addJobProgressListener(onComplete);
    testPending();
  }
}

function onComplete(job) {
  if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
    return;
  }

  job.removeJobProgressListener(onComplete);

  jobFinished();
}

function jobFinished() {
  try { 
    assertTrue(gFileList.length > 0);
    
    // Print metadata or all items so we can see the full set of data instead
    // of just quitting on the first error
    for (var i = 0; i < gFileList.length; i++) {
      var fileName = gFileList[i];
      var expectedProperties = gFiles[fileName];
      var localProperties = 
          SBProperties.arrayToJSObject(gLocalMediaItems[i].getProperties());
      var remoteProperties = 
          SBProperties.arrayToJSObject(gRemoteMediaItems[i].getProperties()); 
      
      log("\n\n--------------------------------------------------------------");
      log("MetadataJob_Reading: results for " + fileName);
      log("Expected properties: " + expectedProperties.toSource());
      log("\nLocal properties: " + localProperties.toSource());
      log("\nRemote properties: " + remoteProperties.toSource());
    }
    
    // Now actually verify the metadata
    for (var i = 0; i < gFileList.length; i++) {
      var fileName = gFileList[i];
      var expectedProperties = gFiles[fileName];
      
      log("MetadataJob_Reading: comparing local properties for " + fileName);
      
      // Verify local properties
      assertObjectIsSubsetOf(expectedProperties, 
          SBProperties.arrayToJSObject(gLocalMediaItems[i].getProperties()));

      log("MetadataJob_Reading: comparing remote properties for " + fileName);
            
      // Verify remote properties
      assertObjectIsSubsetOf(expectedProperties, 
          SBProperties.arrayToJSObject(gRemoteMediaItems[i].getProperties()));      
    }
    

    // So testing is complete
    gFileMetadataService = null;
    
  } finally {
   gServer.stop(function() {});
  }
  
  testFinished(); 
}
