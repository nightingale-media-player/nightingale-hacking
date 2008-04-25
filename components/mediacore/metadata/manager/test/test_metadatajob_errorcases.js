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
 * \brief Test error handling in metadata jobs
 */
 
var gTestFileLocation = "testharness/metadatamanager/errorcases/";
  

/**
 * Confirm that Songbird doesn't crash or damage files when
 * metadata jobs fail
 */
function runTest() {
  var files = [];
  var filesToRemove = [];

  
  ///////////////////////
  // Set up test files //
  ///////////////////////
  
  // A file that doesnt exist
  var file = newAppRelativeFile("testharness/metadatamanager/errorcases/file_that_doesnt_exist.mp3");
  assertEqual(file.exists(), false);
  
  // Bogus files
  var fakeFile = newAppRelativeFile("testharness/metadatamanager/errorcases/fake-file.mp3");
  fakeFile = getCopyOfFile(fakeFile, "fake-file-temp.mp3");
  files.push(fakeFile);
  filesToRemove.push(fakeFile);
  var corruptFile = newAppRelativeFile("testharness/metadatamanager/errorcases/corrupt.mp3");
  corruptFile = getCopyOfFile(corruptFile, "corrupt-file-temp.mp3");
  files.push(corruptFile);
  filesToRemove.push(corruptFile);
  
  // Misc file permissions
  file = newAppRelativeFile("testharness/metadatamanager/errorcases/access-tests.mp3");  
  var readonly = getCopyOfFile(file, "readonly.mp3");
  readonly.permissions = 0400;
  files.push(readonly);
  filesToRemove.push(readonly);
  
  var writeonly = getCopyOfFile(file, "writeonly.mp3");
  writeonly.permissions = 0200;
  // If we aren't able to set write only, don't bother with this test (e.g. on windows)
  if (writeonly.permissions == 0200) {
    files.push(writeonly);
  } else {
    log("MetadataJob_ErrorCases: platform does not support write-only.");
  }
  filesToRemove.push(writeonly);
  
  var noaccess = getCopyOfFile(file, "noaccess.mp3");
  noaccess.permissions = 0000;  
  files.push(noaccess);
  filesToRemove.push(noaccess);
  
  // A remote file that doesn't exist
  files.push(newURI("http://localhost/remote/file/that/doesnt/exist.mp3"));
  
    
  ///////////////////////////////////////
  // Load the files into two libraries //
  ///////////////////////////////////////
  var library1 = createNewLibrary( "test_metadatajob_errorcases_library1" );
  var library2 = createNewLibrary( "test_metadatajob_errorcases_library2" );
  var items1 = importFilesToLibrary(files, library1);
  var items2 = importFilesToLibrary(files, library2);
  assertEqual(items1.length, files.length);
  assertEqual(items2.length, files.length);

  var job = startMetadataJob(items1, Components.interfaces.sbIMetadataJob.JOBTYPE_READ);
  
  
  /////////////////////////////////////
  // Write new metadata to the files //
  /////////////////////////////////////
  
  // Called when the first scan into library1 completes 
  function onLib1ReadComplete(aSubject, aTopic, aData) {    
    try {
      assertEqual(aTopic, "complete");
      assertTrue(aSubject.completed);
      aSubject.removeObserver();
      
      // Ok great, lets try writing back new metadata for all the files via library 2
      for each (var item in items2) {
        item.setProperty(SBProperties.artistName, SBProperties.artistName);
        item.setProperty(SBProperties.albumName, SBProperties.albumName);
        item.setProperty(SBProperties.trackName, SBProperties.trackName);
      }
      
      job = startMetadataJob(items2, Components.interfaces.sbIMetadataJob.JOBTYPE_WRITE);
      
      // Wait for reading to complete before continuing
      job.setObserver({ observe: onWriteComplete });      

    // print errors, since otherwise they will be eaten by the observe call
    } catch (e) {
      log("\nERROR: " + e + "\n");
      assertEqual(true, false);
    }
  }
  
  
  //////////////////////////////////////
  // Read the metadata into library 2 //
  //////////////////////////////////////
    
  // Called when the write out from library2 completes
  function onWriteComplete(aSubject, aTopic, aData) {
    try {
      assertEqual(aTopic, "complete");
      assertTrue(aSubject.completed);
      aSubject.removeObserver();
      
      // Nothing should have been written.  
      // Make sure by reimporting library2 and comparing it with library1 
      library2.clear();
      items2 = importFilesToLibrary(files, library2);
      assertEqual(items2.length, files.length);

      job = startMetadataJob(items2, Components.interfaces.sbIMetadataJob.JOBTYPE_READ);

      // Wait for reading to complete before continuing
      job.setObserver({ observe:  onLib2ReadComplete });      

    // print errors, since otherwise they will be eaten by the observe call
    } catch (e) {
      log("\nERROR: " + e + "\n");
      assertEqual(true, false);
    }
  }
  
  
  ///////////////////////////////////////
  // Compare library 1 with library 2  //
  ///////////////////////////////////////

  // Called when reading metadata back into library2 completes
  function onLib2ReadComplete(aSubject, aTopic, aData) {
    try {
      assertEqual(aTopic, "complete");
      assertTrue(aSubject.completed);
      aSubject.removeObserver();
      // Make sure writing didnt break anything by
      // comparing library1 with library2
      var diffingService = Cc["@songbirdnest.com/Songbird/Library/DiffingService;1"]
                            .getService(Ci.sbILibraryDiffingService);
      var libraryChangeset = diffingService.createChangeset(library2, 
                                                            library1);
      var changes = libraryChangeset.changes;
      log("\n\n\nMetadataJob_ErrorCases: There are " + changes.length + 
           " differences between library1 and library2.\n\n");
      var changesEnum = changes.enumerate();
      
      // Only the bogus files should have changed,
      // since taglib doesn't know to leave them alone.
      var fakeFileURL = newFileURI(fakeFile).spec;
      var corruptFileURL = newFileURI(corruptFile).spec;
      while(changesEnum.hasMoreElements()) {
        var change = changesEnum.getNext().QueryInterface(Ci.sbILibraryChange);
        var propEnum = change.properties.enumerate();
        var url = change.sourceItem.contentSrc.spec;
        log("MetadataJob_ErrorCases: changes in " + 
             url + "\n");
        while(propEnum.hasMoreElements()) {
          var prop = propEnum.getNext().QueryInterface(Ci.sbIPropertyChange);
          log("\t\t[" + prop.id + "] " + prop.oldValue + " -> " + prop.newValue + "\n");
        }
        assertEqual(url == fakeFileURL || url == corruptFileURL, true);
      }
      assertEqual(changes.length, 2);

    // print errors, since otherwise they will be eaten by the observe call
    } catch (e) {
      log("\nERROR: " + e + "\n");
      assertEqual(true, false);
    }
    finish();
  }
  
  ///////////////////////////////////////////////
  // Get rid of temp files and finish the test //
  ///////////////////////////////////////////////
  function finish() {
    try {
      // Clean up temp files
      for each (file in filesToRemove) {
        // Restore file perms so that windows can remove the file
        file.permissions = 0600; 
          
        file.remove(true);
      }
      job = null;
    } catch (e) {
      log("ERROR: " + e + "\n");
    }  
    testFinished();
  }
  
  // Wait for reading to complete before continuing
  job.setObserver({ observe: onLib1ReadComplete });
  testPending();
}



/**
 * Add files to a library, returning media items
 */
function importFilesToLibrary(files, library) {
  var items = [];
  for each (var file in files) {
    if (!(file instanceof Components.interfaces.nsIURI)) {
      file = newFileURI(file);
    }
    items.push(library.createMediaItem(file, null, true));
  }
  return items;
}


/**
 * Get a metadata job for the given items
 */
function startMetadataJob(items, type) {
  var prefSvc = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefBranch);
  var oldWritingEnabledPref = prefSvc.getBoolPref("songbird.metadata.enableWriting");
  prefSvc.setBoolPref("songbird.metadata.enableWriting", true);
  var array = Components.classes["@mozilla.org/array;1"]
                        .createInstance(Components.interfaces.nsIMutableArray);
  for each (var item in items) {
    array.appendElement(item, false);
  }                     
  manager = Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                      .getService(Components.interfaces.sbIMetadataJobManager);
  var job = manager.newJob(array, 5, type);
  prefSvc.setBoolPref("songbird.metadata.enableWriting", oldWritingEnabledPref); 
  
  return job;
}

