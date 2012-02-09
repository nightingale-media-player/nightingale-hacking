/* error.js
 * This will be loaded into the error window, and performs all actions in it.
 */
// Make a namespace.
if (typeof foldersync == 'undefined') {
  var foldersync = {};
};

/* Error Dialog Controller
 * Controller for all actions in the error dialog.
 */
foldersync.errorDialog = {
  /* Startup code for the error dialog
   * error (string): The short description of the error
   * message (string): The full error message
   */
  onLoad: function(e, error, message){
    // Display the error
    document.getElementById("foldersync-error-description").textContent=error;
    // Display the lines of the message
    if (message.toString() != "")
    {
      var sMessage = message.split("\n");
      for each (var cLine in sMessage)
      {
        var clabel = document.getElementById("foldersync-error-messagebox").
                              appendChild(document.createElement("label"));
        clabel.setAttribute("width","200");
        clabel.setAttribute("style","-moz-user-select:text; cursor: text");
        clabel.textContent=cLine;
      }
    } else {
      document.getElementById("foldersync-error-message").
               removeChild(document.
                           getElementById("foldersync-error-messagegroup"));
    }
  },
  
  // Shutdown code for the error dialog
  onUnload: function(e){
    
  },
};

window.addEventListener("load",
                        function(e){
                          foldersync.errorDialog.onLoad(e,
                                                        window.arguments[0],
                                                        window.arguments[1]);
                        },
                        false);
window.addEventListener("unload",
                        function(e){
                          foldersync.errorDialog.onUnload(e);
                        },
                        false);