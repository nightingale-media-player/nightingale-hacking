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

function ngLocalizableEqualizerPreset() {
    XPCOMUtils.defineLazyServiceGetter(this, "_gMM", "@songbirdnest.com/Songbird/Mediacore/Manager;1", "sbIMediacoreManager");
}

/**
 * \class ngLocalizableEqualizerPreset
 * \cid {1c4b8b2c-a2c1-4652-9fb3-523face810b9}
 * \contractid @getnightingale.com/equalizer-presets/localizable;1
 * \implements ngILocalizableEqualizerPreset
 */
ngLocalizableEqualizerPreset.prototype = {
    classDescription: "Basic mutable equalizer preset with localizable name",
    classID:          Components.ID("{1c4b8b2c-a2c1-4652-9fb3-523face810b9}"),
    contractID:       "@getnightingale.com/equalizer-presets/localizable;1",
    QueryInterface:   XPCOMUtils.generateQI([Components.interfaces.ngILocalizableEqualizerPreset]),

    _gMM: null,

    _values: null,
    _stringBundle: null,
    _property: null,

    get stringBundle() {
        return this._stringBundle;
    },
    set stringBundle(val) {
        this._stringBundle = val;
    },
    get property() {
        return this._property;
    },
    set property(val) {
        this._property = val;
    },
    get name() {
        if(this._stringBundle && this._property) {
            var name = this._property;
            try {
                name = this._stringBundle.GetStringFromName(this._property);
            }
            catch(e) {
                Components.utils.reportError(e);
            }
            finally {
                return name;
            }
        }
        else
            throw Components.results.NS_ERROR_NULL_POINTER;
    },
    
    get values() {
        return this._values;
    },
    setValues: function(aValues) {
        if(aValues instanceof Components.interfaces.nsIArray &&
           aValues.length == 10) //TODO shouldn't hardcode 10, though the equalizer component may not yet be initialized
            this._values = aValues;
        else
            throw Components.results.NS_ERROR_ILLEGAL_VALUE;
    }
};

var NSGetModule = XPCOMUtils.generateNSGetModule([ngLocalizableEqualizerPreset]);