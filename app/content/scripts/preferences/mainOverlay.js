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

    if (!paneAddons.loaded) {
      // this is needed in case the pane hasn't loaded yet and the pref window
      // loads it dynamically as we show it (in reality, this will happen all
      // the time, since this is the initial load)
      document.documentElement.showPane(paneAddons);
    }
  }

  // making the menus in the preference pane selector stack vertically to avoid multi-level tabs
  var browserPreferences = document.getElementById("BrowserPreferences");
  var selector = document.getAnonymousElementByAttribute(browserPreferences, 'anonid', 'selector');
  selector.removeAttribute("orient");
  if (getComputedStyle(selector, "").getPropertyPriority("visibility") == "") {
    // reset the visibility style, _unless_ it was !important
    selector.style.visibility = "visible";
  }

  // making the menus in the preference pane selector stack vertically to avoid multi-level tabs
  var browserPreferences = document.getElementById("BrowserPreferences");
  var selector = document.getAnonymousElementByAttribute(browserPreferences, 'anonid', 'selector');
  selector.removeAttribute("orient");
  if (getComputedStyle(selector, "").getPropertyPriority("visibility") == "") {
    // reset the visibility style, _unless_ it was !important
    selector.style.visibility = "visible";
  }
  
  // hide the CDRip prefpane if no cd-rip engines are installed
  var catMan = Cc["@mozilla.org/categorymanager;1"]
                 .getService(Ci.nsICategoryManager);
  var enum = catMan.enumerateCategory("cdrip-engine");
  if (!enum.hasMoreElements()) {
    var radio = document.getAnonymousElementByAttribute(browserPreferences,
                                                        'pane', 'paneCDRip');
    radio.hidden = true;
  }
}, false);



/*********************************************************
 HACK for Bug 13456 - document.loadOverlay is very buggy,
 and crashes when pref panes are loaded too quickly.  
 This is a workaround to prevent multiple simultaneous 
 loadOverlay operations.
 
 This code can be removed when we update to trunk Mozilla.
 ********************************************************/ 
 
// Replace document.loadOverlay with a version that
// prevents multiple loads
document._loadOverlay = document.loadOverlay;
document._overlayLoading = false;
document.loadOverlay = function(overlay, observer) {   
  if (document._overlayLoading) {
    throw new Error("HACK for Bug 13456 - Only one loadOverlay at a time!");
  }
  document._realLoadObserver = observer;
  document._overlayLoading = true;
  document._loadOverlay(overlay, 
                        document._loadOverlayObserver);
}
document._loadOverlayObserver = {
  observe: function() {
    // We just finished a load, notify the original observer
    if (document._realLoadObserver) {
      document._realLoadObserver.observe();
      document._realLoadObserver = null;
    }
    document._overlayLoading = false;
  }
}
