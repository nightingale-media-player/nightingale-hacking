/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://songbirdnest.com
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
// END SONGBIRD GPL
//
*/


/* This XPCOM service knows how to ask Amazon.com where to find art for 
 * music albums. 
 */

if (typeof(Cc) == "undefined")
  var Cc = Components.classes;
if (typeof(Ci) == "undefined")
  var Ci = Components.interfaces;
if (typeof(Cu) == "undefined")
  var Cu = Components.utils;
if (typeof(Cr) == "undefined")
  var Cr = Components.results;

const STRING_BUNDLE = "chrome://albumartmanager/locale/albumartmanager.properties";

Cu.import('resource://app/jsmodules/sbProperties.jsm');
Cu.import("resource://app/jsmodules/StringUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import('resource://gre/modules/XPCOMUtils.jsm');

function sbAmazonCoverFetcher() {
  this.DEBUG = false;
  this._ioService = Cc['@mozilla.org/network/io-service;1']
    .getService(Ci.nsIIOService);
  this._prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);
  var stringBundleService = Cc["@mozilla.org/intl/stringbundle;1"]
                             .getService(Ci.nsIStringBundleService);
  this._bundle = stringBundleService.createBundle(STRING_BUNDLE);
  this._shutdown = false;

  this._prefService = Cc['@mozilla.org/preferences-service;1']
                        .getService(Ci.nsIPrefBranch);

  try {
    this._consoleService = Cc["@mozilla.org/consoleservice;1"]
                            .getService(Ci.nsIConsoleService);
    this._prefService = Cc['@mozilla.org/preferences-service;1']
                          .getService(Ci.nsIPrefBranch);
    this.DEBUG = prefService.getBoolPref("songbird.albumartmanager.debug");
  } catch (err) {}
}
sbAmazonCoverFetcher.prototype = {
  className: 'sbAmazonCoverFetcher',
  classDescription: 'Songbird Amazon Album Cover Fetcher',
  // Please generate a new classID with uuidgen.
  classID: Components.ID('{958e914f-84d6-4501-aa0c-17c575de0dd1}'),
  contractID: '@songbirdnest.com/Songbird/cover/amazon-fetcher;1'
}

// nsISupports
sbAmazonCoverFetcher.prototype.QueryInterface =
XPCOMUtils.generateQI([Ci.sbICoverFetcher]);

sbAmazonCoverFetcher.prototype._xpcom_categories = [
  { category: "sbCoverFetcher" }
];

/**
 * \brief Dumps out a message if the DEBUG flag is enabled with
 *        the className pre-appended.
 * \param message String to print out.
 */
sbAmazonCoverFetcher.prototype._debug =
function sbAmazonCoverFetcher__debug(message) {
  if (!this.DEBUG) return;
  try {
    dump(this.className + ": " + message + "\n");
    this._consoleService.logStringMessage(this.className + ": " + message);
  } catch (err) {}
}

/**
 * \brief Dumps out an error message with "the className and "[ERROR]"
 *        pre-appended, and will report the error so it will appear in the
 *        error console.
 * \param message String to print out.
 */
sbAmazonCoverFetcher.prototype._logError =
function sbAmazonCoverFetcher__logError(message) {
  try {
    dump(this.className + ": [ERROR] - " + message + "\n");
    Cu.reportError(this.className + ": [ERROR] - " + message);
  } catch (err) {}
}

// sbICoverFetcher
sbAmazonCoverFetcher.prototype.__defineGetter__("shortName",
function sbAmazonCoverFetcher_name() {
  return "amazon";
});

sbAmazonCoverFetcher.prototype.__defineGetter__("name",
function sbAmazonCoverFetcher_name() {
  return SBString("albumartmanager.fetcher.amazon.name", null, this._bundle);
});

sbAmazonCoverFetcher.prototype.__defineGetter__("description",
function sbAmazonCoverFetcher_description() {
  return SBString("albumartmanager.fetcher.amazon.description", null, this._bundle);
});

sbAmazonCoverFetcher.prototype.__defineGetter__("userFetcher",
function sbAmazonCoverFetcher_userFetcher() {
  return true;
});

sbAmazonCoverFetcher.prototype.__defineGetter__("isEnabled",
function sbAmazonCoverFetcher_isEnabled() {
  var retVal = false;
  try {
    retVal = this._prefService.getBoolPref("songbird.albumartmanager.fetcher." +
                                            this.shortName + ".isEnabled");
  } catch (err) { }
  return retVal;
});
sbAmazonCoverFetcher.prototype.__defineSetter__("isEnabled",
function sbAmazonCoverFetcher_isEnabled(aNewVal) {
  this._prefService.setBoolPref("songbird.albumartmanager.fetcher." +
                                  this.shortName + ".isEnabled",
                                aNewVal);
});

