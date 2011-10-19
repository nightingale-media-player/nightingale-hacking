/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

// these constants make everything better
const Cc = Components.classes;
const CC = Components.Constructor;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

// Last.fm API key, secret and URL
const API_KEY = 'ad68d3b69dee88a912b193a35d235a5b';
const API_SECRET = '5cb0c1f1cceb3bff561a62b718702175'; // obviously not secret
const API_URL = 'http://ws.audioscrobbler.com/2.0/';

// Import some helper scripts
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/sbProperties.jsm");

// XPCOM Constants
const CONTRACTID = "@getnightingale.com/Nightingale/webservices/last-fm;1";
const CLASSNAME = "Nightingale Last.FM WebService Interface";
const CID = Components.ID("{6582d596-95dd-4449-be8b-7793a15bdfa2}");
const IID = Ci.sbILastFmWebServices;

// helper for enumerating enumerators.
function enumerate(enumerator, func) {
  while(enumerator.hasMoreElements()) {
    try {
      func(enumerator.getNext());
    } catch(e) {
      Cu.reportError(e);
    }
  }
}

// calculate a hex md5 digest thing
function md5(str) {
  var converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
      .createInstance(Ci.nsIScriptableUnicodeConverter);

  converter.charset = "UTF-8";
  // result is an out parameter,
  // result.value will contain the array length
  var result = {};
  // data is an array of bytes
  var data = converter.convertToByteArray(str, result);
  var ch = Cc["@mozilla.org/security/hash;1"].createInstance(Ci.nsICryptoHash);
  ch.init(ch.MD5);
  ch.update(data, data.length);
  var hash = ch.finish(false);

  // return the two-digit hexadecimal code for a byte
  function toHexString(charCode) {
    return ("0" + charCode.toString(16)).slice(-2);
  }

  // convert the binary hash data to a hex string.
  var s = [toHexString(hash.charCodeAt(i)) for (i in hash)].join("");
  return s;
}

// urlencode an object's keys & values
function urlencode(o) {
  s = '';
  for (var k in o) {
    var v = o[k];
    if (s.length) { s += '&'; }
    s += encodeURIComponent(k) + '=' + encodeURIComponent(v);
  }
  return s;
}

// make an HTTP POST request
function POST(url, params, onload, onerror) {
  var xhr = null;
  try {
    // create the XMLHttpRequest object
    xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance();
    // don't tie it to a XUL window
    xhr.mozBackgroundRequest = true;
    // open the connection to the url
    xhr.open('POST', url, true);
    // set up event handlers
    xhr.onload = function(event) { onload(xhr); }
    xhr.onerror = function(event) { onerror(xhr); }
    // we're always sending url encoded parameters
    xhr.setRequestHeader('Content-Type', 'application/x-www-form-urlencoded');
    // send url encoded parameters
    xhr.send(urlencode(params));
    // pass the XHR back to the caller
  } catch(e) {
    Cu.reportError(e);
    onerror(xhr);
  }
  return xhr;
}

/**
 * sbILastFmWebServices
 */

function sbLastFmWebServices() {
  // our interface is really lightweight - make the service available as a JS
  // object so we can avoid the IDL / XPConnect complexity.
  this.wrappedJSObject = this;

  // report errors using the console service
  this._console = Cc["@mozilla.org/consoleservice;1"]
                    .getService(Ci.nsIConsoleService);

}
sbLastFmWebServices.prototype = {
  constructor: sbLastFmWebServices,       // Constructor to this object
  classDescription:  CLASSNAME,
  classID:           CID,
  contractID:        CONTRACTID,
  _xpcom_categories:
  [{
    category: "app-startup",
    entry: "webservices-lastfm",
    value: "service," + CONTRACTID
  }],

  // Error reporting
  log: function sbLastFmWebServices_log(message) {
    this._console.logStringMessage('[last-fm] '+message);
  },

  /**
   * See sbILastFmWebServices.idl
   */
  apiCall: function sbLastFmWebServices_apiCall(aMethod,
                                                aArguments,
                                                aCallback) {
    // make a new Last.fm Web Service API call
    // see: http://www.last.fm/api/rest
    // note: this isn't really REST.
  
    function callback(success, response) {
      if (typeof(aCallback) == 'function') {
        aCallback(success, response);
      } else {
        aCallback.responseReceived(success, response);
      }
    }
  
    // create an object to hold the HTTP params
    var post_params = new Object();
  
    // load the params from the nsIPropertyBag
    if (aArguments instanceof Ci.nsIPropertyBag) {
      enumerate(aArguments.enumerator, function(item) {
        item.QueryInterface(Ci.nsIProperty);
        post_params[item.name] = item.value;
      })
    } else {
      // or from the object
      for (var k in aArguments) { post_params[k] = aArguments[k]; }
    }
  
    // set the method and API key
    post_params.method = aMethod;
    post_params.api_key = API_KEY;
  
    // calculate the signature...
    // put the key/value pairs in an array
    var sorted_params = new Array();
    for (var k in post_params) {
      sorted_params.push(k+post_params[k])
    }
    // sort them
    sorted_params.sort();
    // join them into a string
    sorted_params = sorted_params.join('');
    // hash them with the "secret" to get the API signature
    post_params.api_sig = md5(sorted_params+API_SECRET);
    
    // post the request
    var self = this;
    POST(API_URL, post_params, function (xhr) {
      if (!xhr.responseXML) {
        // we expect all API responses to have XML
        self.log('Last.fm WebServices Error: No valid XML response.');
        callback(false, null);
        return;
      }
      
      // Check for an error status
      var nsResolver = xhr.responseXML.createNSResolver(
                                xhr.responseXML.ownerDocument == null ?
                                xhr.responseXML.documentElement :
                                xhr.responseXML.ownerDocument.documentElement);
      var result = xhr.responseXML.evaluate("/lfm/@status",
                                            xhr.responseXML,
                                            nsResolver,
                                            2,  //XPathResult.STRING_TYPE,
                                            null);
      if (result.stringValue && result.stringValue == 'failed') {
        result = xhr.responseXML.evaluate("/lfm/error",
                                          xhr.responseXML,
                                          nsResolver,
                                          2,  //XPathResult.STRING_TYPE,
                                          null);
        var error = "Unknown Error";
        if (result.stringValue) {
          error = result.stringValue;
        }
        self.log('Last.fm WebServices Error: ' + error);
        callback(false, xhr.responseXML);
        return;
      }

      // all should be good!
      callback(true, xhr.responseXML);
    }, function (xhr) {
      self.log('Last.fm WebServices Error: Bad response from server.');
      callback(false, null);
    });
  },

  /**
   * See nsISupports.idl
   */
  QueryInterface: XPCOMUtils.generateQI([IID])
}

/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 */

function NSGetModule(comMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbLastFmWebServices]);
} // NSGetModule
