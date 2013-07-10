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

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/DOMUtils.jsm");
Cu.import("resource://app/jsmodules/PlatformUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");

const CDRIPNS = 'http://songbirdnest.com/rdf/servicepane/cdrip#';
const SPNS = 'http://songbirdnest.com/rdf/servicepane#';

// For some device events we need to check if they are from the CD Device
// marshall and not others.
const CDDEVICEMARSHALLNAME = "sbCDDeviceMarshall";

var sbCDRipServicePaneServiceConfig = {
  className:      "Songbird CD Rip Device Support Module",
  cid:            Components.ID("{9925b565-5c19-4feb-87a8-413d86570cd9}"),
  contractID:     "@songbirdnest.com/servicepane/cdDevice;1",
  
  ifList: [ Ci.sbIServicePaneModule,
            Ci.nsIObserver ],
            
  categoryList:
  [
    {
      category: "service-pane",
      entry: "cdrip-device"
    }
  ],

  devCatName:     "CD Rip Device",

  appQuitTopic:   "quit-application",

  devMgrURL:      "chrome://songbird/content/mediapages/cdripMediaView.xul"
};
if ("sbIWindowsAutoPlayActionHandler" in Ci) {
  sbCDRipServicePaneServiceConfig.ifList.push(Ci.sbIWindowsAutoPlayActionHandler);
}

function sbCDRipServicePaneService() {

}

sbCDRipServicePaneService.prototype.constructor = sbCDRipServicePaneService;

