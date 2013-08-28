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

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function sbSongbirdDispatch() {}
sbSongbirdDispatch.prototype = {
  classDescription: "Nightingale ngale:// protocol command dispatch",
  classID:          Components.ID("2c15f30e-815f-4414-8d18-277d943a2f68"),
  contractID:       "@mozilla.org/network/protocol;1?name=ngale",
 
  newChannel: function(aUri) {
    if (!(aUri.scheme == "ngale")) {
      throw Components.Exception("URI " + aUri.spec + " not supported",
                                 Cr.NS_ERROR_UNEXPECTED);
    }
    aUri.QueryInterface(Ci.nsIURL);

    // the URI is automatically changed by standard-url to look like this:
    // ngale://open/?url=http://foo.com
    // thus "host" will contain the command (if it exists)
    if (aUri.host) {
      switch(aUri.host) {
        case "open": 
          var params = this._parseQueryString(aUri.query);
          var ios = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
          
          // Errors can occur if the URL fails to parse, so in that case
          // we quietly report to the error console.
          try {
            var channel = ios.newChannel(params["url"], null, null);
            // setting the spec here makes the address bar
            // look nice and correct.
            aUri.spec = params["url"];
            if (aUri.scheme == "chrome") {
              Components.utils.reportError(
               "sbSongbirdProtocol::newChannel: " +
               "chrome URLs are not permitted as an argument to open.");
              return new BogusChannel(aUri, "application/dummy-mime-type");
            }
            return channel;
          }
          catch (e) {
            Components.utils.reportError(
               "sbSongbirdProtocol::newChannel: " +
               "failed to make a new channel/url for: " + aUri.query +
               "\nThe syntax is: ngale:open?url=<encodedURL>");
            return new BogusChannel(aUri, "application/dummy-mime-type");
          }
          
          break; // pedantry
        case "nocommand":
          var origURISpec = aUri.spec.replace(/nocommand/, "");
          Components.utils.reportError(
                  "sbSongbirdProtocol::newChannel: no command: " + origURISpec);
          return new BogusChannel(aUri, "application/dummy-mime-type");
          break;
        default:
          Components.utils.reportError(
                  "sbSongbirdProtocol::newChannel: bad command: " + aUri.host);
          return new BogusChannel(aUri, "application/dummy-mime-type");
          break;
      }
    } else {
      Components.utils.reportError(
              "sbSongbirdProtocol::newChannel: no command: " + aUri.spec);
      return new BogusChannel(aUri, "application/dummy-mime-type");
    }
  },

  newURI: function (spec, charSet, baseURI) {
    var url = Cc["@mozilla.org/network/standard-url;1"]
                .createInstance(Ci.nsIURL);
    url.spec = spec;
    if (!url.host) {
      // Returning a URL from newURI with no host causes XULRunner to crash.
      // mozbug 478478
      Components.utils.reportError("sbSongbirdProtocol::newURI: no command provided.\n"+
                                   "Syntax is in the form: ngale:<command>?p1=a&p2=b");

      // Set up the host so that a valid URI can be returned that indicates that
      // no command was provided.
      url.host = "nocommand";
    }
    return url;
  },
  
  getURIFlags: function(uri) {
    return Ci.nsIAboutModule.ALLOW_SCRIPT
  },

  // modified from filtersPage.js
  _parseQueryString: function(query) {
    var queryString = query.split("&");
    var queryObject = {};
    var key, value;
    for each (var pair in queryString) {
      [key, value] = pair.split("=");
      queryObject[key] = unescape(value);
    }
    return queryObject;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProtocolHandler])
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbSongbirdDispatch]);
}

/* We need to be able to throw NS_ERROR_NO_CONTENT to placate the channel
   consumer so that it knows it doesn't have to do any work. 
   This BogusChannel is based on the Chatzilla BogusChannel in 
   chatzilla-service.js */
function BogusChannel(URI, contentType)
{
    this.URI = URI;
    this.originalURI = URI;
    this.contentType = contentType;
}

BogusChannel.prototype.QueryInterface =
function bc_QueryInterface(iid)
{
    if (!iid.equals(nsIChannel) && !iid.equals(nsIRequest) &&
        !iid.equals(nsISupports))
        throw Components.results.NS_ERROR_NO_INTERFACE;

    return this;
}

BogusChannel.prototype.loadAttributes = null;
BogusChannel.prototype.contentLength = 0;
BogusChannel.prototype.owner = null;
BogusChannel.prototype.loadGroup = null;
BogusChannel.prototype.notificationCallbacks = null;
BogusChannel.prototype.securityInfo = null;

BogusChannel.prototype.open =
BogusChannel.prototype.asyncOpen =
function bc_open(observer, ctxt)
{
    throw Components.results.NS_ERROR_NO_CONTENT;
}

BogusChannel.prototype.asyncRead =
function bc_asyncRead(listener, ctxt)
{
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
}

/* nsIRequest */
BogusChannel.prototype.isPending =
function bc_isPending()
{
    return true;
}

BogusChannel.prototype.status = Components.results.NS_OK;

BogusChannel.prototype.cancel =
function bc_cancel(status)
{
    this.status = status;
}

BogusChannel.prototype.suspend =
BogusChannel.prototype.resume =
function bc_suspres()
{
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
}

