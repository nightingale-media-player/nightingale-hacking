/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \file sbDeviceErrorMonitor.js
 * \brief This service monitors devices for errors and stores them for easy
 *        access later.
 *
 */
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");

var deviceErrorMonitorConfig = {
  className:      "Nightingale Device Error Monitor Service",
  cid:            Components.ID("{7a2a55d1-0270-4789-bc7c-12ffaa19b4cd}"),
  contractID:     "@getnightingale.com/device/error-monitor-service;1",

  ifList: [ Ci.sbIDeviceEventListener,
            Ci.sbIDeviceErrorMonitor,
            Ci.nsIObserver ],

  categoryList:
  [
    {
      category: 'app-startup',
      entry: 'service-device-error-monitor',
      value: 'service,@getnightingale.com/device/error-monitor-service;1'
    }
  ]
};

function deviceErrorMonitor () {
  this._initialized = false;
  var obsSvc = Cc['@mozilla.org/observer-service;1']
                 .getService(Ci.nsIObserverService);

  // We want to wait until profile-after-change to initialize
  obsSvc.addObserver(this, 'profile-after-change', false);
  obsSvc.addObserver(this, 'quit-application', false);
}

deviceErrorMonitor.prototype = {
  // XPCOM stuff
  classDescription: deviceErrorMonitorConfig.className,
  classID: deviceErrorMonitorConfig.cid,
  contractID: deviceErrorMonitorConfig.contractID,
  _xpcom_categories: deviceErrorMonitorConfig.categoryList,

  // Internal properties
  _deviceList: [],
  _listenerList: [],
  _sbStrings: null,
  NONE : "none",
  // Internal functions

  /**
   * \brief Initialize the deviceErrorMonitor service.
   */
  _init: function deviceErrorMonitor__init() {
    var deviceManagerSvc = Cc["@getnightingale.com/Nightingale/DeviceManager;2"]
                             .getService(Ci.sbIDeviceManager2);
    deviceManagerSvc.addEventListener(this);
    var sbs = Cc["@mozilla.org/intl/stringbundle;1"]
                .getService(Ci.nsIStringBundleService);
    this._sbStrings = sbs.createBundle("chrome://nightingale/locale/nightingale.properties");
  },

  /**
   * \brief Shutdown (cleanup) the deviceErrorMonitorService.
   */
  _shutdown: function deviceErrorMonitor__shutdown() {
    var deviceManagerSvc = Cc["@getnightingale.com/Nightingale/DeviceManager;2"]
                             .getService(Ci.sbIDeviceManager2);
    deviceManagerSvc.removeEventListener(this);
    while(this._deviceList.length > 0) {
      if (this._deviceList[0].device) {
        this._removeDevice(this._deviceList[0].device);
      }
    }
  },

  /**
   * \brief Adds a device that has just connected to the list of available
   *        devices to monitor for errors.
   *
   * \param aDevice Device to add to the list.
   */
  _addDevice: function deviceErrorMonitor__addDevice(aDevice) {
    var devIndex = this._findDeviceIndex(aDevice);
    if (devIndex == -1) {

      var newDeviceObj = {};
      newDeviceObj.device = aDevice;
      newDeviceObj.errorLists = { };
      this._deviceList.push(newDeviceObj);
    }
  },

  /**
   * \brief Remove a device from the list of available devices to monitor for
   *        errors.
   *
   * \params aDevice Device to remove from the list.
   */
  _removeDevice: function deviceErrorMonitor__removeDevice(aDevice) {
    var devIndex = this._findDeviceIndex(aDevice);
    if (devIndex > -1) {
      this._deviceList.splice(devIndex, 1);
    }
  },

  /**
   * \brief Finds a device index in the device list.
   *
   * \param aDevice Device to locate in the list.
   * \returns index in _deviceList aDevice is located
   */
  _findDeviceIndex: function deviceErrorMonitor__findDeviceIndex(aDevice) {
    for (var i = 0; i < this._deviceList.length; ++i) {
      if (this._deviceList[i].device.id.equals(aDevice.id)) {
        return i;
      }
    }
    return -1;
  },

  /**
   * \brief Returns the media type given an event.
   *
   * \param aDeviceEvent the device event to inspect
   * \return The content type of the item in the device event or "none" if
   *         the event isn't device related
   */
  _getContentType : function deviceErrorMonitor__getContentType(aDeviceEvent) {
    if (aDeviceEvent.data instanceof Ci.sbIMediaItem) {
      return aDeviceEvent.data.contentType;
    }
    return this.NONE;
  },

  /**
   * \brief Returns a list of all the errors on a device.
   *
   * \param aDevIndex the device index in our array to get errors for.
   * \return array of errors for a device.
   */
  _getAllErrors : function deviceErrorMonitor__getAllErrors(aDevIndex) {
    var device = this._deviceList[aDevIndex];
    var errorList = [];
    if (!device)
      return errorList;

    // Concat all the error contentType arrays together.
    for (var contentType in device.errorLists) {
      errorList = errorList.concat(device.errorLists[contentType]);
    }
    return errorList;
  },

  /**
   * \brief Returns a list of errors for a content type on a device.
   *
   * \param aDevIndex the device index in our array to get errors for.
   * \param aContentType  the type of content we want errors for.
   * \return array of errors for a device of a particular media type.
   */
  _getErrorList : function deviceErrorMonitor__getErrorList(aDevIndex,
                                                            aContentType) {
    var device = this._deviceList[aDevIndex];
    var errorList = device.errorLists[aContentType];
    if (!errorList) {
      errorList = [];
      device.errorLists[aContentType] = errorList;
    }
    return errorList;
   },

  /**
   * \brief Save an error in the list of errors for a device.
   *
   * \param aDeviceEvent Event from the device.
   * \param aErrorMsg Error Message to display with the item.
   */
  _logError: function deviceErrorMonitor__logError(aDeviceEvent, aErrorMsg) {
    var device = aDeviceEvent.origin;
    var mediaItem;
    if (device instanceof Ci.sbIDevice) {
      var devIndex = this._findDeviceIndex(device);
      if (devIndex > -1) {
        var mediaURL = "";
        if (aDeviceEvent.data) {
          if (aDeviceEvent.data instanceof Ci.sbIMediaList) {
            mediaURL = aDeviceEvent.data.name;
            mediaItem = aDeviceEvent.data;
          } else if (aDeviceEvent.data instanceof Ci.sbIMediaItem) {
            mediaURL = aDeviceEvent.data.contentSrc.spec;
            mediaURL = decodeURIComponent(mediaURL);
            mediaItem = aDeviceEvent.data;
          } else if (aDeviceEvent.data instanceof Ci.nsIPropertyBag2) {
            var bag = aDeviceEvent.data;
            if (bag.hasKey("item")) {
              mediaItem = bag.getPropertyAsInterface("item", Ci.sbIMediaItem);
              mediaURL = mediaItem.contentSrc.spec;
              mediaURL = decodeURIComponent(mediaURL);
            }
          } else if (aDeviceEvent.data instanceof Ci.nsIFile) {
            var ios = Components.classes["@mozilla.org/network/io-service;1"]
                                .getService(Components.interfaces.nsIIOService);
            var fileHandler = ios.getProtocolHandler("file")
                                 .QueryInterface(Components.interfaces
                                    .nsIFileProtocolHandler);
            mediaURL = fileHandler.getURLSpecFromFile(aDeviceEvent.data);
          }
        } else {
          mediaURL = SBString("device.info.unknown");
        }

        // Format the error message
        var errorString = aErrorMsg;
        if (mediaURL) {
          errorString = this._sbStrings.formatStringFromName(
                                                        "device.error.format",
                                                        [aErrorMsg, mediaURL],
                                                        2);
        }
        // Store the error information
        var errorInfo = {
          msg : errorString,
          item : mediaItem,
          state : aDeviceEvent.deviceState
        };
        // Get the contentType and add the error information to the list.
        var contentType = this._getContentType(aDeviceEvent);
        // Get the error list, if one doesn't exists create it
        var errorList = this._getErrorList(devIndex, contentType);

        // Ignore error if an error has already been logged for item
        if (errorInfo.item) {
          for (var i = 0; i < errorList.length; i++) {
            var item = errorList[i].item;
            if (item && item.equals(errorInfo.item))
              return;
          }
        }

        errorList.push(errorInfo);

        this._notifyListeners(device);
      }
    }
  },

  /**
   * \brief Notify listeners that an error has been logged for the device
   *        specified by aDevice.
   *
   * \param aDevice device for which error was logged.
   */
  _notifyListeners: function deviceErrorMonitor__notifyListeners(aDevice) {
    // Call listeners in reverse order so they may remove themselves.
    for (var i = this._listenerList.length - 1; i >= 0; i--) {
      try {
        this._listenerList[i].onDeviceError(aDevice);
      } catch (ex) {
        Cu.reportError(ex);
        // Remove the listener since it probably does not exist anymore
        this._listenerList.splice(i, 1);
      }
    }
  },

  // sbIDeviceErrorMonitor

  /**
   * \brief Checks to see if a device has had any recent errors.
   *
   * \param aDevice device to check for errors on.
   * \param aContentType type of content we want to check for errors on.
   * \returns true if any errors are currently registered for this device.
   */
  deviceHasErrors: function deviceErrorMonitor_deviceHasErrors(aDevice,
                                                               aContentType) {
    var devIndex = this._findDeviceIndex(aDevice);
    if (devIndex > -1) {
      var device = this._deviceList[devIndex];
      var errorList;
      if (!aContentType)
        errorList = this._getAllErrors(devIndex);
      else
        errorList = this._getErrorList(devIndex, aContentType);

      return (errorList ? (errorList.length > 0) : false);
    }
    return false;
  },

  /**
   * \brief Gets an array of property bags containing information about the
   *        errors.
   *
   * \param aDevice The device to get the list of errors for.
   * \param aContentType type of content we want errosr for.
   * \returns array of property bags that contain error information
   *
   * The property bag currently contains
   *  msg   - nsISupportsString of the message that occured.
   *  item  - sbIMediaItem that the error occured with.
   *  state - State of the device at the point of the error.
   */
  getDeviceErrors: function deviceErrorMonitor_getDeviceErrors(aDevice,
                                                               aContentType) {
    var devIndex = this._findDeviceIndex(aDevice);
    var errorList = Cc["@getnightingale.com/moz/xpcom/threadsafe-array;1"]
                      .createInstance(Ci.nsIMutableArray);

    if (devIndex == -1)
      return errorList;

    var jsErrorList;
    if (!aContentType)
      jsErrorList = this._getAllErrors(devIndex);
    else
      jsErrorList = this._getErrorList(devIndex, aContentType);

    for (var index = 0; index < jsErrorList.length; index++) {
      // Add the information to a property bag
      var errorBag = Cc["@mozilla.org/hash-property-bag;1"]
                       .createInstance(Ci.nsIWritablePropertyBag2);
      errorBag.setPropertyAsAString("msg", jsErrorList[index].msg);
      errorBag.setPropertyAsInterface("item",
                                      jsErrorList[index].item,
                                      Ci.sbIMediaItem);
      errorBag.setPropertyAsUint32("state", jsErrorList[index].state);

      errorList.appendElement(errorBag, false);
    }

    return errorList;
  },

  /**
   * \brief Clears the array of error strings for a device.
   *
   * \param aDevice device to clear error list from.
   */
  clearErrorsForDevice: function deviceErrorMonitor_clearErrorsForDevice(aDevice) {
    var devIndex = this._findDeviceIndex(aDevice);

    if (devIndex > -1) {
      this._deviceList[devIndex].errorLists = {};
    }
  },

  /**
   * \brief Adds a listener for new device errors.
   *
   * \param aListener listener to call when a device error is logged.
   */
  addListener: function deviceErrorMonitor_addListener(aListener) {
    if (this._listenerList.indexOf(aListener) < 0)
      this._listenerList.push(aListener);
  },

  /**
   * \brief Removes a listener for new device errors.
   *
   * \param aListener listener to remove.
   */
  removeListener: function deviceErrorMonitor_addListener(aListener) {
    var listenerIndex = this._listenerList.indexOf(aListener);
    if (listenerIndex >= 0)
      this._listenerList.splice(listenerIndex, 1);
  },

  // sbIDeviceEventListener
  onDeviceEvent: function deviceErrorMonitor_onDeviceEvent(aDeviceEvent) {
    switch(aDeviceEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED:
        var device = aDeviceEvent.data.QueryInterface(Ci.sbIDevice);
        this._addDevice(device);
      break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED:
        var device = aDeviceEvent.data.QueryInterface(Ci.sbIDevice);
        this._removeDevice(device);
      break;

      // An error has occurred, we need to store it for later
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ACCESS_DENIED:
        this._logError(aDeviceEvent,
                       SBString("device.error.access_denied"));
      break;
      case Ci.sbIDeviceEvent.EVENT_DEVICE_NOT_ENOUGH_FREESPACE:
        this._logError(aDeviceEvent,
                       SBString("device.error.not_enough_free_space"));
      break;
      case Ci.sbIDeviceEvent.EVENT_DEVICE_NOT_AVAILABLE:
        this._logError(aDeviceEvent,
                       SBString("device.error.not_available"));
      break;
      case Ci.sbIDeviceEvent.EVENT_DEVICE_ERROR_UNEXPECTED:
        this._logError(aDeviceEvent,
                       SBString("device.error.unexpected"));
      break;
      case Ci.sbIDeviceEvent.EVENT_DEVICE_FILE_MISSING:
        this._logError(aDeviceEvent,
                       SBString("device.error.file_missing"));
      break;
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MEDIA_WRITE_UNSUPPORTED_TYPE:
        this._logError(aDeviceEvent,
                       SBString("device.error.unsupported_type"));
      break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSCODE_ERROR:
        // Grab the extended error info from the property bag
        var message = "";
        if (aDeviceEvent.data instanceof Ci.nsIPropertyBag2) {
          message = aDeviceEvent.data.get("message");
          if (!message && aDeviceEvent.data.hasKey("mediacore-error")) {
            message = aDeviceEvent.data
                                  .getPropertyAsInterface("mediacore-error",
                                                          Ci.sbIMediacoreError)
                                  .message;
          }
        }
        this._logError(aDeviceEvent, message);
      break;

    }
  },

  // nsIObserver
  observe: function deviceErrorMonitor_observer(subject, topic, data) {
    var obsSvc = Cc['@mozilla.org/observer-service;1']
                   .getService(Ci.nsIObserverService);

    switch (topic) {
      case 'quit-application':
        obsSvc.removeObserver(this, 'quit-application');
        this._shutdown();
      break;
      case 'profile-after-change':
        obsSvc.removeObserver(this, 'profile-after-change');
        this._init();
      break;
    }
  },

  // nsISupports
  QueryInterface: XPCOMUtils.generateQI(deviceErrorMonitorConfig.ifList)
};

/**
 * /brief XPCOM initialization code
 */
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([deviceErrorMonitor]);
}
