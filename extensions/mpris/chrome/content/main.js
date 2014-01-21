/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
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

/*
 * Written by Logan F. Smyth Ĺ  2009
 * http://logansmyth.com
 * me@logansmyth.com
 */


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm"); 

// Make a namespace.
if (typeof Mpris == 'undefined') {
  var Mpris = {};
}


Mpris.Controller = {
  prefs: null,
  debug_mode: false,
  handler: null,

  onLoad: function() {
    this.handler = Components.classes['@getnightingale.com/Nightingale/Mpris;1'].createInstance(Components.interfaces.ngIMprisPlugin);

    this.prefs = Components.classes["@mozilla.org/preferences-service;1"].getService(Components.interfaces.nsIPrefService).getBranch("mpris.");
    this.prefs.QueryInterface(Components.interfaces.nsIPrefBranch2);
    this.prefs.addObserver("", this, false);
    this.debug_mode = this.prefs.getBoolPref("debug_mode");
  
    this.handler.init(this.debug_mode);
    //~ this.handler.dbus.setDebugMode(this.debug_mode);

  },
  onUnLoad: function() {
    this.prefs.removeObserver("", this);
  },
  
  observe: function(subject, topic, data) {
     if (topic != "nsPref:changed") return;
     switch(data) {
       case "debug_mode":
         this.debug_mode = this.prefs.getBoolPref("debug_mode");
	 
	 this.handler.debug_mode = this.debug_mode;
	 this.handler.dbus.debug_mode = this.debug_mode;
         break;
     }
   },

};

window.addEventListener("load", function(e) { Mpris.Controller.onLoad(e); }, false);
window.addEventListener("unload", function(e) { Mpris.Controller.onUnLoad(e); }, false);