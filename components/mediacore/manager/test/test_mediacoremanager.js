/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
 
function newURI(spec) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  
  return ioService.newURI(spec, null, null);
}

function runTest () {
  var mediacoreManager = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                           .getService(Ci.sbIMediacoreManager);
  
  var factories = mediacoreManager.factories;
  log("Number of factories found: " + factories.length);
  
  for(let i = 0; i < factories.length; ++i) {
    let factory = factories.queryElementAt(i, Ci.sbIMediacoreFactory);
    
    log("Factory name: " + factory.name);
    log("Factory contract id: " + factory.contractID);
  }
  
  var uri = newURI("file:///e:/Media/gnb_mix3.mp3");
  var mediacoreVotingChain = mediacoreManager.voteWithURI(uri);
  
  var mediacore = mediacoreVotingChain.mediacoreChain.queryElementAt(0, Ci.sbIMediacore);

  if(mediacore instanceof Ci.sbIWindowsMediacore) {
    log("Windows media player version: " + mediacore.windowsMediaVersion);
  }

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
  
  log("Using next core in mediacore chain to play a different file.");
  mediacore = mediacoreVotingChain.mediacoreChain.queryElementAt(1, Ci.sbIMediacore);
  
  if(mediacore instanceof Ci.sbIQuickTimeMediacore) {
    log("QuickTime version: " + mediacore.quickTimeVersion);
  }
  
  var uri2 = newURI("file:///c:/Users/Aus/Desktop/My%20eMusic/Junior%20Boys/So%20This%20Is%20Goodbye/Junior%20Boys_02_The%20Equalizer.mp3");
  
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
