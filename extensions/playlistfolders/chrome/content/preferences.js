/* preferences.js
 * This will be loaded into the main window, and manage all preference-
 * related tasks.
 */
// Make a namespace.
if (typeof playlistfolders == 'undefined') {
  var playlistfolders = {};
};

/* Preference controller
 * Controller for all preference-related stuff.
 */
playlistfolders.preferences={
  // Playlist Folder's preference branch
  _prefs: null,
  /* The JSON object stored in a preference
   * Structure:
   * {
   *   version: <string>, // version for migrating issues. See _migrateIfNeeded
   *   folders: [<folder>], // see createFolder for folder structure
   * }
   */
  _root: null,
  
  // Startup code for the preference controller
  onLoad: function(e){
    playlistfolders.central.logEvent("preferences", "Perference controller " +
                                     "initialisation started.", 5);
    try {
      // Get the preferences
      var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                    getService(Components.interfaces.nsIPrefService);
      this._prefs = prefs.getBranch("extensions.playlistfolders.");
      // Get JSON
      try {
        this._root = JSON.parse(this._prefs.getCharPref("JSON"));
      } catch (e) {
        this._root = {};
        playlistfolders.central.logEvent("preferences",
                                         "extensions.playlistfolders.JSON " +
                                         "is invalid. Fallback to default " +
                                         "value.", 2,
                                         "chrome://playlistfolders/content/" +
                                         "preferences.js",
                                         e.lineNumber);
      }
      // Perform migration if needed
      this._migrateIfNeeded();
      // We're done.
      playlistfolders.central.logEvent("preferences",
                                       "Preference controller started.", 4);
    } catch (e) {
      playlistfolders.central.logEvent("preferences",
                                       "Startup of Preference controller " +
                                       "failed:\n\n" + e, 1, "chrome://" +
                                       "playlistfolders/content/" +
                                       "preferences.js", e.lineNumber);
    }
  },
  // Shutdown code for the preference controller
  onUnload: function(e){
    playlistfolders.central.logEvent("preferences", "Preference controller " +
                                     "shutdown started.", 5);
    // Save JSON
    this._saveCurrent();
    // We're done!
    playlistfolders.central.logEvent("preferences",
                                     "Preference controller stopped.", 4);
  },
  
  // Migrates JSON preferences (_root), if needed
  _migrateIfNeeded: function(){
    /* Playlist Folders JSON Migration Structure
     * For each AddOn version with new/changed entrys in JSON there is a
     * JSON version string (1.0.0.0 -> "1.0.0.0"). But not every Version
     * needs a separate JSON version (so in 1.0.0.0b2 the current JSON
     * version might be "1.0.0.0b1"). Final versions will have a non-beta
     * version, so 1.0.0.0 can't get "1.0.0.0b1", although nothing changed.
     * The migration will be performed in steps, if there are more than one
     * JSON version between the saved preference and the installed release.
     * Alpha and Beta Versions might have a own JSON version WITHOUT migration
     * code needed (so there might be no migration from 1.0.0.0a1 to 1.0.0.0b1)
     * There will be migration code for public beta releases in following
     * releases with the same version number, but taken out in next releases.
     */
    // Migration from fresh install
    if (!this._root.version){
      playlistfolders.central.logEvent("preferences", "Migration from fresh " +
                                       "install", 4);
      this._root.version = "1.0.0.0";
      this._root.folders = [];
      var root = this.createFolder("");
      root.GUID = "{root}";
    }
		// Don't kill alpha tester's JSON
		if (this._root.version == "0.0.0.0a")
		  this._root.version = "1.0.0.0";
    // If we weren't able to migrate, log a fatal error message and reset
    // everything.
    if (this._root.version != "1.0.0.0"){
      playlistfolders.central.logEvent("preferences", "There is no rule to " +
                                       "migrate from JSON version " +
                                       this._root.version + ". Fallback to " +
                                       "empty JSON root.", 2, "chrome://" +
                                       "playlistfolders/content/" +
                                       "preferences.js");
      this._root = {};
      this._migrateIfNeeded();
    }
  },
  
  // Saves current JSON (_root) to preferences
  _saveCurrent: function(){
    try {
      playlistfolders.central.logEvent("preferences",
                                       "Save JSON preferences", 5);
      this._prefs.setCharPref("JSON", JSON.stringify(this._root));
    } catch (e){
      playlistfolders.central.logEvent("preferences",
                                       "Saving of JSON preferences failed:" +
                                       "\n\n" + e, 1, "chrome://" +
                                       "playlistfolders/content/" +
                                       "preferences.js", e.lineNumber);
    }
  },
  
  /* Generate a new Folder
   * name (string): the name of the folder
   */
  createFolder: function(name){
    // generate a folder with the given name and the default values
    var result = {
      name: name, // The folder's name; MUST get changed via renameFolder ONLY!
      GUID: Components.classes["@mozilla.org/uuid-generator;1"].
            getService(Components.interfaces.nsIUUIDGenerator).generateUUID().
            toString(), // A unique ID for referencing purposes
      content: [], // GUIDs of subfolders and playlists in this folder; MUST
                   // get changed via move functions ONLY!
      closed: false, // True if the Folder's node was closed on last shutdown
    };
    // append and return the folder
    playlistfolders.central.logEvent("preferences",
                                     "Create Folder "+result.GUID, 5);
    try {
      this._root.folders.push(result);
      if (name != "")
        this.getFolderByGUID("{root}").content.push(result.GUID);
      this._saveCurrent(); // save current state to prevent loss of data
      return result;
    } catch (e){
      playlistfolders.central.logEvent("preferences",
                                       "Adding of folder failed:\n\n" + e, 1,
                                       "chrome://playlistfolders/content/" +
                                       "preferences.js", e.lineNumber);
    }
    return null;
  },
  
  /* Rename the given Folder
   * folder (folder): the folder to rename
   * name (string): the new name
   */
  renameFolder: function(folder, name){
    playlistfolders.central.logEvent("preferences", "Rename folder " +
                                     folder.GUID, 5);
    try {
      folder.name = name;
    } catch (e){
      playlistfolders.central.logEvent("preferences",
                                       "Renaming folder failed:\n\n" + e, 1,
                                       "chrome://playlistfolders/content/" +
                                       "preferences.js", e.lineNumber);
    }
    this._saveCurrent(); // save current state to prevent loss of data
  },
  
  /* Moves a folder and all sub-folders into another folder
   * folder (folder): the folder to move
   * targetF (folder): the target folder, or null for root folder
   * before (string, optional): insert before this (guid)
   */
  moveFolder: function(folder, targetF, before){
    playlistfolders.central.logEvent("preferences", "Move folder " +
                                     folder.GUID, 5);
    try {
      var target = targetF;
      if (!target)
        target = this.getFolderByGUID("{root}");
      // Search for current parent folder, and unlink it there
      for each (var f in this._root.folders)
        for (var i = 0; i < f.content.length; i++)
          if (f.content[i] == folder.GUID)
            f.content.splice(i,1);
      // Register new sub-folder at target
      var sort = false;
      if (before)
        for each (var guid in target.content)
          if (guid == before)
            sort = true;
      if (sort) {
        var tmp = [];
        for each (var f in target.content){
          if (f == before)
            tmp.push(folder.GUID);
          tmp.push(f);
        }
        target.content = tmp;
      } else {
        target.content.push(folder.GUID);
      }
    } catch (e){
      playlistfolders.central.logEvent("preferences",
                                       "Moving folder failed:\n\n" + e, 1,
                                       "chrome://playlistfolders/content/" +
                                       "preferences.js", e.lineNumber);
    }
    this._saveCurrent(); // save current state to prevent loss of data
  },
  
  /* Moves a playlist into another folder
   * pguid (string): the playlist's guid
   * targetF (folder): the target folder or null for root folder
   * before (string, optional): insert before this playlist (guid)
   */
  movePlaylist: function(pguid, targetF, before){
    playlistfolders.central.logEvent("preferences", "Move playlist " +
                                     pguid, 5);
    try {
      var target = targetF;
      if (!target)
        target = this.getFolderByGUID("{root}");
      // Search for current parent folder, and unlink it there
      for each (var f in this._root.folders)
        for (var i = 0; i < f.content.length; i++)
          if (f.content[i] == pguid)
            f.content.splice(i,1);
      // Register new sub-playlist at target
      var sort = false;
      if (before)
        for each (var guid in target.content)
          if (guid == before)
            sort = true;
      if (sort) {
        var tmp = [];
        for each (var p in target.content){
          if (p == before)
            tmp.push(pguid);
          tmp.push(p);
        }
        target.content = tmp;
      } else {
        target.content.push(pguid);
      }
    } catch (e){
      playlistfolders.central.logEvent("preferences",
                                       "Moving playlist failed:\n\n" + e, 1,
                                       "chrome://playlistfolders/content/" +
                                       "preferences.js", e.lineNumber);
    }
    this._saveCurrent(); // save current state to prevent loss of data
  },
  
  /* Removes the given Folder from the folder list, return true if succeed
   * folder (folder): the folder to remove
   * recursive (boolean): weather this should work even if there are subfolders
   *                      or playlists (true) or not (false)
   */
  removeFolder: function(folder, recursive){
    try {
      playlistfolders.central.logEvent("preferences",
                                       "Remove Folder " + folder.GUID, 5);
      // Check if we need recursive deletion
      if (folder.content.length){
        if (!recursive){
          playlistfolders.central.logEvent("preferences",
                                           "Removing of folder failed:\n\n" +
                                           folder.GUID + " is not empty" +
                                           " folder", 1, "chrome://" +
                                           "playlistfolders/content/" +
                                           "preferences.js");
          return;
        }
        var succeed = true;
        for each (var GUID in folder.content){
          if (this.isFolder(GUID)){
            if (!this.removeFolder(this.getFolderByGUID(GUID), true))
              succeed = false;
          } else if (this.isPlaylist(GUID)){
            if (!playlistfolders.central.removePlaylist(GUID))
              succeed = false;
          }
        }
        if (!succeed){
          playlistfolders.central.logEvent("preferences",
                                           "Removing of folder failed:\n\n" +
                                           folder.GUID + " has content " +
                                           "that can't get deleted", 1,
                                           "chrome://playlistfolders/" +
                                           "content/preferences.js");
          return;
        }
      }
      // Delete this folder
      var deleted = false;
      for (var i = 0; i < this._root.folders.length; i++){
        if (this._root.folders[i] == folder){
          this._root.folders.splice(i,1);
          deleted = true;
        }
      }
      // Delete references to this folder, if any
      for each (var f in this._root.folders)
        for (var i = 0; i < f.content.length; i++)
          if (f.content[i] == folder.GUID)
            f.content.splice(i,1);
      
      if (!deleted)
        playlistfolders.central.logEvent("preferences",
                                         "Removing of folder failed:\n\n" +
                                         folder.GUID + " is no registered" +
                                         " folder", 3, "chrome://" +
                                         "playlistfolders/content/" +
                                         "preferences.js");
      else
        return true;
    } catch (e){
      playlistfolders.central.logEvent("preferences",
                                       "Removing of folder failed:\n\n" + e, 1,
                                       "chrome://playlistfolders/content/" +
                                       "preferences.js", e.lineNumber);
    }
    this._saveCurrent(); // save current state to prevent loss of data
  },
  
  /* Remove the given Playlist from list WITHOUT deleting it
   * playlist (string): guid of the playlist to remove
   * folder (string): guid of parent folder of playlist
   */
  removePlaylist: function(playlist, folder){
    try {
      var plists = playlistfolders.preferences.getFolderByGUID(folder).content;
      var deleted = false;
      for (var i in plists)
        if (plists[i] == playlist){
          plists.splice(i,1);
          deleted = true;
        }
      if (!deleted)
        playlistfolders.central.logEvent("preferences", "Playlist to remove " +
                                         "not found in parent folder.", 2,
                                         "chrome://playlistfolders/content/" +
                                         "preferences.js");
      else
        playlistfolders.central.logEvent("preferences", "Playlist " +
                                         playlist + " removed in folder " +
                                         folder + ".", 5);
    } catch (e){
      playlistfolders.central.logEvent("preferences",
                                       "Removing of playlist failed:\n\n" + e,
                                       1, "chrome://playlistfolders/content/" +
                                       "preferences.js", e.lineNumber);
    }
    this._saveCurrent(); // save current state to prevent loss of data
  },
  
  // Returns the array of all folders
  getFolders: function(){
    try {
      return this._root.folders;
    } catch (e){
      playlistfolders.central.logEvent("preferences",
                                       "Getting Folder list failed:\n\n" + e,
                                       1, "chrome://playlistfolders/content/" +
                                       "preferences.js",
                                       e.lineNumber);
    }
    return null;
  },
  
  /* Returns the folder with the given GUID
   * guid (string): the GUID of the requested folder
   */
  getFolderByGUID: function(guid){
    try {
      for each (var f in this._root.folders){
        if (f.GUID == guid)
          return f;
      }
      playlistfolders.central.logEvent("preferences",
                                       "Getting folder " + guid + " failed:" +
                                       "\n\n" + guid + " is no registered " +
                                       "folder", 1, "chrome://" +
                                       "playlistfolders/content/" +
                                       "preferences.js");
    } catch (e){
      playlistfolders.central.logEvent("preferences",
                                       "Getting folder " + guid + " failed:" +
                                       "\n\n" + e, 1, "chrome://" +
                                       "playlistfolders/content/" +
                                       "preferences.js", e.lineNumber);
    }
    return null;
  },
  
  /* Return true, if given GUID belongs to a folder
   * guid (string): guid to check
   */
  isFolder: function(guid){
    // All GUIDs wrapped in {} are folders, currently
    return guid[0] == "{";
  },
  
  /* Return true, if given GUID belongs to a playlist
   * guid (string): guid to check
   */
  isPlaylist: function(guid){
    // All GUIDS not wrapped in {} are playlists, currently
    return guid[0] != "{";
  },
  
  // Return false if we should ask the user before deleting folders
  isDeleteBypass: function(){
    return this._prefs.getBoolPref("bypassdeletewarning");
  },
  
  // Set the "do not ask me again" flag
  doDeleteBypass: function(){
    this._prefs.setBoolPref("bypassdeletewarning", true);
  },
  
  /* Get the internal event logging level for Playlist Folders, return values:
   *   0: do not log anything
   *   1: log only fatal errors
   *   2: log all errors (default)
   *   3: log all errors and warnings
   *   4: log all errors, warnings and essential debug messages
   *   5: log everything
   */
  getEvtLogLevel: function(){
    /* We can't use _prefs here because this function might be called before
     * onLoad, hence we might want to log things before the preferences are up.
     */
    return Components.classes["@mozilla.org/preferences-service;1"].
           getService(Components.interfaces.nsIPrefService).
           getBranch("extensions.playlistfolders.").getIntPref("debugLevel");
  },
};