/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
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

// Nightingale imports.
Cu.import("resource://app/jsmodules/DOMUtils.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// Device volume selector defs.
//
//------------------------------------------------------------------------------

// XUL XML namespace.
var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";


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
    // Get the device volume selector widget and widget device.
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


//------------------------------------------------------------------------------
//
// Device volume menuitems services.
//
//------------------------------------------------------------------------------

var deviceVolumeMenuItemsSvc = {
  //
  // Device volume menuitems object fields.
  //
  //   _widget                  Device volume menuitems widget.
  //   _device                  Widget device.
  //   _addedElementList        List of UI elements added by this widget.
  //   _addedElementListenerSet Set of listeners added to UI elements.
  //

  _widget: null,
  _device: null,
  _addedElementList: null,
  _addedElementListenerSet: null,


  /**
   * Initialize the device volume menuitems services for the device volume
   * menuitems widget specified by aWidget.
   *
   * \param aWidget             Device volume menuitems widget.
   */

  initialize: function deviceVolumeMenuItemsSvc_initialize(aWidget) {
    // Do nothing if widget is not yet bound to a device.
    if (!aWidget.device)
      return;

    // Get the device volume menuitems widget and widget device.
    this._widget = aWidget;
    this._device = this._widget.device;

    // Listen to device events.
    var deviceEventTarget =
          this._device.QueryInterface(Ci.sbIDeviceEventTarget);
    deviceEventTarget.addEventListener(this);

    // Update UI.
    this._update();
  },


  /**
   * Finalize the device volume menuitems services.
   */

  finalize: function deviceVolumeMenuItemsSvc_finalize() {
    // Stop listening to device events.
    if (this._device) {
      var deviceEventTarget =
            this._device.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }

    // Remove all added elements.
    this._removeAddedElements();

    // Clear object fields.
    this._widget = null;
    this._device = null;
  },


  //----------------------------------------------------------------------------
  //
  // Device volume menuitems sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * Handle the device event specified by aEvent.
   *
   * \param aEvent              Device event.
   */

  onDeviceEvent: function deviceVolumeMenuItemsSvc_onDeviceEvent(aEvent) {
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
  // Internal volume menuitems sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * Update the widget UI.
   */

  _update: function deviceVolumeMenuItemsSvc__update() {
    // Remove all added elements.
    this._removeAddedElements();

    // Get the minimum number of volumes to add any UI elements.
    var minVolumeCount = 1;
    if (this._widget.hasAttribute("minvolumes"))
      minVolumeCount = parseInt(this._widget.getAttribute("minvolumes"));

    // Get the list of device volume libraries.  Don't add any UI elements if
    // not enough volumes are present.
    var libraries = this._device.content.libraries;
    var libraryCount = libraries.length;
    if (libraryCount < minVolumeCount)
      return;

    // Check if the default volume menuitem should be checked.
    var checkDefault = this._widget.getAttribute("checkdefault") == "true";

    // Create a list of added elements.
    this._addedElementList = [];
    this._addedElementListenerSet = new DOMEventListenerSet();

    // If specified, add a menu separator before menuitems.
    if (this._widget.getAttribute("addseparatorbefore") == "true") {
      var separator = document.createElementNS(XUL_NS, "menuseparator");
      this._widget.parentNode.insertBefore(separator, this._widget);
      this._addedElementList.push(separator);
    }

    // Add a menu item for each device volume.
    for (var i = 0; i < libraryCount; i++) {
      // Get the next volume library.
      library = libraries.queryElementAt(i, Ci.sbIDeviceLibrary);

      // Create a volume menuitem.
      var menuItem = document.createElementNS(XUL_NS, "menuitem");
      menuItem.setAttribute("label", library.name);
      menuItem.setAttribute("value", library.guid);

      // If specified, check the default volume menuitem.
      if (checkDefault && (library.guid == this._device.defaultLibrary.guid))
        menuItem.setAttribute("checked", "true");

      // Add a command listener to the menuitem.
      var _this = this;
      var func = function(aEvent) { return _this._onVolumeChange(aEvent); };
      this._addedElementListenerSet.add(menuItem, "command", func, false);

      // Add the volume menuitem.
      this._widget.parentNode.insertBefore(menuItem, this._widget);
      this._addedElementList.push(menuItem);
    }

    // If specified, add a menu separator after menuitems.
    if (this._widget.getAttribute("addseparatorafter") == "true") {
      var separator = document.createElementNS(XUL_NS, "menuseparator");
      this._widget.parentNode.insertBefore(separator, this._widget);
      this._addedElementList.push(separator);
    }
  },


  /**
   * Remove all UI elements added by this widget.
   */

  _removeAddedElements:
    function deviceVolumeMenuItemsSvc__removeAddedElements() {
    // Remove all added element listeners.
    if (this._AddedElementListenerSet) {
      this._addedElementListenerSet.removeAll();
      this._addedElementListenerSet = null;
    }

    // Remove all added elements.
    if (this._addedElementList) {
      for (var i = 0; i < this._addedElementList.length; i++) {
        var element = this._addedElementList[i];
        if (element.parentNode)
          element.parentNode.removeChild(element);
      }
      this._addedElementList = null;
    }
  },


  /**
   * Handle the volume change event specified by aEvent.
   *
   * \param aEvent              Volume change event.
   */

  _onVolumeChange: function deviceVolumeMenuItemsSvc__onVolumeChange(aEvent) {
    // Get the selected volume library GUID.
    var libraryGUID = aEvent.target.value;

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
  }
};

