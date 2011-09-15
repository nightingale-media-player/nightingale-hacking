/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the "GPL").
 *
 * Software distributed under the License is distributed
 * on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

const Cc = Components.classes;
const CC = Components.Constructor;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");

function sbMLM() {
  // initialise private variables
  this._providers = {};
  this._numProviders = 0;

  this._defaultProvider = null;
  // enumerate the catman for all the metadata-lookup managers
  var catMgr = Cc["@mozilla.org/categorymanager;1"]
                 .getService(Ci.nsICategoryManager);
  var e = catMgr.enumerateCategory('metadata-lookup');
  while (e.hasMoreElements()) {
    var key = e.getNext().QueryInterface(Ci.nsISupportsCString);
    var contract = catMgr.getCategoryEntry('metadata-lookup', key);
    try {
      var provider = Cc[contract].getService(Ci.sbIMetadataLookupProvider);
      this._providers[provider.name] = provider;
      this._numProviders++;

      // always make the first one we find the default... by default
      if (!this._defaultProvider)
        this._defaultProvider = provider;
      else {
        // multiple providers
        // if the current provider has a lower weight than the currently
        // set default provider, then change the default to be this one
        if (provider.weight < this._defaultProvider.weight)
          this._defaultProvider = provider;
      }
    } catch (e) {
      dump("mlm // failed to register: " + contract + "\n" +
           "mlm //             reason: " + e + "\n");
    }
  }

  Cc["@mozilla.org/observer-service;1"]
    .getService(Ci.nsIObserverService)
    .addObserver(this, "songbird-library-manager-before-shutdown", false);
}

sbMLM.prototype = {
  classDescription : 'Songbird Metadata Lookup Manager',
  classID : Components.ID('46733000-1dd2-11b2-8022-ba2da8a6a950'),
  contractID : '@songbirdnest.com/Songbird/MetadataLookup/manager;1',
  QueryInterface : XPCOMUtils.generateQI([Ci.sbIMetadataLookupManager,
                                          Ci.nsIObserver]),

  _providers: null,
  _numProviders: 0,
  _defaultProvider: null,

  // Return an nsISimpleEnumerator of all the providers registered with the
  // metadata lookup manager
  getProviders: function() {
    return ArrayConverter.enumerator([p for each (p in this._providers)]);
  },

  // Return the metadata provider of the passed in name
  getProvider: function(aProviderName) {
    if (this._providers[aProviderName])
      return this._providers[aProviderName];
    else
      throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  get numProviders() this._numProviders,

  get defaultProvider() {
    if (this._numProviders == 0)
      throw Cr.NS_ERROR_NOT_AVAILABLE;
    else {
      // check to see if the user has specified a default provider in their
      // preferences.  if so, return that one.
      var Application = Cc["@mozilla.org/fuel/application;1"]
                          .getService(Ci.fuelIApplication);
      var userDefault = Application.prefs.getValue(
                              "metadata.lookup.default_provider", "");
      if (userDefault && this._providers[userDefault]) {
        return this._providers[userDefault];
      } else {
        // if the specified default doesn't exist, or no preference has been
        // set, then return our default one as we scanned it
        return this._defaultProvider;
      }
    }
  },

  observe: function sbMLM_observe(aSubject, aTopic, aData) {
    if (aTopic == "songbird-library-manager-before-shutdown") {
      // clean up providers list; this is necessary because the service shuts
      // down way too late
      Cc["@mozilla.org/observer-service;1"]
        .getService(Ci.nsIObserverService)
        .removeObserver(this, "songbird-library-manager-before-shutdown");
      this._providers = {};
      this._numProviders = 0;
      this._defaultProvider = null;
    }
  },
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbMLM]);
}