sbAmazonCoverFetcher.prototype._findCoverURL = 
function sbAmazonCoverFetcher_findCoverURL(aMediaItem, aCallback) {
  /* build the amazon query */
  var amazonKey = '068VR1FECHFBJ1NBK5R2';
  var amazonTag = 'bengtonlin-20';
  var amazonTld = 'com';

  try {
    var amazonCountry = this._prefService
                            .getCharPref("songbird.albumartmanager.country");
    amazonTld = amazonCountry;
  } catch (err) {}

  var amazonURL = 'http://webservices.amazon.' + amazonTld + 
    '/onca/xml?Service=AWSECommerceService&AWSAccessKeyId=' + amazonKey + 
    '&AssociateTag=' + amazonTag + '&Operation=ItemSearch&SearchIndex=Music';

  var artist = aMediaItem.getProperty(SBProperties.artistName);
  if (artist && artist != '') {
    amazonURL += '&Keywords=' + encodeURI(artist);
  }

  var album = aMediaItem.getProperty(SBProperties.albumName);
  if (album && album != '') {
    amazonURL += '&Title=' + encodeURI(album);
    amazonURL += '&ResponseGroup=Small,Images';
  } else {
    amazonURL += '&ResponseGroup=Small,Images,Tracks';
  }

  var xhr = Cc['@mozilla.org/xmlextras/xmlhttprequest;1']
    .createInstance(Ci.nsIXMLHttpRequest);
  xhr.open('get', amazonURL, true);
  xhr.onreadystatechange = function() {
    if (xhr.readyState == 4) {
      // for bug in mozilla that throw error when calling status
      try {
        xhr.status;
      } catch (e) {
        aCallback(null);
        xhr.cancel();
        return;
      }

      if (xhr.status != 200) {
        // a non-200 response is considered an error
        aCallback(null);
        return;
      }

      if (!Components.isSuccessCode(xhr.channel.status)) {
        // make sure we weren't aborted
        aCallback(null);
        return;
      }

      if (!xhr.responseXML) {
        // the response has to be XML to be useful to us
        aCallback(null);
        return;
      }

      // so at this point we have a 200 response with XML data

      // FIXME: move this to XPath
      var ioService =  Cc['@mozilla.org/network/io-service;1']
                           .getService(Ci.nsIIOService);

      // find all the items in the response
      var elements = xhr.responseXML.getElementsByTagName('Item');
      // what order do we look for the sizes?
      var sizes = ['Large', 'Medium', 'Small'];
      var covers = [];
      // for each element, in relevance order...
      for (var i=0; i<elements.length; i++) {
        var element = elements[i];
        // look for the size of image we want
        for (var j=0; j<sizes.length; j++) {
          var url = null;
          try {
            url = 
              // find the first <size>Image element
              element.getElementsByTagName(sizes[j]+'Image')[0]
              // find the first URL element
              .getElementsByTagName('URL')[0]
              // get the contents as text
              .firstChild.nodeValue;
            // looks like we found it
          } catch (e) {
            // no problem, we'll try another way
          }
          
          if (url) {
            try {
              var uri = ioService.newURI(url, null, null);
              covers.push(uri);
            } catch (err) {
            }
          }
        }
      }

      aCallback(covers);
    }
  }
  
  this._debug("Sending url : " + amazonURL);
  xhr.send(null);
}

// sbICoverFetcher
sbAmazonCoverFetcher.prototype.fetchCoverForMediaItem = 
function sbAmazonCoverFetcher_fetchCoverForMediaItem(aMediaItem, aListener, aWindow) {
  if (this._shutdown) {
    return;
  }

  var contentURI = this._ioService.newURI(
      aMediaItem.getProperty(SBProperties.contentURL), null, null);

  // if the URL isn't valid, fail
  if (!contentURI) {
    aListener.coverFetchFailed(aMediaItem, null);
    return;
  }

  // if the URL isn't local, fail
  var fileURL = contentURI.QueryInterface(Ci.nsIFileURL);
  if (!fileURL) {
    aListener.coverFetchFailed(aMediaItem, null);
    return;
  }

  // the result is scoped to the artist & album
  var scope = Cc['@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1']
    .createInstance(Ci.sbIMutablePropertyArray);
  scope.appendProperty(SBProperties.albumName, 
      aMediaItem.getProperty(SBProperties.albumName));
  scope.appendProperty(SBProperties.artistName, 
      aMediaItem.getProperty(SBProperties.artistName));

  var self = this;
  this._findCoverURL(aMediaItem, function(aCoverList) {
    if (aCoverList == null || aCoverList.length == 0) {
      // epic fail
      aListener.coverFetchFailed(aMediaItem, scope);
    } else {
      if (aCoverList && aCoverList.length > 0) {
        self._debug("Found " + aCoverList.length + " covers");
        self._debug("First cover is " + aCoverList[0].spec);
        aListener.coverFetchSucceeded(aMediaItem, scope, aCoverList[0].spec);
      } else {
        aListener.coverFetchFailed(aMediaItem, scope);
      }
   }
  });
}

sbAmazonCoverFetcher.prototype.fetchCoverListForMediaItem = 
function sbAmazonCoverFetcher_fetchCoverListForMediaItem(aMediaItem, aListener, aWindow) {
  if (this._shutdown) {
    return;
  }

  var contentURI = this._ioService.newURI(
      aMediaItem.getProperty(SBProperties.contentURL), null, null);

  // if the URL isn't valid, fail
  if (!contentURI) {
    aListener.coverFetchFailed(aMediaItem, null);
    return;
  }

  // if the URL isn't local, fail
  var fileURL = contentURI.QueryInterface(Ci.nsIFileURL);
  if (!fileURL) {
    aListener.coverFetchFailed(aMediaItem, null);
    return;
  }

  // the result is scoped to the artist & album
  var scope = Cc['@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1']
    .createInstance(Ci.sbIMutablePropertyArray);
  scope.appendProperty(SBProperties.albumName, 
      aMediaItem.getProperty(SBProperties.albumName));
  scope.appendProperty(SBProperties.artistName, 
      aMediaItem.getProperty(SBProperties.artistName));

  this._findCoverURL(aMediaItem, function(aCoverList) {
    if (!aCoverList) {
      // epic fail
      aListener.coverFetchFailed(aMediaItem, scope);
    } else {
      aListener.coverListFetchSucceeded(aMediaItem,
                                        scope,
                                        new ArrayConverter.enumerator(aCoverList));
    }
  });
}

sbAmazonCoverFetcher.prototype.shutdown =
function sbAmazonCoverFetcher_shutdown() {
  this._shutdown = true;
}


function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbAmazonCoverFetcher]);
}




