/**
//
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
// 
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
// 
// Software distributed under the License is distributed 
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either 
// express or implied. See the GPL for the specific language 
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this 
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc., 
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
// 
// END SONGBIRD GPL
//
 */

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

const MTPNS = 'http://songbirdnest.com/rdf/servicepane/mtp-device#';
const TYPE_X_SB_TRANSFER_MEDIA_LIST = "application/x-sb-transfer-media-list";

var mtpServicePaneServiceConfig = {
  className:      "Songbird MTP Device Support Module",
  cid:            Components.ID("{e0296079-a13a-4f5d-94c6-8d7a8f5c3c91}"),
  contractID:     "@songbirdnest.com/servicepane/mtpDevice;1",
  
  ifList: [ Ci.sbIServicePaneModule,
            Ci.nsIObserver ],
            
  categoryList:
  [
    {
      category: "service-pane",
      entry: "mtp-device"
    }
  ],

  devCatName:     "Media Transfer Protocol Device",

  devURNPrefix:   "urn:device:",
  libURNPrefix:   "urn:library:",
  itemURNPrefix:  "urn:item:",

  appQuitTopic:   "quit-application",

  devImgURL:      "chrome://songbird/skin/icons/icon-device.png",
  devBusyImgURL:  "chrome://songbird/skin/icons/icon-busy.png",
  devMgrURL:      "chrome://mtp/content/xul/mtpDeviceSummaryPage.xul",
};

function mtpServicePaneService() {

}

mtpServicePaneService.prototype.constructor = mtpServicePaneService;

