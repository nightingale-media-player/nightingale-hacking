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
* \file  deviceProgress.js
* \brief Javascript source for the device progress widget.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device progress widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device progress defs.
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
Cu.import("resource://app/jsmodules/sbTimeFormatter.jsm");

//------------------------------------------------------------------------------
//
// Device progress configuration.
//
//------------------------------------------------------------------------------

/*
 * operationInfoList            List of device operation information records.
 *   state                      Operation state value.
 *   localeSuffix               Locale string key suffix for operation.
 *   progressMeterUndetermined  If true, display undetermined progress meter for
 *                              operation.
 *   progressMeterDetermined    If true, display determined progress meter for
 *                              operation.
 *   canBeCompleted             If true, operation can be completed and should
 *                              be set for last completed operation.
 *   showIdleMessage            If true, and this was the last completed
 *                              operation, then reflect that in the idle msg.
 *   showProgress               If true, progress should be displayed for
 *                              operation.
 *   updateIdle                 If true, update UI for an idle operation.
 *   updateBusy                 If true, update UI for a busy operation.
 *   operationCanceled          If true, remember that the current operation
 *                              was canceled
 */

var DPWCfg = {
  operationInfoList: [
    /* Idle. */
    {
      state: Ci.sbIDevice.STATE_IDLE,
      localeSuffix: "idle",
      updateIdle: true
    },

    /* Sync. */
    {
      state: Ci.sbIDevice.STATE_SYNCING,
      localeSuffix: "syncing",
      progressMeterDetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      preparingOnIdle: true,
      operationCanceled: false
    },

    /* Sync preparation. */
    {
      state: Ci.sbIDevice.STATE_SYNC_PREPARING,
      localeSuffix: "sync_preparing",
      progressMeterUndetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      operationCanceled: false
    },

    /* Sync preparation. */
    {
      state: Ci.sbIDevice.STATE_IMAGESYNC_PREPARING,
      localeSuffix: "imagesync_preparing",
      progressMeterUndetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      operationCanceled: false
    },

    /* Copy. */
    {
      state: Ci.sbIDevice.STATE_COPYING,
      localeSuffix: "copying",
      progressMeterDetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      operationCanceled: false
    },

    /* Delete. */
    {
      state: Ci.sbIDevice.STATE_DELETING,
      localeSuffix: "deleting",
      progressMeterDetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      operationCanceled: false
    },

    /* Update. */
    {
      state: Ci.sbIDevice.STATE_UPDATING,
      localeSuffix: "updating",
      progressMeterDetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      operationCanceled: false
    },

    /* Mount. */
    {
      state: Ci.sbIDevice.STATE_MOUNTING,
      localeSuffix: "mounting",
      progressMeterUndetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      operationCanceled: false
    },

    /* Cancel. */
    {
      state: Ci.sbIDevice.STATE_CANCEL,
      localeSuffix: "cancelling",
      progressMeterUndetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      operationCanceled: true
    },

    /* Transcode */
    {
      state: Ci.sbIDevice.STATE_TRANSCODE,
      localeSuffix: "transcoding",
      progressMeterDetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      operationCanceled: false
    },

    /* Format. */
    {
      state: Ci.sbIDevice.STATE_FORMATTING,
      localeSuffix: "formatting",
      progressMeterUndetermined: true,
      canBeCompleted: true,
      showIdleMessage: true,
      showProgress: true,
      updateBusy: true,
      operationCanceled: false
    }
  ]
};


//------------------------------------------------------------------------------
//
// Device progress services.
//
//------------------------------------------------------------------------------

