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
const Cr = Components.results;
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/DebugUtils.jsm");

/**
 * To log this module, set the following environment variable:
 *   NSPR_LOG_MODULES=ngEqualizerPresetProviderManager:5
 */
const LOG = DebugUtils.generateLogFunction("ngEqualizerPresetProviderManager", 2);

const PRESETS_CHANGED_TOPIC = "equalizer-presets-changed";

/**
 * \class ngEqualizerPresetProviderManager
 * \cid {5e503ed9-1c8c-4135-8d25-61b0835475b4}
 * \contractid @getnightingale.com/equalizer-presets/manager;1
 * \implements ngIEqualizerPresetProviderManager
 * \implements ngIEqualizerPresetCollection
 */
function ngEqualizerPresetProviderManager() {
    LOG("Initializing equalizer preset provider manager");
    this._providers = [];
    this._presets = [];
    XPCOMUtils.defineLazyServiceGetter(this, "_mainProvider",
             "@getnightingale.com/equalizer-presets/main-provider;1",
             "ngIMainEqualizerPresetProvider");
    XPCOMUtils.defineLazyServiceGetter(this, "_observerService",
                    "@mozilla.org/observer-service;1", "nsIObserverService");
}

ngEqualizerPresetProviderManager.prototype = {
    classDescription: "Manages equalizer preset providers",
    classID:          Components.ID("{fead9fed-346b-49f1-94ca-21edd9e5fbf8}"),
    contractID:       "@getnightingale.com/equalizer-presets/manager;1",
    QueryInterface:   XPCOMUtils.generateQI([Ci.ngIEqualizerPresetProviderManager,
                                             Ci.ngIEqualizerPresetCollection,
                                             Ci.nsIObserver]),
    _xpcom_categories: [{
        category: 'app-startup',
        entry: "ngEqualizerPresetProviderManager",
        service: true
    }],
    _observerService: null,

    observe: function(aSubject, aTopic, aData) {
        if(aTopic == "app-startup") {
            this._observerService.addObserver(this, "profile-after-change", false);
        }
        else if(aTopic == "profile-after-change") {
            this._observerService.removeObserver(this, "profile-after-change");

            LOG("Adding the default presets");
            var dProvider = Cc["@getnightingale.com/equalizer-presets/defaults;1"]
                                .getService(Ci.ngIEqualizerPresetProvider);
            if(!this._providers.length)
                this.registerPresetProvider(dProvider);
            else
                this._providers.splice(0, 0, dProvider);

            LOG("Adding the user set presets");
            // those get added secondly, so they override the default presets.
            this._addPresetsFromProvider(this._mainProvider);

            this._observerService.addObserver(this, "equalizer-preset-saved", false);
            this._observerService.addObserver(this, "equalizer-preset-deleted", false);
        }
        else if(aTopic == "equalizer-preset-saved"
                 || aTopic == "equalizer-preset-deleted") {
            this._regeneratePresetsList();
        }
    },

    _providers: null,
    _mainProvider: null,
    _presets: null,
    
    get providers() {
        if(this._providers) {
            LOG("Adding the main provider to the presets and returning them in reverse.");
            var providers = this._providers;
            // make sure the main provider is the first one in the array (reverse in the nsIArray generation)
            providers.push(this._mainProvider.QueryInterface(Ci.ngIEqualizerPresetProvider));
            return ArrayConverter.nsIArray(providers.reverse());
        }
        else
            throw Cr.NS_ERROR_UNEXPECTED;
    },
    get presets() {
        if(this._presets)
            return ArrayConverter.nsIArray(this._presets);
        else
            throw Cr.NS_ERROR_UNEXPECTED;
    },
    getPresetByName: function(aName) {
        var preset = null;
        this._presets.some(function(p) {
            if(p.name == aName) {
                preset = p;
                return true;
            }
            return false;
        });
        return preset;
    },
    hasPresetNamed: function(aName) {
        return this.getPresetByName(aName) != null;
    },
    registerPresetProvider: function(aNewProvider) {
        if(this._providers.indexOf(aNewProvider) == -1 ) {
            /** \todo check that we're not getting the main provider. */

            LOG("Adding a new equalizer provider");            
            this._providers.push(aNewProvider);

            this._addPresetsFromProvider(aNewProvider);
            LOG("Nearly done adding the provider, just need to notify observers!");
            this._observerService.notifyObservers(this.presets, PRESETS_CHANGED_TOPIC, null);
        }
        else {
            LOG("Provider already registered");
        }
    },
    unregisterPresetProvider: function(aProvider) {
        var index = this._providers.indexOf(aProvider);
        // > 0, so the default preset provider can not be removed.
        if(index > 0) {
            LOG("Removed equalizer preset provider");
            this._providers.splice(index, 1);
        }
        this._regeneratePresetsList();
    },
    
    _regeneratePresetsList: function() {
        LOG("Regenerating presets array");
        this._presets = [];
        this._providers.forEach(function(provider) {
            this._addPresetsFromProvider(provider);
        }, this);
        this._addPresetsFromProvider(this._mainProvider);
        this._presets.sort(function(a, b) {
            return a.name.localeCompare(b.name);
        });
        
        this._observerService.notifyObservers(this.presets, PRESETS_CHANGED_TOPIC, null);
    },
    _addPresetsFromProvider: function(aProvider) {
        LOG("Adding presets from provider");
        var providerPresets = aProvider.presets.enumerate(),
            preset;
        while(providerPresets.hasMoreElements()) {
            preset = providerPresets.getNext().QueryInterface(Ci.ngIEqualizerPreset);
            // Add a preset if there is no user set one.
            if(aProvider instanceof Ci.ngIMainEqualizerPresetProvider ||
               (this._mainProvider &&
                !this._mainProvider.QueryInterface(Ci.ngIEqualizerPresetCollection)
                    .hasPresetNamed(preset.name)))
                this._presets.push(preset);
        }
        LOG("Presets added");
    }
};

var NSGetModule = XPCOMUtils.generateNSGetModule([ngEqualizerPresetProviderManager]);