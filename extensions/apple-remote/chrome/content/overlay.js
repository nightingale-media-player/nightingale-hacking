/*
//
// BEGIN NIGHTINGALE GPL
// 
// This file is part of the Nightingale Media Player.
//
// Copyright(c) 2014
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
 */

var Cc = Components.classes;
var Ci = Components.interfaces;

var isListening = false;

var gRemoteControlService = Cc["@songbirdnest.com/mac-remote-service;1"]
                            .getService(Ci.sbIAppleRemoteService);
var gPrefService = Cc["@mozilla.org/preferences-service;1"]
                     .getService(Ci.nsIPrefBranch2);

//------------------------------------------------------------------------------
// Handle autolaunching of the remote control

var shouldAutoEnable = false;
try {
  shouldAutoEnable = 
    gPrefService.getBoolPref("extensions.apple-remote.autoenable");

  if (shouldAutoEnable && gRemoteControlService.isSupported) {
    gRemoteControlService.startListening();
    isListening = true;

    setTimeout(
      function() { 
        var menuItem = document.getElementById("mac_remote_menuitem");
        if (menuItem) {
          menuItem.setAttribute("checked", "true");
        }
      }, 
      1000
    );
  }
}
catch (e) {
  dump("\n\n\n\n  ERROR: " + e + "\n\n\n\n");
}

//------------------------------------------------------------------------------
// If the current machine doesn't support the apple remote, don't
// enable the menu item.

if (!gRemoteControlService.isSupported) {
  // HACK: Set a timeout to find the menuitem..
  setTimeout(
    function() { 
      var menuItem = document.getElementById("mac_remote_menuitem");
      if (menuItem) {
        menuItem.setAttribute("disabled", "true");
      }
    }, 1000);
}

//------------------------------------------------------------------------------
// Callback function from menuitem.

function ToggleAppleRemote()
{
  if (isListening) {
    gRemoteControlService.stopListening();
    isListening = false;
  }
  else {
    gRemoteControlService.startListening();
    isListening = true;
  }
}

//------------------------------------------------------------------------------
// Loading callback method when the pref pane loads

function onPrefPaneLoad()
{
}

