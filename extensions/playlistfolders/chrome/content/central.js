/* central.js
 * This will be loaded into the main window, and manage all global Playlist
 * Folder related tasks.
 */
// Make a namespace.
if (typeof playlistfolders == 'undefined') {
  var playlistfolders = {};
};

/* Central controller
 * Controller for central issues, like startup and shutdown.
 */
playlistfolders.central={ 
  // Startup code for the central controller
  onLoad: function(e){
    this.logEvent("central", "Central controller initialisation started.", 5);
    try {
      // Load localisation
      var sbSvc = Cc["@mozilla.org/intl/stringbundle;1"].
                  getService(Ci.nsIStringBundleService);
      var stringBundle = sbSvc.createBundle("chrome://songbird/locale/" +
                                            "songbird.properties");
      document.getElementById("menuitem_file_folder").
               setAttribute("label",
                            stringBundle.GetStringFromName("menu.servicepane." +
                                                           "file.folder"));
      document.getElementById("menuitem_file_folder").
               setAttribute("accesskey",
                            stringBundle.GetStringFromName("menu.servicepane." +
                                                           "file.folder." +
                                                           "accesskey"));
      
      // Load the other global controllers
      playlistfolders.preferences.onLoad(e);
      playlistfolders.servicepane.onLoad(e);
      // We're done!
      this.logEvent("central", "Central controller started.", 4);
    } catch (e){
      this.logEvent("central", "Startup of Central controller failed:\n\n" + e,
                    1, "chrome://playlistfolders/content/central.js",
                    e.lineNumber);
    }
  },
  
  // Shutdown code for the central controller
  onUnload: function(e){
    this.logEvent("central", "Central controller shutdown started.", 5);
    // Unload other global controllers
    playlistfolders.preferences.onUnload(e);
    playlistfolders.servicepane.onUnload(e);
    // We're done!
    this.logEvent("central", "Central controller stopped.", 4);
  },
  
  /* Post event in error console, if logging of event type is turned on
   * module (string): what module caused the event (for example: "central")
   * msg (string): more detailed description what happened
   * type (int): What event type the error is:
   *   1: fatal error
   *   2: error
   *   3: warning
   *   4: essential debug message (for example startup/shutdown of controllers)
   *   5: debug message (for example renaming of a folder)
   * srcFile (string): The source file's URL, or null (errors and warnings)
   * srcLine (int): The event's line number, or null (errors and warnings)
   */
  logEvent: function(module, msg, type, srcFile, srcLine){
    // Drop messages for that logging is disabled
    if (playlistfolders.preferences.getEvtLogLevel() < type)
      return;
    // Parse message
    var msgStr = "Playlist Folders: Event raised in '" + module + "':\n" + msg;
    // Post message
    var consoleService = Components.classes["@mozilla.org/consoleservice;1"].
                         getService(Components.interfaces.nsIConsoleService);
    switch (type){
    case 1:
    case 2:
    case 3:
      var message = Components.classes["@mozilla.org/scripterror;1"].
                    createInstance(Components.interfaces.nsIScriptError);
      message.init(msgStr,
                   srcFile,
                   null,
                    srcLine == 0 ? null : srcLine,
                   null,
                   type != 3 ? 0x0 /*errorFlag*/ : 0x1 /*warningFlag*/,
                   null);
      consoleService.logMessage(message);
      break;
    default:
      consoleService.logStringMessage(msgStr);
    }
  },
  
  /* Deletes given Playlist without asking user, returns true if succeed
   * playlist (string): GUID of the playlist to delete
   */
  removePlaylist: function(playlist){
    this.logEvent("central", "Delete Playlist " + playlist, 5);
    var library = Cc["@songbirdnest.com/Songbird/library/Manager;1"].
                  getService(Ci.sbILibraryManager).mainLibrary;
    var pl = library.getItemByGuid(playlist);
    if (pl){
      library.remove(pl);
      return true;
    }
    this.logEvent("central", "Could not delete Playlist " + playlist + "\n\n" +
                             "Playlist not found in library", 2,
                             "chrome://playlistfolders/content/central.js");
    return true; // if we didn't find a playlist, continue deleting
  },
};

// load / unload central controller with window
window.addEventListener("load",
                        function(e){
                          playlistfolders.central.onLoad(e);
                        },
                        false);
window.addEventListener("unload",
                        function(e){
                          playlistfolders.central.onUnload(e);
                        },
                        false);