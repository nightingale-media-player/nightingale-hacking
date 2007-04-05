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

function runTest () {

  var SB_NS = "http://songbirdnest.com/data/1.0#";
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  var databaseGUID = "test_localdatabaselibrary";
  var library = createLibrary(databaseGUID);

  // test create
  var uriSpec = "file:///foo";
  var uri = ios.newURI(uriSpec, null, null);

  
  var item1 = library.createMediaItem(uri);
  var now = new Date();
  assertEqual(item1.getProperty(SB_NS + "contentUrl"), uriSpec);

  var created = new Date(parseInt(item1.getProperty(SB_NS + "created")));
  var updated = new Date(parseInt(item1.getProperty(SB_NS + "updated")));

  assertEqual(created.getTime() == updated.getTime(), true);
  // Compare the dates with a little slack -- they should be very close
  assertEqual(now - created < 5000, true);
  assertEqual(now - updated < 5000, true);

  // Make sure we are getting different guids
  var item2 = library.createMediaItem(uri);
  assertNotEqual(item1.guid, item2.guid);

  // Test that they items were added to the library view the view list
  var libraryList = library.QueryInterface(Ci.sbIMediaList);
  assertEqual(libraryList.getItemByGuid(item1.guid).guid, item1.guid);
  assertEqual(libraryList.getItemByGuid(item2.guid).guid, item2.guid);
  
  var listListener = new TestMediaListListener();
  libraryList.addListener(listListener);

  var uri2 = ios.newURI("file:///bar", null, null);
  var item3 = library.createMediaItem(uri2);

  assertEqual(listListener.addedItem, item3);
  
  libraryList.remove(item1);
  assertEqual(listListener.removedItem, item1);
  
  // Test if removing items from the library also remove items from the
  // playlist.
  var enumerationListener = new TestMediaListEnumerationListener();
  
  var list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  assertEqual(list.length, 20);
  
  list.enumerateAllItems(enumerationListener,
                         Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
  
  var listCount = enumerationListener.count;
  assertTrue(listCount > 0);
  
  listListener.reset();
  
  var removedItemCount = 0;
  listListener.onItemRemoved = function onItemRemoved(list, item) {
    removedItemCount++;
  }
  
  libraryList.removeSome(enumerationListener.QueryInterface(Ci.nsISimpleEnumerator));
  
  assertEqual(removedItemCount, listCount);
  
  assertEqual(list.length, 0);
  
  // Now test if clearing the library also clears the playlist.
  library = createLibrary(databaseGUID);
  list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  
  assertEqual(list.length, 20);
  
  libraryList = library.QueryInterface(Ci.sbIMediaList);
  libraryList.clear();
  
  assertEqual(list.length, 0);

}
