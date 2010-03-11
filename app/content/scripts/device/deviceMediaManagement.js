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
    // Get the media management widget.
    this._widget = aWidget;
    
    // Initialize object fields.
    this._device = this._widget.device;

    // Initialize, read, and apply the music preferences.
    this.musicManagementPrefsInitialize();
    this.resetPreferences();
  },

  /**
   * \brief Finalize the device progress services.
   */

  finalize: function DeviceMediaManagementServices_finalize() {
    this.musicManagementPrefsFinalize();

    // Clear object fields.
    this._widget = null;
    this._device = null;
  },

  savePreferences: function DeviceMediaManagementServices_savePreferences() {
    // Extract preferences from UI and write to storage
    this.musicManagementPrefsExtract();
    this.musicManagementPrefsWrite();

    // Now re-read from the newly-stored preference to ensure we don't get out
    // of sync
    this.resetPreferences();
  },

  resetPreferences: function DeviceMediaManagementServices_resetPreferences() {
    // Re-read from preferences storage and apply
    this.musicManagementPrefsRead();
    this.musicManagementPrefsApply();
  },

  onUIPrefChange: function DeviceMediaManagementServices_onUIPrefChange() {
    /* Extract preferences from UI and apply */
    this.musicManagementPrefsExtract();
    this.musicManagementPrefsApply();
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
   * _mediaManagementPrefs                  Working media management prefs.
   * _storedMediaManagementPrefs            Stored media management prefs.
   *
   * The current working set of music preferences are maintained in
   * _mediaManagementPrefs.
   * These are the preferences that the user has set and that is represented by
   * the UI but has not neccessarily been written to the preference storage.
   * The set of music preferences in the preference storage is maintained in
   * _storedMediaManagementPrefs.  This is used to determine whether any 
   * preferences have been changed and need to be written to the preference
   * storage.
   */
  
  _mediaManagementPrefs : null,
  _storedMediaManagementPrefs : null,


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
   * \breif This function finalizes the music preference services.
   */

  musicManagementPrefsFinalize:
      function DeviceMediaManagementServices_musicManagementPrefsFinalize()
  {
      /* Clear object fields. */
      this._mediaManagementPrefs = null;
      this._storedMediaManagementPrefs = null;
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
      this._mediaManagementPrefs = {};
      this._mediaManagementPrefs.transcodeModeManual = false;
      this._mediaManagementPrefs.selectedProfile = null;
      this._mediaManagementPrefs.selectedBitrate = null;

      this._mediaManagementPrefs.transcodeProfiles = [];

      var transcodeManager =
          Cc["@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1"]
            .getService(Ci.sbITranscodeManager);

      var profiles = transcodeManager.getTranscodeProfiles();
      var deviceCaps = this._device.capabilities;
      var formatMimeTypes = deviceCaps.getSupportedMimeTypes(
              Ci.sbIDeviceCapabilities.CONTENT_AUDIO, {});

      for (formatIndex in formatMimeTypes) {
        var format = deviceCaps.
          getFormatType(Ci.sbIDeviceCapabilities.CONTENT_AUDIO,
                        formatMimeTypes[formatIndex]);
        var container = format.containerFormat;
        var audioCodec = format.audioCodec;

        for (var i = 0; i < profiles.length; i++) {
          var profile = profiles.queryElementAt(i, Ci.sbITranscodeProfile);

          if (profile.type == Ci.sbITranscodeProfile.TRANSCODE_TYPE_AUDIO &&
              profile.containerFormat == container &&
              profile.audioCodec == audioCodec)
          {
            this._mediaManagementPrefs.transcodeProfiles.push(profile);
          }
        }
      }
  },


  /*
   * musicManagementPrefsInitUI
   *
   * \brief This function initializes the music management preference UI
   * elements to match the set of working music preference fields.
   * It does not apply the preference values to the UI.
   */

  musicManagementPrefsInitUI:
      function DeviceMediaManagementServices_musicManagementPrefsInitUI()
  {
      /* Get the transcoding format menu list */
      var profilesMenuList = this._getElement("encoding-format-list");
      var profiles = this._mediaManagementPrefs.transcodeProfiles;
  
      /* Clear the menu list */
      while (profilesMenuList.firstChild)
          profilesMenuList.removeChild(profilesMenuList.firstChild);
  
      /* Fill in the menu list with each available profile. */
      for (var i = 0; i < profiles.length; i++)
      {
        var profile = profiles[i];
        var readableName = profile.description;
        var menuItem = document.createElementNS(XUL_NS, "menuitem");
        menuItem.value = profile.id;
        menuItem.setAttribute("label", readableName);
 
        /* Add the menu item to the list. */
        profilesMenuList.appendChild(menuItem);
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

    if (profileId) {
      this._mediaManagementPrefs.transcodeModeManual = true;
      this._mediaManagementPrefs.selectedProfile =
          this.profileFromProfileId(profileId);
      this._mediaManagementPrefs.selectedBitrate = 
          this._device.getPreference("transcode_profile.audio_properties.bitrate");
    }
    else {
      this._mediaManagementPrefs.transcodeModeManual = false;
      this._mediaManagementPrefs.selectedProfile = null;
      this._mediaManagementPrefs.selectedBitrate = null;
    }

      /* Make a copy of the stored music prefs. */
      this._storedMediaManagementPrefs = {};
      this._storedMediaManagementPrefs.transcodeModeManual =
          this._mediaManagementPrefs.transcodeModeManual;
      this._storedMediaManagementPrefs.selectedProfile =
          this._mediaManagementPrefs.selectedProfile;
      this._storedMediaManagementPrefs.selectedBitrate =
          this._mediaManagementPrefs.selectedBitrate;
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
    if (this._mediaManagementPrefs.transcodeModeManual) {
      var profileId = this._mediaManagementPrefs.selectedProfile.id;
      this._device.setPreference("transcode_profile.profile_id", profileId);
      this._device.setPreference("transcode_profile.audio_properties.bitrate",
              this._mediaManagementPrefs.selectedBitrate);
    }
    else {
      this._device.setPreference("transcode_profile.profile_id", "");
      this._device.setPreference("transcode_profile.audio_properties.bitrate", "");
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
    var modeRadioGroup = this._getElement("transcoding_mode_radio_group");

    if (this._mediaManagementPrefs.transcodeModeManual)
      this._selectRadio("transcoding-mode-manual");
    else {
      this._selectRadio("transcoding-mode-auto");
    }

    var modeDetailsBox = this._getElement("transcoding-mode-details");
    for (var i = 0; i < modeDetailsBox.childNodes.length; i++) {
      var curChildNode = modeDetailsBox.childNodes.item(i);
      if (this._mediaManagementPrefs.transcodeModeManual) {
        curChildNode.removeAttribute("disabled");
      }
      else {
        curChildNode.setAttribute("disabled", "true");
      }
    }
    var menulist = this._getElement("encoding-format-menu");
    if (this._mediaManagementPrefs.transcodeModeManual)
      menulist.removeAttribute("disabled");
    else
      menulist.setAttribute("disabled", "true");

    var activeProfile;
    if (this._mediaManagementPrefs.selectedProfile) {
      activeProfile = this._mediaManagementPrefs.selectedProfile;
    }
    else {
      // Figure out what the default profile will be - highest priority of the
      // ones available.
      var highestPrio = 0;
      var defaultProfile = null;
      
      var profiles = this._mediaManagementPrefs.transcodeProfiles;
      for (var i = 0; i < profiles.length; i++) {
        var profile = profiles[i];
        if (!defaultProfile || profile.priority > highestPrio) {
          defaultProfile = profile;
          highestPrio = profile.priority;
        }
      }
      activeProfile = defaultProfile;
    }

    // Now find the menu item with a value the same as the profile id, and
    // select it.
    var formatList = this._getElement("encoding-format-list");
    for (var i = 0; i < formatList.childNodes.length; i++) {
      var formatMenuItem = formatList.childNodes.item(i);

      if (formatMenuItem.value == activeProfile.id) {
        formatMenuItem.setAttribute("selected", "true");
      }
      else {
        formatMenuItem.setAttribute("selected", "false");
      }
    }

    // Does the active profile have a bitrate property?
    var foundBitrate = false;
    if (activeProfile) {
      var propertiesArray = activeProfile.audioProperties;
      if (propertiesArray) {
        for (var i = 0; i < propertiesArray.length; i++) {
          var property = propertiesArray.queryElementAt(i,
                  Ci.sbITranscodeProfileProperty);
          if (property.propertyName == "bitrate") {
            foundBitrate = true;
            var bitrateEntry = this._getElement("transcoding-bitrate-kbps");
            bitrateEntry.min = parseInt(property.valueMin) / 1000;
            bitrateEntry.max = parseInt(property.valueMax) / 1000;
            if (this._mediaManagementPrefs.selectedBitrate)
              bitrateEntry.value = parseInt(
                      this._mediaManagementPrefs.selectedBitrate) / 1000;
            else
              bitrateEntry.value = parseInt(property.value) / 1000;
          }
        }
      }
    }
    var bitrateBox = this._getElement("transcoding-bitrate");
    if (foundBitrate) {
      bitrateBox.removeAttribute("hidden");
    }
    else {
      bitrateBox.setAttribute("hidden", "true");
    }
  },

  /*
   * musicManagementPrefsExtract
   *
   * \brief This function extracts the preference settings from the preference
   *        UI and writes them to the working preference set.
   */

  musicManagementPrefsExtract:
      function DeviceMediaManagementServices_musicManagementPrefsExtract()
  {
    this._mediaManagementPrefs.transcodeModeManual =
        this._getElement("transcoding-mode-manual").selected;
    
    if (this._mediaManagementPrefs.transcodeModeManual) {
      var bitrateEntry = this._getElement("transcoding-bitrate-kbps");
      this._mediaManagementPrefs.selectedBitrate = "" +
          parseInt(bitrateEntry.value) * 1000;
      this._mediaManagementPrefs.selectedProfile =
          this.profileFromProfileId(
                  this._getElement("encoding-format-menu").selectedItem.value);
    }
    else {
      this._mediaManagementPrefs.selectedBitrate = null;
      this._mediaManagementPrefs.selectedProfile = null;
    }
  },
  
  profileFromProfileId:
      function DeviceMediaManagementServices_profileFromProfileId(aId)
  {
    var profiles = this._mediaManagementPrefs.transcodeProfiles;
    for (var i = 0; i < profiles.length; i++) {
      var profile = profiles[i];
      if (profile.id == aId)
        return profile;
    }
    return null;
  }
};
