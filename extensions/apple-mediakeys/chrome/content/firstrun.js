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

// Make a namespace.
if (typeof AppleMediaKeysFirstRun == 'undefined') {
  var AppleMediaKeysFirstRun = {};
}

/**
 * UI controller that is loaded into the main player window
 */
AppleMediaKeysFirstRun.Controller = {

  /**
   * Called when the window finishes loading
   */
  onLoad: function() {
    
    // initialization code
    this._initialized = true;
    this._strings = document.getElementById("apple-mediakeys-strings");
       
    // Perform extra actions the first time the extension is run
    if (Application.prefs.get("extensions.apple-mediakeys.firstrun").value) {
      Application.prefs.setValue("extensions.apple-mediakeys.firstrun", false);
      this._firstRunSetup();
    }
         
  },
  
  /**
   * Perform extra setup the first time the extension is run
   */
  _firstRunSetup : function() {
  
    // Call this.firstrun() after a 3 second timeout
    setTimeout(function(controller) { controller.firstrun(); }, 3000, this); 
  
  },
  
  firstrun : function() {
    var welcomeTitle = this._strings.getString("WelcomeTitle");
    var welcomeMsg = this._strings.getString("WelcomeMessage");
    var promptService = Cc['@mozilla.org/embedcomp/prompt-service;1']
                          .getService(Ci.nsIPromptService);
    promptService.alert(null, welcomeTitle, welcomeMsg);
    },
  
};

window.addEventListener("load", function(e) { AppleMediaKeysFirstRun.Controller.onLoad(e); }, false);