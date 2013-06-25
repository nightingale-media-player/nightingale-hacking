/**
 * \file pfServicePane.js
 */

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/DropHelper.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");

const CONTRACTID = "@rsjtdrjgfuzkfg.com/servicepane/folders;1";

const URN_PREFIX_FOLDER = 'urn:folder:';

const LSP = 'http://songbirdnest.com/rdf/library-servicepane#';
const SP='http://songbirdnest.com/rdf/servicepane#';

const TYPE_X_SB_TRANSFER_MEDIA_LIST = "application/x-sb-transfer-media-list";
const TYPE_X_PF_TRANSFER_FOLDER = "application/x-pf-transfer-folder";

/**
 * /class pfServicePane
 * /brief Injects folder related things in service pane
 * \sa sbIServicePaneService
 */
function pfServicePane() {
  this._servicePane = null;
}

//////////////////////////
// nsISupports / XPCOM  //
//////////////////////////

pfServicePane.prototype.QueryInterface =
  XPCOMUtils.generateQI([Ci.nsIObserver,
                         Ci.sbIServicePaneModule,
                         Ci.sbIServicePaneService,
                         Ci.nsIWindowMediatorListener]);

pfServicePane.prototype.classID =
  Components.ID("{12382b5a-e782-4de5-8b34-a26b82cdeeb8}");
pfServicePane.prototype.classDescription =
  "Playlist Folder AddOn Service Pane Service";
pfServicePane.prototype.contractID =
  CONTRACTID;
pfServicePane.prototype._xpcom_categories = [{
  category: 'service-pane',
  entry: 'folders',
  value: CONTRACTID
}];


//////////////////////////
// sbIServicePaneModule //
//////////////////////////

pfServicePane.prototype.servicePaneInit =
function pfServicePane_servicePaneInit(sps) {

  // keep track of the service pane service
  this._servicePane = sps;
  
  // connect to the global controllers of this AddOn
  var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].
                      getService(Components.interfaces.nsIWindowMediator);
  var mainWindow = wm.getMostRecentWindow("Songbird:Main");
  if (mainWindow)
    this._playlistfolders = mainWindow.playlistfolders;
  else{
    Cc['@mozilla.org/observer-service;1'].getService(Ci.nsIObserverService).
      addObserver(this, 'songbird-main-window-presented', false);
  }
}

pfServicePane.prototype.shutdown =
function pfServicePane_shutdown() {}

pfServicePane.prototype.fillContextMenu =
function pfServicePane_fillContextMenu(aNode, aContextMenu, aParentWindow) {
  var libraryMgr = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                     .getService(Ci.sbILibraryManager);
  
  // the playlists folder and the local library node get the "New Folder" item
  if ((aNode.id == 'SB:Playlists') ||
      (aNode.id.split('urn:library:' + libraryMgr.mainLibrary.guid).length > 1) ||
      (aNode.getAttributeNS(LSP, 'ListCustomType') == 'local')) {
    this.fillNewItemMenu(aNode, aContextMenu, aParentWindow);
  }
  
  // the folders get rename and delete entries
  if (aNode.id.split(URN_PREFIX_FOLDER).length > 1){
    this.fillFolderMenu(aNode, aContextMenu, aParentWindow);
  }
}

pfServicePane.prototype.fillNewItemMenu =
function pfServicePane_fillNewItemMenu(aNode, aContextMenu, aParentWindow) {
  var sbSvc = Cc["@mozilla.org/intl/stringbundle;1"].
                getService(Ci.nsIStringBundleService);
  var stringBundle = sbSvc.createBundle("chrome://songbird/locale/songbird." +
                                        "properties");

  // Add the new folder Button
  var menuitem = aContextMenu.ownerDocument.createElement("menuitem");
  menuitem.setAttribute("id", "menuitem_file_folder");
  menuitem.setAttribute("class", "menuitem-iconic");
  menuitem.setAttribute("label", stringBundle.
                                 GetStringFromName("menu.servicepane." +
                                                   "file.folder"));
  menuitem.setAttribute("accesskey", stringBundle.
                                     GetStringFromName("menu." +
                                                       "servicepane." +
                                                       "file.folder." +
                                                       "accesskey"));
  menuitem.setAttribute("oncommand", "playlistfolders.servicepane." +
                        "newFolder();");
  aContextMenu.appendChild(menuitem);
}

