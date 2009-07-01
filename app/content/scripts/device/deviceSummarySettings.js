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
* \file  deviceSummarySettings.js
* \brief Javascript source for initializing the device summary page, settings tab
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device sync widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device sync defs.
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

var DeviceSettingsPane = {
  _mediaManagementWidgets: [],

  save: function DeviceSettingsPane_save() {
    for each (let widget in this._mediaManagementWidgets) {
      widget.save();
    }
  },

  reset: function DeviceSettingsPane_reset() {
    for each (let widget in this._mediaManagementWidgets) {
      widget.reset();
    }
  },

  refreshMediaManagement: function DeviceSettingsPane_refreshMediaManagement(aEvent) {
    var deviceID = document.getElementById("deviceIDBroadcaster")
                           .getAttribute("device-id");
    var seenLibraries = {};
    for (let idx = this._mediaManagementWidgets.length - 1; idx >= 0; --idx) {
      let widget = this._mediaManagementWidgets[idx];
      if (widget.deviceID == deviceID) {
        seenLibraries[widget.libraryID] = true;
        continue;
      }
      this._mediaManagementWidgets.splice(idx, 1);
      ++idx;
    }

    // Get the device.
    var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceManager2);
    var device = deviceManager.getDevice(Components.ID(deviceID));
    var content = device.content;

    // add new bindings as needed
    var container = document.getElementById("device_management_settings_content");
    for each (let library in ArrayConverter.JSArray(content.libraries)) {
      library.QueryInterface(Ci.sbIDeviceLibrary);
      if (library.guid in seenLibraries) {
        continue;
      }
      var widget = document.createElement("sb-device-management");
      widget.setAttribute("device-id", deviceID);
      widget.setAttribute("library-id", library.guid);
      container.appendChild(widget);

      widget.device = device;

      this._mediaManagementWidgets.push(widget);
    }
  },
};
