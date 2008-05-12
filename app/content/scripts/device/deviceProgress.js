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
Cu.import("resource://app/components/WindowUtils.jsm");

/**
 * \brief Constants for the completed states.
 */
const STATE_COMPLETE_SYNC           = Ci.sbIDevice.STATE_USER + 1;
const STATE_COMPLETE_COPY           = Ci.sbIDevice.STATE_USER + 2;
const STATE_COMPLETE_DELETE         = Ci.sbIDevice.STATE_USER + 3;
const STATE_COMPLETE_UPDATE         = Ci.sbIDevice.STATE_USER + 4;
const STATE_COMPLETE_MOUNTING       = Ci.sbIDevice.STATE_USER + 5;



//------------------------------------------------------------------------------
//
// Device progress services.
//
//------------------------------------------------------------------------------

var DPW = {
  //
  // Device progress object fields.
  //
  //  _widget                   Device info widget.
  //  _progressInfoBox          Device progress info.
  //  _deviceID                 Device ID.
  //  _inSyncMode               Indicates if the device is in sync mode.
  //  _currentState             Current state of the device (does not always
  //                            match sbIDevice.state)
  //  _curItemIndex             Current index in the batch of items to process
  //  _totalItems               Total items in the batch
  //  _itemName                 Track name of the last item processed
  //  _itemArtist               Artist name of the last item processed
  //  _itemAlbum                Album name of the last item processed
  //

  _widget: null,
  _progressInfoBox: null,
  _deviceID: null,

  _inSyncMode: false,
  _currentState: Ci.sbIDevice.STATE_IDLE,
  
  _curItemIndex: null,
  _totalItems: null,
  _itemName: null,
  _itemArtist: null,
  _itemAlbum: null,

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

    var createDataRemote = new Components.Constructor(
                                    "@songbirdnest.com/Songbird/DataRemote;1",
                                    Ci.sbIDataRemote,
                                    "init");
    this._totalItems = createDataRemote(
                    this._deviceID + ".status.totalcount", null);
    this._curItemIndex = createDataRemote(
                    this._deviceID + ".status.workcount", null);

    // Set up the observes elements.
    this._setupObservesElements();

    // Hide the progress info by default and then update it.
    this._progressInfoBox.hidden = true;
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
        this._finish();
      break;

      case "show_errors":
        this._displayErrors();
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

  _updateDisplay: function DPW__updateDisplay(aOperationHead,
                                              aOperationFoot,
                                              aBusy) {
    var syncButton   = this._getElement("sync_operation_button");
    var cancelButton = this._getElement("cancel_operation_button");
    var finishButton = this._getElement("finish_progress_button");
    var errorLink = this._getElement("progress_text1_label");
    var progressMeter = this._getElement("progress_meter");

    if (aBusy) {
      this._dText1Remote.stringValue =
                  SBFormattedString("device.status.progress_header_" +
                                      aOperationHead,
                                    [this._curItemIndex.intValue,
                                     this._totalItems.intValue]);
      this._dText2Remote.stringValue =
                  SBFormattedString("device.status.progress_footer_" +
                                      aOperationFoot,
                                    [this._itemName,
                                     this._itemArtist,
                                     this._itemAlbum]);

      if (this._currentState != Ci.sbIDevice.STATE_MOUNTING) {
        // Now show progress for all items rather than each one.
        var curProgress = Math.round((this._curItemIndex.intValue / this._totalItems.intValue) * 100);
        progressMeter.setAttribute("mode", "determined");
        this._dProgressRemote.intValue = curProgress;
      } else {
        progressMeter.setAttribute("mode", "undetermined");
        this._dProgressRemote.intValue = 0;
      }

      this._progressInfoBox.hidden = false;
      cancelButton.hidden = false;
      syncButton.hidden = true;
      finishButton.hidden = true;
      errorLink.removeAttribute("error");
    } else {
      this._dText1Remote.stringValue =
                SBFormattedString("device.status.progress_complete_" +
                                    aOperationHead,
                                  [this._totalItems.intValue]);
      this._dText2Remote.stringValue =
                                  SBString("device.status.progress_idle");

      progressMeter.setAttribute("mode", "determined");
      this._dProgressRemote.intValue = 100;

      this._progressInfoBox.hidden = false;
      cancelButton.hidden = true;
      syncButton.hidden = true;
      finishButton.hidden = false;

      this._checkForErrors();
    }
  },

  /**
   * \brief Update the device progress UI according to the current device state.
   */

  _update: function DPW__update() {
    // Get the sync, cancel and finish buttons.
    var syncButton   = this._getElement("sync_operation_button");
    var cancelButton = this._getElement("cancel_operation_button");
    var finishButton = this._getElement("finish_progress_button");

    switch (this._currentState) {
      case Ci.sbIDevice.STATE_SYNCING:
        var propertyStub = "device.status.progress_footer_syncing_";
        var propertyKey = "copying";
        switch(this._device.state) {
          case Ci.sbIDevice.STATE_COPYING:
            propertyKey = "copying";
          break;
          case Ci.sbIDevice.STATE_UPDATING:
            propertyKey = "updating";
          break;
          case Ci.sbIDevice.STATE_DELETING:
            propertyKey = "deleting";
          break;
        }

        this._updateDisplay("syncing", propertyStub + propertyKey, true);
      break;
    
      case STATE_COMPLETE_SYNC:
        this._updateDisplay("syncing", "syncing", true);
      break;

      case Ci.sbIDevice.STATE_MOUNTING:
        this._updateDisplay("mounting", "mounting", true);
      break;
    
      case STATE_COMPLETE_MOUNTING:
        this._updateDisplay("mounting", "mounting", false);
      break;
    
      case Ci.sbIDevice.STATE_COPYING:
        this._updateDisplay("copying", "copying", true);
      break;
      case STATE_COMPLETE_COPY:
        this._updateDisplay("copying", "copying", false);
      break;

      case Ci.sbIDevice.STATE_UPDATING:
        this._updateDisplay("updating", "updating", true);
      break;
      case STATE_COMPLETE_UPDATE:
        this._updateDisplay("updating", "updating", false);
      break;

      case Ci.sbIDevice.STATE_DELETING:
        this._updateDisplay("deleting", "deleting", true);
      break;
      case STATE_COMPLETE_DELETE:
        this._updateDisplay("deleting", "deleting", false);
      break;

      //case Ci.sbIDevice.STATE_IDLE:
      default:
        this._dText1Remote.stringValue = SBString("device.status.progress_idle");
        this._dText2Remote.stringValue = SBString("device.status.progress_idle");
        this._progressInfoBox.hidden = true;
        cancelButton.hidden = true;
        syncButton.hidden = false;
        finishButton.hidden = true;
      break;
    }
  },

  _displayErrors: function DPW__displayErrors() {
    try {
      var deviceErrorMonitor = Cc["@songbirdnest.com/device/error-monitor-service;1"]
                                 .getService(Ci.sbIDeviceErrorMonitor);
      if (deviceErrorMonitor.deviceHasErrors(this._device)) {
        var errorItems = deviceErrorMonitor.getErrorsForDevice(this._device);
        WindowUtils.openModalDialog
                   (window,
                    "chrome://songbird/content/xul/device/deviceErrorDialog.xul",
                    "device_error_dialog",
                    "chrome,centerscreen",
                    [ "", this._device, errorItems ],
                    null);
      }
    } catch (err) {
      Cu.reportError(err);
    }
  },
  
  /**
   * \brief Cleans up the progress widget on the summary page to an IDLE state.
   */

  _finish: function DPW__finish() {
    var errorLink = this._getElement("progress_text1_label");
    errorLink.removeAttribute("error");
    this._currentState = Ci.sbIDevice.STATE_IDLE;
    try {
      var deviceErrorMonitor = Cc["@songbirdnest.com/device/error-monitor-service;1"]
                                 .getService(Ci.sbIDeviceErrorMonitor);
      deviceErrorMonitor.clearErrorsForDevice(this._device);
    } catch (err) {
      Cu.reportError(err);
    }
    this._update();
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

  _checkForErrors: function DPW__checkForErrors() {
    var errorLink = this._getElement("progress_text1_label");
    errorLink.removeAttribute("error");
    try {
      var deviceErrorMonitor = Cc["@songbirdnest.com/device/error-monitor-service;1"]
                                 .getService(Ci.sbIDeviceErrorMonitor);
      var hasErrors = deviceErrorMonitor.deviceHasErrors(this._device);
      errorLink.setAttribute("error", hasErrors);
    } catch (err) {
      Cu.reportError(err);
    }
  },

  _clearDeviceErrors: function DPW__clearDeviceErrors() {
    try {
      var deviceErrorMonitor = Cc["@songbirdnest.com/device/error-monitor-service;1"]
                                 .getService(Ci.sbIDeviceErrorMonitor);
      deviceErrorMonitor.clearErrorsForDevice(this._device);
    } catch (err) {
      Cu.reportError(err);
    }
  },
  
  /**
   * \brief Updates the current state of the device for the UI
   *
   * \param aState state from the device
   */
  _updateDeviceState : function DPW__updateDeviceState(aState) {
    // Ignore if the state has not changed
    if (aState == this._currentState) {
      return;
    }

    if (this._currentState == Ci.sbIDevice.STATE_IDLE) {
      // Check if we are done the batch, if not, ignore this state change
      if (this._curItemIndex.intValue != this._totalItems.intValue) {
        return;
      }
    }

    var newState = aState;

    switch (this._currentState) {
      case Ci.sbIDevice.STATE_SYNCING:
        if (aState == Ci.sbIDevice.STATE_IDLE) {
          newState = STATE_COMPLETE_SYNC;
        }
      break;
    
      case Ci.sbIDevice.STATE_COPYING:
        if (aState == Ci.sbIDevice.STATE_IDLE) {
          newState = STATE_COMPLETE_COPY;
        }
      break;
    
      case Ci.sbIDevice.STATE_UPDATING:
        if (aState == Ci.sbIDevice.STATE_IDLE) {
          newState = STATE_COMPLETE_UPDATE;
        }
      break;
    
      case Ci.sbIDevice.STATE_DELETING:
        if (aState == Ci.sbIDevice.STATE_IDLE) {
          newState = STATE_COMPLETE_DELETE;
        }
      break;
    
      case Ci.sbIDevice.STATE_MOUNTING:
      case STATE_COMPLETE_MOUNTING:
        if (aState == Ci.sbIDevice.STATE_IDLE) {
          newState = STATE_COMPLETE_MOUNTING;
        }
      break;
    
      default:
        newState = aState;
      break;
    }

    this._currentState = newState;
  },

  /**
   * \brief Updates the device sync/copy status
   *
   * \param aMediaItem the media item that is currently being processed.
   */

  _updateDeviceInfo : function DPW__updateDeviceInfo(aMediaItem) {

    if (aMediaItem) {
      this._itemName = aMediaItem.getProperty(SBProperties.trackName);
      this._itemArtist = aMediaItem.getProperty(SBProperties.artistName);
      this._itemAlbum = aMediaItem.getProperty(SBProperties.albumName);
    } else {
      this._itemName = SBString("device.info.unknown");
      this._itemArtist = SBString("device.info.unknown");
      this._itemAlbum = SBString("device.info.unknown");
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
      case Ci.sbIDeviceEvent.EVENT_DEVICE_SYNC_END:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_END:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_END:
        var aMediaItem = aEvent.data;
        if (aMediaItem instanceof Ci.sbIMediaItem) {
          aMediaItem.QueryInterface(Ci.sbIMediaItem);
        } else {
          aMediaItem = null;
        }
        this._updateDeviceInfo(aMediaItem);
        this._updateDeviceState(Ci.sbIDevice.STATE_IDLE);
        this._update();
      break;
    
      case Ci.sbIDeviceEvent.EVENT_DEVICE_SYNC_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_SYNC_PROGRESS:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_PROGRESS:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_PROGRESS:
        var aMediaItem = aEvent.data;
        if (aMediaItem instanceof Ci.sbIMediaItem) {
          aMediaItem.QueryInterface(Ci.sbIMediaItem);
        } else {
          aMediaItem = null;
        }
        this._updateDeviceInfo(aMediaItem);
        this._updateDeviceState(this._device.state);
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
      this._updateDeviceInfo(null);
      this._updateDeviceState(Ci.sbIDevice.STATE_IDLE);
      this._update();
    } catch (e) {
      dump("Error: " + e);
      Cu.reportError("Error occurred when canceling requests: " + e);
    }
  },

  /**
   * \brief Sync device
   */
  _deviceSync: function DPW__deviceSync() {
    try {
      this._progressInfoBox.hidden = false;
      this._device.syncLibraries();
    } catch (e) {
      dump("Error: " + e + "\n");
      Cu.reportError("Error occurred when attempting to sync: " + e);
      this._currentState = Ci.sbIDevice.STATE_IDLE;
      this._updateDeviceInfo(null);
      this._update();
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
  }
};


