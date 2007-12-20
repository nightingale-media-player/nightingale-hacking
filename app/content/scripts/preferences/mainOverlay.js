/*
 //
// BEGIN SONGBIRD GPL
// 
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

addEventListener("load", function() {
  removeEventListener("load", arguments.callee, false);
  
  // switch to the Addons view if we're getting one installed
  if ("arguments" in window &&
      window.arguments[0] instanceof Components.interfaces.nsIDialogParamBlock &&
      window.arguments[1] instanceof Components.interfaces.nsIObserver) {

    var paneAddons = document.getElementById("paneAddons");

    function doStartup() {
      var frameAddons = document.getElementById("addonsFrame");
      frameAddons.contentWindow.arguments = window.arguments;
      frameAddons.contentWindow.Startup();

      paneAddons.removeEventListener("paneload", doStartup, false);
    }
    
    if (!paneAddons.loaded) {
      // this is needed in case the pane hasn't loaded yet and the pref window
      // loads it dynamically as we show it (in reality, this will happen all
      // the time, since this is the initial load)
      paneAddons.addEventListener("paneload", doStartup, false);
      document.documentElement.showPane(paneAddons);
    } else {
      // also pass to the internal frame if it is already loaded
      doStartup();
    }
  }
}, false);
