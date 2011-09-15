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
 * \brief Test the media core listener
 */

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

function testListener() {
  this.wrappedJSObject = this;
  this.log = [];
  this.onEnd = null;
}

testListener.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediacoreEventListener]),
  onMediacoreEvent: function(event) {
    this.log.push(event);
    if (this.onEnd && event.type == Ci.sbIMediacoreEvent.STREAM_END) {
      this.onEnd();
      testFinished();
    }
  }
}

function dummyCore() {
  this.wrappedJSObject = this;
}

dummyCore.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.sbIMediacore]),
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
  
function createEventTarget() {
  return Cc["@songbirdnest.com/mediacore/sbTestDummyMediacoreManager;1"]
            .createInstance(Ci.sbIMediacoreEventTarget);
}

function createEvent(type, error, data) {
  var creator = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                  .getService(Ci.sbIMediacoreManager);
  return creator.createEvent(type, new dummyCore(), error, data);
}

/**
 * Tests single thread synchronous and asynchronous events
 */
function runTest () {
  log("Synchronous\n");
  testSimpleListener(false);
  
  log("Asynchronous\n");
  testSimpleListener(true);
  testPending();
  
  eventTarget.removeListener(listener);  
  
  listener = null;
  eventTarget = null;
}

var eventTarget = null;
var listener = null;

function testSimpleListener(async) {
  
  if(!eventTarget) {
    eventTarget = createEventTarget();
  }
  
  if(!listener) {
    listener = new testListener();
  }

  eventTarget.addListener(listener);
  
  for (index = 0; index < eventIDs.length; ++index) {
    var event = createEvent(eventIDs[index], null, null);
    eventTarget.dispatchEvent(event, async);
  }
  var testValues = function() {
    assertEqual(listener.log.length, eventIDs.length, "event received count not equal to events sent"); 
    for (index = 0; index < eventIDs.length; ++index) {
      assertEqual(listener.log[index].type, eventIDs[index], "Event type doesn't match event ID sent");
    }
  }
  if (async) {
    listener.onEnd = testValues;
  }
  else {
    testValues();
    listener.log = [];
    eventTarget.removeListener(listener);
  }
}
 
