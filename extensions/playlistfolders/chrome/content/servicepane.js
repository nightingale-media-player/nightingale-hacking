/* servicepane.js
 * This will be loaded into the main window, and manage Service Pane
 * integration of playlist folders.
 */
// Make a namespace.
if (typeof playlistfolders == 'undefined') {
  var playlistfolders = {};
};

/* Service Pane controller
 * Controller for playlist folder node handling in service pane
 * Things that are only related to the nodes (Drag'n'drop, etc.) are done
 * in pfServicePane XPCOM. This controller handles its connection to the
 * preference controller.
 */
playlistfolders.servicepane={
  /* Ensure the node has dndacceptnear set so it accepts our folder drops
   * n (node): the node to check
   */
  _ensureDND: function(n){
    var dndacc = n.dndAcceptNear.split(",");
    var found = false;
    for each (var e in dndacc)
      if (e == "application/x-pf-transfer-folder")
        found = true;
    if (!found){
      dndacc.push("application/x-pf-transfer-folder");
      n.dndAcceptNear = dndacc.join(",");
      // if DND missed, the playlist might need to be included:
      if (n.id.split("urn:item:").length > 1)
        this._ensureAvailable(n.id.split("urn:item:")[1]);
    }
  },
  
  /* Ensure the playlist's guid is registered somewhere in our sort
   * guid (string): the playlist's guid to check
   */
  _ensureAvailable: function(guid){
    var found = false;
    var folders = playlistfolders.preferences.getFolders();
    for each (var f in folders)
      for each (var e in f.content)
        if (e == guid)
          found = true;
    if (!found)
      playlistfolders.preferences.getFolderByGUID("{root}").content.push(guid);
  },
  
  // Inject our Playlist Folder Service Pane Module into service pane
  _inject: function(){
    // Ensure our Service Pane Module is registered and running
    var Cr = Components.manager.QueryInterface(Components.interfaces.
                                               nsIComponentRegistrar);
    if (!Cr.isContractIDRegistered("@rsjtdrjgfuzkfg.com/servicepane/" +
                                   "folders;1")){
      playlistfolders.central.logEvent("servicepane",
                                       "Register Service Pane Module.", 4);
      var emgr = Components.classes['@mozilla.org/extensions/manager;1'].
                            getService(Ci.nsIExtensionManager);
      var moduleFile = emgr.getInstallLocation("playlistfolders@" +
                                               "rsjtdrjgfuzkfg.com").
                            getItemLocation("playlistfolders@" +
                                            "rsjtdrjgfuzkfg.com");
      moduleFile.append("components");
      moduleFile.append("pfServicePane.js");
      Cr.autoRegister(moduleFile);
    }
    
    var SPS = Cc["@getnightingale.com/servicepane/service;1"].
                getService(Ci.sbIServicePaneService);
    var nodes = SPS.getNode("SB:Playlists").childNodes;
    while (nodes.hasMoreElements()){
      this._ensureDND(nodes.getNext());
    }
    
  },
  
  /* Returns the node for a folder
   * folder(folder): the folder object for the node
   */
  _getFolderNode: function(folder){
    var SPS = Cc["@getnightingale.com/servicepane/service;1"].
                getService(Ci.sbIServicePaneService);
    var dtSP_NS = 'http://getnightingale.com/rdf/servicepane#';
    var node = SPS.getNode("urn:folder:" + folder.GUID);
    if (!node){
      node = SPS.createNode();
      node.id = "urn:folder:" + folder.GUID;
      node.url = "chrome://playlistfolders/content/folder.xul?guid=" +
                 folder.GUID;
      node.name = folder.name;
      node.contractid = "@rsjtdrjgfuzkfg.com/servicepane/folders;1";
      node.editable = true;
      node.setAttributeNS(dtSP_NS, "addonID",
                          "playlistfolders@rsjtdrjgfuzkfg.com");
      if (folder.closed){
        node.setAttribute("isOpen", "false");
        node.image = "chrome://playlistfolders/skin/" +
                     "icon-folder-closed.png";
      } else {
        node.image = "chrome://playlistfolders/skin/icon-folder.png";
      }
      node.className = "playlistfolder"; 
    } else
      playlistfolders.central.logEvent("servicepane", "Node urn:folder:" +
                                       folder.GUID + " exists already, " +
                                       "won't create a new one.", 3,
                                       "chrome://playlistfolders/content" +
                                       "/servicepane.js");
    return node;
  },
  
  // Loads folders from preference controller into service pane
  onLoad: function(e) {
    playlistfolders.central.logEvent("servicepane",
                                     "Service Pane initialisation started", 5);
    try {
      // Inject our custom management code
      this._inject();
      // Get the ServicePaneService
      var SPS = Cc["@getnightingale.com/servicepane/service;1"].
                getService(Ci.sbIServicePaneService);
      
      // Fill service pane
      var fillFolder = function (node, folder){
        for each (var e in folder.content){
          var snode = null;
          if (playlistfolders.preferences.isFolder(e)){
            var f = playlistfolders.preferences.getFolderByGUID(e);
            snode = playlistfolders.servicepane._getFolderNode(f);
            fillFolder(snode, f);
          } else if (playlistfolders.preferences.isPlaylist(e)){
            snode = SPS.getNode("urn:item:" + e);
          }
          if (snode)
            node.appendChild(snode);
        }
      }
      var rootFolder = playlistfolders.preferences.getFolderByGUID("{root}");
      fillFolder(SPS.getNode("SB:Playlists"), rootFolder);
      // Register us as listener
      SPS.getNode("SB:Playlists").addMutationListener(this);
      // We're done!
      playlistfolders.central.logEvent("servicepane", "Service Pane " +
                                       "controller started.", 4);
    } catch (e){
      playlistfolders.central.logEvent("servicepane", "Startup of Service " +
                                       "Pane controller failed:\n\n" + e, 1,
                                       "chrome://playlistfolders/content/" +
                                       "servicepane.js", e.lineNumber);
    }
  },
  
  // Save order, unregister listener, etc.
  onUnload: function(e){
    playlistfolders.central.logEvent("servicepane", "Servicepane controller " +
                                     "shutdown started.", 5);
    // Unregister service pane listener
    var SPS = Cc["@getnightingale.com/servicepane/service;1"].
                getService(Ci.sbIServicePaneService);
    SPS.getNode("SB:Playlists").removeMutationListener(this);
    // We're done!
    playlistfolders.central.logEvent("servicepane",
                                     "Servicepane controller stopped.", 4);
  },
  
  // Called by UI, create a new Folder
  newFolder: function(){
    playlistfolders.central.logEvent("servicepane", "Create new Folder", 5);
    // Generate a new folder
    var bstrings = document.getElementById("playlistfolders-bookmark-strings");
    var fname = bstrings.getString("newFolderDefault");
    var folder = playlistfolders.preferences.createFolder(fname);
    var node = this._getFolderNode(folder);
    // Append it to the Playlist section in service pane, as last folder
    var SPS = Cc["@getnightingale.com/servicepane/service;1"]
                  .getService(Ci.sbIServicePaneService);
    var fPlist = null;
    var nodes = SPS.getNode("SB:Playlists").childNodes;
    while (nodes.hasMoreElements()){
      var n = nodes.getNext();
      if ((!fPlist) && n.id && (n.id.split("urn:item:").length > 1))
        fPlist = n;
    }
    SPS.getNode("SB:Playlists").insertBefore(node, fPlist);
    // Start a renaming
    gServicePane.startEditingNode(node);
  },
  
  /* Called by UI, delete a given folder
   * id (string): the folder's node's id
   */
  deleteFolder: function(id){
    playlistfolders.central.logEvent("servicepane", "Delete Folder with " +
                                     "Node " + id, 5);
    // Parse folder object from node's id
    var GUID = id.split('urn:folder:')[1];
    var folder = playlistfolders.preferences.getFolderByGUID(GUID);
    
    // Ensure the user wants to delete
    if (!playlistfolders.preferences.isDeleteBypass()) {
      var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                            .getService(Ci.nsIPromptService);
      var check = { value: false };

      var sbs = Cc["@mozilla.org/intl/stringbundle;1"]
                  .getService(Ci.nsIStringBundleService);
      
      var ngaleStrings = sbs.createBundle("chrome://nightingale/locale/" +
                                             "nightingale.properties");
      var pfoldersStrings = sbs.createBundle("chrome://playlistfolders/locale/" +
                                             "playlistfolders.properties");
      var strTitle = pfoldersStrings.GetStringFromName("delete.title");
      var strMsg = pfoldersStrings.formatStringFromName("delete.msg",
                                                        [folder.name], 1);

      var strCheck = SBString("playlist.deletewarning.check");

      var r = promptService.confirmEx(window,
                              strTitle,
                              strMsg,
                              Ci.nsIPromptService.STD_YES_NO_BUTTONS,
                              null,
                              null,
                              null,
                              strCheck,
                              check);
      if (check.value == true) {
        playlistfolders.preferences.doDeleteBypass()
      }
      if (r == 1) { // 0 = yes, 1 = no
        return;
      }
    }
    
    // Delete the folder, and if that succeeded delete the node
    if (playlistfolders.preferences.removeFolder(folder, true))
      document.getElementById(id).parentNode.
               removeChild(document.getElementById(id));
  },
  
  /* Called by UI, rename a given folder
   * node (node): the folder's node
   * name (string): the new name
   */
  renameFolder: function(node, name){
    playlistfolders.central.logEvent("servicepane", "Rename Folder with " +
                                     "Node " + node.id + " to " + name, 5);
    // Parse folder object from node's id
    var GUID = node.id.split('urn:folder:')[1];
    var folder = playlistfolders.preferences.getFolderByGUID(GUID);
    // Rename the folder
    playlistfolders.preferences.renameFolder(folder, name);
    node.name = name;
  },
  
  /* Called by UI, move a given folder to a given place
   * node (string): the id of the node of the folder to move
   * targetFolder (node): the target folder's node or SB:Playlists for root
   * insertBefore (node): the subfolder or playlist after target pos, or null
   */
  moveFolder: function(node, targetFolder, insertBefore){
    playlistfolders.central.logEvent("servicepane", "Move Folder with " +
                                     "Node " + node + " to " +
                                     targetFolder.id + ", before " +
                                     (insertBefore ? insertBefore.id : ""), 5);
    // Move the node(s) only, we'll keep track in preferences via listener
    var SPS = Cc["@getnightingale.com/servicepane/service;1"]
                  .getService(Ci.sbIServicePaneService);
    targetFolder.insertBefore(SPS.getNode(node), insertBefore);
  },
  
  /* Called by UI, move a given playlist to a given place
   * pguid (string): the id of the playlist to move
   * targetFolder (node): the target folder's node or SB:Playlists for root
   * insertBefore (node): the playlist after target pos, or null
   */
  movePlaylist: function(pguid, targetFolder, insertBefore){
    playlistfolders.central.logEvent("servicepane", "Move Playlist " +
                                     pguid + " to " +
                                     targetFolder.id + ", before " +
                                     (insertBefore ? insertBefore.id : ""), 5);
    // Move the node(s) only, we'll keep track in preferences via listener
    var SPS = Cc["@getnightingale.com/servicepane/service;1"]
                  .getService(Ci.sbIServicePaneService);
    targetFolder.insertBefore(SPS.getNode('urn:item:' + pguid), insertBefore);
  },
  
  ////////////////////////////////////
  // sbIServicePaneMutationListener //
  ////////////////////////////////////
  
  attrModified: function(aNode, aAttrName, aNamespace, aOldVal, aNewVal){
    // Check for opening / closing Nodes
    if ((aNode.id.split("urn:folder").length > 1)&&(aAttrName == "isOpen")){
      playlistfolders.central.logEvent("servicepane",
                                       "Open/Closes State change: " +
                                       aNode.id, 5);
      var guid = aNode.id.split('urn:folder:')[1];
      var folder = playlistfolders.preferences.getFolderByGUID(guid);
      folder.closed = aNewVal == "false";
      aNode.image =  folder.closed ? "chrome://playlistfolders/skin/" +
                                      "icon-folder-closed.png"
                                   : "chrome://playlistfolders/skin/" +
                                     "icon-folder.png";
    }
  },
  nodeInserted: function(aNode, aParent, aInsertBefores){
    // We're interested in new / moved Playlist nodes, to update our sort
    if ((aNode.id.split("urn:item:").length > 1)){
      playlistfolders.central.logEvent("servicepane",
                                       "Moved Playlist node " + aNode.id, 5);
      // Ensure the node accepts folders, if this is a new created node
      this._ensureDND(aNode);
      // Track movement in preference controller
      var pguid = aNode.id.split("urn:item:")[1];
      var tfolder = null;
      if (aParent.id != "SB:Playlists"){
        var tGUID = aParent.id.split('urn:folder:')[1];
        tfolder = playlistfolders.preferences.getFolderByGUID(tGUID);
      }
      var bGUID = null;
      if (aInsertBefores && (aInsertBefores.id.split('urn:item:').length > 1))
        bGUID = aInsertBefores.id.split('urn:item:')[1];
      if (aInsertBefores && (aInsertBefores.id.split('urn:folder:').length > 1))
        bGUID = aInsertBefores.id.split('urn:folder:')[1];
      playlistfolders.preferences.movePlaylist(pguid, tfolder, bGUID);
    }
    // We're interested in moved Folders, to update our sort
    if ((aNode.id.split("urn:folder:").length > 1)){
      playlistfolders.central.logEvent("servicepane",
                                       "Moved Folder node " + aNode.id, 5);
      // Track movement in preference controller
      var fguid = aNode.id.split("urn:folder:")[1];
      var tfolder = null;
      if (aParent.id != "SB:Playlists"){
        var tGUID = aParent.id.split('urn:folder:')[1];
        tfolder = playlistfolders.preferences.getFolderByGUID(tGUID);
      }
      var bGUID = null;
      if (aInsertBefores && (aInsertBefores.id.split('urn:item:').length > 1))
        bGUID = aInsertBefores.id.split('urn:item:')[1];
      if (aInsertBefores && (aInsertBefores.id.split('urn:folder:').length > 1))
        bGUID = aInsertBefores.id.split('urn:folder:')[1];
      var folder = playlistfolders.preferences.getFolderByGUID(fguid);
      //alert("bGUID: " + bGUID + "\nID: " + aInsertBefores ? aInsertBefores.id : "(missing)");
      playlistfolders.preferences.moveFolder(folder, tfolder, bGUID);
    }
  },
  nodeRemoved: function(aNode, aParent){
    // The only case we're interested for removed nodes is removing playlists
    if (aNode.id.split("urn:item:").length <= 1)
      return;
    // We're interested in playlists inside folders only
    if (aParent.id.split("urn:folder:").length <= 1)
      return;
    // Remove the playlist from the folder WITHOUT any central-delete stuff
    var pGUID = aNode.id.split("urn:item:")[1];
    var fGUID = aParent.id.split("urn:folder:")[1];
    playlistfolders.preferences.removePlaylist(pGUID, fGUID);
  },
};