/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
 * \brief Test creation and execution of metadata writing jobs
 */
 
var gTestFileLocation = "testharness/metadatamanager/files/";

// TODO Confirm this extension list. Don't forget to update files/Makefile.in as well.
var gSupportedFileExtensions = [
    "mp3", "mov", "m4p", "ogm", "ogx", "tta", "wv",
    
// TODO Taglib does not support video files
// "m4v", "ogv"
    
   "ogg", "flac", "mpc"

// TODO oga and spx reading isn't working yet.  Filed as bug 8768.
// "oga", "spx"

// TODO Unable to write metadata for m4a files. Filed as bug 8812.
// "m4a"
  ];

/**
 * Copy some media files into a temp directory, then confirm that metadata
 * properties can be modified using a write job.
 */
function runTest() {
  /* SETUP */
  
  // Make a copy of everything in the test file folder
  // so that our changes don't interfere with other tests
  var testFolder = getCopyOfFolder(newAppRelativeFile(gTestFileLocation), "_temp_writing_files");
  
  // Make a file with a unicode filename.  This would be checked in, except
  // the windows build system can't handle unicode filenames.
  var unicodeFile = testFolder.clone();
  unicodeFile.append("MP3_ID3v23.mp3");
  unicodeFile = getCopyOfFile(unicodeFile, "\u2606\u2606\u2606\u2606\u2606\u2606.mp3");
  
  var prefSvc = Cc["@mozilla.org/preferences-service;1"]
                .getService(Ci.nsIPrefBranch);
  var oldWritingEnabledPref = prefSvc.getBoolPref("nightingale.metadata.enableWriting");
  prefSvc.setBoolPref("nightingale.metadata.enableWriting", true);

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
  
  var job = startMetadataJob(items, "read");
  
  // Wait for reading to complete before continuing
  job.addJobProgressListener(function onReadComplete(job) {
    reportJobProgress(job, "MetadataJob_Writing - onReadComplete");
    
    if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
      return;
    }
    job.removeJobProgressListener(onReadComplete);
  
    //////////////////////////////////////
    // Save new metadata into the files //
    //////////////////////////////////////
    var unicodeSample =
            "\u008C\u00A2\u00FE" + // Latin-1 Sup
            "\u141A\u142B\u1443" + // Canadian Aboriginal
            "\u184F\u1889\u1896" + // Mongolian
            "\u4E08\u4E02\u9FBB" + // CJK Unified Extension
            "\u4DCA\u4DC5\u4DD9" + // Hexagram Symbols
            "\u0308\u030F\u034F" + // Combining Diacritics
            "\u033C\u034C\u035C" +
            "\uD800\uDC83\uD800" + // Random Surrogate Pairs 
            "\uDC80\uD802\uDD00";
    
    var successValues = [
      // test a basic value
      [SBProperties.trackName, SBProperties.trackName],
      
      // and some unicode
      [SBProperties.albumName, SBProperties.albumName + unicodeSample],
      
      // what about a longer one? (ID3v1 only allows 30 char.)
      // bug <NNN>: id3v22 files are coming back truncated but not ID3v1 for some reason
      //[SBProperties.comment,
      //  SBProperties.comment + SBProperties.comment + SBProperties.comment],
      
      // this is slightly thorny, because the specs allow for multiple values
      // but our database just concats things together.
      // we should think about this some day
      [SBProperties.genre, SBProperties.genre],
      
      // make sure extended properties work
      [SBProperties.composerName, SBProperties.composerName],
      [SBProperties.lyricistName, SBProperties.lyricistName],
      [SBProperties.lyrics, SBProperties.lyrics],
      
      // bug 9086: producer is fucked for ID3v2 -- need TIPL/IPLS
      //           and if i'm going to do that, all the other ones should check it too
      //[SBProperties.producerName, SBProperties.producerName],
      
      // bug 9084: rating is fucked for ID3v2
      //[SBProperties.rating, 3],
      
      // make sure track numbers work
      [SBProperties.trackNumber, 7],
      [SBProperties.totalTracks, 13],
      
      [SBProperties.discNumber, 1],
      [SBProperties.totalDiscs, 2],
      
      // what about a longer one? (ID3v1 only allows 30 char.)
      // bug 9088: id3v22 files are coming back truncated but not ID3v1 for some reason
      //[SBProperties.comment,
      //  SBProperties.comment + SBProperties.comment + SBProperties.comment],
      
      // try a blank one
      [SBProperties.artistName, null],

      // what about a partial disc number?
      //[SBProperties.totalDiscs, 0], // TODO: reconcile 0/""/null
      //[SBProperties.totalDiscs, 2],
      
      [SBProperties.year, 2004],
      
      [SBProperties.isPartOfCompilation, 1]
    ];

    // List of properties we want to write
    var propertiesToWrite = [];
    
    // set all mediaitems to the supplied metadata
    for each (var item in items) {
      for each (var pair in successValues) {
        item.setProperty(pair[0], pair[1]);
        
        // Add the property name to the write list if we have not alredy done so
        if (propertiesToWrite.indexOf(pair[0]) < 0) {
          propertiesToWrite.push(pair[0]);
        }
      }
    }

    job = startMetadataJob(items, "write", propertiesToWrite);

    // Wait for writing to complete before continuing
    job.addJobProgressListener(function onWriteComplete(job) {
      reportJobProgress(job, "MetadataJob_Writing - onWriteComplete");
    
      if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
        return;
      }
      job.removeJobProgressListener(onWriteComplete);
      
      // Verify job progress reporting.  
      // NOTE: Comment these out to receive more useful debug information
      assertEqual(urls.length, job.total);
      assertEqual(urls.length, job.progress);
      assertEqual(0, job.errorCount);
      assertEqual(job.status, Components.interfaces.sbIJobProgress.STATUS_SUCCEEDED);
      
      /////////////////////////////////////////////////////
      // Now reimport and confirm that the write went ok //
      /////////////////////////////////////////////////////
      library.clear();
      items = importFilesToLibrary(urls, library);
      assertEqual(items.length, urls.length);
      job = startMetadataJob(items, "read");

      // Wait for reading to complete before continuing
      job.addJobProgressListener(function onSecondReadComplete(job) {
        reportJobProgress(job, "MetadataJob_Writing - onSecondReadComplete");
        if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
          return;
        }
        job.removeJobProgressListener(onSecondReadComplete);
        
        // Verify job progress reporting.  
        // NOTE: Comment these out to receive more useful debug information
        assertEqual(urls.length, job.total);
        assertEqual(urls.length, job.progress);
        assertEqual(0, job.errorCount);
        assertEqual(job.status, Components.interfaces.sbIJobProgress.STATUS_SUCCEEDED);
        
        var failedFiles = 0;
        for each (var item in items) {
          var failedProperties = 0;
          log("MetadataJob_Write: verifying " + item.contentSrc.path);
          for each (var pair in successValues) {
            if (item.getProperty(pair[0]) != pair[1]) {
              
              // bug 9091 -- ID3v22 doesn't seem to support lyrics
              if(item.contentSrc.path.indexOf("MP3_ID3v22.mp3") != 0) {
                if(pair[0] == SBProperties.lyricistName || pair[0] == SBProperties.lyrics) {
                  continue;
                }
              }
              
              log("MetadataJob_Write: \n" + pair[0]+ ":\nfound:\t" + item.getProperty(pair[0])
                  + "\nwanted:\t" + pair[1]);
              failedProperties++;
            }
          }
          if (failedProperties != 0) {
            failedFiles++;
          }
        }
        assertEqual(failedFiles, 0, "Some files failed to write correctly.");
        
        prefSvc.setBoolPref("nightingale.metadata.enableWriting", oldWritingEnabledPref); 
        // We're done, so kill all the temp files
        testFolder.remove(true);
        job = null;
        testFinished();
      });
    });
  });
  testPending();
}


/**
 * Get an array of all media files below the given folder
 */
function getMediaFilesInFolder(folder) {
  var scan = Cc["@getnightingale.com/Nightingale/FileScan;1"]
               .createInstance(Ci.sbIFileScan);
  var query = Cc["@getnightingale.com/Nightingale/FileScanQuery;1"]
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

  scan.finalize();
  
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
function startMetadataJob(items, type, writeProperties) {
  var array = Components.classes["@getnightingale.com/moz/xpcom/threadsafe-array;1"]
                        .createInstance(Components.interfaces.nsIMutableArray);
  for each (var item in items) {
    array.appendElement(item, false);
  }                     
  manager = Components.classes["@getnightingale.com/Nightingale/FileMetadataService;1"]
                      .getService(Components.interfaces.sbIFileMetadataService);
                      
  var job;
  if (type == "write") {
    job = manager.write(array, ArrayConverter.stringEnumerator(writeProperties));
  } else {
    job = manager.read(array);
  }
  return job;
}

