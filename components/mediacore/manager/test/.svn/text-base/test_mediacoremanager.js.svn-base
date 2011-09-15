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
 * \brief Test file
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

  // TODO: XXX Remove when manager fully meets unit test requirements
  return;

  log("Test factory logic\n");
  var factories = mediacoreManager.factories;
  log("Number of factories found: " + factories.length);

  for(let i = 0; i < factories.length; ++i) {
    let factory = factories.queryElementAt(i, Ci.sbIMediacoreFactory);

    log("Factory name: " + factory.name);
    log("Factory contract id: " + factory.contractID);
  }

  var uri = newURI("file:///Volumes/Aus-DATA/Media/gnb_mix3.mp3");
  var mediacoreVotingChain = mediacoreManager.voteWithURI(uri);

  var mediacore = mediacoreVotingChain.mediacoreChain.queryElementAt(0, Ci.sbIMediacore);

  // The following may throw if the interface isn't present for instanceof
  try {
    if(mediacore instanceof Ci.sbIQuickTimeMediacore) {
      log("QuickTime version: " + mediacore.quickTimeVersion);
    }

    if(mediacore instanceof Ci.sbIWindowsMediacore) {
      log("Windows media player version: " + mediacore.windowsMediaVersion);
    }
  }
  catch(e) { };

  log("Attempting to play: " + uri.spec);
  mediacore.uri = uri;
  mediacore.play();

  log("Attempting to seek to: " + 120000 / 1000 + "s");
  mediacore.position = 120000;

  var vol = 0.5;
  log("Attempting to set volume to: " + vol);
  mediacore.volume = vol;

  sleep(25000);

  log("Attempting to pause.");
  mediacore.pause();

  log("Attempting to stop.");
  mediacore.stop();

  var index = 0;
  if(factories.length > 1) {
    index = 1;
  }

  log("Going to attempt to play another file, with another core if possible.");
  mediacore = mediacoreVotingChain.mediacoreChain.queryElementAt(index, Ci.sbIMediacore);

  try {
    if(mediacore instanceof Ci.sbIQuickTimeMediacore) {
      log("QuickTime version: " + mediacore.quickTimeVersion);
    }

    if(mediacore instanceof Ci.sbIWindowsMediacore) {
      log("Windows media player version: " + mediacore.windowsMediaVersion);
    }
  }
  catch(e) { };


  var uri2 = newURI("file:///Users/aus/Music/fluxblog.org/fujiyamiyagi_knickerbocker.mp3");

  log("Attempting to play: " + uri2.spec);
  mediacore.uri = uri2;
  mediacore.play();

  log("Attempting to seek to: " + 1200 / 1000 + "s");
  mediacore.position = 1200;

  var vol = 0.8;
  log("Attempting to set volume to: " + vol);
  mediacore.volume = vol;

  sleep(25000);

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
}
