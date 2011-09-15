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
             "VIDEO": 2,
             "IMAGE": 4 };

//------------------------------------------------------------------------------
//
// Device info panel service.
//
//------------------------------------------------------------------------------

var DIPW = {
  //
  // Device info panel object fields.
  //   MEDIA_TYPES_IN_TAB_DISPLAY_ORDER An array of media type strings in tab
  //                                    display order
  //   _widget                  Device info widget.
  //   _deviceID                Device ID.
  //   _panelBar                The information panel bar widget to use.
  //   _lastOperation           The last operation performed on this device.
  //   _lastDirection           The direction of the last operation performed on
  //                            on this device
  //   _itemType                The processing item type
  //   _removePanels            The flag to remove all the panels before setup

  MEDIA_TYPES_IN_TAB_DISPLAY_ORDER: ["audio", "video", "image", "none"],
  _widget: null,
  _deviceID: null,
  _panelBar: null,
  _lastOperation: null,
  _lastDirection: null,
  _deviceErrorMonitor : null,
  _itemType: null,
  _removePanels: false,
  _lastUpdateTimeout: null,
  _operationDR : null,
  _debug : Application.prefs.getValue("songbird.device.errorInfoPanel.debug",
                                      false),
  _log : function DIPW__log(aMsg) {
    if (this._debug) {
      dump("deviceInfoPanel-" + aMsg + "\n");
    }
  },
  /**
   * \brief Initialize the device info panel services for the device info panel
   *        widget specified by aWidget.
   *
   * \param aWidget             Device info panel widget.
   */

  initialize: function DIPW__initialize(aWidget) {
    this._log("initialize");
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
    this._removePanels = true;

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

    this._itemType = createDataRemote(this._deviceID + ".status.type", null);

    this._operationDR = createDataRemote(this._deviceID + ".status.operation",
                                         null);
    this._update();

  },

  /**
   * \brief Finalize the device info panel services.
   */

  finalize: function DIPW_finalize() {
    this._log("finalize");
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
    this._operationDR = null;
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
    else if (this._itemType.intValue & TYPE.IMAGE)
      return Ci.sbIDevice.STATE_COPYING_IMAGE;
    else
      return Ci.sbIDevice.STATE_IDLE;
  },

  /**
   * \brief Get the state to update panel based on the device's state and
   *        substate.
   *
   * \return the substate of the device when syncing
   *         or the copying state based on the item type
   *         or the device state when device is not syncing (IDLE/PREPARING)
   */

  _getDeviceState: function DIPW__getDeviceState() {
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
   * Check the device capabilities to see if it supports video.
   */
  _supportsVideo: function DIPW__supportsVideo() {
    var capabilities = this._device.capabilities;
    var sbIDC = Ci.sbIDeviceCapabilities;
    try {
      if (capabilities.supportsContent(sbIDC.FUNCTION_VIDEO_PLAYBACK,
                                       sbIDC.CONTENT_VIDEO)) {
        return true;
      }
    } catch (e) {}

    // couldn't find VIDEO support
    return false;
  },

  /**
   * \brief Updates the panel(s) for the current operation
   * \param aMediaType             Type of media panel (contentType).
   * \param aDirection             sbIDeviceStatus.[EXPORT|IMPORT]
   */
  _updateOperation: function DIPW__updateOperation(aMediaType, aState, aDirection) {
    if (this._lastOperation != aState ||
        this._lastDirection != aDirection) {
      this._updateMediaInfoPanelState(aMediaType,
                                      aDirection,
                                      Ci.sbIDevice.STATE_SYNCING,
                                      true);

      // Update the current panel
      let panel = this._findMediaInfoPanel(aMediaType);
      if (panel && panel.index != 0) {
        this._removeMediaInfoPanelsAfter(aMediaType, aDirection);
        this._updateMediaInfoPanelState(aMediaType,
                                        aDirection,
                                        Ci.sbIDevice.STATE_SYNCING,
                                        true);
      }

      // Update the preceding panels to a complete state
      for each (let type in this.MEDIA_TYPES_IN_TAB_DISPLAY_ORDER) {

        // Don't worry about the current mediaType
        if (aMediaType == type) {
          break;
        }

        // If we find a different mediaType's panel, set it to complete with
        // STATE_IDLE.  It should handle error reporting if necessary.
        if (this._findMediaInfoPanel(type)) {
          this._updateMediaInfoPanelState(type,
                                          -1,
                                          Ci.sbIDevice.STATE_IDLE,
                                          false);
        }
      }
      this._lastOperation = aState;
      this._lastDirection = aDirection;
      // Make sure we are visible since the state has changed
      if (!this._panelBar.isShown)
        this._panelBar.animateIn();
    }
  },

  /**
   * \brief Update the device info panel UI according to the current device
   *        state.
   */

  /**
   *  STATE_SYNC_PREPARING   // Clear out the panel.
   *  STATE_COPY_PREPARING   // Set up what types we are syncing, show panel.
   *  STATE_SYNCING_TYPE     // Set up what types we are syncing, show panel.
   *  STATE_UPDATING         // Set up what types we are syncing, show panel.
   *  STATE_DELETING         // Set up what types we are syncing, show panel.
   *  STATE_SYNCING_MUSIC    // Put music panel in sync state.
   *  STATE_SYNCING_VIDEO    // Finish up music, Put video panel in sync state.
   *  STATE_SYN_PLAYLIST     // Finish up video, Add complete panel.
   *  STATE_IDLE             // Finish up complete panel.
   */
  _update: function DIPW__update() {
    this._log("deviceInfoPanel._update");
    var deviceType = this._device.parameters.getProperty("DeviceType");
    var state = Ci.sbIDevice.STATE_IDLE;
    if ((deviceType == "WPD") ||
        (deviceType == "MTP") ||
        (deviceType == "MSCUSB")) {
      state = this._getDeviceState();
    }
    if (state != Ci.sbIDevice.STATE_IDLE &&
        this._lastUpdateTimeout) {
      clearTimeout(this._lastUpdateTimeout);
      this._lastUpdateTimeout = null;
    }
    // See if we're importing or exporting. "Reading" is the only import
    // operation
    var direction = this._operationDR.stringValue == "reading" ?
                      Ci.sbIDeviceStatus.IMPORT : Ci.sbIDeviceStatus.EXPORT;
    this._log("state=" + state +
              ", lastOperation=" + this._lastOperation +
              ", direction=" + direction);
    switch (state) {
      case Ci.sbIDevice.STATE_COPYING:
      case Ci.sbIDevice.STATE_COPY_PREPARING:
      case Ci.sbIDevice.STATE_SYNCING_TYPE:
        break;

      case Ci.sbIDevice.STATE_SYNC_PREPARING:
        // Need to clear things up if users did not click OK button.
        this._panelBar.removeAllPanels();
        if (this._panelBar.isShown)
          this._panelBar.animateOut();
        this._lastOperation = state;
        this._lastDirection = direction;
        break;

      case Ci.sbIDevice.STATE_IMAGESYNC_PREPARING:
        this._lastOperation = state;
        this._lastDirection = direction;
        break;

      case Ci.sbIDevice.STATE_UPDATING:
      case Ci.sbIDevice.STATE_DELETING:
        var active = state == Ci.sbIDevice.STATE_UPDATING ||
                     state == Ci.sbIDevice.STATE_DELETING;
        // If we are syncing set up the panels for the types we are going to
        // sync.

        // Clear any previous errors we have.
        this._deviceErrorMonitor.clearErrorsForDevice(this._device);

        // When dragging items to the library and syncing starts, no state
        // SYNC_PREPARING in between. Remove all panels.
        if (this._removePanels) {
          this._panelBar.removeAllPanels();
          this._removePanels = false;
        }

        // Make sure we are visible
        if (this._itemType.intValue &&
            !this._panelBar.isShown)
          this._panelBar.animateIn();

        var completeAudio, completeVideo;
        // Set up the panels for the upcoming syncing.
        if ((this._itemType.intValue & TYPE.MUSIC) &&
            !this._findMediaInfoPanel("audio")) {
          this._updateMediaInfoPanel("audio",
                                     Ci.sbIDeviceStatus.EXPORT,
                                     SBString("device.infoPanel.audio_sync"),
                                     "sync",
                                     active);
          if (active) {
            this._updateMediaInfoPanelState("audio",
                                            Ci.sbIDeviceStatus.EXPORT,
                                            Ci.sbIDevice.STATE_SYNCING,
                                            true);
          }
        }

        if ((this._itemType.intValue & TYPE.VIDEO) &&
            this._supportsVideo() &&
            !this._findMediaInfoPanel("video")) {
          this._updateMediaInfoPanel("video",
                                     Ci.sbIDeviceStatus.EXPORT,
                                     SBString("device.infoPanel.video_sync"),
                                     "sync",
                                     active);
          if (active) {
            this._updateMediaInfoPanelState("video",
                                            Ci.sbIDeviceStatus.EXPORT,
                                            Ci.sbIDevice.STATE_SYNCING,
                                            true);
          }
          completeAudio = true;
        }
        if ((this._itemType.intValue & TYPE.IMAGE) &&
            !this._findMediaInfoPanel("image")) {
          this._updateMediaInfoPanel("image",
                                     Ci.sbIDeviceStatus.EXPORT,
                                     SBString("device.infoPanel.image_sync"),
                                     "sync",
                                     active);
          if (active) {
            this._updateMediaInfoPanelState("image",
                                            Ci.sbIDeviceStatus.EXPORT,
                                            Ci.sbIDevice.STATE_SYNCING,
                                            true);
          }
          completeAudio = true;
          completeVideo = true;
        }

        if (completeAudio) {
          if (this._findMediaInfoPanel("audio")) {
            this._updateMediaInfoPanelState("audio",
                                            Ci.sbIDeviceStatus.EXPORT,
                                            Ci.sbIDevice.STATE_IDLE,
                                            false);
          }
        }
        if (completeVideo) {
          if (this._findMediaInfoPanel("audio")) {
            this._updateMediaInfoPanelState("audio",
                                            Ci.sbIDeviceStatus.EXPORT,
                                            Ci.sbIDevice.STATE_IDLE,
                                            false);
          }
        }

        // - Set last operation when itemType is 0 (!= audio|video|image). This
        //   is necessary for cases that IDLE follows SYNCING_TYPE.
        //   If _itemType == 0, the sync will complete for the following IDLE.
        if (!this._itemType.intValue) {
          this._lastOperation = state;
          this._lastDirection = direction;
        }
        break;

      case Ci.sbIDevice.STATE_COPYING_MUSIC:
        this._updateOperation("audio", state, direction);
        break;

      case Ci.sbIDevice.STATE_COPYING_VIDEO:
        this._updateOperation("video", state, direction);
        break;

      case Ci.sbIDevice.STATE_SYNC_PLAYLIST:
        // Syncing playlist.
        this._lastOperation = state;
        this._lastDirection = direction;
        break;

      case Ci.sbIDevice.STATE_COPYING_IMAGE:
        this._updateOperation("image", state, direction);
        break;

      case Ci.sbIDevice.STATE_IDLE:
        // For MTP, if IDLE after a SYNC_PLAYLIST then it is ok to disconnect.
        // For MSC, IDLE after COPYING_MUSIC/COPYING_VIDEO is also applicable.
        // Sync empty playlists can leave the last operation to COPY_PREPARING,
        // SYNCING_TYPE and UPDATING. Remove playlists with only one item can
        // give out DELETING.
          switch (this._lastOperation) {
            case Ci.sbIDevice.STATE_SYNC_PLAYLIST:
            case Ci.sbIDevice.STATE_IMAGESYNC_PREPARING:
            case Ci.sbIDevice.STATE_COPYING_IMAGE:
            case Ci.sbIDevice.STATE_COPYING_MUSIC:
            case Ci.sbIDevice.STATE_COPYING_VIDEO:
            case Ci.sbIDevice.STATE_SYNCING_TYPE:
            case Ci.sbIDevice.STATE_COPY_PREPARING:
            case Ci.sbIDevice.STATE_UPDATING:
            case Ci.sbIDevice.STATE_DELETING:
              break;
            default:
              for each (let type in ["audio", "video", "image", "none"]) {
                if (this._checkForDeviceErrors(type, Ci.sbIDeviceStatus.EXPORT)) {
                  this._log("Export errors for " + type);
                  this._updateMediaInfoPanelState(type,
                                                  Ci.sbIDeviceStatus.EXPORT,
                                                  state,
                                                  false);
                  // Make sure we are visible
                  if (!this._panelBar.isShown)
                    this._panelBar.animateIn();
                }
                if (this._checkForDeviceErrors(type, Ci.sbIDeviceStatus.IMPORT)) {
                  this._log("Import errors for " + type);
                  this._updateMediaInfoPanelState(type,
                                                  Ci.sbIDeviceStatus.IMPORT,
                                                  state,
                                                  false);
                  // Make sure we are visible
                  if (!this._panelBar.isShown)
                    this._panelBar.animateIn();
                }
                if (this._checkForDeviceErrors(type, 0)) {
                  this._log("Other errors for " + type);
                  this._updateMediaInfoPanelState(type,
                                                  0,
                                                  state,
                                                  false);
                  // Make sure we are visible
                  if (!this._panelBar.isShown)
                    this._panelBar.animateIn();
                }
              }
              return;
          }
          // Intentional fall through, to process common idle/cancel logic
      case Ci.sbIDevice.STATE_CANCEL:

        if (this._lastUpdateTimeout)
          break;

        var _onLastUpdate = function(self) {

          // For MSC device. Finish up video panel if opened.
          // audio panel has to be finished up for CANCEL.
          if (self._findMediaInfoPanel("video")) {
            self._updateMediaInfoPanelState("video",
                                            self._lastDirection,
                                            Ci.sbIDevice.STATE_IDLE,
                                            false);
          }
          if (self._findMediaInfoPanel("audio")) {
            self._updateMediaInfoPanelState("audio",
                                            self._lastDirection,
                                            Ci.sbIDevice.STATE_IDLE,
                                            false);
          }
          if (self._findMediaInfoPanel("image")) {
            self._updateMediaInfoPanelState("image",
                                            0,
                                            Ci.sbIDevice.STATE_IDLE,
                                            false);
          }

          // Update the existing one to OK to disconnect
          self._updateMediaInfoPanel("complete",
                                     direction,
                                     SBString("device.status.progress_complete"),
                                     "success",
                                     true);

          // Reset to remove all panel for next sync.
          self._removePanels = true;

          self._lastOperation = state;
        }

        this._lastUpdateTimeout = setTimeout(_onLastUpdate, 1000, this);

        break;

      default:
        this._lastOperation = null;
        this._lastDirection = null;
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
   * \param aDirection             sbIDeviceStatus.[EXPORT|IMPORT]
   */

  _removeMediaInfoPanel: function DIPW__removeMediaInfoPanel(aMediaType,
                                                             aDirection) {
    var mediaPanel;
    while ((mediaPanel = this._findMediaInfoPanel(aMediaType)) != null) {
      var index = mediaPanel.index;
      this._panelBar.removePanelAt(index);
    }
  },

  /**
   * \brief This removes all panels after the panel that matches aMediaType and
   *        aDirection
   * \param aMediaType             Type of media panel (contentType).
   * \param aDirection             sbIDeviceStatus.[EXPORT|IMPORT]
   */
  _removeMediaInfoPanelsAfter: function DIPW__removeMediaInfoPanelAfter(
                                                                   aMediaType,
                                                                   aDirection) {
    let panelCount = this._panelBar.panelCount;
    for (var index = 0; index < panelCount; index++) {
      var aPanel = this._panelBar.getPanelAtIndex(index);
      if (aPanel &&
          aPanel.getAttribute("mediatype") == aMediaType &&
          aPanel.getAttribute("direction") == aDirection) {
        while (++index < panelCount) {
          this._panelBar.removePanelAt(index);
        }
        return;
      }
    }
  },
  /**
   * \brief Updates a media panel with the proper state.
   *
   * \param aMediaType             Type of media panel (contentType).
   * \param aDirection             sbIDeviceStatus.[EXPORT|IMPORT]
   * \param aState                 State of the device for this media type.
   * \param aIsActive              Make this panel active or not.
   * \return                       Whether state has error.
   */

  _updateMediaInfoPanelState: function DIPW__updateMediaInfoPanelState(
                                               aMediaType,
                                               aDirection,
                                               aState,
                                               aIsActive) {
    this._log("_updateMediaInfoPanelState(" + aMediaType +
              ", " + aDirection +
              ", " + aState +
              ", " + aIsActive + ")");
    // Anything but import is considered "sync"
    var direction = "sync";
    if (aDirection == Ci.sbIDeviceStatus.IMPORT) {
      direction = "import";
    }
    var baseString = "device.infoPanel." + aMediaType;
    var hasErrors = false;
    switch (aState) {
      case Ci.sbIDevice.STATE_IDLE:
        hasErrors = this._checkForDeviceErrors(aMediaType);
        this._log("hasErrors=" + hasErrors);
        if (hasErrors) {
          var mediaPanel = this._updateMediaInfoPanel(
                                     aMediaType,
                                     aDirection,
                                     SBString(baseString + "_errors"),
                                     "error",
                                     aIsActive);
          mediaPanel.addEventListener("sbInfoPanel-icon-click",
                                      this,
                                      false);
          mediaPanel.setAttribute("icon-clickable", true);
        }
        else {
          var mediaPanel = this._updateMediaInfoPanel(
                                     aMediaType,
                                     aDirection,
                                     SBString(baseString + "_complete"),
                                     "success",
                                     aIsActive);
        }
        break;

      case Ci.sbIDevice.STATE_SYNCING:
        var mediaPanel = this._updateMediaInfoPanel(
                                   aMediaType,
                                   aDirection,
                                   SBString(baseString + "_" + direction + "_progress"),
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
   * \param aDirection             sbIDeviceStatus.[EXPORT|IMPORT]
   * \param aSyncState             Sync, Compelete, or Error.
   * \param aIsActive              Make this panel active or not.
   */

  _updateMediaInfoPanel: function DIPW__updateMediaInfoPanel(aMediaType,
                                                             aDirection,
                                                             aMediaLabel,
                                                             aSyncState,
                                                             aIsActive) {
    this._log("_updateMediaInfoPanel(" + aMediaType + ", " +
              aDirection + ", " +
              aMediaLabel + ", " +
              aSyncState + ", " +
              aIsActive + ")");
    var mediaPanel = this._findMediaInfoPanel(aMediaType);
    if (mediaPanel == null) {
      this._log("New panel");
      mediaPanel = this._panelBar.appendPanel(aMediaLabel, "", aIsActive);
      mediaPanel.setAttribute("mediatype", aMediaType);
      mediaPanel.setAttribute("direction", aDirection);
    }
    else {
      this._log("Existing panel: " + mediaPanel.label);
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
        let mediaType = target.getAttribute("mediatype");
        let direction = target.getAttribute("direction");
        this._log("handleEvent: mediaType=" + mediaType + ", directionType=" + direction);
        if (this._checkForDeviceErrors(mediaType, direction)) {
          var errorItems =
            this._deviceErrorMonitor.getDeviceErrors(this._device,
                                                     mediaType,
                                                     direction);
          WindowUtils.openDialog
            (window,
             "chrome://songbird/content/xul/device/deviceErrorDialog.xul",
             "device_error_dialog",
             "chrome,centerscreen",
             false,
             [ "", this._device, errorItems, direction != Ci.sbIDeviceStatus.IMPORT ? "syncing" : "importing"],
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

  _checkForDeviceErrors: function DIPW__checkForDeviceErrors(aMediaType,
                                                             aDirection) {
    if (aDirection) {
      let hasErrors = false;
      try {
        hasErrors = this._deviceErrorMonitor.deviceHasErrors(this._device,
                                                             aMediaType,
                                                             aDirection);
      } catch (err) {
        Cu.reportError(err);
      }
      return hasErrors;
    }
    else {
      let hasImportErrors = false;
      let hasExportErrors = false;
      try {
        hasImportErrors = this._deviceErrorMonitor.deviceHasErrors
                                                 (this._device,
                                                  aMediaType,
                                                  Ci.sbIDeviceStatus.IMPORT);
        hasExportErrors = this._deviceErrorMonitor.deviceHasErrors
                                                  (this._device,
                                                   aMediaType,
                                                   Ci.sbIDeviceStatus.EXPORT);
      } catch (err) {
        Cu.reportError(err);
      }
      return (hasImportErrors || hasExportErrors);
    }
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
