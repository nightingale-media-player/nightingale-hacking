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

/**
 * \brief Tests the event target functionality of sbMediacoreManager class. 
 * This just does the simple tests as the event target implementation has 
 * more extensive tests
 */
 
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
 
function runTest () {

  var mediacoreManager = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                           .getService(Ci.sbIMediacoreManager);

  log("Testing basic event target functionality\n");
  testSimpleListener(mediacoreManager);
}

/**
 * Event Target functionality testing
 */
function testListener() {
  this.wrappedJSObject = this;
  this.log = [];
}

testListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediacoreEventListener]),
  onMediacoreEvent: function(event) {
    this.log.push(event);
  }
}

var eventIDs = [Ci.sbIMediacoreEvent.METADATA_CHANGE,
                Ci.sbIMediacoreEvent.URI_CHANGE,
                Ci.sbIMediacoreEvent.DURATION_CHANGE,
                Ci.sbIMediacoreEvent.VOLUME_CHANGE,
                Ci.sbIMediacoreEvent.MUTE_CHANGE,
                Ci.sbIMediacoreEvent.STREAM_FOUND,
                Ci.sbIMediacoreEvent.BUFFERING,
                Ci.sbIMediacoreEvent.BUFFER_UNDERRUN,
                Ci.sbIMediacoreEvent.STREAM_START,
                Ci.sbIMediacoreEvent.STREAM_PAUSE,
                Ci.sbIMediacoreEvent.STREAM_END];
  

function dummyMediacore() {
  this.wrappedJSObject = this;
}

dummyMediacore.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediacore]),
}

function testSimpleListener(mediaManager) {
  var listener = new testListener();

  mediaManager.addListener(listener);
  for (index = 0; index < eventIDs.length; ++index) {
    var event = mediaManager.createEvent(eventIDs[index], new dummyMediacore(), "", null);
    mediaManager.dispatchEvent(event, false);
  }
  assertEqual(listener.log.length, eventIDs.length, "event received count not equal to events sent"); 
  for (index = 0; index < eventIDs.length; ++index) {
    assertEqual(listener.log[index].type, eventIDs[index], "Event type doesn't match event ID sent");
  }
  mediaManager.removeListener(listener);
  listener.wrappedJSObject = null;
}
 