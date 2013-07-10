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

// This is the mapping from about:<module> to actual URI loaded.  Just changing
// this part should be enough to register new things.
const PAGES = {
  "keyboard-help": "chrome://songbird/content/html/keyboardHelp.xhtml"
};

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "Songbird about: protocol redirector";
const CID         = "42631c40-1cbe-4f3a-b859-7936e542130f";
const CONTRACTID  = "@songbirdnest.com/aboutRedirector";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const PROTOCOL_PREFIX = "@mozilla.org/network/protocol/about;1?what=";

// XXX How to register factory w/ protocol_prefix?
// 
// function doPostRegister(aCompMgr, aFile, aComponents) {
//   // get our own factory, since XPCOMUtils doesn't expose it
//   var factory = aCompMgr.getClassObject(sbAboutRedirector.prototype.classID,
//                                         Ci.nsIFactory);
//   for (let key in PAGES) {
//     aCompMgr.registerFactory(sbAboutRedirector.prototype.classID,
//                              sbAboutRedirector.prototype.classDescription,
//                              PROTOCOL_PREFIX + key,
//                              factory);
//   }
// }

function sbAboutRedirector() {}
sbAboutRedirector.prototype = {
  classDescription: DESCRIPTION,
  classID:          Components.ID(CID),
  contractID:       CONTRACTID,
 
  newChannel: function(uri) {
    if (!(uri.path in PAGES)) {
      throw Components.Exception("URI " + uri.spec + " not supported",
                                 Components.results.NS_ERROR_UNEXPECTED);
    }
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    var childURI = ios.newURI(PAGES[uri.path], null, null);
    var channel = ios.newChannelFromURI(childURI);
    channel.originalURI = uri;
    return channel;
  },
  
  getURIFlags: function(uri) {
    return Ci.nsIAboutModule.ALLOW_SCRIPT
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule])
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbAboutRedirector]);
