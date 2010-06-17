/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
* \file  deviceSummarySettings.js
* \brief Javascript source for initializing the device summary page, settings tab
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device summary settings widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device summary settings defs.
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

Cu.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/DOMUtils.jsm");

if (typeof(XUL_NS) == "undefined")
  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var DeviceSummarySettings = {
  //
  // Device Sync Tabs object fields.
  //
  //   _widget                  Device sync tabs widget.
  //   _device                  sbIDevice object.
  //   _settingsChanged         Whether the settings have been changed.
  //   _saveButton              The save button of the settings box
  //   _cancelButton            The cancel button of the settings box
  //   _deviceMgmtSettings      The device management settings box
  //   _deviceGeneralSettings   The device general settings box
  //   _domEventListenerSet     Set of DOM event listeners.
  //

  _widget: null,
  _device: null,
  _settingsChanged: false,
  _saveButton: null,
  _cancelButton: null,
  _deviceMgmtSettings: null,
  _deviceGeneralSettings: null,
  _domEventListenerSet: null,

  /**
   * \brief Initialize the device summary settings services for the device
   * summary settings widget specified by aWidget.
   *
   * \param aWidget             Device sync tabs widget.
   */

  initialize: function DeviceSummarySettings_initialize(aWidget) {
    // Get the device widget.
    this._widget = aWidget;

    // Initialize object fields.
    this._device = this._widget.device;

    if ((this._device && this._device.defaultLibrary) ||
        !this._getElement("device_general_settings").hidden) {
      this._widget.removeAttribute("disabledTab");
    }
    else {
      this._widget.setAttribute("disabledTab", "true");
    }

    // Listen for device events.
    var deviceEventTarget =
          this._device.QueryInterface(Ci.sbIDeviceEventTarget);
    deviceEventTarget.addEventListener(this);

    this._saveButton = this._getElement("device_settings_button_save");
    this._cancelButton = this._getElement("device_settings_button_cancel");
    this._deviceMgmtSettings = this._getElement("device_management_settings");
    this._deviceGeneralSettings = this._getElement("device_general_settings");

    this._saveButton.setAttribute("disabled", "true");
    this._cancelButton.setAttribute("disabled", "true");

    // Create a DOM event listener set.
    this._domEventListenerSet = new DOMEventListenerSet();

    // Listen for sync settings management change events.
    var self = this;
    var func = function(aEvent) { self._onSettingsChangeEvent(); };
    this._domEventListenerSet.add(document,
                                  "settings-changed",
                                  func,
                                  false,
                                  false);

    this.updateBusyState();
  },

  /**
   * \brief Finalize the device summary settings services.
   */

  finalize: function DeviceSummarySettings_finalize() {
    // Remove DOM event listeners.
    if (this._domEventListenerSet) {
      this._domEventListenerSet.removeAll();
      this._domEventListenerSet = null;
    }

    // Stop listening for device events.
    if (this._device) {
      var deviceEventTarget =
            this._device.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }

    this._widget = null;
    this._device = null;
    this._settingsChanged = false;
    this._saveButton = null;
    this._cancelButton = null;
    this._deviceMgmtSettings = null;
    this._deviceGeneralSettings = null;
  },

  /**
   * \brief Update the busy state of the UI.
   */
  updateBusyState: function DeviceSummarySettings_updateBusyState() {
    let isBusy = this._device && this._device.isBusy;
    if (isBusy || !this._settingsChanged) {
      this._saveButton.setAttribute("disabled", "true");
      this._cancelButton.setAttribute("disabled", "true");
    }
    else {
      this._saveButton.removeAttribute("disabled");
      this._cancelButton.removeAttribute("disabled");
    }

    if (isBusy) {
      this._deviceMgmtSettings.setAttribute("disabled", "true");
      this._deviceGeneralSettings.setAttribute("disabled", "true");
    }
    else {
      this._deviceMgmtSettings.removeAttribute("disabled");
      this._deviceGeneralSettings.removeAttribute("disabled");
    }
  },

  /**
   * \brief Call save on all device management widgets.
   */

  // Currently we save every setting (transcode, and diskmgmt).  Since both
  // of those settings are settings where the default is the same as 'unset',
  // this happens to work nicely (e.g. for transcode it's "automatic", and
  // diskmgmt it's "disabled").  If we ever add a setting here where the
  // default setting is not the same as "explicitly unset" for the device,
  // then we should make sure we only save the settings that aren't in the
  // 'hide' list.
  save: function DeviceSummarySettings_save() {
    this._settingsChanged = false;
    this._saveButton.setAttribute("disabled", "true");
    this._cancelButton.setAttribute("disabled", "true");
    this._deviceMgmtSettings.save();
    if (!this._deviceGeneralSettings.hidden)
      this._deviceGeneralSettings.save();
  },

  /**
   * \brief Call reset all device management widgets.
   */

  reset: function DeviceSummarySettings_reset() {
    this._settingsChanged = false;
    this._saveButton.setAttribute("disabled", "true");
    this._cancelButton.setAttribute("disabled", "true");
    this._deviceMgmtSettings.reset();
    if (!this._deviceGeneralSettings.hidden)
      this._deviceGeneralSettings.reset();
  },

  //----------------------------------------------------------------------------  //
  // Device sync sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------
  /**
   * \brief Handle the device event specified by aEvent.
   *
   * \param aEvent              Device event.
   */

  onDeviceEvent: function DeviceSummarySettings_onDeviceEvent(aEvent) {
    // Dispatch processing of the event.
    switch(aEvent.type)
    {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        this.updateBusyState();
        break;

      default :
        break;
    }
  },

  //----------------------------------------------------------------------------
  //
  // Device summary settings XUL services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handles settings change event
   */
  _onSettingsChangeEvent:
    function DeviceSummarySettings__onSettingsChangeEvent(aEvent) {
    this._settingsChanged = true;
    this._saveButton.removeAttribute("disabled");
    this._cancelButton.removeAttribute("disabled");
  },

  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "sbid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function DeviceSummarySettings__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  }
};
