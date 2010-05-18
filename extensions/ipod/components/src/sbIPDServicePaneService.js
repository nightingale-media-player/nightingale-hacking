/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
 * \file  sbIPDServicePaneService.js
 * \brief Javascript source for the iPod device service pane service.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// iPod device service pane services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// iPod device service pane imported services.
//
//------------------------------------------------------------------------------

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbStorageFormatter.jsm");


//------------------------------------------------------------------------------
//
// iPod device service pane services defs.
//
//------------------------------------------------------------------------------

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

const TYPE_X_SB_TRANSFER_MEDIA_LIST = "application/x-sb-transfer-media-list";
const IPODSP_NS = "http://songbirdnest.com/rdf/ipod-servicepane#";


//------------------------------------------------------------------------------
//
// iPod device service pane services configuration.
//
//------------------------------------------------------------------------------

//
// className                    Name of component class.
// cid                          Component CID.
// contractID                   Component contract ID.
// ifList                       List of external component interfaces.
// categoryList                 List of component categories.
//
// localeBundlePath             Path to locale string bundle.
//
// devURNPrefix                 Device URN prefix.
// libURNPrefix                 Library URN prefix.
// itemURNPrefix                Item URN prefix.
//
// appQuitTopic                 Application quit event observer topic.
//
// devImgURL                    URL of device service pane node image.
// devBusyImgURL                URL of busy device service pane node image.
// devMgrURL                    URL of device manager window.
//

var IPD_SPSCfg= {
  className: "Songbird iPod Device Service Pane Service",
  cid: Components.ID("{EC76F798-AB27-4B85-A2AD-0D794E0232F9}"),
  contractID: "@songbirdnest.com/servicepane/IPodDevice;1",
  ifList: [ Ci.sbIServicePaneModule,
            Ci.nsIObserver,
            Ci.sbIDeviceEventListener ],
  categoryList:
  [
    {
      category: "service-pane",
      entry: "ipod-device"
    }
  ],

  localeBundlePath: "chrome://ipod/locale/IPodDevice.properties",

  devURNPrefix: "urn:device:",
  libURNPrefix: "urn:library:",
  itemURNPrefix: "urn:item:",

  devImgURL: "chrome://ipod/skin/icon_ipod_16x16.png",
  devBusyImgURL: "chrome://songbird/skin/icons/icon-busy.png",
  devMgrURL: "chrome://ipod/content/xul/iPodDeviceSummaryPage.xul"
};


//------------------------------------------------------------------------------
//
// iPod device service pane services.
//
//------------------------------------------------------------------------------

// IPD_SPS
/**
 * \brief Construct an iPod device service pane services object.
 */

function IPD_SPS()
{
}

/* Set the constructor. */
IPD_SPS.prototype.constructor = IPD_SPS;

