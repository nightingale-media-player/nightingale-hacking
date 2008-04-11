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

var gLocalFiles = [
  "testharness/metadatamanager/test1.mp3",
  "testharness/metadatamanager/test2.mp3",
  "testharness/metadatamanager/test3.mp3"
];


/**
 * Copy some media files into a temp directory, then confirm that metadata
 * properties can be modified using a write job.
 */
function runTest() {
    
  // Make sure we have files to test
  assertEqual(gLocalFiles.length > 0, true);
  
  // Convert file paths into nsIFiles
  for (var i = 0; i < gLocalFiles.length; i++) {
    gLocalFiles[i] = newAppRelativeFile(gLocalFiles[i]);
    assertNotEqual( gLocalFiles[i], null );
  }
  
  // Copy all the test files into a temp folder so that
  // writing metadata doesn't interfere with other tests
  var tempFolder = gLocalFiles[0].parent;
  var temp;
  tempFolder.append("metadatajob_write_test");
  tempFolder.create(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0700);
  for (var i = 0; i < gLocalFiles.length; i++) {
    gLocalFiles[i].copyTo(tempFolder, null);
    temp = tempFolder.clone();
    temp.append(gLocalFiles[i].leafName);
    gLocalFiles[i] = temp;
  }
  
  ///////////////////////////
  // Import the test items //
  ///////////////////////////
  var library = createNewLibrary( "test_metadatajob_writing_library" );
  var items = importFilesToLibrary(gLocalFiles, library);
  assertEqual(items.length, gLocalFiles.length);
  var job = startMetadataJob(items, Components.interfaces.sbIMetadataJob.JOBTYPE_READ);
  
  // Wait for reading to complete before continuing
  job.setObserver(new MetadataJobObserver(function onReadComplete(aSubject, aTopic, aData) {
    assertEqual(aTopic, "complete");
    assertTrue(aSubject.completed);
    job.removeObserver();
  
    //////////////////////////////////////
    // Save new metadata into the files //
    //////////////////////////////////////
    for each (var item in items) {
      item.setProperty(SBProperties.artistName, SBProperties.artistName);
      item.setProperty(SBProperties.albumName, SBProperties.albumName);
      item.setProperty(SBProperties.trackName, SBProperties.trackName);
      // TODO expand 
    }
    
    // While we're at it, confirm that metadata can only be written when allowed via prefs
    var prefSvc = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefBranch);
    var oldWritingEnabledPref = prefSvc.getBoolPref("songbird.metadata.enableWriting");
    prefSvc.setBoolPref("songbird.metadata.enableWriting", false);
    try {
      startMetadataJob(items, Components.interfaces.sbIMetadataJob.JOBTYPE_WRITE);
      throw new Error("MetadataJobManager does not respect enableWriting pref!");
    } catch (e) {
      if (Components.lastResult != Components.results.NS_ERROR_NOT_AVAILABLE) {
        throw new Error("MetadataJobManager does not respect enableWriting pref!");
      }
    }
    prefSvc.setBoolPref("songbird.metadata.enableWriting", true);
    job = startMetadataJob(items, Components.interfaces.sbIMetadataJob.JOBTYPE_WRITE);
    prefSvc.setBoolPref("songbird.metadata.enableWriting", oldWritingEnabledPref); 
    
    // Wait for writing to complete before continuing
    job.setObserver(new MetadataJobObserver(function onReadComplete(aSubject, aTopic, aData) {
      assertEqual(aTopic, "complete");
      assertTrue(aSubject.completed);
      job.removeObserver();
      
      /////////////////////////////////////////////////////
      // Now reimport and confirm that the write went ok //
      /////////////////////////////////////////////////////
      library.clear();
      items = importFilesToLibrary(gLocalFiles, library);
      assertEqual(items.length, gLocalFiles.length);
      job = startMetadataJob(items, Components.interfaces.sbIMetadataJob.JOBTYPE_READ);

      // Wait for reading to complete before continuing
      job.setObserver(new MetadataJobObserver(function onSecondReadComplete(aSubject, aTopic, aData) {
        assertEqual(aTopic, "complete");
        assertTrue(aSubject.completed);
        job.removeObserver();
        
        for each (var item in items) {
          assertEqual(item.getProperty(SBProperties.artistName), SBProperties.artistName);
          assertEqual(item.getProperty(SBProperties.albumName), SBProperties.albumName);
          assertEqual(item.getProperty(SBProperties.trackName), SBProperties.trackName);
          // TODO expand 
        }
       
        // We're done, so kill all the temp files
        tempFolder.remove(true);
        job = null;
        gTest = null;    
        testFinished();
      }));
    }));
  }));
  testPending();
}

/**
 * Add files to a library, returning media items
 */
function importFilesToLibrary(files, library) {
  var items = [];
  for each (var file in files) {
    items.push(library.createMediaItem(newFileURI(file)));
  }
  return items;
}

/**
 * Get a metadata job for the given items
 */
function startMetadataJob(items, type) {
  var array = Components.classes["@mozilla.org/array;1"]
                        .createInstance(Components.interfaces.nsIMutableArray);
  for each (var item in items) {
    array.appendElement(item, false);
  }                     
  manager = Components.classes["@songbirdnest.com/Songbird/MetadataJobManager;1"]
                      .getService(Components.interfaces.sbIMetadataJobManager);
  return manager.newJob(array, 5, type);
}

