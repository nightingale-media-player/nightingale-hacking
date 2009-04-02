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
 * \brief Test COPY/MOVE/RENAME/DELETE operations of the Media File Manager
 */

// We use this in many places for creating our file paths
var gFileLocation = "testharness/mediamanager/files/";
// This is the root of our test folder (temporary items)
var testFolder = getCopyOfFolder(newAppRelativeFile(gFileLocation),
                                 "_temp_mediafilemanager_files");
// Items we add to the library to test our file management
var gTestMediaItems;

// An array of what our test results should be
var gResultInformation = [
  { originalFileName: "TestFile1.mp3",
    expectedFileName: "1 - Sample.mp3",
    expectedFolder:   "Managed/Songbird/Unit Test Classics" },
  { originalFileName: "TestFile2.mp3",
    expectedFileName: "2 - Sample.mp3",
    expectedFolder:   "Managed/Songbird/Unit Test Classics" },
  { originalFileName: "TestFile3.mp3",
    expectedFileName: "3 - Sample.mp3",
    expectedFolder:   "Managed/Songbird/Unit Test Classics" },

  { originalFileName: "TestFile4.mp3",
    expectedFileName: "1 - TestFile4.mp3.mp3",
    expectedFolder:   "Managed/Unknown Artist/Unknown Album" },

  { originalFileName: "TestFile5.mp3",
    expectedFileName: "1 - Sample.mp3",
    expectedFolder:   "Managed/Songbird/Unknown Album" },

  { originalFileName: "TestFile6.mp3",
    expectedFileName: "1 - Sample.mp3",
    expectedFolder:   "Managed/Unknown Artist/Unit Test Classics" }
];

/**
 * Check the items path with the expected results, return true if ok.
 */
function checkItem(aMediaItem, aResultInformationIndex) {
  // First get the current path from the item
  var current = aMediaItem.contentSrc.QueryInterface(Ci.nsIFileURL);
  if (!(current instanceof Ci.nsIFileURL)) {
    return false;
  }
  current = current.file;
  var currentPath = decodeURIComponent(current.path);
  
  // Now put together the expected path for the item
  var expected = testFolder.clone();
  expected = appendPathToDirectory(expected,
                                   gResultInformation[aResultInformationIndex].expectedFolder);
  expected.append(gResultInformation[aResultInformationIndex].expectedFileName);
  var expectedPath = expected.path;
  
  // Compare the current to expected
  return (current.equals(expected));
}

/**
 * Check that a file for an item has been deleted.
 */
function checkDeletedItem(aMediaItem) {
  var fileURI = aMediaItem.contentSrc.QueryInterface(Ci.nsIFileURL);
  if (fileURI instanceof Ci.nsIFileURL) {
    var file = fileURI.file;
    return !file.exists();
  }
  return false;
} 

/**
 * Adds some test files to the test library, we need to use items in a library
 * since the MediaFileManager uses the property information to organize.
 */
function addItemsToLibrary(aLibrary) {
  var toAdd = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                .createInstance(Ci.nsIMutableArray);
  var propertyArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                        .createInstance(Ci.nsIMutableArray);
  var ioService = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);
  for (var i = 0; i < gResultInformation.length; i++) {
    // Set up the item
    var newFile = testFolder.clone();
    newFile.append(gResultInformation[i].originalFileName);
    toAdd.appendElement(ioService.newFileURI(newFile), false);
    
    // Setup default properties for this item
    var props = Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
                  .createInstance(Ci.sbIMutablePropertyArray);
    props.appendProperty(SB_NS + "contentLength", i + 1);
    props.appendProperty(SB_NS + "trackNumber", i + 1);
    propertyArray.appendElement(props, false);
  }

  aLibrary.batchCreateMediaItems(toAdd, propertyArray);
}

/**
 * Set up what we want the preferences to be, we need to do this since the
 * MediaFileManager depends on the preferences to organize.
 */
function setupMediaManagerPreferences () {
  var prefBranch = Cc["@mozilla.org/preferences-service;1"]
                     .getService(Ci.nsIPrefBranch2);

  var separator = "/";
  var OS = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
  if (OS == "WINNT") {
    separator = "\\";
  }
  var managedFolder = testFolder.clone();
  managedFolder.append("Managed");
  // Create the folder
  managedFolder.create(Ci.nsIFile.DIRECTORY_TYPE, 0777);
  
  prefBranch.setComplexValue(SB_MM_PREF_FOLDER, Ci.nsILocalFile, managedFolder);
  prefBranch.setBoolPref(SB_MM_PREF_ENABLED, true);
  prefBranch.setBoolPref(SB_MM_PREF_COPY, true);
  prefBranch.setBoolPref(SB_MM_PREF_MOVE, true);
  prefBranch.setBoolPref(SB_MM_PREF_RENAME, true);
  prefBranch.setCharPref(SB_MM_PREF_FMTDIR,
                         SB_NS + "artistName," +
                         separator + "," +
                         SB_NS + "albumName");
  prefBranch.setCharPref(SB_MM_PREF_FMTFILE,
                         SB_NS + "trackNumber," +
                         " - ," +
                         SB_NS + "trackName");
}

/**
 * Call back for after the metadata read job completes
 */
function onComplete(job) {
  try { 
    if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
      return;
    }
    job.removeJobProgressListener(onComplete);
    
    // Setup the preferences for the media manager
    setupMediaManagerPreferences();
    
    // Manage the files
    var fileManager = Cc[SB_MEDIAFILEMANAGER]
                        .createInstance(Ci.sbIMediaFileManager);
    // COPY/MOVE/RENAME
    for (var i = 0; i < gTestMediaItems.length; i++) {
      var isManaged = false;
      var item = gTestMediaItems.queryElementAt(i, Ci.sbIMediaItem);
      try {
        isManaged = fileManager.organizeItem(item,
                                             Ci.sbIMediaFileManager.MANAGE_RENAME |
                                               Ci.sbIMediaFileManager.MANAGE_COPY |
                                               Ci.sbIMediaFileManager.MANAGE_MOVE);
      } catch (err) {
        doFail("ERROR: Unable to manage item: " + err);
      }
      assertTrue(isManaged, "Unable to manage item " + i);
      assertTrue(checkItem(item, i), "Item " + i + " was not relocated properly");
    }
    
    // DELETE
    for (var i = 0; i < gTestMediaItems.length; i++) {
      var isManaged = false;
      var item = gTestMediaItems.queryElementAt(i, Ci.sbIMediaItem);
      try {
        isManaged = fileManager.organizeItem(item,
                                             Ci.sbIMediaFileManager.MANAGE_DELETE);
      } catch (err) {
        doFail("Unable to delete item: " + err);
      }
      assertTrue(isManaged, "Unable to delete item " + i);
      assertTrue(checkDeletedItem(item), "Item " + i + " was not deleted");
    }
  } catch (err) {
    doFail("ERROR: " + err);
  }

  testFinished();
}

function runTest () {
  // Save our preferences
  saveMediaManagerPreferences();
  
  // Create the test library
  var testLibrary = createLibrary("test_mediafilemanager");

  var listListener = new TestMediaListListener();
  testLibrary.addListener(listListener, false);

  // Add test items to the library
  addItemsToLibrary(testLibrary);
  gTestMediaItems = listListener.added;
  assertTrue(gTestMediaItems.length, 6);
  
  testLibrary.removeListener(listListener);

  // Read the metadata for these items
  var fileMetadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);
  var job = fileMetadataService.read( gTestMediaItems);

  // Set an observer to know when we complete
  job.addJobProgressListener(onComplete);
  testPending();
}
