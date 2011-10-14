/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \brief Test COPY/MOVE/RENAME/DELETE operations of the Media File Manager
 */

// This is the root of our test folder (temporary items)
var testFolder = getCopyOfFolder(newAppRelativeFile(gFileLocation),
                                 "_temp_mediafilemanager_files");

/**
 * Job progress listener for the organizeList
 */
function onManagementComplete(job) {
  try { 
    if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
      return;
    }
    job.removeJobProgressListener(onManagementComplete);

    for (var i = 0; i < gTestMediaItems.length; i++) {
      var item = gTestMediaItems.queryElementAt(i, Ci.sbIMediaItem);
      /**
       * Check each item to see if it was copied to the managed folder and
       * organized properly. The last file in our list started in the managed
       * folder so it should be checked that it was moved not copied.
       * parameter 3 is a boolean True is check if original file is still in
       * place, and false is check that the original file has been removed.
       */
      assertTrue(checkItem(item,
                           i,
                           (i != (gTestMediaItems.length - 1))),
                 "Item " + i + " was not relocated properly");
    }
    
    // Check if everything worked
    testFinished();
  } catch (err) {
    doFail("ERROR: " + err);
  }
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
    
    // Manage the files
    var mediaManagerJob = Cc[SB_MEDIAMANAGERJOB]
                            .createInstance(Ci.sbIMediaManagementJob);
    mediaManagerJob.init(gTestLibrary);
    mediaManagerJob.addJobProgressListener(onManagementComplete);
    mediaManagerJob.organizeMediaList();
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

  var libraryManager = Cc["@getnightingale.com/Nightingale/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);

  // Create the test library, do not init with items
  gTestLibrary = createLibrary("test_mediamanagementjob", null, false);
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
  var fileMetadataService = Cc["@getnightingale.com/Nightingale/FileMetadataService;1"]
                              .getService(Ci.sbIFileMetadataService);
  var job = fileMetadataService.read(gTestMediaItems);

  // Set an observer to know when we complete
  job.addJobProgressListener(onComplete);
  testPending();
}
