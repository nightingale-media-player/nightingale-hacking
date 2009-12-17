/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");

if (typeof(XUL_NS) == "undefined")
  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var DeviceSummarySettings = {
  //
  // Device Sync Tabs object fields.
  //
  //   _widget                  Device sync tabs widget.
  //   _deviceID                Device ID.
  //  _deviceManagementWidgets  array of widgets for device management.
  //  _mediaManagementWidgets   array of widgets for media management.
  //

  _widget: null,
  _deviceID: null,
  _deviceManagementWidgets: [],
  _mediaManagementWidgets: [],

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
    this._deviceID = this._widget.deviceID;
    this._device = this._widget.device;

    this.refreshDeviceSettings();
  },

  /**
   * \brief Finalize the device summary settings services.
   */

  finalize: function DeviceSummarySettings_finalize() {
    this._device = null;
    this._deviceID = null;
    this._widget = null;
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
    for each (let widget in this._deviceManagementWidgets) {
      widget.save();
    }
  },

  /**
   * \brief Call reset all device management widgets.
   */

  reset: function DeviceSummarySettings_reset() {
    for each (let widget in this._deviceManagementWidgets) {
      widget.reset();
    }
  },

  /**
   * \brief Refresh all the device widgets settings.
   */

  refreshDeviceSettings:
    function DeviceSummarySettings_refreshDeviceSettings() {
    // Add device general settings widget if it hasn't been already.
    var deviceGeneralSettings = this._getElement("device_general_settings");
    if (this._deviceManagementWidgets.indexOf(deviceGeneralSettings) < 0)
      this._deviceManagementWidgets.push(deviceGeneralSettings);

    // Refresh the media management.
    this.refreshMediaManagement();
  },

  /**
   * \brief Refresh all the media management widgets.
   */

  refreshMediaManagement:
    function DeviceSummarySettings_refreshMediaManagement() {
    // determine what settings to hide so we can pass it onto the
    // sb-device-management binding
    var settingsToHide = this._widget.getAttribute("hideSettings");

    var seenLibraries = {};
    for (let idx = this._mediaManagementWidgets.length - 1; idx >= 0; --idx) {
      let widget = this._mediaManagementWidgets[idx];
      if (widget.deviceID == this._deviceID) {
        seenLibraries[widget.libraryID] = true;
        continue;
      }
      let deviceManagementWidgetsIndex =
            this._deviceManagementWidgets.indexOf(widget);
      if (deviceManagementWidgetsIndex >= 0)
        this._deviceManagementWidgets.splice(deviceManagementWidgetsIndex, 1);
      this._mediaManagementWidgets.splice(idx, 1);
      ++idx;
    }

    // Get the device content.
    var content = this._device.content;

    // add new bindings as needed
    var container = this._getElement("device_management_settings_content");
    for each (let library in ArrayConverter.JSArray(content.libraries)) {
      library.QueryInterface(Ci.sbIDeviceLibrary);
      if (library.guid in seenLibraries) {
        continue;
      }
      var widget = document.createElement("sb-device-management");
      widget.setAttribute("device-id", this._deviceID);
      widget.setAttribute("library-id", library.guid);
      widget.setAttribute("hide", settingsToHide);
      container.appendChild(widget);

      widget.device = this._device;

      this._deviceManagementWidgets.push(widget);
      this._mediaManagementWidgets.push(widget);
    }
  },

  //----------------------------------------------------------------------------
  //
  // Device summary settings XUL services.
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

  _getElement: function DeviceSummarySettings__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  }
};
