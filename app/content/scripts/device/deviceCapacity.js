/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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
  //   _device                    sbIDevice object.
  //   _deviceProperties          Cache properties to avoid costly garbage
  //   _updateInterval            Timing interval used for updating UI.
  //   _capTable                  Table of capacity values.
  //

  _cfg: DCWCfg,
  _widget: null,
  _device: null,
  _deviceProperties : null,
  _updateInterval: null,
  _capTable: null,
  _panel: null,

  _panelElements: [ "device-cap-bar-music-box",
                    "device-cap-bar-video-box",
                    "device-cap-bar-image-box",
                    "device-capacity-legend-music",
                    "device-capacity-legend-video",
                    "device-capacity-legend-image"
                  ],

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
    this._device = this._widget.device;
    this._deviceProperties = this._device.properties;
    this._capTable = {};

    // Shortcuts to elements
    this._capacityBar = this._getElement("device-capacity-bar");
    this._capacityLegend = this._getElement("device-cap-legend-box");

    // Get the list of elements to hide.
    var hideListAttr = this._widget.getAttribute("hidelist");
    if (hideListAttr) {
      var hideList = this._widget.getAttribute("hidelist").split(",");

      // Hide the specified elements.
      for (var i = 0; i < hideList.length; i++) {
        var element = this._getHideElement(hideList[i]);
        element.hidden = true;
      }
    }

    // Hook up the panel to the right boxes
    var panelId = this._widget.getAttribute("panel");
    this._panel = document.getElementById(this._widget.getAttribute("panel"));
    for each (var elId in this._panelElements) {
      var el = this._getElement(elId);
      if (el)
        el.addEventListener("click", this._openCapacityPanel, false);
    }

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

    try {
      for each (var elId in this._panelElements) {
        var el = this._getElement(elId);
        if (el)
          el.removeEventListener("click", this._openCapacityPanel, false);
      }
    } catch (e) {
      dump("Failed to remove event listener: " + e + "\n");
    }

    // Clear object fields.
    this._widget = null;
  },

  /**
   * \brief Open the capacity panel
   *
   * \param aEvent             The event triggering this popup
   */
  _openCapacityPanel: function DIW__openCapacityPanel(aEvent) {
    DCW._panel.openPopup(aEvent.target);
  },

  /**
   * \brief Return the hide element with the hide ID specified by aHideID.
   *
   * \param aHideID             Element hide ID.
   *
   * \return Hide element.
   */

  _getHideElement: function DIW__getHideElement(aHideID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "hideid",
                                                   aHideID);
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
    var needsUpdate = false;

    for (var cap in capTable) {
      if (capTable[cap] != this._capTable[cap]) {
        needsUpdate = true;
        break;
      }
    }
    if (!needsUpdate)
      return;

    if (!this._device.defaultLibrary)
      this._capacityBar.setAttribute("disabled", "true");
    else
      this._capacityBar.removeAttribute("disabled");

    // Update the capacity.
    this._capTable = capTable;
    if (!this._capacityBar.hidden)
      this._updateCapBars();
    if (!this._capacityLegend.hidden)
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
      if (value) {
        storageConverter =
          Cc["@songbirdnest.com/Songbird/Properties/UnitConverter/Storage;1"]
            .createInstance(Ci.sbIPropertyUnitConverter);
        child.setAttribute("value", storageConverter.autoFormat(value, -1, 1));
        child.hidden = false;
      }
      else {
        child.hidden = true;
      }
    }
  },


  //----------------------------------------------------------------------------
  //
  // Device capacity device services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Get a device property if available
   *
   * \param aPropertyName name of the property to get
   * \param aDefault default to return if property not found
   * 
   * \return string value of the property or the default if not found.
   * 
   * \sa sbStandardDeviceProperties.h
   */
  
  _getDeviceProperty: function DIW__getDeviceProperty(aPropertyName, aDefault) {
    try {
      if (this._deviceProperties.properties.hasKey(aPropertyName))
        return this._deviceProperties.properties.getPropertyAsAString(aPropertyName);
    } catch (ex) { }
    return aDefault;
  },

  /**
   * \brief Return a table of device capacities.
   *
   * \return Table object of device capacities.
   *           total            Total device capacity.
   *           free             Free device capacity.
   *           music            Device capacity used for music.
   *           video            Device capacity used for video.
   *           other            Device capacity used for all other storage.
   */

  _getDeviceCapacity: function DCW__getDeviceCapacity() {
    // Get the storage statistics from the device.
    var freeSpace = parseInt(this.
        _getDeviceProperty("http://songbirdnest.com/device/1.0#freeSpace", 0));
    var musicSpace = parseInt(this.
        _getDeviceProperty("http://songbirdnest.com/device/1.0#musicUsedSpace", 0));
    var videoSpace = parseInt(this.
        _getDeviceProperty("http://songbirdnest.com/device/1.0#videoUsedSpace", 0));
    var imageSpace = parseInt(this.
        _getDeviceProperty("http://songbirdnest.com/device/1.0#imageUsedSpace", 0));
    var usedSpace = parseInt(this.
        _getDeviceProperty("http://songbirdnest.com/device/1.0#totalUsedSpace", 0));
    var totalSpace = usedSpace + freeSpace;
    var otherSpace = usedSpace - musicSpace - videoSpace - imageSpace;

    // Set up the device capacity table.
    var capTable = {};
    capTable.total = Math.max(0, totalSpace);
    capTable.free = Math.max(0, freeSpace);
    capTable.music = Math.max(0, musicSpace);
    capTable.video = Math.max(0, videoSpace);
    capTable.image = Math.max(0, imageSpace);
    capTable.other = Math.max(0, otherSpace);
    return capTable;
  }
};