var DPW = {
  //
  // Device progress object fields.
  //
  //   _cfg                     Configuration.
  //   _widget                  Device info widget.
  //   _device                  Device bound to device control widget.
  //   _deviceID                Device ID.
  //   _deviceLibrary           Device library we are working with.
  //   _deviceErrorMonitor      Error monitor for the device
  //   _curItemIndex            Current index in the batch of items to process
  //   _totalItems              Total items in the batch
  //   _operationInfoTable      Table mapping operation state values to
  //                            operation information records.
  //   _lastEventOperation      Operation of last state changed event.
  //   _lastCompletedEventOperation
  //                            Last completed operation of last state changed
  //                            event.
  //   _lastIsTranscoding       True if the locale suffix of previous operation
  //                            is transcoding.
  //   _operationCanceled       If true, last operation was canceled
  //   _showProgress            If true, progress status should be displayed.
  //   _syncSettingsChanged     If true, buttons to apply/cancel sync settings
  //                            should be displayed.
  //
  //   _progressInfoBox         Device progress info.
  //   _settingsCancelButton    Cancel button for sync settings modifications.
  //   _settingsApplyButton     Apply button for sync settings modifications.
  //   _syncButton              Sync button element.
  //   _idleBox                 Idle message & spacer.
  //   _cancelButtonBox         Cancel button box element.
  //   _finishButton            Finish button element.
  //   _progressMeter           Progress meter element.
  //   _progressTextLabel       Progress text label element.
  //   _idleLabel               Idle text label element.
  //   _idleBox                 Idle box holding idle text.
  //
  //   _lastProgress            Saved last item progress number.
  //   _ratio                   The ratio for transcoding : transferring.
  //   _firstCopying            True for the first copying after transcoding
  //
  //   _transcodingStart        Time stamp when transcoding starts.
  //   _transcodingEnd          Time stamp when transcoding ends.
  //   _copyingStart            Time stamp when copying starts.
  //   _copyingEnd              Time stamp when copying ends.
  //

  _cfg: DPWCfg,
  _widget: null,
  _device: null,
  _deviceErrorMonitor : null,
  _deviceID: null,
  _deviceLibrary: null,
  _curItemIndex: null,
  _totalItems: null,
  _operationInfoTable: null,
  _lastIsTranscoding: false,
  _lastEventOperation: Ci.sbIDevice.STATE_IDLE,
  _lastCompletedEventOperation: Ci.sbIDevice.STATE_IDLE,
  _operationCanceled: false,
  _showProgress: false,
  _syncSettingsChanged: false,

  _progressInfoBox: null,
  _settingsCancelButton: null,
  _settingsApplyButton: null,
  _syncButton: null,
  _cancelButtonBox: null,
  _finishButton: null,
  _progressMeter: null,
  _progressTextLabel: null,
  _idleLabel: null,
  _idleErrorsLabel: null,
  _idleBox: null,
  _syncManualBox: null,

  _lastProgress: 0,
  _ratio: 1,
  _firstCopying: false,

  _transcodingStart: 0,
  _transcodingEnd: 0,
  _copyingStart: 0,
  _copyingEnd: 0,

  /**
   * \brief Initialize the device progress services for the device progress
   *        widget specified by aWidget.
   *
   * \param aWidget             Device progress widget.
   */

  initialize: function DPW__initialize(aWidget) {
    Cu.import("resource://app/jsmodules/DeviceHelper.jsm", this);

    // Get the device widget.
    this._widget = aWidget;

    this._deviceErrorMonitor =
      Cc["@songbirdnest.com/device/error-monitor-service;1"]
        .getService(Ci.sbIDeviceErrorMonitor);

    // Initialize the operation info table.
    this._operationInfoTable = {};
    for (var i = 0; i < this._cfg.operationInfoList.length; i++) {
      var operationInfo = this._cfg.operationInfoList[i];
      this._operationInfoTable[operationInfo.state] = operationInfo;
    }

    // Get some widget elements.
    this._progressInfoBox  = this._getElement("progress_information_box");
    this._settingsCancelButton = this._getElement("settings_cancel_button");
    this._settingsApplyButton = this._getElement("settings_apply_button");
    this._syncButton       = this._getElement("sync_operation_button");
    this._cancelButtonBox = this._getElement("cancel_operation_box");
    this._finishButton = this._getElement("finish_progress_button");
    this._progressMeter = this._getElement("progress_meter");
    this._progressTextLabel = this._getElement("progress_text_label");
    this._subProgressTextLabel = this._getElement("sub_progress_text_label")
    this._idleBox = this._getElement("progress_idle_box");
    this._syncManualBox = this._getElement("syncmode_box");

    this._finishButton.addEventListener("click", this._onButtonEvent, false);
    this._finishButton.addEventListener("keypress", this._onButtonEvent, false);

    document.addEventListener("sbDeviceSync-settings", this._onSettingsEvent, false);

    // Initialize object fields.
    this._deviceID = this._widget.deviceID;
    this._device = this._widget.device;
    this._deviceLibrary = this._widget.devLib;

    // Set the label accordingly.
    var syncModeLabel = this._getElement("syncmode_label");
    var key = this._supportsVideo() ? "device.progress.audiovideosync.label"
                                    : "device.progress.audiosync.label";
    syncModeLabel.value = SBString(key, "");

    // Initialize the device services.
    this._deviceInitialize();

    // Get the device operation total items and current item index data remotes.
    var createDataRemote = new Components.Constructor(
                                    "@songbirdnest.com/Songbird/DataRemote;1",
                                    Ci.sbIDataRemote,
                                    "init");
    this._totalItems = createDataRemote(
                    this._deviceID + ".status.totalcount", null);
    this._curItemIndex = createDataRemote(
                    this._deviceID + ".status.workcount", null);
    this._itemProgress = createDataRemote(
                    this._deviceID + ".status.progress", null);
    this._itemType = createDataRemote(
                    this._deviceID + ".status.type", null);

    // Update the last completed operation
    this._lastCompletedEventOperation = this._device.previousState;

    // Read the ratio value from preference.
    var prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);
    if (!this._device.isDirectTranscoding) {
      try {
        let key = "device.transcoding.transfer.ratio";
        this._ratio = prefService.getIntPref(key);
      } catch (err) {}
    }

    // Simulate a device state changed event to initialize the operation state
    // and update the UI.
    this._handleStateChanged({ data: this._device.state });
    this._update();
  },


  /**
   * \brief Finalize the device progress services.
   */

  finalize: function DPW_finalize() {
    // Finalize the device services.
    this._deviceFinalize();

    // Clear object fields.
    this._widget = null;
    this._device = null;
    this._deviceErrorMonitor = null;
    this._deviceID = null;
    this._deviceLibrary = null;
    this._operationInfoTable = null;
    this._progressInfoBox = null;
    this._idleBox = null;
    this._settingsCancelButton = null;
    this._settingsApplyButton = null;
    this._syncButton   = null;
    this._cancelButtonBox = null;
    this._finishButton = null;
    this._progressMeter = null;
    this._progressTextLabel = null;
    this._idleLabel = null;
    this._idleErrorLabel = null;
    this._syncManualBox = null;
  },


  //----------------------------------------------------------------------------
  //
  // Device progress UI update services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Check the device capabilities to see if it supports video.
   */

  _supportsVideo: function DPW__supportsVideo() {
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
   * \brief Update the device progress UI according to the current device state.
   */

  _update: function DPW__update() {
    // Get the current device state.
    var deviceState = this._device.state;

    // Dispatch progress update processing based upon the current device state.
    var deviceStateOperationInfo = this._getOperationInfo(deviceState);
    if (deviceStateOperationInfo.updateBusy)
      this._updateProgressBusy();
    else if (deviceStateOperationInfo.updateIdle)
      this._updateProgressIdle();

    // Show cancel button while busy and finish button while idle.
    var cancelButtonHidden;
    var finishButtonHidden;
    if (deviceState == Ci.sbIDevice.STATE_IDLE) {
      cancelButtonHidden = true;
      finishButtonHidden = false;
    } else {
      cancelButtonHidden = false;
      finishButtonHidden = true;
    }

    // Hide or show progress.
    var hideProgress = !this._showProgress;
    this._progressInfoBox.hidden = hideProgress;
    this._idleBox.hidden = !hideProgress;
    if (hideProgress) {
      cancelButtonHidden = true;
      finishButtonHidden = true;
    }

    // Only show sync button if not showing progress.
    this._settingsCancelButton.hidden = this._showProgress || !this._syncSettingsChanged;
    this._settingsApplyButton.hidden = this._showProgress || !this._syncSettingsChanged;
    this._syncButton.hidden = this._showProgress || this._syncSettingsChanged;
    this._syncManualBox.hidden = this._showProgress;

    // Set cancel and hide button hidden property.  The device cancel command
    // widget automatically hides/shows itself depending upon the device state.
    // Thus, the cancel button is hidden by hiding an enclosing box so as not to
    // interfere with the internal cancel hiding logic.
    this._cancelButtonBox.hidden = cancelButtonHidden;
    this._finishButton.hidden = finishButtonHidden;

    // Update the sync mode toggle buttons
    var syncModeToggle = this._getElement("syncmode-toggle");
    if (syncModeToggle) {
      syncModeToggle.deviceInitialize();
    }
  },


  /**
   * \brief Update the progress UI for the idle state.
   */

  _updateProgressIdle: function DPW__updateProgressIdle() {
    this._lastProgress = 0;
    this._lastIsTranscoding = false;

    // Write back the ratio so this can be used in the future.
    var prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);
    if (!this._device.isDirectTranscoding) {
      let key = "device.transcoding.transfer.ratio";
      prefService.setIntPref(key, this._ratio);
    }

    var oInfo = this._getOperationInfo(this._lastCompletedEventOperation);
    if (oInfo.showIdleMessage) {
      // Per bug 20294, don't bother showing different idle messages for each
      // operation type.
      var key = "device.status.progress_completed";
      
      // Check for errors, notify of errors, cancel takes precedence
      var hasErrors = this._checkForDeviceErrors("");
      if (this._operationCanceled) {
        key = "device.status.progress_aborted";
      }
      else if (hasErrors) { 
        key += "_errors";
      }
      else {
        key += "_ok";
      }
      this._progressTextLabel.value = SBString(key, "");
    }

    this._progressMeter.hidden = true;
    this._subProgressTextLabel.hidden = true;

    this._startTimestamp = null;
    this._lastTimeLeft = Infinity;
  },


  /**
   * \brief get the request item type
   */

  _getItemType: function DPW__getItemType() {
    if (this._itemType.intValue == 1)
      return "audio";
    else if (this._itemType.intValue == 2)
      return "video";
    else if (this._itemType.intValue == 4)
      return "image";
    else
      return null;
  },


  /**
   * \brief Update the progress UI for the busy state.
   */

  _updateProgressBusy: function DPW__updateProgressBusy() {
    var localeKey, subLocaleKey;

    // Get the device status.
    var deviceStatus = this._device.currentStatus;
    var operation = deviceStatus.currentState;
    var substate = deviceStatus.currentSubState;
    var curItemIndex = this._curItemIndex.intValue;
    var totalItems = this._totalItems.intValue;
    var duration = deviceStatus.elapsedTime;

    // Get the operation info.
    var operationInfo = this._getOperationInfo(operation);
    var operationLocaleSuffix = operationInfo.localeSuffix;

    // Show progress meter if it was previously hidden
    this._progressMeter.hidden = false;
    this._subProgressTextLabel.hidden = false;

    if (!this._startTimestamp) {
      this._startTimestamp = new Date();
      this._lastTimeLeft = Infinity;
    }

    if (deviceStatus.isNewBatch)
    {
//      dump("==> New Batch!\n");
      deviceStatus.isNewBatch = false;
    }

    var eta, etaString;

    // Update the operation progress meter.
    if (operationInfo.progressMeterDetermined) {
      // Compute the progress from 0-1.
      var progress = 0;
      if ((totalItems > 0) &&
          (curItemIndex > 0) &&
          (curItemIndex <= totalItems))
      {
        progress = (curItemIndex - 1) / totalItems;
      }

      // We are often incorrectly told that 100% of the zeroth file
      // has been completed. This will be fixed by bug 20363.
      if (curItemIndex != 0) {
        let increase = (this._itemProgress.intValue / 100) / totalItems;

        // Some device has two phases for transcoding (MTP): transcode to
        // temporary file, then copy to the device.
        if (!this._device.isDirectTranscoding) {
          // Calculate the transcoding and copying progress based on ratio.
          //
          // Make sure the progress bar moves forwards in a continuous way
          // for transcoding/copying.
          if (operation == Ci.sbIDevice.STATE_TRANSCODE ||
              substate == Ci.sbIDevice.STATE_TRANSCODE) {
            if (this._itemProgress.intValue == 0) {
              this._lastIsTranscoding = true;
              this._firstCopying = true;

              // Calculate the ratio based on the processing time of
              // previous items.
              if (curItemIndex > 1) {
                this._copyingEnd = duration;
                let transcodingDuration =
                  this._transcodingEnd - this._transcodingStart;
                let copyingDuration =
                  this._copyingEnd - this._copyingStart;

                // Replace the saved ratio with the first calculated one.
                if (curItemIndex == 2) {
                  this._ratio = transcodingDuration / copyingDuration + 0.5;
                  if (!this._ratio) this._ratio = 1;
                }
                // Calculate the avarage ratio.
                else {
                  this._ratio = this._ratio * (curItemIndex - 2) + 0.5 +
                                  transcodingDuration / copyingDuration;
                  this._ratio /= (curItemIndex - 1);
                  if (!this._ratio) this._ratio = 1;
                }
              }
              this._transcodingStart = duration;
            }

            let divisor = totalItems * (1 / this._ratio + 1);
            increase = (this._itemProgress.intValue / 100) / divisor;
          }
          else if (this._lastIsTranscoding &&
                   (operation == Ci.sbIDevice.STATE_COPYING ||
                    substate == Ci.sbIDevice.STATE_COPYING)) {
            if (this._itemProgress.intValue == 0) {
              this._copyingStart = duration;
              this._transcodingEnd = duration;
            }

            let divisor = totalItems * (this._ratio + 1);

            // XXX Hack: the item progress of first copying after transcoding
            //           is still the one from transcoding. Due to asynchronous
            //           update?
            //           Ignore the first copying so that the progress bar works
            if (this._firstCopying) {
              this._firstCopying = false;
              increase = this._ratio / divisor;
            }
            else {
              increase =
                (this._itemProgress.intValue / 100 + this._ratio) / divisor;
            }
          }
        }

        progress += increase;
      }

      // Show the time remaining detail starting from 5%. The value could
      // change dramatically before 5%.
      if (progress > 0.05)
      {
        if (progress > this._lastProgress)
          this._lastProgress = progress;
        else
          progress = this._lastProgress;

        eta = (duration / progress) - duration;
        etaString = TimeFormatter.formatHMS(eta / 1000);
      }
      else
        this._lastProgress = progress;

      // Set the progress meter to determined.
      this._progressMeter.setAttribute("mode", "determined");
      // convert from 0-1 to 0-100, and round
      this._progressMeter.value = Math.round(progress * 100);
    } else if (operationInfo.progressMeterUndetermined) {
      this._progressMeter.setAttribute("mode", "undetermined");
      this._progressMeter.value = 0;
    } else {
      this._progressMeter.setAttribute("mode", "determined");
      this._progressMeter.value = 0;
    }

//     var now = new Date().valueOf();
//     dump("==> "+ now +": "+
//          operationInfo.localeSuffix +"+"+
//          this._getOperationInfo(substate).localeSuffix +" "+
//          curItemIndex +"/"+ totalItems +".  "+
//          this._itemProgress.intValue +"% of this item, "+
//          Math.round(progress * 10000) / 100 +"% in total.  "+
//          duration +"ms spent so far, "+
//          Math.round(eta) +"ms left until "+
//          (new Date(now + eta)).valueOf() +"\n");

    var itemType = this._getItemType();
    // Determine if this is a playlist
    var isPlaylist = deviceStatus.mediaItem instanceof Ci.sbIMediaList;
    
    // If we're preparing to sync (indicated by an idle sub)
    // then show the preparing label
    if (operationInfo.preparingOnIdle && (substate == Ci.sbIDevice.STATE_IDLE))
    {
      localeKey = "device.status.progress_header_" + operationLocaleSuffix;
      if (itemType)
        localeKey = localeKey + "." + itemType;
      subLocaleKey = "device.status.progress_preparing_" + operationLocaleSuffix;
    } else if (operation == Ci.sbIDevice.STATE_TRANSCODE ||
               substate == Ci.sbIDevice.STATE_TRANSCODE) {
      localeKey = "device.status.progress_header_transcoding";
      if (itemType)
        localeKey = localeKey + "." + itemType;
    } else if (operation == Ci.sbIDevice.STATE_SYNCING &&
               substate == Ci.sbIDevice.STATE_DELETING) {
      // we don't want to show deleting playlists counts during sync
      if (!isPlaylist) {
        localeKey = "device.status.progress_header_deleting";
      }
      else {
        localeKey = "device.status.progress_header_syncing";
        subLocaleKey = "device.status.progress_footer_syncing_finishing";
      }
    } else if (operation == Ci.sbIDevice.STATE_SYNCING &&
               substate == Ci.sbIDevice.STATE_SYNC_PLAYLIST) {
      localeKey = "device.status.progress_header_" + operationLocaleSuffix;
      subLocaleKey = "device.status.progress_footer_syncing_finishing";
      this._progressMeter.setAttribute("mode", "undetermined");
    } else if (operation == Ci.sbIDevice.STATE_COPYING ||
               substate == Ci.sbIDevice.STATE_COPYING) {
      localeKey = "device.status.progress_header_copying";
      if (itemType)
        localeKey = localeKey + "." + itemType;
    } else {
      localeKey = "device.status.progress_header_" + operationLocaleSuffix;
    }

    if (!subLocaleKey) {
        subLocaleKey = etaString ? "device.status.progress_detail"
                                 : "device.status.progress_item_index";
    }

    // Update the operation progress text.
    this._progressTextLabel.value = SBString(localeKey, "");
    if (curItemIndex > 0 && curItemIndex <= totalItems &&
        operation != Ci.sbIDevice.STATE_MOUNTING &&
        substate != Ci.sbIDevice.STATE_SYNCING &&
        substate != Ci.sbIDevice.STATE_COPY_PREPARING &&
        substate != Ci.sbIDevice.STATE_UPDATING &&
        substate != Ci.sbIDevice.STATE_SYNCING_TYPE) {
      let params = [curItemIndex, totalItems, etaString];
      this._subProgressTextLabel.value =
             SBFormattedString(subLocaleKey, params, "");
    } else {
      this._subProgressTextLabel.value = null;
    }
  },

  //----------------------------------------------------------------------------
  //
  // Device progress event handler services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Dispatch sync settings change event.
   */

  onModeChange: function DeviceSyncWidget_onModeChange() {
    if (this._deviceLibrary.syncMode == Ci.sbIDeviceLibrary.SYNC_MANUAL) {
      // Update the UI in case the Apply/Cancel buttons are still there.
      DPW._syncSettingsChanged = false;
      DPW._device.cacheSyncRequests = false;
      DPW._update();
    }
    else
      this._dispatchSettingsEvent(this.SYNCSETTINGS_CHANGE);
  },

  /**
   * \brief Handle the event specified by aEvent for elements with defined
   *        actions.
   *
   * \param aEvent              Event to handle.
   */

  onAction: function DPW_onAction(aEvent) {
    // Dispatch processing of action.
    switch (aEvent.target.getAttribute("action")) {
      case "finish" :
        this._finish();
        break;

      case "settings-apply":
        this._dispatchSettingsEvent(this.SYNCSETTINGS_APPLY);
        this._device.syncLibraries();
        break;

      case "settings-cancel":
        this._dispatchSettingsEvent(this.SYNCSETTINGS_CANCEL);
        break;

      default :
        break;
    }
  },


  /**
   * \brief Handle the button event specified by aEvent and dispatch
   *        sbDeviceProgress-ok-button-enter
   *
   * \param aEvent              Event to handle.
   */

  _onButtonEvent: function DPW__onButtonEvent(aEvent) {
    //Create a new event for mouse click and key press
    if ((aEvent.type == "click" && aEvent.button == 0) ||
        (aEvent.type == "keypress" &&
         (aEvent.keyCode == KeyEvent.DOM_VK_ENTER ||
          aEvent.keyCode == KeyEvent.DOM_VK_RETURN))) {
      var newEvent = document.createEvent("Events");
      newEvent.initEvent("sbDeviceProgress-ok-button-enter", false, true);
      document.dispatchEvent(newEvent);
    }
  },


  /**
   * \brief Handles the sbDeviceSync-settings and updates sync settings UI
   *        accordingly
   *
   * \param aEvent              Event to handle.
   */

  _onSettingsEvent: function DPW__onSettingsEvent(aEvent) {
    switch (aEvent.detail) {
      case DPW.SYNCSETTINGS_CHANGE:
        DPW._syncSettingsChanged = true;
        DPW._device.cacheSyncRequests = true;
        break;

      case DPW.SYNCSETTINGS_APPLY:
        DPW._syncSettingsChanged = false;
        DPW._device.cacheSyncRequests = false;
        break;

      case DPW.SYNCSETTINGS_CANCEL:
        DPW._syncSettingsChanged = false;
        DPW._device.cacheSyncRequests = false;

        // Reset back the syncmode if there is pending sync mode change.
        if (DPW._deviceLibrary.syncModeChanged) {
          DPW._deviceLibrary.syncMode = Ci.sbIDeviceLibrary.SYNC_MANUAL;
        }
        break;
    }

    // Click apply button will trigger sync, which will do the update later.
    if (aEvent.detail != DPW.SYNCSETTINGS_APPLY)
      DPW._update();
  },


  /**
   * \brief Handle the device state changed event specified by aEvent.
   *
   * \param aEvent              Device state changed event.
   *
   *   This function tracks the sequence of device states in order to properly
   * update the UI.  Since the device state changed events are queued, the
   * current device state may not be the same as the state received in a state
   * changed event.  Without tracking all of the device state changed events,
   * some device states and operations may be missed, resulting in incorrect UI
   * updates.
   */

  _handleStateChanged: function DPW__handleStateChanged(aEvent) {
    // Update the last event operation.
    var prevLastEventOperation = this._lastEventOperation;
    this._lastEventOperation = aEvent.data;

    // Get the last and previous last event operation info.
    var lastEventOperationInfo = this._getOperationInfo
                                        (this._lastEventOperation);
    var prevLastEventOperationInfo = this._getOperationInfo
                                            (prevLastEventOperation);

    // Set to show progress if busy.
    if (lastEventOperationInfo.showProgress)
      this._showProgress = true;

    // Remember canceled operations.
    if ("operationCanceled" in lastEventOperationInfo)
      this._operationCanceled = lastEventOperationInfo.operationCanceled;

    // Update the last completed event operation.
    if (prevLastEventOperationInfo.canBeCompleted)
      this._lastCompletedEventOperation = prevLastEventOperation;

    // Update the widget.
    this._update();
  },


  //----------------------------------------------------------------------------
  //
  // Device progress XUL services.
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

  _getElement: function DIW__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  },


  //----------------------------------------------------------------------------
  //
  // Internal device info services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Cleans up the progress widget on the summary page to an IDLE state.
   */

  _finish: function DPW__finish() {
    // Set to no longer show progress.
    this._showProgress = false;

    // Update the UI.
    this._update();
  },


  /**
   * \brief Notifies listener about a settings apply/cancel action.
   * 
   * \param detail              One of the SYNCSETTINGS_* constants
   */

  _dispatchSettingsEvent: function DPW__dispatchSettingsEvent(detail) {
    let event = document.createEvent("UIEvents");
    event.initUIEvent("sbDeviceSync-settings", false, false, window, detail);
    document.dispatchEvent(event);
  },

  /**
   * \brief Get and return the operation information record for the device
   *        operation specified by aOperation.
   *
   * \param aOperation          Device operation.
   *
   * \return                    Device operation information record.
   */

  _getOperationInfo: function DPW__getOperationInfo(aOperation) {
    var operationInfo = this._operationInfoTable[aOperation];
    if (!operationInfo)
      operationInfo = {};
    return operationInfo;
  },


  //----------------------------------------------------------------------------
  //
  // Device progress device services.
  //
  //   These services provide an interface to the device object.
  //   These services register for any pertinent device notifications and call
  // _update to update the UI accordingly.  In particular, these services
  // register to receive notifications when the device state changes.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Initialize the device services.
   */

  _deviceInitialize: function DPW__deviceInitialize() {
    // Add a listener for status operations
    if (this._device) {
      var deviceEventTarget = this._device;
      deviceEventTarget.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.addEventListener(this);
    }

    if (this._deviceLibrary) {
      // Update the progress UI to apply/cancel for pending sync mode change
      var prefs = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefBranch);
      var syncModeChangedKey = "songbird.device." + this._widget.deviceID +
                               ".syncmode.changed";

      if (prefs.prefHasUserValue(syncModeChangedKey) &&
          prefs.getBoolPref(syncModeChangedKey)) {
        prefs.clearUserPref(syncModeChangedKey);
        this._dispatchSettingsEvent(this.SYNCSETTINGS_CHANGE);
      }
    }
  },

  /**
   * \brief Finalize the device services.
   */

  _deviceFinalize: function DPW__deviceFinalize() {
    // Clear object fields.
    if (this._device) {
      // Make sure we turn off the cacheSyncRequests if leaving.
      this._device.cacheSyncRequests = false;

      var deviceEventTarget = this._device;
      deviceEventTarget.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }
    this._device = null;
  },

  /**
   * \brief Gets a pref value (string) or returns a default if not found.
   *
   * \param aPrefID id of preference to get.
   * \param aDefaultValue default value to retun in case of error.
   *
   * \return string value of preference or aDefaultValue if error occurs.
   */

  _getPrefValue : function DPW__getPrefValue(aPrefID, aDefaultValue) {
    var prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);
    try {
      return prefService.getCharPref(aPrefID);
    } catch (err) {
      return aDefaultValue;
    }
  },

  /**
   * \brief Listener for device events.
   *
   * \sa sbIDeviceEvent.idl
   */

   onDeviceEvent : function DPW_onDeviceEvent(aEvent) {
    switch (aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        var state = this._device.state;
        var substate = this._device.currentStatus.currentSubState;
        this._handleStateChanged(aEvent);
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_PROGRESS:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSFER_END:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_PROGRESS:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_MOUNTING_END:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_PREFS_CHANGED:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSCODE_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSCODE_PROGRESS:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_TRANSCODE_END:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_FORMATTING_START:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_FORMATTING_PROGRESS:
      case Ci.sbIDeviceEvent.EVENT_DEVICE_FORMATTING_END:
        this._update();
        break;
    }
  },


  /**
   * \brief Return the device status data remote prefix.
   *
   * \return Device status data remote prefix.
   */

  _deviceGetStatusDRPrefix: function DPW__deviceGetStatusDRPrefix() {
    return "device." + this._deviceID + ".status";
  },


  /**
   * \brief Return true if device is idle.
   *
   * \return True if device is idle.
   */

  _deviceIsIdle: function DPW__deviceIsIdle() {
    return this._isIdle;
  },

  /**
   * \brief Return true if device is syncing
   */
  _deviceIsSyncing: function DPW__deviceIsSyncing() {
    return this._isSyncing;
  },
  
  /**
   * \brief Returns true if the device has errors for the media type
   * \param aMediaType the media type to check errors for, or if it's an
   *                   an empty string it returns true if any media type has
   *                   has errors.
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
};
