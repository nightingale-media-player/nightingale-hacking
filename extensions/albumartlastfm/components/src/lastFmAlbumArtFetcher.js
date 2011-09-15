/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
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

/* This XPCOM service knows how to ask Last.fm where to find art for 
 * music albums. 
 */

const Cc = Components.classes;
const CC = Components.Constructor;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

// The root of our preferences branch
const PREF_BRANCH = "extensions.albumart.lastfm.";

// Importing helper modules
Cu.import('resource://app/jsmodules/sbProperties.jsm');
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/sbCoverHelper.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

/**
 * Since we can't use the FUEL components until after all other components have
 * been loaded we define a lazy getter here for when we need it.
 */
__defineGetter__("Application", function() {
  delete this.Application;
  return this.Application = Cc["@mozilla.org/fuel/application;1"]
                              .getService(Ci.fuelIApplication);
});

// This is the fetcher module, you will need to change the name
// sbLastFMAlbumArtFetcher to you own in all instances.
function sbLastFMAlbumArtFetcher() {
  // Use the last FM web api to make things easier.
  this._lastFMWebApi = Cc['@songbirdnest.com/Songbird/webservices/last-fm;1']
                         .getService(Ci.sbILastFmWebServices);
  this._strings = Cc["@mozilla.org/intl/stringbundle;1"]
                    .getService(Ci.nsIStringBundleService)
                    .createBundle("chrome://albumartlastfm/locale/albumartlastfm.properties");
};
sbLastFMAlbumArtFetcher.prototype = {
  // XPCOM Magic
  className: 'lastFMAlbumArtFetcher',
  classDescription: 'LastFM Album Cover Fetcher',
  classID: Components.ID('{8569316f-13a0-44d8-9d08-800999ed1f1c}'),
  contractID: '@songbirdnest.com/album-art/lastfm-fetcher;1',
  _xpcom_categories: [{
    category: "songbird-album-art-fetcher"
  }],

  // Variables
  _shutdown: false,
  _albumArtSourceList: null,
  _isFetching: false,


  _findImageForItem: function(aMediaItem, aCallback) {
    
    var albumName = aMediaItem.getProperty(SBProperties.albumName);
    var artistName = aMediaItem.getProperty(SBProperties.artistName);
    var albumArtistName = aMediaItem.getProperty(SBProperties.albumArtistName);
    if (albumArtistName) {
      artistName = albumArtistName;
    }
    
    var arguments = Cc["@mozilla.org/hash-property-bag;1"]
                    .createInstance(Ci.nsIWritablePropertyBag2);
    arguments.setPropertyAsAString("album", albumName);
    arguments.setPropertyAsAString("artist", artistName);
    
    var self = this;
    var apiResponse = function response(success, xml) {
      // Indicate that fetcher is no longer fetching.
      self._isFetching = false;

      // Abort if we are shutting down
      if (self._shutdown) {
        return;
      }
      
      // Failed to get a good response back from the server :(
      if (!success) {
        aCallback(null);
        return;
      }
      
      var foundCover = null;
      var imageSizes = ['large', 'medium', 'small'];
      // Use XPath to parse out the image, we want to find the first image
      // in the order of the imageSizes array above.
      var nsResolver = xml.createNSResolver(xml.ownerDocument == null ?
                                            xml.documentElement :
                                            xml.ownerDocument.documentElement);
      for (var iSize = 0; iSize < imageSizes.length; iSize++) {
        var result = xml.evaluate("//image[@size='" + imageSizes[iSize] + "']",
                                  xml,
                                  nsResolver,
                                  7,  //XPathResult.ORDERED_NODE_SNAPSHOT_TYPE,
                                  null);
        if (result.snapshotLength > 0) {
          foundCover = result.snapshotItem(0).textContent;
          break;
        }
      }
    
      aCallback(foundCover);
    };

    // Indicate that fetcher is fetching.
    this._isFetching = true;

    this._lastFMWebApi.apiCall("album.getInfo",  // method
                               arguments,        // Property bag of strings
                               apiResponse);     // Callback
  },

  /*********************************
   * sbIAlbumArtFetcher
   ********************************/
  // These are a bunch of getters for attributes in the sbICoverFetcher.idl
  get shortName() {
    return "lastfm"; // Change this to something that represents your fetcher
  },
  
  // These next few use the .properties file to get the information
  get name() {
    return SBString(PREF_BRANCH + "name", null, this._strings);
  },
  
  get description() {
    return SBString(PREF_BRANCH + "description", null, this._strings);
  },
  
  get isLocal() {
    return false;
  },
  
  // These are preference settings
  get priority() {
    return parseInt(Application.prefs.getValue(PREF_BRANCH + "priority", 10), 10);
  },
  set priority(aNewVal) {
    return Application.prefs.setValue(PREF_BRANCH + "priority", aNewVal);
  },
  
  get isEnabled() {
    return Application.prefs.getValue(PREF_BRANCH + "enabled", false);
  },
  set isEnabled(aNewVal) {
    return Application.prefs.setValue(PREF_BRANCH + "enabled", aNewVal);
  },
  
  get albumArtSourceList() {
    return this._albumArtSourceList;
  },
  set albumArtSourceList(aNewVal) {
    this._albumArtSourceList = aNewVal;
  },

  get isFetching() {
    return this._isFetching;
  },
  
  fetchAlbumArtForAlbum: function (aMediaItems, aListener) {
    if (aMediaItems.length <= 0) {
      // No Items so abort
      Cu.reportError("No media items passed to fetchAlbumArtForAlbum.");
      if (aListener) {
        aListener.onAlbumResult(null, aMediaItems);
        aListener.onSearchComplete(aMediaItems, null);
      }
      return;
    }
    
    // Extract the first item and use that to get album information
    var firstMediaItem = null;
    try {
      firstMediaItem = aMediaItems.queryElementAt(0, Ci.sbIMediaItem);
    } catch (err) {
      Cu.reportError(err);
      aListener.onAlbumResult(null, aMediaItems);
      aListener.onAlbumComplete(aMediaItems);
      return;
    }

    var returnResult = function (aImageLocation) {
      if (aImageLocation) {
        // Convert to an nsIURI
        var ioService = Cc["@mozilla.org/network/io-service;1"]
                          .getService(Ci.nsIIOService);
        var uri = null;
        try {
          uri = ioService.newURI(aImageLocation, null, null);
        } catch (err) {
          Cu.reportError("lastFM: Unable to convert to URI: [" + aImageLocation +
                         "] " + err);
          uri = null;
        }
        aImageLocation = uri;
      }
      aListener.onAlbumResult(aImageLocation, aMediaItems);
      aListener.onSearchComplete(aMediaItems);
    };
    var downloadCover = function (aFoundCover) {
      if (aFoundCover) {
        sbCoverHelper.downloadFile(aFoundCover, returnResult);
      } else {
        returnResult(null);
      }
    };
    this._findImageForItem(firstMediaItem, downloadCover);
  },
  
  fetchAlbumArtForTrack: function (aMediaItem, aListener) {
    var returnResult = function (aImageLocation) {
      if (aImageLocation) {
        // Convert to an nsIURI
        var ioService = Cc["@mozilla.org/network/io-service;1"]
                          .getService(Ci.nsIIOService);
        var uri = null;
        try {
          uri = ioService.newURI(aImageLocation, null, null);
        } catch (err) {
          Cu.reportError("lastFM: Unable to convert to URI: [" + aImageLocation +
                         "] " + err);
          uri = null;
        }
        aImageLocation = uri;
      }
      aListener.onTrackResult(aImageLocation, aMediaItem);
      // We need to wrap the item in an nsIArray
      var items = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                    .createInstance(Ci.nsIMutableArray);
      items.appendElement(aMediaItem, false);
      aListener.onSearchComplete(items);
    };
    var downloadCover = function (aFoundCover) {
      if (aFoundCover) {
        sbCoverHelper.downloadFile(aFoundCover, returnResult);
      } else {
        returnResult(null);
      }
    };
    this._findImageForItem(aMediaItem, downloadCover);
  },

  shutdown: function () {
    // Don't shutdown if still fetching.
    if (this._isFetching)
      return;

    this._shutdown = true;
  },

  /*********************************
   * nsISupports
   ********************************/
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIAlbumArtFetcher])
}

// This is for XPCOM to register this Fetcher as a module
function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbLastFMAlbumArtFetcher]);
}