IPD_SPS.prototype = {
  //
  // iPod device service pane services fields.
  //
  //   classDescription         Description of component class.
  //   classID                  Component class ID.
  //   contractID               Component contract ID.
  //   _xpcom_categories        List of component categories.
  //
  //   _cfg                     Configuration settings.
  //   _locale                  Locale string bundle.
  //   _observerSvc             Observer service object.
  //   _devMgr                  Device manager object.
  //   _servicePaneSvc          Service pane service object.
  //   _devServicePaneSvc       Device service pane service object.
  //   _libServicePaneSvc       Library service pane service object.
  //   _libMgr                  Library manager object.
  //   _devInfoList             List of device information.
  //

  classDescription: IPD_SPSCfg.className,
  classID: IPD_SPSCfg.cid,
  contractID: IPD_SPSCfg.contractID,
  _xpcom_categories: IPD_SPSCfg.categoryList,

  _cfg: IPD_SPSCfg,
  _locale: null,
  _observerSvc: null,
  _devMgr: null,
  _servicePaneSvc: null,
  _devServicePaneSvc: null,
  _libServicePaneSvc: null,
  _libMgr: null,
  _devInfoList: null,


  //----------------------------------------------------------------------------
  //
  // iPod device service pane sbIServicePaneModule services.
  //
  //----------------------------------------------------------------------------

  /* \brief Initialize this service pane module
   * This is where the module should set itself up in the tree if it hasn't
   * before.
   * \param aServicePaneService the service pane service instance
   */

  servicePaneInit: function IPD_SPS_servicePaneInit(aServicePaneService) {
    // Save the service pane service.
    this._servicePaneSvc = aServicePaneService;

    // Initialize the services.
    this._initialize();
  },


  /* \brief Fill the context menu for the given node
   * \param aNode the node that was context-clicked on
   * \param aContextMenu the menu we're trying to fill
   * \param aParentWindow the toplevel window we're displaying in
   */

  fillContextMenu: function IPD_SPS_fillContextMenu(aNode,
                                                    aContextMenu,
                                                    aParentWindow) { },


  /* \brief Fill a menu with menuitems to create new tree nodes
   * \param aNode the node or null for the click and hold menu
   * \param aContextMenu the menu we're trying to fill
   * \param aParentWindow the toplevel window we're displaying in
   */

  fillNewItemMenu: function IPD_SPS_fillNewItemMenu(aNode,
                                                    aContextMenu,
                                                    aParentWindow) { },


  /* \brief Selection changes notification
   * \param aNode the node that is now selected
   * \param aParentWindow the toplevel window we're displaying in
   */

  onSelectionChanged: function IPD_SPS_onSelectionChanged(aNode,
                                                          aContainer,
                                                          aParentWindow) { },


  /* \brief Return whether the item can be dropped on the node
   */

  canDrop: function IPD_SPS_canDrop(aNode,
                                    aDragSession,
                                    aOrientation,
                                    aWindow) { },


  /* \brief Handle dropping of an item on node
   */

  onDrop: function IPD_SPS_onDrop(aNode, aDragSession, aOrientation, aWindow) {
  },


  /* \brief Handle drag gesture
   */

  onDragGesture: function IPD_SPS_onDragGesture(aNode, aTransferable) { },


  /* \brief Called before a user starts to rename a node.
   * \param aNode the node that is about to be renamed
   */

  onBeforeRename: function IPD_SPS_onBeforeRename(aNode) {
  },


  /* \brief Called when a node is renamed by the user, allows
   *        the module to accept the action by setting the
   *        node name to the given value.
   * \param aNode the node that was renamed
   * \param aNewName new name entered by the user
   */

  onRename: function IPD_SPS_onRename(aNode, aNewName) {
    // Get the device.  Do nothing if not a device node.
    var deviceID = aNode.getAttributeNS(IPODSP_NS, "device-id");
    if (!deviceID)
      return;
    var deviceInfo = this._devInfoList[deviceID];
    if (!deviceInfo)
      return;
    var device = deviceInfo.device;
    if (!device)
      return;

    // Set the new device name.
    device.properties.friendlyName = aNewName;
  },


  /* \brief Called when the service pane service is shutting down
   */
  shutdown: function IPD_SPS_shutdown() {
    // Do nothing, since we shut down on quit-application
  },


  //----------------------------------------------------------------------------
  //
  // iPod device service pane sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * aEvent - information about the event
   */

  onDeviceEvent: function IPD_SPS_onDeviceEvent(aEvent) {
    // Dispatch processing of the event.
    switch(aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED :
        this._addDevice(aEvent.data.QueryInterface(Ci.sbIDevice));
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED :
        this._removeDevice(aEvent.data.QueryInterface(Ci.sbIDevice));
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED :
        this._updateDevState(aEvent.origin.QueryInterface(Ci.sbIDevice));
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_INFO_CHANGED :
        this._updateDevInfo(aEvent.origin.QueryInterface(Ci.sbIDevice));
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // iPod device service pane sbIMediaListListener services.
  //
  //----------------------------------------------------------------------------

  //
  // _inBatchMap                Map of media lists in a batch update.
  // _rescanListMap             Map of media lists to be rescanned.
  //

  _inBatchMap: null,
  _rescanListMap: null,


  /**
   * \brief Initialize the sbIMediaListListener services.
   */

  _sbIMLL_Initialize: function IPD_SPS__sbIMLL_Initialize() {
    this._inBatchMap = { };
    this._rescanListMap = { };
  },


  /**
   * \brief Finalize the sbIMediaListListener services.
   */

  _sbIMLL_Finalize: function IPD_SPS__sbIMLL_Finalize() {
    this._inBatchMap = null;
    this._rescanListMap = null;
  },


  /**
   * \brief Called when a media item is added to the list.
   * \param aMediaList The list that has changed.
   * \param aMediaItem The new media item.
   * \return True if you do not want any further onItemAdded notifications for
   *         the current batch.  If there is no current batch, the return value
   *         is ignored.
   */

  onItemAdded: function IPD_SPS_onItemAdded(aMediaList, aMediaItem, aIndex) {
    var returnVal = false;
    try { returnVal = this._onItemAdded(aMediaList, aMediaItem); }
    catch (ex) { dump("onItemAdded exception: " + ex + "\n"); }
    return (returnVal);
  },

  _onItemAdded: function IPD_SPS__onItemAdded(aMediaList, aMediaItem) {
    var                         noFurtherNotifications = false;

    // If in a batch, mark media list for rescanning.  Otherwise, add any
    // playlists to the device playlist maps.
    if (this._inBatchMap[aMediaList.guid]) {
      this._rescanListMap[aMediaList.guid] = true;
      noFurtherNotifications = true;
    } else {
      if (aMediaItem instanceof Components.interfaces.sbIMediaList)
        this._addDevPlaylist(aMediaItem);
    }

    return noFurtherNotifications;
  },


  /**
   * \brief Called before a media item is removed from the list.
   * \param aMediaList The list that has changed.
   * \param aMediaItem The media item to be removed
   * \return True if you do not want any further onBeforeItemRemoved
   *         notifications for the current batch.  If there is no current batch,
   *         the return value is ignored.
   */

  onBeforeItemRemoved: function IPD_SPS_onBeforeItemRemoved(aMediaList,
                                                            aMediaItem,
                                                            aIndex) {
    var                         noFurtherNotifications = false;

    // If in a batch, mark media list for rescanning.  Otherwise, remove any
    // playlists from the device playlist maps.
    if (this._inBatchMap[aMediaList.guid]) {
      this._rescanListMap[aMediaList.guid] = true;
      noFurtherNotifications = true;
    } else {
      if (aMediaItem instanceof Components.interfaces.sbIMediaList)
        this._removeDevPlaylist(aMediaItem);
    }

    return noFurtherNotifications;
  },


  /**
   * \brief Called after a media item is removed from the list.
   * \param aMediaList The list that has changed.
   * \param aMediaItem The removed media item.
   * \return True if you do not want any further onAfterItemRemoved for the
   *         current batch.  If there is no current batch, the return value is
   *         ignored.
   */

  onAfterItemRemoved: function IPD_SPS_onAfterItemRemoved(aMediaList,
                                                          aMediaItem,
                                                          aIndex) {
    return true;
  },


  /**
   * \brief Called when a media item is changed.
   * \param aMediaList The list that has changed.
   * \param aMediaItem The item that has changed.
   * \param aProperties Array of properties that were updated.  Each property's
   *        value is the previous value of the property
   * \return True if you do not want any further onItemUpdated notifications
   *         for the current batch.  If there is no current batch, the return
   *         value is ignored.
   */

  onItemUpdated: function IPD_SPS_onItemUpdated(aMediaList,
                                                aMediaItem,
                                                aIndex,
                                                aProperties) {
    return true;
  },

  /**
   * \Brief Called before a media list is cleared.
   * \param sbIMediaList aMediaList The list that is about to be cleared.
   * \param aExcludeLists If true, only media items, not media lists, are being
   *                      cleared.
   * \return True if you do not want any further onBeforeListCleared notifications
   *         for the current batch.  If there is no current batch, the return
   *         value is ignored.
   */

  onBeforeListCleared: function IPD_SPS_onBeforeListCleared(aMediaList,
                                                            aExcludeLists) {
    return true;
  },
  
  /**
   * \Brief Called when a media list is cleared.
   * \param sbIMediaList aMediaList The list that was cleared.
   * \param aExcludeLists If true, only media items, not media lists, were
   *                      cleared.
   * \return True if you do not want any further onListCleared notifications
   *         for the current batch.  If there is no current batch, the return
   *         value is ignored.
   */

  onListCleared: function IPD_SPS_onListCleared(aMediaList,
                                                aExcludeLists) {
    var                         noFurtherNotifications = false;

    // If in a batch, mark media list for rescanning.  Otherwise, update the
    // device playlist maps.
    if (this._inBatchMap[aMediaList.guid]) {
        this._rescanListMap[aMediaList.guid] = true;
        noFurtherNotifications = true;
    } else {
        this._updateDevPlaylists(aMediaList);
    }

    return noFurtherNotifications;
  },


  /**
   * \brief Called when the library is about to perform multiple operations at
   *        once.
   *
   * This notification can be used to optimize behavior. The consumer may
   * choose to ignore further onItemAdded or onItemRemoved notifications until
   * the onBatchEnd notification is received.
   *
   * \param aMediaList The list that has changed.
   */

  onBatchBegin: function IPD_SPS_onBatchBegin(aMediaList) {
    // Add media list to the in batch map.
    this._inBatchMap[aMediaList.guid] = true;
  },


  /**
   * \brief Called when the library has finished performing multiple operations
   *        at once.
   *
   * This notification can be used to optimize behavior. The consumer may
   * choose to stop ignoring onItemAdded or onItemRemoved notifications after
   * receiving this notification.
   *
   * \param aMediaList The list that has changed.
   */

  onBatchEnd: function(aMediaList) {
    // Remove media list from the in batch map.
    delete this._inBatchMap[aMediaList.guid];

    // Update device playlists if needed.
    if (this._rescanListMap[aMediaList.guid]) {
      this._updateDevPlaylists(aMediaList);
      delete(this._rescanListMap[aMediaList.guid]);
    }
  },


  //----------------------------------------------------------------------------
  //
  // iPod device service pane nsIObserver services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle the observed event specified by aSubject, aTopic, and aData.
   *
   * \param aSubject            Event subject.
   * \param aTopic              Event topic.
   * \param aData               Event data.
   */

  observe: function IPD_SPS_observe(aSubject, aTopic, aData) {
    try { this._observe(aSubject, aTopic, aData); }
    catch (ex) { dump("observe exception: " + ex + "\n"); }
  },

  _observe: function IPD_SPS__observe(aSubject, aTopic, aData) {
    // Dispatch processing of the event.
    switch (aTopic) {
      case "quit-application" :
        this._handleAppQuit();
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // iPod device service pane nsISupports services.
  //
  //----------------------------------------------------------------------------

  QueryInterface: XPCOMUtils.generateQI(IPD_SPSCfg.ifList),


  //----------------------------------------------------------------------------
  //
  // iPod device service pane event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle application quit events.
   */

  _handleAppQuit: function IPD_SPS__handleAppQuit() {
    // Finalize the services.
    this._finalize();
  },


  //----------------------------------------------------------------------------
  //
  // iPod device service pane services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Initialize the iPod device service pane services.
   */

  _initialize: function IPD_SPS__initialize() {
    // Initialize the device lists and maps.
    this._devInfoList = {};

    // Get the locale string bundle.
    var stringBundleSvc = Cc["@mozilla.org/intl/stringbundle;1"]
                            .getService(Ci.nsIStringBundleService);
    this._locale = stringBundleSvc.createBundle(this._cfg.localeBundlePath);

    // Set up observers.
    this._observerSvc = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
    this._observerSvc.addObserver(this, "quit-application", false);

    // Get the library manager.
    this._libMgr = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                     .getService(Ci.sbILibraryManager);

    // Get the device manager and device and library service pane services.
    this._devMgr = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                     .getService(Ci.sbIDeviceManager2);
    this._devServicePaneSvc = Cc["@songbirdnest.com/servicepane/device;1"]
                                .getService(Ci.sbIDeviceServicePaneService);
    this._libServicePaneSvc = Cc["@songbirdnest.com/servicepane/library;1"]
                                .getService(Ci.sbILibraryServicePaneService);

    // Initialize the sbIMediaListListener services.
    this._sbIMLL_Initialize();

    // Add device event listener.
    this._devMgr.addEventListener(this);

    // Add all connected devices.
    this._addAllConnectedDevices();
  },


  /**
   * \brief Finalize the iPod device service pane services.
   */

  _finalize: function IPD_SPS__finalize() {
    // Remove observers.
    this._observerSvc.removeObserver(this, "quit-application");

    // Remove device event listener.
    this._devMgr.removeEventListener(this);

    // Finalize the sbIMediaListListener services.
    this._sbIMLL_Finalize();

    // Remove all devices.
    this._removeAllDevices();

    // Clear the device lists
    this._devInfoList = null;

    // Clear object references.
    this._observerSvc = null;
    this._devMgr = null;
    this._servicePaneSvc = null;
    this._libServicePaneSvc = null;
  },

  /**
   * \brief Return the device info object for a given device library ID
   *
   * \param aDevLibid The device library ID to search for
   */
  _findDevInfoForDevLib : function sbMSC_SPS_servicePaneInit(aDevLibId) {
    for (prop in this._devInfoList) {
      var devInfo = this._devInfoList[prop];
      if (devInfo && devInfo.devLibId == aDevLibId) {
        return devInfo;
      }
    }
    return null;
  },
  
  /**
   * \brief Add the device specified by aDevice to the service pane.
   *
   * \param aDevice             The device object.
   */

  _addDevice: function IPD_SPS__addDevice(aDevice) {
    // Do nothing if device has already been added.
    var deviceID = aDevice.id;
    if (this._devInfoList[deviceID])
      return;

    // Do nothing if device is not an iPod.
    if (aDevice.parameters.getProperty("DeviceType") != "iPod")
      return;

    // Do nothing if device is not connected.
    if (!aDevice.connected)
      return;

    // Create a device info list entry.
    this._devInfoList[deviceID] = {};
    this._devInfoList[deviceID].playlistList = {};
    this._devInfoList[deviceID].device = aDevice;

    // Add a device node in the service pane.
    var devNode = this._devServicePaneSvc.createNodeForDevice2(aDevice, true);
    devNode.setAttributeNS(IPODSP_NS, "device-id", deviceID);
    devNode.contractid = this._cfg.contractID;
    devNode.name = aDevice.properties.friendlyName;
    devNode.image = this._cfg.devImgURL;
    devNode.hidden = false;
    devNode.url = this._cfg.devMgrURL + "?device-id=" + deviceID;
    devNode.editable = true;
    this._devInfoList[deviceID].svcPaneNode = devNode;

    // Fill device node context menu with default items.
    this._devServicePaneSvc.setFillDefaultContextMenu(devNode, true);

    // Add the device library.  Use the raw library instead of the
    // sbIDeviceLibrary since the sbIDeviceLibrary is mostly useless in the
    // device removal events.
    var devLib;
    try
    {
      devLib = aDevice.content.libraries.queryElementAt(0, Ci.sbILibrary);
      devLib = this._libMgr.getLibrary(devLib.guid);
    } catch (ex) {}
    if (devLib)
      this._addDevLib(devLib, aDevice);

    // Update the device playlists.
    if (devLib)
      this._updateDevPlaylists(devLib);
  },


  /**
   * \brief Remove the device specified by aDevice from the service pane.
   *
   * \param aDevice             The device object.
   */

  _removeDevice: function IPD_SPS__removeDevice(aDevice) {
    // Get the device info.  Do nothing if no device info available.
    var device = aDevice.QueryInterface(Ci.sbIDevice);
    var deviceID = device.id;
    var devInfo = this._devInfoList[deviceID];
    if (!devInfo)
      return;

    // Remove the device library.
    if (devInfo.devLibId)
      this._removeDevLib(aDevInfo);

    // Remove the device node.
    if (devInfo.svcPaneNode) {
      this._servicePaneSvc.removeNode(devInfo.svcPaneNode);
    }

    // Remove device info list entry.
    delete this._devInfoList[deviceID];
  },


  /**
   * \brief Add all connected devices to the service pane.
   */

  _addAllConnectedDevices: function IPD_SPS__addAllConnectedDevices() {
    var deviceList = ArrayConverter.JSArray(this._devMgr.devices);
    for each (device in deviceList) {
      this._addDevice(device.QueryInterface(Ci.sbIDevice));
    }
  },


  /**
   * \brief Remove all devices from the service pane.
   */

  _removeAllDevices: function IPD_SPS__removeAllDevices() {
    // Remove all devices.
    for (var deviceID in this._devInfoList)
      this._removeDevice(this._devInfoList[deviceID].device);
  },


  /**
   * \brief Add the device library specified by aLibrary for the device
   *        specified by aDevice to the list of device libraries.
   *
   * \param aLibrary            Device library to add.
   * \param aDevice             Device for which to add library.
   */

  _addDevLib: function IPD_SPS__addDevLib(aLibrary, aDevice) {
    // Get the device ID.
    var deviceID = aDevice.id;

    // Save of the device library's ID.
    this._devInfoList[deviceID].devLibId = aLibrary.guid;

    // Create the device library service pane node.
    var devLibNode = this._devServicePaneSvc
                         .createLibraryNodeForDevice(aDevice, aLibrary);
    this._devInfoList[deviceID].devLibNode = devLibNode;

    // Fill device library node context menu with default items.
    this._devServicePaneSvc.setFillDefaultContextMenu(devLibNode, true);

    // Set the read-only property for the device library node.
    this._setNodeReadOnly(aDevice, devLibNode);

    // Add a listener for the device library.
    aLibrary.addListener(this,
                         false,
                         Ci.sbIMediaList.LISTENER_FLAGS_ALL &
                           ~Ci.sbIMediaList.LISTENER_FLAGS_ITEMUPDATED);
  },


  /**
   * \brief Remove the device library specified by aLibrary from the list of
   *        device libraries.
   *
   * \param aLibrary            Device library to remove.
   */

  _removeDevLib: function IPD_SPS__removeDevLib(aDevInfo) {

    // Get library from GUID.  It may not be available if it's already been
    // removed.
    var library;
    try {
      library = this._libMgr.getLibrary(aDevInfo.devLibId);
      // Remove the device library listener
      if (library) {
        library.removeListener(this);
      }
    } catch (ex) {}
  },


  /**
   * \brief Add the device playlist specified by aMediaList to the list of
   *        device playlists.
   *
   * \param aMediaList          Device playlist to add.
   */

  _addDevPlaylist: function IPD_SPS__addDevPlaylist(aMediaList) {
    // Get the device info and playlist URN.
    var devInfo = this._findDevInfoForDevLib(aMediaList.library.guid);
    var playlistURN = this._cfg.itemURNPrefix + aMediaList.guid;

    var device = devInfo.device;

    // Do nothing if playlist already added.
    if (devInfo.playlistList[playlistURN])
      return;

    // Get the playlist service pane node, creating one if necessary.
    var playlistNode = this._libServicePaneSvc
                           .getNodeForLibraryResource(aMediaList);
    if (!playlistNode) {
      playlistNode = this._servicePaneSvc.addNode(playlistURN,
                                                  this._servicePaneSvc.root,
                                                  true);
    }

    // Set the read-only property for the playlist node.
    this._setNodeReadOnly(device, playlistNode);

    // Move the playlist service pane node underneath the device service pane
    // node.
    devInfo.svcPaneNode.appendChild(playlistNode);

    // Add the playlist to the device playlist list.
    devInfo.playlistList[playlistURN] = playlistNode;
  },


  /**
   * \brief Remove the device playlist specified by aMediaList from the list of
   *        device playlists.
   *
   * \param aMediaList          Device playlist to remove.
   */

  _removeDevPlaylist: function IPD_SPS__removeDevPlaylist(aMediaList) {
    // Remove the playlist from the device playlist list.
    var devInfo = this._findDevInfoForDevLib(aMediaList.library.guid);
    var playlistURN = this._cfg.itemURNPrefix + aMediaList.guid;
    delete devInfo.playlistList[playlistURN];
  },


  /**
   * \brief Update the device state in the service pane for the device specified
   *        by aDevice.
   *
   * \param aDevice             Device for which to update state.
   */

  _updateDevState: function IPD_SPS__updateDevState(aDevice) {
    // Get the device node.
    if (!this._devInfoList[aDevice.id])
      return;
    var devNode = this._devInfoList[aDevice.id].svcPaneNode;

    // Get the list of device node properties.
    devNodePropList = devNode.properties.split(" ");

    // Update the device node busy property.
    devNodePropList = devNodePropList.filter
                        (function(aProperty) { return aProperty != "busy"; });
    if (aDevice.state != Ci.sbIDevice.STATE_IDLE)
      devNodePropList.push("busy");

    // Determine whether or not the device is read-only
    var deviceProperties = aDevice.properties.properties;
    var accessCompatibility = deviceProperties.getPropertyAsAString(
          "http://songbirdnest.com/device/1.0#accessCompatibility");
    if (accessCompatibility == "ro") {
      devNodePropList = devNodePropList.filter
                  (function(aProperty) { return aProperty != "read-only"; });
      devNodePropList.push("read-only");
    }

    // Update the device node image.
    if (aDevice.state == Ci.sbIDevice.STATE_IDLE)
      devNode.image = this._cfg.devImgURL;
    else
      devNode.image = null;

    // Write the device node properties.
    devNode.properties = devNodePropList.join(" ");
  },


  /**
   * \brief Update the device info in the service pane for the device specified
   *        by aDevice.
   *
   * \param aDevice             Device for which to update info.
   */

  _updateDevInfo: function IPD_SPS__updateDevInfo(aDevice) {
    
    var devInfo = this._devInfoList[aDevice.id];

    // Not our device, let someone else handle it.
    if(!devInfo) {
      return;
    }

    devInfo.svcPaneNode.name = aDevice.properties.friendlyName;
  },


  /**
   * \brief Update the list of device playlists with the playlists contained in
   *        the media list specified by aMediaList.
   *
   * \param aMediaList          Media list from which to update.
   */

  _updateDevPlaylists: function IPD_SPS__updateDevPlaylists(aMediaList) {
    // Set up an enumeration listener to collect items into a simple array.
    var enumListener = {
      itemList: [],
      onEnumerationBegin: function() { },
      onEnumerationEnd: function() { },
      onEnumeratedItem: function(aList, aItem) {
        this.itemList.push(aItem);
      }
    };

    // Create a list of media lists.
    aMediaList.enumerateItemsByProperty(SBProperties.isList, "1", enumListener);

    // Add each media list as a device playlist.
    var length = enumListener.itemList.length;
    for (var i = 0; i < length; i++) {
      this._addDevPlaylist(enumListener.itemList[i]);
    }
  },


  /**
   * \brief Set the read-only property for the node specified by aNode as
   *        appropriate for the current state of the device specified by
   *        aDevice.
   *
   * \param aDevice             Device for which to set node read-only property.
   * \param aNode               Node for which to set read-only property.
   */

  _setNodeReadOnly: function IPD_SPS__setNodeReadOnly(aDevice, aNode) {
  //XXXeps implement
  }
};


//------------------------------------------------------------------------------
//
// iPod device service pane services component services.
//
//------------------------------------------------------------------------------

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([IPD_SPS]);
}

