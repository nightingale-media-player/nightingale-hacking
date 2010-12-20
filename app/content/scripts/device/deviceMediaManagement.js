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
* \file  deviceMediaManagement.js
* \brief Javascript source for the device media management widget.
*/

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Device media management widget.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//
// Device media management defs.
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

if (typeof(XUL_NS) == "undefined")
  var XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var DeviceMediaManagementServices = {
  //
  // Device media management object fields.
  //
  //   _widget                  Device media management widget.
  //   _device                  Device we are working with.
  //

  _widget: null,
  _device: null,

  /**
   * \brief Initialize the device progress services for the device progress
   *        widget specified by aWidget.
   *
   * \param aWidget             Device progress widget.
   */

  initialize: function DeviceMediaManagementServices__initialize(aWidget) {
    if (!aWidget.device)
      return;

    // Get the media management widget.
    this._widget = aWidget;

    // Initialize object fields.
    this._device = this._widget.device;

    // Listen for device events.
    var deviceEventTarget =
          this._device.QueryInterface(Ci.sbIDeviceEventTarget);
    deviceEventTarget.addEventListener(this);

    // Initialize, read, and apply the music preferences.
    this.musicManagementPrefsInitialize();
    this._widget.reset();
  },

  /**
   * \brief Finalize the device progress services.
   */

  finalize: function DeviceMediaManagementServices_finalize() {
    this.musicManagementPrefsFinalize();

    // Stop listening for device events.
    if (this._device) {
      var deviceEventTarget =
            this._device.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }

    // Clear object fields.
    this._widget = null;
    this._device = null;
  },

  savePreferences: function DeviceMediaManagementServices_savePreferences() {
    if (!this._device)
      return;

    // Extract preferences from UI and write to storage
    this._transcodeModeManual =
        this._getElement("transcoding-mode-manual").selected;
    this.musicManagementPrefsWrite();

    // Now re-read from the newly-stored preference to ensure we don't get out
    // of sync
    this.resetPreferences();
  },

  resetPreferences: function DeviceMediaManagementServices_resetPreferences() {
    if (!this._device)
      return;

    // Re-read from preferences storage and apply
    this.musicManagementPrefsRead();
    this.musicManagementPrefsApply();
  },

  _updateNotchDisabledState:
    function DeviceMediaManagementServices__updateNotchDisabledState() {
    var notchHbox = this._getElement("notch-hbox");
    var spaceLimitScale = this._getElement("space_limit");
    var zeroPercentDescription = this._getElement("notch-0percent-description");
    var hundredPercentDescription =
          this._getElement("notch-100percent-description");

    // Enable/disable the notch boxes.
    for (var i = 0; i < notchHbox.childNodes.length; i++) {
      var curChildNode = notchHbox.childNodes.item(i);
      if (spaceLimitScale.disabled) {
        curChildNode.setAttribute("disabled", "true");
      }
      else {
        curChildNode.removeAttribute("disabled");
      }
    }

    // Enable/disable the percentage labels
    if (spaceLimitScale.disabled) {
      zeroPercentDescription.setAttribute("disabled", "true");
      hundredPercentDescription.setAttribute("disabled", "true");
    }
    else {
      zeroPercentDescription.removeAttribute("disabled");
      hundredPercentDescription.removeAttribute("disabled");
    }
  },

  /**
   * \brief Update the busy state of the UI.
   */
  _updateBusyState: function DeviceMediaManagementServices__updateBusyState() {
    var isBusy = this._device && this._device.isBusy;

    if (this._transcodeModeManual && !isBusy) {
      this._transcodeSettings.disabled = false;
    }
    else {
      this._transcodeSettings.disabled = true;
    }

    var spaceLimitToggle = this._getElement("space_limit_enable");
    var spaceLimitScale = this._getElement("space_limit");
    if (!isBusy && spaceLimitToggle.checked) {
      spaceLimitScale.removeAttribute("disabled");
    }
    else {
      spaceLimitScale.setAttribute("disabled", "true");
    }

    this._updateNotchDisabledState();

    var mgmtDirFormat = this._getElement("mgmt_dir_format");
    var mgmtToggle = this._getElement("mgmt_enable");
    mgmtDirFormat.disableAll = !mgmtToggle.checked || isBusy;
  },

  onUIPrefChange: function DeviceMediaManagementServices_onUIPrefChange() {
    /* Extract preferences from UI and apply */
    this._transcodeModeManual =
        this._getElement("transcoding-mode-manual").selected;
    this.musicManagementPrefsApply();

    this.dispatchSettingsChangeEvent();
  },

  /**
   * \brief Notifies listener about a settings change actions.
   */

  dispatchSettingsChangeEvent:
    function DeviceMediaManagementServices_dispatchSettingsChangeEvent() {
    let event = document.createEvent("UIEvents");
    event.initUIEvent("settings-changed", false, false, window, null);
    document.dispatchEvent(event);
  },

  //----------------------------------------------------------------------------
  //
  // Device sync sbIDeviceEventListener services.
  //
  //----------------------------------------------------------------------------

  /**
   * \brief Handle the device event specified by aEvent.
   *
   * \param aEvent              Device event.
   */

  onDeviceEvent:
    function DeviceMediaManagementServices_onDeviceEvent(aEvent) {
    // Dispatch processing of the event.
    switch(aEvent.type)
    {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        this._updateBusyState();
        break;

      default :
        break;
    }
  },

  //----------------------------------------------------------------------------
  //
  // Device media management XUL services.
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

  _getElement: function DeviceMediaManagementServices__getElement(aElementID) {
    return document.getAnonymousElementByAttribute(this._widget,
                                                   "sbid",
                                                   aElementID);
  },

  /*
   * \brief selects the radio element specified by aRadioID.
   *
   * \param aRadioID               ID of radio to select.
   *
   */

  _selectRadio : function DeviceMediaManagementServices__selectRadio(aRadioID)
  {
      var radioElem;

      /* Get the radio element. */
      radioElem = this._getElement(aRadioID);

      /* Select the radio. */
      radioElem.radioGroup.selectedItem = radioElem;
  },

  /* *****************************************************************************
   *
   * device media management preference services.
   *
   ******************************************************************************/

  /*
   * _transcodeSettings                     Transcode settings for the device.
   *
   */

  _transcodeSettings : null,

  /*
   * musicManagementPrefsInitialize
   *
   * \brief This function initializes the music preference services.
   */

  musicManagementPrefsInitialize:
      function DeviceMediaManagementServices_musicManagementPrefsInitialize()
  {
      /* Initialize the working preference set. */
      this.musicManagementPrefsInitPrefs();

      /* Initialize the preference UI. */
      this.musicManagementPrefsInitUI();
  },


  /*
   * musicManagementPrefsFinalize
   *
   * \brief This function finalizes the music preference services.
   */

  musicManagementPrefsFinalize:
      function DeviceMediaManagementServices_musicManagementPrefsFinalize()
  {
      /* Clear object fields. */
      this._transcodeSettings = null;
  },


  /*
   * musicManagementPrefsInitPrefs
   *
   * \brief This function initializes the working music management preference
   * set.  It sets up the working music management preference object with all
   * of the preference fields and sets default values.  It does not read the
   * preferences from the preference storage.
   */

  musicManagementPrefsInitPrefs:
      function DeviceMediaManagementServices_musicManagementPrefsInitPrefs()
  {
      this._transcodeModeManual = false;

      this._transcodeSettings = this._getElement("transcode-settings");

      var transcodeManager =
          Cc["@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1"]
            .getService(Ci.sbITranscodeManager);

      var supportedProfiles = [];
      var profiles = transcodeManager.getTranscodeProfiles(
              Ci.sbITranscodeProfile.TRANSCODE_TYPE_AUDIO);
      var deviceCaps = this._device.capabilities;
      var formatMimeTypes = deviceCaps.getSupportedMimeTypes(
              Ci.sbIDeviceCapabilities.CONTENT_AUDIO, {});

      for (formatMimeTypeIndex in formatMimeTypes) {
        var formats = deviceCaps.
          getFormatTypes(Ci.sbIDeviceCapabilities.CONTENT_AUDIO,
                         formatMimeTypes[formatMimeTypeIndex], {});
        for (formatIndex in formats) {
          var format = formats[formatIndex];
          var container = format.containerFormat;
          var audioCodec = format.audioCodec;

          for (var i = 0; i < profiles.length; i++) {
            var profile = profiles.queryElementAt(i, Ci.sbITranscodeProfile);

            if (profile.type == Ci.sbITranscodeProfile.TRANSCODE_TYPE_AUDIO &&
                profile.containerFormat == container &&
                profile.audioCodec == audioCodec)
            {
              supportedProfiles.push(profile);
            }
          }
        }
      }

    this._transcodeSettings.profiles = supportedProfiles;
  },


  /*
   * musicManagementPrefsInitUI
   *
   * \brief This function initializes the music management preference UI
   * elements to match the set of working music preference fields.
   * It does not apply the preference values to the UI.
   */

  musicManagementPrefsInitUI:
    function DeviceMediaManagementServices_musicManagementPrefsInitUI() {
    // Get the available transcoding profiles.
    var profiles = this._transcodeSettings.profiles;

    // Show the transcoding preferences UI if any transcoding profiles are
    // available.  Otherwise, show the no encoders available description.
    var transcodingSettingsDeckElem =
          this._getElement("transcoding_settings_deck");
    if (profiles.length > 0) {
      transcodingSettingsDeckElem.selectedPanel =
        this._getElement("transcoding_preferences");
    }
    else {
      transcodingSettingsDeckElem.selectedPanel =
        this._getElement("no_encoders_available_description");
    }
  },


  /*
   * musicManagementPrefsRead
   *
   * \brief This function reads the media Management preferences from the
   *        preference storage into the working media management preferences
   *        object. It also updates the stored media Management prefs.
   */

  musicManagementPrefsRead:
      function DeviceMediaManagementServices_musicManagementPrefsRead(aPrefs)
  {
    var profileId = this._device.getPreference("transcode_profile.profile_id");

    // Only store the profile id if transcode is in manual mode.
    if (profileId) {
      this._transcodeModeManual = true;
      this._transcodeSettings.transcodeBitrate =
          this._device.getPreference(
              "transcode_profile.audio_properties.bitrate");
      this._transcodeSettings.transcodeProfileId = profileId;
    }
    // Otherwise, get the profile id from global one or pick the one
    // with highest priority.
    else {
      this._transcodeModeManual = false;

      // Get the global default profile if there's one of those.
      var gProfileId = Application.prefs.getValue(
              "songbird.device.transcode_profile.profile_id", null);
      if (gProfileId) {
        this._transcodeSettings.transcodeBitrate =
            Application.prefs.getValue(
                "songbird.device.transcode_profile.audio_properties.bitrate",
                null);
        this._transcodeSettings.transcodeProfileId = gProfileId;
      } else {
        this._transcodeSettings.transcodeProfile = null;
        this._transcodeSettings.transcodeBitrate = null;
      }
    }
  },


  /*
   * musicManagementPrefsWrite
   *
   * \brief This function writes the working music preferences to the preference
   * storage.
   */

  musicManagementPrefsWrite:
      function DeviceMediaManagementServices_musicManagementPrefsWrite()
  {
    if (this._transcodeModeManual) {
      this._device.setPreference("transcode_profile.profile_id",
                                 this._transcodeSettings.transcodeProfileId);
      this._device.setPreference("transcode_profile.audio_properties.bitrate",
                                 this._transcodeSettings.transcodeBitrate);
    } else {
      this._device.setPreference("transcode_profile.profile_id", "");
      // Resets transcode settings to default
      this._transcodeSettings.transcodeProfileId = "";
    }
  },


  /*
   * musicManagementPrefsApply
   *
   * \brief This function applies the working preference set to the preference UI.
   */

  musicManagementPrefsApply:
      function DeviceMediaManagementServices_musicManagementPrefsApply()
  {
    if (this._transcodeModeManual)
      this._selectRadio("transcoding-mode-manual");
    else {
      this._selectRadio("transcoding-mode-auto");
    }

    return this._updateBusyState();
  }
}
