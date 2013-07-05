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

const SB_NS = 'http://songbirdnest.com/data/1.0#';

Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/kPlaylistCommands.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbCDDeviceUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

//------------------------------------------------------------------------------
// XUL ID Constants

var CDRIP_BASE                      = "sb-cdrip-";
const CDRIP_HBOX_CLASS              = CDRIP_BASE + "header-hbox";
const CDRIP_HBOX_PLAIN_CLASS        = CDRIP_BASE + "header-plain-hbox";

const RIP_SETTINGS_HBOX             = CDRIP_BASE + "settings-hbox";
const RIP_SETTINGS_FORMAT_LABEL     = CDRIP_BASE + "format-label";
const RIP_SETTINGS_BITRATE_HBOX     = CDRIP_BASE + "bitrate-hbox";
const RIP_SETTINGS_QUALITY_LABEL    = CDRIP_BASE + "quality-label";
const RIP_SETTINGS_BUTTON           = CDRIP_BASE + "settings-button";

const RIP_SPACER                    = CDRIP_BASE + "spacer";

const RIP_STATUS_HBOX               = CDRIP_BASE + "actionstatus-hbox";

const RIP_STATUS_IMAGE_PRE_SPACER   = CDRIP_BASE + "statusimage-pre-spacer";
const RIP_STATUS_IMAGE_HBOX         = CDRIP_BASE + "actionstatus-image-box";
const RIP_STATUS_IMAGE              = CDRIP_BASE + "status-image";
const RIP_STATUS_IMAGE_POST_SPACER  = CDRIP_BASE + "statusimage-post-spacer";

const RIP_STATUS_LABEL_HBOX         = CDRIP_BASE + "actionstatus-label-box";
const RIP_STATUS_LABEL              = CDRIP_BASE + "status-label";

const RIP_STATUS_LOGO_PRE_SPACER    = CDRIP_BASE + "logo-pre-spacer";
const RIP_STATUS_LOGO_HBOX          = CDRIP_BASE + "actionstatus-logo-box";
const RIP_STATUS_LOGO               = CDRIP_BASE + "provider-logo";
const RIP_STATUS_LOGO_POST_SPACER   = CDRIP_BASE + "logo-post-spacer";

const RIP_STATUS_BUTTONS_PRE_SPACER = CDRIP_BASE + "buttons-pre-spacer";
const RIP_STATUS_BUTTON_HBOX        = CDRIP_BASE + "status-buttons";
const RIP_STATUS_RIP_CD_BUTTON      = CDRIP_BASE + "rip-cd-button";
const RIP_STATUS_STOP_RIP_BUTTON    = CDRIP_BASE + "stop-rip-button";
const RIP_STATUS_EJECT_CD_BUTTON    = CDRIP_BASE + "eject-cd-button";

const RIP_COMMAND_EJECT             = CDRIP_BASE + "eject-command";
const RIP_COMMAND_STARTRIP          = CDRIP_BASE + "startrip-command";
const RIP_COMMAND_STOPRIP           = CDRIP_BASE + "stoprip-command";

const CDRIP_PLAYLIST                = CDRIP_BASE + "playlist";

// Time to display lookup provider logo
const LOGO_DISPLAY_MIN_TIMER        = 5000;
// Delay before displaying lookup message
const LOOKUP_MESSAGE_DELAY_TIMER    = 1000;

//------------------------------------------------------------------------------
// Misc internal constants

const FINAL_TRANSCODE_STATUS_SUCCESS = 0;
const FINAL_TRANSCODE_STATUS_PARTIAL = 1;
const FINAL_TRANSCODE_STATUS_FAILED  = 2;
const FINAL_TRANSCODE_STATUS_ABORT   = 3;


//------------------------------------------------------------------------------
// CD rip view controller

