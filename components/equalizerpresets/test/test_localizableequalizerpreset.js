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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

if (typeof(Cc) == "undefined")
  this.Cc = Components.classes;
if (typeof(Ci) == "undefined")
  this.Ci = Components.interfaces;
if(!Cr)
    this.Cr = Components.results;

function runTest() {
    var localizablePreset = Cc["@getnightingale.com/equalizer-presets/localizable;1"]
                        .createInstance(Ci.ngILocalizableEqualizerPreset);

    assertEqual(localizablePreset.values, null, "Values are already defined");

    var stringBundle = Cc["@mozilla.org/intl/stringbundle;1"]
                       .getService(Ci.nsIStringBundleService)
                       .createBundle("chrome://songbird/locale/songbird.properties");

    localizablePreset.property = "equalizer.preset.rock"
    localizablePreset.stringBundle = stringBundle;
    assertEqual(localizablePreset.name,
                stringBundle.GetStringFromName("equalizer.preset.rock"),
                "Name not set correctly");

    localizablePreset.setValues(ArrayConverter.nsIArray([0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9]));
    assertTrue(localizablePreset.values, "Values not defined even though previously set");
    assertEqual(localizablePreset.values.length, 10, "Not all values were written into the array");

    return;
}
