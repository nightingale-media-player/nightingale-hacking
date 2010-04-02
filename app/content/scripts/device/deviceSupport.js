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
 * \file  deviceSupport.js
 * \brief Javascript source for device support.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device support services.
//
//   These services provide device support for the main Songbird window.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device support imported services.
//
//------------------------------------------------------------------------------

// Component manager defs.
var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;
var Cu = Components.utils;

// Songbird imports.
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/DOMUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/sbStorageFormatter.jsm");


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device volume support services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

var sbDeviceVolumeSupport = {
  //
  // Device volume support services configuration.
  //
  //   volumeListStabilizationTime  Amount of time in milliseconds to wait for
  //                                a device volume list to stabilize.
  //

  volumeListStabilizationTime: 1000,


  //
  // Device volume support services fields.
  //
  //   _deviceInfoTable         Table of device info.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _deviceManager           Device manager.
  //

  _deviceInfoTable: null,
  _domEventListenerSet: null,
  _deviceManager: null,


  //----------------------------------------------------------------------------
  //
  // Device volume support services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the device volume support services.
   */

  initialize: function sbDeviceVolumeSupport_initialize() {
    var _this = this;
    var func;

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Finalize when the window unloads.
    func = function onUnload(aEvent) { _this.finalize(); };
    this._domEventListenerSet.add(window, "unload", func, false, true);

    // Continue initializing after the main window loads.
    func = function onLoad(aEvent) { _this._initialize(); };
    this._domEventListenerSet.add(window, "load", func, false, true);
  },

  _initialize: function sbDeviceVolumeSupport__initialize() {
    // Initialize the device info table.
    this._deviceInfoTable = {};

    // Get the device manager.
    this._deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                            .getService(Ci.sbIDeviceEventTarget)
                            .QueryInterface(Ci.sbIDeviceRegistrar);

    // Listen to all device events.
    this._deviceManager.addEventListener(this);

    // Add each device.
    var deviceRegistrar = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                            .getService(Ci.sbIDeviceRegistrar);
    for each (device in ArrayConverter.JSEnum(deviceRegistrar.devices)) {
      this._addDevice(device.QueryInterface(Ci.sbIDevce));
    }
  },


  /**
   * Finalize the device volume support services.
   */

  finalize: function sbDeviceVolumeSupport_finalize() {
    // Stop listening to device events.
    if (this._deviceManager)
      this._deviceManager.removeEventListener(this);

    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }

    // Remove all devices.
    this._removeAllDevices();

    // Remove all references.
    this._deviceInfoTable = null;
    this._domEventListenerSet = null;
    this._deviceManager = null;
  },


  //----------------------------------------------------------------------------
  //
  // Device volume support sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the device event specified by aEvent.
   *
   * \param aEvent              Device event.
   */

  onDeviceEvent: function deviceVolumeSupport_onDeviceEvent(aEvent) {
    // Dispatch processing of event.
    switch (aEvent.type)
    {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED :
        this._addDevice(aEvent.data.QueryInterface(Ci.sbIDevice));
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED :
        this._removeDevice(aEvent.data.QueryInterface(Ci.sbIDevice));
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_LIBRARY_ADDED :
      case Ci.sbIDeviceEvent.EVENT_DEVICE_LIBRARY_REMOVED :
        this._monitorDeviceVolumes(aEvent.origin.QueryInterface(Ci.sbIDevice));
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal device volume support services.
  //
  //----------------------------------------------------------------------------

  /**
   * Add the device specified by aDevice to the list of devices managed by the
   * device volume support services.
   *
   * \param aDevice             Device to add.
   */

  _addDevice: function sbDeviceVolumeSupport__addDevice(aDevice) {
    // Do nothing if device already added.
    if (aDevice.id in this._deviceInfoTable)
      return;

    // Create a device info data record.
    var deviceInfo = { device: aDevice };

    // Add the device info to the device info table.
    this._deviceInfoTable[aDevice.id] = deviceInfo;

    // Monitor device volumes.
    this._monitorDeviceVolumes(deviceInfo);
  },


  /**
   * Remove the device specified by aDeviceSpec from the list of devices managed
   * by the device volume support services.
   *
   * \param aDeviceSpec         Device to remove.  Either sbIDevice or a device
   *                            info data record.
   */

  _removeDevice: function sbDeviceVolumeSupport__removeDevice(aDeviceSpec) {
    // Get the device info data record from the device spec.  Return if no
    // device info data record is available.
    var deviceInfo;
    if (aDeviceSpec instanceof Ci.sbIDevice) {
      deviceInfo = this._deviceInfoTable[aDeviceSpec.id];
    }
    else {
      deviceInfo = aDeviceSpec
    }
    if (!deviceInfo)
      return;

    // Remove the device info from the device info table.
    delete this._deviceInfoTable[deviceInfo.device.id];

    // Clear any volume monitor timeouts.
    if (deviceInfo.monitorVolumesTimeout) {
      clearTimeout(deviceInfo.monitorVolumesTimeout);
      deviceInfo.monitorVolumesTimeout = null;
    }
  },


  /**
   * Remove all devices from the list of devices managed by the device volume
   * support services.
   */

  _removeAllDevices: function sbDeviceVolumeSupport__removeAllDevices() {
    // Remove all devices.
    var deviceInfoList = [];
    for each (deviceInfo in this._deviceInfoTable) {
      deviceInfoList.push(deviceInfo);
    }
    for each (deviceInfo in deviceInfoList) {
      this._removeDevice(deviceInfo);
    }
  },


  /**
   * Monitor the volumes on the device specified by aDeviceSpec and take any
   * necessary actions.
   *
   * \param aDeviceSpec         Device to monitor.  Either sbIDevice or a device
   *                            info data record.
   */

  _monitorDeviceVolumes:
    function sbDeviceVolumeSupport__monitorDeviceVolumes(aDeviceSpec) {
    // Get the device info data record from the device spec.  Return if no
    // device info data record is available.
    var deviceInfo;
    if (aDeviceSpec instanceof Ci.sbIDevice) {
      deviceInfo = this._deviceInfoTable[aDeviceSpec.id];
    }
    else {
      deviceInfo = aDeviceSpec
    }
    if (!deviceInfo)
      return;

    // Check if the device volume list is stable.
    var stable = this._isDeviceVolumeListStable(deviceInfo);

    // If the device volume list is not stable, start a timer to check again
    // after a delay and return.
    if (!stable) {
      // Clear any currently running timer.
      if (deviceInfo.monitorVolumesTimeout)
        clearTimeout(deviceInfo.monitorVolumesTimeout);

      // Start a timer.
      var _this = this;
      var func = function _monitorDeviceVolumesWrapper() {
        deviceInfo.monitorVolumesTimeout = null;
        _this._monitorDeviceVolumes(deviceInfo);
      };
      deviceInfo.monitorVolumesTimeout =
                   setTimeout(func, this.volumeListStabilizationTime);

      return;
    }

    // Check for new volumes.
    this._checkForNewVolumes(deviceInfo.device);
  },


  /**
   * Return true if the list of volumes for the device specified by aDeviceInfo
   * is stable.  The list is stable if it has not changed for a set period of
   * time.
   *
   * \param aDeviceInfo         Info for device to check.
   *
   * \return true               Device volume list is stable.
   */

  _isDeviceVolumeListStable:
    function sbDeviceVolumeSupport__isDeviceVolumeListStable(aDeviceInfo) {
    // Get the current device volume list and sort it.
    var volumeList = [];
    var libraries = aDeviceInfo.device.content.libraries;
    for (var i = 0; i < libraries.length; i++) {
      var library = libraries.queryElementAt(i, Ci.sbIDeviceLibrary);
      volumeList.push(library.guid);
    }
    volumeList.sort();

    // Determine if the device volume list has changed.
    var volumeListChanged = false;
    if (!aDeviceInfo.volumeList ||
        (aDeviceInfo.volumeList.length != volumeList.length)) {
      volumeListChanged = true;
    }
    else {
      for (var i = 0; i < volumeList.length; i++) {
        if (aDeviceInfo.volumeList[i] != volumeList[i]) {
          volumeListChanged = true;
          break;
        }
      }
    }

    // If the device volume list has changed, update it and return that the
    // volume list is not stable.
    if (volumeListChanged) {
      aDeviceInfo.volumeList = volumeList;
      aDeviceInfo.volumeListTimeStamp = Date.now();
      return false;
    }

    // If the device volume list has not been stable for long enough, return
    // that the volume list is not stable.
    var currentTime = Date.now();
    if ((currentTime - aDeviceInfo.volumeListTimeStamp) <
        this.volumeListStabilizationTime) {
      return false;
    }

    // Device volume list is stable.
    return true;
  },


  /**
   * Check if any new volumes have been added to the device specified by
   * aDevice.  If any have, and there are more than one volume to be managed,
   * notify the user.
   *
   * \aDevice                   Device to check.
   */

  _checkForNewVolumes:
    function sbDeviceVolumeSupport__checkForNewVolumes(aDevice) {
    // Get the current device volume library list.
    var volumeLibraryList = ArrayConverter.JSArray(aDevice.content.libraries);

    // Just return if there are currently no volumes.  This could be because
    // the device has not yet made its volumes available.
    if (volumeLibraryList.length == 0)
      return;

    // Get the last device volume GUID list.
    var lastVolumeGUIDList = aDevice.getPreference("last_volume_list", null);
    if (lastVolumeGUIDList) {
      lastVolumeGUIDList = lastVolumeGUIDList.split(",");
    }
    else {
      lastVolumeGUIDList = [];
    }

    // Produce the list of new volume libraries.
    var newVolumeLibraryList = [];
    for each (var library in volumeLibraryList) {
      if (lastVolumeGUIDList.indexOf(library.guid) < 0)
        newVolumeLibraryList.push(library);
    }

    // Notify user of new volume if necessary.
    if ((newVolumeLibraryList.length > 0) && (volumeLibraryList.length > 1)) {
      // Use the first new volume library.
      var volumeLibrary = newVolumeLibraryList[0];

      // Get the volume capacity.
      var capacity = volumeLibrary.getProperty
                       ("http://songbirdnest.com/device/1.0#capacity");

      // Produce the notification label.
      var label = SBFormattedString("device.new_volume_notification.label",
                                    [ StorageFormatter.format(capacity),
                                      aDevice.name ]);

      // Produce the list of notification buttons.  Only add a manage volumes
      // button if the service pane is available.
      var buttonList = [];
      var _this = this;
      if (gServicePane) {
        buttonList.push({
          label: SBString("device.new_volume_notification.manage_button.label"),
          accessKey:
            SBString("device.new_volume_notification.manage_button.accesskey"),
          callback: function buttonCallback(aNotification, aButton) {
            _this._doManageVolumes(aDevice);
          }
        });
      }

      // Notify the user of the new volume.
      var notificationBox = gBrowser.getNotificationBox();
      var notification = notificationBox.appendNotification
                           (label,
                            "new_volume",
                            null,
                            notificationBox.PRIORITY_INFO_LOW,
                            buttonList);
    }

    // Update the device last volume list.
    var volumeGUIDList = [];
    for each (var library in volumeLibraryList)
      volumeGUIDList.push(library.guid);
    aDevice.setPreference("last_volume_list", volumeGUIDList.join(","));
  },


  /**
   * Navigate to the device summary page so that the user can manage their
   * volumes.
   */

  _doManageVolumes: function sbDeviceVolumeSupport__doManageVolumes(aDevice) {
    // Get the device service pane node.
    var dsps = Cc["@songbirdnest.com/servicepane/device;1"]
                 .getService(Ci.sbIDeviceServicePaneService);
    var deviceNode = dsps.getNodeForDevice(aDevice);

    // Load the device node.
    if (gServicePane) {
      gServicePane.mTreePane.selectAndLoadNode(deviceNode,
                                               null,
                                               { manageVolumes: "true" });
    }
  }
};

// Initialize the device volume support services.
sbDeviceVolumeSupport.initialize();


//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device transcode support services.
//
//   These services provide device transcode support for the main Songbird
// window.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

(function DeviceTranscodeSupport(){
  Cu.import("resource://app/jsmodules/StringUtils.jsm");
  Cu.import("resource://app/jsmodules/WindowUtils.jsm");

  // A set of devices with events. The key is the device id; only the
  // presence of the key matters.
  var devicesWithEvents = {};

  function deviceManagerListener(event) {

    var device = event.origin;
    if (!(device instanceof Ci.sbIDevice)) {
      return;
    }

    var msg = SBString("transcode.error.notification.label");

    switch (event.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSCODE_ERROR: {
        let bag = event.data;
        if (!bag || !(bag instanceof Ci.nsIPropertyBag2)) {
          // no event data, nothing we can do
          break;
        }
        if (bag.hasKey("transcode-error")) {
          let error = bag.get("transcode-error");
          if ((error instanceof Ci.nsIScriptError) &&
              (error instanceof Ci.sbITranscodeError))
          {
            devicesWithEvents[device.id] = true;
          }
        }
        break;
      }
      case Ci.sbIDeviceEvent.EVENT_DEVICE_NOT_ENOUGH_FREESPACE: {
        devicesWithEvents[device.id] = true;
        break;
      }
    }

    if (device.isBusy) {
      // The device is still busy; wait until it becomes idle before reporting
      return;
    }
    if (!(device.id in devicesWithEvents)) {
      // this device has no events we want to report
      return;
    }

    // If we get here, there is some unreported but interesting event,
    // but the device is now idle
    delete devicesWithEvents[device.id];
    var notificationBox = gBrowser.getNotificationBox();
    var buttons = [
      {
        label: SBString("transcode.error.notification.detail.label"),
        accessKey: SBString("transcode.error.notification.detail.accesskey"),
        callback: function(notification, button) {
          let deviceErrorMonitor =
            Cc["@songbirdnest.com/device/error-monitor-service;1"]
              .getService(Ci.sbIDeviceErrorMonitor);
          let errorItems = deviceErrorMonitor.getDeviceErrors(device);
          WindowUtils.openDialog
            (window,
             "chrome://songbird/content/xul/device/deviceErrorDialog.xul",
             "device_error_dialog",
             "chrome,centerscreen",
             false,
             [ "", device, errorItems, "syncing" ],
             null);
          deviceErrorMonitor.clearErrorsForDevice(device);
        },
        popup: null
      }
    ];
    var notification = notificationBox.appendNotification(
                         msg,
                         event,
                         "chrome://songbird/skin/device/error.png",
                         notificationBox.PRIORITY_CRITICAL_MEDIUM,
                         buttons);
    var onNotificationCommand = function(event) {
      let classes = event.originalTarget.className.split(/\s+/);
      if (classes.indexOf("messageCloseButton") > -1) {
        // the user clicked on the dismiss button; clear the device errors
        let deviceErrorMonitor =
          Cc["@songbirdnest.com/device/error-monitor-service;1"]
            .getService(Ci.sbIDeviceErrorMonitor);
        deviceErrorMonitor.clearErrorsForDevice(device);
      }
    };
    notification.addEventListener("command", onNotificationCommand, false);
  }

  // attach our device event listener
  var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                        .getService(Ci.sbIDeviceEventTarget);
  deviceManager.addEventListener(deviceManagerListener);

  // attach our cleanup routine
  addEventListener("unload", function(event) {
    removeEventListener(event.type, arguments.callee, false);
    deviceManager.removeEventListener(deviceManagerListener);
    delete deviceManager;
  }, false);
})();

