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

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
  
var mtpCore = {
  _deviceEventListener: null,
  _deviceManager:       null,
  _promptSvc:           null,
  _stringBundle:        null,
  _stringBundleSvc:     null,
  _deviceInfoList:      null,
  
  // ************************************
  // Internal methods
  // ************************************
  _initialize: function mtpCore_initialize() {
    var self = this;
    window.addEventListener("unload", function() {self._shutdown()}, false);

    this._deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                            .getService(Ci.sbIDeviceManager2);
    
    var deviceEventListener = {
      mtpCore: this,
      onDeviceEvent: function deviceEventListener_onDeviceEvent(aDeviceEvent) {
        this.mtpCore._processDeviceManagerEvent(aDeviceEvent);
      }
    };

    this._deviceEventListener = deviceEventListener;
    this._deviceManager.addEventListener(deviceEventListener);
    
    this._promptSvc = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                        .getService(Ci.nsIPromptService);
                        
    this._stringBundleSvc = Cc["@mozilla.org/intl/stringbundle;1"]
                              .getService(Ci.nsIStringBundleService);
    this._stringBundle = 
      this._stringBundleSvc
          .createBundle( "chrome://songbird/locale/songbird.properties" );
    
    // Initialize the device info list.
    this._deviceInfoList = {};
    var deviceEnum = this._deviceManager.devices.enumerate();
    while (deviceEnum.hasMoreElements()) {
      this._addDevice(deviceEnum.getNext().QueryInterface(Ci.sbIDevice));
    }
  },
  
  _shutdown: function mtpCore_shutdown() {
    window.removeEventListener("unload", arguments.callee, false);
    
    this._deviceManager.removeEventListener(this._deviceEventListener);
    this._deviceEventListener = null;

    this._removeAllDevices();
    this._deviceInfoList = null;

    this._deviceManager = null;
    this._promptSvc = null;
    
    this._stringBundle = null;
    this._stringBundleSvc = null;
  },
  
  _processDeviceManagerEvent:
    function mtpCore_processDeviceManagerEvent(aDeviceEvent) {
    switch(aDeviceEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED: {
        this._addDevice(aDeviceEvent.data.QueryInterface(Ci.sbIDevice));
      }
      break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED: {
        this._removeDevice(aDeviceEvent.data.QueryInterface(Ci.sbIDevice));
      }
      break;
    }
  },

  _processDeviceEvent: function mtpCore_processDeviceEvent(aDeviceEvent) {
    
    // For now, we only process events coming from sbIDevice.
    if(!(aDeviceEvent.origin instanceof Ci.sbIDevice))
      return;
    
    var applicationName = SBString("brandShortName");

    var errorTitle = "";
    var errorMsg = "";
    
    var device = aDeviceEvent.origin.QueryInterface(Ci.sbIDevice);
    
    var windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"]
                           .getService(Ci.nsIWindowMediator);
    var mainWindow = windowMediator.getMostRecentWindow("Songbird:Main");
    
    switch(aDeviceEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MEDIA_WRITE_UNSUPPORTED_TYPE: {
        this._addUnsupportedMediaItem(device, aDeviceEvent.data);
      }
      break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_END: {
        this._processUnsupportedMediaItems(device, mainWindow);
      }
      break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_ACCESS_DENIED: {
        errorTitle = 
          this._stringBundle
              .getStringFromName("device.error.access_denied.title");
        errorMsg = 
          this._stringBundle
              .formatStringFromName("device.error.access_denied.message",
                                    [applicationName, device.name],
                                    2);
        this._promptSvc.alert(mainWindow, errorTitle, errorMsg);
      }
      break;
      
      case Ci.sbIDeviceEvent.EVENT_DEVICE_NOT_ENOUGH_FREESPACE: {
        errorTitle = 
          this._stringBundle
              .getStringFromName("device.error.not_enough_freespace.title");
        errorMsg = 
          this._stringBundle
              .formatStringFromName("device.error.not_enough_freespace.message",
                                    [applicationName, device.name],
                                    2);
        this._promptSvc.alert(mainWindow, errorTitle, errorMsg);
      }
      break;
      
      case Ci.sbIDeviceEvent.EVENT_DEVICE_NOT_AVAILABLE: {
        errorTitle = 
          this._stringBundle
              .getStringFromName("device.error.not_available.title");
        errorMsg = 
          this._stringBundle
              .formatStringFromName("device.error.not_avaialble.message",
                                    [applicationName, device.name],
                                    2);
        this._promptSvc.alert(mainWindow, errorTitle, errorMsg);
      }
      break;
      
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ERROR_UNEXPECTED: {
        errorTitle = 
          this._stringBundle
              .getStringFromName("device.error.generic.title");
        errorMsg = 
          this._stringBundle
              .formatStringFromName("device.error.generic.message",
                                    [applicationName, device.name],
                                    2);
        this._promptSvc.alert(mainWindow, errorTitle, errorMsg);
      }
      break;
    }
  },

  _processUnsupportedMediaItems:
    function mtpCore_processUnsupportedMediaItems(aDevice, aMainWindow) {
    // Get the unsupported media items.  Do nothing if there are no unsupported
    // media items.
    var deviceInfo = this._deviceInfoList[aDevice.id];
    if (!deviceInfo)
      return;
    var unsupportedMediaItems = deviceInfo.unsupportedMediaItems;
    // Get the unsupported media items.  Do nothing if there are no unsupported
    if (!unsupportedMediaItems)
      return;

    // Clear the unsupported media items before presenting the dialog as this
    // function can be reentered while the dialog is open.
    deviceInfo.unsupportedMediaItems = null;

    // Present unsupported media dialog.
    SBWindow.openModalDialog
               (aMainWindow,
                "chrome://songbird/content/xul/device/unsupportedMedia.xul",
                "unsupported_media_dialog",
                "chrome,centerscreen",
                [ aDevice, unsupportedMediaItems ],
                null);
  },

  //XXXErikS This won't work properly if user switches feathers during a
  // transfer.  This Javascript will get unloaded and reloaded, losing this
  // context.
  _addUnsupportedMediaItem: function mtpCore_addUnsupportedMediaItem(aDevice,
                                                                     aGuid) {
    // Get the device media item.
    var mediaItem = this._getDeviceItemByGuid(aDevice, aGuid);
    if (!mediaItem) {
      dump("addUnsupportedMediaItem: " +
           "Could not get media item from GUID.\n");
      return;
    }

    // Get the unsupported media items list.
    var deviceInfo = this._deviceInfoList[aDevice.id];
    if (!deviceInfo.unsupportedMediaItems) {
      deviceInfo.unsupportedMediaItems =
        Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    }
    var unsupportedMediaItems = deviceInfo.unsupportedMediaItems;

    // Add the item to the unsupported media items list if it's not already
    // there.
    try {
      unsupportedMediaItems.indexOf(0, mediaItem);
    } catch (ex) {
      unsupportedMediaItems.appendElement(mediaItem, false);
    }
  },

  _addDevice: function mtpCore_addDevice(aDevice) {
    // Do nothing if device is already in list.
    if (this._deviceInfoList[aDevice.id])
      return;

    // Add device info to list.
    var deviceInfo = { device: aDevice };
    this._deviceInfoList[aDevice.id] = deviceInfo;

    // Listen for device events.
    deviceInfo.deviceEventListener = {
      mtpCore: this,
      onDeviceEvent: function deviceEventListener_onDeviceEvent(aDeviceEvent) {
        this.mtpCore._processDeviceEvent(aDeviceEvent);
      }
    };
    var deviceEventTarget = aDevice.QueryInterface(Ci.sbIDeviceEventTarget);
    deviceEventTarget.addEventListener(deviceInfo.deviceEventListener);
  },

  _removeDevice: function mtpCore_removeDevice(aDevice) {
    // Get the device info.
    var deviceInfo = this._deviceInfoList[aDevice.id];

    // Remove device event listener.
    var deviceEventTarget = aDevice.QueryInterface(Ci.sbIDeviceEventTarget);
    deviceEventTarget.removeEventListener(deviceInfo.deviceEventListener);

    // Delete the device info.
    delete this._deviceInfoList[aDevice.id];
  },

  _removeAllDevices: function mtpCore_removeAllDevices() {
    for (var id in this._deviceInfoList) {
      this._removeDevice(this._deviceInfoList[id].device);
    }
  },

  _getDeviceItemByGuid: function mtpCore_getDeviceItemByGuid(aDevice, aGuid) {
    // Search all device libraries for item.
    var libraries = aDevice.content.libraries;
    var libEnum = libraries.enumerate();
    while (libEnum.hasMoreElements()) {
      // Get the next device library.
      var library = libEnum.getNext().QueryInterface(Ci.sbILibrary);

      // Look up item in device library.  Continue searching on exception as
      // this indicates that the item is not present in the library.
      try {
        return library.getItemByGuid(aGuid);
      } catch (ex) {}
    }

    return null;
  },
};

mtpCore._initialize();
