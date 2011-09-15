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
 * \brief Test the directory importer service
 */

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

// This test relies on the sample files used in the metadata test cases
var gDirectories = ArrayConverter.nsIArray([newAppRelativeFile("testharness/metadatamanager/files/")]);

var gDirectoryImporter = Cc['@songbirdnest.com/Songbird/DirectoryImportService;1']
                            .getService(Ci.sbIDirectoryImportService);
var gLibrary;
var gMediaList;

const MEDIALIST_TARGET_INDEX = 1;

/**
 * Start by creating a library and doing a directory import of the
 * metadata unit test files.
 */
function runTest () {
  gLibrary = createLibrary("test_directoryimport", null, false);
  gLibrary.clear();

  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"].
                          getService(Ci.sbILibraryManager);
  libraryManager.registerLibrary(gLibrary, false);

  var job = gDirectoryImporter.import(gDirectories, gLibrary);
  job.addJobProgressListener(onFirstImportProgress);
  testPending();
}


/**
 * Once the first directory import completes, confirm that
 * we found some files, and that at least some of them
 * have expected metadata
 */
function onFirstImportProgress(job) {
  try {
    log("DirectoryImport: onFirstImportProgress: " + job.statusText +
        " status " + job.status + " added " + job.totalAddedToLibrary );

    // Just ignore progress while running
    if (job.status == Ci.sbIJobProgress.STATUS_RUNNING) {
      return;
    }
    // Finished
    job.removeJobProgressListener(onFirstImportProgress);

    // Confirm that we got some items
    log("DirectoryImport: onFirstImportProgress: totalAddedToLibrary=" + job.totalAddedToLibrary);
    log("DirectoryImport: onFirstImportProgress: totalDuplicates=" + job.totalDuplicates);
    log("DirectoryImport: onFirstImportProgress: gLibrary.length=" + gLibrary.length);
    assertTrue(gLibrary.length > 0);
    assertEqual(gLibrary.length, job.totalAddedToLibrary);
    assertEqual(job.totalAddedToMediaList, 0);
    assertTrue(job.enumerateAllItems().hasMoreElements());
    assertTrue(job.enumerateAllItems().getNext() instanceof Ci.sbIMediaItem);

    // Confirm that at least some metadata was scanned (the scanner is tested elsewhere).
    // Most of the test files fail our dupe check, so just look for anything.
    var foundSomeMetadata = false;
    for (var i=0; i < gLibrary.length; i++) {
      var item = gLibrary.getItemByIndex(i);
      var artist = item.getProperty(SBProperties.artistName);
      log("DirectoryImport: onFirstImportProgress: found item with artist=" + artist);
      if (artist) {
        foundSomeMetadata = true;
        break;
      }
    }
    assertEqual(foundSomeMetadata, true);

  } catch (e) {
    log("Error: " + e);
    // Force the test to fail.  If we throw it will
    // be eaten by the job progress notify function,
    // and the test will never finish.
    assertEqual(true, false);
  }

  startSecondImport();
}

/**
 * Now test directory import into a media list.
 */
function startSecondImport () {
  gMediaList = gLibrary.createMediaList("simple");

  // Add two items to the list, then have the importer
  // insert all the items into the middle
  gMediaList.add(gLibrary.getItemByIndex(1));
  gMediaList.add(gLibrary.getItemByIndex(2));

  var job = gDirectoryImporter.import(gDirectories, gMediaList,
                                      MEDIALIST_TARGET_INDEX);
  job.addJobProgressListener(onSecondImportProgress);
}


/**
 * Once the second directory import completes,
 * confirm that the files were inserted into the list
 * as expected, and that the items were reported
 * as dupes in the library
 */
function onSecondImportProgress(job) {
 try {
   log("DirectoryImport: onSecondImportProgress: " + job.statusText +
       " status " + job.status + " added " + job.totalAddedToLibrary );

   // Just ignore progress while running
   if (job.status == Ci.sbIJobProgress.STATUS_RUNNING) {
     return;
   }
   // Finished
   job.removeJobProgressListener(onSecondImportProgress);

   log("DirectoryImport: onSecondImportProgress: totalAddedToLibrary=" + job.totalAddedToLibrary);
   log("DirectoryImport: onSecondImportProgress: totalDuplicates=" + job.totalDuplicates);
   log("DirectoryImport: onSecondImportProgress: totalAddedToMediaList=" + job.totalAddedToMediaList);
   log("DirectoryImport: onSecondImportProgress: gLibrary.length=" + gLibrary.length);

   // Make sure everything adds up
   assertEqual(job.totalAddedToLibrary, 0);
   assertEqual(job.totalDuplicates, gLibrary.length - 1);
   assertEqual(gLibrary.length, job.totalAddedToMediaList + 1);
   assertEqual(gLibrary.length - 1, gMediaList.length - 2);
   assertEqual(gMediaList, job.targetMediaList);
   assertEqual(job.targetIndex, MEDIALIST_TARGET_INDEX);

   // All found items should appear in order between the first and
   // last items in the new list.
   var enumerator = job.enumerateAllItems();
   for (var i=1; i < gMediaList.length - 1; i++) {
     var item = gMediaList.getItemByIndex(i);
     log("DirectoryImport: onSecondImportProgress: found item with guid=" +
         item.guid);
     assertTrue(enumerator.hasMoreElements());
     assertEqual(enumerator.getNext(), item);
   }

 } catch (e) {
   log("Error: " + e);
   // Force the test to fail.  If we throw it will
   // be eaten by the job progress notify function,
   // and the test will never finish.
   assertEqual(true, false);
 }

 testFinished();
}
