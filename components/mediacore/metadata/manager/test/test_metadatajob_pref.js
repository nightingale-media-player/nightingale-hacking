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
 * \brief Test creation and execution of metadata writing jobs
 */
 
var gTestFileLocation = "testharness/metadatamanager/files/";

/**
 * Ensure metadata.enableWriting pref can prevent writing.
 */
function runTest() {
  /* SETUP */
  
  // Make a copy of everything in the test file folder
  // so that any accidental changes don't interfere with other tests
  var testFolder = getCopyOfFolder(newAppRelativeFile(gTestFileLocation), "_temp_writing_files");
  
  // Now find all the media files in our testing directory
  var urls = getMediaFilesInFolder(testFolder);

  // Make sure we have files to test
  assertEqual(urls.length > 0, true);
  
  ///////////////////////////
  // Import the test items //
  ///////////////////////////
  var library = createNewLibrary( "test_metadatajob_writing_library" );
  var items = importFilesToLibrary(urls, library);
  assertEqual(items.length, urls.length);
  
  var prefSvc = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefBranch);
  var oldWritingEnabledPref = prefSvc.getBoolPref("songbird.metadata.enableWriting");
  
  // While we're at it, confirm that metadata can only be written when allowed via prefs
  prefSvc.setBoolPref("songbird.metadata.enableWriting", false);
  try {
    startMetadataWriteJob(items);
    // This line should not be reached, as startMetadataWriteJob should throw NS_ERROR_NOT_AVAILABLE
    throw new Error("MetadataJobManager does not respect enableWriting pref!");
  } catch (e) {
    if (Components.lastResult != Components.results.NS_ERROR_NOT_AVAILABLE) {
      throw new Error("MetadataJobManager does not respect enableWriting pref!");
    }
  }
  
  prefSvc.setBoolPref("songbird.metadata.enableWriting", oldWritingEnabledPref); 
  // We're done, so kill all the temp files
  testFolder.remove(true);
  job = null;
  gTest = null;
}


/**
 * Get an array of all media files below the given folder
 */
function getMediaFilesInFolder(folder) {
  var scan = Cc["@songbirdnest.com/Songbird/FileScan;1"]
               .createInstance(Ci.sbIFileScan);
  var query = Cc["@songbirdnest.com/Songbird/FileScanQuery;1"]
               .createInstance(Ci.sbIFileScanQuery);
  query.setDirectory(folder.path);
  query.setRecurse(true);
  
  for each (var extension in gSupportedFileExtensions) {
    query.addFileExtension(extension);
  }

  scan.submitQuery(query);

  log("Scanning...");

  while (query.isScanning()) {
    sleep(1000);
  }
  
  assertEqual(query.getFileCount() > 0, true);
  var urls = query.getResultRangeAsURIStrings(0, query.getFileCount() - 1);

  fileScan.finalize();
  
  return urls;
}


/**
 * Add files to a library, returning media items
 */
function importFilesToLibrary(files, library) {
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
function startMetadataWriteJob(items) {
  var array = Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                        .createInstance(Components.interfaces.nsIMutableArray);
  for each (var item in items) {
    array.appendElement(item, false);
  }                     
  manager = Components.classes["@songbirdnest.com/Songbird/FileMetadataService;1"]
                      .getService(Components.interfaces.sbIFileMetadataService);
  return manager.write(array);
}

