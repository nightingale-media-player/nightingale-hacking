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
 
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Ce = Components.Exception;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function JobProgressService() {
}
JobProgressService.prototype = {
  QueryInterface          : XPCOMUtils.generateQI(
      [Ci.sbIJobProgressService, Ci.nsIClassInfo]),
  classDescription        : 'Songbird Job Progress Service Implementation',
  classID                 : Components.ID("{afef6b00-90d5-11dd-ad8b-0800200c9a66}"),
  contractID              : "@songbirdnest.com/Songbird/JobProgressService;1",
  flags                   : Ci.nsIClassInfo.MAIN_THREAD_ONLY,
  implementationLanguage  : Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage    : function(aLanguage) { return null; },
  getInterfaces : function(count) {
    var interfaces = [Ci.sbIJobProgressService,
                      Ci.nsIClassInfo,
                      Ci.nsISupports
                     ];
    count.value = interfaces.length;
    return interfaces;
  },

  showProgressDialog: function(aJobProgress, aWindow, aTimeout) {
    if (typeof(SBJobUtils) == "undefined") {
      Cu.import("resource://app/jsmodules/SBJobUtils.jsm");
    }
    // Delegate!
    SBJobUtils.showProgressDialog(aJobProgress, aWindow, aTimeout);
  }
}


var NSGetFactory = XPCOMUtils.generateNSGetFactory([JobProgressService]);
