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

"use strict";
const Cc = Components.classes;
const Cu = Components.utils;
const Ci = Components.interfaces;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/DebugUtils.jsm");

const LOG = DebugUtils.generateLogFunction("ngDefaultPresetProvider", 2);

/**
 * \class ngDefaultPresetProvider
 * \cid {1f3b39da-c169-49df-af70-eaec9c10640a}
 * \contractid @getnightingale.com/equalizer-presets/defaults;1
 " \implements ngIEqualizerPresetProvider
 */
function ngDefaultPresetProvider() {
    LOG("Constructing");

    var stringBundle = Cc["@mozilla.org/intl/stringbundle;1"]
                       .getService(Ci.nsIStringBundleService)
                       .createBundle("chrome://songbird/locale/songbird.properties");
    this._presets = [];

    this._presets.push(this._createPreset("songbird.eq.currentPreset", stringBundle, [0, 0, 0, 0, 0, 0, 0, 0, 0, 0]));
    this._presets.push(this._createPreset("equalizer.preset.classical", stringBundle, [0, 0, 0, 0, 0, 0, -0.2, -0.2, -0.2, -0.4]));
    this._presets.push(this._createPreset("equalizer.preset.club", stringBundle, [0, 0, 0.15, 0.2, 0.2, 0.2, 0.15, 0, 0, 0]));
    this._presets.push(this._createPreset("equalizer.preset.dance", stringBundle, [0.5, 0.25, 0.05, 0, 0, -0.2, -0.3, -0.3, 0, 0]));
    this._presets.push(this._createPreset("equalizer.preset.full_bass", stringBundle, [0.4, 0.4, 0.4, 0.2, 0, -0.2, -0.3, -0.35, -0.4, -0.4]));
    this._presets.push(this._createPreset("equalizer.preset.full_treble", stringBundle, [-0.4, -0.4, -0.4, -0.15, 0.1, 0.4, 0.8, 0.8, 0.8, 0.8]));
    this._presets.push(this._createPreset("equalizer.preset.small_speakers", stringBundle, [0.2, 0.4, 0.2, -0.2, -0.15, 0, 0.2, 0.4, 0.6, 0.7]));
    this._presets.push(this._createPreset("equalizer.preset.large_hall", stringBundle, [0.45, 0.45, 0.2, 0.2, 0, -0.2, -0.2, -0.2, 0, 0]));
    this._presets.push(this._createPreset("equalizer.preset.live", stringBundle, [-0.2, 0, 0.15, 0.2, 0.2, 0.2, 0.1, 0.05, 0.05, 0]));
    this._presets.push(this._createPreset("equalizer.preset.party", stringBundle, [0.25, 0.25, 0, 0, 0, 0, 0, 0, 0.25, 0.25]));
    this._presets.push(this._createPreset("equalizer.preset.pop", stringBundle, [-0.15, 0.15, 0.2, 0.25, 0.15, -0.15, -0.15, -0.15, -0.1, -0.1]));
    this._presets.push(this._createPreset("equalizer.preset.reggae", stringBundle, [0, 0, -0.1, -0.2, 0, 0.2, 0.2, 0, 0, 0]));
    this._presets.push(this._createPreset("equalizer.preset.rock", stringBundle, [0.3, 0.15, -0.2, -0.3, -0.1, 0.15, 0.3, 0.35, 0.35, 0.35]));
    this._presets.push(this._createPreset("equalizer.preset.ska", stringBundle, [-0.1, -0.15, -0.12, -0.05, 0.15, 0.2, 0.3, 0.3, 0.4, 0.3]));
    this._presets.push(this._createPreset("equalizer.preset.soft", stringBundle, [0.2, 0, -0.1, -0.15, -0.1, 0.2, 0.3, 0.35, 0.4, 0.5]));
    this._presets.push(this._createPreset("equalizer.preset.soft_rock", stringBundle, [0.2, 0.2, 0, -0.1, -0.2, -0.3, -0.2, -0.1, 0.2, 0.4]));
    this._presets.push(this._createPreset("equalizer.preset.techno", stringBundle, [0.3, 0.25, 0, -0.25, -0.2, 0, 0.3, 0.35, 0.35, 0.3]));
}

ngDefaultPresetProvider.prototype = {
    classDescription: "Preset provider that provides the default equalizer presets",
    classID:          Components.ID("{1f3b39da-c169-49df-af70-eaec9c10640a}"),
    contractID:       "@getnightingale.com/equalizer-presets/defaults;1",
    QueryInterface:   XPCOMUtils.generateQI([Ci.ngIEqualizerPresetProvider]),

    _presets: null,

    _createPreset: function(property, strBundle, values) {
        LOG("_createPreset("+property+", "+strBundle+", "+values+")");
        var preset = Cc["@getnightingale.com/equalizer-presets/localizable;1"]
                        .createInstance(Ci.ngILocalizableEqualizerPreset);
        preset.stringBundle = strBundle;
        preset.property = property;
        preset.setValues(ArrayConverter.nsIArray(
                            this._makeArraySupportDouble(values)));

        return preset;
    },
    _makeArraySupportDouble: function(array) {
        return array.map(function(item) {
            var double = Cc["@mozilla.org/supports-double;1"]
                            .createInstance(Ci.nsISupportsDouble);
            double.data = item;
            return double;
        });
    },
    
    get presets() {
        LOG("presets");
        if(this._presets)
            return ArrayConverter.nsIArray(this._presets);
        else
            throw Components.results.NS_ERROR_UNEXPECTED;
    }
};

var NSGetModule = XPCOMUtils.generateNSGetModule([ngDefaultPresetProvider]);
