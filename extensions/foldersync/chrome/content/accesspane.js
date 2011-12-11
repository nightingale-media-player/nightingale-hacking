/* accesspane.js
 * This will be loaded into the main FolderSync control tab. It conncts to
 * all central controllers and performs all UI-related stuff in the tab.
 */
// Make a namespace.
if (typeof foldersync == 'undefined') {
  var foldersync = {};
};

/* Accesspane controller
 * Performs all UI-related stuff in the tab.
 */
foldersync.accesspane = {
  // If this is true our listeners may cancel the current sync
  _cancel: false,
  
  // The error message for the last failed sync, see foldersync.sync
  _details: null,
  
  /* Show the current box with given text / buttons
   * text (string): the current message text
   * progress (int): the current progress in % or null for undetermined
   * sync (bool): weather we're in sync or not
   * fail (bool): weather we got details to display or not
   */
  _updateCrt: function(text,progress,sync,fail){
    // Show status box
    document.getElementById("foldersync-current-box").setAttribute("style","");
    // Show cancel button in sync
    document.getElementById("foldersync-current-cancel").
             setAttribute("style",sync ? "" : "display:none");
    // Show details button on failed syncs
    document.getElementById("foldersync-current-details").
             setAttribute("style",fail ? "" : "display:none");
    // Update text / progress bar
    document.getElementById("foldersync-current-state").value = text;
    document.getElementById("foldersync-current-progress").
             setAttribute("mode",progress ? "determined" :
                                            "undetermined");
    document.getElementById("foldersync-current-progress").
             value = progress ? progress : 0;
  },
  
  // Update the queue field
  _updateQueue: function(){
    var numQueued = foldersync.sync.getQueue().length;
    if (numQueued){
      // Show queue
      document.getElementById("foldersync-queue-box").
               setAttribute("style", "");
      // Display number of queued items
      var msg = foldersync.central.getLocaleString("status.queued");
      msg = msg.replace("%d", numQueued);
      document.getElementById("foldersync-queue-state").value = msg;
    } else {
      // Hide queue
      document.getElementById("foldersync-queue-box").
               setAttribute("style", "display:none");
    }
  },
  
  // the tab listener
  _listener: function(state, object) {
    // Update queue status box
    if ((state.event == "added") || (state.event == "removed") ||
        (state.event == "started"))
      foldersync.accesspane._updateQueue();
    // Update status box
    var msg = "";
    if ((state.event == "started") ||
        (state.event == "initing")){
      msg = foldersync.central.getLocaleString("status.init");
      msg += ": " + object.sync.targetFolder;
      foldersync.accesspane._updateCrt(msg, null, true, false);
    }
    if ((state.event == "started-sync")||
        (state.event == "status-update")){
      msg = foldersync.central.getLocaleString("status.sync");
      msg += ": " + object.sync.targetFolder;
      foldersync.accesspane._updateCrt(msg, state.value, true, false);
    }
    if (state.event == "completed"){
      msg = foldersync.central.getLocaleString("status.completed");
      msg += ": " + object.sync.targetFolder;
      foldersync.accesspane._updateCrt(msg, state.value, false, false);
    }
    if (state.event == "cancelled"){
      msg = foldersync.central.getLocaleString("status.cancelled");
      msg += ": " + object.sync.targetFolder;
      foldersync.accesspane._updateCrt(msg, state.value, false, false);
      // Make sure cancel button is not disabled, if we cancelled
      document.getElementById("foldersync-current-cancel").
                 setAttribute("disabled","false");
      // Make sure it does not look like we're working
      document.getElementById("foldersync-current-progress").
               setAttribute("mode", "determined");
    }
    var error = foldersync.sync.generateErrorMessage(state);
    if (error){
      msg = error.error + ": " + object.sync.targetFolder;
      foldersync.accesspane._updateCrt(msg, 100, false, true);
      // Save error details
      foldersync.accesspane._details = error;
    }
    
    // Cancel if user set cancel
    if (foldersync.accesspane._cancel)
      if ((state.event == "started-sync") ||
          (state.event == "status-update")){
        object.cancel = true;
        foldersync.accesspane._cancel = false;
      }
    
    // Show notification above the "normal" status if we finished the sync
    if (!(error || (state.event == "completed")))
      return;
    var fNotify = document.getElementById("foldersync-current-notification");
    if (!fNotify)
      return;
    var level = error ? (error.fatal ? 8 : 5) : 2;
    fNotify.appendNotification(msg,
                               level,
                               "chrome://foldersync/skin/node.png",
                               level,
                               error ? [{
                                          accessKey: null,
                                          callback: function(){
                                              foldersync.accesspane.
                                                         showDetails(error);
                                            },
                                          label: foldersync.central.
                                                   getLocaleString(
                                                     "notification.details"),
                                          popup: null,
                                        }] : []);
  },
  
  // Startup code for the tab
  onLoad: function(e){
    foldersync.central.logEvent("accesspane", "Accesspane controller " +
                                "initialisation started.", 5);
    // Register tab listener
    foldersync.sync.addListener(this._listener);
    // Refister at central controller
    foldersync.central.registerUI();
    // Get UI prefs
    var ui = foldersync.preferences.getUIPrefs();
    // Helper for hiding XUL elements
    var hide = function(id){
      document.getElementById(id).setAttribute("style","display:none");
    };
    if (ui.show.favorites)
      // Init the Favorite list
      this.favorite._init();
    else {
      // Hide Favorite list
      hide("foldersync-favorite-box");
      hide("foldersync-direct-add");
      hide("foldersync-direct-update");
    }
    if (!ui.show.help)
      // Hide help section
      hide("foldersync-help-box");
    // Init the Profile list
    this.profile._init();
    // Init the Playlist list
    this.playlists._init();
    // Load data from prior session, if any
    if (ui.lastUI){
      this.target.setFolder(ui.lastUI.target);
      this.playlists.setGUIDs(ui.lastUI.playlists);
      var profile = foldersync.preferences.getProfileByGUID(ui.lastUI.profile);
      if (profile)
        this.profile.setProfile(profile);
      else
        foldersync.central.logEvent("accesspane",
                                    "There is no valid Profile avaiable " +
                                    "for last UI. Skip Profile restore. ", 2,
                                    "chrome://foldersync/content/" +
                                    "accesspane.js");
    }
    // Show the user the sync thread is running, if so
    if (foldersync.sync.isSyncing()){
      // Display sync box, without text and without determined progress
      this._updateCrt("", null, true, false);
      // Update queue status
      this._updateQueue();
    }
    foldersync.central.logEvent("accesspane",
                                "Accesspane controller started.", 4);
  },
  
  // Shutdown code for the tab
  onUnload: function(e){
    foldersync.central.logEvent("accesspane", "Accesspane controller " +
                                "shutdown started.", 5);
    // Remove tab listener
    foldersync.sync.removeListener(this._listener);
    
    // Store UI data for next time the user opens the tab
    var ui = foldersync.preferences.getUIPrefs();
    if (!ui.lastUI)
      ui.lastUI = {};
    // delete old temporary profile if there was a custom profile last time
    var oProfile = foldersync.preferences.getProfileByGUID(ui.lastUI.profile);
    if (oProfile && !oProfile.visible)
      foldersync.preferences.removeProfile(oProfile);
    // Save data
    ui.lastUI.target = this.target.getFolder();
    ui.lastUI.playlists = this.playlists.getGUIDs();
    ui.lastUI.profile = this.profile.getGUID();
    foldersync.preferences.
               getProfileByGUID(ui.lastUI.profile).temporary = false;
    
    // Unregister at central controller
    foldersync.central.unregisterUI();
    foldersync.central.logEvent("accesspane",
                                "Accesspane controller stopped.", 4);
  },
  
  // Cancel current sync
  cancelSync: function(){
    this._cancel = true;
    document.getElementById("foldersync-current-cancel").
             setAttribute("disabled","true");
  },
  
  // Cancels the whole sync queue
  cancelQueue: function(){
    try {
      var queue = foldersync.sync.getQueue();
      for each (var e in queue)
        foldersync.sync.removeFromQueue(e);
    } catch (e){
      foldersync.central.logEvent("accesspane",
                                  "Cancelling queue failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/accesspane.js",
                                  e.lineNumber);
    }
  },
  
  /* Shows details on error
   * error (error message): the error message to show, or null for the last one
   */
  showDetails: function(error){
    var aError = error;
    if (!aError)
      aError = this._details;
    if (aError)
      foldersync.central.showDialog(aError.error,
                                    aError.message);
    else
      foldersync.central.logEvent("accesspane",
                                  "showDetails has no details to show", 2,
                                  "chrome://foldersync/content/accesspane.js");
  },
  
  // Start a Sync with currently entered values
  startSync: function(){
    try {
      var sync = this.getSync();
      foldersync.sync.performSync(sync);
    } catch(e){
      foldersync.central.logEvent("accesspane",
                                  "Starting Sync failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/accesspane.js",
                                  e.lineNumber);
    }
  },
  
  // Get the current data as sync object
  getSync: function(){
    try {
      return foldersync.sync.generateSync(this.target.getFolder(),
                                          this.profile.getGUID(),
                                          this.playlists.getGUIDs());
    } catch(e){
      foldersync.central.logEvent("accesspane",
                                  "Generating Sync failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/accesspane.js",
                                  e.lineNumber);
    }
  },
  
  // All profile related things
  profile: {
    // displays profile preferences
    show: function(){
      document.getElementById("foldersync-profile-box").
               setAttribute("style", "");
      document.getElementById("foldersync-profile-collapse").
               selectedIndex = 1;
    },
    
    // displays profile preferences
    hide: function(){
      document.getElementById("foldersync-profile-box").
               setAttribute("style", "display:none");
      document.getElementById("foldersync-profile-collapse").
               selectedIndex = 0;
    },
    
    // set to changed profile, called on any change
    onChange: function(){
      document.getElementById("foldersync-profile").selectedIndex = 0;
      this.changeProfile();
      
      // notify favorite controller of change
      foldersync.accesspane.favorite.onChange();
    },
    
    // Open non-modal dialog showing available expressions in schema
    openAvailableTags: function(){
      try{
        window.openDialog("chrome://foldersync/content/dialogs/" +
                          "availabletags.xul",
                          "foldersync-availabletags-dialog",
                          "dialog=yes,modal=no,alwaysLowered=no," +
                          "centerscreen=yes,resizable=yes,dependent=yes");
      } catch (e){
        foldersync.central.logEvent("accesspane-profile",
                                    "Open Available Tag Dialog failed:\n\n" +
                                    e, 1, "chrome://foldersync/content/" +
                                    "accesspane.js", e.lineNumber);
      }
    },
    
    /* Enable or disable a node and all its child nodes
     * area (node): the xul node
     * enabled (bool): the element should get enabled
     */
    _enableArea: function(area, enabled){
      // Exclude captions because they look... in disabled state.
      if (area.nodeName != "caption")
        area.disabled = !enabled;
      for each (var child in area.childNodes)
        this._enableArea(child, enabled);
    },
    
    // make sure areas that should be inactive are disabled
    updateArea: function(){
      // helper
      var ea = function(id, cid){
        var self = foldersync.accesspane.profile;
        self._enableArea(document.getElementById(id),
                         document.getElementById(cid).checked);
      };
      ea("foldersync-structure", "foldersync-basic-struct");
      ea("foldersync-tab-structure", "foldersync-basic-struct");
      ea("foldersync-playlists", "foldersync-basic-plists");
      ea("foldersync-tab-playlists", "foldersync-basic-plists");
      ea("foldersync-structure-cover-box", "foldersync-structure-covers");
      ea("foldersync-playlists-sort-box", "foldersync-playlists-sort");
    },
    
    /* Load Profiles in menupopup of profile combo box; restore selection
     * selection (string): the GUID of the profile to select or null for crt
     */
    reloadProfiles: function(selection){
      try {
        // Get the current selection, if any
        var cSelection = selection;
        var cIndex = document.getElementById("foldersync-profile").
                              selectedIndex;
        var popup = document.getElementById("foldersync-profile-popup");
        if ((!selection) && (cIndex > 0))
          cSelection = popup.childNodes[cIndex].value;
        // Clear popup, except changed entry
        while (popup.childNodes.length > 1)
          popup.removeChild(popup.childNodes[1]);
        // Add all profiles
        var selected = false; // we selected the prior selected profile?
        var profiles = foldersync.preferences.getProfiles();
        for each (var profile in profiles){
          if (!profile.visible)
            continue; // We don't want to show invisible profiles ;)
          var node = document.createElement("menuitem");
          node.value = profile.GUID;
          node.setAttribute("label", profile.name);
          popup.appendChild(node);
          // Restore selection
          if (cSelection == node.value){
            document.getElementById("foldersync-profile").selectedItem = node;
            selected = true;
          }
        }
        // Select changed entry if we don't have a fitting profile
        if (!selected)
          document.getElementById("foldersync-profile").selectedIndex = 0;
        // Selection might have changed
        this.changeProfile();
      } catch (e){
        foldersync.central.logEvent("accesspane-profile",
                                    "Loading Profiles failed:\n\n" + e, 1,
                                    "chrome://foldersync/content/" +
                                    "accesspane.js", e.lineNumber);
      }
    },
    
    // Initialize profile stuff
    _init: function(){
      try {
        // Load available playlist formats
        var pflist = document.
                     getElementById("foldersync-playlists-format-popup");
        var pfs = foldersync.sync.getPFormats();
        for each (var pf in pfs){
          var node = pflist.appendChild(document.createElement("menuitem"));
          node.value = pf.internalName;
          node.setAttribute("label", foldersync.central.
                                                getLocaleString(pf.name));
        }
        // Load available profiles
        this.reloadProfiles();
        // Select first profile, fallback to default if there is no profile
        document.getElementById("foldersync-profile").selectedIndex = 1;
        if (document.getElementById("foldersync-profile").selectedIndex == -1){
          // Create new Default Profile (should not happen)
          foldersync.preferences.createProfile(foldersync.central.
                                 getLocaleString("profile.defaultname"));
          // Select it
          this.reloadProfiles();
          document.getElementById("foldersync-profile").selectedIndex = 1;
        }
        this.changeProfile();
        // Update enabled/disabled state
        this.updateArea();
      } catch (e){
        foldersync.central.logEvent("accesspane-profile",
                                    "Init Profiles failed:\n\n" + e, 1,
                                    "chrome://foldersync/content/" +
                                    "accesspane.js", e.lineNumber);
      }
    },
    
    /* Loads the given profile into UI
     * profile (profile): the profile to load
     */
    _readProfile: function(profile){
      try {
        // helper for checkboxes
        var ck = function (id, value){
          document.getElementById(id).checked = value;
        };
        // helper for textboxes
        var tx = function (id, value){
          document.getElementById(id).value = value;
        };
        // helper for menulists
        var ml = function(id, value){
          for each (var node in document.getElementById(id).childNodes[0].
                                                            childNodes)
            if (node.value == value)
              document.getElementById(id).selectedItem = node;
        }
        
        // Basic sync feature box
        ck("foldersync-basic-update", profile.flags.doUpdate);
        ck("foldersync-basic-delete", profile.flags.doDelete);
        ck("foldersync-basic-rockbox", profile.flags.doRockbox);
        ck("foldersync-basic-struct", profile.structure.isEnabled);
        ck("foldersync-basic-plists", profile.playlists.isEnabled);
        
        // Structure box
        tx("foldersync-structure-schema", profile.structure.schema);
        tx("foldersync-structure-tndigits", profile.structure.tnDigits);
        ck("foldersync-structure-covers", profile.structure.doCovers);
        tx("foldersync-structure-cover-schema", profile.structure.
                                                        coverSchema);
        tx("foldersync-structure-cover-file", profile.structure.coverFile);
        
        // Playlists box
        ml("foldersync-playlists-format", profile.playlists.format);
        ml("foldersync-playlists-encoding", profile.playlists.encoding);
        tx("foldersync-playlists-target", profile.playlists.toFolder);
        ck("foldersync-playlists-relpoint", profile.playlists.
                                                   doRelativePoint);
        tx("foldersync-playlists-split", profile.playlists.splitChar);
        ck("foldersync-playlists-sort", profile.playlists.isSorted);
        tx("foldersync-playlists-sortingscheme", profile.playlists.
                                                        sortingScheme);
        
        // Advanced sync feature box
        ck("foldersync-advanced-case", profile.advanced.compareCase);
        tx("foldersync-advanced-blocked", profile.advanced.blockedChars);
        tx("foldersync-advanced-replace", profile.advanced.replaceChar);
        ck("foldersync-advanced-cutReplace", profile.advanced.cutReplaced);
        ck("foldersync-advanced-cutSpaces", profile.advanced.cutSpaces);
        tx("foldersync-advanced-length", profile.advanced.fMaxLength);
      } catch(e){
        foldersync.central.logEvent("accesspane-profile",
                                    "Reading data from Profile failed:" +
                                    "\n\n" + e, 1, "chrome://foldersync/" +
                                    "content/accesspane.js", e.lineNumber);
      }
    },
    
    // Load the selected profile, called on changes in profile combo box
    changeProfile: function(){
      try{
        var profileNode = document.getElementById("foldersync-profile");
        var popup = document.getElementById("foldersync-profile-popup");
        // Update UI
        var customized = profileNode.selectedIndex > 0 ? false : true;
        document.getElementById("foldersync-profile-savedelete").
                 selectedIndex = customized ? 0 : 1;
        var dProf = profileNode.selectedIndex == 1;
        document.getElementById("foldersync-profile-delete").disabled = dProf;
        if (!customized){
          // Load Profile
          var guid = profileNode.selectedItem.value;
          var profile = foldersync.preferences.getProfileByGUID(guid);
          
          // Load it
          this._readProfile(profile);
        }          
        // ensure area en-/ disabled states
        this.updateArea();

        // ensure area en-/ disabled states
        this.updateArea();
      } catch (e){
        foldersync.central.logEvent("accesspane-profile",
                                    "Changing Profile failed:\n\n" + e, 1,
                                    "chrome://foldersync/content/" +
                                    "accesspane.js", e.lineNumber);
      }
    },
    
    // delete the selected profile, if user agreed a warning
    delete: function(){
      try{
        var profileNode = document.getElementById("foldersync-profile");
        var popup = document.getElementById("foldersync-profile-popup");
        var customized = !(profileNode.selectedIndex > 0);
        if (customized)
          throw Components.Exception("Custom Profile can't get deleted");
        var defaultProfile = profileNode.selectedIndex == 1;
        if (defaultProfile)
          throw Components.Exception("Default Profile can't get deleted");
        var guid = profileNode.selectedItem.value;
        var profile = foldersync.preferences.getProfileByGUID(guid);
        
        var msg = foldersync.central.getLocaleString("delete.sure");
        msg = msg.replace("%s", profile.name);
        if (!confirm(msg))
          return;
        
        // Mark as temporary, so syncs in queue won't have a missing profile
        profile.temporary = true;
        profile.visible = false;
        // Update profile list
        this.reloadProfiles();
      } catch (e){
        foldersync.central.logEvent("accesspane-profile",
                                    "Marking Profile for deletion failed:" +
                                    "\n\n" + e, 1, "chrome://foldersync/" +
                                    "content/accesspane.js", e.lineNumber);
      }
    },
    
    /* Write current data in a given profile
     * profile (profile): the profile to write the data to
     */
    _writeProfile: function(profile){
      try{
        // helper for checkboxes
        var ck = function (id){
          return document.getElementById(id).checked;
        };
        // helper for textboxes
        var tx = function (id){
          return document.getElementById(id).value;
        };
        // helper for textboxes with integers
        var ti = function(id){
          return document.getElementById(id).value*1;
        };
        // helper for menulists
        var ml = function(id){
          return document.getElementById(id).selectedItem.value;
        }
        
        // Basic sync feature box
        profile.flags.doUpdate = ck("foldersync-basic-update");
        profile.flags.doDelete = ck("foldersync-basic-delete");
        profile.flags.doRockbox = ck("foldersync-basic-rockbox");
        profile.structure.isEnabled = ck("foldersync-basic-struct");
        profile.playlists.isEnabled = ck("foldersync-basic-plists");
        
        // Structure box
        profile.structure.schema = tx("foldersync-structure-schema");
        profile.structure.tnDigits = ti("foldersync-structure-tndigits");
        profile.structure.doCovers = ck("foldersync-structure-covers");
        profile.structure.coverSchema = tx("foldersync-structure-cover-" +
                                           "schema");
        profile.structure.coverFile = tx("foldersync-structure-cover-file");
        
        // Playlists box
        profile.playlists.format = ml("foldersync-playlists-format");
        profile.playlists.encoding = ml("foldersync-playlists-encoding");
        profile.playlists.toFolder = tx("foldersync-playlists-target");
        profile.playlists.doRelativePoint = ck("foldersync-playlists-" +
                                               "relpoint");
        profile.playlists.splitChar = tx("foldersync-playlists-split");
        profile.playlists.isSorted = ck("foldersync-playlists-sort");
        profile.playlists.sortingScheme = tx("foldersync-playlists-" +
                                             "sortingscheme");
        
        // Advanced sync feature box
        profile.advanced.compareCase = ck("foldersync-advanced-case");
        profile.advanced.blockedChars = tx("foldersync-advanced-blocked");
        profile.advanced.replaceChar = tx("foldersync-advanced-replace");
        profile.advanced.cutReplaced = ck("foldersync-advanced-cutReplace");
        profile.advanced.cutSpaces = ck("foldersync-advanced-cutSpaces");
        profile.advanced.fMaxLength = ti("foldersync-advanced-length");
      } catch(e){
        foldersync.central.logEvent("accesspane-profile",
                                    "Writing data to Profile failed:" +
                                    "\n\n" + e, 1, "chrome://foldersync/" +
                                    "content/accesspane.js", e.lineNumber);
      }
    },
    
    // update a existing profile or add a new profile with current data
    save: function(){
      var result={};
      window.openDialog('chrome://foldersync/content/dialogs/saveprofile.xul',
                        'foldersync-saveprofile-dialog',
                        'dialog=yes,modal=yes,centerscreen=yes,resizable=no,' +
                        'dependent=yes', result);
      if (result.action){
        var profile = null;
        if (result.action == "new"){
          profile = foldersync.preferences.createProfile(result.name);
        } else { // Update
          profile = foldersync.preferences.getProfileByGUID(result.profile);
        }
        // Write to profile
        this._writeProfile(profile);
        // Select the new / updated profile
        this.reloadProfiles(profile.GUID);
      }
    },
    
    /* set the current active profile
     * profile (profile): the profile to set
     */
    setProfile:function(profile){
      // Search the profile
      var list = document.getElementById("foldersync-profile");
      var popup = document.getElementById("foldersync-profile-popup");
      for each (var node in popup.childNodes)
        if (profile.GUID == node.value){
          list.selectedItem = node;
          this.changeProfile();
          return;
        }
      // if it is not available, load it as custom
      this._readProfile(profile);
      list.selectedIndex = 0;
      this.changeProfile();
    },
    
    // get the currently active profile's GUID
    getGUID: function(){
      var cIndex = document.getElementById("foldersync-profile").selectedIndex;
      if (cIndex > 0){ // we have a profile
        return document.getElementById("foldersync-profile").
                        selectedItem.value;
      } else { // custom profile
        try {
          foldersync.central.logEvent("accesspane-profile",
                                      "Create temporary Profile", 4);
          // create a temporary profile with the current data
          var profile = foldersync.preferences.createProfile("Internal");
          profile.temporary = true;
          profile.visible = false;
          this._writeProfile(profile);
          return profile.GUID;
        } catch(e){
          foldersync.central.logEvent("accesspane-profile",
                                      "Creating temporary Profile failed:" +
                                      "\n\n" + e, 1, "chrome://foldersync/" +
                                      "content/accesspane.js", e.lineNumber);
        }
      }
    },
  },
  
  // All target folder related things
  target: {
    // get the current target folder
    getFolder: function(){
      return document.getElementById("foldersync-target").value;
    },
    
    // Called on any change in target folder box
    onChange: function(){
      // notify favorite controller of change
      foldersync.accesspane.favorite.onChange();
    },
    
    /* Set the current target folder
     * folder (string): the new target folder
     */
    setFolder: function(folder){
      document.getElementById("foldersync-target").value = folder;
      this.onChange();
    },
    
    // open Browse dialog and fill textbox
    browse: function(){
      // Create FilePicker and set GetFolder-Mode
      var fp = Components.classes["@mozilla.org/filepicker;1"].
                          createInstance(Components.interfaces.nsIFilePicker);
      fp.init(window, foldersync.central.getLocaleString("browse.info"),
              Components.interfaces.nsIFilePicker.modeGetFolder);
      // Show Dialog 
      var ret = fp.show();
      // check for OK input by user
      if ((ret == Components.interfaces.nsIFilePicker.returnOK) ||
          (ret == Components.interfaces.nsIFilePicker.returnReplace)) {
        var file = fp.file;
        // fill textbox
        this.setFolder(file.path);
      }
    },
  },
  
  // All playlist-list related things
  playlists: {
    // get array of playlist GUIDs
    getGUIDs: function(){
      var list = document.getElementById("foldersync-playlists-list");
      var result = [];
      // Add all GUIDs that are selected...
      for (var i = 0; i < list.childNodes.length; i++){
        var node = list.childNodes[i];
        if (node.getAttribute("checked") == "true")
          result.push(node.value);
      }
      return result;
    },
    
    /* select array of playlist GUIDs, unselect others
     * guids (array of string): the guids to get selected
     */
    setGUIDs: function(guids){
      var list = document.getElementById("foldersync-playlists-list");
      for (var i = 0; i < list.childNodes.length; i++){
        var node = list.childNodes[i];
        node.setAttribute("checked", "false");
        for each (var guid in guids)
          if (guid == node.value)
            node.setAttribute("checked", "true");
      }
      // Re-calculate Size
      this.calcSize();
    },
    
    // Reload list of playlists, restore old selection
    reload: function(){
      // Get old selection, to restore it later
      var oldSelection = this.getGUIDs();
      // Clear list
      var list = document.getElementById("foldersync-playlists-list");
      while (list.childNodes.length > 0)
        list.removeChild(list.childNodes[0]);
      // Get all playlists the user could want, add them to list
      var plists = foldersync.central.getPlaylists(true, true);
      for each (var plist in plists){
        var node = list.appendChild(document.createElement("listitem"));
        node.setAttribute("label", plist.name);
        node.value = plist.guid;
        node.setAttribute("type", "checkbox");
      }
      // Restore old selection
      for each (var node in list.childNodes)
        for each (var guid in oldSelection)
          if (node.value == guid)
            node.setAttribute("checked", "true");
      // Re-calc Size
      this.calcSize();
    },
    
    // Called on any change
    onChange: function(){
      // notify favorite controller of change
      foldersync.accesspane.favorite.onChange();
      // recalculate site
      this.calcSize();
    },
    
    // Calculate and display file size of current selection
    calcSize: function(){
      try{
        // Make sure Modules we'll use are loaded
        if (!window.SBProperties) 
          Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
        // Get playlist GUIDs
        var playlistGUIDs = foldersync.accesspane.playlists.getGUIDs();
        // Detect if we should count files in more than one playlists twice
        var useAll = (document.getElementById("foldersync-basic-struct").
                               checked) &&
                     (document.getElementById("foldersync-structure-schema").
                               value.split("%playlist%").length > 1);
        // Go through playlists and calculate size
        var size = 0;
        var hadFiles = [];
        for each (var guid in playlistGUIDs){
          var playlist = foldersync.central.getPlaylistByGUID(guid);
          if (useAll)
            hadFiles = [];
          for (var i = 0; i < playlist.length; i++){
            var track = playlist.getItemByIndex(i);
            // Look if we had this already
            var trackURL = track.getProperty(SBProperties.contentURL);
            var found = false;
            for each (var had in hadFiles)
              if (had == trackURL)
                found = true;
            if (found)
              continue;
            // Add to size and push into had already array
            hadFiles.push(trackURL);
            size += track.getProperty(SBProperties.contentLength)*1;
            
          }
        }
        
        // Calculate MB
        size = size / 1024 / 1024;
        // Switch MB / GB -> get the Text
        var text = foldersync.central.getLocaleString("playlists.size");
        if (size < 1024) {
          text = text.replace("%s", size.toFixed(2)+" MB");
        } else {
          text = text.replace("%s", (size/1024).toFixed(2)+" GB");
        }
        
        // Display
        var label = document.getElementById("foldersync-space");
        while (label.childNodes.length > 0)
          label.removeChild(label.childNodes[0]);
        label.appendChild(document.createTextNode(text));
      } catch(e){
        foldersync.central.logEvent("accesspane-playlists",
                                    "Calculating Size failed:\n\n" + e,
                                    1, "chrome://foldersync/content/" +
                                    "accesspane.js", e.lineNumber);
      }
    },
    
    // initialisation
    _init: function(){
      this.reload();
    },
  },
  
  // All favorite-sync-list related things
  favorite: {
    /* Load Favorites in menupopup of favorite combo box; restore old selection
     * selection (string): the GUID of the favorite to select or null for crt
     */
    reloadFavorites: function(selection){
      try {
        // Get the current selection, if any
        var cSelection = selection;
        var cIndex = document.getElementById("foldersync-favorites").
                              selectedIndex;
        var popup = document.getElementById("foldersync-favorites-popup");
        if ((!selection) && (cIndex > 0))
          cSelection = popup.childNodes[cIndex].value;
        // Clear popup, except changed entry
        while (popup.childNodes.length > 1)
          popup.removeChild(popup.childNodes[1]);
        // Add all profiles
        var selected = false; // we selected the prior selected favorite?
        var favorites = foldersync.preferences.getFavorites();
        for each (var fav in favorites){
          var node = document.createElement("menuitem");
          node.value = fav.GUID;
          node.setAttribute("label", fav.name);
          popup.appendChild(node);
          // Restore selection
          if (cSelection == node.value){
            document.getElementById("foldersync-favorites").
                     selectedItem = node;
            selected = true;
          }
        }
        // Select changed entry if we don't have a fitting profile
        if (!selected)
          document.getElementById("foldersync-favorites").selectedIndex = 0;
        // Selection might have changed, but not due to a user input
        this.changeFavorite(!document.getElementById("foldersync-favorites").
                                      selectedIndex, true);
      } catch (e){
        foldersync.central.logEvent("accesspane-favorite",
                                    "Loading Favorites failed:\n\n" + e, 1,
                                    "chrome://foldersync/content/" +
                                    "accesspane.js", e.lineNumber);
      }
    },
    
    // Initialize favorite things
    _init: function(){
      this.reloadFavorites();
    },
    
    // Called on a change of any profile-related control except profile box
    onChange: function(){
      // Enable load button, if we have a favorite selected
      if (this.getGUID())
        document.getElementById("foldersync-favorite-load").
                 disabled = false;
    },
    
    /* Disable perform button, and load Favorite into UI
     * empty (bool): if we have a favorite selected (false) or not (true).
     * dontload (bool): true if the user hasn't selected the favorite yet
     */
    changeFavorite: function(empty, dontload){
      document.getElementById("foldersync-favorite-start").
               setAttribute("disabled", empty ? "true" : "false");
      document.getElementById("foldersync-favorite-delete").
               setAttribute("disabled", empty ? "true" : "false");
      document.getElementById("foldersync-direct-update").
               setAttribute("disabled", empty ? "true" : "false");
      
      if (empty)
        document.getElementById("foldersync-favorite-load").
                 disabled = true;
      else if (!dontload){
        // display button to load into ui
        document.getElementById("foldersync-favorite-load").
                 disabled = false;
      }
    },
    
    // Delete selected favorite, if user agreed a warning
    delete: function(){
      try{
        // Get Favorite
        var guid = this.getGUID()
        if (!guid)
          throw Components.Exception("There is not favorite selected");
        var fav = foldersync.preferences.getFavoriteByGUID(guid);
        
        // Ensure the user is sure
        var msg = foldersync.central.getLocaleString("delete.sure");
        msg = msg.replace("%s", fav.name);
        if (!confirm(msg))
          return;
        
        // Delete profile associated with this favorite, if it is not a user defined profile
        var profile = foldersync.preferences.getProfileByGUID(fav.sync.profileGUID);
        if (!profile.visible)
          foldersync.preferences.removeProfile(profile);
        // Delete favorite
        foldersync.preferences.removeFavorite(fav);
        // Update favorite list
        this.reloadFavorites();
      } catch (e){
        foldersync.central.logEvent("accesspane-favorite",
                                    "Deleting Favorite failed:\n\n" + e, 1,
                                    "chrome://foldersync/" +
                                    "content/accesspane.js", e.lineNumber);
      }
    },
    
    // Perform the selected favorite
    start: function(){
      try{
        // Get Favorite
        var guid = this.getGUID();
        if (!guid)
          throw Components.Exception("There is not favorite selected");
        var fav = foldersync.preferences.getFavoriteByGUID(guid);
        // Start the sync in the Favorite
        foldersync.sync.performSync(fav.sync);
      } catch(e){
        foldersync.central.logEvent("accesspane-favorite",
                                    "Starting Favorite Sync failed:\n\n" + e,
                                    1, "chrome://foldersync/" +
                                    "content/accesspane.js", e.lineNumber);
      }
    },
    
    // Add a favorite with the selected values, asks user for name
    add: function(){
      try {
        var res = {};
        window.openDialog('chrome://foldersync/content/dialogs/addfav.xul',
                          'foldersync-addfav-dialog',
                          'dialog=yes,modal=yes,centerscreen=yes,' +
                          'resizable=no,dependent=yes', res);
        if (!res.name)
          return; // user cancelled
        var sync = foldersync.accesspane.getSync();
        // Ensure the profile is not temporary
        foldersync.preferences.getProfileByGUID(sync.profileGUID).
                   temporary = false;
        var favorite = foldersync.preferences.createFavorite(res.name, sync);
        this.reloadFavorites();
      } catch(e){
        foldersync.central.logEvent("accesspane-favorite",
                                    "Adding Favorite failed:\n\n" + e, 1,
                                    "chrome://foldersync/" +
                                    "content/accesspane.js", e.lineNumber);
      }
    },
    
    // Update current favorite with the selected values, asks user for name
    update: function(){
      try {
        // Get Favorite
        var guid = this.getGUID()
        if (!guid)
          throw Components.Exception("There is not favorite selected");
        var fav = foldersync.preferences.getFavoriteByGUID(guid);
        
        // Ensure the user is sure
        var msg = foldersync.central.getLocaleString("update.sure");
        msg = msg.replace("%s", fav.name);
        if (!confirm(msg))
          return;
        
        // Delete profile associated with this favorite, if it is not a user
        // defined profile
        var profile = foldersync.preferences.
                                 getProfileByGUID(fav.sync.profileGUID);
        if (!profile){
          foldersync.central.logEvent("accesspane-favorite",
                                      "There is no valid Profile avaiable " +
                                      "for Favorite " + guid + ". Skip old " +
                                      "Profile deletion.", 3,
                                      "chrome://foldersync/content/" +
                                      "accesspane.js");
        } else
          if (!profile.visible)
            foldersync.preferences.removeProfile(profile);
        
        // Fill profile with new data
        fav.sync = foldersync.accesspane.getSync();
        // Ensure the new profile is not temporary
        foldersync.preferences.getProfileByGUID(fav.sync.profileGUID).
                   temporary = false;
      } catch(e){
        foldersync.central.logEvent("accesspane-favorite",
                                    "Updating Favorite failed:\n\n" + e, 1,
                                    "chrome://foldersync/" +
                                    "content/accesspane.js", e.lineNumber);
      }
    },
    
    // Load current favorite into UI
    load: function(){
      try{
        // Get favorite
        var guid = this.getGUID();
        if (!guid)
          throw Components.Exception("There is not favorite selected");
        var fav = foldersync.preferences.getFavoriteByGUID(guid);
        // Update UI
        foldersync.accesspane.target.setFolder(fav.sync.targetFolder);
        foldersync.accesspane.playlists.setGUIDs(fav.sync.playlists);
        var profile = foldersync.preferences.
                                 getProfileByGUID(fav.sync.profileGUID);
        if (profile)
          foldersync.accesspane.profile.setProfile(profile);
        else
          foldersync.central.logEvent("accesspane-favorite",
                                      "There is no valid Profile avaiable " +
                                      "for Favorite " + guid + ". Skip " +
                                      "Profile loading.", 2,
                                      "chrome://foldersync/content/" +
                                      "accesspane.js");
        // Disable load button
        document.getElementById("foldersync-favorite-load").
                 disabled = true;
      } catch(e){
        foldersync.central.logEvent("accesspane-favorite",
                                    "Loading Favorite failed:\n\n" + e, 1,
                                    "chrome://foldersync/" +
                                    "content/accesspane.js", e.lineNumber);
      }
    },
    
    // Get the currently selected Favorite's GUID
    getGUID: function(){
      var favNode = document.getElementById("foldersync-favorites");
      var hasOne = favNode.selectedIndex > 0;
      if (!hasOne)
        return null;
      return favNode.selectedItem.value;
    },
  },
  
  // All help-related things
  help: {
    // Open preferences
    preferences: function(){
      window.openDialog("chrome://foldersync/content/dialogs/options.xul",
                        "foldersync-options",
                        "chrome,titlebar,toolbar,centerscreen,modal=no");
    },
    
    /* Open given URL in a new tab (foreground)
     * url (string): the url to open
     */
    openURL: function(url){
      try {
        // Get the main window
        var mainWindow = window.QueryInterface(Components.interfaces.
                                               nsIInterfaceRequestor).
                                getInterface(Components.interfaces.
                                             nsIWebNavigation).
                                QueryInterface(Components.interfaces.
                                               nsIDocShellTreeItem).
                                rootTreeItem.
                                  QueryInterface(Components.interfaces.
                                                 nsIInterfaceRequestor).
                                getInterface(Components.interfaces.
                                             nsIDOMWindow);
        mainWindow.gBrowser.selectedTab = mainWindow.gBrowser.
                                                     addTab(url);
      } catch (e){
        foldersync.central.logEvent("accesspane",
                                    "Opening of '" + url + "' failed:\n\n" + e,
                                    1, "chrome://foldersync/content/" +
                                    "accesspane.js", e.lineNumber);
      }
    },
  },
};

// load / unload accesspane controller with accesspane
window.addEventListener("load",
                        function(e){
                          foldersync.accesspane.onLoad(e);
                        },
                        false);
window.addEventListener("unload",
                        function(e){
                          foldersync.accesspane.onUnload(e);
                        },
                        false);