window.cdripController =
{
    // The sbIMediaListView that this page is to display
  _mediaListView: null,
    // The playlist we bind our CD Tracks to.
  _playlist:      null,
    // The device we are working with
  _device:        null,
  _deviceID:      null,
  _deviceLibrary: null,

  // Closure for device manager events
  _onDeviceManagerEventClosure: null,

  // Transcoding pref branch
  _transcodePrefBranch: null,

  _supportedProfiles: null,

  _logoTimer: null,
  logoTimerEnabled: false,
  _lookupDelayTimer: null,

  _libraryListener: null,

  onLoad: function cdripController_onLoad() {
    // Add our device listener to listen for lookup notification events
    this._device = this._getDevice();

    // This will set this._deviceLibrary
    this._getDeviceLibrary();

    // Go back to previous page and return if device or device library are not
    // available.
    if (!this._device || !this._deviceLibrary) {
      var browser = SBGetBrowser();
      browser.getTabForDocument(document).backWithDefault();
      return;
    }

    // Update the header information
    this._updateHeaderView();
    this._toggleRipStatus(false);

    this._logoTimer = Cc["@mozilla.org/timer;1"]
                        .createInstance(Ci.nsITimer);
    this._lookupDelayTimer = Cc["@mozilla.org/timer;1"]
                               .createInstance(Ci.nsITimer);

    this._toggleLookupNotification(false);

    var eventTarget = this._device.QueryInterface(Ci.sbIDeviceEventTarget);
    eventTarget.addEventListener(this);

    var self = this;
    this._onDeviceManagerEventClosure =
      function cdripController__onDeviceManagerEventClosure(aEvent)
        { self._onDeviceManagerEvent(aEvent); };
    var deviceManagerSvc = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                             .getService(Ci.sbIDeviceManager2);
    deviceManagerSvc.addEventListener(this._onDeviceManagerEventClosure);

    // Disable player controls & load the playlist
    this._updateRipSettings();
    this._togglePlayerControls(true);
    this._loadPlaylist();

    var servicePaneNode =
      Cc["@songbirdnest.com/servicepane/device;1"]
        .getService(Ci.sbIDeviceServicePaneService)
        .getNodeForDevice(this._device);
    if (servicePaneNode) {
      document.title = servicePaneNode.displayName;
    }
    else {
      // failed to find the node, stick with the default node name
      document.title = SBString("cdrip.service.default.node_name");
    }

    // Listen to transcoding pref changes.
    this._transcodePrefBranch = Cc["@mozilla.org/preferences-service;1"]
                                  .getService(Ci.nsIPrefService)
                                  .getBranch("songbird.cdrip.transcode_profile.")
                                  .QueryInterface(Ci.nsIPrefBranch2);
    this._transcodePrefBranch.addObserver(this, "", false);

    // Now that we are all initialized we can act like a regular media page.
    window.mediaPage = new MediaPageImpl();

    // Now enable the buttons, they're disabled to prevent clicking on them
    // before all the device information has been wired up.
    this._enableCommand(RIP_COMMAND_EJECT);
    this._enableCommand(RIP_COMMAND_STARTRIP);
    this._enableCommand(RIP_COMMAND_STOPRIP);

    // Check if we need to preform a look up on this disc.
    this._checkIfNeedsLookup();
  },

  onUnload: function cdripController_onUnload() {
    // Cleanup the medialist listener
    this._mediaListView.mediaList.removeListener(this._libraryListener);
    this._libraryListener = null;

    if (this._onDeviceManagerEventClosure) {
      var deviceManagerSvc = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                               .getService(Ci.sbIDeviceManager2);
      deviceManagerSvc.removeEventListener(this._onDeviceManagerEventClosure);
    }

    this._togglePlayerControls(false);
    if (this._device) {
      var eventTarget = this._device.QueryInterface(Ci.sbIDeviceEventTarget);
      eventTarget.removeEventListener(this);
      this._device.removeEventListener(this);
    }

    this._transcodePrefBranch.removeObserver("", this, false);
    this._transcodePrefBranch = null;
  },

  onDeviceEvent: function cdripController_onDeviceEvent(aEvent) {
    switch (aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_READY:
        this._checkIfNeedsLookup();
        break;

      // START CD LOOKUP
      case Ci.sbICDDeviceEvent.EVENT_CDLOOKUP_INITIATED:
        this._toggleLookupNotification(true);
        break;

      // CD LOOKUP COMPLETE
      case Ci.sbICDDeviceEvent.EVENT_CDLOOKUP_COMPLETED:
        this._toggleLookupNotification(false);
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        var device = aEvent.origin.QueryInterface(Ci.sbIDevice);
        if (device.id.toString() == this._deviceID) {
          this._updateHeaderView();
        }
        break;

      case Ci.sbICDDeviceEvent.EVENT_CDRIP_COMPLETED:
        this._toggleRipStatus(true);
        break;

      default:
        break;
    }
  },

  observe: function cdripController_observe(subject, topic, data) {
    if (topic == 'timer-callback') {
      if (subject == this._logoTimer) {
        this.logoTimerEnabled = false;
        // Hide any lookup objects if lookup is complete
        if (this._device.state != Ci.sbICDDeviceEvent.STATE_LOOKINGUPCD)
          this._toggleLookupNotification(false);
      } else if (subject == this._lookupDelayTimer) {
        // Enable lookup image/label if still looking up
        if (this._device.state == Ci.sbICDDeviceEvent.STATE_LOOKINGUPCD) {
          this._toggleLookupLabel(true);
        }
      }
    } else {
      // Something changed in the rip transcoding prefs, update that UI now.
      this._updateRipSettings();
    }
  },

  showCDRipSettings: function cdripController_showCDRipSettings() {
    Cc["@mozilla.org/appshell/window-mediator;1"]
      .getService(Ci.nsIWindowMediator)
      .getMostRecentWindow("Songbird:Main")
      .SBOpenPreferences("paneCDRip");
  },

  _onDeviceManagerEvent:
    function cdripController__onDeviceManagerEvent(aEvent) {
    switch (aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED :
        // Go back to previous page if device removed.
        var device = aEvent.data.QueryInterface(Ci.sbIDevice);
        if (device.id.toString() == this._deviceID) {
          var browser = SBGetBrowser();
          browser.getTabForDocument(document).backWithDefault();
        }
        break;

      default:
        break;
    }
  },

  _toggleLookupNotification: function
                             cdripController_toggleLookupNotification(show) {
    if (show) {
      var manager = Cc["@songbirdnest.com/Songbird/MetadataLookup/manager;1"]
                      .getService(Ci.sbIMetadataLookupManager);
      try {
        var provider = manager.defaultProvider;
        var providerLogo = provider.logoURL;
        var providerName = provider.name;
        var providerURL = provider.infoURL;

        if (providerLogo) {
          this._showLookupVendorInfo(providerLogo, providerName, providerURL);

          // Arm the logo timer.
          this._logoTimer.init(this,
                               LOGO_DISPLAY_MIN_TIMER,
                               Ci.nsITimer.TYPE_ONE_SHOT);
          this.logoTimerEnabled = true;
        }
      }
      catch (e) {
        Cu.reportError(e);
      }

      // Arm the lookup delay timer now.
      this._lookupDelayTimer.init(this,
                                  LOOKUP_MESSAGE_DELAY_TIMER,
                                  Ci.nsITimer.TYPE_ONE_SHOT);

      // Disable visible buttons
      this._disableCommand(RIP_COMMAND_EJECT);
      this._disableCommand(RIP_COMMAND_STARTRIP);
      this._disableCommand(RIP_COMMAND_STOPRIP);
    }
    else {
      // If the logo timer isn't enabled, hide all of the lookup vendor
      // logo stuff.
      if (!this.logoTimerEnabled) {
        this._hideLookupVendorInfo();
      }

      this._toggleLookupLabel(false);

      // Reset the action buttons.
      this._showElement(RIP_STATUS_BUTTON_HBOX);
      this._hideElement(RIP_STATUS_BUTTONS_PRE_SPACER);
    }

    this._updateHeaderView();
  },

  _toggleLookupLabel: function cdripController_lookupLabel(show) {
    if (show) {
      this._showElement(RIP_STATUS_IMAGE_PRE_SPACER);
      this._showElement(RIP_STATUS_IMAGE_POST_SPACER);
      this._showElement(RIP_STATUS_IMAGE_HBOX);

      this._setImageSrc(RIP_STATUS_IMAGE,
                        "chrome://songbird/skin/base-elements/icon-loading-large.png");
      this._setLabelValue(RIP_STATUS_LABEL,
                          SBString("cdrip.mediaview.status.lookup"));

      this._showElement(RIP_STATUS_LABEL_HBOX);
      this._showElement(RIP_STATUS_BUTTONS_PRE_SPACER);
    }
    else {
      this._hideElement(RIP_STATUS_IMAGE_PRE_SPACER);
      this._hideElement(RIP_STATUS_IMAGE_POST_SPACER);
      this._hideElement(RIP_STATUS_IMAGE_HBOX);
      this._hideElement(RIP_STATUS_LABEL_HBOX);

      this._setImageSrc(RIP_STATUS_IMAGE, "");
      this._setLabelValue(RIP_STATUS_LABEL, "");

      this._hideElement(RIP_STATUS_BUTTONS_PRE_SPACER);
    }

    this._updateHeaderView();
  },

  _toggleRipStatus: function cdripController_toggleRipStatus(aShouldShow) {
    if (aShouldShow) {
      // Unhide the status elements.
      this._showElement(RIP_STATUS_IMAGE_PRE_SPACER);
      this._showElement(RIP_STATUS_IMAGE_POST_SPACER);
      this._showElement(RIP_STATUS_LABEL_HBOX);
      this._showElement(RIP_STATUS_IMAGE_HBOX);
      this._showElement(RIP_STATUS_BUTTONS_PRE_SPACER);

      // Update the status label and image based on the final
      // transcode job status.
      var status = this._getLastTranscodeStatus();
      var statusLabel = "";
      var statusImageSrc = "";
      switch (status) {
        case FINAL_TRANSCODE_STATUS_SUCCESS:
          statusLabel = "cdrip.mediaview.status.rip_success";
          statusImageSrc = "chrome://songbird/skin/device/icon-complete.png";
          break;

        case FINAL_TRANSCODE_STATUS_FAILED:
          statusLabel = "cdrip.mediaview.status.rip_failed";
          statusImageSrc = "chrome://songbird/skin/device/icon-warning.png";
          break;

        case FINAL_TRANSCODE_STATUS_PARTIAL:
          statusLabel = "cdrip.mediaview.status.rip_partial";
          statusImageSrc = "chrome://songbird/skin/device/icon-warning.png";
          break;

        case FINAL_TRANSCODE_STATUS_ABORT:
          statusLabel.Truncate();
          statusImageSrc.Truncate();
          break;

        default:
          Cu.reportError("Unhandled transcode state!");
      }

      this._setLabelValue(RIP_STATUS_LABEL, SBString(statusLabel));
      this._setImageSrc(RIP_STATUS_IMAGE, statusImageSrc);
    }
    else {
      this._hideElement(RIP_STATUS_LABEL_HBOX);
      this._hideElement(RIP_STATUS_IMAGE_HBOX);
      this._hideElement(RIP_STATUS_BUTTONS_PRE_SPACER);
      this._hideElement(RIP_STATUS_IMAGE_PRE_SPACER);
      this._hideElement(RIP_STATUS_IMAGE_POST_SPACER);
    }
  },

  _updateHeaderView: function cdripController_updateHeaderView() {
    switch (this._device.state) {
      // STATE_LOOKINGUPCD
      case Ci.sbICDDeviceEvent.STATE_LOOKINGUPCD:
        // Ignore this state since it is handled by the events in the
        // onDeviceEvent function.
        break;

      case Ci.sbIDevice.STATE_TRANSCODE:
        // Currently ripping
        this._showSettingsView();
        this._toggleRipStatus(false);

        // Display only the Stop Rip button.
        this._hideElement(RIP_STATUS_RIP_CD_BUTTON);
        this._showElement(RIP_STATUS_STOP_RIP_BUTTON);
        this._hideElement(RIP_STATUS_EJECT_CD_BUTTON);

        // Update commands:
        this._disableCommand(RIP_COMMAND_EJECT);
        this._disableCommand(RIP_COMMAND_STARTRIP);
        this._enableCommand(RIP_COMMAND_STOPRIP);
        break;

      default:
        // By default we assume IDLE
        this._showSettingsView();

        // Display only the Rip button and the eject button.
        this._showElement(RIP_STATUS_RIP_CD_BUTTON);
        this._hideElement(RIP_STATUS_STOP_RIP_BUTTON);
        this._showElement(RIP_STATUS_EJECT_CD_BUTTON);

        // Update commands:
        this._enableCommand(RIP_COMMAND_EJECT);
        this._enableCommand(RIP_COMMAND_STARTRIP);
        this._disableCommand(RIP_COMMAND_STOPRIP);
        break;
    }
  },

  _updateRipSettings: function cdripController_updateRipSettings() {
    var profileId = this._device.getPreference("transcode_profile.profile_id");
    var profile;
    var bitrate = 0;

    var profiles = this._findSupportedProfiles();

    if (profileId) {
      profile = this._profileFromProfileId(profileId, profiles);
      bitrate = parseInt(this._device.getPreference(
                  "transcode_profile.audio_properties.bitrate"));
    }

    var highestPriority = 0;
    if (!profile) {
      // Nothing set in preferences, or set value is not available. Determine the
      // default profile.
      for (var i = 0; i < profiles.length; i++) {
        var current = profiles[i];
        if (!profile || current.priority > highestPriority) {
          profile = current;
          highestPriority = profile.priority;
        }
      }
    }

    var hasBitrate = false;
    var propertiesArray = profile.audioProperties;
    if (propertiesArray) {
      for (var i = 0; i < propertiesArray.length; i++) {
        var prop = propertiesArray.queryElementAt(i,
                Ci.sbITranscodeProfileProperty);
        if (prop.propertyName == "bitrate") {
          hasBitrate = true;
          if (!bitrate) {
            bitrate = parseInt(prop.value);
          }
        }
      }
    }

    if (hasBitrate) {
      this._showElement(RIP_SETTINGS_BITRATE_HBOX);
      var bitrateLabel = document.getElementById(RIP_SETTINGS_QUALITY_LABEL);
      bitrateLabel.setAttribute("value", "" + (bitrate / 1000) + " kbps");
    }
    else {
      this._hideElement(RIP_SETTINGS_BITRATE_HBOX);
    }

    var formatLabel = document.getElementById(RIP_SETTINGS_FORMAT_LABEL);
    formatLabel.setAttribute("value", profile.description);
  },

  _findSupportedProfiles:
      function cdripController_findSupportedProfiles()
  {
    if (this._supportedProfiles)
      return this._supportedProfiles;

    this._supportedProfiles = [];

    var transcodeManager =
        Cc["@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1"]
          .getService(Ci.sbITranscodeManager);
    var profiles = transcodeManager.getTranscodeProfiles(
            Ci.sbITranscodeProfile.TRANSCODE_TYPE_AUDIO);

    for (var i = 0; i < profiles.length; i++) {
      var profile = profiles.queryElementAt(i, Ci.sbITranscodeProfile);

      if (profile.type == Ci.sbITranscodeProfile.TRANSCODE_TYPE_AUDIO)
      {
        this._supportedProfiles.push(profile);
      }
    }
    return this._supportedProfiles;
  },

  _profileFromProfileId:
      function cdripController_profileFromProfileId(aId, aProfiles)
  {
    for (var i = 0; i < aProfiles.length; i++) {
      var profile = aProfiles[i];
      if (profile.id == aId)
        return profile;
    }
    return null;
  },

  disableTags : ['sb-player-back-button', 'sb-player-playpause-button',
                 'sb-player-forward-button', 'sb-player-volume-slider',
                 'sb-player-shuffle-button', 'sb-player-repeat-button'],
  disableMenuControls : ['play', 'next', 'prev', 'shuf', 'repx',
                         'repa', 'rep1'],
  _togglePlayerControls: function
                         cdripController_togglePlayerControls(disabled) {

    // disable player controls in faceplate
    var mainWin = Cc["@mozilla.org/appshell/window-mediator;1"]
                     .getService(Ci.nsIWindowMediator)
                     .getMostRecentWindow("Songbird:Main");
    if (!mainWin) {
      // Shutdown sequence? Don't do anything
      return;
    }

    for (var i in this.disableTags) {
      var elements = mainWin.document.getElementsByTagName(this.disableTags[i]);
      for (var j=0; j<elements.length; j++) {
        if (disabled) {
          elements[j].setAttribute('disabled', 'true');
        } else {
          elements[j].removeAttribute('disabled');
        }
      }
    }

    // disable playlist play events & menu controls
    var pls = document.getElementById("sb-cdrip-playlist");
    var playMenuItem = mainWin.document.getElementById("menuitem_control_play");
    if (disabled) {
      pls.addEventListener("Play", this._onPlay, false);
      mainWin.addEventListener("keypress", this._onKeypress, true);
      for each (var i in this.disableMenuControls) {
        var menuItem = mainWin.document.getElementById("menuitem_control_" + i);
        menuItem.setAttribute("disabled", "true");
      }
    } else {
      pls.removeEventListener("Play", this._onPlay, false);
      mainWin.removeEventListener("keypress", this._onKeypress, true);
      playMenuItem.removeAttribute("disabled");
      for each (var i in this.disableMenuControls) {
        var menuItem = mainWin.document.getElementById("menuitem_control_" + i);
        menuItem.removeAttribute("disabled");
      }
    }
  },

  _onPlay: function(e) {
    e.stopPropagation();
    e.preventDefault();
  },

  _onKeypress: function(e) {
    // somebody else has focus
    if (document.commandDispatcher.focusedWindow != window ||
        (document.commandDispatcher.focusedElement &&
         document.commandDispatcher.focusedElement.id != "sb-playlist-tree"))
    {
      return true;
    }

    if (e.charCode != KeyboardEvent.DOM_VK_SPACE ||
        e.ctrlKey || e.altKey || e.metaKey || e.altGraphKey)
    {
      return true;
    }

    e.stopPropagation();
    e.preventDefault();
    return true;
  },

  _ejectDevice: function cdripController_ejectDevice() {
    if (this._device) {
      this._device.eject();
    }
  },

  _startRip: function cdripController_startRip() {
    if (this._device) {
      sbCDDeviceUtils.doCDRip(this._device);
    } else {
      Cu.reportError("No device defined to start rip on.");
    }
  },

  _stopRip: function cdripController_stopRip() {
    if (this._device) {
      this._device.cancelRequests();
    } else {
      Cu.reportError("No device defined to stop rip on.");
    }
  },

  _hideSettingsView: function cdripController_hideSettingsView() {
    // Hack: Prevent the header box from resizing by setting the |min-height|
    //       CSS value of the status hbox.
    var settingsHeight =
      document.getElementById(RIP_SETTINGS_HBOX).boxObject.height;

    this._hideElement(RIP_SPACER);
    this._hideElement(RIP_SETTINGS_HBOX);

    var ripStatusHbox = document.getElementById(RIP_STATUS_HBOX);
    ripStatusHbox.setAttribute("flex", "1");
    ripStatusHbox.setAttribute("class", CDRIP_HBOX_PLAIN_CLASS);
    ripStatusHbox.style.minHeight = settingsHeight + "px";

    this._isSettingsViewVisible = false;
  },

  _showSettingsView: function cdripController_showSettingsView() {
    this._showElement(RIP_SPACER);
    this._showElement(RIP_SETTINGS_HBOX);

    var ripStatusHbox = document.getElementById(RIP_STATUS_HBOX);
    ripStatusHbox.removeAttribute("flex");
    ripStatusHbox.setAttribute("class", CDRIP_HBOX_CLASS);

    this._isSettingsViewVisible = true;
  },

  _hideElement: function cdripController_hideElement(aElementId) {
    document.getElementById(aElementId).setAttribute("hidden", "true");
  },

  _showElement: function cdripController_showElement(aElementId) {
    document.getElementById(aElementId).removeAttribute("hidden");
  },

  _isElementHidden: function cdripController_isElementHidden(aElementId) {
    return document.getElementById(aElementId).hasAttribute("hidden");
  },

  _disableCommand: function cdripController_disableCommand(aElementId) {
    document.getElementById(aElementId).setAttribute("disabled", "true");
  },

  _enableCommand: function cdripController_enableCommand(aElementId) {
    document.getElementById(aElementId).removeAttribute("disabled");
  },

  _setLabelValue: function cdripController_setLabelValue(aElementId, aValue) {
    document.getElementById(aElementId).value = aValue;
  },

  _setImageSrc: function cdripController_setImageSrc(aElementId, aValue) {
    document.getElementById(aElementId).src = aValue;
  },

  _setToolTip: function cdripController_setToolTip(aElementId, aValue) {
    document.getElementById(aElementId).setAttribute('tooltiptext', aValue);
  },

  _showLookupVendorInfo: function
                              cdripController_showLookupLogo(aProviderLogoSrc,
                                                             aProviderName,
                                                             aProviderURL) {
    if (aProviderLogoSrc) {
      this._showElement(RIP_STATUS_LOGO_PRE_SPACER);
      this._showElement(RIP_STATUS_LOGO_HBOX);
      this._showElement(RIP_STATUS_LOGO_POST_SPACER);
      this._setImageSrc(RIP_STATUS_LOGO, aProviderLogoSrc);

      // Set the tooltip on the provider logo if provided.
      if (aProviderName) {
        this._setToolTip(
            RIP_STATUS_LOGO,
            SBFormattedString("cdrip.mediaview.status.lookup_provided_by",
              [aProviderName]));
      }

      // Set the click handler URL if the URL is also provided.
      if (aProviderURL) {
        this._setLogoURLListener(RIP_STATUS_LOGO, aProviderURL);
      }
    }
  },

  _hideLookupVendorInfo: function cdripController_hideLookupVendorInfo() {
    this._hideElement(RIP_STATUS_LOGO_PRE_SPACER);
    this._hideElement(RIP_STATUS_LOGO_HBOX);
    this._hideElement(RIP_STATUS_LOGO_POST_SPACER);
    this._setImageSrc(RIP_STATUS_LOGO, "");
  },

  _setLogoURLListener: function cdripController_setLogoURLListener(aElementId, aURL) {
    var element = document.getElementById(aElementId);
    if (element) {
      element.setAttribute('class', "sb-cdrip-link");
      element.addEventListener('click', function(event) {
        var browser = SBGetBrowser();
        browser.loadURI(aURL, null, null);
      }, false);
    }
  },

  _loadPlaylist: function cdripController_loadPlaylist() {
    this._mediaListView = this._getMediaListView();
    if (!this._mediaListView) {
      Cu.reportError("Unable to get media list view for device.");
      return;
    }

    this._playlist = document.getElementById(CDRIP_PLAYLIST);

    // Clear the playlist filters. This is a filter-free view.
    this._clearFilterLists();

    // Set a flag so sb-playlist won't try to writeback metadata to files
    this._mediaListView.mediaList.setProperty(
      "http://songbirdnest.com/data/1.0#dontWriteMetadata", 1);

    // Make sure we can not drag items from the cd to libraries or playlists
    this._playlist.disableDrag = true;
    // Make sure we can not drop items to the cd rip library
    this._playlist.disableDrop = true;

    // Setup our columns for the CD View
    if (!this._mediaListView.mediaList.getProperty(SBProperties.columnSpec)) {
      this._mediaListView.mediaList.setProperty(SBProperties.columnSpec,
        "http://songbirdnest.com/data/1.0#shouldRip 25 " +
        "http://songbirdnest.com/data/1.0#trackNumber 50 a " +
        "http://songbirdnest.com/data/1.0#cdRipStatus 75 " +
        "http://songbirdnest.com/data/1.0#trackName 255 " +
        "http://songbirdnest.com/data/1.0#duration 70 " +
        "http://songbirdnest.com/data/1.0#artistName 122 " +
        "http://songbirdnest.com/data/1.0#albumName 122 " +
        "http://songbirdnest.com/data/1.0#genre 70");
    }

    // Get playlist commands (context menu, keyboard shortcuts, toolbar)
    // Note: playlist commands currently depend on the playlist widget.
    var mgr =
      Components.classes["@songbirdnest.com/Songbird/PlaylistCommandsManager;1"]
                .createInstance(Components.interfaces.sbIPlaylistCommandsManager);
    var cmds = mgr.request(kPlaylistCommands.MEDIALIST_CDDEVICE_LIBRARY);

    // Set up the playlist widget
    this._playlist.bind(this._mediaListView, cmds);

    // Listen to changes for readonly status.
    this._libraryListener =
      new CDRipLibraryListener(this, this._playlist);
    var flags = Ci.sbIMediaList.LISTENER_FLAGS_ITEMUPDATED;
    this._mediaListView.mediaList.addListener(this._libraryListener,
                                              false,
                                              flags);
  },

  /**
   * Create a mediaListView from the CD Device Library.
   */
  _getMediaListView: function cdripController_getMediaListView() {
    if (!this._device) {
      return null;
    }

    var lib = this._getDeviceLibrary();
    if (!lib) {
      Cu.reportError("Unable to get library for device: " + this._device.id);
      return null;
    }

    // Create a view for the library and return it.
    return lib.createView();
  },

  /**
   * Configure the playlist filter lists
   */
  _clearFilterLists: function cdripController_clearFilterLists() {
    this._mediaListView.filterConstraint =
      LibraryUtils.standardFilterConstraint;
  },

  /**
   * Checks the main library of the device to see if it needs a lookup.
   */
  _checkIfNeedsLookup: function cdripController_checkIfNeedsLookup() {
    if (!this._device) {
      return;
    }

    var lib = this._getDeviceLibrary();
    if (!lib) {
      Cu.reportError("Unable to get library for device: " + this._device.id);
      return;
    }

    var deviceNeedsLookupPref = "songbird.cdrip." + lib.guid + ".needsLookup";
    if (Application.prefs.has(deviceNeedsLookupPref)) {
      var shouldLookup = Application.prefs.getValue(deviceNeedsLookupPref, false);
      if (shouldLookup) {
        Application.prefs.setValue(deviceNeedsLookupPref, false);
        sbCDDeviceUtils.doCDLookUp(this._device);
      }
    }
  },

  //----------------------------------------------------------------------------
  // Transcode job status helpers

  _getLastTranscodeStatus: function cdripController_getLastTranscodeStatus() {
    if (!this._device) {
      return 0;
    }

    //Check if any tracks were cancelled, ie the rip operation was cancelled
    var deviceLibrary = this._getDeviceLibrary();
    var abortCount = 0;
    try {
      // Get all the did not successfully ripped tracks
      var propArray =
        Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
          .createInstance(Ci.sbIMutablePropertyArray);
      //tracks with 4|100 were aborted
      propArray.appendProperty(SBProperties.cdRipStatus, "4|100");
      propArray.appendProperty(SBProperties.shouldRip, "1");

      var abortedItems = deviceLibrary.getItemsByProperties(propArray);
      abortCount = abortedItems.length;
    }
    catch (err if err.result == Cr.NS_ERROR_NOT_AVAILABLE) {
      // deviceLibrary.getItemsByProperties() will throw NS_ERROR_NOT_AVAILABLE
      // if there are no cancelled rips in the list.  Ignore this error.
    }
    catch (err) {
      Cu.reportError("ERROR GETTING TRANSCODE ERROR COUNT " + err);
    }
    if (abortCount > 0) return FINAL_TRANSCODE_STATUS_ABORT;

    // Check for any tracks that have a failed status ('3|100')
    var errorCount = 0;
    try {
      // Get all the did not successfully ripped tracks
      var propArray =
        Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
          .createInstance(Ci.sbIMutablePropertyArray);
      propArray.appendProperty(SBProperties.cdRipStatus, "3|100");
      propArray.appendProperty(SBProperties.shouldRip, "1");

      var rippedItems = deviceLibrary.getItemsByProperties(propArray);
      errorCount = rippedItems.length;
    }
    catch (err if err.result == Cr.NS_ERROR_NOT_AVAILABLE) {
      // deviceLibrary.getItemsByProperties() will throw NS_ERROR_NOT_AVAILABLE
      // if there are no failed rips in the list.  Ignore this error.
    }
    catch (err) {
      Cu.reportError("ERROR GETTING TRANSCODE ERROR COUNT " + err);
    }

    var status = FINAL_TRANSCODE_STATUS_SUCCESS;
    if (errorCount > 0) {
      // Compare the error count to how many tracks were supposed to be ripped.
      var shouldRipItems =
        deviceLibrary.getItemsByProperty(SBProperties.shouldRip, "1");
      if (errorCount == shouldRipItems.length)
        status = FINAL_TRANSCODE_STATUS_FAILED;
      else
        status = FINAL_TRANSCODE_STATUS_PARTIAL;
    }

    return status;
  },

  _getDeviceLibrary: function cdripController_getDeviceLibrary() {
    if (this._deviceLibrary)
      return this._deviceLibrary;

    if (!this._device) {
      return null;
    }

    // Get the libraries for device
    var libraries = this._device.content.libraries;
    if (libraries.length < 1) {
      // Oh no, we have no libraries
      Cu.reportError("Device " + this._device + " has no libraries!");
      return null;
    }

    // Get the requested library
    this._deviceLibrary = libraries.queryElementAt(0, Ci.sbIMediaList);
    if (!this._deviceLibrary) {
      Cu.reportError("Unable to get library for device: " + aDevice.id);
      return null;
    }

    return this._deviceLibrary;
  },

  /**
   * Get the device from the id passed in on the query string.
   */
  _getDevice: function cdripController_getDevice() {
    // Get the device id from the query string in the uri
    var queryMap = this._parseQueryString();

    this._deviceID = queryMap["device-id"];

    // Get the device for this media view
    var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceManager2);

    try {
      return deviceManager.getDevice(Components.ID(this._deviceID));
    } catch (x) {
      Cu.reportError("Device: " + this._deviceID + " does not exist");
    }

    return null;
  },

  /**
   * Helper function to parse and unescape the query string.
   * Returns a object mapping key->value
   */
  _parseQueryString: function cdripController_parseQueryString() {
    var queryString = (location.search || "?").substr(1).split("&");
    var queryObject = {};
    var key, value;
    for each (var pair in queryString) {
      [key, value] = pair.split("=");
      queryObject[key] = unescape(value);
    }
    return queryObject;
  }
};

