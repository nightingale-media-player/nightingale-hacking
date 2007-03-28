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

function countItems(enumerator) {
  var count = 0;
  while (enumerator.hasMoreElements()) {
    var item = enumerator.getNext().QueryInterface(Ci.sbIMediaItem);
    assertNotEqual(item, null);
    count++;
  }
  return count;
}

function runTest () {

  var databaseGUID = "test_localdatabaselibrary";
  var library = createLibrary(databaseGUID);

  var view = library.getMediaItem("songbird:view");
  assertList(view, "data_sort_created_asc.txt");

  var item = view.getItemByIndex(0);
  assertEqual(item.guid, "3E2549C0-AD99-11DB-9321-C22AB7121F49");

  var contains = view.contains(view);
  assertEqual(contains, true);
  // TODO: test when this method fails, but how can i generate a media item
  // that is not in the view media list?

  try {
    view.add(item);
    fail("No exception thrown");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  // Note that these next two tests never fail since items that already exist
  // in the database are silently skipped
  var e = new SimpleArrayEnumerator([item]);
  view.addSome(e);

  var list = view.getItemByGuid("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  view.addAll(list);

  var titleProperty = "http://songbirdnest.com/data/1.0#trackName";
  var albumProperty = "http://songbirdnest.com/data/1.0#albumName";
  var genreProperty = "http://songbirdnest.com/data/1.0#genre";
  
  var enumerationListener = new TestMediaListEnumerationListener();
  
  view.enumerateItemsByProperty(titleProperty, "Train of Thought",
                                enumerationListener,
                                Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 1);
  enumerationListener.reset();
  
  view.enumerateItemsByProperty(albumProperty, "Back in Black",
                                enumerationListener,
                                Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 10);
  enumerationListener.reset();
  
  view.enumerateItemsByProperty(genreProperty, "KJaskjjbfjJDBs",
                                enumerationListener,
                                Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 0);
  enumerationListener.reset();

  var propertyArray =
    Cc["@songbirdnest.com/Songbird/Properties/PropertyArray;1"].
    createInstance(Ci.sbIPropertyArray);
  propertyArray.appendProperty(albumProperty, "Back in Black");
  
  view.enumerateItemsByProperties(propertyArray, enumerationListener,
                                  Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 10);
  enumerationListener.reset();
  
  propertyArray.appendProperty(titleProperty, "Rock and Roll Ain't Noise Pollution");
  propertyArray.appendProperty(titleProperty, "Shake a Leg");

  view.enumerateItemsByProperties(propertyArray, enumerationListener,
                                  Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 2);
  enumerationListener.reset();

  propertyArray.removeElementAt(1);
  
  view.enumerateItemsByProperties(propertyArray, enumerationListener,
                                  Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 1);
  enumerationListener.reset();

  //Test getIemByIndex, indexOf, lastIndexOf.
  var mediaItem = view.getItemByIndex(8);
  assertNotEqual(mediaItem, null);
  
  var mediaItemIndex = view.indexOf(mediaItem, 0);
  assertEqual(mediaItemIndex, 8);

  var indexOfException;
  try {
    mediaItemIndex = view.indexOf(mediaItem, 10);
  } catch (e) {
    indexOfException = e;
  }
  assertEqual(indexOfException.result, Cr.NS_ERROR_NOT_AVAILABLE);
  
  var indexOfException2;
  try {
    mediaItemIndex = view.indexOf(mediaItem, 45000);
  } catch (e) {
    indexOfException2 = e;
  }
  assertEqual(indexOfException2.result, Cr.NS_ERROR_INVALID_ARG);

  mediaItemIndex = view.lastIndexOf(mediaItem, 0);
  assertEqual(mediaItemIndex, 8);
  
  var lastIndexOfException;
  try {
    mediaItemIndex = view.lastIndexOf(mediaItem, 10);
  } catch (e) {
    lastIndexOfException = e;
  }
  assertEqual(lastIndexOfException.result, Cr.NS_ERROR_NOT_AVAILABLE);
  
  var lastIndexOfException2;
  try {
    mediaItemIndex = view.lastIndexOf(mediaItem, 45000);
  } catch (e) {
    lastIndexOfException2 = e;
  }
  assertEqual(lastIndexOfException2.result, Cr.NS_ERROR_INVALID_ARG);
  
  // Test listeners

  /* // Remove tests disabled for the moment
  // Test remove
  library = createLibrary(databaseGUID);

  item = library.getMediaItem("3E2549C0-AD99-11DB-9321-C22AB7121F49");
  assertEqual(view.contains(item), true);
  var oldlength = view.length;
  view.remove(item);
  assertEqual(view.contains(item), false);
  assertEqual(view.length, oldlength - 1);

  item = view.getItemByIndex(0);
  oldlength = view.length;
  assertEqual(view.contains(item), true);
  view.removeByIndex(0);
  assertEqual(view.contains(item), false);
  assertEqual(view.length, oldlength - 1);

  // test bad index
  try {
    view.removeByIndex(view.length);
    fail("NS_ERROR_ILLEGAL_VALUE not thrown");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_ILLEGAL_VALUE);
  }

  var toRemove = [
    view.getItemByIndex(0),
    view.getItemByIndex(1),
    view.getItemByIndex(2),
    view.getItemByIndex(3),
    view.getItemByIndex(4)
  ];
  oldlength = view.length;
  toRemove.forEach(function(item) { assertEqual(view.contains(item), true); });
  view.removeSome(new SimpleArrayEnumerator(toRemove));
  toRemove.forEach(function(item) { assertEqual(view.contains(item), false); });
  assertEqual(view.length, oldlength - toRemove.length);

  // Remove an item that is part of a list
  list = view.getItemByGuid("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  item = list.getItemByIndex(0);
  oldlength = list.length;
  view.remove(item);
  // XXX: SK - Need to do this until invalidation listener is hooked up
  list = view.getItemByGuid("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  assertEqual(list.contains(item), false);
  assertEqual(list.length, oldlength - 1);

  // Remove a list
  oldlength = view.length;
  view.remove(list);
  assertEqual(view.contains(list), false);
  assertEqual(view.length, oldlength - 1);

  // Test clear
  view.clear();
  assertEqual(view.length, 0);
  */
}

