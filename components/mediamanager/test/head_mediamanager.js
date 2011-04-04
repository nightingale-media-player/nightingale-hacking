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
 * Since we can't use the FUEL components until after all other components have
 * been loaded we define a lazy getter here for when we need it.
 */
__defineGetter__("Application", function() {
  delete this.Application;
  return this.Application = Cc["@mozilla.org/fuel/application;1"]
                              .getService(Ci.fuelIApplication);
});


Components.utils.import('resource://gre/modules/XPCOMUtils.jsm');

/**
 * \brief Some globally useful stuff for the tests
 */
var SB_NS = "http://songbirdnest.com/data/1.0#";
var SB_MEDIAFILEMANAGER = "@songbirdnest.com/Songbird/media-manager/file;1";

var SB_MM_PROP_FOLDER = "media-folder";
var SB_MM_PROP_FMTFILE = "file-format";
var SB_MM_PROP_FMTDIR = "dir-format";


// An array of what our test results should be
var gResultInformation = [
  { originalFileName: "TestFile1.mp3",
    expectedFileName: "1 - Sample.mp3",
    expectedFolder:   "Managed/Songbird/Unit Test Classics",
    expectedAction:   Ci.sbIMediaFileManager.MANAGE_COPY |
                      Ci.sbIMediaFileManager.MANAGE_RENAME },
  { originalFileName: "TestFile2.mp3",
    expectedFileName: "2 - Sample.mp3",
    expectedFolder:   "Managed/Songbird/Unit Test Classics",
    expectedAction:   Ci.sbIMediaFileManager.MANAGE_COPY |
                      Ci.sbIMediaFileManager.MANAGE_RENAME },
  { originalFileName: "TestFile3.mp3",
    expectedFileName: "3 - Sample.mp3",
    expectedFolder:   "Managed/Songbird/Unit Test Classics",
    expectedAction:   Ci.sbIMediaFileManager.MANAGE_COPY |
                      Ci.sbIMediaFileManager.MANAGE_RENAME },

  { originalFileName: "TestFile4.mp3",
    expectedFileName: "1 - TestFile4.mp3",
    expectedFolder:   "Managed/Unknown Artist/Unknown Album",
    expectedAction:   Ci.sbIMediaFileManager.MANAGE_COPY |
                      Ci.sbIMediaFileManager.MANAGE_RENAME },

  { originalFileName: "TestFile5.mp3",
    expectedFileName: "1 - Sample.mp3",
    expectedFolder:   "Managed/Songbird/Unknown Album",
    expectedAction:   Ci.sbIMediaFileManager.MANAGE_COPY |
                      Ci.sbIMediaFileManager.MANAGE_RENAME },

  { originalFileName: "TestFile6.mp3",
    expectedFileName: "1 - Sample.mp3",
    expectedFolder:   "Managed/Unknown Artist/Unit Test Classics",
    expectedAction:   Ci.sbIMediaFileManager.MANAGE_COPY |
                      Ci.sbIMediaFileManager.MANAGE_RENAME },

  { originalFileName: "TestFile7.mp3",
    expectedFileName: "01 - Sample7.mp3",
    expectedFolder:   "Managed/Songbird/Unit Test Classics",
    expectedAction:   Ci.sbIMediaFileManager.MANAGE_COPY |
                      Ci.sbIMediaFileManager.MANAGE_RENAME },

  { originalFileName: "TestFile8.mp3",
    expectedFileName: "11 - Sample8.mp3",
    expectedFolder:   "Managed/Songbird/Unit Test Classics",
    expectedAction:   Ci.sbIMediaFileManager.MANAGE_MOVE |
                      Ci.sbIMediaFileManager.MANAGE_RENAME }
];

// Test library to use.
var gTestLibrary;
// Items we add to the library to test our file management
var gTestMediaItems;
// We use this in many places for creating our file paths
var gFileLocation = "testharness/mediamanager/files/";


/**
 * Get a property bag containing all the configuration properties we want for
 * these tests.
 */
