/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
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

/** 
* \file  deviceInfo.js
* \brief Javascript source for the device info widget.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device info widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device capacity imported services.
//
//------------------------------------------------------------------------------

// Component manager defs.
if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

// Songbird imports.
Cu.import("resource://app/jsmodules/SBTimer.jsm");
Cu.import("resource://app/jsmodules/sbStorageFormatter.jsm");


//------------------------------------------------------------------------------
//
// Device info services.
//
//------------------------------------------------------------------------------

var DIW = {
  //
  // Default device image url
  //
  
  _devImgURL:      "chrome://songbird/skin/icons/icon-device-64x64.png",
  
  //
  // Device info configuration.
  //
  //   _pollPeriodTable         Table of polling periods for device information
  //                            that needs to be polled.
  //

  _pollPeriodTable: { "battery": 60000 },


  //
  // Device info object fields.
  //
  //   _widget                  Device info widget.
  //

  _widget: null,


  /**
   * \brief Initialize the device info services for the device info widget
   *        specified by aWidget.
   *
   * \param aWidget             Device info widget.
   */

  initialize: function DIW__initialize(aWidget) {
    // Get the device info widget.
    this._widget = aWidget;

    // Initialize object fields.
    this._device = this._widget.device;

    // Show specified elements.
    this._showElements();

    // Initialize the device spec rows.
    this._deviceSpecInitialize();

    // Update the UI.
    this._update();
    
    // Watch for device changes
    this._device.addEventListener(this);
  },

  onDeviceEvent : function DIW_onDeviceEvent(anEvent) {
    if (anEvent.type == Ci.sbIDeviceEvent.EVENT_DEVICE_INFO_CHANGED) {
      this._update();
    }
  },
  /**
   * \brief Finalize the device info services.
   */

  finalize: function DIW_finalize() {
    // Finalize the pollsing services.
    this._pollingFinalize();
    
    // Stop listening
    if (this._device != null) {
      this._device.removeEventListener(this);
    }
    
    // Finalize the device services.
    this._deviceFinalize();

    // Clear object fields.
    this._widget = null;
  },


  //----------------------------------------------------------------------------
  //
  // Device info device spec row services.
  //
  // NOTE: These services may move the device spec rows within the document.
  //       Doing so disables the use of certain XUL element properties such as
  //       value.  Thus, attributes must be used instead of properties.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Initialize all device spec rows according to the "devicespecs"
   *        attribute.
   */

  _deviceSpecInitialize: function DIW__deviceSpecInitialize() {
    // Get the list of device specs to present.
    var deviceSpecs = this._widget.getAttribute("devicespecs");
    var deviceSpecList = deviceSpecs.split(",");

    // Get the list of device spec row elements.
    var deviceSpecRows = this._getElement("device_spec_rows");
    var deviceSpecRowList = deviceSpecRows.getElementsByTagNameNS
              ("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
               "row");

    // Hide all device spec rows.
    for (var i = 0; i < deviceSpecRowList.length; i++) {
      deviceSpecRowList[i].hidden = true;
    }

    // Move and update specified device spec rows into position.  Also determine
    // the device info polling period.
    var pollPeriod = Number.POSITIVE_INFINITY;
    var refDeviceSpecRow = deviceSpecRows.firstChild;
    for (var i = 0; i < deviceSpecList.length; i++) {
      // Get the device spec and row.
      var deviceSpec = deviceSpecList[i];
      var deviceSpecRow = this._getDeviceSpecRow(deviceSpec);

      // Skip device spec if it's not supported.
      if (!this._supportsDeviceSpec(deviceSpec))
        continue;

      // Move the device spec row into position.
      if (deviceSpecRow != refDeviceSpecRow) {
        deviceSpecRows.insertBefore(deviceSpecRow, refDeviceSpecRow);
      }
      refDeviceSpecRow = deviceSpecRow.nextSibling;

      // Show the device spec row.
      deviceSpecRow.hidden = false;

      // Check the device spec polling period.
      var specPollPeriod = this._pollPeriodTable[deviceSpec];
      if (specPollPeriod && (specPollPeriod < pollPeriod))
        pollPeriod = specPollPeriod;
    }

    // Start polling if needed.
    if (pollPeriod != Number.POSITIVE_INFINITY)
      this._pollingInitialize(pollPeriod);
  },


  /**
   * \brief Update all device spec rows.
   */

  _deviceSpecUpdateAll: function DIW__deviceSpecUpdateAll() {
    // Update all of the shown device spec rows.
    var deviceSpecRows = this._getElement("device_spec_rows");
    var deviceSpecRowList = deviceSpecRows.getElementsByTagNameNS
              ("http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul",
               "row");
    for (var i = 0; i < deviceSpecRowList.length; i++) {
      var deviceSpecRow = deviceSpecRowList[i];
      if (!deviceSpecRow.hidden)
        this._deviceSpecUpdate(deviceSpecRow);
    }
  },


  /**
   * \brief Update the device spec row specified by aRow.
   *
   * \param aRow                Device spec row to update.
   */

  _deviceSpecUpdate: function DIW__deviceSpecUpdate(aRow) {
    // Dispatch updating.
    var deviceSpec = aRow.getAttribute("devicespec");
    switch (deviceSpec) {
      case "model" :
        this._deviceSpecUpdateValue("model_value_label",
                                    this._getDeviceModel());
        break;

      case "capacity" :
        this._deviceSpecUpdateValue("capacity_value_label",
                                    this._getDeviceModelSize());
        break;

      case "model_capacity" :
        this._deviceSpecUpdateModelCapacity();
        break;

      case "friendly_name" :
        this._deviceSpecUpdateValue("friendly_name_value_label",
                                    this._getDeviceFriendlyName());
        break;

      case "serial_number" :
        this._deviceSpecUpdateValue("serial_number_value_label",
                                    this._getDeviceSerialNumber());
        break;

      case "vendor" :
        this._deviceSpecUpdateValue("vendor_value_label",
                                    this._getDeviceVendor());
        break;

      case "access" :
        this._deviceSpecUpdateValue("access_value_label",
                                    this._getDeviceAccessCompatibility());
        break;

      case "playback_formats" :
        this._deviceSpecUpdateDesc("playback_formats_value_label",
                                   this._getDevicePlaybackFormats());
        break;

      case "battery" :
        this._deviceSpecUpdateBattery();
        break;

      default :
        break;
    }
  },


  /**
   * \brief Update the device spec value of the label specified by aLabelID with
   *        the value specified by aValue.
   *
   * \param aLabelID            ID of label to update.
   * \param aValue              Value with which to update label.
   */

  _deviceSpecUpdateValue: function DIW__deviceSpecUpdateValue(aLabelID,
                                                              aValue) {
    // Set the value label.
    var labelElem = this._getElement(aLabelID);
    if (labelElem.getAttribute("value") != aValue)
      labelElem.setAttribute("value", aValue);
  },


  /**
   * \brief Update the device spec text of the description specified by aDescID
   *        with the text specified by aText.  Assume that the first child node
   *        of the description exists and is a text node.
   *
   * \param aDescID             ID of description to update.
   * \param aText               Text with which to update description.
   */

  _deviceSpecUpdateDesc: function DIW__deviceSpecUpdateDesc(aDescID, aText) {
    // Set the description text.
    var descElem = this._getElement(aDescID);
    if (descElem.firstChild.data != aText)
      descElem.firstChild.data = aText;
  },


  /**
   * \brief Update the device spec model/capacity row.
   */

  _deviceSpecUpdateModelCapacity: function
                                    DIW__deviceSpecUpdateModelCapacity() {
    // Get the device model and model capacity.
    var devModelValue = this._getDeviceModel();
    var devCapValue = this._getDeviceModelSize();
    var devModelCapValue = devModelValue + " (" + devCapValue + ")";

    // Upate the device model/capacity.
    this._deviceSpecUpdateValue("model_capacity_value_label", devModelCapValue);
  },


  /**
   * \brief Update the device spec battery row.
   */

  _deviceSpecUpdateBattery: function DIW__deviceSpecUpdateBattery() {
    // Get the battery status.
    var batteryLevel = {};
    var onBatteryPower = {};
    this._getDeviceBatteryStatus(batteryLevel, onBatteryPower);
    batteryLevel = batteryLevel.value;
    onBatteryPower = onBatteryPower.value;
    var batteryStatus;
    if (onBatteryPower)
      batteryStatus = "battery-powered";
    else if (batteryLevel == 100)
      batteryStatus = "charged";
    else
      batteryStatus = "charging";

    // Update the battery status and level.
    var batteryElem = this._getElement("battery_status");
    if ((batteryElem.getAttribute("status") != batteryStatus) ||
        (batteryElem.getAttribute("level") != batteryLevel)) {
      batteryElem.setAttribute("status", batteryStatus);
      batteryElem.setAttribute("level", batteryLevel);
    }
  },

  //----------------------------------------------------------------------------
  //
  // Device info event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle the event specified by aEvent for elements with defined
   *        actions.
   *
   * \param aEvent              Event to handle.
   */

  onAction: function DIW_onAction(aEvent) {
    // Dispatch processing of action.
    switch (aEvent.target.getAttribute("action")) {
      case "eject" :
        this._eject();
        break;

      case "more_info" :
        this._presentMoreInfo();
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Device info XUL services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "sbid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function DIW__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  },


  /**
   * \brief Return the device spec row element with the device spec specified by
   *        aDeviceSpec.
   *
   * \param aDeviceSpec         Device spec of row to get.
   *
   * \return Device spec row.
   */

  _getDeviceSpecRow: function DIW__getDeviceSpecRow(aDeviceSpec) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "devicespec",
                                                   aDeviceSpec);
  },


  /**
   * \brief Return the show element with the show ID specified by aShowID.
   *
   * \param aShowID             Element show ID.
   *
   * \return Show element.
   */

  _getShowElement: function DIW__getShowElement(aShowID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "showid",
                                                   aShowID);
  },


  //----------------------------------------------------------------------------
  //
  // Device info polling services.
  //
  //----------------------------------------------------------------------------

  //
  // Device info polling services fields.
  //
  //   _pollingTimer            Timer used for polling device info.
  //

  _pollingTimer: null,


  /**
   * \brief Initialize the polling services to poll with the period specified by
   *        aPollPeriod.
   *
   * \param aPollPeriod         Polling time period in milliseconds.
   */

  _pollingInitialize: function DIW__pollingInitialize(aPollPeriod) {
    // Start a timing interval to poll for device info UI updates.
    var _this = this;
    var func = function() { _this._update(); }
    this._pollingTimer = new SBTimer(func,
                                     aPollPeriod,
                                     Ci.nsITimer.TYPE_REPEATING_PRECISE);
  },


  /**
   * \brief Finalize the polling services.
   */

  _pollingFinalize: function DIW__pollingFinalize() {
    if (this._pollingTimer) {
      this._pollingTimer.cancel();
      this._pollingTimer = null;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal device info services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Show the elements specified by the "showlist" attribute".
   */

  _showElements: function DIW__showElements() {
    // Get the list of elements to show.
    var showListAttr = this._widget.getAttribute("showlist");
    if (!showListAttr)
      return;
    var showList = this._widget.getAttribute("showlist").split(",");

    // Show the specified elements.
    for (var i = 0; i < showList.length; i++) {
      var element = this._getShowElement(showList[i]);
      element.hidden = false;
    }
  },


  /**
   * \brief Update the device info UI.  Read the device info, and if anything
   *        has changed, update the UI to reflect the new info.
   */

  _update: function DIW__update() {
    // Update the device name label.
    var devNameLabel = this._getElement("device_name_label");
    var devName = this._getDeviceFriendlyName();
    if (devNameLabel.value != devName)
        devNameLabel.value = devName;

    // Update the device image.
    var devImage = this._getElement("device_image");
    var devIconURL = this._getDeviceIcon();
    if (devIconURL && (devImage.src != devIconURL))
      devImage.src = devIconURL;

    // Update the device specs.
    this._deviceSpecUpdateAll();
  },


  /**
   * \brief Present the more info dialog.
   */

  _presentMoreInfo: function DIW__presentMoreInfo() {
    SBWindow.openModalDialog
               (window,
                "chrome://songbird/content/xul/device/deviceInfoDialog.xul",
                "",
                "chrome,centerscreen",
                [ this._device ],
                null);
  },


  //----------------------------------------------------------------------------
  //
  // Device info device services.
  //
  //   These services provide an interface to the device object.
  //   These services register for any pertinent device notifications and call
  // _update to update the UI accordingly.  These include notifications for
  // changes to the device friendly name.
  //
  //----------------------------------------------------------------------------

  //
  // Device info services fields.
  //
  //   _device                  sbIDevice object.
  //

  _device: null,


  /**
   * \brief Finalize the device services.
   */

  _deviceFinalize: function DIW__deviceFinalize() {
    // Clear object fields.
    this._device = null;
  },

  /**
   * \brief Get a device property if available
   *
   * \param aPropertyName name of the property to get
   * \param aDefault default to return if property not found
   * 
   * \return string value of the property or the default if not found.
   * 
   * \sa sbStandardDeviceProperties.h
   */
  
  _getDeviceProperty: function DIW__getDeviceProperty(aPropertyName, aDefault) {
    try {
      return this._device.properties.properties.getPropertyAsAString(aPropertyName);
    } catch (err) {
      return aDefault;
    }
  },

  /**
   * \brief Return the device model name.
   *
   * \return Device model name.
   */

  _getDeviceModel: function DIW__getDeviceModel() {
    try {
      return this._device.properties.modelNumber;
    } catch (err) {
      return SBString("device.info.unknown");
    }
  },


  /**
   * \brief Return the device model size.
   *
   * \return Device model size.
   */

  _getDeviceModelSize: function DIW__getDeviceModelSize() {
    try {
      var modelSize = this._getDeviceProperty("http://songbirdnest.com/device/1.0#capacity");
      return StorageFormatter.format(modelSize);
    } catch (err) {
      return SBString("device.info.unknown");
    }
  },


  /**
   * \brief Return the device friendly name.
   *
   * \return Device friendly name.
   */

  _getDeviceFriendlyName: function DIW__getDeviceFriendlyName() {
    try {
      return this._device.properties.friendlyName;
    } catch (err) {
      return SBString("device.info.unknown");
    }
  },


  /**
   * \brief Return the device serial number.
   *
   * \return Device serial number.
   */

  _getDeviceSerialNumber: function DIW__getDeviceSerialNumber() {
    try {
      return this._device.properties.serialNumber;
    } catch (err) {
      return SBString("device.info.unknown");
    }
  },


  /**
   * \brief Return the device vendor.
   *
   * \return Device vendor.
   */

  _getDeviceVendor: function DIW__getDeviceVendor() {
    try {
      return this._device.properties.vendorName;
    } catch (err) {
      return SBString("device.info.unknown");
    }
  },


  /**
   * \brief Return the device access compatibility.
   *
   * \return Device access compatibility.
   */

  _getDeviceAccessCompatibility: function DIW__getDeviceAccessCompatibility() {
    var accessCompatibility =
          this._getDeviceProperty
                 ("http://songbirdnest.com/device/1.0#accessCompatibility",
                  SBString("device.info.unknown"));
    if (accessCompatibility == "ro")
      accessCompatibility = SBString("device.info.read_only");
    else if (accessCompatibility == "rw")
      accessCompatibility = SBString("device.info.read_write");

    return accessCompatibility;
  },

  /**
   * \brief Format a mime type to human readable
   *
   * \return Human readable information for mime type.
   */

  _getExtForFormat : function(aMimeType) {
    var mimeService = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService);
    if (mimeService) {
      var mimeExtension;
      try {
        mimeExtension = mimeService.getFromTypeAndExtension(aMimeType, null)
                                   .primaryExtension;
      } catch (err) {}
      
      if (mimeExtension) {
        return SBString("device.info.mimetype." + mimeExtension, mimeExtension);
      }
    }
    // Fall back to mime type if all else failed.
    return aMimeType;
  },

  /**
   * \brief Return the device playback formats.
   *
   * \return Device playback formats.
   */

  _getDevicePlaybackFormats: function DIW__getDevicePlaybackFormats() {
    var retFormats = [];
    try {
      var deviceCapabilities = this._device.capabilities;
      var functionArray = deviceCapabilities.getSupportedFunctionTypes({});
      for (var functionCounter = 0; functionCounter < functionArray.length; functionCounter++) {
        var contentArray = deviceCapabilities.getSupportedContentTypes(functionArray[functionCounter], {});
        for (var contentCounter = 0; contentCounter < contentArray.length; contentCounter++) {
          var formatArray = deviceCapabilities.getSupportedFormats(contentArray[contentCounter], {});
          retFormats = retFormats.concat(formatArray);
        }
      }
    } catch (err) { }

    if (retFormats.length > 0) {
      var formatIndex;
      for (formatIndex in retFormats) {
        retFormats[formatIndex] = this._getExtForFormat(retFormats[formatIndex]);
      }
    }
    return retFormats.join(", ") || SBString("device.info.unknown");
  },


  /**
   * \brief Return the device battery status.  Return the battery power level in
   *        aBatteryLevel.value.  If the device is running off of battery power,
   *        return true in aOnBatteryPower.value; otherwise, return false.
   *
   * \param aBatteryLevel       Returned battery power level.
   * \param aOnBatteryPower     True if running on battery power.
   */

  _getDeviceBatteryStatus: function DIW__getDeviceBatteryStatus
                                      (aBatteryLevel,
                                       aOnBatteryPower) {
    aBatteryLevel.value = this._getDeviceProperty
                        ("http://songbirdnest.com/device/1.0#batteryLevel", 0);
    var powerSource = this._getDeviceProperty
                        ("http://songbirdnest.com/device/1.0#powerSource", 0);
    aOnBatteryPower.value = (parseInt(powerSource) ? true : false);
  },


  /**
   * \brief Return true if device supports device spec specified by aDeviceSpec.
   *
   * \return True if device supports device spec; false otherwise.
   */

  _supportsDeviceSpec: function DIW__supportsDeviceSpec(aDeviceSpec) {
    // Check if device supports device spec.
    switch (aDeviceSpec) {
      case "model" :
      case "capacity" :
      case "model_capacity" :
      case "friendly_name" :
      case "serial_number" :
      case "vendor" :
      case "access" :
      case "playback_formats" :
      case "battery" :
        return true;

      default :
        break;
    }

    return false;
  },


  /**
   * \brief Return the device icon URL.
   *
   * \return Device icon URL.
   */

  _getDeviceIcon: function DIW__getDeviceIcon() {
    try {
      var uri = this._device.properties.iconUri;
      var spec = uri.spec;
      if (uri.schemeIs("file") && /\.ico$/(spec)) {
        // try to use a suitably sized image
        spec = "moz-icon://" + spec + "?size=64";
      }
      return spec;
    } catch (err) {
      // default image dictated by CSS.
      return null;
    }
  },


  /**
   * \brief Eject the device.
   */

  _eject: function DIW__eject() {
    this._device.eject();
  },
};


