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
const RIP_STATUS_IMAGE              = CDRIP_BASE + "actionstatus-image";
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
    // For now, just show the "Looking up info" message.
    this._hideSettingsView();
    this._setLabelValue(RIP_STATUS_LABEL,
                        SBString("cdrip.mediaview.status.lookup"));

    // Hide the status buttons.
    this._hideElement(RIP_STATUS_RIP_CD_BUTTON);
    this._hideElement(RIP_STATUS_STOP_RIP_BUTTON);
    this._hideElement(RIP_STATUS_VIEW_TRACKS_BUTTON);
    this._hideElement(RIP_STATUS_EJECT_CD_BUTTON);
  },

  showCDRipSettings: function cdripController_showCDRipSettings() {
    Cc["@mozilla.org/appshell/window-mediator;1"]
      .getService(Ci.nsIWindowMediator)
      .getMostRecentWindow("Songbird:Main")
      .SBOpenPreferences("paneCDRip");
  },

  _testView: function() {
    //
    // XXXkreeger Testing method for stevo.
    //
    if (this._isSettingsViewVisible) {
      this._hideSettingsView();
    }
    else {
      this._showSettingsView();
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
};

