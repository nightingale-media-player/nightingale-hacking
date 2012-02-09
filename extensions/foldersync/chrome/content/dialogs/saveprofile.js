/* saveprofile.js
 * This will be loaded into the save profile dialog. It provides the dialog's
 * controller.
 */
// Make a namespace.
if (typeof foldersync == 'undefined') {
  var foldersync = {};
};

/* Save Profile Dialog Controller
 * Responsible for all stuff that is related to the Save Profile Dialog
 */
foldersync.saveprofile = {
  // If we cleared the name field already
  _clearedName: false,
  
  // Startup code for the save profile dialog
  onLoad: function(e){
    // Update en-/disabled state of controls
    this.onRChange();
    this.reloadProfiles();
  },
  
  // En-/Disables controls depending on current selection
  onRChange: function(e){
    var rGroup = document.getElementById("foldersync-saveprofile-radio");
    var nName = document.getElementById("foldersync-saveprofile-new-name");
    var uList = document.getElementById("foldersync-savedprofile-" +
                                        "update-profile");
    if (rGroup.selectedIndex == 0){ // new profile
      nName.disabled = false;
      uList.disabled = true;
    } else { // update profile
      nName.disabled = true;
      uList.disabled = false;
    }
  },
  
  // Writes the result to window's arguments, return null. Called on accepting
  onAccept: function(){
    var rGroup = document.getElementById("foldersync-saveprofile-radio");
    var nName = document.getElementById("foldersync-saveprofile-new-name");
    var uList = document.getElementById("foldersync-savedprofile-" +
                                        "update-profile");
    window.arguments[0].action = rGroup.selectedIndex == 0 ? "new" : "update";
    window.arguments[0].profile = uList.selectedItem.value;
    window.arguments[0].name = nName.value;
    return true;
  },
  
  // Clears the name textbox
  onNameFocus: function(){
    if (!this._clearedName){
      document.getElementById("foldersync-saveprofile-new-name").value = "";
      this._clearedName = true;
    }
  },
  
  // Load Profiles in menupopup of profile combo box; restore old selection
  reloadProfiles: function(){
    try {
      var uList = document.getElementById("foldersync-savedprofile-" +
                                          "update-profile");
      var popup = document.getElementById("foldersync-savedprofile-" +
                                           "update-profile-popup");
      // Get the current selection, if any
      var cSelection = null;
      var cIndex = uList.selectedIndex;
      if (cIndex >= 0)
        cSelection = popup.childNodes[cIndex].value;
      // Clear popup
      while (popup.childNodes.length > 0)
        popup.removeChild(popup.childNodes[0]);
      // Add all profiles
      var selected = false; // we selected the prior selected profile?
      var profiles = foldersync.preferences.getProfiles();
      for each (var profile in profiles){
        if (!profile.visible)
          continue; // We don't want to show invisible profiles
        var node = document.createElement("menuitem");
        node.value = profile.GUID;
        node.setAttribute("label", profile.name);
        popup.appendChild(node);
        // Restore selection
        if (cSelection == node.value){
          uList.selectedItem = node;
          selected = true;
        }
      }
      // Select first if the profile just got deleted / we're starting
      if (!selected)
        uList.selectedIndex = 0;
    } catch (e){
      foldersync.central.logEvent("saveprofile",
                                  "Loading Profiles failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/" +
                                  "dialogs/saveprofile.js", e.lineNumber);
    }
  },
  
  // Shutdown code for the save profile dialog
  onUnload: function(e){
    
  },
};

window.addEventListener("load",
                        function(e){
                          foldersync.saveprofile.onLoad(e);
                        },
                        false);
window.addEventListener("unload",
                        function(e){
                          foldersync.saveprofile.onUnload(e);
                        },
                        false);