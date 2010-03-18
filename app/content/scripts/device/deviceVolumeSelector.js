/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * \file  deviceVolumeSelector.js
 * \brief Javascript source for the device volume selector widget.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device volume selector widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device volume selector imported services.
//
//------------------------------------------------------------------------------

// Component manager defs.
var Cc = Components.classes;
var Ci = Components.interfaces;
var Cr = Components.results;
var Cu = Components.utils;

// Songbird imports.
Cu.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// Device volume selector services.
//
//------------------------------------------------------------------------------

var deviceVolumeSelectorSvc = {
  //
  // Device volume selector object fields.
  //
  //   _widget                  Device volume selector widget.
  //   _device                  Widget device.
  //   _volumeSelectorDeck      Volume selector deck element.
  //

  _widget: null,
  _device: null,
  _volumeSelectorDeck: null,


  /**
   * Initialize the device volume selector services for the device volume
   * selector widget specified by aWidget.
   *
   * \param aWidget             Device volume selector widget.
   */

  initialize: function deviceVolumeSelectorSvc_initialize(aWidget) {
    // Get the device volume selector widget and widget device
    this._widget = aWidget;
    this._device = this._widget.device;

    // Get the volume selector deck element.
    this._volumeSelectorDeck = this._getElement("volume_selector_deck");

    // Listen to device events.
    var deviceEventTarget =
          this._device.QueryInterface(Ci.sbIDeviceEventTarget);
    deviceEventTarget.addEventListener(this);

    // Update UI.
    this._update();
  },


  /**
   * Finalize the device volume selector services.
   */

  finalize: function deviceVolumeSelectorSvc_finalize() {
    // Stop listening to device events.
    if (this._device) {
      var deviceEventTarget =
            this._device.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }

    // Clear object fields.
    this._widget = null;
    this._device = null;
    this._volumeSelectorDeck = null;
  },


  /**
   * Handle a volume change event.
   */

  onVolumeChange: function deviceVolumeSelectorSvc_onVolumeChange() {
    // Get the selected volume library GUID.
    var volumeMenuList = this._getElement("volume_menulist");
    var libraryGUID = volumeMenuList.value;

    // Set the device default library to the selected volume library.
    var content = this._device.content;
    var libraries = content.libraries;
    libraryCount = libraries.length;
    for (var i = 0; i < libraryCount; i++) {
      // Get the next library.
      var library = libraries.queryElementAt(i, Ci.sbIDeviceLibrary);

      // If library is the selected library, set it as the default.
      if (library.guid == libraryGUID) {
        this._device.defaultLibrary = library;
        break;
      }
    }
  },


  //----------------------------------------------------------------------------
  //
  // Device volume selector sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the device event specified by aEvent.
   *
   * \param aEvent              Device event.
   */

  onDeviceEvent: function deviceVolumeSelectorSvc_onDeviceEvent(aEvent) {
    // Dispatch processing of event.
    switch (aEvent.type)
    {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_LIBRARY_ADDED :
      case Ci.sbIDeviceEvent.EVENT_DEVICE_LIBRARY_REMOVED :
      case Ci.sbIDeviceEvent.EVENT_DEVICE_DEFAULT_LIBRARY_CHANGED :
        this._update();
        break;

      default :
        break;
    }
  },


  //----------------------------------------------------------------------------
  //
  // Internal volume selector sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the widget UI.
   */

  _update: function deviceVolumeSelectorSvc__update() {
    // Dispatch update depending upon how many device volumes are available.
    var content = this._device.content;
    var libraries = content.libraries;
    if (libraries.length == 0)
      this._updateNoVolumes();
    else if (libraries.length == 1)
      this._updateSingleVolume();
    else
      this._updateMultipleVolumes();
  },


  /**
   * Update the UI for a device with no volumes.
   */

  _updateNoVolumes: function deviceVolumeSelectorSvc__updateNoVolumes() {
    // Set the volume selector deck to display a single volume label element.
    var volumeLabel = this._getElement("volume_label");
    this._volumeSelectorDeck.selectedPanel = volumeLabel;

    // Indicate that no device volumes are available.
    volumeLabel.value = SBString("device.volume_selector.label.no_volumes");
  },


  /**
   * Update the UI for a device with a single volume.
   */

  _updateSingleVolume: function deviceVolumeSelectorSvc__updateSingleVolume() {
    // Set the volume selector deck to display a single volume label element.
    var volumeLabel = this._getElement("volume_label");
    this._volumeSelectorDeck.selectedPanel = volumeLabel;

    // Get the device volume library.
    var content = this._device.content;
    var libraries = content.libraries;
    var library = libraries.queryElementAt(0, Ci.sbIDeviceLibrary);

    // Set the volume label to the volume library name.
    volumeLabel.value = library.name;
  },


  /**
   * Update the UI for a device with multiple volumes.
   */

  _updateMultipleVolumes:
    function deviceVolumeSelectorSvc__updateMultipleVolumes() {
    // Set the volume selector deck to display a volume selector menu list.
    var volumeMenuList = this._getElement("volume_menulist");
    this._volumeSelectorDeck.selectedPanel = volumeMenuList;

    // Empty volume menu list.
    volumeMenuList.removeAllItems();

    // Fill volume menu list with all device volumes.
    var content = this._device.content;
    var libraries = content.libraries;
    libraryCount = libraries.length;
    for (var i = 0; i < libraryCount; i++) {
      // Add the next device volume library to the menu list.
      var library = libraries.queryElementAt(i, Ci.sbIDeviceLibrary);
      volumeMenuList.appendItem(library.name, library.guid);
    }

    // Set the selected volume to the device default library volume.
    var defaultLibrary = this._device.defaultLibrary;
    if (defaultLibrary)
      volumeMenuList.value = defaultLibrary.guid;
    else
      volumeMenuList.selectedIndex = 0;
  },


  //----------------------------------------------------------------------------
  //
  // Device volume selector XUL services.
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

  _getElement: function deviceVolumeSelectorSvc__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  }
};


