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

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

/**
 * \class ngMutableEqualizerPreset
 * \cid {f87e3ba3-c3fe-437a-8b2f-3aeb48536a9b}
 * \contractid @getnightingale.com/equalizer-presets/mutable;1
 * \implements ngIMutableEqualizerPreset
 */
function ngMutableEqualizerPreset() {
    XPCOMUtils.defineLazyServiceGetter(this, "_gMM", "@songbirdnest.com/Songbird/Mediacore/Manager;1", "sbIMediacoreManager");
}

ngMutableEqualizerPreset.prototype = {
    classDescription: "Basic mutable equalizer preset",
    classID:          Components.ID("{f87e3ba3-c3fe-437a-8b2f-3aeb48536a9b}"),
    contractID:       "@getnightingale.com/equalizer-presets/mutable;1",
    QueryInterface:   XPCOMUtils.generateQI([Components.interfaces.ngIMutableEqualizerPreset]),

    _gMM: null,

    _name: null,
    _values: null,

    get name() {
        return this._name;
    },
    setName: function(aName) {
        this._name = aName;
    },
    get values() {
        return this._values;
    },
    setValues: function(aValues) {
        if(aValues instanceof Components.interfaces.nsIArray &&
           aValues.length == 10) //TODO shouldn't hardcode 10, though the equalizer component my not yet be initialized
            this._values = aValues;
        else
            throw Components.results.NS_ERROR_ILLEGAL_VALUE;
    }
};

var NSGetModule = XPCOMUtils.generateNSGetModule([ngMutableEqualizerPreset]);