mtpServicePaneService.prototype = {
  classDescription: mtpServicePaneServiceConfig.className,
  classID: mtpServicePaneServiceConfig.cid,
  contractID: mtpServicePaneServiceConfig.contractID,

  _cfg: mtpServicePaneServiceConfig,
  _xpcom_categories: mtpServicePaneServiceConfig.categoryList,
  
  _deviceInfoList: [],
  _deviceManagerSvc:      null,
  _deviceServicePaneSvc:  null,
  _observerSvc:           null,
  _servicePaneSvc:        null,
  
  _sync_table: {},              // a table of the last sync states per deviceid
  _sync_listeners: [],          // a list of local listener objects to call back
  _watched_deviceId: null,      // the deviceId whose sync state is being watched

  // ************************************
  // sbIServicePaneService implementation
  // ************************************
  servicePaneInit: function mtpServicePaneService_servicePaneInit(aServicePaneService) {
    this._servicePaneSvc = aServicePaneService;
    this._initialize();
  },

  fillContextMenu: function mtpServicePaneService_fillContextMenu(aNode,
                                                                  aContextMenu,
                                                                  aParentWindow) {
    var mtpDevice = this._getMtpDeviceFromNode(aNode);
    if (mtpDevice) {
      this._appendDeviceCommands(aNode, 
                                 aContextMenu, 
                                 mtpDevice, 
                                 aParentWindow,
                                 true);

    } else {
      // get the resource for the node
      var resource = this._libServicePaneSvc.getLibraryResourceForNode(aNode);
      // did we find one, is it a library ?
      if (resource && 
          resource.library == resource) {
        // get its parent
        var deviceNode = aNode.parentNode;
        // see if that node is an mtp device
        var mtpDevice = this._getMtpDeviceFromNode(deviceNode);
        // append the "new ..." commands on the context menu for the library
        this._appendDeviceCommands(aNode, 
                                   aContextMenu, 
                                   mtpDevice, 
                                   aParentWindow, 
                                   false);
      }
    }
  },

  fillNewItemMenu: function mtpServicePaneService_fillNewItemMenu(aNode,
                                                                  aContextMenu,
                                                                  aParentWindow) {
  },

  onSelectionChanged: function mtpServicePaneService_onSelectionChanged(aNode,
                                                          aContainer,
                                                          aParentWindow) {
  },

  canDrop: function mtpServicePaneService_canDrop(aNode, 
                                                  aDragSession, 
                                                  aOrientation, 
                                                  aWindow) {
    
    // only accept drops that are ON this node, not above or below
    if (aOrientation != Ci.nsITreeView.DROP_ON) 
      return false;
      
    // find the target library to which we need to forward the event
    var libraryNode = this._getLibraryNodeForMtpDeviceNode(aNode);
    if (libraryNode) {
      // forward the event
      return this._servicePaneSvc.canDrop(libraryNode.id, 
                                          aDragSession, 
                                          Ci.nsITreeView.DROP_ON, 
                                          aWindow);
    }
    return false;
  },

  onDrop: function mtpServicePaneService_onDrop(aNode, 
                                                aDragSession, 
                                                aOrientation, 
                                                aWindow) {
    // find the target library to which we need to forward the event
    var libraryNode = this._getLibraryNodeForMtpDeviceNode(aNode);
    if (libraryNode) {
      // forward the event
      this._servicePaneSvc.onDrop(libraryNode.id, 
                                  aDragSession, 
                                  Ci.nsITreeView.DROP_ON, 
                                  aWindow);
    } 
  },
  
  onDragGesture: function mtpServicePaneService_onDragGesture(aNode, 
                                                              aTransferable) {
  },

  onRename: function mtpServicePaneService_onRename(aNode, 
                                                    aNewName) {
    var mtpDevice = this._getMtpDeviceFromNode(aNode);
    if (mtpDevice) {
      try {
        mtpDevice.properties.friendlyName = aNewName;
        aNode.name = aNewName;
      }
      catch (e) {
        dump("Error: " + e);
        Components.utils.reportError("Unable to rename device");
      }
    }
  },

  // ************************************
  // nsIObserver implementation
  // ************************************
  observe: function mtpServicePaneService_observe(aSubject, 
                                                  aTopic, 
                                                  aData) {
    switch (aTopic) {
      case this._cfg.appQuitTopic :
        this._shutdown();
      break;
    }

  },

  // ************************************
  // nsISupports implementation
  // ************************************
  QueryInterface: XPCOMUtils.generateQI(mtpServicePaneServiceConfig.ifList),
  
  // ************************************
  // Internal methods
  // ************************************
  _initialize: function mtpServicePaneService_initialize() {
    this._observerSvc = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
    
    this._observerSvc.addObserver(this, this._cfg.appQuitTopic, false);

    this._deviceServicePaneSvc = Cc["@songbirdnest.com/servicepane/device;1"]
                                   .getService(Ci.sbIDeviceServicePaneService);

    this._libServicePaneSvc = Cc["@songbirdnest.com/servicepane/library;1"]
                                .getService(Ci.sbILibraryServicePaneService);
                                
    this._deviceManagerSvc = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                               .getService(Ci.sbIDeviceManager2);
    
    // Remove all stale nodes.
    this._removeDeviceNodes(this._servicePaneSvc.root);
    
    var deviceEventListener = {
      mtpDeviceServicePaneSvc: this,
      
      onDeviceEvent: function deviceEventListener_onDeviceEvent(aDeviceEvent) {
        this.mtpDeviceServicePaneSvc._processDeviceEvent(aDeviceEvent);
      }
    };
    
    this._deviceEventListener = deviceEventListener;
    this._deviceManagerSvc.addEventListener(deviceEventListener);

    this._createConnectedDevices();

    var sbSvc = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
    this._stringBundle = sbSvc.createBundle("chrome://songbird/locale/songbird.properties");
  },
  
  _shutdown: function mtpServicePaneService_shutdown() {
    this._observerSvc.removeObserver(this, this._cfg.appQuitTopic);

    this._deviceManagerSvc.removeEventListener(this._deviceEventListener);
    this._deviceEventListener = null;
    
    // Purge all device nodes before shutdown.
    this._removeDeviceNodes(this._servicePaneSvc.root);
    
    // Remove all references to nodes
    this._deviceInfoList = [];
    
    this._deviceManagerSvc = null;
    this._deviceServicePaneSvc = null;
    this._libServicePaneSvc = null;
    this._servicePaneSvc = null;
    this._observerSvc = null;
    this._stringBundle = null;
  },
  
  _processDeviceEvent: function mtpServicePaneService_processDeviceEvent(aDeviceEvent) {
    dump("XXXAus: " + aDeviceEvent + "\n\n");
    
    switch(aDeviceEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED: {
        this._addDeviceFromEvent(aDeviceEvent);
      }
      break;
      // maintain a table of sync states per deviceid, and call our local
      // listeners if the new state is for the deviceid being watched.
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_START: // fallthru
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_END: {
        var device = aDeviceEvent.origin;
        var deviceId = device.QueryInterface(Ci.sbIDevice).id;
        this._sync_table[deviceId] = 
          (aDeviceEvent.type == Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_START);
        this._callLocalSyncListeners(deviceId);
      }
      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED: {
        this._removeDeviceFromEvent(aDeviceEvent);
      }
      break;
      
      default:
    }
  },
  
  _addDeviceFromEvent: function mtpServicePaneService_addDeviceFromEvent(aDeviceEvent) {
    var device = aDeviceEvent.data.QueryInterface(Ci.sbIDevice);
    try {
      this._addDevice(device);
    }
    catch(e) {
      Components.utils.reportError(e);
    }
  },
  
  _removeDeviceFromEvent: function mtpServicePaneService_removeDeviceFromEvent(aDeviceEvent) {
    var device = aDeviceEvent.data.QueryInterface(Ci.sbIDevice);
    try {
      this._removeDevice(device);
    }
    catch(e) {
      Components.utils.reportError(e);
    }
  },
  
  _addDevice: function mtpServicePaneService_addDevice(aDevice) {
    dump("XXXAus: addDevice!!!\n\n");
    
    var device = aDevice.QueryInterface(Ci.sbIDevice);
    var devId = device.id;

    this._deviceInfoList[devId] = {};
    
    // Add a device node in the service pane.
    var devNode = this._deviceServicePaneSvc.createNodeForDevice2(device);
    devNode.setAttributeNS(MTPNS, "DeviceId", devId);
    devNode.contractid = this._cfg.contractID;
    devNode.image = this._cfg.devImgURL;
    devNode.url = this._cfg.devMgrURL + "?device-id=" + devId;
    devNode.editable = true;
    devNode.name = device.properties.friendlyName;

    devNode.hidden = false;

    this._deviceInfoList[devId].svcPaneNode = devNode;
    this._deviceInfoList[devId].device = device;
    
    // Add the device library.
    var libraries = device.content.libraries;
    if (libraries.length < 1) {
      // Oh no, we have no libraries
      Components.utils.reportError("Device " + devId + "(" + device.name + ") has no libraries!");
      return;
    }
    var lib = libraries.queryElementAt(0, Components.interfaces.sbILibrary);
    
    // this is very important. null named libraries make all hell break lose
    // in servicePaneService when they are rescanned.
    // it can go away once the library does get a default name tho.
    if (!lib.name) {
      lib.name = "Library";
      Components.utils.reportError("MTP Device Library had no name ! This is bad. Fixing.");
    }
      
    var devLibNode = this._libServicePaneSvc.createNodeForLibrary(lib);
    devNode.appendChild(devLibNode);
    devLibNode.hidden = false;
    devLibNode.name = lib.name || device.name || devId;

    // Update the device playlists.
    // Update the device state.
  },
  
  _removeDevice: function mtpServicePaneService_removeDevice(aDevice) {
    dump("XXXAus: removeDevice!!!\n\n");
    
    var device = aDevice.QueryInterface(Ci.sbIDevice);
    var devId = device.id;
    
    var devInfo = this._deviceInfoList[devId];
    if (!devInfo)
      return;

    // Remove the device library.

    // Remove the device node.
    this._servicePaneSvc.removeNode(devInfo.svcPaneNode);

    // Remove device info list entry.
    delete this._deviceInfoList[devId];
    
    // Also remove it from our table of sync states
    delete this._sync_table[devId];
    
    // Unload any page from the browser that might be related to this device
    this._unloadDevicePages(aDevice);
  },

  _createConnectedDevices: function mtpServicePaneService_createConnectedDevices() {
    var devices = ArrayConverter.JSArray(this._deviceManagerSvc.devices);
    dump("XXXAus: createConnectedDevices!!!\n\n");
    dump("XXXAus: creating " + devices.length + " devices!!!\n\n");
    
    for each (device in devices) {
      try {
        this._addDevice(device);
      }
      catch(e) {
        Components.utils.reportError(e);
      }
    }
  },
  
  /**
   * Attempt to extract a servicepane node into a corresponding mtp device
   * (returns null if not a device node, or not an mtp device)
   */
  _getMtpDeviceFromNode: function 
    mtpServicePaneService_getMtpDeviceFromNode(aNode) {
    var mtpDeviceId = aNode.getAttributeNS(MTPNS, 'DeviceId');
    if ( mtpDeviceId ) {
      return this._getDeviceForId(mtpDeviceId);
    }
    return null;
  },

  /**
   * Find the first library node for a given mtp device node
   */
  _getLibraryNodeForMtpDeviceNode: function
    mtpServicePaneService_getLibraryNodeForMtpDeviceNode(aNode) {
    var mtpDevice = this._getMtpDeviceFromNode(aNode);
    if (mtpDevice) {
      var library = null;
      var libs = mtpDevice.content.libraries;
      if (libs && libs.length > 0) {
        library = libs.queryElementAt(0, Ci.sbIMediaList);
        if (library) {
          return this._libServicePaneSvc.getNodeForLibraryResource(library);
        }
      }
    }
  },
  
  /**
   * Convert a deviceId into its corresponding sbIDevice
   */
  _getDeviceForId: function mtpServicePaneService_getDeviceForId(aDeviceId) {
    if (typeof(this._deviceInfoList[aDeviceId]) != 'undefined')
      return this._deviceInfoList[aDeviceId].device;
    return null;
  },
  
  _removeDeviceNodes: function mtpServicePaneService_removeDeviceNodes(aNode) {
    // Remove child device nodes.
    if (aNode.isContainer) {
      var childEnum = aNode.childNodes;
      while (childEnum.hasMoreElements()) {
        var child = childEnum.getNext().QueryInterface(Ci.sbIServicePaneNode);
        this._removeDeviceNodes(child);
      }
    }

    // Remove device nodes.
    if (aNode.contractid == this._cfg.contractID)
      this._servicePaneSvc.removeNode(aNode);
  },
  
  /**
   * Call our local listeners with the sync state for the deviceid 
   * that is currently being watched.
   */
  _callLocalSyncListeners: function 
    mtpServicePaneService_callLocalSyncListeners(aDeviceId) {
    // only call the listeners if the given deviceid is the one being watched
    if (aDeviceId == this._watched_deviceId) {
      // retrieve the last sync state for this deviceid
      var isSyncing = this._sync_table[aDeviceId];
      // if we do not have a sync state yet, it's because we haven't sync'ed
      // yet, so isSynching = false
      if (isSyncing == "undefined") 
        isSyncing = false;
      // call them all
      for (var i in this._sync_listeners) {
        var listener = this._sync_listeners[i];
        listener.onSyncStateChanged(isSyncing);
      }
    }
  },
  
  
  /**
   * Unload any page that may be loaded that belong to the specified device,
   * that is, device library views, device playlist views, and device summary
   * pages.
   */
  _unloadDevicePages: function 
    mtpServicePaneService_unloadDevicePages(aDevice) {

    // get gBrowser for the main window
    var window;
    var windowManager = Components.classes['@mozilla.org/appshell/window-mediator;1']
                                  .getService(Ci.nsIWindowMediator);
    window = windowManager.getMostRecentWindow("Songbird:Main");
    var gBrowser = window.gBrowser;
    
    if (gBrowser) {
      var devUrl = this._cfg.devMgrURL + "?device-id=" + aDevice.id;
      // get all browser tabs
      var tabs = gBrowser.mTabs;
      for (var i=0;i<tabs.length;i++) {
        var tab = tabs[i];
        // is this a library/playlist view ?
        if (tab.mediaListView) {
          // does it belong to this device ?
          if (this._viewBelongsToDevice(tab.mediaListView, aDevice)) {
            // unload it
            tab.backWithDefault(); 
          }
        } else {
          var document = tab.linkedBrowser.contentDocument;
          // is this the summary page for the device ?
          if (document.documentURI == devUrl) {
            // unload it
            tab.backWithDefault();
          }
        }
      }
    }
  },
  
  /**
   * Returns true if the view belongs to the given device
   */
  _viewBelongsToDevice: function 
    mtpServicePaneService_viewBelongsToDevice(aMediaListView, aDevice) {
    // get the parent library
    var mediaList = aMediaListView.mediaList;
    var viewLibrary = mediaList.library;
    
    return this._isDeviceLibrary(viewLibrary, aDevice);
  },
  
  _isDeviceLibrary: function
    mtpServicePaneService_isDeviceLibrary(aLibrary, aDevice) {
    // compare it to all the libraries for the device
    for (var i=0;i<aDevice.content.libraries.length;i++) {
      var devLib = aDevice.content.libraries[i];
      if (devLib == aLibrary) return true;
    }
    return false;
  },
  
  // ************************************
  // Context Commands
  // ************************************

  /**
   * Handles the "Get Info" context menu command for a device
   */
  _commandHandler_getDeviceInfo: function 
    mtpServicePaneService_commandHandler_getDeviceInfo(aNode,
                                                       aDevice, 
                                                       aParentWindow) {
    aParentWindow.SBWindow.openModalDialog
      (aParentWindow,
       "chrome://songbird/content/xul/device/deviceInfoDialog.xul",
       "",
       "chrome,centerscreen",
       [ aDevice ],
       null);
  },
  
  /**
   * Handles the "New Playlist" context menu command for a device
   */
  _commandHandler_newPlaylist: function 
    mtpServicePaneService_commandHandler_getDeviceInfo(aNode,
                                                       aDevice,
                                                       aParentWindow) {
    return aParentWindow.SBNewPlaylist();
  },

  /**
   * Handles the "Rename Device" context menu command for a device
   */
  _commandHandler_renameDevice: function 
    mtpServicePaneService_commandHandler_renameDevice(aNode,
                                                      aDevice, 
                                                      aParentWindow) {
    // todo: start node edition
    //aParentWindow.alert("Begin Editing Device Name");
    var servicePane = aParentWindow.gServicePane;
    if (servicePane) {
      servicePane.startEditingNode(aNode);
    }
  },
  
  /**
   * Handles the "Eject Device" context menu command for a device
   */
  _commandHandler_ejectDevice: function 
    mtpServicePaneService_commandHandler_ejectDevice(aNode,
                                                     aDevice, 
                                                     aParentWindow) {
    // No actual need for an Eject function on MTP, 
    // so simply remove the node :D
    this._removeDevice(aDevice);
  },
  
  /**
   * Handles the "Cancel Sync" context menu command for a device
   */
  _commandHandler_cancelDeviceSync: function 
    mtpServicePaneService_commandHandler_cancelDeviceSync(aNode,
                                                          aDevice, 
                                                          aParentWindow) {
    // todo: cancel the device syncing
    aParentWindow.alert("Cancel Device Sync");
  },
  
  _stringBundle                        : null, // the songbird string bundle
  
  /**
   * Appends all commands for a device node
   */
  _appendDeviceCommands: function 
    mtpServicePaneService_appendCommands(aNode,
                                         aContextMenu, 
                                         aDevice, 
                                         aParentWindow,
                                         aIsDevice) {

    var service = this;
    var command_handlers = [];
    
    // add a command to the context menu
    function addItem(id, label, accesskey, handler, disabled) {
      var menuitem = aContextMenu.ownerDocument.createElement('menuitem');
      menuitem.setAttribute('id', id);
      menuitem.setAttribute('class', 'menuitem-iconic');
      menuitem.setAttribute('label', label);
      if (accesskey) {
        menuitem.setAttribute('accesskey', accesskey);
      }
      if (handler) {
        command_handlers.push(["command", menuitem, handler]);
        menuitem.addEventListener("command", handler, false);
      }
      aContextMenu.appendChild(menuitem);
      if (disabled) {
        menuitem.setAttribute("disabled", "true");
      }
      return menuitem;
    };
    
    // add a separator to the context menu
    function addSeparator() {
      var separator = aContextMenu.ownerDocument.createElement('menuseparator');
      aContextMenu.appendChild(separator);
      return separator;
    };
    
    // creates a command handler that calls the specified method on the service
    function makeCommandHandler(handlerMethod) {
      var handler = {
        _node         : aNode,
        _service      : service, 
        _device       : aDevice,
        _parentWindow : aParentWindow,
        handleEvent   : function mtpServicePaneService_commandHandler( event ) {
          this._service[handlerMethod](this._node, this._device, this._parentWindow);
        }
      };
      return handler;
    };
    
    // add a listener to automatically update a context menu item's
    // attribute when the sync state changes for the device
    function addSyncListener(aDomNode, 
                              aAttribute, 
                              aValueWhenSyncing, 
                              aValueWhenNotSyncing) {
      var syncChangeListener = {
        _device      : aDevice,
        _node        : aDomNode,
        _attribute   : aAttribute,
        _syncvalue   : aValueWhenSyncing,
        _nosyncvalue : aValueWhenNotSyncing,
        onSyncStateChanged: function(aSyncing) {
          var val = aSyncing ? this._syncvalue : this._nosyncvalue;
          if (val)
            this._node.setAttribute(this._attribute, val);
          else
            this._node.removeAttribute(this._attribute);
        }
      };
      // add local listener
      service._sync_listeners.push(syncChangeListener);
    };

    if (aIsDevice) {
      var device_friendly_name = 
        aDevice.parameters.getProperty("DeviceFriendlyName");
      
      // todo: get actual device capacity
      var device_capacity = "?GB";

      var device_descriptor = device_friendly_name + " (" + device_capacity + ")";

      // "Device Name (XGB)"
      addItem('command_mtp_devicedescriptor', 
              device_descriptor, 
              null, 
              null,
              true);

      // "Get Info"
      addItem('command_mtp_getdeviceinfo', 
              this._localizeString('command.mtp.getdeviceinfo', "Get Info"), 
              this._localizeString('command.mtp.getdeviceinfo.accesskey', 'I'), 
              makeCommandHandler("_commandHandler_getDeviceInfo"));

      // "------------------"
      addSyncListener(addSeparator(), "hidden", null, "true");

      // "Cancel Sync"
      addSyncListener(addItem('command_mtp_canceldevicesync', 
                               this._localizeString('command.mtp.canceldevicesync',
                                                    'Cancel Sync'),
                               this._localizeString('command.mtp.canceldevicesync.accesskey',
                                                    'C'), 
                               makeCommandHandler("_commandHandler_cancelDeviceSync")),
                       "hidden",
                       null,
                       "true");

      // "------------------"
      addSeparator();
      
      // "New Playlist"
      addItem('command_mtp_newplaylist', 
              this._localizeString('menu.file.new', "New Playlist"), 
              this._localizeString('menu.file.new.accesskey', 'P'), 
              makeCommandHandler("_commandHandler_newPlaylist"));

      // "------------------"
      addSeparator();

      // "Rename"
      addItem('command_mtp_renamedevice', 
              this._localizeString('command.mtp.renamedevice', 'Rename'), 
              this._localizeString('command.mtp.renamedevice.accesskey', 'R'), 
              makeCommandHandler("_commandHandler_renameDevice"));

      // "------------------"
      addSeparator();

      // "Eject"
      addSyncListener(addItem('command_mtp_ejectdevice', 
                             this._localizeString('command.mtp.ejectdevice', 
                                                  'Eject'), 
                             this._localizeString('command.mtp.ejectdevice.accesskey',
                                                  'E'),
                             makeCommandHandler("_commandHandler_ejectDevice")),
                       "disabled",
                       "true",
                       null);
      
      // remember that this is the device we are watching
      this._watched_deviceId = aDevice.id;
      this._callLocalSyncListeners(aDevice.id);
    } else {

      // "New Playlist"
      addItem('command_mtp_newplaylist', 
              this._localizeString('menu.file.new', "New Playlist"), 
              this._localizeString('menu.file.new.accesskey', 'P'), 
              makeCommandHandler("_commandHandler_newPlaylist"));

    }
    
    // set up a listener for the popup hiding so we can clean up after ourselves               
    var popupHidingHandler = {
      _menu    : aContextMenu,
      _service : this,
      handleEvent : function mtpServicePaneService_popupHidingHandler( event ) {
        this._menu.removeEventListener("popuphiding", this, false);
        this._service._sync_listeners = [];
        this._watched_deviceId = null;
        while (command_handlers.length > 0) {
          var entry = command_handlers[0];
          var eventname = entry[0];
          var domnode = entry[1];
          var handler = entry[2];
          domnode.removeEventListener(eventname, handler);
          command_handlers.shift();
        }
      }
    };
    aContextMenu.addEventListener("popuphiding", popupHidingHandler, false);
  },
  

  /**
   * Localize a string, with default
   */
  _localizeString: function mtpServicePaneService_localizeString(aString, 
                                                                 aDefault) {
    var localized = aDefault;
    try {
      localized = this._stringBundle.GetStringFromName(aString);
    } catch (e) {
      //Components.utils.reportError(e + " when trying to translate " + aString);
    }
    return localized;
  }
};


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([mtpServicePaneService]);
}
