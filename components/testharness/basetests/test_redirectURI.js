/**
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

/**
 * \brief Unit test for retrieving the final URL for a redirected URL
 *
 * This file is also an example of how to do aynchronous unit tests.
 */
function reqObserver(uri, check) {
  this.uri = uri;
  this.checker = check;
}

reqObserver.prototype = {
  uri: null,
  checker: null,
  onStartRequest: function (req, con) {
    log("***    starting uri is: " + this.uri.spec);
    log("*** destination uri is: " + this.checker.baseChannel.URI.spec);
    req.cancel(Components.results.NS_ERROR_FAILURE);
  },
  onStopRequest:  function (req, con, stat) {
    testFinished();
  }
} // reqObserver.prototype

function runTest() {

  // Services needed to do the checking
  var ioService = Components.classes["@mozilla.org/network/io-service;1"].getService(Components.interfaces.nsIIOService);
  var uriChecker = Components.classes["@mozilla.org/network/urichecker;1"].getService(Components.interfaces.nsIURIChecker);

  // URI to check
  var uriStr = "http://www.podtrac.com/pts/redirect.mp3?http://aolradio.podcast.aol.com/twit/TWiT0076H.mp3";
  var uri = ioService.newURI(uriStr, null, null);
  //var channel = ioService.newChannelFromURI(uri);

  // setup the checker
  uriChecker.init(uri);
  var obs = new reqObserver(uri, uriChecker);

  // fire the async request
  uriChecker.asyncCheck(obs, null);

  // This needs to be called after the async call
  testPending();

} // runTest

