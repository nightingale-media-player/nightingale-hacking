/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
* \file  deviceControl.js
* \brief Javascript source for the device control widget.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device control widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device control imported services.
//
//------------------------------------------------------------------------------

// Songbird imports.
Components.utils.import("resource://app/jsmodules/DOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbStorageFormatter.jsm");
Components.utils.import("resource://app/jsmodules/WindowUtils.jsm");

Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");


//------------------------------------------------------------------------------
//
// Device control services defs.
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
// Device control services.
//
//------------------------------------------------------------------------------

/**
 * Construct a device control services object for the widget specified by
 * aWidget.
 *
 * \param aWidget               Device control widget.
 */

function deviceControlWidget(aWidget) {
  this._widget = aWidget;
}

// Set the constructor.
deviceControlWidget.prototype.constructor = deviceControlWidget;

// Define the object.
deviceControlWidget.prototype = {
  //
  // Device control object fields.
  //
  //   _widget                  Device control widget.
  //   _device                  Device bound to device control widget.
  //   _deviceLibrary           Bound device library.
  //   _boundElem               Element bound to device control widget.
  //   _domEventListenerSet     Set of DOM event listeners.
  //   _deviceListenerAdded     True if a device listener has been added.
  //   _currentState            Current device operational state.
  //   _currentReadOnly         Current device read only state.
  //   _currentMgmtType         Current device management type.
  //

  _widget: null,
  _device: null,
  _deviceLibrary: null,
  _boundElem: null,
  _domEventListenerSet: null,
  _deviceListenerAdded: false,
  _currentState: Ci.sbIDevice.STATE_IDLE,
  _currentReadOnly: false,
  _currentMgmtType: Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL,


  //----------------------------------------------------------------------------
  //
  // Device control services.
  //
  //----------------------------------------------------------------------------

  /**
   * Bind the device control widget to a device.
   */

  bindDevice: function deviceControlWidget_bindDevice() {
    // Get the bound device ID.  Do nothing if it hasn't changed.
    var deviceID = this._widget.getAttribute("device-id");
    if ((deviceID == this._widget.deviceID) || !deviceID)
      return;

    // Get the bound device.
    var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceManager2);
    this._widget.device = deviceManager.getDevice(Components.ID(deviceID));
    this._widget.deviceID = deviceID;

    // Re-initialize the device control widget services.
    this.finalize(true);
    this.initialize();

    // Send a device bound event.  It doesn't bubble, and it can't be cancelled.
    var event = document.createEvent("Events");
    event.initEvent("deviceBound", false, false);
    this._widget.dispatchEvent(event);
  },


  /**
   * Initialize the device control services.
   */

  initialize: function deviceControlWidget_initialize() {
    var _this = this;
    var func;

    // Initialize object fields.
    this._device = this._widget.device;

    // Get the bound device library.
    if (this._device.content.libraries.length > 0) {
      this._deviceLibrary = this._device.content.libraries
                                .queryElementAt(0, Ci.sbIDeviceLibrary);
    } else {
      this._deviceLibrary = null;
    }

    // Get the bound element.
    this._getBoundElem();

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Add a DOM command event handler.
    func = function(aEvent) { _this.onAction(aEvent); };
    this._domEventListenerSet.add(this._boundElem, "command", func, false);

    // Add device event listener.
    if (this._widget.getAttribute("listentodevice") == "true") {
      var deviceEventTarget = this._device.QueryInterface
                                             (Ci.sbIDeviceEventTarget);
      deviceEventTarget.addEventListener(this);
      this._deviceListenerAdded = true;
    }

    // Force an update of the widget.
    this._update(true);
  },


  /**
   * Finalize the device info services.  If aReset is true, the finalization is
   * for a services reset, so don't clear the device control widget.
   */

  finalize: function deviceControlWidget_finalize(aReset) {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
    }
    this._domEventListenerSet = null;

    // Remove device event listener.
    if (this._deviceListenerAdded) {
      var deviceEventTarget = this._device.QueryInterface
                                             (Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }
    this._deviceListenerAdded = false;

    // Clear object fields.
    this._device = null;
    this._deviceLibrary = null;
    this._boundElem = null;
    if (!aReset)
      this._widget = null;
  },


  //----------------------------------------------------------------------------
  //
  // Device control event handling services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the event specified by aEvent for elements with defined actions.
   *
   * \param aEvent              Event to handle.
   */

  onAction: function deviceControlWidget_onAction(aEvent) {
    // Do nothing if not bound to a device.
    if (!this._device)
      return;

    // Dispatch processing of action.
    switch (aEvent.target.getAttribute("action")) {
      case "cancel" :
        this._device.cancelRequests();
        break;

      case "eject" :
        this._device.eject();
        break;

      case "format" :
        this._device.format();
        break;

      case "ignore" :
        this._ignoreDevice();
        break;

      case "get_info" :
        this._getDeviceInfo();
        break;

      case "new_playlist" :
        this._createPlaylist();
        break;

      case "rename" :
        this._renameDevice();
        break;

      case "sync" :
        this._device.syncLibraries();
        break;
      
      case "rip" :
        sbCDDeviceUtils.doCDRip(this._device);
        break;
      
      case "rescan" :
        sbCDDeviceUtils.doCDLookUp(this._device);
        break;
      
      case "toggle-mgmt" :
        this._toggleManagement();
        break;

      default :
        break;
    }
  },

  /**
   * Create a new device playlist.
   */

  _createPlaylist: function deviceControlWidget__createPlaylist() {
    // Create the playlist.
    var mediaList = this._deviceLibrary.createMediaList("simple");
    mediaList.name = SBString("playlist", "Playlist");

    // Edit the playlist service pane node.
    if (gServicePane) {
      var libSPS = Cc["@songbirdnest.com/servicepane/library;1"]
                     .getService(Ci.sbILibraryServicePaneService);
      var libDSPS = Cc["@songbirdnest.com/servicepane/device;1"]
                     .getService(Ci.sbIDeviceServicePaneService);
      var node = libSPS.getNodeForLibraryResource(mediaList);
      var deviceNode = libDSPS.getNodeForDevice(this._device);
      deviceNode.appendChild(node);
      gServicePane.startEditingNode(node);
    }
  },

  /**
   * Toggle the Management Flag for the default (first) device library.
   */
  
  _toggleManagement: function deviceControlWidget__toggleManagement() {
    if (this._deviceLibrary.mgmtType != Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL) {
      // Manual Mode
      this._deviceLibrary.mgmtType = Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL;
    }
    else {
      // One of the SYNC modes
      // Read the stored sync playlist list preferences.
      var storedSyncPlaylistList = this._deviceLibrary.getSyncPlaylistList();
      if (storedSyncPlaylistList.length > 0) {
        // The user has selected some playlists previously so default to
        // SYNC_PLAYLISTS
        this._deviceLibrary.mgmtType = Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_PLAYLISTS;
      }
      else {
        // If no playlists selected then set to SYNC_ALL
        this._deviceLibrary.mgmtType = Ci.sbIDeviceLibrary.MGMT_TYPE_SYNC_ALL;
      }
    }
    
    // Ensure our UI is all updated properly.
    this._update(true);
  },
  
  /**
   * Rename the device.
   */

  _renameDevice: function deviceControlWidget__renameDevice() {
    // Get the service pane node associated with the widget.
    var servicePaneNode = this._getServicePaneNode();

    // Edit the service pane node if specified.  Otherwise, present a rename
    // dialog.
    if (servicePaneNode && (typeof(gServicePane) != "undefined")) {
      gServicePane.startEditingNode(servicePaneNode);
    } else {
      // Get the device friendly name.
      var friendlyName = null;
      try { friendlyName = this._device.properties.friendlyName; } catch(ex) {}

      // Get the device vendor/model name.
      var vendorModelName = "";
      var vendorName = null;
      var modelNumber = null;
      try { vendorName = this._device.properties.vendorName; } catch(ex) {}
      try { modelNumber = this._device.properties.modelNumber; } catch(ex) {}
      if (!modelNumber)
        modelNumber = SBString("device.default.model.number");
      if (vendorName)
        vendorModelName += vendorName + " ";
      vendorModelName += modelNumber;

      // Prompt user for new name.
      var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                            .getService(Ci.nsIPromptService);
      var newName = { value: friendlyName };
      var ok = promptService.prompt
                 (window,
                  SBString("device.dialog.rename.title"),
                  SBFormattedString("device.dialog.rename.message",
                                    [vendorModelName]),
                  newName,
                  null,
                  {});

      // Set new device name.
      if (ok) {
        try {
          this._device.properties.friendlyName = newName.value;
        } catch (ex) {
          Cu.reportError("Cannot write device name.\n");
        }
      }
    }
  },

  /**
   * Ignore device
   * First, explain to the user what is happening, then
   * add the device to the blacklist, and "eject" it.
   */
  _ignoreDevice: function deviceControlWidget__ignoreDevice() {
    var promptService = Cc["@mozilla.org/embedcomp/prompt-service;1"]
                                  .getService(Ci.nsIPromptService);

    window.focus();
    var buttonPressed = promptService.confirmEx(window,
                          SBString('device.dialog.setup.ignoreDeviceTitle'),
                          SBString('device.dialog.setup.ignoreDevice'),
                          promptService.STD_YES_NO_BUTTONS,
                          null, null, null,
                          null, {});
    // don't set the pref unless they press OK
    acceptedBlacklist = (buttonPressed == 0);
    if (acceptedBlacklist) {
      var ignoreList = Application.prefs
                                  .getValue("songbird.device.ignorelist", "");
      // Append a separator if necessary.
      if (ignoreList != "") {
        ignoreList += ";";
      }
      ignoreList += this._device.id;
      Application.prefs.setValue("songbird.device.ignorelist", ignoreList);

      // Remove the device from the application
      var manager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                      .getService(Ci.sbIDeviceManager2);
      var controller = manager.getController(this._device.controllerId);
      controller.releaseDevice(this._device);
    }
    return acceptedBlacklist;
  },

  /**
   * Get the device information and show it to the user.
   */

  _getDeviceInfo: function deviceControlWidget__getDeviceInfo() {
    // Show the device info dialog.
    WindowUtils.openModalDialog
      (window,
       "chrome://songbird/content/xul/device/deviceInfoDialog.xul",
       "",
       "chrome,centerscreen",
       [ this._device ],
       null);
  },


  //----------------------------------------------------------------------------
  //
  // Device control sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle the device event specified by aEvent.
   *
   * \param aEvent              Device event.
   */

  onDeviceEvent: function DeviceSyncWidget_onDeviceEvent(aEvent) {
    // Dispatch processing of the event.
    switch(aEvent.type)
    {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_PREFS_CHANGED :
        this._update();
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED :
        this._update();
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal device control device services.
  //
  //----------------------------------------------------------------------------

  /**
   * Return the device model/capacity string (e.g., "Apple iPod(4GB)").
   *
   * \return Device model/capacity string.
   */

  _getDeviceModelCap: function deviceControlWidget__getDeviceModelCap() {
    // Get the device vendor name.
    var vendorName = "";
    try {
      vendorName = this._device.properties.vendorName;
    } catch (ex) {};

    // Get the device model number.
    var modelNumber = "";
    try {
      modelNumber = this._device.properties.modelNumber;
    } catch (ex) {};

    // Get the device capacity.
    var capacity = "";
    try {
      capacity = this._device.properties.properties.getPropertyAsAString
                   ("http://songbirdnest.com/device/1.0#capacity");
      capacity = StorageFormatter.format(capacity);
    } catch (ex) {};

    return SBFormattedString("device.info.model_cap",
                             [ vendorName, modelNumber, capacity ]);
  },


  /**
   * Return true if device is read-only; return false otherwise.
   *
   * \return true if device is read-only.
   */

  _isReadOnly: function deviceControlWidget__isReadOnly() {
    var readOnly;

    // Treat device as read-only if it doesn't have a library.
    if (!this._deviceLibrary)
      return true;

    // This is just preventative measure in case the library is being
    // destroyed or bad state, we'll just treat as readOnly
    try {
      // Set to read-only if not in manual management mode.
      if (this._deviceLibrary.mgmtType == Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL)
        readOnly = false;
      else
        readOnly = true;
    }
    catch (e) {
      readOnly = true;
    }
    return readOnly;
  },


  //----------------------------------------------------------------------------
  //
  // Internal device control services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the widget to reflect device state changes.  Do nothing if no device
   * state has changed and aForce is false.
   *
   * \param aForce              Force an update.
   */

  _update: function deviceControlWidget__update(aForce) {
    // Get the device state.
    var state = this._device.state;
    var readOnly = this._isReadOnly();
    var supportsReformat = this._device.supportsReformat;
    var supportsPlaylist = this._supportsPlaylist();
    var msc = (this._device.parameters.getProperty("DeviceType") == "MSCUSB");

    var mgmtType;
    // This is just preventative measure in case the library is being
    // destroyed, we'll just treat as readOnly
    try {
      // Get the management type for the device library.  Default to manual if no
      // device library.
      mgmtType = (this._deviceLibrary ? this._deviceLibrary.mgmtType
                                        : Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL);
    }
    catch (e) {
      return;
    }
    // Do nothing if no device state changed and update is not forced.
    if (!aForce &&
        (this._currentState == state) &&
        (this._currentReadOnly == readOnly) &&
        (this._currentMgmtType == mgmtType) &&
        (this._currentSupportsReformat == supportsReformat) &&
        (this._currentSupportsPlaylist == supportsPlaylist) &&
        (this._currentMsc == msc)) {
      return;
    }

    // Update the current state.
    this._currentState = state;
    this._currentReadOnly = readOnly;
    this._currentMgmtType = mgmtType;
    this._currentSupportsReformat = supportsReformat;
    this._currentSupportsPlaylist = supportsPlaylist;
    this._currentMsc = msc;

    // Update widget attributes.
    var updateAttributeList = [];
    this._updateAttribute("accesskey", updateAttributeList);
    this._updateAttribute("action", updateAttributeList);
    this._updateAttribute("disabled", updateAttributeList);
    this._updateAttribute("label", updateAttributeList);
    this._updateAttribute("hidden", updateAttributeList);
    this._updateAttribute("checked", updateAttributeList);

    // Update bound element attributes.
    if (this._boundElem != this._widget) {
      DOMUtils.copyAttributes(this._widget,
                              this._boundElem,
                              updateAttributeList,
                              true);
    }
  },

  /**
   * Check the device capabilities to see if it supports playlists.
   * Device implementations may respond to CONTENT_PLAYLIST for either 
   * FUNCTION_DEVICE or FUNCTION_AUDIO_PLAYBACK.
   */
  _supportsPlaylist: function deviceControlWidget__supportsPlaylist() {
    var capabilities = this._device.capabilities;
    var sbIDC = Ci.sbIDeviceCapabilities;
    try {
      var contentTypes = capabilities.
        getSupportedContentTypes(sbIDC.FUNCTION_DEVICE, {});
      for (var i in contentTypes) {
        if (contentTypes[i] == sbIDC.CONTENT_PLAYLIST) {
          return true;
        }
      }
    } catch (e) {}

    try {
      var contentTypes = capabilities
        .getSupportedContentTypes(sbIDC.FUNCTION_AUDIO_PLAYBACK, {});
      for (var i in contentTypes) {
        if (contentTypes[i] == sbIDC.CONTENT_PLAYLIST) {
          return true;
        }
      }
    } catch (e) {}

    // couldn't find PLAYLIST support in either the DEVICE 
    // or AUDIO_PLAYBACK category
    return false;
  },

  /**
   * Update the widget attribute specified by aAttrName to reflect changes to
   * the device state.  If a value is specified for the attribute, add the
   * attribute name to the attribute update list specified by aAttrList.
   *
   * \param aAttrName           Name of attribute to update.
   * \param aAttrList           List of attributes to update.
   */

  _updateAttribute: function deviceControlWidget__updateAttribute(aAttrName,
                                                                  aAttrList) {
    // Get the attribute value for the current state.
    var attrVal = {};
    if (this._currentReadOnly &&
        this._getStateAttribute(attrVal, aAttrName, "readonly")) {}
    else if (!this._currentReadOnly &&
             this._getStateAttribute(attrVal, aAttrName, "readwrite")) {}
    else if ((this._currentState == Ci.sbIDevice.STATE_CANCEL) &&
             this._getStateAttribute(attrVal, aAttrName, "cancel")) {}
    else if ((this._currentState == Ci.sbIDevice.STATE_COPYING) &&
             this._getStateAttribute(attrVal, aAttrName, "copy")) {}
    else if ((this._currentState == Ci.sbIDevice.STATE_DELETING) &&
             this._getStateAttribute(attrVal, aAttrName, "delete")) {}
    else if ((this._currentState == Ci.sbIDevice.STATE_MOUNTING) &&
             this._getStateAttribute(attrVal, aAttrName, "mount")) {}
    else if ((this._currentState == Ci.sbIDevice.STATE_SYNCING) &&
             this._getStateAttribute(attrVal, aAttrName, "sync")) {}
    else if ((this._currentState == Ci.sbIDevice.STATE_UPDATING) &&
             this._getStateAttribute(attrVal, aAttrName, "update")) {}
    else if ((this._currentState == Ci.sbIDevice.STATE_TRANSCODE) &&
             this._getStateAttribute(attrVal, aAttrName, "transcode")) {}
    else if ((this._currentState == Ci.sbIDevice.STATE_FORMATTING) &&
             this._getStateAttribute(attrVal, aAttrName, "format")) {}
    else if ((this._currentState != Ci.sbIDevice.STATE_IDLE) &&
             this._getStateAttribute(attrVal, aAttrName, "busy")) {}
    else if ((this._currentState == Ci.sbIDevice.STATE_IDLE) &&
             this._getStateAttribute(attrVal, aAttrName, "idle")) {}
    else if ((this._currentMgmtType != Ci.sbIDeviceLibrary.MGMT_TYPE_MANUAL) &&
             this._getStateAttribute(attrVal, aAttrName, "mgmt_not_manual")) {}
    else if (this._currentSupportsReformat && 
             this._getStateAttribute(attrVal, aAttrName, "supports_reformat")) {}
    else if (this._currentMsc &&
             this._getStateAttribute(attrVal, aAttrName, "msc")) {}
    else if (this._currentSupportsPlaylist && 
             this._getStateAttribute(attrVal, aAttrName, "supports_playlist")) {}
    else if (this._getStateAttribute(attrVal, aAttrName, "default")) {}
    else this._getStateAttribute(attrVal, aAttrName, null);

    // Do nothing if no attribute value to set.
    if ((typeof(attrVal.value) == "undefined") || (attrVal.value == null))
      return;
    attrVal = attrVal.value;

    // If attribute value is "remove_attribute", remove the attribute instead
    // of setting it.
    var removeAttr = false;
    if (attrVal == "remove_attribute")
      removeAttr = true;

    // Replace "device_model_cap" in attribute value with the device
    // model/capacity string.
    if (attrVal.match(/device_model_cap/)) {
      var deviceModelCap = this._getDeviceModelCap();
      attrVal = attrVal.replace(/device_model_cap/g, deviceModelCap);
    }

    // Update attribute value.
    if (!removeAttr)
      this._widget.setAttribute(aAttrName, attrVal);
    else
      this._widget.removeAttribute(aAttrName);
    aAttrList.push(aAttrName);
  },


  /**
   *   Get the state attribute value for the attribute with the name specified
   * by aAttrName and the state specified by aAttrState.  Return the state
   * attribute value in aAttrVal.  If a state attribute value is available,
   * return true; otherwise return false and don't return anything in aAttrVal.
   *   If aAttrState is not specified, simply return the attribute with the name
   * specified by aAttrName.
   *
   * \param aAttrVal            Returned state attribute value.
   * \param aAttrName           Attribute name.
   * \param aAttrState          State.
   *
   * \return                    True if state attribute value is available.
   */

  _getStateAttribute: function deviceControlWidget__getStateAttribute
                                 (aAttrVal,
                                  aAttrName,
                                  aAttrState) {
    // Get the state attribute name.
    var stateAttrName = aAttrName;
    if (aAttrState)
      stateAttrName += "_" + aAttrState;

    // Return false if no state attribute value available.
    if (!this._widget.hasAttribute(stateAttrName))
      return false;

    // Return the state attribute value.
    aAttrVal.value = this._widget.getAttribute(stateAttrName);
    return true;
  },


  /**
   * Get the element bound to the device control widget.  Default to binding the
   * widget to itself.
   */

  _getBoundElem: function deviceControlWidget__getBoundElem() {
    var boundElem;

    // Get the bound element from the bind element attributes.
    boundElem = this._getElementFromSpec(this._widget, "bindforward");
    boundElem = this._getElementFromSpec(boundElem, "bindelem");

    // Set the bound element.
    this._boundElem = boundElem;
    this._widget.boundElem = boundElem;
  },


  /**
   * Search for and return the element specified by the element spec
   * aElementSpec, starting from the element specified by aElementStart.
   *
   * \param aElementStart       Element from which to start search.
   * \param aElementSpec        Element search specification.
   *
   * \return                    Found element.
   */

  _getElementFromSpec: function deviceControlWidget__getElement(aElementStart,
                                                                aElementSpec) {
    // Get the element from the spec.
    var element;
    var elementSpec = this._widget.getAttribute(aElementSpec).split("=");
    switch (elementSpec[0]) {
      case "prev" :
        element = aElementStart.previousSibling;
        while (element && (element.nodeType != Node.ELEMENT_NODE)) {
          element = element.previousSibling;
        }
        break;

      case "next" :
        element = aElementStart.nextSibling;
        while (element && (element.nodeType != Node.ELEMENT_NODE)) {
          element = element.nextSibling;
        }
        break;

      case "parent" :
        element = aElementStart.parentNode;
        break;

      default :
        element = aElementStart;
        break;
    }

    return element;
  },


  /**
   * Return the service pane node associated with the widget.
   *
   * \return                    Service pane node associated with widget.
   */

  _getServicePaneNode: function deviceControlWidget__getServicePaneNode() {
    // Get the service pane node ID.
    var servicePaneNodeID = this._widget.getAttribute("service_pane_node_id");
    if (!servicePaneNodeID)
      return null;

    // Get the service pane node.
    var servicePaneService = Cc["@songbirdnest.com/servicepane/service;1"]
                               .getService(Ci.sbIServicePaneService);
    servicePaneNode = servicePaneService.getNode(servicePaneNodeID);

    return servicePaneNode;
  }
};


