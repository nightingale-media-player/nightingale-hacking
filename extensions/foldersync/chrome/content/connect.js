/* connect.js
 * This will be loaded into all windows, that need access to the global
 * foldersync controllers. It connects a local foldersync namespace with the
 * global one.
 */
// Make a namespace.
if (typeof foldersync == 'undefined') {
  var foldersync = {};
};

/* Connection controller
 * This connects our local foldersync namespace with the global foldersync
 * namespace in the main window.
 */
foldersync.connect = {
  // connects local and global foldersync namespaces
  establishConnection: function(){
    // Get the main window
    var mainWindow = window.QueryInterface(Components.interfaces.
                                                      nsIInterfaceRequestor).
                            getInterface(Components.interfaces.
                                                    nsIWebNavigation).
                            QueryInterface(Components.interfaces.
                                                      nsIDocShellTreeItem).
                            rootTreeItem.QueryInterface(Components.interfaces.
                                                        nsIInterfaceRequestor).
                            getInterface(Components.interfaces.nsIDOMWindow);
    // We might be in a area the main window is not accessible directly
    if (mainWindow == window){
      var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].
                          getService(Components.interfaces.nsIWindowMediator);
      mainWindow = wm.getMostRecentWindow("Nightingale:Main");
    }
    // Get the global namespace
    var _foldersync = mainWindow.foldersync;
    // Link controllers in local namespace
    foldersync.central = _foldersync.central;
    foldersync.sync = _foldersync.sync;
    foldersync.preferences = _foldersync.preferences;
    foldersync.rockbox = _foldersync.rockbox;
    foldersync.JSON = _foldersync.JSON;
    
    foldersync.central.logEvent("connect", "Connected: " + window.location, 4);
  },
};

// Connect to global namespace
foldersync.connect.establishConnection();