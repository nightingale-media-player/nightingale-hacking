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

const Cu = Components.utils;

var XUL_NS = XUL_NS ||
             "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

Cu.import("resource://app/jsmodules/ArrayConverter.jsm");

function runTest () {
    var url = "data:application/vnd.mozilla.xul+xml," +
              "<?xml-stylesheet href='chrome://global/skin' type='text/css'?>" +
              "<?xml-stylesheet href='chrome://songbird/content/bindings/bindings.css' type='text/css'?>" +
              "<window xmlns='"+XUL_NS+"'/>";

    beginWindowTest(url, setup);
}

function setup () {

    testWindow.resizeTo(200,200);

    var document = testWindow.document;

    function createElement(aType) {
        var element = document.createElementNS(XUL_NS, aType);
        element.setAttribute("flex", "1");
        document.documentElement.appendChild(element);
        return element;
    }

    continueWindowTest(test_save, [ createElement("ng-eq-preset-list") ]);
}

function test_save(aPresetList) {
    var currentPreset = aPresetList.preset;
    aPresetList.save();
    assertEqual(currentPreset, aPresetList.preset, "Saving the preset changed its name");
    var resetButton = aPresetList.ownerDocument.getAnonymousElementByAttribute(aPresetList, "anonid", "eq-preset-reset");
    assertTrue(!resetButton.hasAttribute("disabled") && !resetButton.hasAttribute("hidden"), "Reset button hasn't been activated")
    
    continueWindowTest(test_delete,  [ aPresetList ]);
}

function test_delete(aPresetList) {
    var currentPreset = aPresetList.preset;
    aPresetList.delete();
    assertEqual(currentPreset, aPresetList.preset, "Deleting the preset changed its name (we saved an already existing preset and then deleted the custom version of it)");
    var resetButton = aPresetList.ownerDocument.getAnonymousElementByAttribute(aPresetList, "anonid", "eq-preset-reset");
    assertTrue(resetButton.hasAttribute("disabled") && resetButton.hasAttribute("hidden"), "Reset button hasn't been disabled")
    
    continueWindowTest(test_change,  [ aPresetList ]);
}

function test_change(aPresetList) {
    var eqPresets = ArrayConverter.JSArray(Cc["@getnightingale.com/equalizer-presets/manager;1"]
                    .getService(Ci.ngIEqualizerPresetProviderManager).presets),
        preset = aPresetList.preset,
        newPreset;
    assertTrue(eqPresets.some(function(item) {
        if(item.QueryInterface(Ci.ngIEqualizerPreset).name != preset) {
            newPreset = item.QueryInterface(Ci.ngIEqualizerPreset).name;
            return true;
        }
        return false;
    }), "No eq preset with a different name than the current name was found");
    aPresetList.preset = newPreset;
    assertEqual(aPresetList.preset, newPreset, "EQ preset was not applied successfully");

    Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
        .getService(Ci.sbIMediacoreManager).equalizer.currentPresetName = preset;
    assertEqual(aPresetList.preset, preset, "EQ preset change did not populate to GUI");
    
    endWindowTest();
}