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

var TYPE = { "NONE": 0,
             "MUSIC": 1,
             "VIDEO": 2 };

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
  //   _itemType                The processing item type
  //   _removePanels            The flag to remove all the panels before setup

  _widget: null,
  _deviceID: null,
  _panelBar: null,
  _lastOperation: null,
  _deviceErrorMonitor : null,
  _itemType: null,
  _removePanels: null,

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
    this._panelBar = this._getElement("device_info_panel_bar");

    // Listen to the click/keypress events for OK button in the progress widget
    document.addEventListener("sbDeviceProgress-ok-button-enter",
                              this,
                              false);

    // Initialize object fields.
    this._deviceID = this._widget.deviceID;
    this._device = this._widget.device;

    // Always want to remove all the panels before syncing starts
    this._removePanels = 1;

    // Initialize the device services.
    this._deviceInitialize();

    // Grab the device error monitor service so we can check for errors on this
    // device when we need to.
    this._deviceErrorMonitor =
                          Cc["@songbirdnest.com/device/error-monitor-service;1"]
                            .getService(Ci.sbIDeviceErrorMonitor);

    // Get the processing item type
    var createDataRemote = new Components.Constructor(
                                    "@songbirdnest.com/Songbird/DataRemote;1",
                                    Ci.sbIDataRemote,
                                    "init");

    this._itemType = createDataRemote(
                    this._deviceID + ".status.type", null);

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
    this._itemType = null;
  },

  //----------------------------------------------------------------------------
  //
  // Device info panel UI update services.
  //
  //----------------------------------------------------------------------------


  /**
   * \brief Get the copying state
   *
   * \return state COPYING_MUSIC, COPYING_VIDEO or IDLE when item type not match
   */

  _getCopyingState: function DIPW__getCopyingState() {
    if (this._itemType.intValue & TYPE.MUSIC)
      return Ci.sbIDevice.STATE_COPYING_MUSIC;
    else if (this._itemType.intValue & TYPE.VIDEO)
      return Ci.sbIDevice.STATE_COPYING_VIDEO;
    else
      return Ci.sbIDevice.STATE_IDLE;
  },


  /**
   * \brief Get the state to update panel based on MSC device state and substate.
   *
   * \return the substate when the MSC device is when syncing
   *         or the copying state based on the item type
   *         or the device state when device is not syncing (IDLE/PREPARING)
   */

  _getMSCDeviceState: function DIPW__getMSCDeviceState() {
    // Get the current device state.
    var deviceState = this._device.state;
    var substate = this._device.currentStatus.currentSubState;
    var state = deviceState;
    if (deviceState ==  Ci.sbIDevice.STATE_SYNCING) {
      if (substate == Ci.sbIDevice.STATE_SYNCING_TYPE ||
          substate == Ci.sbIDevice.STATE_COPY_PREPARING ||
          substate == Ci.sbIDevice.STATE_SYNC_PLAYLIST ||
          substate == Ci.sbIDevice.STATE_DELETING ||
          substate == Ci.sbIDevice.STATE_UPDATING ||
          substate == Ci.sbIDevice.STATE_IDLE) {
        state = substate;
      }
      else if (substate == Ci.sbIDevice.STATE_COPYING ||
               substate == Ci.sbIDevice.STATE_TRANSCODE) {
        state = this._getCopyingState();
      }
    }
    else if (deviceState == Ci.sbIDevice.STATE_COPYING ||
             deviceState == Ci.sbIDevice.STATE_TRANSCODE ||
             deviceState == Ci.sbIDevice.STATE_DELETING) {
      state = this._getCopyingState();
    }

    return state;
  },

  /**
   * \brief Get the state to update panel based on MTP device state and substate.
   *
   * \return the substate when the MTP device is when syncing
   *         or the copying state based on the item type
   *         or the device state when device is not syncing (IDLE/PREPARING)
   */

  _getMTPDeviceState: function DIPW__getMTPDeviceState() {
    // Get the current device state.
    var deviceState = this._device.state;
    var substate = this._device.currentStatus.currentSubState;
    var state = deviceState;
    if (deviceState ==  Ci.sbIDevice.STATE_SYNCING) {
      if (substate == Ci.sbIDevice.STATE_SYNCING_TYPE ||
          substate == Ci.sbIDevice.STATE_COPY_PREPARING ||
          substate == Ci.sbIDevice.STATE_SYNC_PLAYLIST ||
          substate == Ci.sbIDevice.STATE_UPDATING ||
          substate == Ci.sbIDevice.STATE_IDLE) {
        state = substate;
      }
      else if (substate == Ci.sbIDevice.STATE_COPYING ||
               substate == Ci.sbIDevice.STATE_DELETING ||
               substate == Ci.sbIDevice.STATE_TRANSCODE) {
        state = this._getCopyingState();
      }
    }
    else if (deviceState == Ci.sbIDevice.STATE_TRANSCODE) {
      state = this._getCopyingState();
    }
    // Handle the case when dragging items to the main library while device
    // is in syncing mode
    else if (deviceState == Ci.sbIDevice.STATE_COPYING &&
             substate == Ci.sbIDevice.STATE_IDLE) {
      if (this._itemType.intValue == TYPE.NONE) {
        state = Ci.sbIDevice.STATE_COPYING;
      }
      else {
        state = this._getCopyingState();
      }
    }

    return state;
  },

  /**
   * \brief Update the device info panel UI according to the current device state.
   */

  /**
   *  STATE_SYNC_PREPARING          // Clear out the panel.
   *  STATE_COPY_PREPARING          // Set up what types we are syncing, show panel.
   *  STATE_SYNCING_TYPE            // Set up what types we are syncing, show panel.
   *  STATE_UPDATING                // Set up what types we are syncing, show panel.
   *  STATE_DELETING                // Set up what types we are syncing, show panel.
   *  STATE_SYNCING_MUSIC           // Put music panel in sync state.
   *  STATE_SYNCING_VIDEO           // Finish up music, Put video panel in sync state.
   *  STATE_SYN_PLAYLIST            // Finish up video, Add complete panel.
   *  STATE_IDLE                    // Finish up complete panel.
   */

  _update: function DIPW__update() {
    var deviceType = this._device.parameters.getProperty("DeviceType");
    var state = Ci.sbIDevice.STATE_IDLE;
    if ((deviceType == "WPD") || (deviceType == "MTP"))
      state = this._getMTPDeviceState();
    else if (deviceType == "MSCUSB")
      state = this._getMSCDeviceState();

    switch (state) {
      case Ci.sbIDevice.STATE_COPYING:
        break;

      case Ci.sbIDevice.STATE_SYNC_PREPARING:
        // Need to clear things up if users did not click OK button.
        this._panelBar.removeAllPanels();
        if (this._panelBar.isShown)
          this._panelBar.animateOut();
        this._lastOperation = state;
        break;

      case Ci.sbIDevice.STATE_COPY_PREPARING:
      case Ci.sbIDevice.STATE_SYNCING_TYPE:
      case Ci.sbIDevice.STATE_UPDATING:
      case Ci.sbIDevice.STATE_DELETING:
        // If we are syncing set up the panels for the types we are going to
        // sync.

        // Clear any previous errors we have.
        this._deviceErrorMonitor.clearErrorsForDevice(this._device);

        // When dragging items to the library and syncing starts, no state
        // SYNC_PREPARING in between. Remove all panels.
        if (this._removePanels) {
          this._panelBar.removeAllPanels();
          this._removePanels = 0;
        }

        // Set up the panels for the upcoming syncing.
        if ((this._itemType.intValue & TYPE.MUSIC) &&
            !this._findMediaInfoPanel("audio")) {
          this._updateMediaInfoPanel("audio",
                                     SBString("device.infoPanel.sync_audio"),
                                     "sync",
                                     false);
        }
        if ((this._itemType.intValue & TYPE.VIDEO) &&
            !this._findMediaInfoPanel("video")) {
          this._updateMediaInfoPanel("video",
                                     SBString("device.infoPanel.sync_video"),
                                     "sync",
                                     false);
        }

        // Make sure we are visible
        if (!this._panelBar.isShown)
          this._panelBar.animateIn();

        // - Set last operation when itemType is 0 (!= audio|video). This is
        //   necessary for cases that IDLE follows SYNCING_TYPE.
        //   If _itemType == 0, the sync will complete for the following IDLE.
        if (!this._itemType.intValue)
          this._lastOperation = state;

        break;

      case Ci.sbIDevice.STATE_COPYING_MUSIC:
        if (this._lastOperation != Ci.sbIDevice.STATE_COPYING_MUSIC) {
          // Syncing music
          this._updateMediaInfoPanelState("audio", Ci.sbIDevice.STATE_SYNCING, true);
          this._lastOperation = state;

          // Necessary to test and remove the preceding video panel
          audioPanel = this._findMediaInfoPanel("audio");
          if (audioPanel && audioPanel.index != 0) {
            this._removeMediaInfoPanel("video");
            this._updateMediaInfoPanelState("audio", Ci.sbIDevice.STATE_SYNCING, true);
          }
        }
        break;

      case Ci.sbIDevice.STATE_COPYING_VIDEO:
        if (this._lastOperation != Ci.sbIDevice.STATE_COPYING_VIDEO) {
          // Syncing video. Finish up audio panel if opened.
          if (this._findMediaInfoPanel("audio")) {
            this._updateMediaInfoPanelState("audio", Ci.sbIDevice.STATE_IDLE, false);
          }

          // Mark video panel active
          this._updateMediaInfoPanelState("video", Ci.sbIDevice.STATE_SYNCING, true);
          this._lastOperation = state;
        }
        break;

      case Ci.sbIDevice.STATE_SYNC_PLAYLIST:
        // Syncing playlist. Finish up audio/video panels if opened.
        if (this._findMediaInfoPanel("video")) {
          this._updateMediaInfoPanelState("video", Ci.sbIDevice.STATE_IDLE, false);
        }
        if (this._findMediaInfoPanel("audio")) {
          this._updateMediaInfoPanelState("audio", Ci.sbIDevice.STATE_IDLE, false);
        }

        this._updateMediaInfoPanelState("complete", Ci.sbIDevice.STATE_SYNCING, true);
        this._lastOperation = state;
        break;

      case Ci.sbIDevice.STATE_IDLE:
      case Ci.sbIDevice.STATE_CANCEL:
        // For MTP, if IDLE after a SYNC_PLAYLIST then it is ok to disconnect.
        // For MSC, IDLE after COPYING_MUSIC/COPYING_VIDEO is also applicable.
        // Sync empty playlists can leave the last operation to COPY_PREPARING,
        // SYNCING_TYPE and UPDATING. Remove playlists with only one item can
        // give out DELETING.
        if (state != Ci.sbIDevice.STATE_CANCEL &&
            this._lastOperation != Ci.sbIDevice.STATE_SYNC_PLAYLIST &&
            this._lastOperation != Ci.sbIDevice.STATE_COPYING_MUSIC &&
            this._lastOperation != Ci.sbIDevice.STATE_COPYING_VIDEO &&
            this._lastOperation != Ci.sbIDevice.STATE_SYNCING_TYPE &&
            this._lastOperation != Ci.sbIDevice.STATE_COPY_PREPARING &&
            this._lastOperation != Ci.sbIDevice.STATE_UPDATING &&
            this._lastOperation != Ci.sbIDevice.STATE_DELETING) {
          break;
        }

        var hasError = false;
        // For MSC device. Finish up video panel if opened.
        // audio panel has to be finished up for CANCEL.
        if (this._lastOperation != Ci.sbIDevice.STATE_SYNC_PLAYLIST) {
          if (this._findMediaInfoPanel("video")) {
            this._updateMediaInfoPanelState("video", Ci.sbIDevice.STATE_IDLE, false);
          }
          if (this._findMediaInfoPanel("audio")) {
            this._updateMediaInfoPanelState("audio", Ci.sbIDevice.STATE_IDLE, false);
          }
        }
        else {
          // Finish up complete panel
          hasError = this._updateMediaInfoPanelState("complete", state, true);
        }

        if (!hasError) {
          // Update the existing one to OK to disconnect
          this._updateMediaInfoPanel("complete",
                                     SBString("device.status.progress_complete"),
                                     "success",
                                     true);
        }
        else {
          // Append a Ok to disconnect panel
          this._updateMediaInfoPanelState("complete", state, false);
          this._updateMediaInfoPanel("complete_success",
                                     SBString("device.status.progress_complete"),
                                     "success",
                                     true);
        }

        // Reset to remove all panel for next sync.
        this._removePanels = 1;

        this._lastOperation = state;
        break;

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
   * \param aIsActive              Make this panel active or not.
   * \return                       Whether state has error.
   */

  _updateMediaInfoPanelState: function DIPW__updateMediaInfoPanelState(aMediaType,
                                                                       aState,
                                                                       aIsActive) {
    var baseString = "device.infoPanel.sync_" + aMediaType;

    var hasErrors = false;
    switch (aState) {
      case Ci.sbIDevice.STATE_IDLE:
        hasErrors = this._checkForDeviceErrors(aMediaType);
        if (hasErrors) {
          var mediaPanel = this._updateMediaInfoPanel(aMediaType,
                                     SBString(baseString + "_errors"),
                                     "error",
                                     aIsActive);
          mediaPanel.addEventListener("sbInfoPanel-icon-click",
                                      this,
                                      false);
          mediaPanel.setAttribute("icon-clickable", true);
        }
        else {
          var mediaPanel = this._updateMediaInfoPanel(aMediaType,
                                     SBString(baseString + "_complete"),
                                     "success",
                                     aIsActive);
        }
        break;
      case Ci.sbIDevice.STATE_SYNCING:
        var mediaPanel = this._updateMediaInfoPanel(aMediaType,
                                   SBString(baseString + "_progress"),
                                   "sync",
                                   aIsActive);
        break;
    }

    return hasErrors;
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

    // Remove icon-clickable by default.
    mediaPanel.removeAttribute("icon-clickable");
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
    else if (aEvent.type == "sbDeviceProgress-ok-button-enter") {
      this._panelBar.removeAllPanels();
      if (this._panelBar.isShown)
        this._panelBar.animateOut();
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
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MEDIA_WRITE_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_START:
        this._update();
        break;
    }
  }

};
