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

function runTest() {
    var mainProvider = Cc["@getnightingale.com/equalizer-presets/main-provider;1"]
                        .getService(Ci.ngIMainEqualizerPresetProvider);

    assertTrue(mainProvider.presets instanceof Ci.nsIArray, "Presets aren't an nsIArray"); 
    var initialLength = mainProvider.presets.length;
    var array = [0,0.1,0.2,0.3,0.4,0.5,0.6,0.7,0.8,0.9];
    array = array.map(function(item) {
                var double = Cc["@mozilla.org/supports-double;1"]
                                .createInstance(Ci.nsISupportsDouble);
                double.data = item;
                return double;
            });
    mainProvider.savePreset("test", ArrayConverter.nsIArray(array));
    assertEqual(initialLength + 1, mainProvider.presets.length, "Preset isn't in the presets list after saving");

    // check if overwriting a preset works...
    mainProvider.savePreset("test", ArrayConverter.nsIArray(array));

    var collection = mainProvider.QueryInterface(Ci.ngIEqualizerPresetCollection);
    assertTrue(collection.hasPresetNamed("test"), "Preset isn't in the collection");
    assertTrue(collection.getPresetByName("test") instanceof Ci.ngIEqualizerPreset, "Returned preset doesn't implement ngIEqualizerPreset");

    mainProvider.deletePreset("test");
    assertTrue(!collection.hasPresetNamed("test"), "Preset wasn't deleted");

    // test persistency accross restarts?
    return;
}