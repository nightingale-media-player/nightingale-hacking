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


function runTest () {
  var playlist = [
    "file:///e:/Download/Miss%20Kittin%20-%20Batbox%20(flac)/13%20-%20Mightmaker.flac",
    "file:///e:/Download/Gnarls%20Barkley%20-%20The%20Odd%20Couple/%5B05%5D%20Gnarls%20Barkley%20-%20Would%20Be%20Killer.flac",
    "file:///e:/Download/Miss%20Kittin%20-%20Batbox%20(flac)/09%20-%20Metalhead.flac",
    "file:///e:/Download/Broken%20Toy%20-%20The%20Low%20Down%20Dirty%20Sound%20Of%20(%20Alchemy%20Records%202007%20)/01.%20Broken%20Toy%20-%20Whatever.flac"
    ];

  var mediacoreManager = Cc["@songbirdnest.com/Songbird/Mediacore/Manager;1"]
                           .getService(Ci.sbIMediacoreManager);

  var library = createLibrary("test_mediacoresequencer", null, false);

  var mediaList = library.createMediaList("simple");
  mediaList.name = "Sequencer Test";

  for(let i = 0; i < playlist.length; ++i) {
    let item = library.createMediaItem(newURI(playlist[i]));
    mediaList.add(item);
  }

  var sequencer = mediacoreManager.sequencer;
  sequencer.view = mediaList.createView();

  sequencer.play();
  //mediacoreManager.volumeControl.volume = 1;

  sleep(2000000);

}
