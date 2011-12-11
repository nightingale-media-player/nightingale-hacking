/* central.js
 * This will be loaded into the main window, and manage all global FolderSync-
 * related tasks that do not fit into another file / controller.
 */
// Make a namespace.
if (typeof foldersync == 'undefined') {
  var foldersync = {};
};

/* Central controller
 * Controller for central issues that doesn't fit to any of the controllers
 * above, like debug messages and UI-independent localisation. Also controlls
 * startup (calls other onLoads) and shutdown (calls other onUnloads), and
 * UI integration.
 */
foldersync.central={
  // central.properties DOM node
  _strings: null,
  // the OS we're running on
  _OS: null,
  // the number of UIs connected
  _numUI: 0,
  // weather we show notifications currently
  _showNotifications: false,
  
  // The central UI update listener for the sync controller
  _listener: function(state, object) {
    if (state.event == "started")
      foldersync.central._updateSPNode(true);
    var error = foldersync.sync.generateErrorMessage(state);
    if ((state.event == "completed") ||
        (state.event == "cancelled") ||
        error){
      foldersync.central._updateSPNode(false);
      
      // show a global notification that the sync is completed, if pref set
      if (!foldersync.central._showNotifications)
        return;
      var msg = error ? error.error : foldersync.central.
                                      getLocaleString("status.completed");
      msg += ": " + object.sync.targetFolder;
      var gNotify = document.getElementById("application-notificationbox");
      if (!gNotify){
        this.logEvent("central", "Application's notificationbox is not " +
                      "avaiable", 1, "chrome://foldersync/content/central.js");
        return;
      }
      var level = error ? (error.fatal ? 8 : 5) : 2;
      gNotify.appendNotification(msg,
                                 level,
                                 "chrome://foldersync/skin/node.png",
                                 level,
                                 error ? [{
                                            accessKey: null,
                                            callback: function(){
                                                foldersync.central.showDialog(
                                                  error.error, error.message);
                                              },
                                            label: foldersync.central.
                                                     getLocaleString(
                                                       "notification.details"),
                                            popup: null,
                                          }] : []);
    }
  },
  
  // Startup code for the central controller
  onLoad: function(e){
    foldersync.central.logEvent("central", "Central controller " +
                                "initialisation started.", 5);
    try {
      // Get OS
      this._OS = window.navigator.platform;
      // Get strings
      this._strings = document.getElementById("foldersync-central-strings");
      
      // Load the other global controllers
      foldersync.preferences.onLoad(e);
      foldersync.sync.onLoad(e);
      
      // Set up service pane
      this._setupServicePaneNode();
      
      // Check weather our updater displays notifications
      var UIprefs = foldersync.preferences.getUIPrefs();
      this._showNotifications = UIprefs.notifications.isEnabled;
      
      // Register our updater
      foldersync.sync.addListener(this._listener);
      
      // We're done!
      foldersync.central.logEvent("central", "Central controller started.", 4);
    } catch (e){
      foldersync.central.logEvent("central",
                                  "Startup of Central controller failed:\n\n" +
                                  e, 1,
                                  "chrome://foldersync/content/central.js",
                                  e.lineNumber);
    }
  },
  // Shutdown code for the central controller
  onUnload: function(e){
    foldersync.central.logEvent("central", "Central controller " +
                                "shutdown started.", 5);
    // Remove the updater for our node
    foldersync.sync.removeListener(this._listener);
    // Unload other global controllers
    foldersync.preferences.onUnload(e);
    foldersync.sync.onUnload(e);
    // We're done!
    foldersync.central.logEvent("central", "Central controller stopped.", 4);
  },
  
  /* Post event in error console, if logging of event type is turned on
   * module (string): what module caused the event (for example: "central")
   * msg (string): more detailed description what happened
   * type (int): What event type the error is:
   *   1: fatal error
   *   2: error
   *   3: warning
   *   4: essential debug message (for example startup/shutdown of controllers)
   *   5: debug message (for example a single song being synced)
   * srcFile (string): The source file's URL, or null (errors and warnings)
   * srcLine (int): The event's line number, or null (errors and warnings)
   */
  logEvent: function(module, msg, type, srcFile, srcLine){
    // Drop messages for that logging is disabled
    if (foldersync.preferences.getEvtLogLevel() < type)
      return;
    // Parse message
    var msgStr = "FolderSync: Event raised in '" + module + "':\n" + msg;
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
                   typeof srcLine != 'number' ? null : srcLine,
                   null,
                   type != 3 ? 0x0 /*errorFlag*/ : 0x1 /*warningFlag*/,
                   null);
      consoleService.logMessage(message);
      break;
    default:
      consoleService.logStringMessage(msgStr);
    }
  },
  
  /* Get a specific localisation
   * string (string): the name of the string
   */
  getLocaleString: function(string){
    try {
      return this._strings.getString(string);
    } catch (e){
      this.logEvent("central", "Getting locale string '"+string+"' failed:" +
                    "\n\n" + e, 1, "chrome://foldersync/content/central.js",
                    e.lineNumber);
    }
  },
  
  /* Get the main library, described as
   * {
   *   name:<string>, // The library's name
   *   guid:<string>, // The library's guid
   * }
   */
  getMainLibaray: function(){
    foldersync.central.logEvent("central", "Get main library", 4);
    // Get the library
    var sbILibraryManager = Components.classes
                            ["@getnightingale.com/Nightingale/library/Manager;1"].
                            getService(Components.interfaces.
                                       sbILibraryManager);
    var libraries=sbILibraryManager.getLibraries();
    var mLibrary = sbILibraryManager.mainLibrary;
    // return it
    return {name: mLibrary.name, guid: mLibrary.guid};
  },
  
  /* Get a array of Playlists, each playlist described as
   * {
   *   name:<string>, // The name the user entered
   *   guid:<string>, // The playlist's GUID
   * }
   * onlyMain (boolean): Only playlists avaiable in the main library
   * onlyEditable (boolean): Only playlists that have the UserEditable flag
   */
  getPlaylists: function(onlyMain, onlyEditable){
    foldersync.central.logEvent("central", "Get playlists:\n" +
                                "in main library only: " + onlyMain + "\n" +
                                "editable by user only: " + onlyEditable, 4);
    // We'll store the result here
    var result = [];
    // Get the libraries
    var sbILibraryManager = Components.classes
                            ["@getnightingale.com/Nightingale/library/Manager;1"].
                            getService(Components.interfaces.
                                       sbILibraryManager);
    var libraries=sbILibraryManager.getLibraries();
    var crtLibrary = sbILibraryManager.mainLibrary;
    
    try {
      while (crtLibrary){
        // Write the library's MediaList to the result
        result.push({name: crtLibrary.name, guid:crtLibrary.guid});
        foldersync.central.logEvent("central", "Found Library:\n" +
                                    "Name: " + crtLibrary.name + "\n" +
                                    "GUID: " + crtLibrary.guid, 5);
        // Get all other MediaLists and write them to the result
        var libView = crtLibrary.createView();
        var libFilters = libView.cascadeFilterSet;
        libFilters.clearAll;
        libFilters.appendSearch(new Array("*"), 1);
        libFilters.appendFilter("http://getnightingale.com/data/1.0#isList");
        libFilters.set(1, new Array("1"), 1);
        for (var i = 0; i < libView.length; i++){
          try {
            var pList=libView.getItemByIndex(i);
            // Use playlists with name only, do not use smart ones, they have a
            // simple "second identity" :P
            if (pList.name && (pList.name != "") && (pList.name != " ") &&
                (pList.type != "smart")){
              result.push({name: pList.name, guid:pList.guid});
              foldersync.central.logEvent("central", "Found Playlist:\n" +
                                          "Name: " + pList.name + "\n" +
                                          "GUID: " + pList.guid, 5);
            }
          } catch (e) {
            this.logEvent("central", "Getting playlist failed:\n\n" +
                      e, 2, "chrome://foldersync/content/central.js",
                      e.lineNumber);
          }
        }
        
        // Get the next library, if we want one
        if (onlyMain || (!libraries.hasMoreElements()))
          crtLibrary = null;
        else
          crtLibrary = libraries.getNext();
      }
    } catch (e){
      this.logEvent("central", "Getting playlists failed:\n\n" +
                    e, 1, "chrome://foldersync/content/central.js",
                    e.lineNumber);
    }
    return result;
  },
  
  /* Get the playlist object with the given GUID or null if there is none
   * guid (string): The guid
   */
  getPlaylistByGUID: function(guid){
    foldersync.central.logEvent("central", "Search for playlist " + guid, 4);
    // We'll store the result here
    var result = null;
    // Get the libraries
    var sbILibraryManager = Components.classes
                            ["@getnightingale.com/Nightingale/library/Manager;1"].
                            getService(Components.interfaces.
                                       sbILibraryManager);
    var libraries=sbILibraryManager.getLibraries();
    var crtLibrary = sbILibraryManager.mainLibrary;
    
    try {
      while (crtLibrary){
        // Check the Library's MediaList
        if (crtLibrary.guid == guid)
          return crtLibrary;
        // Check all other MediaLists
        var libView = crtLibrary.createView();
        var libFilters = libView.cascadeFilterSet;
        libFilters.clearAll;
        libFilters.appendSearch(new Array("*"), 1);
        libFilters.appendFilter("http://getnightingale.com/data/1.0#isList");
        libFilters.set(1, new Array("1"), 1);
        for (var i = 0; i < libView.length; i++){
          try {
            var pList=libView.getItemByIndex(i);
            if (pList.guid == guid)
              return pList;
          } catch (e) {
            this.logEvent("central", "Searching playlist failed:\n\n" +
                      e, 2, "chrome://foldersync/content/central.js",
                      e.lineNumber);
          }
        }
        
        // Get the next library
        if (!libraries.hasMoreElements())
          crtLibrary = null;
        else
          crtLibrary = libraries.getNext();
      }
    } catch (e){
      this.logEvent("central", "Searching playlists failed:\n\n" +
                    e, 1, "chrome://foldersync/content/central.js",
                    e.lineNumber);
    }
    this.logEvent("central", "Playlist " + guid + " not found.",
                  2, "chrome://foldersync/content/central.js");
    return null;
  },
  
  // Get OS we're running on
  getOS: function(){
    return this._OS;
  },
  
  /* Displays a Error / Message dialog. See error.xul and error.js
   * error (string): A short error description
   * message (string): The (error) message itself
   */
  showDialog: function(error, message){
    foldersync.central.logEvent("central", "Open Error Dialog:\n" +
                                "Error: " + error + "\n" +
                                "Message: " + message, 4);
    try {
      window.openDialog("chrome://foldersync/content/dialogs/error.xul",
                        "foldersync-error-dialog",
                        "dialog=yes,modal=no,alwaysRaised=yes," +
                        "centerscreen=yes,resizable=yes,dependent=yes",
                        error, message);
    } catch (e){
      this.logEvent("central", "Opening Error Dialog failed:\n\n" +
                    e, 1, "chrome://foldersync/content/central.js",
                    e.lineNumber);
    }
  },
  
  // Sets the service Pane Node for FolderSync up
  _setupServicePaneNode : function() {
    foldersync.central.logEvent("central", "Setup Service Pane Node", 4);
    // The NS we need
    var dtSB_NS = 'http://getnightingale.com/data/1.0#';
    var dtSP_NS = 'http://getnightingale.com/rdf/servicepane#';
    var dtSC_NS_DT  = "http://getnightingale.com/data/1.0#";
    // Get the ServicePaneService
    var SPS = Cc["@getnightingale.com/servicepane/service;1"]
                .getService(Ci.sbIServicePaneService);
    // Check whether the node already exists
    if (SPS.getNode("SB:MainLib:FolderSync")){
      this.logEvent("central", "Wasn't able to setup Service Pane Node:\n\n" +
                    "Node exists already", 3,
                    "chrome://foldersync/content/central.js");
      return;
    }
    // Get the main library's node
    var libNode = SPS.getNode("urn:library:" + this.getMainLibaray().guid);
    if (!libNode){
      this.logEvent("central", "Wasn't able to setup Service Pane Node:\n\n" +
                    "Main Library's node not found", 1,
                    "chrome://foldersync/content/central.js");
      return;
    }

    // Create FolderSync Node
    var foldersyncNode = SPS.createNode();
    foldersyncNode.id = "SB:MainLib:FolderSync";
    foldersyncNode.url = "chrome://foldersync/content/accesspane.xul";
    var tNo = document.getElementById("foldersync-central-branding-tab-title");
    foldersyncNode.name = tNo.getAttribute("value");
    foldersyncNode.editable = false;
    foldersyncNode.setAttributeNS(dtSP_NS,
                                  "addonID",
                                  "foldersync@rsjtdrjgfuzkfg.com");
    foldersyncNode.image = 'chrome://foldersync/skin/node.png';
    libNode.appendChild(foldersyncNode);
  },
  
  /* Updates the Service Pane node, if we're working or not
   * working (bool): show the user we're working
   */
  _updateSPNode: function(working){
    // Get the ServicePaneService
    var SPS = Cc["@getnightingale.com/servicepane/service;1"]
                .getService(Ci.sbIServicePaneService);
    // Get our node
    var spNode = SPS.getNode("SB:MainLib:FolderSync");
    if (!spNode){
      this.logEvent("central", "Wasn't able to update node:\n\n" +
                    "FolderSync's node not found", 1,
                    "chrome://foldersync/content/central.js");
      return;
    }
    spNode.image = working ? 'chrome://foldersync/skin/node-working.png'
                           : 'chrome://foldersync/skin/node.png';
  },
  
  // Called by a UI (for example our tab) to say that it is running
  registerUI: function(){
    foldersync.central.logEvent("central", "Register UI", 4);
    this._numUI++;
    var UIprefs = foldersync.preferences.getUIPrefs();
    if (UIprefs.notifications.onlyExclusive)
      this._showNotifications = false;
  },
  
  // Called by a UI (for example our tab) to sync that 
  unregisterUI: function(){
    foldersync.central.logEvent("central", "Unregister UI", 4);
    this._numUI--;
    var UIprefs = foldersync.preferences.getUIPrefs();
    if ((this._numUI == 0) && UIprefs.notifications.isEnabled)
      this._showNotifications = true;
  },
};

// load / unload central controller with window
window.addEventListener("load",
                        function(e){
                          foldersync.central.onLoad(e);
                        },
                        false);
window.addEventListener("unload",
                        function(e){
                          foldersync.central.onUnload(e);
                        },
                        false);