function getMediaManagerProperties () {

  var separator = "/";
  if (getPlatform() == "Windows_NT") {
    separator = "\\";
  }
  var managedFolder = testFolder.clone();
  managedFolder.append("Managed");
  // Create the folder
  managedFolder.create(Ci.nsIFile.DIRECTORY_TYPE, 0777);

  var properties = Cc["@mozilla.org/hash-property-bag;1"]
                     .createInstance(Ci.nsIWritablePropertyBag2);;

  properties.setPropertyAsInterface(SB_MM_PROP_FOLDER, managedFolder);
  properties.setPropertyAsAString(SB_MM_PROP_FMTFILE, 
                         SB_NS + "trackNumber," +
                         " - ," +
                         SB_NS + "trackName");
  properties.setPropertyAsAString(SB_MM_PROP_FMTDIR,
                         SB_NS + "artistName," +
                         separator + "," +
                         SB_NS + "albumName");

  return properties;
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
    newFile = appendPathToDirectory(newFile,
                                    gResultInformation[i].originalFileName);
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
 * \brief checkItem - Check to make sure the item has been organized properly,
 * this means it has been copyied to the managed folder if not originally there,
 * moved to the correct path under the managed folder, and renamed to the
 * correct filename.
 * \param aMediaItem - The media item to check
 * \param aResultInformationIndex - Index of the gResultInformation array that
 *  this item falls under.
 * \param aShouldHaveOriginal - Should the original file still be in place?
 */
function checkItem(aMediaItem, aResultInformationIndex, aShouldHaveOriginal) {
  // First get the current path from the item
  var current = aMediaItem.contentSrc;
  if (!(current instanceof Ci.nsIFileURL)) {
    log("item [" + current.spec + "] is not a file URL!");
    return false;
  }
  current = current.file;

  // Now put together the expected path for the item
  var expected = testFolder.clone();
  var expFolder = gResultInformation[aResultInformationIndex].expectedFolder;
  expected = appendPathToDirectory(expected, expFolder);
  expected.append(gResultInformation[aResultInformationIndex].expectedFileName);

  // Ensure that the file has been copied to its new location with proper
  // filename and folder path.
  if (!current.equals(expected)) {
    log("current value ["+ current.path +
        "] is not expectd value [" + expected.path + "]!");
    return false;
  }

  // Now check that the original file is still in place or not
  var original = testFolder.clone();
  var origName = gResultInformation[aResultInformationIndex].originalFileName;
  original = appendPathToDirectory(original, origName);

  if (!original.exists() && aShouldHaveOriginal) {
    log("original doesn't exist, but expected!");
    return false;
  } else if (original.exists() && !aShouldHaveOriginal) {
    log("original exists, but expected to be removed!");
    return false;
  }

  // All tests passed so the file has been organized correctly.
  return true;
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
 * A test Media List Listener to watch for additions to the library
 */
function TestMediaListListener() {
  this._added = Components.classes["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                          .createInstance(Components.interfaces.nsIMutableArray);
}
TestMediaListListener.prototype = {
  _added: null,
  _retval: false,

  get added() {
    return this._added;
  },

  onItemAdded: function onItemAdded(list, item, index) {
    this._added.appendElement( item, false );
    //this._added.push({list: list, item: item, index: index});
    return this._retval;
  },

  onBeforeItemRemoved: function onBeforeItemRemoved(list, item, index) {
    return this._retval;
  },

  onAfterItemRemoved: function onAfterItemRemoved(list, item, index) {
    return this._retval;
  },

  onItemUpdated: function onItemUpdated(list, item, properties) {
    return this._retval;
  },

  onItemMoved: function onItemMoved(list, fromIndex, toIndex) {
    return this._retval;
  },

  onBatchBegin: function onBatchBegin(list) {
  },

  onBatchEnd: function onBatchEnd(list) {
  },
  onBeforeListCleared: function onBeforeListCleared() {
    return this._retval;
  },
  onListCleared: function onListCleared() {
    return this._retval;
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaListListener])
}


/**
 * Copy the given folder to tempName, returning an nsIFile for the new location
 */
function getCopyOfFolder(folder, tempName) {
  assertNotEqual(folder, null);
  var tempFolder = getTempFolder();
  folder.copyTo(tempFolder, tempName);
  folder = tempFolder.clone();
  folder.append(tempName);
  assertEqual(folder.exists(), true);
  return folder;
}

/**
 * Appends each directory found in path to the nsIFile object directory.
 */
function appendPathToDirectory(directory, path) {
  var nodes = path.split("/");
  for ( var i = 0, end = nodes.length; i < end; i++ )
  {
    directory.append( nodes[ i ] );
  }
  return directory;
}

/**
 * Get a temporary folder for use in metadata tests.
 * Will be removed in tail_metadatamanager.js
 */
var gTempFolder = null;
function getTempFolder() {
  if (gTempFolder) {
    return gTempFolder;
  }
  gTempFolder = Components.classes["@mozilla.org/file/directory_service;1"]
                       .getService(Components.interfaces.nsIProperties)
                       .get("TmpD", Components.interfaces.nsIFile);
  gTempFolder.append("songbird_mediamanager_tests.tmp");
  gTempFolder.createUnique(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0777);
  return gTempFolder;
}

/**
 * Get rid of the temp folder created by getTempFolder.
 * Called in tail_metadatamanager.js
 */
function removeTempFolder() {
  if (gTempFolder && gTempFolder.exists()) {
    gTempFolder.remove(true);
  } else {
    log("\n\n\nMedia Manager Test may not have performed cleanup.  Temp files may exist.\n\n\n");
  }
}

/**
 * Get rid of any test librarys we have created
 */
function removeTestLibraries() {
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"].
                       getService(Ci.sbILibraryManager);

  // Since unregisterLibrary does not clear or delete the library we have
  // to clear it so that subsequent createLibrary calls with the same guid do
  // not have items in it.
  gTestLibrary.clear();
  libraryManager.unregisterLibrary(gTestLibrary);
}
