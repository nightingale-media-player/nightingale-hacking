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

EXPORTED_SYMBOLS = ["StorageFormatter"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results

const BASE = 1024;

var gMagnitudes = ["b", "kb", "mb", "gb"];

var gLongUnits = {
 b:  "bytes",
 kb: "kilobytes",
 mb: "megabytes",
 gb: "gigabytes"
};

var gShortUnits = {
 b:  "B",
 kb: "KB",
 mb: "MB",
 gb: "GB"
};

var StorageFormatter = {

  B: "b",
  KB: "kb",
  MB: "mb",
  GB: "gb",

  get _bundle() {
    var src = "chrome://songbird/locale/songbird.properties";
    var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                                .getService(Ci.nsIStringBundleService);
    var bundle = stringBundleService.createBundle(src);

    delete this._bundle;
    this._bundle = bundle;
    return this._bundle;
  },

  get longUnits() {
    var longUnits = this._translate(gLongUnits);

    delete this.longUnits;
    this.longUnits = longUnits;
    return this.longUnits;
  },

  get shortUnits() {
    var shortUnits = this._translate(gShortUnits);

    delete this.shortUnits;
    this.shortUnits = shortUnits;
    return this.shortUnits;
  },

  format: function(bytes) {

    if (bytes < 0) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    var magnitude = 0;
    var scaled = 0;
    if (bytes > 0) {
      magnitude = gMagnitudes.length;
      for (; magnitude >= 0; magnitude--) {
        if (bytes >= Math.pow(BASE, magnitude)) {
          break;
        }
      }

      scaled = bytes / Math.pow(BASE, magnitude);
      // Round the value to a single decimal point
      scaled = scaled.toFixed(1);
    }

    scaled += " " + this.shortUnits[gMagnitudes[magnitude]];
    return scaled;
  },

  _translate: function(units) {
    for (var key in units) {
      try {
        units[key] =
          this._bundle.GetStringFromName("storageformatter." + units[key]);
      }
      catch (e) {
        // Ignore the exception and leave the default alone
      }
    }
    return units;
  }
}

