/*
 //
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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

if (typeof(Sanitizer) != "undefined") {

  var gSongbirdSanitize = {
    onLoad: function() { gSongbirdSanitize.init(); },
    onUnload: function(event) { gSongbirdSanitize.reset(); },

    init: function() {
      if (document.documentElement.getAttribute("windowtype") == "Songbird:Core") {
        this.onSanitizerStartup();
      }
      window.removeEventListener('load', gSongbirdSanitize.onLoad, false);
      window.addEventListener('unload', gSongbirdSanitize.onUnload, false);
      
      var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                            .getService(Components.interfaces.nsIPrefBranch2);
      var guid = prefs.getComplexValue("songbird.library.web", 
                                        Components.interfaces.nsISupportsString);

      var libraryManager =
        Components.classes["@songbirdnest.com/Songbird/library/Manager;1"]
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
      window.removeEventListener('unload', gSongbirdSanitize.onUnload, false);
      if (document.documentElement.getAttribute("windowtype") == "Songbird:Core") {
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

  window.addEventListener('load', gSongbirdSanitize.onLoad, false);
}

