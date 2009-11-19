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
  _deviceErrorMonitor : null,

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

    // Grab the device error monitor service so we can check for errors on this
    // device when we need to.
    this._deviceErrorMonitor =
                          Cc["@songbirdnest.com/device/error-monitor-service;1"]
                            .getService(Ci.sbIDeviceErrorMonitor);

    this._update();

  },

  /**
   * \brief Finalize the device info panel services.
   */

  finalize: function DIPW_finalize() {
    // Clean up the device error monitor
    if (this._deviceErrorMonitor) {
      this._deviceErrorMonitor = null;
    }
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
   *
   * Later we will have the following states:
   *  STATE_PREPARING_SYNC          // Do nothing.
   *  STATE_SYNCING                 // Set up what types we are syncing, show panel.
   *    STATE_PREPARING_SYNC_MUSIC  // Put music panel in sync state.
   *      STATE_SYNCING_MUSIC       // Do nothing
   *    STATE_PREPARING_SYNC_VIDEO  // Finish up music, Put video panel in sync state.
   *      STATE_SYNCING_VIDEO       // Do nothing
   *    STATE_SYNCING_PLAYLISTS     // Finish up video, Add complete panel.
   *  STATE_IDLE                    // Finish up complete panel.
   */

  _update: function DIPW__update() {
    // Get the current device state.
    var deviceState = this._device.state;
    switch (deviceState) {
      // case Ci.sbIDevice.STATE_PREPARING_SYNC: // Ignore
      case Ci.sbIDevice.STATE_SYNCING:
        // If we are syncing set up the panels for the types we are going to
        // sync.

        // Clear any previous errors we have.
        this._deviceErrorMonitor.clearErrorsForDevice(this._device);

        // Setup the panel
        this._panelBar.removeAllPanels();
        // "audio" will be changed to the contentType later on.
        this._updateMediaInfoPanelState("audio", deviceState);

        // Make sure we are visible
        if (!this._panelBar.isShown)
          this._panelBar.animateIn();

        this._lastOperation = Ci.sbIDevice.STATE_SYNCING;
        break;

      case Ci.sbIDevice.STATE_IDLE:
        // If we are IDLE after a SYNC then it is ok to disconnect
        // Later change this to STATE_SYCNING_PLAYLISTS
        if (this._lastOperation != Ci.sbIDevice.STATE_SYNCING)
          break;

        // This is here temporarly until we have the VIDEO/MUSIC sync states.
        this._updateMediaInfoPanelState("audio", deviceState);

        // Now add a Ok to disconnect panel
        this._updateMediaInfoPanel("complete",
                                   SBString("device.status.progress_complete"),
                                   "success",
                                   true);
        this._lastOperation = null;
        break;
      // TODO: Add other states
      // case Ci.sbIDevice.STATE_PREPARING_SYNC_MUSIC:
      // case Ci.sbIDevice.STATE_SYNCING_MUSIC:
      // case Ci.sbIDevice.STATE_PREPARING_SYNC_VIDEO:
      // case Ci.sbIDevice.STATE_SYNCING_VIDEO:
      // case Ci.sbIDevice.STATE_SYNCING_PLAYLISTS:

      default:
        this._lastOperation = null;
        if (this._panelBar.isShown)
          this._panelBar.animateOut();
        break;
    }
  },

  /**
   * \brief Remove a panel of a particular media type, we should only ever have
   *        one panel of each type so this will ensure that doubles are removed.
   *
   * \param aMediaType             Type of media panel (contentType).
   */

  _removeMediaInfoPanel: function DIPW__removeMediaInfoPanel(aMediaType) {
    var mediaPanel;
    while ((mediaPanel = this._findMediaInfoPanel(aMediaType)) != null) {
      var index = mediaPanel.index;
      this._panelBar.removePanelAt(index);
    }
  },

  /**
   * \brief Updates a media panel with the proper state.
   *
   * \param aMediaType             Type of media panel (contentType).
   * \param aState                 State of the device for this media type.
   */

  _updateMediaInfoPanelState: function DIPW__updateMediaInfoPanelState(aMediaType,
                                                                       aState) {
    var baseString = "device.infoPanel.sync_" + aMediaType;

    switch (aState) {
      case Ci.sbIDevice.STATE_IDLE:
        var hasErrors = this._checkForDeviceErrors(aMediaType);
        if (hasErrors) {
          var mediaPanel = this._updateMediaInfoPanel(aMediaType,
                                     SBString(baseString + "_errors"),
                                     "error",
                                     true);
          mediaPanel.addEventListener("sbInfoPanel-icon-click",
                                      this,
                                      false);
          mediaPanel.setAttribute("icon-clickable", true);
        }
        else {
          var mediaPanel = this._updateMediaInfoPanel(aMediaType,
                                     SBString(baseString + "_complete"),
                                     "success",
                                     true);
          mediaPanel.removeAttribute("icon-clickable");
        }
        break;
      case Ci.sbIDevice.STATE_SYNCING:
        var mediaPanel = this._updateMediaInfoPanel(aMediaType,
                                   SBString(baseString),
                                   "sync",
                                   true);
        mediaPanel.removeAttribute("icon-clickable");
        break;
    }
  },

  /**
   * \brief Updates a media panel information.
   *
   * \param aMediaType             Type of media panel (contentType).
   * \param aMediaLabel            Text to display in the panel.
   * \param aMediaIcon             Icon to display in the panel.
   * \param aIsActive              Make this panel active or not.
   */

  _updateMediaInfoPanel: function DIPW__updateMediaInfoPanel(aMediaType,
                                                             aMediaLabel,
                                                             aSyncState,
                                                             aIsActive) {
    var mediaPanel = this._findMediaInfoPanel(aMediaType);

    if (mediaPanel == null) {
      mediaPanel = this._panelBar.appendPanel(aMediaLabel, "", aIsActive);
      mediaPanel.setAttribute("mediatype", aMediaType);
    }
    else {
      mediaPanel.label = aMediaLabel;
      mediaPanel.active = aIsActive;
    }

    if (aSyncState)
      mediaPanel.setAttribute("state", aSyncState);
    else
      mediaPanel.removeAttribute("state");

    return mediaPanel;
  },

  /**
   * \brief Finds a panel of a particular type in the panel bar. Normally there
   *        should only be one or none of each type.
   *
   * \param aMediaType             Type of media panel (contentType).
   */

  _findMediaInfoPanel: function DIPW__findMediaInfoPanel(aMediaType) {
    var panelCount = this._panelBar.panelCount;
    for (var index = 0; index < panelCount; index++) {
      var aPanel = this._panelBar.getPanelAtIndex(index);
      if (aPanel &&
          aPanel.getAttribute("mediatype") == aMediaType)
        return aPanel;
    }
    return null;
  },

  /**
   * \brief Handles events. Currently we only listen for click events on the
   *        icons in the info panels.
   *
   * \param aEvent             nsIDOMEvent of the event that occured.
   */

  handleEvent: function DIPW_handleEvent(aEvent) {
    // Check if this is a panel icon click event.
    if (aEvent.type == "sbInfoPanel-icon-click") {
      var target = aEvent.target
      // Make sure our target has a media type
      if (target.hasAttribute("mediatype")) {
        var aMediaType = target.getAttribute("mediatype");
        if (this._checkForDeviceErrors(aMediaType)) {
          var errorItems =
            this._deviceErrorMonitor.getDeviceErrors(this._device, aMediaType);
          WindowUtils.openDialog
            (window,
             "chrome://songbird/content/xul/device/deviceErrorDialog.xul",
             "device_error_dialog",
             "chrome,centerscreen",
             false,
             [ "", this._device, errorItems, "syncing" ],
             null);

          // We handled this event so stop the propagation
          aEvent.stopPropagation();
          aEvent.preventDefault();
        }
      }
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
   * \brief Check if the device has errors for a particular type of media
   */

  _checkForDeviceErrors: function DIPW__checkForDeviceErrors(aMediaType) {
    var hasErrors = false;
    try {
      hasErrors = this._deviceErrorMonitor.deviceHasErrors(this._device,
                                                           aMediaType);
    } catch (err) {
      Cu.reportError(err);
    }
    return hasErrors;
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