pfServicePane.prototype.fillFolderMenu =
function pfServicePane_fillFolderMenu(aNode, aContextMenu, aParentWindow) {
  var sbSvc = Cc["@mozilla.org/intl/stringbundle;1"].
                getService(Ci.nsIStringBundleService);
  var stringBundle = sbSvc.createBundle("chrome://songbird/locale/songbird." +
                                        "properties");

  // Add the delete folder button
  var menuitem = aContextMenu.ownerDocument.createElement("menuitem");
  menuitem.setAttribute("id", "menuitem_folder_delete");
  menuitem.setAttribute("class", "menuitem-iconic");
  menuitem.setAttribute("label", stringBundle.
                                 GetStringFromName("command.playlist.remove"));
  menuitem.setAttribute("oncommand", "playlistfolders.servicepane." +
                        "deleteFolder('" + aNode.id + "');");
  aContextMenu.appendChild(menuitem);
  
  // Add the rename folder button
  menuitem = aContextMenu.ownerDocument.createElement("menuitem");
  menuitem.setAttribute("id", "menuitem_folder_rename");
  menuitem.setAttribute("class", "menuitem-iconic");
  menuitem.setAttribute("label", stringBundle.
                                 GetStringFromName("command.playlist.rename"));
  menuitem.setAttribute("oncommand", "gServicePane.startEditingNode(document." +
                        "getElementById('" + aNode.id + "').nodeData);");
  aContextMenu.appendChild(menuitem);
}

pfServicePane.prototype.onSelectionChanged =
function pfServicePane_onSelectionChanged(aNode, aContainer, aParentWindow) {
}

pfServicePane.prototype._canDropReorder =
function pfServicePane__canDropReorder(aNode, aDragSession, aOrientation) {
  // see if we can handle the drag and drop based on node properties
  let types = [];
  if (aOrientation == 0) {
    // drop in
    if (aNode.dndAcceptIn) {
      types = aNode.dndAcceptIn.split(',');
    }
  } else {
    // drop near
    if (aNode.dndAcceptNear) {
      types = aNode.dndAcceptNear.split(',');
    }
  }
  for each (let type in types) {
    if (aDragSession.isDataFlavorSupported(type)) {
      return type;
    }
  }
  return null;
}

pfServicePane.prototype.canDrop =
function pfServicePane_canDrop(aNode, aDragSession, aOrientation, aWindow) {
  // Accept Folders, if they are no parent folders
  if (aDragSession.isDataFlavorSupported(TYPE_X_PF_TRANSFER_FOLDER)){
    let transferable = Cc["@mozilla.org/widget/transferable;1"].
                          createInstance(Ci.nsITransferable);
    transferable.addDataFlavor(TYPE_X_PF_TRANSFER_FOLDER);
    aDragSession.getData(transferable, 0);
    let data = {};
    transferable.getTransferData(TYPE_X_PF_TRANSFER_FOLDER, data, {});
    data = data.value.QueryInterface(Ci.nsISupportsString).data;
    var isparent = function(node, parentid){
      if (node.id == parentid){
        return true;
      }
      if (node.parentNode.id.split(URN_PREFIX_FOLDER).length > 1){
        if (isparent(node.parentNode, parentid)){
          return true;
        }
      }
      return false;
    };
    if (isparent(aNode, data)){
      return false;
    }
    return true;
  }
  
  // Use Drag and Drop Attributes
  if (this._canDropReorder(aNode, aDragSession, aOrientation))
    return true;
  
  // Accept playlists
  return aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_LIST);
}

