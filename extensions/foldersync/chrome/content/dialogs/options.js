/* options.js
 * This will be loaded into the options dialog. It provides the dialog's
 * controller.
 */
// Make a namespace.
if (typeof foldersync == 'undefined') {
  var foldersync = {};
};

/* Option Dialog Controller
 * Responsible for all stuff that is related to the Options Dialog
 */
foldersync.options = {
  // The init code for the dialog
  onLoad: function(e){
    // Get current preferences
    this._getPref();
    // En- / disable areas
    this.onChange();
  },
  
  // Updates all Preference UI elements with data from preference controller
  _getPref: function(){
    try{
      // Helper for textboxes
      var tx = function(id, value){
        document.getElementById(id).value = value;
      };
      // Helper for checkboxes
      var ck = function(id, value){
        document.getElementById(id).checked = value;
      };
      
      // Default tags
      var dtags = foldersync.preferences.getDefaultTags();
      for (var tagname in dtags)
        tx("foldersync-options-tags-" + tagname, dtags[tagname]);
      // Fallback
      var fallbacks = foldersync.preferences.getFallbacks();
      for (var tagname in fallbacks)
        ck("foldersync-options-tags-" + tagname + "-fallback",
           fallbacks[tagname]);
      // UI
      var ui = foldersync.preferences.getUIPrefs();
      ck("foldersync-options-ui-notification", ui.notifications.isEnabled);
      ck("foldersync-options-ui-notification-exclusive", ui.notifications.
                                                            onlyExclusive);
      ck("foldersync-options-ui-show-favorite", ui.show.favorites);
      ck("foldersync-options-ui-show-help", ui.show.help);
    } catch(e){
      foldersync.central.logEvent("options",
                                  "Getting preferences failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/dialogs/" +
                                  "options.js", e.lineNumber);
    }
  },
  
  // Writes all Preference UI element values to preference controller
  _setPref: function(){
    try{
      // Helper for textboxes
      var tx = function(id){
        return document.getElementById(id).value;
      };
      // Helper for checkboxes
      var ck = function(id){
        return document.getElementById(id).checked;
      };
      
      // Default tags
      var dtags = foldersync.preferences.getDefaultTags();
      for (var tagname in dtags)
        dtags[tagname] = tx("foldersync-options-tags-" + tagname);
      // Fallbacks
      var fallbacks = foldersync.preferences.getFallbacks();
      for (var tagname in fallbacks)
        fallbacks[tagname] = ck("foldersync-options-tags-" + tagname +
                                "-fallback");
      // UI
      var ui = foldersync.preferences.getUIPrefs();
      ui.notifications.isEnabled = ck("foldersync-options-ui-notification");
      ui.notifications.onlyExclusive = ck("foldersync-options-ui-" +
                                          "notification-exclusive");
      ui.show.favorites = ck("foldersync-options-ui-show-favorite");
      ui.show.help = ck("foldersync-options-ui-show-help");
    } catch(e){
      foldersync.central.logEvent("options",
                                  "Setting preferences failed:\n\n" + e, 1,
                                  "chrome://foldersync/content/dialogs/" +
                                  "options.js", e.lineNumber);
    }
  },
  
  // Called on changes in relevant elements, en- and disables areas.
  onChange: function(){
    document.getElementById("foldersync-options-ui-notification-exclusive").
             disabled = !document.getElementById("foldersync-options-ui-" +
                                                 "notification").checked;
    document.getElementById("foldersync-options-tags-albumartist").
             disabled = document.getElementById("foldersync-options-tags-" +
                                                "albumartist-fallback").
                                 checked;
  },
  
  // Called on accepting dialog, must return true for the dialog to close
  onAccept: function(){
    // Save changes
    this._setPref();
    return true;
  },
  
  // Shutdown code for the dialog
  onUnload: function(e){
    // on OS without Accept button: simulate accept on closing
    if (window.navigator.platform.slice(0,3) != "Win")
      this.onAccept();
  },
}

window.addEventListener("load",
                        function(e){
                          foldersync.options.onLoad(e);
                        },
                        false);
window.addEventListener("unload",
                        function(e){
                          foldersync.options.onUnload(e);
                        },
                        false);
