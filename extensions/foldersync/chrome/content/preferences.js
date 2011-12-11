/* preferences.js
 * This will be loaded into the main window, and manage all preference-
 * related tasks.
 */
// Make a namespace.
if (typeof foldersync == 'undefined') {
  var foldersync = {};
};

/* Preference controller
 * Controller for all preference-related stuff, like Profiles, saved Folders,
 * preference migration, etc.
 */
foldersync.preferences={
  // FolderSync's preference branch
  _prefs: null,
  /* The JSON object stored in a preference
   * Structure:
   * {
   *   version: <string>, // version for migrating issues. See _migrateIfNeeded
   *   profiles: [<profiles>], // the profiles, see createProfile for details
   *   favorites: [<favorites], // the favorites, see createFavorite
   *   defaulttags: {<strings>}, // the default value for each tag
   *   ui: {
   *     notifications: {
   *       isEnabled: <bool>, // if there should be global notifications
   *       onlyExclusive: <bool>, // if global notifications are exclusive
   *     },
   *     show: {
   *       favorites: <bool>, // if favorite UI elements should get displayed
   *       help: <bool>, // if help group should get displayed
   *     },
   *     lastUI: {<things>}, // the last UI data, see accesspane.js
   *   },
   * }
   */
  _root: null,
  
  // Startup code for the preference controller
  onLoad: function(e){
    foldersync.central.logEvent("preferences", "Perference controller " +
                                "initialisation started.", 5);
    try {
      // Get the preferences
      var prefs = Components.classes["@mozilla.org/preferences-service;1"].
                    getService(Components.interfaces.nsIPrefService);
      this._prefs = prefs.getBranch("extensions.FolderSync.");
      // Get JSON
      try {
        this._root = foldersync.JSON.parse(this._prefs.getCharPref("JSON"));
      } catch (e) {
        this._root = {};
        foldersync.central.logEvent("preferences",
                                    "extensions.FolderSync.JSON is invalid. " +
                                    "Fallback to default value.", 2,
                                    "chrome://foldersync/content/" +
                                    "preferences.js",
                                    e.lineNumber);
      }
      // Perform migration if needed
      this._migrateIfNeeded();
      // We're done.
      foldersync.central.logEvent("preferences",
                                  "Preference controller started.", 4);
    } catch (e) {
      foldersync.central.logEvent("preferences",
                                  "Startup of Preference controller " +
                                  "failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
  },
  // Shutdown code for the preference controller
  onUnload: function(e){
    foldersync.central.logEvent("preferences", "Preference controller " +
                                "shutdown started.", 5);
    // Remove temp profiles
    for each (var profile in this._root.profiles)
      if (profile.temporary)
        this.removeProfile(profile);
    // Save JSON
    this._saveCurrent();
    // We're done!
    foldersync.central.logEvent("preferences",
                                "Preference controller stopped.", 4);
  },
  
  // Migrates JSON preferences (_root), if needed
  _migrateIfNeeded: function(){
    /* FolderSync JSON Migration Structure
     * For each FolderSync version with new/changed entrys in JSON there is a
     * JSON version string (3.0.0.0 -> "3.0.0.0"). But not every FolderSync
     * version needs a separate JSON version (so in 3.0.0.0b2 the current JSON
     * version might be "3.0.0.0b1"). Final versions will have a non-beta
     * version, so 3.0.0.0 can't get "3.0.0.0b1", although nothing changed.
     * The migration will be performed in steps, if there are more than one
     * JSON version between the saved preference and the installed release.
     * Alpha and Beta Versions might have a own JSON version WITHOUT migration
     * code needed (so there might be no migration from 3.0.0.0a1 to 3.0.0.0b1)
     * There will be migration code for public beta releases in following
     * releases with the same version number, but taken out in next releases.
     */
    // Migration from FolderSync < 3.0.0.0 OR fresh install
    if (!this._root.version){
      foldersync.central.logEvent("preferences", "Migration from FolderSync " +
                                  "< 3.0.0.0 / fresh install", 4);
      this._root.version = "3.0.0.0";
      
      // Create initial structure (with a default profile)
      this._root.profiles = [];
      this._root.favorites = [];
      this._root.defaulttags = {
        artist: "unknown",
        title: "unknown",
        album: "unknown",
        albumartist: "unknown",
        discnumber: "0",
        tracknumber: "0",
        genre: "Other",
        rating: "0",
        year: "0000",
        disccount: "0",
        composer: "unknown",
        producer: "unknown"
      };
      this._root.ui = {
        notifications: {
          isEnabled: true,
          onlyExclusive: true,
        },
        show: {
          favorites: true,
          help: true,
        },
      };
      this.createProfile(foldersync.central.
                              getLocaleString("profile.defaultname"));
                              
      // Detect < 3.0.0.0
      var migrateFromOld = false;
      try {
        migrateFromOld = !this._prefs.getBoolPref("firstrun");
      } catch (e){} // Drop exception if we haven't got a prior version
      if (migrateFromOld){    
        foldersync.central.logEvent("preferences", "FolderSync < 3.0.0.0 " +
                                  "detected.", 5);
        // Migrate defaulttags, if there are any
        try { // 2.4.0.0
          this._root.defaulttags.artist = this._prefs.
                                 getCharPref("defaultTag.artist");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.title = this._prefs.
                                 getCharPref("defaultTag.title");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.album = this._prefs.
                                 getCharPref("defaultTag.album");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.albumartist = this._prefs.
                                 getCharPref("defaultTag.albumartist");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.discnumber = this._prefs.
                                 getCharPref("defaultTag.discnumber");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.tracknumber = this._prefs.
                                 getCharPref("defaultTag.tracknumber");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.genre = this._prefs.
                                 getCharPref("defaultTag.genre");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.rating = this._prefs.
                                 getCharPref("defaultTag.rating");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.year = this._prefs.
                                 getCharPref("defaultTag.year");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.disccount = this._prefs.
                                 getCharPref("defaultTag.disccount");
        } catch (e){} // Drop exception if we haven't got those tags
        try { // 2.4.2.0
          this._root.defaulttags.composer = this._prefs.
                                 getCharPref("defaultTag.composer");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          this._root.defaulttags.producer = this._prefs.
                                 getCharPref("defaultTag.producer");
        } catch (e){} // Drop exception if we haven't got those tags
        
        // Migrate default profile
        var dProfile = this.getProfiles()[0];
        try { // 1.0.0.0
          dProfile.structure.isEnabled = this._prefs.getBoolPref("naming");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          dProfile.structure.schema = this._prefs.getCharPref("namingString");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          dProfile.playlists.isEnabled = this._prefs.getBoolPref("writeM3U");
        } catch (e){} // Drop exception if we haven't got this pref
        try { // 1.3.0.0
          dProfile.flags.doDelete = true; // We had another default value
          dProfile.flags.doDelete = this._prefs.getBoolPref("enabledelete");
        } catch (e){} // Drop exception if we haven't got this pref
        try { // 1.3.1.0
          dProfile.structure.tnDigits = this._prefs.getIntPref("trackdigits");
        } catch (e){} // Drop exception if we haven't got this pref
        try { // 1.4.2.0
          dProfile.playlists.splitChar = this._prefs.getCharPref("SplitChar");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          dProfile.playlists.doRelativePoint = this._prefs.
                                                    getBoolPref("doPointSlash");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          dProfile.playlists.format = this._prefs.getBoolPref("doExtendedM3U") ?
                                      "m3uEx" : "m3u";
        } catch (e){} // Drop exception if we haven't got this pref
        try { // 1.4.3.2
          var blChars = this._prefs.getCharPref("badcharacters").split("#");
          dProfile.advanced.blockedChars = "";
          for each (var c in blChars)
            dProfile.advanced.blockedChars += c;
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          dProfile.advanced.cutReplaced = this._prefs.getBoolPref("cutoff");
        } catch (e){} // Drop exception if we haven't got this pref
        try { // 1.4.4.0
          dProfile.flags.doUpdate = this._prefs.getBoolPref("ChangeCheck");
        } catch (e){} // Drop exception if we haven't got this pref
        try { // 2.3.0.0
          dProfile.structure.doCovers = this._prefs.getBoolPref("writecover");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          dProfile.structure.coverSchema = this._prefs.
                                                getCharPref("coverfolder");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          dProfile.structure.coverFile = this._prefs.
                                              getCharPref("coverfilename");
        } catch (e){} // Drop exception if we haven't got those tags
        try {
          dProfile.playlists.toFolder = this._prefs.getCharPref("m3ufolder");
        } catch (e){} // Drop exception if we haven't got this pref
        try { // 2.5.0.0
          dProfile.flags.doRockbox = this._prefs.getBoolPref("RockboxSyncBack");
        } catch (e){} // Drop exception if we haven't got this pref
        
        // Migrate other profiles
        try {
          var oldProfiles = this._prefs.getCharPref("Profiles").split(";?&?;");
          if (oldProfiles[0] == "")
            throw "No profile in there";
          for each (var oProfile in oldProfiles){
            var pParts = oProfile.split(";!&!;");
            // Create profile for old data
            var profile = this.createProfile(unescape(pParts[0]));
            
            // Playlist prefs weren't part of profile...
            profile.playlists.splitChar = dProfile.playlists.splitChar;
            profile.playlists.doRelativePoint = dProfile.playlists.
                                                         doRelativePoint;
            profile.playlists.format = dProfile.playlists.format;
            
            // Advanced prefs weren't part of profile...
            profile.advanced.cutReplaced = dProfile.advanced.cutReplaced;
            profile.advanced.blockedChars = dProfile.advanced.blockedChars;
            
            // Copy data
            profile.structure.schema = unescape(pParts[1]);
            profile.structure.isEnabled = unescape(pParts[2]) == "true";
            profile.playlists.isEnabled = unescape(pParts[3]) == "true";
            profile.flags.doDelete = unescape(pParts[4]) == "true";
            profile.structure.tnDigits = unescape(pParts[5])*1;
            profile.flags.doUpdate = unescape(pParts[6]) == "true";
            if (pParts.length == 7)
              continue;
            profile.structure.doCovers = unescape(pParts[7]) == "true";
            profile.structure.coverSchema = unescape(pParts[8]);
            profile.structure.coverFile = unescape(pParts[9]);
            profile.playlists.toFolder = unescape(pParts[10]);
            if (pParts.length == 11)
              continue;
            profile.flags.doRockbox = unescape(pParts[11]) == "true";
          }
        } catch (e){} // Drop exception if we haven't got profiles
        
        // Migrate favorites
        try {
          var sString = this._prefs.getCharPref("selections");
          var sParts = sString.split(";!&");
          for each (var part in sParts){
            // Migrate Data
            var parts = part.split("!;&");
            var pParts = parts[1].split("&!;");
            var plists = [];
            for each (var plist in pParts)
              if (plist != "")
                plists.push(plist);
            var pName = parts.length > 2 ? parts[2] : null;
            var profiles = this.getProfiles();
            var pGUID = dProfile.GUID;
            for each (var profile in profiles)
              if (profile.name == pName)
                pGUID = profile.GUID;
            // Add migrated data to Favorite list
            var sync = foldersync.sync.generateSync(parts[0], pGUID, plists);
            this.createFavorite(parts[0], sync);
          }
        } catch(e){} // Drop exception if we haven't got favorites
        
        // Migrate UI preferences
        var ui = this.getUIPrefs();
        // Show help / preference button -> show help area
        var sHelp = ui.show.help;
        try {
          sHelp = this._prefs.getBoolPref("showHelpLink");
        } catch(e){} // Drop exception if we haven't got this pref
        try{
          sHelp = sHelp || this._prefs.getBoolPref("showPrefBtn");
        } catch(e){} // Drop exception if we haven't got this pref
        ui.show.help = sHelp;
        // Last shown UI
        ui.lastUI = {
          target: "",
          playlists: [],
          profile: dProfile,
        };
        try {
          ui.lastUI.target = this._prefs.getCharPref("lastfolder");
        } catch(e){} // Drop exception if we haven't got this pref
        try {
          var parts = this._prefs.getCharPref("lastselection").split("&!;");
          for each (var part in parts)
            if (part != "")
              ui.lastUI.playlists.push(part);
        } catch(e){} // Drop exception if we haven't got this pref
        try {
          var pName = this._prefs.getCharPref("lastProfileName");
          var profiles = this.getProfiles();
          for each (var profile in profiles)
            if (profile.name == pName)
              ui.lastUI.profile = profile;
        } catch(e){} // Drop exception if we haven't got this pref
      }
    }
    
    // Migration from FolderSync 3.0.0.0 to 3.0.2.0
    if (this._root.version == "3.0.0.0"){
      foldersync.central.logEvent("preferences", "Migration from 3.0.0.0 " +
                                  "to 3.0.2.0", 5);
      this._root.version = "3.0.2.0";
      // Move lastUI profiles to normal profile list, and set its GUID
      if (this._root.ui.lastUI){
        var profile = this.getProfileByGUID(this._root.ui.lastUI.profile.GUID);
        // Not a user defined profile?
        if (!profile){
          profile.temporary = false;
          this._root.profiles.push(profile);
        }
        this._root.ui.lastUI.profile = profile.GUID;
      }
      // Migrate profiles
      for each (var profile in this.getProfiles()){
        profile.playlists.isSorted = false;
        profile.playlists.sortingScheme = "%albumartist:a%,%album%,%tracknumber%,%title%";
      }
    }
    // Migration from FolderSync 3.0.2.0 to 3.0.3.0
    if (this._root.version == "3.0.2.0"){
      foldersync.central.logEvent("preferences", "Migration from 3.0.2.0 " +
                                  "to 3.0.3.0", 5);
      this._root.version = "3.0.3.0";
      // Add Fallback for album artist
      this._root.fallbacks = {
        albumartist: true,
      };
    }
    // If we weren't able to migrate, log a fatal error message and reset
    // everything.
    if (this._root.version != "3.0.3.0"){
      foldersync.central.logEvent("preferences", "There is no rule to " +
                                  "migrate from JSON version " +
                                  this._root.version + ". Fallback to empty " +
                                  "JSON root.", 2, "chrome://" +
                                  "foldersync/content/preferences.js");
      this._root = {};
      this._migrateIfNeeded();
    }
  },
  
  // Saves current JSON (_root) to preferences
  _saveCurrent: function(){
    try {
      foldersync.central.logEvent("preferences",
                                  "Save JSON preferences", 5);
      this._prefs.setCharPref("JSON", foldersync.JSON.stringify(this._root));
    } catch (e){
      foldersync.central.logEvent("preferences",
                                  "Saving of JSON preferences failed:\n\n" + e,
                                  1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
  },
  
  // Returns the array of all Profiles
  getProfiles: function(){
    try {
      return this._root.profiles;
    } catch (e){
      foldersync.central.logEvent("preferences",
                                  "Getting Profile list failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
    return null;
  },
  
  /* Returns the profile with the given GUID
   * guid (string): the GUID of the requested Profile
   */
  getProfileByGUID: function(guid){
    try {
      for each (var profile in this._root.profiles){
        if (profile.GUID == guid)
          return profile;
      }
      foldersync.central.logEvent("preferences",
                                  "Getting Profile " + guid + " failed:\n\n" +
                                  guid + " is no registered profile", 1,
                                  "chrome://foldersync/content/" +
                                  "preferences.js");
    } catch (e){
      foldersync.central.logEvent("preferences",
                                  "Getting Profile " + guid + " failed:\n\n" +
                                  e, 1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
    return null;
  },
  
  /* Generate a new Profile
   * name (string): the name of the profile
   */
  createProfile: function(name){
    // generate a profile with the given name and the default values
    var result = {
      name: name, // The profile's name
      GUID: Components.classes["@mozilla.org/uuid-generator;1"].
            getService(Components.interfaces.nsIUUIDGenerator).generateUUID().
            toString(), // A unique ID for referencing purposes
      temporary: false, // set this to true if the profile should get deleted
      visible: true, // visible to user?
      
      flags: {
        doDelete: false, // Delete files that aren't part of the sync
        doUpdate: true, // Overwrite files that changed since last sync
        doRockbox: false, // Do a Rockbox back-sync
      },
      
      structure: {
        isEnabled: false, // Use filename scheme
        schema: "%artist%/%album%/%title%", // The filename scheme
        tnDigits: 0, // The minimal number of digits
        
        doCovers: false, // Write Album cover
        coverSchema: "%album%", // The schema of the folder covers go to
        coverFile: "cover", // The filename of covers ("cover"->"cover.jpg")
      },
      
      playlists: {
        isEnabled: false, // Write M3U playlists
        toFolder: ".", // The relative folder playlists go to
        
        splitChar: "/", // The split character in file paths
        doRelativePoint: true, // Start a relative path with .[splitChar]
        
        format: "m3uEx", // Playlist format, see foldersync.sync.getPFormats
        encoding: "UTF-8", // The encoding (for nsIConverterOutputStream)
        
        isSorted: false, // Sort playlists' content
        sortingScheme: "%albumartist:a%,%album%,%tracknumber%,%title%", // Sorting scheme 
      },
      
      advanced: {
        blockedChars: '/\\:*?><|".', // Characters that will get replaced
        replaceChar: "_", // Character to use inserted instead of a blocked one
        cutReplaced: true, // Cut off replace characters on the end of a tag
        cutSpaces: true, // Cut off spaces at beginning/end of a file/folder
        
        fMaxLength: 150, // Maximum length for a file name
        
        compareCase: false, // see up- and downcase as different folders
      },
    };
    // append and return the profile
    foldersync.central.logEvent("preferences",
                                "Create Profile "+result.GUID, 5);
    try {
      this._root.profiles.push(result);
      return result;
    } catch (e){
      foldersync.central.logEvent("preferences",
                                  "Adding of profile failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
    return null;
  },
  
  /* Removes the given Profile from the profile list
   * profile (profile): the profile to remove
   */
  removeProfile: function(profile){
    try {
      foldersync.central.logEvent("preferences",
                                  "Remove Profile "+profile.GUID, 5);
      var deleted = false;
      for (var i = 0; i < this._root.profiles.length; i++){
        if (this._root.profiles[i] == profile){
          this._root.profiles.splice(i,1);
          deleted = true;
        }
      }
      if (!deleted)
        foldersync.central.logEvent("preferences",
                                    "Removing of profile failed:\n\n" +
                                    profile.GUID + " is no registered" +
                                    " profile", 3, "chrome://" +
                                    "foldersync/content/preferences.js");
    } catch (e){
      foldersync.central.logEvent("preferences",
                                  "Removing of profile failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
  },
  
  // Returns the array of all Favorites
  getFavorites: function(){
    try {
      return this._root.favorites;
    } catch (e){
      foldersync.central.logEvent("preferences",
                                  "Getting Favorite list failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
    return null;
  },
  
  /* Returns the favorite with the given GUID
   * guid (string): the GUID of the requested Favorite
   */
  getFavoriteByGUID: function(guid){
    try {
      for each (var fav in this._root.favorites){
        if (fav.GUID == guid)
          return fav;
      }
      foldersync.central.logEvent("preferences",
                                  "Getting Favorite " + guid + " failed:\n\n" +
                                  guid + " is no registered Favorite", 1,
                                  "chrome://foldersync/content/" +
                                  "preferences.js");
    } catch (e){
      foldersync.central.logEvent("preferences",
                                  "Getting Favorite " + guid + " failed:\n\n" +
                                  e, 1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
    return null;
  },
  
  /* Generate a new Sync Fav
   * name (string): the favorite's name
   * sync (sync): a sync (see foldersync.sync.generateSync)
   */
  createFavorite: function(name,sync){
    // generate a fav with the given attributes
    var result = {
      GUID: Components.classes["@mozilla.org/uuid-generator;1"].
            getService(Components.interfaces.nsIUUIDGenerator).generateUUID().
            toString(), // A unique ID for referencing purposes
      name: name, // The Favorite's name
      sync: sync, // The Favorite's sync
    };
    // append and return the fav
    foldersync.central.logEvent("preferences",
                                "Create Favorite '" + result.name + "'", 5);
    try {
      this._root.favorites.push(result);
      return result;
    } catch (e){
      foldersync.central.logEvent("preferences",
                                  "Adding of favorite failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
    return null;
  },
  
  /* Removes the given Favorite from the favorites list
   * favorite (favorite): the favorite to remove
   */
  removeFavorite: function(favorite){
    try {
      foldersync.central.logEvent("preferences",
                                  "Remove Favorite "+favorite.name, 5);
      var deleted = false;
      for (var i = 0; i < this._root.favorites.length; i++){
        if (this._root.favorites[i] == favorite){
          this._root.favorites.splice(i,1);
          deleted = true;
        }
      }
      if (!deleted)
        foldersync.central.logEvent("preferences",
                                    "Removing of favorite failed:\n\n" +
                                    "'" + favorite.name + "' is no " +
                                    "registered favorite", 3, "chrome://" +
                                    "foldersync/content/preferences.js");
    } catch (e){
      foldersync.central.logEvent("preferences",
                                  "Removing of favorite failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/preferences.js",
                                  e.lineNumber);
    }
  },
  
  /* Get the internal event logging level for FolderSync, return values:
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
           getBranch("extensions.FolderSync.").getIntPref("debugLevel");
  },
  
  // Get the array of default tags
  getDefaultTags: function(){
    return this._root.defaulttags;
  },
  
  // Get the ui preference root
  getUIPrefs: function(){
    return this._root.ui;
  },
  
  // Get the array of fallbacks
  getFallbacks: function(){
    return this._root.fallbacks;
  },
};