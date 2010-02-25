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
 * \file  deviceGeneralSettings.js
 * \brief Javascript source for the device general settings widget services.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device general settings widget services.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device general settings widget services defs.
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
// Device general settings widget services.
//
//------------------------------------------------------------------------------

/**
 * Construct a device general settings widget services object for the widget
 * specified by aWidget.
 *
 * \param aWidget               Device general settings widget.
 */

function deviceGeneralSettingsSvc(aWidget) {
  this._widget = aWidget;
}

// Define the object.
deviceGeneralSettingsSvc.prototype = {
  // Set the constructor.
  constructor: deviceGeneralSettingsSvc,

  //
  // Internal widget services fields.
  //
  //   _widget                  Device general settings widget.
  //   _device                  Device bound to widget.
  //   _autoLaunchSupported     True if device supports auto-launch.
  //   _autoFirmwareCheckSupported
  //                            True if device supports auto-firmware checking.
  //

  _widget: null,
  _device: null,
  _autoLaunchSupported: false,
  _autoFirmwareCheckSupported: false,


  //----------------------------------------------------------------------------
  //
  // Widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Initialize the widget services.
   */

  initialize: function deviceGeneralSettingsSvc_initialize() {
    // Get the bound device.  Do nothing more if no device bound.
    this._device = this._widget.device;
    if (!this._device)
      return;

    // Determine which settings are supported.
    this._getSupportedSettings();
    
    // Hide device settings and do nothing more if no settings are supported.
    if (!this._autoLaunchSupported && !this._autoFirmwareCheckSupported) {
      this._widget.hidden = true;
      return;
    }

    // Initialize the UI.
    this._initializeUI();
  },


  /**
   * Finalize the widget services.
   */

  finalize: function deviceGeneralSettingsSvc_finalize() {
    // Clear object fields.
    this._device = null;
  },


  /**
   * Save the UI settings to the device.
   */

  save: function deviceGeneralSettingsSvc_save() {
    // Set the device settings to the current UI settings.
    this._setDeviceSettingsFromUISettings();
  },


  /**
   * Reset the UI settings to the current device settings.
   */

  reset: function deviceGeneralSettingsSvc_reset() {
    // Reset the UI settings from the device settings.
    this._setUISettingsFromDeviceSettings();
  },


  //----------------------------------------------------------------------------
  //
  // Internal widget services.
  //
  //----------------------------------------------------------------------------

  /**
   * Determine which settings are supported.
   */

  _getSupportedSettings:
    function deviceGeneralSettingsSvc__getSupportedSettings() {
    // Determine whether the device supports application auto-launch.
    var propertyName = "http://songbirdnest.com/device/1.0#supportsAutoLaunch";
    if (this._device.properties.properties.hasKey(propertyName)) {
      this._autoLaunchSupported =
        this._device.properties.properties.getProperty(propertyName);
    }

    // Determine whether the device supports auto-firmware update checks.
    var updater = Cc['@songbirdnest.com/Songbird/Device/Firmware/Updater;1']
                    .getService(Ci.sbIDeviceFirmwareUpdater);
    this._autoFirmwareCheckSupported = updater.hasHandler(this._device, 0, 0);
  },


  /**
   * Initialize the widget UI.
   */

  _initializeUI: function deviceGeneralSettingsSvc__initializeUI() {
    // Initialize auto-launch checkbox.
    var autoLaunchCheckbox = this._getElement("auto_launch_checkbox");
    autoLaunchCheckbox.hidden = !this._autoLaunchSupported;

    // Initialize auto-firmware check checkbox.
    var autoFirmwareCheckCheckbox =
          this._getElement("auto_firmware_check_checkbox");
    autoFirmwareCheckCheckbox.hidden = !this._autoFirmwareCheckSupported;

    // Set UI settings from device settings.
    this._setUISettingsFromDeviceSettings();
  },


  /**
   * Set the UI setting for the UI element with the id specified by aSettingID to
   * match the current device settings.  If aSettingID is not specified, set all
   * UI settings.
   *
   * \param aSettingID          UI setting element ID or null for all settings.
   */

  _setUISettingsFromDeviceSettings: function
    deviceGeneralSettingsSvc__setUISettingsFromDeviceSettings(aSettingID) {
    // Determine whether all settings are being set.
    var allSettings = (aSettingID ? false : true);

    // Set the auto-launch setting.
    if (this._autoLaunchSupported &&
        (allSettings || (aSettingID == "auto_launch_checkbox"))) {
      // Get the device auto-launch setting.  Use a default if no setting set.
      var autoLaunchEnabled = this._device.getPreference("auto_launch_enabled");
      if (autoLaunchEnabled == null)
        autoLaunchEnabled = true;

      // Set the UI setting.
      var autoLaunchCheckbox = this._getElement("auto_launch_checkbox");
      autoLaunchCheckbox.checked = autoLaunchEnabled;
    }

    // Set the auto-firmware check setting.
    if (this._autoFirmwareCheckSupported &&
        (allSettings || (aSettingID == "auto_firmware_check_checkbox"))) {
      // Get the device auto-firmware check setting.  Use a default if no
      // setting set.  (no pref = null, defaults to true in the case)
      var deviceAutoFirmwarePref = this._device.getPreference("firmware.update.enabled");
      
      var autoFirmwareCheckEnabled = 
        (deviceAutoFirmwarePref == null) ? true : deviceAutoFirmwarePref;

      // Also set the device pref if it was not already present.        
      if(deviceAutoFirmwarePref == null) {
        this._device.setPreference("firmware.update.enabled",
                                   autoFirmwareCheckEnabled);        
      }

      // Set the UI setting.
      var autoFirmwareCheckCheckbox =
            this._getElement("auto_firmware_check_checkbox");
      autoFirmwareCheckCheckbox.checked = autoFirmwareCheckEnabled;
    }
  },


  /**
   * Set the device settings to match the UI setting for the UI element with the
   * id specified by aSettingID.  If aSettingID is not specified, set the device
   * settings to match all UI settings.
   *
   * \param aSettingID          UI setting element ID or null for all settings.
   */

  _setDeviceSettingsFromUISettings: function
    deviceGeneralSettingsSvc__setDeviceSettingsFromUISettings(aSettingID) {
    // Determine whether all settings are being set.
    var allSettings = (aSettingID ? false : true);

    // Set the auto-launch setting.
    if (this._autoLaunchSupported &&
        (allSettings || (aSettingID == "auto_launch_checkbox"))) {
      // Get the UI auto-launch setting.
      var autoLaunchCheckbox = this._getElement("auto_launch_checkbox");
      var autoLaunchEnabled = autoLaunchCheckbox.checked;

      // Set the device setting.
      this._device.setPreference("auto_launch_enabled", autoLaunchEnabled);
    }

    // Set the auto-firmware check setting.
    if (this._autoFirmwareCheckSupported &&
        (allSettings || (aSettingID == "auto_firmware_check_checkbox"))) {
      // Get the UI auto-firmware check setting.
      var autoFirmwareCheckCheckbox =
            this._getElement("auto_firmware_check_checkbox");
      var autoFirmwareCheckEnabled = autoFirmwareCheckCheckbox.checked;

      // Set the device setting.
      this._device.setPreference("firmware.update.enabled",
                                 autoFirmwareCheckEnabled);
    }
  },


  /**
   * \brief Return the XUL element with the ID specified by aElementID.  Use the
   *        element "anonid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element.
   */

  _getElement: function deviceGeneralSettingsSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  }
}

