/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END NIGHTINGALE GPL
 */


/**
 * \brief Test error handling in metadata jobs
 */
 
var gTestFileLocation = "testharness/metadatamanager/errorcases/";

// the number of errors we expect
var gErrorExpected = 0;
// the number of retries we expect
var gRetriesExpected = 0;

/**
 * Calculate how many retries are expected for a given set of metadata handlers
 * \param aHandlers an array of handlers expected
 * \return The number of retries expected
 */
function retries(aHandlers) {
  var contractId = "@getnightingale.com/Nightingale/MetadataHandler/";
  const MAP = {
    "taglib":    "Taglib;1",
    "gstreamer": "GStreamer;1",
    "wma":       "WMA;1"
  };
  var count = 0;
  for each (var handler in aHandlers) {
    var available = (contractId + MAP[String(handler).toLowerCase()]) in Cc;
    if (available)
      ++count;
  }
  return count - 1;
}

/**
 * Confirm that Nightingale doesn't crash or damage files when
 * metadata jobs fail
 */
function runTest() {
  var files = [];
  var filesToRemove = [];

  var isWindows = getPlatform() == "Windows_NT";
  
  ///////////////////////
  // Set up test files //
  ///////////////////////
  // A file that doesnt exist

  var file = newAppRelativeFile("testharness/metadatamanager/errorcases/file_that_doesnt_exist.mp3");
  assertEqual(file.exists(), false, "file_that_doesn't_exist shouldn't exist!");
  files.push(file);
  gErrorExpected++;
  gRetriesExpected += retries(["taglib", "gstreamer"]);

  // Bogus files
  var fakeFile = newAppRelativeFile("testharness/metadatamanager/errorcases/fake-file.mp3");
  fakeFile = getCopyOfFile(fakeFile, "fake-file-temp.mp3");
  files.push(fakeFile);
  filesToRemove.push(fakeFile);
  // fake file is not seen as an error
  var corruptFile = newAppRelativeFile("testharness/metadatamanager/errorcases/corrupt.mp3");
  corruptFile = getCopyOfFile(corruptFile, "corrupt-file-temp.mp3");
  files.push(corruptFile);
  filesToRemove.push(corruptFile);
  // corrupt file is not seen as an error
  
  // Media files with the wrong extensions
  files.push(newAppRelativeFile("testharness/metadatamanager/errorcases/mp3-disguised-as.flac"));
  gErrorExpected++;
  gRetriesExpected += retries(["taglib", "gstreamer"]);
  files.push(newAppRelativeFile("testharness/metadatamanager/errorcases/mp3-disguised-as.ogg"));
  gErrorExpected++;
  gRetriesExpected += retries(["taglib", "gstreamer"]);
  files.push(newAppRelativeFile("testharness/metadatamanager/errorcases/ogg-disguised-as.m4a"));
  gErrorExpected++;
  gRetriesExpected += retries(["taglib", "gstreamer"]);

  // Misc file permissions
  file = newAppRelativeFile("testharness/metadatamanager/errorcases/access-tests.mp3");  
  var readonly = getCopyOfFile(file, "readonly.mp3");
  readonly.permissions = 0400;
  files.push(readonly);
  filesToRemove.push(readonly);

  var writeonly = getCopyOfFile(file, "writeonly.mp3");
  writeonly.permissions = 0200;
  // If we aren't able to set write only, don't bother with this test (e.g. on windows)
  if ((writeonly.permissions & 0777) == 0200) {
    files.push(writeonly);
    gErrorExpected++;
    gRetriesExpected += retries(["taglib", "gstreamer"]);
  } else {
    log("MetadataJob_ErrorCases: platform does not support write-only. Perms=" + (writeonly.permissions & 0777));
  }
  filesToRemove.push(writeonly);
  
  var noaccess = getCopyOfFile(file, "noaccess.mp3");
  noaccess.permissions = 0000;  
  files.push(noaccess);
  filesToRemove.push(noaccess);
  if (!isWindows) {
    // only seen as an error on non-Windows (Windows doesn't support permissions correctly)
    gErrorExpected++
    gRetriesExpected += retries(["taglib", "gstreamer"]);
  }
  
  // A remote file that doesn't exist
  // XXX: temporarily, we use a non-default port. Unfortunately, we don't
  // fail properly if we get a 404 from the server! This is just to fix the
  // unit tests for now; once the other issues are fixed this should go back
  // to port 80.
  files.push(newURI("http://localhost:12345/remote/file/that/doesnt/exist.mp3"));
  gErrorExpected++;
  gRetriesExpected += retries(["taglib", "gstreamer"]);


  ///////////////////////////////////////
  // Load the files into two libraries //
  ///////////////////////////////////////
  log("Creating libraries");
  var library1 = createNewLibrary( "test_metadatajob_errorcases_library1" );
  var library2 = createNewLibrary( "test_metadatajob_errorcases_library2" );
  var items1 = importFilesToLibrary(files, library1);
  var items2 = importFilesToLibrary(files, library2);
  // We need to make the items in the two libraries copies of each other
  // So the synchronization logic is happy
  log("Populating library2");
  for (index = 0; index < items1.length; ++index) {
    // Set item 2 to be a copy of item 1
    let item1 = library1.getItemByIndex(index);
    let item2 = library2.getItemByIndex(index);
    // hopefully these should all be in the same order on reimport
    assertEqual(item1.contentSrc.spec,
                item2.contentSrc.spec,
                "expected item " + index + " of libraries to match");
    item2.setProperty(SBProperties.originItemGuid, item1.guid);
    item2.setProperty(SBProperties.originLibraryGuid, item2.library.guid);
    
  }

  assertEqual(items1.length,
              files.length,
              "expecting number of added items in library1 to equal total number of files");
  assertEqual(items2.length,
              files.length,
              "expecting number of added items in library2 to equal total number of files");
  
  var job = startMetadataJob(items1, "read");
  
  
  /////////////////////////////////////
  // Write new metadata to the files //
  /////////////////////////////////////
  
  // Called when the first scan into library1 completes
  function onLib1ReadComplete(job) {
    try {
      reportJobProgress(job, "onLib1ReadComplete");
          
      if (job.status == Ci.sbIJobProgress.STATUS_RUNNING) {
        return;
      }
      job.removeJobProgressListener(onLib1ReadComplete);
      
      // Verify job progress reporting.
      
      assertEqual(files.length + gRetriesExpected,
                  job.total,
                  "expected files plus retries to equal job total");
      assertEqual(files.length + gRetriesExpected,
                  job.progress,
                  "expected files plus retries to equal job progress");
      assertEqual(job.status,
                  Ci.sbIJobProgress.STATUS_FAILED,
                  "expected job to be failed");
      
      // Ok great, lets try writing back new metadata for all the files via library 2
      var propertiesToWrite = [ SBProperties.artistName,
                                SBProperties.albumName,
                                SBProperties.trackName
                              ];
      
      for each (var item in items2) {
        for each (var prop in propertiesToWrite) {
          item.setProperty(prop, prop);
        }
      }
      
      job = startMetadataJob(items2, "write", propertiesToWrite);
      
      // Wait for reading to complete before continuing
      job.addJobProgressListener(onWriteComplete); 

    // print errors, since otherwise they will be eaten by the observe call
    } catch (e) {
      doFail(e);
    }
  }
  
  
  //////////////////////////////////////
  // Read the metadata into library 2 //
  //////////////////////////////////////
    
  // Called when the write out from library2 completes
  function onWriteComplete(job) {
    try {
      reportJobProgress(job, "onWriteComplete");

      if (job.status == Ci.sbIJobProgress.STATUS_RUNNING) {
        return;
      }
      job.removeJobProgressListener(onWriteComplete);
      
      // Nothing should have been written.  
      // Make sure by reimporting library2 and comparing it with library1 
      library2.clear();
      items2 = importFilesToLibrary(files, library2);
      for (index = 0; index < items1.length; ++index) {
        let item1 = library1.getItemByIndex(index);
        let item2 = library2.getItemByIndex(index);
        // hopefully these should all be in the same order on reimport
        assertEqual(item1.contentSrc.spec,
                    item2.contentSrc.spec,
                    "expected item " + index + " of libraries to match");
        item2.setProperty(SBProperties.originItemGuid, item1.guid);
        item2.setProperty(SBProperties.originLibraryGuid, item2.library.guid);
      }
      assertEqual(items2.length,
                  files.length,
                  "expected number of items added to library to be all files");
      
      // Verify job progress reporting.
      assertEqual(job.total - 2,
                  job.errorCount,
                  "expected all but 2 items to fail");
      assertEqual(job.total,
                  files.length,
                  "expected the total to be the number of files");
      assertEqual(job.progress,
                  files.length,
                  "expected the process to be the number of files");
      assertEqual(job.status,
                  Ci.sbIJobProgress.STATUS_FAILED,
                  "expected the job to have failed");
      
      job = startMetadataJob(items2, "read");

      // Wait for reading to complete before continuing
      job.addJobProgressListener(onLib2ReadComplete); 

    // print errors, since otherwise they will be eaten by the observe call
    } catch (e) {
      doFail(e);
    }
  }
  
  
  ///////////////////////////////////////
  // Compare library 1 with library 2  //
  ///////////////////////////////////////

  // Called when reading metadata back into library2 completes
  function onLib2ReadComplete(job) {
    try {
      reportJobProgress(job, "onLib2ReadComplete");

      if (job.status == Ci.sbIJobProgress.STATUS_RUNNING) {
        return;
      }
      job.removeJobProgressListener(onLib2ReadComplete);
      
      // Make sure writing didnt break anything by
      // comparing library1 with library2
      var diffingService = Cc["@getnightingale.com/Nightingale/Library/DiffingService;1"]
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
        assertTrue(url == fakeFileURL || url == corruptFileURL,
                   "expected url to be either fakeFileURL or curruptFileURL");
      }
      assertEqual(changes.length,
                  2,
                  "expected 2 changes");
      
      // Verify job progress reporting.  Do this last since the info above is
      // useful for debugging.
      
      assertEqual(job.errorCount,
                  gErrorExpected,
                  "error count unexpected");
      assertEqual(files.length + gRetriesExpected,
                  job.total,
                  "expected files plus retries to equal total");
      assertEqual(files.length + gRetriesExpected,
                  job.progress,
                  "expected files plus retries to equal progress");
      assertEqual(job.status,
                  Ci.sbIJobProgress.STATUS_FAILED,
                  "expected job to have failed");
      
    // print errors, since otherwise they will be eaten by the observe call
    } catch (e) {
      doFail(e);
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
      doFail(e);
    }
    testFinished();
  }
  
  // Wait for reading to complete before continuing
  job.addJobProgressListener(onLib1ReadComplete);
  testPending();
}



