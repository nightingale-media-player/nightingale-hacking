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
