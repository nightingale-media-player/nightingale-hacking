/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
 * \brief Test creation and execution of metadata writing jobs
 */

var gTestFileLocation = "testharness/metadatamanager/files/";
var gTestLibrary = createNewLibrary( "test_metadata_mpeg4" );
var gTestMediaItems = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                        .createInstance(Ci.nsIMutableArray);

var gTestFolder;

var gFileMetadataService = Cc["@songbirdnest.com/Songbird/MetadataManager;1"]
                             .getService(Ci.sbIMetadataManager);


Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

/************************************************************************/
/* \brief read the given file and return its contents as an array       */
/************************************************************************/
function getFileData(aFileName) {
  var file = gTestFolder.clone();
  file.append(aFileName);
  
  var inputStream = Cc["@mozilla.org/network/file-input-stream;1"]
                      .createInstance(Ci.nsIFileInputStream);
  gFilesToClose.push(inputStream);
  inputStream.init(file, 0x01, 0600, 0);
  var stream = Cc["@mozilla.org/binaryinputstream;1"]
                 .createInstance(Ci.nsIBinaryInputStream);
  stream.setInputStream(inputStream);
  var size = inputStream.available();
  // use a binary input stream to grab the bytes.
  return stream.readByteArray(size);
}

/************************************************************************/
/* The list of file transformations we'll take to make the files        */
/************************************************************************/
const gTestCases = [
  {
    source: "MPEG4_Empty.m4a",
    ref:   "MPEG4_Empty_Ref.m4a",
    properties: {
      artistName: "Songbird",
      trackName: "Start From Empty"
      
    }
  },
  {
    source: "MPEG4_Empty_Shift.m4a",
    ref:   "MPEG4_Empty_Shift_Ref.m4a",
    properties: {
      artistName: "Songbird",
      trackName: "memmov"
      
    }
  },
  {
    source: "MPEG4_Useless.m4a",
    ref:   "MPEG4_Useless_Ref.m4a",
    properties: {
      artistName: "Songbird",
      trackName: "Simple Test"
    }
  },
  {
    source: "MPEG4_Expand.m4a",
    ref:   "MPEG4_Expand_Ref.m4a",
    properties: {
      artistName: "Songbird",
      albumName: "MPEG4 writing unit test",
      trackName: "expanding and shifting chunk offsets!",
      genre: "Soundtrack",
      year: 2009,
      trackNumber: 42,
      totalTracks: 54,
      comment: "Hello, world!",
      composerName: "Balut",
      discNumber: 13,
      totalDiscs: 27,
      sampleRate: 32768, /* this doesn't actually get set in the file */
      bitRate: 65536, /* this doesn't actually get set in the file */
      channels: 1024 /* this doesn't actually get set in the file */
    }
  },
];

/**
 * Copy some media files into a temp directory, then confirm that metadata
 * properties can be modified using a write job.
 */
function runTest() {
  /* SETUP */

  var prefSvc = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefBranch);
  var oldWritingEnabledPref = prefSvc.getBoolPref("songbird.metadata.enableWriting");
  gTailCallback.push(function() {
    prefSvc.setBoolPref("songbird.metadata.enableWriting", oldWritingEnabledPref); 
  });
  prefSvc.setBoolPref("songbird.metadata.enableWriting", true);
  
  // Make a copy of everything in the test file folder
  // so that our changes don't interfere with other tests
  gTestFolder = getCopyOfFolder(newAppRelativeFile(gTestFileLocation), 
                                "_temp_metadata_mpeg4_files");
  
  ///////////////////////////
  // Import the test items //
  ///////////////////////////
  var library = createNewLibrary( "test_metadatajob_writing_library" );
  assertTrue(library instanceof Ci.sbILibrary, "creating an invalid library");
  var sourceURLs = [];
  for each (let testData in gTestCases) {
    let itemPath = gTestFolder.clone();
    itemPath.append(testData.source);
    sourceURLs.push(newFileURI(itemPath));
  }
  var items = importFilesToLibrary(sourceURLs, library);
  assertEqual(items.length, sourceURLs.length);
  for (let i = 0; i < items.length; ++ i) {
    assertEqual(gTestCases[i].source, 
                items[i].contentSrc.QueryInterface(Ci.nsIURL).fileName);
    gTestCases[i].item = items[i];
  }
  
  var job = startMetadataJob(items, "read");
  // Wait for reading to complete before continuing
  job.addJobProgressListener(onReadComplete);
  testPending();
}

function onReadComplete(job) {  
  reportJobProgress(job, "metadatajob - mpeg4 writing - onReadComplete");
  
  if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
    // the job isn't finished yet!
    return;
  }
  job.removeJobProgressListener(onReadComplete);
  
  var propertiesToWrite = [];
  var items = [];
  for each (let testData in gTestCases) {
    items.push(testData.item);
    for (let prop in testData.properties) {
      log(prop + ": " + SBProperties[prop] + " -> " + testData.properties[prop]);
      testData.item.setProperty(SBProperties[prop], testData.properties[prop]);
      // Add the property name to the write list if we have not already done so
      if (propertiesToWrite.indexOf(SBProperties[prop]) < 0) {
        propertiesToWrite.push(SBProperties[prop]);
      }
    }
  }
  job = startMetadataJob(items, "write", propertiesToWrite);
  // Wait for writing to complete before continuing
  job.addJobProgressListener(onWriteComplete);
}

function onWriteComplete(job) {
  reportJobProgress(job, "metadatajob - mpeg4 writing - onWriteComplete");

  if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
    return;
  }
  job.removeJobProgressListener(onWriteComplete);

  for each (let testData in gTestCases) {
    log("Comparing " + testData.source + " against " + testData.ref);
    let changedData = getFileData(testData.source);
    let referenceData = getFileData(testData.ref);
    assertArraysEqual(changedData, referenceData);
  }

  // Verify job progress reporting.  
  // NOTE: Comment these out to receive more useful debug information
  assertEqual(gTestCases.length, job.total);
  assertEqual(gTestCases.length, job.progress);
  assertEqual(0, job.errorCount, "found errors in job");
  assertEqual(job.status, Components.interfaces.sbIJobProgress.STATUS_SUCCEEDED);
  testFinished();
}

/**
 * Add files to a library, returning media items
 */
function importFilesToLibrary(files, library) {
  if (!(files instanceof Ci.nsIArray)) {
    files = ArrayConverter.nsIArray(files);
  }
  var items = library.batchCreateMediaItems(files, null, true);
  assertEqual(items.length, files.length);
  var jsItems = [];
  for (var i = 0; i < items.length; i++) {
    jsItems.push(items.queryElementAt(i, Ci.sbIMediaItem));
  }
  return jsItems;
}


/**
 * Get a metadata job for the given items
 */
function startMetadataJob(items, type, writeProperties) {
  if (!(items instanceof Ci.nsIArray)) {
    items = ArrayConverter.nsIArray(items);
  }
  manager = Components.classes["@songbirdnest.com/Songbird/FileMetadataService;1"]
                      .getService(Components.interfaces.sbIFileMetadataService);
                      
  var job;
  if (type == "write") {
    job = manager.write(items, ArrayConverter.stringEnumerator(writeProperties));
  } else {
    job = manager.read(items);
  }
  return job;
}

