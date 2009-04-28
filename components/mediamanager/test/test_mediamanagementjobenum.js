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
 * \brief Test treating a Media File Manager Job as an enumerator (dry-run)
 */

// This is the root of our test folder (temporary items)
var testFolder = getCopyOfFolder(newAppRelativeFile(gFileLocation),
                                 "_temp_mediafilemanager_files");

/**
 * Call back for after the metadata read job completes
 */
function onComplete(job) {
  try {
    if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
      return;
    }
    job.removeJobProgressListener(onComplete);
    
    // Manage the files
    var mediaManagerJob = Cc[SB_MEDIAMANAGERJOB]
                            .createInstance(Ci.sbIMediaManagementJob);
    mediaManagerJob.init(gTestLibrary);
    mediaManagerJob.QueryInterface(Ci.nsISimpleEnumerator);
    var index = 0;
    while (mediaManagerJob.hasMoreElements()) {
      jobItem = mediaManagerJob.getNext()
                               .QueryInterface(Ci.sbIMediaManagementJobItem);
      expectedData = gResultInformation[index];

      // check that the media item sources look right
      var expected = testFolder.clone();
      expected = appendPathToDirectory(expected, expectedData.originalFileName);
      var file = jobItem.item.contentSrc.QueryInterface(Ci.nsIFileURL).file;
      assertEqual(expected.path, file.path);

      // check that the destination file is correct
      var expected = testFolder.clone();
      var expFolder = gResultInformation[index].expectedFolder;
      expected = appendPathToDirectory(expected, expFolder);
      expected.append(gResultInformation[index].expectedFileName);

      assertEqual(jobItem.targetPath.path, expected.path);

      // check that the action to take was right
      assertEqual(jobItem.action, expectedData.expectedAction);
      
      ++index;
    }
    testFinished();
  } catch (err) {
    doFail("ERROR: " + err);
  }
}

function runTest () {
  // Setup the preferences for the media manager
  setupMediaManagerPreferences();

  // Move last test item into the managed folder so we can test a file that is
  // already there. (It should be moved/renamed but not copied)
  var changeIndex = (gResultInformation.length - 1);
  var testItem = testFolder.clone();
  testItem.append(gResultInformation[changeIndex].originalFileName);
  var managedFolder = testFolder.clone();
  managedFolder.append("Managed");
  // Managed folder should have already been created in the
  // setupMediaManagerPreferences function.
  testItem.moveTo(managedFolder, "");
  // Update the array information
  gResultInformation[changeIndex].originalFileName = "Managed/" +
    gResultInformation[changeIndex].originalFileName;

  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);

  // Create the test library, do not init with items
  gTestLibrary = createLibrary("test_mediamanagementjobenum", null, false);
  assertTrue(gTestLibrary);
  libraryManager.registerLibrary(gTestLibrary, false);
  assertEqual(gTestLibrary.length, 0);

  var listListener = new TestMediaListListener();
  gTestLibrary.addListener(listListener, false);

  // Add test items to the library
  addItemsToLibrary(gTestLibrary);
  gTestMediaItems = listListener.added;
  assertEqual(gTestLibrary.length, gResultInformation.length);
  assertEqual(gTestMediaItems.length, gResultInformation.length);

  gTestLibrary.removeListener(listListener);

  // Read the metadata for these items
  var fileMetadataService = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);
  var job = fileMetadataService.read(gTestMediaItems);

  // Set an observer to know when we complete
  job.addJobProgressListener(onComplete);
  testPending();
}
