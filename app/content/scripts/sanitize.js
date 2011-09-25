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

if (typeof(Sanitizer) != "undefined") {

  var gNightingaleSanitize = {
    onLoad: function() { gNightingaleSanitize.init(); },
    onUnload: function(event) { gNightingaleSanitize.reset(); },

    init: function() {
      if (document.documentElement.getAttribute("windowtype") == "Nightingale:Core") {
        this.onSanitizerStartup();
      }
      window.removeEventListener('load', gNightingaleSanitize.onLoad, false);
      window.addEventListener('unload', gNightingaleSanitize.onUnload, false);
      
      var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefBranch2);
      var guid = prefs.getComplexValue("nightingale.library.web", 
                                        Components.interfaces.nsISupportsString);

      var libraryManager =
        Components.classes["@getnightingale.com/Nightingale/library/Manager;1"]
                  .getService(Components.interfaces.sbILibraryManager);

      var webLibrary = libraryManager.getLibrary(guid);
      Sanitizer.prototype.items["mediaHistory"] = {
        clear: function ()
        {
          webLibrary.clear();
        },
        
        get canClear()
        {
          return webLibrary.length > 0;
        }
      };
      if (typeof(gSanitizePromptDialog) != "undefined") {
        var oldfn = gSanitizePromptDialog.sanitize;
        gSanitizePromptDialog.sanitize = function() {
          oldfn();
          if (document.getElementById("sanitize.mediaHistory").checked) {
            Sanitizer.prototype.items["mediaHistory"].clear();
          }
        };
      }
    },

    reset: function() {
      window.removeEventListener('unload', gNightingaleSanitize.onUnload, false);
      if (document.documentElement.getAttribute("windowtype") == "Nightingale:Core") {
        this.onSanitizerShutdown();
      }
    },
    
    onSanitizerStartup: function() {
      Sanitizer.onStartup();
      Sanitizer.prefs.setBoolPref(Sanitizer.prefDidShutdown, false);
    },

    onSanitizerShutdown: function() {
      Sanitizer.onShutdown();
    },
    
  };

  window.addEventListener('load', gNightingaleSanitize.onLoad, false);
}

