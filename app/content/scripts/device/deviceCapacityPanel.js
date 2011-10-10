/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

/** 
* \file  deviceCapacityPanel.js
* \brief Javascript source for the device capacity panel widget.
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

// Nightingale imports.
Cu.import("resource://app/jsmodules/sbTimeFormatter.jsm");


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
  //   _deviceLibrary           Device library we are working with.
  //   _panel                     The XUL panel itself
  //   _label                     The label within the panel
  //

  _widget: null,
  _devLib: null,
  _panel: null,
  _label: null,


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
    this._deviceLibrary = this._widget.devLib;

    this._panel = this._getElement("capacity-panel");
    this._label = this._getElement("panel-label");
  },


  /**
   * \brief Finalize the device capacity services.
   */

  finalize: function DCW_finalize() {
    // Clear object fields.
    this._widget = null;
    this._deviceLibrary = null;
  },

  /**
   * \brief Opens the panel popup
   */

  openPopup: function DCW_openPopup(anchor) {
    var type = anchor.getAttribute("type");
    DCW._update(type);
    DCW._panel.openPopup(anchor, "after_start");
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

  _update: function DCW__update(type) {
    // Sanity check
    if (type != "music" && type != "video")
      return;

    var playTimeProperty = "http://getnightingale.com/device/1.0#" +
                           type + "TotalPlayTime";
    var itemCountProperty = "http://getnightingale.com/device/1.0#" +
                           type + "ItemCount";
    var playTime = parseInt(this._getDevLibProperty(playTimeProperty, -1));
    var itemCount = parseInt(this._getDevLibProperty(itemCountProperty, -1));
    if (playTime < 0 || itemCount < 0) {
      this._label.value = SBString("device.capacity.panel." + type + ".error");
    } else {
      var key = "device.capacity.panel." + type;
      playTime = TimeFormatter.formatSingle(playTime/1000000);

      this._label.value = SBFormattedCountString(key, itemCount,
                                                 [ playTime, itemCount ], "");
    }

  },



  //----------------------------------------------------------------------------
  //
  // Device capacity device services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Get a device library property if available
   *
   * \param aPropertyName name of the property to get
   * \param aDefault default to return if property not found
   *
   * \return string value of the property or the default if not found.
   *
   * \sa sbStandardDeviceProperties.h
   */

  _getDevLibProperty: function DCW__getDevLibProperty(aPropertyName, aDefault) {
    try {
      if (this._deviceLibrary)
        return this._deviceLibrary.getProperty(aPropertyName);
    } catch (ex) { }
    return aDefault;
  },
};
