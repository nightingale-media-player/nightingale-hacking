/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
 * \brief Test file
 */

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

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

  var databaseGUID = "test_simplemedialist";
  var library = createLibrary(databaseGUID);

  var list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  assertList(list, "data_sort_sml101_ordinal_asc.txt");

  var item = list.getItemByIndex(0);
  assertEqual(item.guid, "3E586C1A-AD99-11DB-9321-C22AB7121F49");

  // Test contains
  item = library.getMediaItem("3E586C1A-AD99-11DB-9321-C22AB7121F49");
  var contains = list.contains(item);
  assertEqual(contains, true);

  item = library.getMediaItem("3E6DD1C2-AD99-11DB-9321-C22AB7121F49");
  contains = list.contains(item);
  assertEqual(contains, false);

  var titleProperty = "http://songbirdnest.com/data/1.0#trackName";
  var albumProperty = "http://songbirdnest.com/data/1.0#albumName";
  var genreProperty = "http://songbirdnest.com/data/1.0#genre";

  // Test getItemsByProperty(s)
  var enumerationListener = new TestMediaListEnumerationListener();
  
  list.enumerateItemsByProperty(titleProperty, "Train of Thought",
                                enumerationListener,
                                Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 1);
  enumerationListener.reset();
  
  list.enumerateItemsByProperty(albumProperty, "Back in Black",
                                enumerationListener,
                                Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 10);
  enumerationListener.reset();
  
  list.enumerateItemsByProperty(genreProperty, "KJaskjjbfjJDBs",
                                enumerationListener,
                                Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 0);
  enumerationListener.reset();

  var propertyArray =
    Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"].
    createInstance(Ci.sbIMutablePropertyArray);
  propertyArray.appendProperty(albumProperty, "Back in Black");
  
  list.enumerateItemsByProperties(propertyArray, enumerationListener,
                                  Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 10);
  enumerationListener.reset();
  
  propertyArray.appendProperty(titleProperty, "Rock and Roll Ain't Noise Pollution");
  propertyArray.appendProperty(titleProperty, "Shake a Leg");

  list.enumerateItemsByProperties(propertyArray, enumerationListener,
                                  Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 2);
  enumerationListener.reset();

  propertyArray.removeElementAt(1);
  
  list.enumerateItemsByProperties(propertyArray, enumerationListener,
                                  Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  assertEqual(enumerationListener.count, 1);
  enumerationListener.reset();
  
  //Test getIemByIndex, indexOf, lastIndexOf.
  var mediaItem = list.getItemByIndex(8);
  assertNotEqual(mediaItem, null);
  
  var mediaItemIndex = list.indexOf(mediaItem, 0);
  assertEqual(mediaItemIndex, 8);
  
  var indexOfException;
  try {
    mediaItemIndex = list.indexOf(mediaItem, 10);
  } catch (e) {
    indexOfException = e;
  }
  assertEqual(indexOfException.result, Cr.NS_ERROR_NOT_AVAILABLE);
  
  var indexOfException2;
  try {
    mediaItemIndex = list.indexOf(mediaItem, 45);
  } catch (e) {
    indexOfException2 = e;
  }
  assertEqual(indexOfException2.result, Cr.NS_ERROR_INVALID_ARG);
  
  var indexOfException3;
  try {
    mediaItemIndex = list.indexOf(mediaItem);
  } catch (e) {
    indexOfException3 = e;
    assertEqual(indexOfException3.result, Cr.NS_ERROR_INVALID_ARG);
  }

  mediaItemIndex = list.lastIndexOf(mediaItem, 0);
  assertEqual(mediaItemIndex, 8);
  
  var lastIndexOfException;
  try {
    mediaItemIndex = list.lastIndexOf(mediaItem, 10);
  } catch (e) {
    lastIndexOfException = e;
  }
  assertEqual(lastIndexOfException.result, Cr.NS_ERROR_NOT_AVAILABLE);
  
  var lastIndexOfException2;
  try {
    mediaItemIndex = list.lastIndexOf(mediaItem, 45);
  } catch (e) {
    lastIndexOfException2 = e;
  }
  assertEqual(lastIndexOfException2.result, Cr.NS_ERROR_INVALID_ARG);

  //Test add, addSome, addAll
  var data = readList("data_sort_sml101_ordinal_asc.txt");

  list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  item = library.getMediaItem("3E6DD1C2-AD99-11DB-9321-C22AB7121F49");
  var oldlength = list.length;

  list.add(item);
  data.push(item.guid);
  assertEqual(list.contains(item), true);
  assertEqual(list.length, oldlength + 1);
  assertList(list, data);

  list.add(item);
  data.push(item.guid);
  assertEqual(list.contains(item), true);
  assertEqual(list.length, oldlength + 2);
  assertList(list, data);

  list.add(list);
  data.push(list.guid);
  assertEqual(list.contains(item), true);
  assertEqual(list.length, oldlength + 3);
  assertList(list, data);

  // Add the entire database to the list
  var view = library;
  var a = readList("data_sort_created_asc.txt");

  list.addAll(view);
  a.forEach(function(e) { data.push(e); });
  assertList(list, data);
  assertEqual(list.length, oldlength + 3 + view.length);

  // And add it again
  var simpleEnumerator = new TestMediaListEnumerationListener();
  view.enumerateAllItems(simpleEnumerator,
                         Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
  list.addSome(simpleEnumerator);

  a.forEach(function(e) { data.push(e); });
  assertList(list, data);
  assertEqual(list.length, oldlength + 3 + view.length + view.length);
  
  var asyncListener = {
    _itemsProcessed: 0,
    _complete: false,
    onProgress: function(aItemsProcessed, aComplete) {
      if(aComplete) {
        this._itemsProcessed = aItemsProcessed;
        this._complete = true;
        testFinished();
      }
    },
    onItemAdded: function(aMediaItem) {},
    onComplete: function() {}
  };
  
  simpleEnumerator.reset();
  view.enumerateAllItems(simpleEnumerator,
                         Ci.sbIMediaList.ENUMERATIONTYPE_SNAPSHOT);
  {
    // This is kind of annoying but necessary because XPC shell only has
    // one JS context. If we don't do this XPC shell will lock up.
    var tempArray = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                      .createInstance(Ci.nsIMutableArray);
    while(simpleEnumerator.hasMoreElements()) {
      tempArray.appendElement(simpleEnumerator.getNext(), false);
    }
    
    var tempEnumerator = tempArray.enumerate();
    log("Testing addSomeAsync");
    list.addMediaItems(tempEnumerator, asyncListener, true);
    testPending();
  }
  
  log("Processed " + asyncListener._itemsProcessed + " items.");
  a.forEach(function(e) { data.push(e); });
  assertList(list, data);
  assertEqual(list.length, oldlength + 3 + (view.length * 3));

  // Test insertBefore.  These tests seem a bit random but they are testing
  // all the code paths in sbLocalDatabaseSimpleMediaList::GetBeforeOrdinal
  library = createLibrary(databaseGUID);
  list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  a = readList("data_sort_sml101_ordinal_asc.txt");

  // insert at top
  item = library.getMediaItem("3E6DD1C2-AD99-11DB-9321-C22AB7121F49");
  list.insertBefore(0, item);
  a.unshift(item.guid);
  assertList(list, a);

  // insert before last
  item = library.getMediaItem("3E6D8050-AD99-11DB-9321-C22AB7121F49");
  list.insertBefore(list.length - 1, item);
  a.splice(a.length - 1, 0, item.guid);
  assertList(list, a);

  // insert above the previous insert
  item = library.getMediaItem("3E6D3050-AD99-11DB-9321-C22AB7121F49");
  list.insertBefore(list.length - 2, item);
  a.splice(a.length - 2, 0, item.guid);
  assertList(list, a);

  // insert befoe last again
  item = library.getMediaItem("3E6CDB1E-AD99-11DB-9321-C22AB7121F49");
  list.insertBefore(list.length - 1, item);
  a.splice(a.length - 1, 0, item.guid);
  assertList(list, a);

  // insert above the previous insert
  item = library.getMediaItem("3E6C8D80-AD99-11DB-9321-C22AB7121F49");
  list.insertBefore(list.length - 2, item);
  a.splice(a.length - 2, 0, item.guid);
  assertList(list, a);

  // test bad index
  try {
    list.insertBefore(list.length, item);
    fail("NS_ERROR_INVALID_ARG not thrown");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  // Test insertSomeBefore
  var items = [];
  items[0] = library.getMediaItem("3E6DD1C2-AD99-11DB-9321-C22AB7121F49");
  items[1] = library.getMediaItem("3E6D8050-AD99-11DB-9321-C22AB7121F49");
  items[2] = library.getMediaItem("3E6D3050-AD99-11DB-9321-C22AB7121F49");
  list.insertSomeBefore(0, new SimpleArrayEnumerator(items));
  a.unshift(items[2].guid);
  a.unshift(items[1].guid);
  a.unshift(items[0].guid);
  assertList(list, a);

  list.insertSomeBeforeAsync(0,
                             new SimpleArrayEnumerator(items),
                             asyncListener);
  testPending();
  a.unshift(items[2].guid);
  a.unshift(items[1].guid);
  a.unshift(items[0].guid);
  assertList(list, a);

  // Test moveBefore
  library = createLibrary(databaseGUID);
  list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  a = readList("data_sort_sml101_ordinal_asc.txt");

  // move from second to first
  list.moveBefore(1, 0);
  a.unshift(a.splice(1, 1)[0]);
  assertList(list, a);

  // move from first to before last
  list.moveBefore(0, list.length - 1);
  var guid = a.splice(0, 1)[0];
  a.splice(a.length - 1, 0, guid);
  assertList(list, a);

  // Test moveLast
  list.moveLast(0);
  guid = a.splice(0, 1)[0];
  a.push(guid);
  assertList(list, a);

  // Test moveSomeBefore
  var indexes = [5, 6, 7, 8, 9];
  list.moveSomeBefore(indexes, indexes.length, 0);
  var b = a.splice(5, 5);
  a.unshift(b[0], b[1], b[2], b[3], b[4]);
  assertList(list, a);

  // Test moveSomeLast
  list.moveSomeLast(indexes, indexes.length);
  var b = a.splice(5, 5);
  a = a.concat(b);
  assertList(list, a);

  // Test insertAllBefore.
  var insertItems = [];
  insertItems[0] = library.getMediaItem("3E6DD1C2-AD99-11DB-9321-C22AB7121F49");
  insertItems[1] = library.getMediaItem("3E6D8050-AD99-11DB-9321-C22AB7121F49");
  insertItems[2] = library.getMediaItem("3E6D3050-AD99-11DB-9321-C22AB7121F49");
  var insertList = library.createMediaList("simple");
  insertList.addSome(new SimpleArrayEnumerator(insertItems));
  list.insertAllBefore(0, insertList);
  a.unshift(insertItems[2].guid);
  a.unshift(insertItems[1].guid);
  a.unshift(insertItems[0].guid);
  assertList(list, a);

  // Test insertAllBefore again to ensure ordinals are set properly.
  list.insertAllBefore(0, insertList);
  a.unshift(insertItems[2].guid);
  a.unshift(insertItems[1].guid);
  a.unshift(insertItems[0].guid);
  assertList(list, a);

  // Move items added with insertAllBefore to the end of the list. Otherwise,
  // removal tests will fail because they require the items removed from the
  // beginning of the list to only exist once on the list.
  var indexes = [0, 1, 2, 3, 4, 5];
  list.moveSomeLast(indexes, indexes.length);
  b = a.splice(0, 6);
  a = a.concat(b);
  assertList(list, a);

  // Test remove
  item = library.getMediaItem("3E586C1A-AD99-11DB-9321-C22AB7121F49");
  assertEqual(list.contains(item), true);
  oldlength = list.length;
  list.remove(item);
  assertEqual(list.contains(item), false);
  assertEqual(list.length, oldlength - 1);

  item = list.getItemByIndex(0);
  oldlength = list.length;
  assertEqual(list.contains(item), true);
  list.removeByIndex(0);
  assertEqual(list.contains(item), false);
  assertEqual(list.length, oldlength - 1);

  // Remove an item that is not in the list should do nothing
  var notinlist = library.createMediaItem(newURI("http://foo.com/blah"));
  list.remove(notinlist);

  // test bad index
  try {
    list.removeByIndex(list.length);
    fail("NS_ERROR_INVALID_ARG not thrown");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  var toRemove = [
    list.getItemByIndex(0),
    list.getItemByIndex(1),
    list.getItemByIndex(2),
    list.getItemByIndex(3),
    list.getItemByIndex(4)
  ];
  oldlength = list.length;
  toRemove.forEach(function(item) { assertEqual(list.contains(item), true); });
  list.removeSome(new SimpleArrayEnumerator(toRemove));
  toRemove.forEach(function(item) { assertEqual(list.contains(item), false); });
  assertEqual(list.length, oldlength - toRemove.length);

  // Test clear
  list.clear();
  assertEqual(list.length, 0);

  // Test duplicate items in list
  library = createLibrary(databaseGUID);
  var item1 = library.createMediaItem(newURI("file:///foo"));
  var item2 = library.createMediaItem(newURI("file:///foo"));
  list = library.createMediaList("simple");

  a = [item1.guid, item2.guid, item1.guid, item2.guid];
  list.add(item1);
  list.add(item2);
  list.add(item1);
  list.add(item2);
  assertList(list, a);

  list.removeByIndex(2);
  a = [item1.guid, item2.guid, item2.guid];
  assertList(list, a);

  list.removeByIndex(2);
  a = [item1.guid, item2.guid];
  assertList(list, a);

  // Test that removal actually deletes the first instance
  list = library.createMediaList("simple");
  list.add(item1);
  list.add(item2);
  list.add(item1);
  list.add(item2);
  list.add(item2);
  list.add(item1);
  list.add(item2);
  list.add(item2);
  list.add(item2);

  a = [item1.guid, item2.guid, item1.guid, item2.guid, item2.guid, item1.guid, item2.guid, item2.guid, item2.guid];
  assertList(list, a);

  list.remove(item1);
  a = [item2.guid, item1.guid, item2.guid, item2.guid, item1.guid, item2.guid, item2.guid, item2.guid];
  assertList(list, a);

  list.remove(item1);
  a = [item2.guid, item2.guid, item2.guid, item1.guid, item2.guid, item2.guid, item2.guid];
  assertList(list, a);

  list.remove(item1);
  a = [item2.guid, item2.guid, item2.guid, item2.guid, item2.guid, item2.guid];
  assertList(list, a);

  // Test adding foreign and already existing items to the list
  localLibrary = createLibrary("test_simplemedialist_local");
  let localLibraryLength = localLibrary.length;
  foreignLibrary = createLibrary("test_simplemedialist_foreign");
  
  let items = [
    foreignLibrary.createMediaItem(newURI("file:///foo1")),
    foreignLibrary.createMediaItem(newURI("file:///foo2")),
    localLibrary.createMediaItem(newURI("file:///foo3")) 
  ];
  localLibrary.add(items[1]);
  list = localLibrary.createMediaList("simple");
  list.addSome(ArrayConverter.enumerator(items));
  
  // new items should be 4, 3 new items and the playlist
  assertEqual(localLibrary.length - localLibraryLength, 4);
}

