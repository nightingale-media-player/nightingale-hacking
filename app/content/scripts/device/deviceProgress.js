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
* \file  devicePRogress.js
* \brief Javascript source for the device progress widget.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device progress widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device progress defs.
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
// Device progress services.
//
//------------------------------------------------------------------------------

var DPW = {
  //
  // Device progress object fields.
  //
  //   _widget                  Device info widget.
  //   _deviceID                Device ID.
  //

  _widget: null,
  _deviceID: null,


  /**
   * \brief Initialize the device progress services for the device progress
   *        widget specified by aWidget.
   *
   * \param aWidget             Device progress widget.
   */

  initialize: function DPW__initialize(aWidget) {
    // Get the device widget.
    this._widget = aWidget;

    // Initialize object fields.
    this._deviceID = this._widget.deviceID;

    // Initialize the device services.
    this._deviceInitialize();

    // Set up the observes elements.
    this._setupObservesElements();

    // Hide the widget by default and then update it.
    this._widget.hidden = true;
    this._update();
  },


  /**
   * \brief Finalize the device progress services.
   */

  finalize: function DPW_finalize() {
    // Finalize the device services.
    this._deviceFinalize();

    // Clear object fields.
    this._widget = null;
    this._deviceID = null;
  },


  //----------------------------------------------------------------------------
  //
  // Device progress event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle the event specified by aEvent for elements with defined
   *        actions.
   *
   * \param aEvent              Event to handle.
   */

  onAction: function DPW_onAction(aEvent) {
    // Dispatch processing of action.
    switch (aEvent.target.getAttribute("action")) {
      case "cancel" :
        this._deviceCancelOperations();
        break;

      case "finish" :
        this._widget.hidden = true;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Device progress XUL services.
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


  //----------------------------------------------------------------------------
  //
  // Internal device info services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Update the device progress UI according to the current device state.
   */

  _update: function DPW__update() {
    // Get the cancel and finish buttons.
    var cancelButton = this._getElement("cancel_operation_button");
    var finishButton = this._getElement("finish_progress_button");

    // Update the progress widget based on the device state.
    if (this._deviceIsIdle()) {
      cancelButton.hidden = true;
      finishButton.hidden = false;
    } else {
      cancelButton.hidden = false;
      finishButton.hidden = true;
      this._widget.hidden = false;
    }
  },


  /**
   * \brief Set up the observes elements.
   */

  _setupObservesElements: function DPW__setupObservesElements() {
    // Get the device status data remote prefix.
    var observesKeyPrefix = this._deviceGetStatusDRPrefix();

    // Get the list of observes taret elements.
    var deviceProgressBox = this._getElement("device_progress_box");
    var observesTgtElemList = deviceProgressBox.getElementsByAttribute
                                                  ("observestarget", "true");

    // Set up an observes child for each observes target based on the device
    // status data remote prefix.
    for (var i = 0; i < observesTgtElemList.length; i++) {
      // Get the observes target element.
      var observesTgtElem = observesTgtElemList[i];
      var observesKey = observesKeyPrefix + "." +
                        observesTgtElem.getAttribute("observeskey");
      var observesAttribute = observesTgtElem.getAttribute("observesattribute");

      // Create the observes element.
      var observesElem = document.createElement("observes");
      observesElem.setAttribute("type", "dataremote");
      observesElem.setAttribute("key", observesKey);
      observesElem.setAttribute("attribute", observesAttribute);

      // Add the observes element.
      observesTgtElem.appendChild(observesElem);
    }
  },


  //----------------------------------------------------------------------------
  //
  // Device progress device services.
  //
  //   These services provide an interface to the device object.
  //   These services register for any pertinent device notifications and call
  // _update to update the UI accordingly.  In particular, these services
  // register to receive notifications when the device state changes.
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

  _deviceInitialize: function DPW__deviceInitialize() {
    // Get the device object.
    this._device = this._getDevice(this._deviceID);

    // Set up progress status for testing.
    var statusDRPrefix = this._deviceGetStatusDRPrefix();
    SBDataSetStringValue(statusDRPrefix + ".text1",
                         "Syncing Device - Copying 4 of 20 files");
    SBDataSetStringValue(statusDRPrefix + ".text2",
                         "Copying: Global News: 8 Nov 07 AM");
    SBDataSetIntValue(statusDRPrefix + ".progress", 40);
  },


  /**
   * \brief Finalize the device services.
   */

  _deviceFinalize: function DPW__deviceFinalize() {
    // Clear object fields.
    this._device = null;
  },


  /**
   * \brief Cancel device operations.
   */

  _deviceCancelOperations: function DPW__deviceCancelOperations() {
    dump("Cancelling device operations.\n");
  },


  /**
   * \brief Return the device status data remote prefix.
   *
   * \return Device status data remote prefix.
   */

  _deviceGetStatusDRPrefix: function DPW__deviceGetStatusDRPrefix() {
    return "device." + this._deviceID + ".status";
  },


  /**
   * \brief Return true if device is idle.
   *
   * \return True if device is idle.
   */

  _deviceIsIdle: function DPW__deviceIsIdle() {
    // Return non-idle device for testing.
    return false;
  },


  /**
   * \brief Get the device object for the device ID specified by aDeviceID.
   *
   * \param aDeviceID       Device identifier.
   *
   * \return sbIDevice device object.
   */

  _getDevice: function DPW__getDevice(aDeviceID) {
    // Just return the device ID for now.
    return aDeviceID;
  }
};


