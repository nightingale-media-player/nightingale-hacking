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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const DESCRIPTION = "sbAboutCredits";
const CID         = "B6C04B7A-C640-4314-8093-C52370D05ECF";
const CONTRACTID  = "@mozilla.org/network/protocol/about;1?what=credits";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function sbAboutCredits() {
}
sbAboutCredits.prototype = {
  classDescription: DESCRIPTION,
  classID:          Components.ID(CID),
  contractID:       CONTRACTID,

  newChannel: function(uri) {
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var childURI =
      ioService.newURI("chrome://nightingale/content/html/aboutCredits.xhtml",
                       null, null);
    var channel = ioService.newChannelFromURI(childURI);
    channel.originalURI = uri;

    return channel;
  },

  getURIFlags: function(uri) {
    return Ci.nsIAboutModule.ALLOW_SCRIPT;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule])
}

var NSGetModule = XPCOMUtils.generateNSGetModule([sbAboutCredits]);
