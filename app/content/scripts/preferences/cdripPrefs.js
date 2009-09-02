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
// CD Rip preference pane UI controller.
//
//------------------------------------------------------------------------------

var CDRipPrefsUI =
{
  initialize: function CDRipPrefsUI_initialize() {
    //
    // XXXkreeger:
    // Nothing to do until transcoding profiles are available (for populating
    // the format and quality popups).
    //
  },
};

//------------------------------------------------------------------------------
//
// CD Rip preference pane services.
//
//------------------------------------------------------------------------------

var CDRipPrefsPane =
{
  warningShown: false,

  doPaneLoad: function CDRipPrefsPane_doPaneLoad() {
    CDRipPrefsUI.initialize();
    CDRipPrefsPane.prefBranch = Cc["@mozilla.org/preferences-service;1"]
                                  .getService(Ci.nsIPrefService)
                                  .getBranch("songbird.cdrip.")
                                  .QueryInterface(Ci.nsIPrefBranch2);
    CDRipPrefsPane.prefBranch.addObserver("", CDRipPrefsPane, false);
  },

  doPaneUnload: function CDRipPrefsPane_doPaneUnload() {
    CDRipPrefsPane.prefBranch.removeObserver("", CDRipPrefsPane, false);
  },

  observe: function CDRipPrefsPane_observe(subject, topic, data) {
    // enumerate all devices and see if any of them are currently ripping
    var deviceBusy = false;
    var deviceMgr = Cc["@songbirdnest.com/Songbird/DeviceManager;2"]
                 .getService(Ci.sbIDeviceManager2);
    var registrar = deviceMgr.QueryInterface(Ci.sbIDeviceRegistrar);
    for (var i=0; i<registrar.devices.length; i++) {
      var device = registrar.devices.queryElementAt(i, Ci.sbIDevice);
      var deviceType = device.parameters.getProperty("DeviceType");
      if (deviceType == "CD" && device.state != Ci.sbIDevice.STATE_IDLE)
      {
          deviceBusy = true;
          break;
      }
    }

    if (subject instanceof Ci.nsIPrefBranch && !this.warningShown && deviceBusy)
    {
      var notifBox = document.getElementById("cdrip_notification_box");
      notifBox.appendNotification(SBString("cdrip.prefpane.device_busy"),
                                  "device_busy",
                                  null,
                                  notifBox["PRIORITY_WARNING_MEDIUM"],
                                  []);
      this.warningShown = true;
    }
  },
}
