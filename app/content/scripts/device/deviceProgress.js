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

Cu.import("resource://app/components/sbProperties.jsm");

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
  //   _progressInfoBox         Device progress info.
  //   _deviceID                Device ID.
  //   _isIdle                  Flag for state of device on manual transfers.
  //   _isSyncing               Flag for sync state of device for syncing.
  //   _showFinished            Flag for showing the finished button
  //

  _widget: null,
  _progressInfoBox: null,
  _deviceID: null,
  _isIdle: true,
  _isSyncing: false,
  _showFinished: false,


  /**
   * \brief Initialize the device progress services for the device progress
   *        widget specified by aWidget.
   *
   * \param aWidget             Device progress widget.
   */

  initialize: function DPW__initialize(aWidget) {
    // Get the device widget.
    this._widget = aWidget;
    
    // Get the progress info box so we can show/hide it when required
    this._progressInfoBox = this._getElement("progress_information_box");

    // Initialize object fields.
    this._deviceID = this._widget.deviceID;
    this._device = this._widget.device;

    // Initialize the device services.
    this._deviceInitialize();

    // Set up the observes elements.
    this._setupObservesElements();

    // Hide the progress info by default and then update it.
    this._progressInfoBox.hidden = true;
    this._showFinished = false;
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
    this._progressInfoBox = null;
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

      case "sync" :
        this._deviceSync();
        break;

      case "finish" :
        this._progressInfoBox.hidden = true;
        this._showFinished = false;
        this._update();
      break;

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
    // Get the sync, cancel and finish buttons.
    var syncButton   = this._getElement("sync_operation_button");
    var cancelButton = this._getElement("cancel_operation_button");
    var finishButton = this._getElement("finish_progress_button");

    // Update the progress widget based on the device state.
    if (this._showFinished) {
      syncButton.hidden = true;
      cancelButton.hidden = true;
      finishButton.hidden = false;
      this._progressInfoBox.hidden = false;
    } else {
      finishButton.hidden = true;
      if (this._deviceIsSyncing()) {
        syncButton.hidden = true;
        cancelButton.hidden = false;
        this._progressInfoBox.hidden = false;
      } else {
        if (this._deviceIsIdle()) {
          cancelButton.hidden = true;
          syncButton.hidden = false;
        } else {
          cancelButton.hidden = false;
          syncButton.hidden = true;
          this._progressInfoBox.hidden = false;
        }
      }
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
  //   _dProgressRemote         DataRemote for progress
  //   _dText1Remote            DataRemote for Header text on progress
  //   _dText2Remote            DataRemote for bottom text on progress
  //

  _device: null,
  _dProgressRemote: null,
  _dText1Remote: null,
  _dText2Remote: null,


  /**
   * \brief Initialize the device services.
   */

  _deviceInitialize: function DPW__deviceInitialize() {
    // Add a listener for status operations
    if (this._device) {
      var deviceEventTarget = this._device;
      deviceEventTarget.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.addEventListener(this);

      this._dText1Remote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                                     .createInstance(Ci.sbIDataRemote);
      this._dText1Remote.init("device." + this._deviceID + ".status.text1", null);

      this._dText2Remote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                                     .createInstance(Ci.sbIDataRemote);
      this._dText2Remote.init("device." + this._deviceID + ".status.text2", null);
  
      this._dProgressRemote = Cc["@songbirdnest.com/Songbird/DataRemote;1"]
                                     .createInstance(Ci.sbIDataRemote);
      this._dProgressRemote.init("device." + this._deviceID + ".status.progress", null);
    }
  },

  /**
   * \brief Finalize the device services.
   */

  _deviceFinalize: function DPW__deviceFinalize() {
    // Clear object fields.
    if (this._device) {
      var deviceEventTarget = this._device;
      deviceEventTarget.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
      this._dProgressRemote.unbind();
    }
    this._device = null;
  },

  /**
   * \brief Gets a pref value (string) or returns a default if not found.
   *
   * \param aPrefID id of preference to get.
   * \param aDefaultValue default value to retun in case of error.
   *
   * \return string value of preference or aDefaultValue if error occurs.
   */

  _getPrefValue : function DPW__getPrefValue(aPrefID, aDefaultValue) {
    var prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);
    try {
      return prefService.getCharPref(aPrefID);
    } catch (err) {
      return aDefaultValue;
    }
  },

  /**
   * \brief Updates the device sync/copy status
   *
   * \param aMediaItem the media item that is currently being processed.
   */

  _updateDeviceStatus : function DPW__updateDeviceStatus(aMediaItem) {
/*
 * This will update the progress for each item
 * We are currently going to show a progress for total items instead (see below)
 * 
    var curProgress = this._getPrefValue("songbird." + this._deviceID +
                                         ".status.progress", 0);
    curProgress = Math.round(parseInt(curProgress) / 100);
*/
    var totalItems = this._getPrefValue("songbird." + this._deviceID +
                                          ".status.totalcount", 0);
    var curItemIndex = this._getPrefValue("songbird." + this._deviceID +
                                        ".status.workcount", 0);
    var curOperation = this._getPrefValue("songbird." + this._deviceID +
                                        ".status.operation",
                                        SBString("device.info.unknown"));

    // Now show progress for all items rather than each one.
    var curProgress = Math.round((curItemIndex / totalItems) * 100);

    this._dText1Remote.stringValue =
              SBFormattedString("device.status.progress_header_" + curOperation,
                                [curItemIndex, totalItems]);
    this._dProgressRemote.intValue = curProgress;

    if (aMediaItem) {
      var pItemName = aMediaItem.getProperty(SBProperties.trackName);
      var pItemArtist = aMediaItem.getProperty(SBProperties.artistName);
      var pItemAlbum = aMediaItem.getProperty(SBProperties.albumName);
      this._dText2Remote.stringValue =
          SBFormattedString("device.status.progress_footer_" + curOperation,
                            [pItemName, pItemArtist, pItemAlbum]);
    }
  },

  /**
   * \brief Listener for device events.
   *
   * \sa sbIDeviceEvent.idl
   */

  onDeviceEvent : function DPW_onDeviceEvent(aEvent) {
    // Something happened :)
    switch (aEvent.type) {
      /*
      case Ci.sbIDeviceEvent.EVENT_DEVICE_SYNC_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_SYNC_PROGRESS:
        var aMediaItem = aEvent.data;
        if (aMediaItem)
          aMediaItem.QueryInterface(Ci.sbIMediaItem);
        this._updateDeviceStatus(aMediaItem);
        this._isSyncing = true;
        this._update();
      break;
      case Ci.sbIDeviceEvent.EVENT_DEVICE_SYNC_STOP:
        var aMediaItem = aEvent.data;
        if (aMediaItem)
          aMediaItem.QueryInterface(Ci.sbIMediaItem);
        this._updateDeviceStatus(aMediaItem);
        this._isSyncing = false;
        this._showFinished = true;
        this._update();
      break;
      */

      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_PROGRESS:
        var aMediaItem = aEvent.data;
        if (aMediaItem)
          aMediaItem.QueryInterface(Ci.sbIMediaItem);
        this._updateDeviceStatus(aMediaItem);
        this._isIdle = false;
        this._update();
      break;
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_END:
        this._dText1Remote.stringValue = SBString("device.status.progress_complete");
        this._dText2Remote.stringValue = SBString("device.status.progress_idle");
        this._dProgressRemote.intValue = 100;
        this._isIdle = true;
        this._showFinished = true;
        this._update();
      break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_PROGRESS:
        this._dText1Remote.stringValue = SBString("device.status.progress_header_mounting");
        this._dText2Remote.stringValue = SBString("device.status.progress_footer_mounting");

        var progressMeter = this._getElement("progress_meter");
        progressMeter.setAttribute("mode", "undetermined");

        this._dProgressRemote.intValue = 0;
        this._isIdle = false;
        this._update();
      break;
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_END:
        this._dText1Remote.stringValue = SBString("device.status.progress_complete_mounting");
        this._dText2Remote.stringValue = SBString("device.status.progress_idle");
        this._isIdle = true;

        var progressMeter = this._getElement("progress_meter");
        progressMeter.setAttribute("mode", "determined");

        this._dProgressRemote.intValue = 100;
        this._update();
      break;

    }
  },

  /**
   * \brief Cancel device operations.
   */

  _deviceCancelOperations: function DPW__deviceCancelOperations() {
    try {
      this._device.cancelRequests();
      this._isSyncing = false;
      this._isIdle = true;
      this._showFinished = true;
      this._update();
      this._test_cancel = true;       // STEVO: For test syncing below.
    } catch (e) {
      dump("Error: " + e);
      Cu.reportError("Error occurred when canceling requests: " + e);
    }
  },

// STEVO: TEST SYNC STUFF...
  _test_curItemIndex: 0,
  _test_totalItems: 20,
  _test_cancel: false,
  
  _testSync: function DPW__testSync() {
    if (this._test_cancel) {
      this._isSyncing = false;
      this._showFinished = true;
      this._update();
      return;
    }
    
    var progress = Math.round((this._test_curItemIndex / this._test_totalItems) * 100);
    this._dProgressRemote.intValue = progress;
    if (progress < 100) {
      this._progressInfoBox.hidden = false;
      this._isSyncing = true;

      this._dText1Remote.stringValue =
                SBFormattedString("device.status.progress_header_copying",
                                  [this._test_curItemIndex,
                                   this._test_totalItems]);
      this._dText2Remote.stringValue =
          SBFormattedString("device.status.progress_footer_copying",
                            ["Test " + this._test_curItemIndex,
                             "The Testsers",
                             "Sync Test"]);

      self = this;
      setTimeout(function () { self._testSync(); }, 1000);
    } else {
      this._isSyncing = false;
      this._showFinished = true;
      this._dText1Remote.stringValue = SBString("device.status.progress_complete");
      this._dText2Remote.stringValue = SBString("device.status.progress_idle");
      this._dProgressRemote.intValue = 100;
    }

    this._update();
    this._test_curItemIndex += 1;
  },
// STEVO: END TEST SYNC STUFF.
  /**
   * \brief Sync device
   */
  _deviceSync: function DPW__deviceSync() {
    try {
      this._device.syncLibraries();
      this._progressInfoBox.hidden = false;
    } catch (e) {
      this._test_curItemIndex = 0;  // STEVO: TEST SYNC STUFF
      this._test_cancel = false;    // STEVO: TEST SYNC STUFF
      this._testSync();             // STEVO: TEST SYNC STUFF
      dump("Error: " + e + "\n");
      Cu.reportError("Error occurred when attempting to sync: " + e);
    }
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
    return this._isIdle;
  },

  /**
   * \brief Return true if device is syncing
   */
  _deviceIsSyncing: function DPW__deviceIsSyncing() {
    return this._isSyncing;
  },
};


