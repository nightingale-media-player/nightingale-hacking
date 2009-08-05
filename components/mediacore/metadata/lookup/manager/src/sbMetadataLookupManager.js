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

var Application = Cc["@mozilla.org/fuel/application;1"]
                    .getService(Ci.fuelIApplication);

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");

function sbMLM() {
  // initialise private variables
  this._providers = new Array;
  this._numProviders = 0;

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
    } catch (e) {
      dump("mlm // failed to register: " + contract + "\n" +
           "mlm //             reason: " + e + "\n");
    }
  }
  // define our getters for the attributes
  this.__defineGetter__('numProviders', function() {
      return this._numProviders;
  });
  this.__defineGetter__('defaultProvider', function() {
      if (this._numProviders == 0)
        throw Cr.NS_ERROR_NOT_AVAILABLE;
      else {
        // XXXstevel return the first provider for now
        // see bug 17364
        for each (var provider in this._providers) {
          return provider;
        }
      }
  });
}

sbMLM.prototype = {
  classDescription : 'Songbird Metadata Lookup Manager',
  classID : Components.ID('46733000-1dd2-11b2-8022-ba2da8a6a950'),
  contractID : '@songbirdnest.com/Songbird/MetadataLookup/manager;1',
  QueryInterface : XPCOMUtils.generateQI([Ci.sbIMetadataLookupManager]),

  // Return an nsISimpleEnumerator of all the providers registered with the
  // metadata lookup manager
  getProviders: function() {
    return ArrayConverter.enumerator(this._providers);
  },

  // Return the metadata provider of the passed in name
  getProvider: function(aProviderName) {
    if (this._providers[aProviderName])
      return this._providers[aProviderName];
    else
      throw Cr.NS_ERROR_NOT_AVAILABLE;
  },
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbMLM]);
}