pfServicePane.prototype.onDrop =
function pfServicePane_onDrop(aNode, aDragSession, aOrientation, aWindow) {
  if (aDragSession.isDataFlavorSupported(TYPE_X_PF_TRANSFER_FOLDER)){
    // Get the folder's Node's ID
    let transferable = Cc["@mozilla.org/widget/transferable;1"].
                         createInstance(Ci.nsITransferable);
    transferable.addDataFlavor(TYPE_X_PF_TRANSFER_FOLDER);
    aDragSession.getData(transferable, 0);
    let data = {};
    transferable.getTransferData(TYPE_X_PF_TRANSFER_FOLDER, data, {});
    data = data.value.QueryInterface(Ci.nsISupportsString).data;
    
    if (aOrientation == 0)
      // Move Folder into aNode
      this._playlistfolders.servicepane.
           moveFolder(data,
                      aNode,
                      null);
    if (aOrientation > 0)
      // Move Folder under aNode
      this._playlistfolders.servicepane.
           moveFolder(data,
                      aNode.parentNode,
                      aNode.nextSibling);
    if (aOrientation < 0)
      // Move Folder before aNode
      this._playlistfolders.servicepane.
           moveFolder(data,
                      aNode.parentNode,
                      aNode);
  } else if (aDragSession.isDataFlavorSupported(TYPE_X_SB_TRANSFER_MEDIA_LIST)){
    // Get the playlist's id
    var draggedList = DNDUtils.
        getInternalTransferDataForFlavour(aDragSession,
                                          TYPE_X_SB_TRANSFER_MEDIA_LIST,
                                          Ci.sbIMediaListTransferContext);
    var pguid = draggedList.list.guid;
    
    if (aOrientation == 0)
      // Move Folder into aNode
      this._playlistfolders.servicepane.
           movePlaylist(pguid,
                        aNode,
                        null);
    if (aOrientation > 0)
      // Move Folder under aNode
      this._playlistfolders.servicepane.
           movePlaylist(pguid,
                        aNode.parentNode,
                        aNode.nextSibling);
    if (aOrientation < 0)
      // Move Folder before aNode
      this._playlistfolders.servicepane.
           movePlaylist(pguid,
                        aNode.parentNode,
                        aNode);
  }
}

pfServicePane.prototype.onDragGesture =
function pfServicePane_onDragGesture(aNode, aDataTransfer) {
  // Ignore non-folders
  if (aNode.id.split(URN_PREFIX_FOLDER).length <= 1)
    return false;
  // Do not interfere with playlist draggings
  if (aDataTransfer.getData(TYPE_X_SB_TRANSFER_MEDIA_LIST))
    return false;
  // Do not drag more than one folder at a time
  if (aDataTransfer.getData(TYPE_X_PF_TRANSFER_FOLDER))
    return false;
  
  aDataTransfer.setData(TYPE_X_PF_TRANSFER_FOLDER, aNode.id);
  return true;
}


/**
 * Called when the user is about to attempt to rename a folder node
 */
pfServicePane.prototype.onBeforeRename =
function pfServicePane_onBeforeRename(aNode) {
}

/**
 * Called when the user has attempted to rename a folder node
 */
pfServicePane.prototype.onRename =
function pfServicePane_onRename(aNode, aNewName) {
  if (aNode && aNewName) {
    this._playlistfolders.servicepane.renameFolder(aNode, aNewName);
  }
}


/////////////////
// nsIObserver //
/////////////////

pfServicePane.prototype.observe =
function pfServicePane_observe(subject, topic, data) {
  // Link 
  if (topic == 'songbird-main-window-presented') {
    // connect to the global controllers of this AddOn
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"].
                        getService(Components.interfaces.nsIWindowMediator);
    var mainWindow = wm.getMostRecentWindow("Songbird:Main");
    this._playlistfolders = mainWindow.playlistfolders;
    // We no longer need to observe
    Cc['@mozilla.org/observer-service;1'].getService(Ci.nsIObserverService).
      removeObserver(this, 'songbird-main-window-presented');
  }
}

///////////
// XPCOM //
///////////

var NSGetModule = XPCOMUtils.generateNSGetFactory([pfServicePane]);
