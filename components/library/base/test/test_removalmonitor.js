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

/**
 * \brief Test LibraryUtils.RemovalMonitor
 */

function runTest () {

  Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
  
  // Make a test library
  var library = createLibrary("test_library_removalmonitor1");
  var libraryManager = Cc["@getnightingale.com/Nightingale/library/Manager;1"]
                                .getService(Ci.sbILibraryManager);
  libraryManager.registerLibrary(library, false);
  
 
  // Monitor removal of media lists 
  var callback = {
    expectCallback: false,
    seenCallback: false,
    onMediaListRemoved: function() {
      this.seenCallback = true;
    },
    verify: function() {
      assertEqual(this.expectCallback, this.seenCallback);
      this.expectCallback = true;
      this.seenCallback = false;
    }
  }
  var monitor = new LibraryUtils.RemovalMonitor(callback);
  
  // Make sure list removal works
  var list = library.createMediaList("simple");
  callback.expectCallback = true;
  monitor.setMediaList(list);
  library.remove(list);
  callback.verify();
  
  // Make sure unsetting the media list works
  list = library.createMediaList("simple");
  monitor.setMediaList(list);
  monitor.setMediaList(null);
  callback.expectCallback = false;
  library.remove(list);
  callback.verify();
  
  // Make sure removal by library clear works
  list = library.createMediaList("simple");
  callback.expectCallback = true;
  monitor.setMediaList(list);
  library.clear();
  callback.verify();
  
  // Make sure removal in a batch operation works
  list = library.createMediaList("simple");
  var list2 = library.createMediaList("simple");
  callback.expectCallback = true;  
  monitor.setMediaList(list2);
  library.runInBatchMode(function() {
    library.remove(list);
    library.remove(list2);
  }); 
  callback.verify();
  
  // Make sure clearing by library removal works
  callback.expectCallback = true;
  list = library.createMediaList("simple");
  monitor.setMediaList(list);
  libraryManager.unregisterLibrary(library);
  callback.verify();  
  
  // Make sure that unregistering the library works
  // when we are monitoring the library itself
  library = createLibrary("test_library_removalmonitor2");
  libraryManager.registerLibrary(library, false);
  monitor.setMediaList(library);
  callback.expectCallback = true;
  libraryManager.unregisterLibrary(library);
  callback.verify();  
}

