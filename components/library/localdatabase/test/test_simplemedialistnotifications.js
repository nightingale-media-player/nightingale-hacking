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

  var databaseGUID = "test_simplemedialistnotifications";
  var library = createLibrary(databaseGUID, null, false);

  var item = library.createMediaItem(newURI("http://foo.com/"));
  var list = library.createMediaList("simple");
  return;

  var libraryListener = new TestMediaListListener();
  library.addListener(libraryListener, false);
  var listListener = new TestMediaListListener();
  list.addListener(listListener, false);

  // test onItemUpdated
  var prop = "http://songbirdnest.com/data/1.0#albumName";
  item.setProperty(prop, "foo");
  assertTrue(item.equals(libraryListener.updatedItem));
  assertEqual(listListener.updatedItem, null);

  libraryListener.reset();
  listListener.reset();
  list.add(item);
  item.setProperty(prop, "foo");

  assertTrue(item.equals(libraryListener.updatedItem));
  assertTrue(item.equals(listListener.updatedItem));


  // test moving
  list.clear();
  var item1 = library.createMediaItem(newURI("http://foo.com/"));
  var item2 = library.createMediaItem(newURI("http://foo.com/"));
  var item3 = library.createMediaItem(newURI("http://foo.com/"));
  var item4 = library.createMediaItem(newURI("http://foo.com/"));
  list.add(item1);
  list.add(item2);
  list.add(item3);
  list.add(item4);

  libraryListener.reset();
  listListener.reset();

  // move one item up
  list.moveBefore(2, 0);
  assertEqual(listListener.movedItemFromIndex[0], 2);
  assertEqual(listListener.movedItemToIndex[0], 0);
  assertEqual(libraryListener.movedItemFromIndex.length, 0);
  assertEqual(libraryListener.movedItemToIndex.length, 0);

  libraryListener.reset();
  listListener.reset();

  // move one item down
  list.moveBefore(0, 3);
  assertEqual(listListener.movedItemFromIndex[0], 0);
  assertEqual(listListener.movedItemToIndex[0], 2);
  assertEqual(libraryListener.movedItemFromIndex.length, 0);
  assertEqual(libraryListener.movedItemToIndex.length, 0);

  libraryListener.reset();
  listListener.reset();

  // move one item last
  list.moveLast(1);
  assertEqual(listListener.movedItemFromIndex[0], 1);
  assertEqual(listListener.movedItemToIndex[0], 3);
  assertEqual(libraryListener.movedItemFromIndex.length, 0);
  assertEqual(libraryListener.movedItemToIndex.length, 0);

  libraryListener.reset();
  listListener.reset();

  // move two items up
  var a = [2, 3];
  list.moveSomeBefore(a, a.length, 0);
  assertEqual(listListener.movedItemFromIndex[0], 2);
  assertEqual(listListener.movedItemToIndex[0], 0);
  assertEqual(listListener.movedItemFromIndex[1], 3);
  assertEqual(listListener.movedItemToIndex[1], 1);
  assertEqual(libraryListener.movedItemFromIndex.length, 0);
  assertEqual(libraryListener.movedItemToIndex.length, 0);

  libraryListener.reset();
  listListener.reset();

  // move two items down
  a = [0, 1];
  list.moveSomeBefore(a, a.length, 3);
  assertEqual(listListener.movedItemFromIndex[0], 0);
  assertEqual(listListener.movedItemToIndex[0], 2);
  assertEqual(listListener.movedItemFromIndex[1], 0);
  assertEqual(listListener.movedItemToIndex[1], 2);
  assertEqual(libraryListener.movedItemFromIndex.length, 0);
  assertEqual(libraryListener.movedItemToIndex.length, 0);

  libraryListener.reset();
  listListener.reset();

  // move two items last
  a = [0, 1];
  list.moveSomeLast(a, a.length);
  assertEqual(listListener.movedItemFromIndex[0], 0);
  assertEqual(listListener.movedItemToIndex[0], 3);
  assertEqual(listListener.movedItemFromIndex[1], 0);
  assertEqual(listListener.movedItemToIndex[1], 3);

  list.clear();
  libraryListener.reset();
  listListener.reset();

  // test onBeforeItemRemoved/onAfterItemRemoved
  library.remove(item);
  assertTrue(item.equals(libraryListener.removedItemBefore));
  assertTrue(item.equals(libraryListener.removedItemAfter));
  assertEqual(listListener.removedItemBefore, null);
  assertEqual(listListener.removedItemAfter, null);

  list.clear();
  item = library.createMediaItem(newURI("http://foo.com/"));
  list.add(item);
  libraryListener.reset();
  listListener.reset();

  list.remove(item);

  assertEqual(libraryListener.removedItemBefore, null);
  assertEqual(libraryListener.removedItemAfter, null);
  assertTrue(item.equals(listListener.removedItemBefore));
  assertTrue(item.equals(listListener.removedItemAfter));

  libraryListener.reset();
  listListener.reset();
  list.add(item);
  library.remove(item);

  assertTrue(item.equals(libraryListener.removedItemBefore));
  assertTrue(item.equals(libraryListener.removedItemAfter));
  assertTrue(item.equals(listListener.removedItemBefore));
  assertTrue(item.equals(listListener.removedItemAfter));

  // test clear
  item = library.createMediaItem(newURI("http://foo.com/"));
  list.add(item);
  libraryListener.reset();
  listListener.reset();

  list.clear();
  assertFalse(libraryListener.listCleared);
  assertTrue(listListener.listCleared);

  libraryListener.reset();
  listListener.reset();

  library.clear();
  assertTrue(libraryListener.listCleared);
  assertTrue(listListener.listCleared);

  item = library.createMediaItem(newURI("http://foo.com/"));
  list = library.createMediaList("simple");
  list.addListener(listListener, false);

  libraryListener.reset();
  listListener.reset();

  // test begin/end update batch
  library.runInBatchMode(function() {});

  assertTrue(library.equals(libraryListener.batchBeginList));
  assertTrue(library.equals(libraryListener.batchEndList));
  assertEqual(listListener.batchBeginList, null);
  assertEqual(listListener.batchEndList, null);

  libraryListener.reset();
  listListener.reset();

  list.runInBatchMode(function() {});
  assertEqual(libraryListener.batchBeginList, null);
  assertEqual(libraryListener.batchEndList, null);
  assertTrue(list.equals(listListener.batchBeginList));
  assertTrue(list.equals(listListener.batchEndList));

  list.removeListener(listListener);
  library.removeListener(libraryListener);

  // Test cross-library operations
  var databaseGUID2 = "test_simplemedialistnotifications2";
  var library2 = createLibrary(databaseGUID2, null, false);
  var list2 = library2.createMediaList("simple");

  var libraryListener2 = new TestMediaListListener();
  library2.addListener(libraryListener2, false);

  library2.add(item);

  assertEqual(item.contentSrc.spec, libraryListener2.addedItem.contentSrc.spec);
  assertFalse(item.equals(libraryListener2.addedItem));

  library2.remove(libraryListener2.addedItem);
  libraryListener2.reset();

  var listListener2 = new TestMediaListListener();
  list2.addListener(listListener2, false);

  list2.add(item);

  assertEqual(item.contentSrc.spec, libraryListener2.addedItem.contentSrc.spec);
  assertFalse(item.equals(libraryListener2.addedItem));
  assertEqual(item.contentSrc.spec, listListener2.addedItem.contentSrc.spec);
  assertFalse(item.equals(listListener2.addedItem));

  list2.remove(item);
  libraryListener2.reset();
  listListener2.reset();

  list2.add(item);

  assertEqual(libraryListener2.addedItemm, null);
  assertEqual(item.contentSrc.spec, listListener2.addedItem.contentSrc.spec);
  assertFalse(item.equals(listListener2.addedItem));

  library2.clear();
  libraryListener2.reset();
  listListener2.reset();

  var listToCopy = library.createMediaList("simple");
  listToCopy.add(item);

  var listener = {
    adds: [],
    listened: [],
    onItemAdded: function onItemAdded(list, item) {
      this.adds.push({list: list, item: item});
      var l = item.QueryInterface(Ci.sbIMediaList);
      l.addListener(this, false);
      this.listened.push(l);
      return false;
    },
    onBatchBegin: function onBatchBegin(list) {
    },
    onBatchEnd: function onBatchEnd(list) {
    }
  };
  library2.addListener(listener, false);

  var copiedList = library2.copyMediaList("simple", listToCopy);

  assertEqual(listener.adds.length, 3);

  // The first onItemAdded should be for the new list getting created
  assertTrue(listener.adds[0].list.equals(library2));
  assertTrue(listener.adds[0].item.equals(copiedList));

  // The second onItemAdded is for the item getting added to the library
  assertTrue(listener.adds[1].list.equals(library2));

  // The third onItemAdded is the new item getting added to the new list
  assertTrue(listener.adds[2].list.equals(copiedList));
  assertTrue(listener.adds[2].item.equals(listener.adds[1].item));

  // Clean up listeners
  listener.listened.forEach(function(e) {
    e.removeListener(listener);
  });
  library2.removeListener(listener);
  list2.removeListener(listListener2);
  library2.removeListener(libraryListener2);
}
