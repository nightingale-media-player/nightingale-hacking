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
* \file  deviceCapacity.js
* \brief Javascript source for the device capacity widget.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device capacity widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device capacity imported services.
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

// Songbird imports.
Cu.import("resource://app/jsmodules/sbStorageFormatter.jsm");


//------------------------------------------------------------------------------
//
// Device capacity configuration.
//
//------------------------------------------------------------------------------

/*
 * updatePeriod                 Time period in milliseconds of UI updates.
 */

var DCWCfg = {
  updatePeriod: 500
};


//------------------------------------------------------------------------------
//
// Device capacity services.
//
//------------------------------------------------------------------------------

var DCW = {
  //
  // Device capacity object.
  //
  //   _cfg                       Configuration.
  //   _widget                    Device capacity widget.
  //   _deviceID                  Device ID.
  //   _device                    sbIDevice object.
  //   _updateInterval            Timing interval used for updating UI.
  //   _capTable                  Table of capacity values.
  //

  _cfg: DCWCfg,
  _widget: null,
  _deviceID: null,
  _device: null,
  _updateInterval: null,
  _capTable: null,


  /**
   * \brief Initialize the device capacity services for the device capacity
   *        widget specified by aWidget.
   *
   * \param aWidget             Device capacity widget.
   */

  initialize: function DCW__initialize(aWidget) {
    // Get the device capacity widget.
    this._widget = aWidget;

    // Initialize object fields.
    this._deviceID = this._widget.deviceID;
    this._capTable = {};

    // Get the device object.
    this._device = this._getDevice(this._deviceID);

    // Update the UI.
    this._update();

    // Start the update timing interval.
    var _this = this;
    var func = function() { _this._update(); }
    this._updateInterval = window.setInterval(func, this._cfg.updatePeriod);
  },


  /**
   * \brief Finalize the device capacity services.
   */

  finalize: function DCW_finalize() {
    // Clear the update timing interval.
    if (this._updateInterval) {
      window.clearInterval(this._updateInterval);
      this._updateInterval = null;
    }

    // Clear object fields.
    this._widget = null;
    this._deviceID = null;
  },


  //----------------------------------------------------------------------------
  //
  // Device capacity XUL services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Return the XUL element with the ID specified by aElementID.
   *        This function uses the element "sbid" attribute as the ID.
   *
   * \param aElementID          ID of element to get.
   *
   * \return Element with the specified ID.
   */

  _getElement: function DCW__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  },


  //----------------------------------------------------------------------------
  //
  // Internal device capacity services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Update the capacity UI.  Read the device storage statistics, and if
   *        any have changed, update the capacity UI to reflect the new values.
   */

  _update: function DCW__update() {
    // Get the capacity table.
    var capTable = this._getDeviceCapacity();

    // Check if update is needed.  Do nothing more if not.
    var needsUpdate;
    for (var cap in capTable) {
      if (capTable[cap] != this._capTable[cap]) {
        needsUpdate = true;
        break;
      }
    }
    if (!needsUpdate)
      return;

    // Update the capacity.
    this._capTable = capTable;
    this._updateCapBars();
    this._updateCapLegends();
  },


  /**
   * \brief Update the capacity bar UI elements to reflect the current capacity
   *        values.
   */

  _updateCapBars: function DCW__updateCapBars() {
    // Get the total capacity.
    var capTotal = this._capTable["total"];

    // Update each capacity bar.
    var barBox = this._getElement("device-cap-bar-box");
    var childList = barBox.childNodes;
    for (var i = 0; i < childList.length; i++) {
      // Get the next capacity bar.
      var child = childList[i];
      var barType = child.getAttribute("type");
      if (!barType)
        continue;

      // Get the capacity bar value.
      var value = this._capTable[barType];
      if (!value)
        value = 0;
      value = Math.round((value / capTotal) * 10000);

      // Update the capacity bar.
      child.setAttribute("flex", value);
    }
  },


  /**
   * \brief Update the capacity legend UI elements to reflect the current
   *        capacity values.
   */

  _updateCapLegends: function DCW__updateCapLegends() {
    // Update each capacity legend.
    var legendBox = this._getElement("device-cap-legend-box");
    var childList = legendBox.childNodes;
    for (var i = 0; i < childList.length; i++) {
      // Get the next capacity legend.
      var child = childList[i];
      var legendType = child.getAttribute("type");
      if (!legendType)
        continue;

      // Get the capacity legend value.
      var value = this._capTable[legendType];
      if (!value)
        value = 0;

      // Update the capacity legend.
      child.setAttribute("value", StorageFormatter.format(value));
    }
  },


  //----------------------------------------------------------------------------
  //
  // Device capacity device services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Return a table of device capacities.
   *
   * \return Table object of device capacities.
   *           total            Total device capacity.
   *           free             Free device capacity.
   *           music            Device capacity used for music.
   *           other            Device capacity used for all other storage.
   */

  //XXXeps fabricate device capacities.
  _totalSpace: 8 * 1024 * 1024 * 1024,
  _otherSpace: 100 * 1024 * 1024,
  _freeSpace: 8 * 1024 * 1024 * 1024 - 100 * 1024 * 1024,
  _musicSpace: 0,
  _usedSpace: 100 * 1024 * 1024,

  _getDeviceCapacity: function DCW__getDeviceCapacity() {
    // Get the storage statistics from the device.
    //XXXeps fabricate something for now.
    var freeSpace = this._freeSpace;
    var musicSpace = this._musicSpace;
    var usedSpace = this._usedSpace;
    var totalSpace = usedSpace + freeSpace;
    var otherSpace = usedSpace - musicSpace;
    this._usedSpace += 10 * 1024 * 1024;
    this._freeSpace -= 10 * 1024 * 1024;
    if (this._freeSpace < 0)
      this._freeSpace = 0;
    this._musicSpace = this._totalSpace - this._freeSpace - this._otherSpace;
    if (this._musicSpace < 0)
      this._musicSpace = 0;

    // Set up the device capacity table.
    var capTable = {};
    capTable.total = totalSpace;
    capTable.free = freeSpace;
    capTable.music = musicSpace;
    capTable.other = otherSpace;

    return capTable;
  },


  /**
   * \brief Get the device object for the device ID specified by aDeviceID.
   *
   * \param aDeviceID       Device identifier.
   *
   * \return sbIDevice device object.
   */

  _getDevice: function DCW__getDevice(aDeviceID) {
    // Just return the device ID for now.
    return aDeviceID;
  }
};


