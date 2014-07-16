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
    var providerManager = Cc["@getnightingale.com/equalizer-presets/manager;1"]
                        .getService(Ci.ngIEqualizerPresetProviderManager);

    assertTrue(providerManager.providers.length >= 2, "Not all providers were registered");
    assertTrue(providerManager.presets.length >= 17, "Not all presets are offered by the manager");
    
    var presetName = providerManager.presets.enumerate().getNext()
                        .QueryInterface(Ci.ngIEqualizerPreset).name;

    var collection = providerManager.QueryInterface(Ci.ngIEqualizerPresetCollection);
    assertTrue(collection.hasPresetNamed(presetName),
               "Preset wasn't found even though it is in the presets array");
    assertTrue(collection.getPresetByName(presetName) instanceof Ci.ngIEqualizerPreset,
               "Preset returned by getPresetByName isn't actually a preset");
               
    var mainProvider = Cc["@getnightingale.com/equalizer-presets/main-provider;1"]
                        .getService(Ci.ngIMainEqualizerPresetProvider);
    // test regeneration of presets list when saving a preset
    var array = [0,0.1,0.2,-0.3,0.4,0.5,0.6,0.7,0.8,0.9];
    mainProvider.savePreset("test", convertArrayToSupportsDouble(array));
    assertTrue(collection.hasPresetNamed("test"),
                    "Presets list was not regenerated after a preset was saved");
                   
    var otherArray = [-0.1,0,0,0,0,0,0,0,0,0];
    mainProvider.savePreset("test", convertArrayToSupportsDouble(otherArray));
    
    var presetValues = ArrayConverter.JSArray(collection.getPresetByName("test").values);
    presetValues.forEach(function(item, i) {
        var value = item.QueryInterface(Ci.nsISupportsDouble).data;
        assertEqual(value, otherArray[i], "Preset was not overwritten correctly");
    });
    
    mainProvider.deletePreset("test");
    assertTrue(!collection.hasPresetNamed("test"),
                    "Delete preset was not removed from collection");
    
    return;
}


function convertArrayToSupportsDouble(array) {
    var ret = array.map(function(item) {
        var double = Cc["@mozilla.org/supports-double;1"]
                        .createInstance(Ci.nsISupportsDouble);
        double.data = item;
        return double;
    });
    return ArrayConverter.nsIArray(ret);
}
