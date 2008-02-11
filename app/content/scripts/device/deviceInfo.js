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
// Device info defs.
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


//------------------------------------------------------------------------------
//
// Device info services.
//
//------------------------------------------------------------------------------

var DIW = {
  //
  // Device info object fields.
  //
  //   _widget                  Device info widget.
  //   _deviceID                Device ID.
  //

  _widget: null,
  _deviceID: null,


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
    this._deviceID = this._widget.deviceID;

    // Initialize the device services.
    this._deviceInitialize();

    // Show specified elements.
    this._showElements();

    // Update the UI.
    this._update();
  },


  /**
   * \brief Finalize the device info services.
   */

  finalize: function DIW_finalize() {
    // Finalize the device services.
    this._deviceFinalize();

    // Clear object fields.
    this._widget = null;
    this._deviceID = null;
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
   * \brief Update all device spec rows according to the "devicespecs"
   *        attribute.
   */

  _deviceSpecUpdateAll: function DIW__deviceSpecUpdateAll() {
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

    // Move and update specified device spec rows into position.
    var refDeviceSpecRow = deviceSpecRows.firstChild;
    for (var i = 0; i < deviceSpecList.length; i++) {
      // Get the device spec and row.
      var deviceSpec = deviceSpecList[i];
      var deviceSpecRow = this._getDeviceSpecRow(deviceSpec);

      // Move the device spec row into position.
      if (deviceSpecRow != refDeviceSpecRow) {
        deviceSpecRows.insertBefore(deviceSpecRow, refDeviceSpecRow);
      }
      refDeviceSpecRow = deviceSpecRow.nextSibling;

      // Update the device spec row.
      this._deviceSpecUpdate(deviceSpecRow, deviceSpec);
    }
  },


  /**
   * \brief Update the device spec row specified by aRow with the device spec
   *        specified by aDeviceSpec.
   *
   * \param aRow                Device spec row to update.
   * \param aDeviceSpec         Device spec to update.
   */

  _deviceSpecUpdate: function DIW__deviceSpecUpdate(aRow, aDeviceSpec) {
    // Dispatch updating.
    switch (aDeviceSpec) {
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
        this._deviceSpecUpdateValue("playback_formats_value_label",
                                    this._getDevicePlaybackFormats());
        break;

      default :
        aRow.hidden = true;
        return;
    }

    // Show the row.
    aRow.hidden = false;
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
   * \brief Initialize the device services.
   */

  _deviceInitialize: function DIW__deviceInitialize() {
    // Get the device object.
    this._device = this._getDevice(this._deviceID);
  },


  /**
   * \brief Finalize the device services.
   */

  _deviceFinalize: function DIW__deviceFinalize() {
    // Clear object fields.
    this._device = null;
  },


  /**
   * \brief Return the device model name.
   *
   * \return Device model name.
   */

  _getDeviceModel: function DIW__getDeviceModel() {
    return "Generic 2000";
  },


  /**
   * \brief Return the device model size.
   *
   * \return Device model size.
   */

  _getDeviceModelSize: function DIW__getDeviceModelSize() {
    return "2 GB";
  },


  /**
   * \brief Return the device friendly name.
   *
   * \return Device friendly name.
   */

  _getDeviceFriendlyName: function DIW__getDeviceFriendlyName() {
    return "My Device";
  },


  /**
   * \brief Return the device serial number.
   *
   * \return Device serial number.
   */

  _getDeviceSerialNumber: function DIW__getDeviceSerialNumber() {
    return "1234567890ABC";
  },


  /**
   * \brief Return the device vendor.
   *
   * \return Device vendor.
   */

  _getDeviceVendor: function DIW__getDeviceVendor() {
    return "Sony";
  },


  /**
   * \brief Return the device access compatibility.
   *
   * \return Device access compatibility.
   */

  _getDeviceAccessCompatibility: function DIW__getDeviceAccessCompatibility() {
    return "Read-only without deletion";
  },


  /**
   * \brief Return the device playback formats.
   *
   * \return Device playback formats.
   */

  _getDevicePlaybackFormats: function DIW__getDevicePlaybackFormats() {
    return "MP3, AVI, MPEG, ASF, BMP, PICT, WAV, TIFF, OGG, AAC, FLAC, WMV, " +
           "MP2, Microsoft Word Document";
  },


  /**
   * \brief Return the device icon URL.
   *
   * \return Device icon URL.
   */

  _getDeviceIcon: function DIW__getDeviceIcon() {
    return null;
  },


  /**
   * \brief Eject the device.
   */

  _eject: function DIW__eject() {
    dump("Ejecting device.\n");
  },


  /**
   * \brief Get the device object for the device ID specified by aDeviceID.
   *
   * \param aDeviceID       Device identifier.
   *
   * \return sbIDevice device object.
   */

  _getDevice: function DIW__getDevice(aDeviceID) {
    // Use the mock device for now.
    var device =
          Cc["@songbirdnest.com/Songbird/Device/DeviceTester/MockDevice;1"]
            .createInstance(Ci.sbIDevice);
    return device;
  }
};