sbCDRipServicePaneService.prototype = {
  classDescription: sbCDRipServicePaneServiceConfig.className,
  classID: sbCDRipServicePaneServiceConfig.cid,
  contractID: sbCDRipServicePaneServiceConfig.contractID,

  _cfg: sbCDRipServicePaneServiceConfig,
  _xpcom_categories: sbCDRipServicePaneServiceConfig.categoryList,

  // Services to use.
  _deviceManagerSvc:      null,
  _deviceServicePaneSvc:  null,
  _observerSvc:           null,
  _servicePaneSvc:        null,

  _deviceInfoList:        [],
  
  _deviceScanInProgress:  false,
  
  // ************************************
  // sbIServicePaneService implementation
  // ************************************
  servicePaneInit: function sbCDRipServicePaneService_servicePaneInit(aServicePaneService) {
    this._servicePaneSvc = aServicePaneService;
    this._initialize();
  },

  fillContextMenu: function sbCDRipServicePaneService_fillContextMenu(aNode,
                                                                  aContextMenu,
                                                                  aParentWindow) {
    // Get the node device ID.  Do nothing if not a device node.
    var deviceID = aNode.getAttributeNS(CDRIPNS, "DeviceId");
    if (!deviceID)
      return;
  
    // Get the device node type.
    var deviceNodeType = aNode.getAttributeNS(CDRIPNS, "deviceNodeType");

    // Import device context menu items into the context menu.
    if (deviceNodeType == "cd-device") {
      DOMUtils.importChildElements(aContextMenu,
                                   this._deviceContextMenuDoc,
                                   "cddevice_context_menu_items",
                                   { "device-id": deviceID,
                                     "service_pane_node_id": aNode.id });
    }
  },

  fillNewItemMenu: function sbCDRipServicePaneService_fillNewItemMenu(aNode,
                                                                  aContextMenu,
                                                                  aParentWindow) {
  },

  onSelectionChanged: function sbCDRipServicePaneService_onSelectionChanged(aNode,
                                                          aContainer,
                                                          aParentWindow) {
  },

  canDrop: function sbCDRipServicePaneService_canDrop(aNode, 
                                                  aDragSession, 
                                                  aOrientation, 
                                                  aWindow) {
    // Currently no drag and drop allowed
    return false;
  },

  onDrop: function sbCDRipServicePaneService_onDrop(aNode, 
                                                aDragSession, 
                                                aOrientation, 
                                                aWindow) {
    // Currently no drag and drop allowed
  },
  
  onDragGesture: function sbCDRipServicePaneService_onDragGesture(aNode, 
                                                              aDataTransfer) {
    // Currently no drag and drop allowed
  },

  onBeforeRename: function sbCDRipServicePaneService_onBeforeRename(aNode) {
    // Rename is not allowed for CD Devices
  },

  onRename: function sbCDRipServicePaneService_onRename(aNode, 
                                                    aNewName) {
    // Rename is not allowed for CD Devices
  },

  shutdown: function sbCDRipServicePaneService_shutdown() {
    // Do nothing, since we shut down on quit-application
  },

  // ************************************
  // nsIObserver implementation
  // ************************************
  observe: function sbCDRipServicePaneService_observe(aSubject, 
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
  QueryInterface: XPCOMUtils.generateQI(sbCDRipServicePaneServiceConfig.ifList),

  // ************************************
  // Internal methods
  // ************************************
  
  /**
   * \brief Initialize the CD Device nodes.
   */
  _initialize: function sbCDRipServicePaneService_initialize() {
    this._observerSvc = Cc["@mozilla.org/observer-service;1"]
                          .getService(Ci.nsIObserverService);
    
    this._observerSvc.addObserver(this, this._cfg.appQuitTopic, false);

    this._deviceServicePaneSvc = Cc["@songbirdnest.com/servicepane/device;1"]
                                   .getService(Ci.sbIDeviceServicePaneService);

    this._deviceManagerSvc = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                               .getService(Ci.sbIDeviceManager2);
 
    // Add a listener for CDDevice Events
    var deviceEventListener = {
      cdDeviceServicePaneSvc: this,
      
      onDeviceEvent: function deviceEventListener_onDeviceEvent(aDeviceEvent) {
        this.cdDeviceServicePaneSvc._processDeviceManagerEvent(aDeviceEvent);
      }
    };
    
    this._deviceEventListener = deviceEventListener;
    this._deviceManagerSvc.addEventListener(deviceEventListener);
    
    // load the cd-device context menu document
    this._deviceContextMenuDoc =
          DOMUtils.loadDocument
            ("chrome://songbird/content/xul/device/deviceContextMenu.xul");

    if (PlatformUtils.platformString == "Windows_NT") {
      // Register autoplay handler
      var autoPlayActionHandler = {
        cdDeviceServicePaneSvc: this,

        handleAction: function autoPlayActionHandler_handleAction(aAction, aActionArg) {
          return this.cdDeviceServicePaneSvc._processAutoPlayAction(aAction);
        },

        QueryInterface: XPCOMUtils.generateQI([Ci.sbIWindowsAutoPlayActionHandler])
      }

      this._autoPlayActionHandler = autoPlayActionHandler;
      var windowsAutoPlayService =
            Cc["@songbirdnest.com/Songbird/WindowsAutoPlayService;1"]
              .getService(Ci.sbIWindowsAutoPlayService);
      windowsAutoPlayService.addActionHandler
        (autoPlayActionHandler,
         Ci.sbIWindowsAutoPlayService.ACTION_CD_RIP);
    }
  },
  
  /**
   * \brief Shutdown and remove the CD Device nodes.
   */
  _shutdown: function sbCDRipServicePaneService_shutdown() {
    this._observerSvc.removeObserver(this, this._cfg.appQuitTopic);

    this._deviceManagerSvc.removeEventListener(this._deviceEventListener);
    this._deviceEventListener = null;
    
    if (PlatformUtils.platformString == "Windows_NT") {
      // Unregister autoplay handler
      var windowsAutoPlayService =
            Cc["@songbirdnest.com/Songbird/WindowsAutoPlayService;1"]
              .getService(Ci.sbIWindowsAutoPlayService);
      windowsAutoPlayService.removeActionHandler
        (this._autoPlayActionHandler,
         Ci.sbIWindowsAutoPlayService.ACTION_CD_RIP);
      this._autoPlayActionHandler = null;
    }

    // Remove all references to nodes
    this._deviceInfoList = [];

    this._deviceManagerSvc = null;
    this._deviceServicePaneSvc = null;
    this._servicePaneSvc = null;
    this._observerSvc = null;
  },

  /**
   * \brief Process events from the Device Manager.
   * \param aDeviceEvent - Event that occured for a device.
   */
  _processDeviceManagerEvent:
    function sbCDRipServicePaneService_processDeviceManagerEvent(aDeviceEvent) {

    switch(aDeviceEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_SCAN_START:
        var marshall = aDeviceEvent.origin.QueryInterface(Ci.sbIDeviceMarshall);
        if (marshall.name == CDDEVICEMARSHALLNAME)
          this._deviceScanInProgress = true;
        break;
      
      case Ci.sbIDeviceEvent.EVENT_DEVICE_SCAN_END:
        var marshall = aDeviceEvent.origin.QueryInterface(Ci.sbIDeviceMarshall);
        if (marshall.name == CDDEVICEMARSHALLNAME)
          this._deviceScanInProgress = false;
        break;
      
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED:
        var result = this._addDeviceFromEvent(aDeviceEvent);

        // if we successfully added the device, switch the media tab
        // to the CD rip view as long as this is not during the initial scan
        // of CD Devices.
        if (result)
          this._loadCDViewFromEvent(aDeviceEvent);
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED:
        this._removeDeviceFromEvent(aDeviceEvent);
        break;

      case Ci.sbICDDeviceEvent.EVENT_CDLOOKUP_INITIATED:
        this._updateState(aDeviceEvent, true);
        break;

      case Ci.sbICDDeviceEvent.EVENT_CDLOOKUP_COMPLETED:
        this._updateState(aDeviceEvent, false);
        break;

      case Ci.sbICDDeviceEvent.EVENT_CDLOOKUP_METADATA_COMPLETE:
        this._updateState(aDeviceEvent, false);
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        this._updateState(aDeviceEvent, false);
        break;

      default:
        break;
    }
  },

  /**
   * \brief Handles autoplay actions initiated by the user.
   * \param aAction - Action to be handled.
   */
  _processAutoPlayAction:
      function sbCDRipServicePaneService_processAutoPlayAction(aAction) {

    switch (aAction) {
      case Ci.sbIWindowsAutoPlayService.ACTION_CD_RIP:
        // No way to tell which CD the user meant to rip, let's take one randomly
        // (hopefully the last one enumerated is also the last one added)
        var deviceNode = null;
        for each (let deviceInfo in this._deviceInfoList) {
          deviceNode = deviceInfo.svcPaneNode;
        }

        if (deviceNode) {
          Cc['@mozilla.org/appshell/window-mediator;1']
            .getService(Ci.nsIWindowMediator)
            .getMostRecentWindow('Songbird:Main').gBrowser
            .loadURI(deviceNode.url, null, null, null, "_media");
        }
        else {
          // CD is probably not recognized yet, don't do anything - the view
          // will switch to it anyway once it is.
        }

        return true;

      default:
        return false;
    }
  },

  /**
   * \brief Load the CD Rip media view as response to a device event
   * \param aDeviceEvent - Device event being acted upon.
   */
  _loadCDViewFromEvent:
      function sbCDRipServicePaneService_loadCDViewFromEvent(aDeviceEvent) {

    // We only want to do this if it is not during a device scan (usually at
    // startup).
    if (this._deviceScanInProgress)
      return;

    var device = aDeviceEvent.data.QueryInterface(Ci.sbIDevice);
    var url = this._cfg.devMgrURL + "?device-id=" + device.id;
    Cc['@mozilla.org/appshell/window-mediator;1']
      .getService(Ci.nsIWindowMediator)
      .getMostRecentWindow('Songbird:Main').gBrowser
      .loadURI(url, null, null, null, "_media");
  },

  /**
   * \brief Updates the state of the service pane node.
   * \param aDeviceEvent - Device event of device.
   */
  _updateState: function sbCDRipServicePaneService__updateState(aDeviceEvent,
                                                                aForceBusy) {
    // Get the device and its node.
    var device = aDeviceEvent.origin.QueryInterface(Ci.sbIDevice);
    var deviceId = device.id;
    var deviceType = device.parameters.getProperty("DeviceType");

    // We only care about CD devices
    if (deviceType != "CD")
      return;

    if (device.state == Ci.sbIDevice.STATE_TRANSCODE) {
      this._toggleReadOnly(true, device);
    }
    else if (device.state == Ci.sbIDevice.STATE_IDLE) {
      this._toggleReadOnly(false, device);
    }

    if (typeof(this._deviceInfoList[deviceId]) != 'undefined') {
      var devNode = this._deviceInfoList[deviceId].svcPaneNode;

      // The friendly name might have changed, keep it in sync.
      if (devNode.name != device.properties.friendlyName) {
        devNode.name = device.properties.friendlyName;
      }

      // Get the device properties and clear the busy property.
      devProperties = devNode.className.split(" ");
      devProperties = devProperties.filter(function(aProperty) {
                                             return aProperty != "busy";
                                           });

      // Set the busy property if the device is busy.
      if ((device.state != Ci.sbIDevice.STATE_CANCEL) && 
          (aForceBusy || device.state != Ci.sbIDevice.STATE_IDLE)) {
          // Clear success state from previous rip
        devProperties = devProperties.filter(function(aProperty) {
                                               return aProperty != "successful" &&
                                                      aProperty != "unsuccessful";
                                             });
        devProperties.push("busy");
      } else {
        if (devNode.hasAttributeNS(CDRIPNS, "LastState")) {
          var lastState = devNode.getAttributeNS(CDRIPNS, "LastState");
          if (lastState == Ci.sbIDevice.STATE_TRANSCODE) {
            if (this._checkErrors(device)) {
              devProperties.push("unsuccessful");
            } else if (this._checkSuccess(device)){
              devProperties.push("successful");
            }
            //if neither _checkErrors nor _checkSuccess is true, then the user
            //cancelled before any successes or fails.  Go back to idle image.
          }
        }
      }
      devNode.setAttributeNS(CDRIPNS, "LastState", device.state);

      // Write back the device node properties.
      devNode.className = devProperties.join(" ");
    }
  },
 
  /**
   * Get the devices library (defaults to first library)
   * \param aDevice - Device to get library from
   */
  _getDeviceLibrary: function sbCDRipServicePaneService__getDeviceLibrary(aDevice) {
    // Get the libraries for device
    var libraries = aDevice.content.libraries;
    if (libraries.length < 1) {
      // Oh no, we have no libraries
      Cu.reportError("Device " + aDevice.id + " has no libraries!");
      return null;
    }

    // Get the requested library
    var deviceLibrary = libraries.queryElementAt(0, Ci.sbIMediaList);
    if (!deviceLibrary) {
      Cu.reportError("Unable to get library for device: " + aDevice.id);
      return null;
    }
    
    return deviceLibrary;
  },
  
  /**
   * Turn the read only flag for the library on or off. This allows us to disable
   * changes to the CD Device library during a rip.
   * \param aReadOnly - Mark as read only (Disable changes) if true.
   * \param aDevice - CD Device to toggle read only.
   */
  _toggleReadOnly: function sbCDRipServicePaneService__toggleReadOnly(aReadOnly,
                                                                      aDevice) {
    var deviceLibrary = this._getDeviceLibrary(aDevice);
    if (deviceLibrary)
      deviceLibrary.setProperty(SBProperties.isReadOnly,
                                (aReadOnly ? "1" : "0"));
  },

  /**
   * Check for any ripped tracks that failed
   * \param aDevice - Device to check
   * \return True if errors occured, false otherwise
   */
  _checkErrors: function sbCDRipServicePaneService__checkErrors(aDevice) {
    // Check for any tracks that have a failed status
    var deviceLibrary = this._getDeviceLibrary(aDevice);
    var errorCount = 0;

    try {
      // Get all the did not successfully ripped tracks
      var propArray =
        Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
          .createInstance(Ci.sbIMutablePropertyArray);
      propArray.appendProperty(SBProperties.cdRipStatus, "3|100");
      propArray.appendProperty(SBProperties.shouldRip, "1");
      
      var rippedItems = deviceLibrary.getItemsByProperties(propArray);
      errorCount = rippedItems.length;
    }
    catch (err if err.result == Cr.NS_ERROR_NOT_AVAILABLE) {
      // deviceLibrary.getItemsByProperties() will throw NS_ERROR_NOT_AVAILABLE
      // if there are no failed rips in the list, thus no errors.
      return false;
    }
    catch (err) {
      Cu.reportError("ERROR GETTING TRANSCODE ERROR COUNT " + err);
    }
    
    return (errorCount > 0);
  },
  
  /**
   * Check if any tracks successfully ripped
   * \param aDevice - Device to check
   * \return True if at least one track was ripped, false otherwise
   */
  _checkSuccess: function sbCDRipServicePaneService__checkErrors(aDevice) {
    // Check if any tracks were successfully ripped
    var deviceLibrary = this._getDeviceLibrary(aDevice);
    var successCount = 0;

    try {
      var propArray = Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
          .createInstance(Ci.sbIMutablePropertyArray);
      propArray.appendProperty(SBProperties.cdRipStatus, "2|100");
      propArray.appendProperty(SBProperties.shouldRip, "1");

      var rippedItems = deviceLibrary.getItemsByProperties(propArray);
      successCount = rippedItems.length;
    }
    catch (err if err.result == Cr.NS_ERROR_NOT_AVAILABLE) {
      // deviceLibrary.getItemsByProperties() will throw NS_ERROR_NOT_AVAILABLE
      // if there are no successful rips in the list.
      return false;
    }
    catch (err) {
      Cu.reportError("ERROR GETTING TRANSCODE SUCCESS COUNT " + err);
    }

    return (successCount > 0);
  },
  
  /**
   * \brief Add a device that has media from event information.
   * \param aDeviceEvent - Device event of added device.
   */
  _addDeviceFromEvent:
    function sbCDRipServicePaneService_addDeviceFromEvent(aDeviceEvent) {

    var device = aDeviceEvent.data.QueryInterface(Ci.sbIDevice);
    var deviceType = device.parameters.getProperty("DeviceType");

    // We only care about CD devices
    if (deviceType != "CD")
      return false;

    try {
      this._addDevice(device);
    }
    catch(e) {
      Cu.reportError(e);
      return false;
    }
    return true;
  },
  
  /**
   * \brief Remove a device that no longer has media from event information.
   * \param aDeviceEvent - Device event of removed device.
   */
  _removeDeviceFromEvent:
    function sbCDRipServicePaneService_removeDeviceFromEvent(aDeviceEvent) {

    var device = aDeviceEvent.data.QueryInterface(Ci.sbIDevice);
    var deviceType = device.parameters.getProperty("DeviceType");

    // We only care about CD devices
    if (deviceType != "CD")
      return;

    try {
      this._removeDevice(device);
    }
    catch(e) {
      Cu.reportError(e);
    }
  },
  
  /**
   * \brief Add a device that has media.
   * \param aDevice - Device to add, this will only be added if it is of type
   *        "CD".
   */
  _addDevice: function sbCDRipServicePaneService_addDevice(aDevice) {
    var device = aDevice.QueryInterface(Ci.sbIDevice);
    var devId = device.id;
    
    // Do nothing if device is not an CD device.
    var deviceType = device.parameters.getProperty("DeviceType");
    if (deviceType != "CD") {
      return;
    }
    
    // Add a cd rip node in the service pane.
    var devNode = this._deviceServicePaneSvc.createNodeForDevice2(device, true);
    devNode.setAttributeNS(CDRIPNS, "DeviceId", devId);
    devNode.setAttributeNS(CDRIPNS, "deviceNodeType", "cd-device");
    devNode.className = "cd-device";
    devNode.contractid = this._cfg.contractID;
    devNode.url = this._cfg.devMgrURL + "?device-id=" + devId;
    devNode.editable = false;
    devNode.name = device.properties.friendlyName;

    this._deviceInfoList[devId] = {svcPaneNode: devNode};
  },
  
  /**
   * \brief Remove a known CD Device from the service pane.
   * \param aDevice - Device to remove.
   */
  _removeDevice: function sbCDRipServicePaneService_removeDevice(aDevice) {
    var device = aDevice.QueryInterface(Ci.sbIDevice);
    var devId = device.id;
    
    var devInfo = this._deviceInfoList[devId];
    if (!devInfo) {
      return;
    }

    // Remove the device node.
    devInfo.svcPaneNode.parentNode.removeChild(devInfo.svcPaneNode);

    // Remove device info list entry.
    delete this._deviceInfoList[devId];
  }
};

// Instantiate an XPCOM module.
var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbCDRipServicePaneService]);
