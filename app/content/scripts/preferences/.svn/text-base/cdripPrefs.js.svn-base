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

//
// \file cdripPrefs.js
// \brief Javascript source for the CD Rip preferences UI.
//

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;

Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/StringUtils.jsm");


//------------------------------------------------------------------------------
//
// CD Rip preference pane services.
//
//------------------------------------------------------------------------------

var CDRipPrefsPane =
{
  transcodeSettings: null,

  /**
   * \brief Load up the CD Rip Preferences Panel.
   */
  doPaneLoad: function CDRipPrefsPane_doPaneLoad() {
    this.transcodeSettings = document.getElementById("cd-transcode-settings");
    // Initialize the transcoding options:
    this.loadTranscodeSettings();

    // Initialize the list of metadatalookup providers
    var provList = document.getElementById("provider-list");
    var provPopup = document.getElementById("provider-popup");
    var catMgr = Cc["@mozilla.org/categorymanager;1"]
                 .getService(Ci.nsICategoryManager);
    var e = catMgr.enumerateCategory('metadata-lookup');
    var defaultProvider =
              Cc["@songbirdnest.com/Songbird/MetadataLookup/manager;1"]
                .getService(Ci.sbIMetadataLookupManager)
                .defaultProvider.name;

    this.providerDescriptions = new Array();
    while (e.hasMoreElements()) {
      var key = e.getNext().QueryInterface(Ci.nsISupportsCString);
      var contract = catMgr.getCategoryEntry('metadata-lookup', key);
      try {
        var provider = Cc[contract].getService(Ci.sbIMetadataLookupProvider);
        var provMenuItem = document.createElement("menuitem");
        provMenuItem.setAttribute("label", provider.name);
        provMenuItem.setAttribute("value", provider.name);
        provPopup.appendChild(provMenuItem);
        if (provider.name == defaultProvider) {
          provList.selectedItem = provMenuItem;
          this.setProviderDescription(provider.description);
        }
        this.providerDescriptions[provider.name] = provider.description;
      } catch (e) {
        dump("Exception in CD Rip prefs pane: " + e + "\n");
      }
    }

    // Check to see if a device is currently ripping and update the UI
    if (this.isAnyCDDeviceTranscoding())
      this.addNotification();

    // Setup listeners for the devices so we can notify the user when one is
    // currently ripping.
    this.setupListeners();

    window.addEventListener("unload", CDRipPrefsPane.doPaneUnload, true);
  },

  /**
   * \brief Shutdown the panel.
   */
  doPaneUnload: function CDRipPrefsPane_doPaneUnload() {
    this.transcodeSettings = null;
    window.removeEventListener("unload", CDRipPrefsPane.doPaneUnload, true);
    CDRipPrefsPane.shutdownListeners();
  },

  /**
   * \brief Add a listener to a device if it is a CD device.
   * \param aDevice device to add a listener to
   */
  addDeviceListener: function CDRipPrefPane_addDeviceListener(aDevice) {
    var deviceType = aDevice.parameters.getProperty("DeviceType");
    if (deviceType == "CD") {
      var deviceEventTarget = aDevice.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.addEventListener(this);
    }
  },

  /**
   * \brief Remove a listener from a device if it is a CD device.
   * \param aDevice device to remove a listener from
   */
  removeDeviceListener: function CDRipPrefPane_removeDeviceListener(aDevice) {
    var deviceType = aDevice.parameters.getProperty("DeviceType");
    if (deviceType == "CD") {
      var deviceEventTarget = aDevice.QueryInterface(Ci.sbIDeviceEventTarget);
      deviceEventTarget.removeEventListener(this);
    }
  },

  /**
   * \brief Add listeners for the devices so we can update the UI if a device
   * starts ripping (only CD Devices)
   */
  setupListeners: function CDRipPrefPane_setupListeners() {
    var deviceMgr =
      Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
        .getService(Ci.sbIDeviceManager2);
    var registrar = deviceMgr.QueryInterface(Ci.sbIDeviceRegistrar);
    for (var i=0; i<registrar.devices.length; i++) {
      var device = registrar.devices.queryElementAt(i, Ci.sbIDevice);
      this.addDeviceListener(device);
    }

    // We also want to listen for devices being added/removed
    deviceMgr.addEventListener(this);
  },

  /**
   * \brief Remove listeners for the devices we added (only CD Devices)
   */
  shutdownListeners: function CDRipPrefPane_setupListeners() {
    var deviceMgr =
      Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
        .getService(Ci.sbIDeviceManager2);
    var registrar = deviceMgr.QueryInterface(Ci.sbIDeviceRegistrar);
    for (var i=0; i<registrar.devices.length; i++) {
      var device = registrar.devices.queryElementAt(i, Ci.sbIDevice);
      this.removeDeviceListener(device);
    }

    // Remove our device manager listener to clean up.
    deviceMgr.removeEventListener(this);
  },

  /**
   * \brief Callback function when something happens on a device, we only
   * listen to CD devices.
   * \param aEvent Device event that occured.
   * \sa sbIDeviceEvent
   */
  onDeviceEvent: function CDRipPrefsPane_onDeviceEvent(aEvent) {
    switch (aEvent.type) {
      case Ci.sbIDeviceEvent.EVENT_DEVICE_STATE_CHANGED:
        var device = aEvent.origin.QueryInterface(Ci.sbIDevice);
        if (device.state == Ci.sbIDevice.STATE_TRANSCODE) {
          this.addNotification();
        }
        else if (!this.isAnyCDDeviceTranscoding()) {
          // Remove the notifications if all cd devices are not busy
          this.removeNotifications();
        }
      break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_ADDED:
        var device = aEvent.origin.QueryInterface(Ci.sbIDevice);
        this.addDeviceListener(device);
      break;

      case Ci.sbIDeviceEvent.EVENT_DEVICE_REMOVED:
        var device = aEvent.origin.QueryInterface(Ci.sbIDevice);
        this.removeDeviceListener(device);
      break;

      default:
      break;
    }
  },

  /**
   * \brief Remove all the notifications in the cd rip panel.
   */
  removeNotifications: function CDRipPrefsPane_removeNotifications() {
    // Get the current notification box for this window.
    var notifBox = document.getElementById("cdrip_notification_box");
    notifBox.removeAllNotifications(false);
  },

  /**
   * \brief Add a notification if one does not already exist
   */
  addNotification: function CDRipPrefsPane_addNotification() {
    // Get the current notification box for this window.
    var notifBox = document.getElementById("cdrip_notification_box");
    if (notifBox.currentNotification) {
      // Already one shown so abort
      return;
    }
    notifBox.appendNotification(SBString("cdrip.prefpane.device_busy"),
                                "device_busy",
                                null,
                                notifBox["PRIORITY_WARNING_MEDIUM"],
                                []);
  },

  /**
   * \brief Check if any CD Device is transcoding.
   *  NOTE: This method will loop through all devices.
   * \return True if any CD device is transcoding, False otherwise.
   */
  isAnyCDDeviceTranscoding: function CDRipPrefsPane_isAnyCDDeviceTranscoding() {
    // enumerate all devices and see if any of the cd ones are currently busy
    var deviceMgr =  Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                       .getService(Ci.sbIDeviceManager2);
    var registrar = deviceMgr.QueryInterface(Ci.sbIDeviceRegistrar);
    for (var i=0; i<registrar.devices.length; i++) {
      var device = registrar.devices.queryElementAt(i, Ci.sbIDevice);
      var deviceType = device.parameters.getProperty("DeviceType");
      if (deviceType == "CD" && device.state == Ci.sbIDevice.STATE_TRANSCODE)
      {
        return true;
      }
    }
    return false;
  },

  /**
   * \brief Callback for when the user selects a different provider from the
   * list
   * \param aEvent Event information.
   */
  providerChanged: function CDRipPrefsPane_providerChanged(aEvent) {
    var provName = aEvent.target.value;
    this.setProviderDescription(this.providerDescriptions[provName]);
  },

  /**
   * \brief Set the provider description for the user
   * \param aDescription Text change the description to.
   */
  setProviderDescription:
                function CDRipPrefsPane_setProviderDescription(aDescription) {
    var provDescr = document.getElementById("provider-description");
    while (provDescr.firstChild)
      provDescr.removeChild(provDescr.firstChild);
    provDescr.appendChild(document.createTextNode(aDescription));
  },

  //----------------------------------------------------------------------------
  // Transcoding and quality utils

  /**
   * \brief Add all the audio profiles to the pop up list for the user to
   * select.
   */
  loadTranscodeSettings: function CDRipPrefsPane_loadTranscodeSettings() {
    var transcodeManager =
      Cc["@songbirdnest.com/Songbird/Mediacore/TranscodeManager;1"]
        .getService(Ci.sbITranscodeManager);
    var profiles = transcodeManager.getTranscodeProfiles(
            Ci.sbITranscodeProfile.TRANSCODE_TYPE_AUDIO);

    var supportedProfiles = [];
    for (let i = 0; i < profiles.length; i++) {
      let profile = profiles.queryElementAt(i, Ci.sbITranscodeProfile);
      supportedProfiles.push(profile);
    }

    // Set the available transcode profiles.
    this.transcodeSettings.profiles = supportedProfiles;

    // Get the current default transcode profile and bitrate.
    var formatIdPref = document.getElementById("rip_format_pref");
    var bitratePref = document.getElementById("rip_quality_pref");
    var defaultTranscodeId =
      Application.prefs.getValue(formatIdPref.getAttribute("name"), "");
    var defaultBitrate =
      Application.prefs.getValue(bitratePref.getAttribute("name"), "");

    // Set the selected transcode profile and bitrate.
    this.transcodeSettings.transcodeBitrate = defaultBitrate;
    this.transcodeSettings.transcodeProfileId = defaultTranscodeId;

    // In case invalid values were previously stored, write back to prefs
    formatIdPref.value = this.transcodeSettings.transcodeProfileId;
    bitratePref.value = this.transcodeSettings.transcodeBitrate;
  },

  /**
   * \brief Updates cd-rip transcode preferences when binding changes.
   */
  onUIPrefChanged: function CDRipPrefsPane_onUIPrefChanged() {
    var profile = document.getElementById("rip_format_pref");
    profile.value = this.transcodeSettings.transcodeProfileId;
    var qualityPref = document.getElementById("rip_quality_pref");
    qualityPref.value = this.transcodeSettings.transcodeBitrate;
  }
}
