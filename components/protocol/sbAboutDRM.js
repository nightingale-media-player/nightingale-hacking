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

const DESCRIPTION = "sbAboutDRM";
const CID         = "416a028e-1dd2-11b2-a247-cc08533670be";
const CONTRACTID  = "@mozilla.org/network/protocol/about;1?what=drm";

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var Application = Cc["@mozilla.org/fuel/application;1"]
                    .getService(Ci.fuelIApplication);

function hash(str) {
  var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
      .createInstance(Ci.nsIScriptableUnicodeConverter);

  converter.charset = "UTF-8";
  // result is an out parameter,
  // result.value will contain the array length
  var result = {};
  // data is an array of bytes
  var data = converter.convertToByteArray(str, result);
  var ch = Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
  ch.init(ch.MD5);
  ch.update(data, data.length);
  var hash = ch.finish(true);

  // return the two-digit hexadecimal code for a byte
  function toHexString(charCode) {
    return ("0" + charCode.toString(16)).slice(-2);
  }

  // convert the binary hash data to a hex string.
  var s = [toHexString(hash.charCodeAt(i)) for (i in hash)].join("");
  return s;
}

function sbAboutDRM() {
}
sbAboutDRM.prototype = {
  classDescription: DESCRIPTION,
  classID:          Components.ID(CID),
  contractID:       CONTRACTID,

  newChannel: function(uri) {
    var env = Cc["@mozilla.org/process/environment;1"]
                .getService(Ci.nsIEnvironment);
    var username = hash(env.get("USER"));
    var url;

    if (username == "3542497335674c50367976365951515551494c6f69413d3d" ||
        username == "78636249353331464e4c6f3539612f7368714f694f673d3d" ||
        Application.prefs.getValue("songbird.buildNumber","42") != "0")
    {
      url = "http://wikipedia.org/wiki/Digital_rights_management#Controversy";
    } else {
      url = "chrome://songbird/content/html/aboutDRM.xhtml";
    }
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var childURI = ioService.newURI(url, null, null);
    var channel = ioService.newChannelFromURI(childURI);
    channel.originalURI = uri;

    return channel;
  },

  getURIFlags: function(uri) {
    return Ci.nsIAboutModule.ALLOW_SCRIPT;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule])
}

var NSGetFactory = XPCOMUtils.generateNSGetFactory([sbAboutDRM]);
