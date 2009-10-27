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

// Make a namespace.
if (typeof MockCDController == 'undefined') {
  var MockCDController = {};
}

/**
 * UI controller that is loaded into the main player window
 */
MockCDController.Controller =
{
  onLoad: function()
  {
    var showCommand =
      document.getElementById("mockcdcontroller-showcontroller-cmd");

    var self = this;
    showCommand.addEventListener(
      "command",
      function() { self.showCDControllerPane(); },
      false);
      
    var showCommandDevice = 
      document.getElementById("mockdevicecontroller-showcontroller-cmd");
      
    showCommandDevice.addEventListener(
      "command",
      function() { self.showDeviceControllerPane(); },
      false);

    // Only open the controller pane if the pref is currently set.
    if (Application.prefs.getValue("extensions.cdripcontroller.startup_show_controller",
                                   false)) {
      setTimeout(function() { self.showCDControllerPane(); }, 200);
    }
  },

  showCDControllerPane: function()
  {
    window.openDialog("chrome://mockcdcontroller/content/mockCDControllerDialog.xul",
                      "cd-controller-pane",
                      "chrome,centerscreen,resizable=false");
  },
  
  showDeviceControllerPane: function()
  {
    window.openDialog("chrome://mockcdcontroller/content/deviceControllerDialog.xul",
                      "device-controller-pane",
                      "chrome,centerscreen,resizable=false");
  }
};

window.addEventListener(
  "load",
  function(e) { MockCDController.Controller.onLoad(e); },
  false)
