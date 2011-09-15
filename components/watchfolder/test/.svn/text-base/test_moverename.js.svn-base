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

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

// This test relies on a tree of sample files
var gTestFiles = newAppRelativeFile("testharness/watchfolders/files/moverename");

/**
 * Test WatchFolder's ability to correctly handle
 * moved and renamed files without deleting and re-adding.
 */
function runTest () {
  
  // Start with an empty library
  var library = LibraryUtils.mainLibrary;
  library.clear();
  // The library wont actually be empty, since 
  // the download list and stuff like that come back
  var originalLibraryLength = library.length;
  
  // Start watching the temp folder, since we are going to make 
  // changes there
  setWatchFolder(getTempFolder());
  sleep(15000);

  // Get the test files set up
  var testFolder = getCopyOfFolder(gTestFiles, "_temp_moverename_files");
  log('using test folder ' + testFolder.path);

  // That should cause watch folders to add the files...
  sleep(15000);
  
  // Mark each item with it's original url, so we
  // can tell they update correctly
  library.enumerateAllItems({
     onEnumerationBegin: function(list) {
       return Ci.sbIMediaListEnumerationListener.CONTINUE;
     },
     onEnumeratedItem: function(list, item) {
       dump("Found media item:" + item.contentSrc.spec + "\n");
       // Remember the old path so we can verify things changed ok
       item.setProperty(SBProperties.originURL, item.contentSrc.spec);
       return Ci.sbIMediaListEnumerationListener.CONTINUE;
     },
     onEnumerationEnd: function(list, status) {
     }
  });

  assertEqual(library.length, originalLibraryLength + 5);

  // Here's what we expect to happen...
  var root = newFileURI(testFolder).spec.toLowerCase();
  var map = {};
  map[root + "individualmovedfile.mp3"] = root + "dir3/individualmovedfile.mp3";
  map[root + "renamedfile.mp3"] = root + "renamedfile2.mp3";
  map[root + "dir1/dir2/movedfile1.mp3"] = root + "dir3/dir1/dir2/movedfile1.mp3";
  map[root + "dir1/dir2/movedfile2.mp3"] = root + "dir3/dir1/dir2/movedfile2.mp3";
  map[root + "deletedfile.mp3"] = null;
  map["null"] = root + "dir3/newfile.mp3";
  
  var dir1 = testFolder.clone();
  dir1.append("dir1");
  var dir3 = testFolder.clone();
  dir3.append("dir3");
  var individualMovedFile = testFolder.clone();
  individualMovedFile.append("individualMovedFile.mp3");
  var renamedFile = testFolder.clone();
  renamedFile.append("renamedFile.mp3");
  var deletedFile = testFolder.clone();
  deletedFile.append("deletedFile.mp3");
  var newFile = dir3.clone();
  newFile.append("random.txt");
  
  // Perform the move/renames
  dir1.moveTo(dir3, null);
  individualMovedFile.moveTo(dir3, null);
  renamedFile.moveTo(renamedFile.parent, "renamedFile2.mp3");
  deletedFile.remove(false);
  newFile.copyTo(newFile.parent, "newFile.mp3");

  // Wait for watchfolders to kick in...
  sleep(15000);

  // Now verify that the original media items still exist, 
  // and that the contentSrcs have been updated as expected
  var count = 0;
  library.enumerateAllItems({
     onEnumerationBegin: function(list) {
       return Ci.sbIMediaListEnumerationListener.CONTINUE;
     },
     onEnumeratedItem: function(list, item) {
       var newSpec = item.getProperty(SBProperties.contentURL).toLowerCase();
       var oldSpec = item.getProperty(SBProperties.originURL);
       if (oldSpec) oldSpec = oldSpec.toLowerCase()
       dump("Change: '" + oldSpec + "' -> '" + newSpec + "'\n");
       
       if (oldSpec in map) {
         if (newSpec == map[oldSpec]) {
           count++;
         } else {
           dump("FAIL: Expected '" + map[oldSpec] + 
                "' but got '" + newSpec + "'\n");
         }
         delete map[oldSpec]; // Delete, so we can't double count
       }
       return Ci.sbIMediaListEnumerationListener.CONTINUE;
     },
     onEnumerationEnd: function(list, status) {
     }
  });
  assertEqual(count, 5);
  
  // Shut down watch folders
  setWatchFolder(null);
  library.clear();
}
