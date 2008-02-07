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
        this._deviceSpecUpdateModel();
        break;

      case "capacity" :
        this._deviceSpecUpdateCapacity();
        break;

      case "model_capacity" :
        this._deviceSpecUpdateModelCapacity();
        break;

      default :
        aRow.hidden = true;
        return;
    }

    // Show the row.
    aRow.hidden = false;
  },


  /**
   * \brief Update the device spec model row.
   */

  _deviceSpecUpdateModel: function DIW__deviceSpecUpdateModel() {
    // Set the device model label.
    var devModelLabel = this._getElement("model_value_label");
    var devModelValue = this._getDeviceModel();
    if (devModelValue != devModelLabel.getAttribute("value"))
      devModelLabel.setAttribute("value", devModelValue);
  },


  /**
   * \brief Update the device spec capacity row.
   */

  _deviceSpecUpdateCapacity: function DIW__deviceSpecUpdateCapacity() {
    // Set the device model capacity.
    var devCapLabel = this._getElement("capacity_value_label");
    var devCapValue = this._getDeviceModelSize();
    if (devCapValue != devCapLabel.getAttribute("value"))
      devCapLabel.setAttribute("value", devCapValue);
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

    // Set the device model/capacity.
    var devModelCapLabel = this._getElement("model_capacity_value_label");
    if (devModelCapValue != devModelCapLabel.getAttribute("value"))
      devModelCapLabel.setAttribute("value", devModelCapValue);
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

  handleAction: function DIW_handleAction(aEvent) {
    // Dispatch processing of action.
    switch (aEvent.target.getAttribute("action")) {
      case "eject" :
        this._eject();
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


  //----------------------------------------------------------------------------
  //
  // Internal device info services.
  //
  //----------------------------------------------------------------------------

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


  //----------------------------------------------------------------------------
  //
  // Device info device services.
  //
  //   These services provide an interface to the device object.
  //   These services register for any pertinent device notifications and call
  // _update to update the UI accordingly.
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
    // Just return the device ID for now.
    return aDeviceID;
  }
};


