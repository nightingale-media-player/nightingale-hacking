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
* \file  deviceInfoPanel.js
* \brief Javascript source for the device progress widget.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device info panel widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device info panel defs.
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

Cu.import("resource://app/jsmodules/sbProperties.jsm");
Cu.import("resource://app/jsmodules/WindowUtils.jsm");


//------------------------------------------------------------------------------
//
// Device info panel service.
//
//------------------------------------------------------------------------------

var DIPW = {
  //
  // Device info panel object fields.
  //
  //   _widget                  Device info widget.
  //   _deviceID                Device ID.
  //   _panelBar                The information panel bar widget to use.
  //   _lastOperation           The last operation performed on this device.

  _widget: null,
  _deviceID: null,
  _panelBar: null,
  _lastOperation: null,

  /**
   * \brief Initialize the device info panel services for the device info panel
   *        widget specified by aWidget.
   *
   * \param aWidget             Device info panel widget.
   */

  initialize: function DIPW__initialize(aWidget) {
    // Get the device widget.
    this._widget = aWidget;

    // Get some widget elements.
    this._panelBar  = this._getElement("device_info_panel_bar");

    // Initialize object fields.
    this._deviceID = this._widget.deviceID;
    this._device = this._widget.device;

    // Initialize the device services.
    this._deviceInitialize();
    this._update();
  },

  /**
   * \brief Finalize the device info panel services.
   */

  finalize: function DIPW_finalize() {
    // Finalize the device services.
    this._deviceFinalize();

    // Clear object fields.
    this._widget = null;
    this._deviceID = null;
    this._panelBar = null;
  },

  //----------------------------------------------------------------------------
  //
  // Device info panel UI update services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Update the device info panel UI according to the current device state.
   */

  /**
   * TODO: This will get updated with bug 18498 so that it will properly
   *  react to the current state of the device. There needs to be some updates
   *  with the device event states before that can be completed, so for now we
   *  are just fudging it.
   */

  _update: function DIPW__update() {
    // Get the current device state.
    var deviceState = this._device.state;

    switch (deviceState) {
      case Ci.sbIDevice.STATE_SYNCING:
        // If we are syncing set up the panels for the types we are going to
        // sync.

        // Setup the panel (Only Music for now)
        this._panelBar.removeAllPanels();
        this._panelBar.appendPanel(SBString("device.infoPanel.sync_music"),
                                   "chrome://songbird/skin/device/music.png",
                                   true);

        // Make sure we are visible
        if (!this._panelBar.isShown)
          this._panelBar.animateIn();

        this._lastOperation = Ci.sbIDevice.STATE_SYNCING;
        break;
      case Ci.sbIDevice.STATE_IDLE:
        // If we are IDLE after a SYNC then it is ok to disconnect
        if (this._lastOperation != Ci.sbIDevice.STATE_SYNCING)
          break;

        // First update the music panel if it exists
        var musicPanel = this._panelBar.getActivePanel();
        if (musicPanel) {
          musicPanel.label = SBString("device.infoPanel.sync_music_complete");
          musicPanel.icon = "chrome://songbird/skin/info-bar/check.png";
        }

        // Now add a Ok to disconnect panel
        this._panelBar.appendPanel(SBString("device.status.progress_complete"),
                                   "chrome://songbird/skin/info-bar/check.png",
                                   true);
        this._lastOperation = null;
        break;
      default:
        this._lastOperation = null;
        if (this._panelBar.isShown)
          this._panelBar.animateOut();
        break;
    }
  },

  //----------------------------------------------------------------------------
  //
  // Device info panel XUL services.
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

  _getElement: function DIPW__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  },

  //----------------------------------------------------------------------------
  //
  // Device info panel device services.
  //
  //   These services provide an interface to the device object.
  //   These services register for any pertinent device notifications and call
  // _update to update the UI accordingly.  In particular, these services
  // register to receive notifications when the device state changes.
  //
  //----------------------------------------------------------------------------

  //
  // Device info services fields.
  //
  //   _device                  sbIDevice object.
  //

  _device: null,


  /**
   * \brief Initialize the device services.
   */

  _deviceInitialize: function DIPW__deviceInitialize() {
    // Add a listener for status operations
    if (this._device) {
      var deviceEventTarget = this._device;
      deviceEventTarget.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.addEventListener(this);
    }
  },

  /**
   * \brief Finalize the device services.
   */

  _deviceFinalize: function DIPW__deviceFinalize() {
    // Clear object fields.
    if (this._device) {
      var deviceEventTarget = this._device;
      deviceEventTarget.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }
    this._device = null;
  },

  /**
   * \brief Listener for device events.
   *
   * \sa sbIDeviceEvent.idl
   */

   onDeviceEvent : function DIPW_onDeviceEvent(aEvent) {
    switch (aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        this._update();
        break;

    }
  }

};