/**
 * Add files to a library, returning media items
 */
function importFilesToLibrary(files, library) {
  var items = [];
  for each (var file in files) {
    if (!(file instanceof Ci.nsIURI)) {
      file = newFileURI(file);
    }
    items.push(library.createMediaItem(file, null, true));
  }
  return items;
}


/**
 * Get a metadata job for the given items
 */
function startMetadataJob(items, type, writeProperties) {
  var prefSvc = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefBranch);
  var oldWritingEnabledPref = prefSvc.getBoolPref("nightingale.metadata.enableWriting");
  prefSvc.setBoolPref("nightingale.metadata.enableWriting", true);
  var array = Cc["@getnightingale.com/moz/xpcom/threadsafe-array;1"]
                .createInstance(Ci.nsIMutableArray);
  for each (var item in items) {
    array.appendElement(item, false);
  }                     
  manager = Cc["@getnightingale.com/Nightingale/FileMetadataService;1"]
              .getService(Ci.sbIFileMetadataService);
  var job;
  if (type == "write") {
    job = manager.write(array, ArrayConverter.stringEnumerator(writeProperties));
  } else {
    job = manager.read(array);
  }
  prefSvc.setBoolPref("nightingale.metadata.enableWriting", oldWritingEnabledPref); 
  
  return job;
}
