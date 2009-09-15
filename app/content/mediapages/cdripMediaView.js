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

const SB_NS = 'http://songbirdnest.com/data/1.0#';

Components.utils.import("resource://app/jsmodules/StringUtils.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/kPlaylistCommands.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbCDDeviceUtils.jsm");

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
const RIP_STATUS_IMAGE_VBOX         = CDRIP_BASE + "actionstatus-image";
const RIP_STATUS_IMAGE              = CDRIP_BASE + "status-image";
const RIP_STATUS_LABEL              = CDRIP_BASE + "actionstatus-label";
const RIP_STATUS_RIP_CD_BUTTON      = CDRIP_BASE + "rip-cd-button";
const RIP_STATUS_STOP_RIP_BUTTON    = CDRIP_BASE + "stop-rip-button";
const RIP_STATUS_EJECT_CD_BUTTON    = CDRIP_BASE + "eject-cd-button";

const RIP_COMMAND_EJECT             = CDRIP_BASE + "eject-command";
const RIP_COMMAND_STARTRIP          = CDRIP_BASE + "startrip-command";
const RIP_COMMAND_STOPRIP           = CDRIP_BASE + "stoprip-command";

const CDRIP_PLAYLIST                = CDRIP_BASE + "playlist";

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

  _supportedProfiles: null,

  onLoad: function cdripController_onLoad() {
    // Add our device listener to listen for lookup notification events
    this._device = this._getDevice();

    // Go back to previous page and return if device is not available.
    if (!this._device) {
      var browser = SBGetBrowser();
      browser.getTabForDocument(document).backWithDefault();
      return;
    }

    // Update the header information
    this._updateHeaderView();

    if (this._device.state == Ci.sbICDDeviceEvent.STATE_LOOKINGUPCD) {
      this._toggleLookupNotification(true);
    } else {
      this._toggleLookupNotification(false);
    }
    var eventTarget = this._device.QueryInterface(Ci.sbIDeviceEventTarget);
    eventTarget.addEventListener(this);
    
    // Disable player controls & load the playlist
    this._updateRipSettings();
    this._togglePlayerControls(true);
    this._loadPlaylist();
  },

  onUnload: function cdripController_onUnload() {
    this._togglePlayerControls(false);
    if (this._device) {
      var eventTarget = this._device.QueryInterface(Ci.sbIDeviceEventTarget);
      eventTarget.addEventListener(this);
      this._device.removeEventListener(this);
    }
  },

  onDeviceEvent: function cdripController_onDeviceEvent(aEvent) {
    switch (aEvent.type) {
      // START CD LOOKUP
      case Ci.sbICDDeviceEvent.EVENT_CDLOOKUP_INITIATED:
        this._toggleLookupNotification(true);
        break;
      
      // CD LOOKUP COMPLETE
      case Ci.sbICDDeviceEvent.EVENT_CDLOOKUP_COMPLETED:
        this._toggleLookupNotification(false);
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED :
        // Go back to previous page if device removed.
        var device = aEvent.data.QueryInterface(Ci.sbIDevice);
        if (device.id.toString() == this._deviceID) {
          var browser = SBGetBrowser();
          browser.getTabForDocument(document).backWithDefault();
        }
        break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        var device = aEvent.origin.QueryInterface(Ci.sbIDevice);
        if (device.id.toString() == this._deviceID) {
          this._updateHeaderView();
        }
        break;

      default:
        break;
    }
  },

  showCDRipSettings: function cdripController_showCDRipSettings() {
    Cc["@mozilla.org/appshell/window-mediator;1"]
      .getService(Ci.nsIWindowMediator)
      .getMostRecentWindow("Songbird:Main")
      .SBOpenPreferences("paneCDRip");
  },

  _toggleLookupNotification: function
                             cdripController_lookupNotification(show) {
    var ripImage = document.getElementById(RIP_STATUS_IMAGE);
    var statusBox = document.getElementById("sb-cdrip-status-hbox");
    var buttonsBox = document.getElementById("sb-cdrip-status-buttons");
    if (show) {
      statusBox.style.visibility = "visible";
      buttonsBox.style.visibility = "collapse";
      ripImage.src =
        "chrome://songbird/skin/base-elements/icon-loading-large.png";
      this._setLabelValue(RIP_STATUS_LABEL,
                          SBString("cdrip.mediaview.status.lookup"));
    } else {
      statusBox.style.visibility = "collapse";
      buttonsBox.style.visibility = "visible";
      ripImage.src = "";
      this._setLabelValue(RIP_STATUS_LABEL, "");
    }
    this._updateHeaderView();
  },

  _updateHeaderView: function cdripController__updateHeaderView() {
    switch (this._device.state) {
      // STATE_LOOKINGUPCD
      case Ci.sbICDDeviceEvent.STATE_LOOKINGUPCD:
        // Ignore this state since it is handled by the events in the
        // onDeviceEvent function.
        break;
      
      case Ci.sbIDevice.STATE_TRANSCODE:
        // Currently ripping
        this._showSettingsView();

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
        if (prop.propertName == "bitrate") {
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
    var profiles = transcodeManager.getTranscodeProfiles();

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

    // Setup our columns for the CD View
    this._mediaListView.mediaList.setProperty(SBProperties.columnSpec,
      "http://songbirdnest.com/data/1.0#shouldRip 25 " +
      "http://songbirdnest.com/data/1.0#trackNumber 50 a " +
      "http://songbirdnest.com/data/1.0#cdRipStatus 75 " +
      "http://songbirdnest.com/data/1.0#trackName 255 " +
      "http://songbirdnest.com/data/1.0#duration 70 " +
      "http://songbirdnest.com/data/1.0#artistName 122 " +
      "http://songbirdnest.com/data/1.0#albumName 122 " +
      "http://songbirdnest.com/data/1.0#genre 70");

    // Get playlist commands (context menu, keyboard shortcuts, toolbar)
    // Note: playlist commands currently depend on the playlist widget.
    var mgr =
      Components.classes["@songbirdnest.com/Songbird/PlaylistCommandsManager;1"]
                .createInstance(Components.interfaces.sbIPlaylistCommandsManager);
    var cmds = mgr.request(kPlaylistCommands.MEDIALIST_CDDEVICE_LIBRARY);

    // Set up the playlist widget
    this._playlist.bind(this._mediaListView, cmds);
  },

  /**
   * Create a mediaListView from the CD Device Library.
   */
  _getMediaListView: function cdripController_getMediaListView() {
    if (!this._device) {
      return null;
    }
    
    // Get the libraries for device
    var libraries = this._device.content.libraries;
    if (libraries.length < 1) {
      // Oh no, we have no libraries
      Cu.reportError("Device " + this._device.id + " has no libraries!");
      return null;
    }

    // Get the first library
    var lib = libraries.queryElementAt(0, Ci.sbILibrary);
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
    // Don't use filters for this version. We'll clear them out.
    var filters = this._mediaListView.cascadeFilterSet;
    
    for (var i = filters.length - 1; i > 0; i--) {
     filters.remove(i);
    }
    if (filters.length == 0 || !filters.isSearch(0)) {
      if (filters.length == 1) {
        filters.remove(0);
      }
      filters.appendSearch(["*"], 1);
    }
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
    var device = deviceManager.getDevice(Components.ID(this._deviceID));
    if (!device) {
      Cu.reportError("Device: " + this._deviceID + " does not exist");
      return null;
    }
    return device;
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

