/*
 //
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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
var gNightingaleSanitizeOverlay = {
  checkBox: null,
  prefs: null,
  webLibrary: null,
  timeout: null,

  onLoad: function() { gNightingaleSanitizeOverlay.init(); },
  onUnload: function(event) { gNightingaleSanitizeOverlay.reset(); },
  onCommand: function(event) { gNightingaleSanitizeOverlay.onCheckboxChanged(); },

  init: function() {
    window.removeEventListener('load', gNightingaleSanitizeOverlay.onLoad, false);
    window.addEventListener('unload', gNightingaleSanitizeOverlay.onUnload, false);
    var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService(Components.interfaces.nsIPrefBranch2);
    var guid = prefs.getComplexValue("nightingale.library.web", 
                                      Components.interfaces.nsISupportsString);

    var libraryManager =
      Components.classes["@getnightingale.com/Nightingale/library/Manager;1"]
                .getService(Components.interfaces.sbILibraryManager);

    this.webLibrary = libraryManager.getLibrary(guid);
    this.webLibrary.addListener(this, 
                                false, 
                                Components.interfaces.sbIMediaList.LISTENER_FLAGS_ALL, 
                                null);

    var prefpane = document.getElementById("SanitizeDialogPane");
    if (prefpane) {
      var children = prefpane.childNodes;
      for (var i=0;i<children.length;i++) {
        if (children[i].tagName == "preferences" || children[i].tagName == "xul:preferences") {
          this.prefs = children[i];
        }
        if (!this.checkBox && (children[i].tagName == "checkbox" || children[i].tagName == "xul:checkbox")) {
          this.checkBox = document.createElement("checkbox");
          this.checkBox.setAttribute("id", "sanitize.mediaHistory");
          this.checkBox.setAttribute("label", SBString("sanitize.mediaHistory.label", "Web Media History"));
          this.checkBox.setAttribute("accesskey", SBString("sanitize.mediaHistory.accesskey", "W"));
          this.checkBox.setAttribute("oncommand", "gNightingaleSanitizeOverlay.onCommand();");
          this.updateDisabled();
          prefpane.insertBefore(this.checkBox, children[i]);
          continue;
        }
      }
    }
    if (this.checkBox && this.prefs) {
      this.checkBox.setAttribute("checked", this.prefs.rootBranch.getBoolPref("privacy.item.mediaHistory"));
    }
    
    var helpButton = document.documentElement.getButton("help");
    if (helpButton) {
      // remove the help button from the sanitize prefs dialog (not the do sanitize one)
      // Would be better to just set the dlgbuttons attribute, but that doesn't work
      helpButton.collapsed = true;
    }
  },

  updateDisabled: function() {
    this.checkBox.setAttribute("disabled", this.webLibrary.length > 0 ? "false" : "true");
  },

  reset: function() {
    this.clearUpdateTimeout();
    this.webLibrary.removeListener(this);
    window.removeEventListener('unload', gNightingaleSanitizeOverlay.onUnload, false);
  },
  
  onCheckboxChanged: function() {
    this.prefs.rootBranch.setBoolPref("privacy.item.mediaHistory", this.checkBox.checked);
  },
  
  onWebHistoryChanged: function() {
    this.clearUpdateTimeout();
    this.timeout = setTimeout(function(obj) { obj.updateTimeout(); }, 10, this);
  },
  
  clearUpdateTimeout: function() {
    if (this.timeout) { 
      clearTimeout(this.timeout); 
      this.timeout = null; 
    }
  },
  
  updateTimeout: function() {
    this.updateDisabled();
    this.timeout = null;
  },
  
  // mediaListListener
  onItemAdded: function(aMediaList, aMediaItem, aIndex) { gNightingaleSanitizeOverlay.onWebHistoryChanged(); return true; },
  onBeforeItemRemoved: function(aMediaList, aMediaItem, aIndex) { return true; },
  onAfterItemRemoved: function(aMediaList, aMediaItem, aIndex) { gNightingaleSanitizeOverlay.onWebHistoryChanged(); return true; },
  onItemUpdated: function(aMediaList, aMediaItem, aProperties) { return true; },
  onItemMoved: function(aMediaList, aFromIndex, aToIndex) { return true; },
  onBeforeListCleared: function(aMediaList, aExcludeLists) { return true; },
  onListCleared: function(aMediaList, aExcludeLists) { gNightingaleSanitizeOverlay.onWebHistoryChanged(); return true; },
  onBatchBegin: function(aMediaList) {},
  onBatchEnd: function(aMediaList) { gNightingaleSanitizeOverlay.onWebHistoryChanged(); },
  
  QueryInterface: function (aIID) {
    if (aIID.equals(Components.interfaces.nsISupports) ||
        aIID.equals(Components.interfaces.sbIMediaListListener))
    {
      return this;
    }
    throw Components.results.NS_ERROR_NO_INTERFACE;
  }

  
};

window.addEventListener('load', gNightingaleSanitizeOverlay.onLoad, false);