//------------------------------------------------------------------------------
// sbIMediaPage implementation

function MediaPageImpl() {}
MediaPageImpl.prototype = {
  get mediaListView() {
    return window.cdripController._mediaListView;
  },
  set mediaListView(value) {},

  get isOnlyView() true,

  highlightItem: function(aIndex) {
    window.cdripController._playlist.highlightItem(aIndex);
  },

  canDrop: function(aEvent, aSession) {
    return false;
  },
  onDrop: function(aEvent, aSession) {
    return false;
  }
}

//==============================================================================
//
// @brief sbIMediaListListener class for validating the playlist tree when the
//        readonly state changes.
//
//==============================================================================

function CDRipLibraryListener(aCallback, aPlaylistObject)
{
  this._mCallback = aCallback;
  this._mPlaylist = aPlaylistObject;
  this._mTreeBoxObject = aPlaylistObject.tree.boxObject;
}

CDRipLibraryListener.prototype =
{
  _mCallback: null,
  _mTreeBoxObject: null,
  _mPlaylist: null,

  //----------------------------------------------------------------------------
  // sbIMediaListListener

  onItemAdded: function(aMediaList, aMediaItem, aIndex) {
    return true;  // ignore
  },

  onBeforeItemRemoved: function(aMediaList, aMediaItem, aIndex) {
    return true;  // ignore
  },

  onAfterItemRemoved: function(aMediaList, aMediaItem, aIndex) {
    return true;  // ignore
  },

  onItemUpdated: function(aMediaList, aMediaItem, aProperties) {
    // Check for the |isReadOnly| property.
    for (var i = 0; i < aProperties.length; i++) {
      if (aProperties.getPropertyAt(i).id == SBProperties.isReadOnly) {
        this._mTreeBoxObject.invalidate();
        this._mPlaylist.refreshCommands(false);
        break;
      }
    }

    return true;
  },

  onItemMoved: function(aMediaList, aFromIndex, aToIndex) {
    return true;  // ignore.
  },

  onBeforeListCleared: function(aMediaList, aExcludeLists) {
    return true;  // ignore.
  },

  onListCleared: function(aMediaList, aExcludeLists) {
   return true;  // ignore
  },

  onBatchBegin: function(aMediaList) {
    // ignore
  },

  onBatchEnd: function(aMediaList) {
    // ignore
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediaListListener])
};

