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
 * Call back for after the metadata read job completes
 */
function onComplete(job) {
  try { 
    if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
      return;
    }
    job.removeJobProgressListener(onComplete);
    
    // Manage the files
    var fileManager = Cc[SB_MEDIAFILEMANAGER]
                        .createInstance(Ci.sbIMediaFileManager);
    fileManager.init();
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
      assertTrue(checkItem(item, i, true),
                 "Item " + i + " was not relocated properly");
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

    testFinished();
  } catch (err) {
    doFail("ERROR: " + err);
  }

}

function runTest () {
  // Setup the preferences for the media manager
  setupMediaManagerPreferences();

  var libraryManager = Cc["@getnightingale.com/Nightingale/library/Manager;1"]
                         .getService(Ci.sbILibraryManager);

  // Create the test library, do not init with items
  gTestLibrary = createLibrary("test_mediafilemanager", null, false);
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
