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

// Media manager preferences
const SB_MM_PREF_FOLDER = "songbird.media_management.library.folder";
const SB_MM_PREF_ENABLED = "songbird.media_management.library.enabled";
const SB_MM_PREF_COPY = "songbird.media_management.library.copy";
const SB_MM_PREF_MOVE = "songbird.media_management.library.move";
const SB_MM_PREF_RENAME = "songbird.media_management.library.rename";
const SB_MM_PREF_FMTDIR = "songbird.media_management.library.format.dir";
const SB_MM_PREF_FMTFILE = "songbird.media_management.library.format.file";

// Keep a copy of the original prefs so we don't screw anything up
var gOriginalPrefs = null;

/**
 * Save the Media Manager preferences.
 */
function saveMediaManagerPreferences () {
  // Setup some defaults
  gOriginalPrefs = {
    folder: null,
    copy: false,
    move: false,
    rename: false,
    formatDir: "",
    formatFile: "",
    enabled: false
  };

  var prefBranch = Cc["@mozilla.org/preferences-service;1"]
                     .getService(Ci.nsIPrefBranch2);

  // Store the current preferences so we can restore them later
  if (Application.prefs.has(SB_MM_PREF_FOLDER)) {
    gOriginalPrefs.folder = prefBranch.getComplexValue(SB_MM_PREF_FOLDER,
                                                       Ci.nsILocalFile);
  }
  
  gOriginalPrefs.enabled = Application.prefs.getValue(SB_MM_PREF_ENABLED, false);
  gOriginalPrefs.copy = Application.prefs.getValue(SB_MM_PREF_COPY, false);
  gOriginalPrefs.move = Application.prefs.getValue(SB_MM_PREF_MOVE, false);
  gOriginalPrefs.rename = Application.prefs.getValue(SB_MM_PREF_RENAME, false);
  gOriginalPrefs.formatDir = Application.prefs.getValue(SB_MM_PREF_FMTDIR, "");
  gOriginalPrefs.formatFile = Application.prefs.getValue(SB_MM_PREF_FMTFILE, "");
}

/**
 * Restore the Media Manager preferences.
 * Called in tail_metadatamanager.js
 */
function restoreMediaManagerPreferences() {
  // Don't do anything if we didn't save the prefs
  if (gOriginalPrefs == null) {
    return;
  }

  var prefBranch = Cc["@mozilla.org/preferences-service;1"]
                     .getService(Ci.nsIPrefBranch2);

  if (gOriginalPrefs.folder) {
    prefBranch.setComplexValue(SB_MM_PREF_FOLDER,
                               Ci.nsILocalFile,
                               gOriginalPrefs.folder);
  }
  prefBranch.setBoolPref(SB_MM_PREF_ENABLED, gOriginalPrefs.enabled);
  prefBranch.setBoolPref(SB_MM_PREF_COPY, gOriginalPrefs.copy);
  prefBranch.setBoolPref(SB_MM_PREF_MOVE, gOriginalPrefs.move);
  prefBranch.setBoolPref(SB_MM_PREF_RENAME, gOriginalPrefs.rename);
  prefBranch.setCharPref(SB_MM_PREF_FMTDIR, gOriginalPrefs.formatDir);
  prefBranch.setCharPref(SB_MM_PREF_FMTFILE, gOriginalPrefs.formatFile);
}



/**
 * Create our test library
 */
function createNewLibrary(databaseGuid, databaseLocation) {

  var directory;
  if (databaseLocation) {
    directory = databaseLocation.QueryInterface(Ci.nsIFileURL).file;
  }
  else {
    directory = Cc["@mozilla.org/file/directory_service;1"].
                getService(Ci.nsIProperties).
                get("ProfD", Ci.nsIFile);
    directory.append("db");
  }
  
  var file = directory.clone();
  file.append(databaseGuid + ".db");

  var libraryFactory =
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"]
      .getService(Ci.sbILibraryFactory);
  var hashBag = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag2);
  hashBag.setPropertyAsInterface("databaseFile", file);
  var library = libraryFactory.createLibrary(hashBag);
  try {
    library.clear();
  }
  catch(e) {
  }
  
  if (library) {
    var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"].
                         getService(Ci.sbILibraryManager);
    libraryManager.registerLibrary(library, false);
  }
  return library;
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
 * Gets a file/folder in our test folder
 */
function newAppRelativeFile( path ) {
  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("resource:app", Ci.nsIFile);
  file = file.clone();
  return appendPathToDirectory(file, path);
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
 * Make sure to save the current preferences so we do not mess anything up.
 */
//saveMediaManagerPreferences();
