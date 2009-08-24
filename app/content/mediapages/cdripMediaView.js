/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
// http://songbirdnest.com
//
// This file may be licensed under the terms of of the
// GNU General Public License Version 2 (the "GPL").
//
// Software distributed under the License is distributed
// on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
// express or implied. See the GPL for the specific language
// governing rights and limitations.
//
// You should have received a copy of the GPL along with this
// program. If not, go to http://www.gnu.org/licenses/gpl.html
// or write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
//
// END SONGBIRD GPL
//
*/

Components.utils.import("resource://app/jsmodules/StringUtils.jsm");

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
const RIP_SETTINGS_QUALITY_LABEL    = CDRIP_BASE + "quality-label";
const RIP_SETTINGS_BUTTON           = CDRIP_BASE + "settings-button";

const RIP_SPACER                    = CDRIP_BASE + "spacer";

const RIP_STATUS_HBOX               = CDRIP_BASE + "actionstatus-hbox";
const RIP_STATUS_IMAGE_VBOX         = CDRIP_BASE + "actionstatus-image";
const RIP_STATUS_IMAGE              = CDRIP_BASE + "status-image";
const RIP_STATUS_LABEL              = CDRIP_BASE + "actionstatus-label";
const RIP_STATUS_RIP_CD_BUTTON      = CDRIP_BASE + "rip-cd-button";
const RIP_STATUS_STOP_RIP_BUTTON    = CDRIP_BASE + "stop-rip-button";
const RIP_STATUS_VIEW_TRACKS_BUTTON = CDRIP_BASE + "view-tracks-button";
const RIP_STATUS_EJECT_CD_BUTTON    = CDRIP_BASE + "eject-cd-button";

//------------------------------------------------------------------------------
// CD rip view controller

window.cdripController =
{
  onLoad: function cdripController_onLoad() {
    this._hideSettingsView();

    // Hide the status buttons.
    this._hideElement(RIP_STATUS_RIP_CD_BUTTON);
    this._hideElement(RIP_STATUS_STOP_RIP_BUTTON);
    this._hideElement(RIP_STATUS_VIEW_TRACKS_BUTTON);
    this._hideElement(RIP_STATUS_EJECT_CD_BUTTON);

    // Add our device listener to listen for lookup notification events
    this._device = this._getDevice();
    if (this._device.state == 536870913) {
      this._toggleLookupNotification(true);
    } else {
      this._toggleLookupNotification(false);
    }
    var eventTarget = this._device.QueryInterface(Ci.sbIDeviceEventTarget);
    eventTarget.addEventListener(this);

    // Go back to previous page and return if device is not available.
    if (!this._device) {
      var browser = SBGetBrowser();
      browser.getTabForDocument(document).backWithDefault();
      return;
    }
  },

  onUnload: function cdripController_onUnload() {
    if (this._device) {
      var eventTarget = this._device.QueryInterface(Ci.sbIDeviceEventTarget);
      eventTarget.addEventListener(this);
      this._device.removeEventListener(this);
    }
  },

  onDeviceEvent: function cdripController_onDeviceEvent(aEvent) {
    switch (aEvent.type) {
      // START CD LOOKUP
      case Ci.sbIDeviceEvent.EVENT_CDLOOKUP_INITIATED:
        this._toggleLookupNotification(true);
      
      // CD LOOKUP COMPLETE
      case Ci.sbIDeviceEvent.EVENT_CDLOOKUP_COMPLETED:
        this._toggleLookupNotification(false);
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
    if (show) {
      var ripImage = document.getElementById(RIP_STATUS_IMAGE);
      ripImage.src =
        "chrome://songbird/skin/base-elements/icon-loading-large.png";
      this._setLabelValue(RIP_STATUS_LABEL,
                          SBString("cdrip.mediaview.status.lookup"));
    } else {
      var ripImage = document.getElementById(RIP_STATUS_IMAGE);
      ripImage.src = "";
      this._setLabelValue(RIP_STATUS_LABEL, "");
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

  _setLabelValue: function cdripController_setLabelValue(aElementId, aValue) {
    document.getElementById(aElementId).value = aValue;
  },

  _getDevice: function cdripController_getDevice() {
    // Get the device id from the query string in the uri
    var queryMap = this._parseQueryString();
   
    // Get the device for this media view
    var deviceManager = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                          .getService(Ci.sbIDeviceManager2);
    var device = deviceManager.getDevice(Components.ID(queryMap["device-id"]));
    if (!device) {
      Cu.reportError("Device: " + queryMap["device-id"] + " does not exist");
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
  },

};

