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

  Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

  var databaseGUID = "test_simplemedialistnotifications";
  var library = createLibrary(databaseGUID, null, false);

  var item = library.createMediaItem(newURI("http://foo.example.com/"));
  var list = library.createMediaList("simple");

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

  // test onAdded
  list.add(item);
  assertEqual(libraryListener.added.length, 0);
  assertTrue(item.equals(listListener.added[0].item));
  assertEqual(listListener.added[0].index, 0);

  libraryListener.reset();
  listListener.reset();

  var anotheritem = library.createMediaItem(newURI("http://foo.example.com/1"));
  list.add(anotheritem);
  assertTrue(anotheritem.equals(listListener.added[0].item));
  assertEqual(listListener.added[0].index, 1);

  libraryListener.reset();
  listListener.reset();

  var otheritem2 = library.createMediaItem(newURI("http://foo.example.com/2"));
  var otheritem3 = library.createMediaItem(newURI("http://foo.example.com/3"));
  var e = ArrayConverter.enumerator([otheritem2, otheritem3]);
  list.addSome(e);
  assertTrue(otheritem2.equals(listListener.added[0].item));
  assertEqual(listListener.added[0].index, 2);
  assertTrue(otheritem3.equals(listListener.added[1].item));
  assertEqual(listListener.added[1].index, 3);

  libraryListener.reset();
  listListener.reset();

  var otheritem4 = library.createMediaItem(newURI("http://foo.example.com/4"));
  list.insertBefore(2, otheritem4);
  assertTrue(otheritem4.equals(listListener.added[0].item));
  assertEqual(listListener.added[0].index, 2);

  libraryListener.reset();
  listListener.reset();

  var otheritem5 = library.createMediaItem(newURI("http://foo.example.com/5"));
  var otheritem6 = library.createMediaItem(newURI("http://foo.example.com/6"));
  var e = ArrayConverter.enumerator([otheritem5, otheritem6]);
  list.insertSomeBefore(3, e);
  assertTrue(otheritem5.equals(listListener.added[0].item));
  assertEqual(listListener.added[0].index, 3);
  assertTrue(otheritem6.equals(listListener.added[1].item));
  assertEqual(listListener.added[1].index, 4);

  libraryListener.reset();
  listListener.reset();

  item.setProperty(prop, "foo");

  assertTrue(item.equals(libraryListener.updatedItem));
  assertTrue(item.equals(listListener.updatedItem));

  // test moving
  list.clear();
  var item1 = library.createMediaItem(newURI("http://foo.example.com/"));
  var item2 = library.createMediaItem(newURI("http://foo.example.com/"));
  var item3 = library.createMediaItem(newURI("http://foo.example.com/"));
  var item4 = library.createMediaItem(newURI("http://foo.example.com/"));
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
  assertTrue(item.equals(libraryListener.removedBefore[0].item));
  assertTrue(item.equals(libraryListener.removedAfter[0].item));
  assertEqual(listListener.removedBefore.length, 0);
  assertEqual(listListener.removedAfter.length, 0);

  list.clear();
  item1 = library.createMediaItem(newURI("http://foo.example.com/r1"));
  item2 = library.createMediaItem(newURI("http://foo.example.com/r2"));
  item3 = library.createMediaItem(newURI("http://foo.example.com/r3"));
  item4 = library.createMediaItem(newURI("http://foo.example.com/r4"));
  var item5 = library.createMediaItem(newURI("http://foo.example.com/r5"));
  var item6 = library.createMediaItem(newURI("http://foo.example.com/r6"));
  list.add(item1);
  list.add(item2);
  list.add(item3);
  list.add(item4);
  list.add(item5);
  list.add(item6);
  libraryListener.reset();
  listListener.reset();

  list.remove(item4);

  assertEqual(libraryListener.removedBefore.length, 0);
  assertEqual(libraryListener.removedAfter.length, 0);
  assertTrue(item4.equals(listListener.removedBefore[0].item));
  assertEqual(listListener.removedBefore[0].index, 3);
  assertTrue(item4.equals(listListener.removedAfter[0].item));
  assertEqual(listListener.removedAfter[0].index, 3);

  libraryListener.reset();
  listListener.reset();

  // Test that items removed from the library that are in a list are notified
  // properly
  var oldIndex = library.indexOf(item2, 0);
  library.remove(item2);

  assertTrue(item2.equals(libraryListener.removedBefore[0].item));
  assertEqual(libraryListener.removedBefore[0].index, oldIndex);
  assertTrue(item2.equals(libraryListener.removedAfter[0].item));
  assertEqual(libraryListener.removedAfter[0].index, oldIndex);

  assertTrue(item2.equals(listListener.removedBefore[0].item));
  assertEqual(listListener.removedBefore[0].index, 1);
  assertTrue(item2.equals(listListener.removedAfter[0].item));
  assertEqual(listListener.removedAfter[0].index, 1);

  libraryListener.reset();
  listListener.reset();

  var e = ArrayConverter.enumerator([item5, item6]);
  list.removeSome(e);

  assertTrue(item5.equals(listListener.removedBefore[0].item));
  assertEqual(listListener.removedBefore[0].index, 2);
  assertTrue(item5.equals(listListener.removedAfter[0].item));
  assertEqual(listListener.removedAfter[0].index, 2);

  assertTrue(item6.equals(listListener.removedBefore[1].item));
  assertEqual(listListener.removedBefore[1].index, 3);
  assertTrue(item6.equals(listListener.removedAfter[1].item));
  assertEqual(listListener.removedAfter[1].index, 3);

  libraryListener.reset();
  listListener.reset();

  list.removeByIndex(1);
  assertTrue(item3.equals(listListener.removedBefore[0].item));
  assertEqual(listListener.removedBefore[0].index, 1);
  assertTrue(item3.equals(listListener.removedAfter[0].item));
  assertEqual(listListener.removedAfter[0].index, 1);

  // test clear
  item = library.createMediaItem(newURI("http://foo.example.com/"));
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

  item = library.createMediaItem(newURI("http://foo.example.com/"));
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

  assertEqual(item.contentSrc.spec, libraryListener2.added[0].item.contentSrc.spec);
  assertFalse(item.equals(libraryListener2.added[0].item));
  assertTrue(libraryListener2.added[0].item.library.equals(library2));

  library2.remove(libraryListener2.added[0].item);
  libraryListener2.reset();

  var listListener2 = new TestMediaListListener();
  list2.addListener(listListener2, false);

  list2.add(item);

  assertEqual(item.contentSrc.spec, libraryListener2.added[0].item.contentSrc.spec);
  assertFalse(item.equals(libraryListener2.added[0].item));
  assertEqual(item.contentSrc.spec, listListener2.added[0].item.contentSrc.spec);
  assertFalse(item.equals(listListener2.added[0].item));

  // Assert that the items received by the listener are from library2
  assertTrue(libraryListener2.added[0].list.library.equals(library2));
  assertTrue(libraryListener2.added[0].item.library.equals(library2));
  assertTrue(listListener2.added[0].list.library.equals(library2));
  assertTrue(listListener2.added[0].item.library.equals(library2));

  libraryListener2.reset();
  listListener2.reset();

  list2.add(item);

  assertEqual(item.contentSrc.spec, listListener2.added[0].item.contentSrc.spec);
  assertFalse(item.equals(listListener2.added[0].item));

  libraryListener2.reset();
  listListener2.reset();
  list2.removeListener(listListener2);
  library2.removeListener(libraryListener2);
  library2.clear();

  var listToCopy = library.createMediaList("simple");
  listToCopy.add(item);

  var listener = {
    adds: [],
    listened: [],
    onItemAdded: function onItemAdded(list, item) {
      this.adds.push({list: list, item: item});
      if (item instanceof Ci.sbIMediaList) {
        var l = item.QueryInterface(Ci.sbIMediaList);
        l.addListener(this, false);
        this.listened.push(l);
      }
      return false;
    },
    onBatchBegin: function onBatchBegin(list) {
    },
    onBatchEnd: function onBatchEnd(list) {
    },
    reset: function() {
      this.listened.forEach(function(e) {
        e.removeListener(this);
      }, this);
      this.adds = [];
      this.listened = [];
    }
  };
  library2.addListener(listener, false);

  var copiedList = library2.copyMediaList("simple", listToCopy, false);

  assertEqual(listener.adds.length, 3);

  // The first onItemAdded should be for the new list getting created
  assertTrue(listener.adds[0].list.equals(library2));
  assertTrue(listener.adds[0].item.equals(copiedList));
  assertTrue(listener.adds[0].item.library.equals(library2));

  // The second onItemAdded is for the item getting added to the library
  assertTrue(listener.adds[1].list.equals(library2));
  assertTrue(listener.adds[1].item.library.equals(library2));

  // The third onItemAdded is the new item getting added to the new list
  assertTrue(listener.adds[2].list.equals(copiedList));
  assertTrue(listener.adds[2].list.library.equals(library2));
  assertTrue(listener.adds[2].item.equals(listener.adds[1].item));
  assertTrue(listener.adds[2].item.library.equals(library2));

  // Add some items to library2's list so we can test insert
  list2 = library2.createMediaList("simple");
  listListener2 = new TestMediaListListener();
  list2.addListener(listListener2, false);
  list2.add(library2.createMediaItem(newURI("http://bar.com/1.mp3")));
  list2.add(library2.createMediaItem(newURI("http://bar.com/2.mp3")));

  listener.reset();
  listListener2.reset();

  // Test using insertBefore to insert a foreign item
  var foreignItem = library.createMediaItem(newURI("http://foo.example.com/f.mp3"));
  list2.insertBefore(1, foreignItem);

  // The first onItemAdded should be for the new item getting created
  assertTrue(listener.adds[0].list.equals(library2));
  assertTrue(listener.adds[0].list.library.equals(library2));
  assertTrue(listener.adds[0].item.library.equals(library2));
  assertEqual(listener.adds[0].item.contentSrc.spec, foreignItem.contentSrc.spec);

  // The second onItemAdded is for the item getting added to the list
  assertTrue(listListener2.added[0].list.equals(list2));
  assertTrue(listListener2.added[0].list.library.equals(library2));
  assertTrue(listListener2.added[0].item.library.equals(library2));
  assertTrue(listListener2.added[0].item.equals(listener.adds[0].item));

  listener.reset();
  listListener2.reset();

  var foreignItem2 = library.createMediaItem(newURI("http://foo.example.com/f2.mp3"));
  var foreignItem3 = library.createMediaItem(newURI("http://foo.example.com/f3.mp3"));
  var e = ArrayConverter.enumerator([foreignItem2, foreignItem3]);
  list2.insertSomeBefore(1, e);

  // The first onItemAddeds should be for the new items getting created
  assertTrue(listener.adds[0].list.equals(library2));
  assertTrue(listener.adds[0].list.library.equals(library2));
  assertTrue(listener.adds[0].item.library.equals(library2));
  assertEqual(listener.adds[0].item.contentSrc.spec, foreignItem2.contentSrc.spec);

  assertTrue(listener.adds[1].list.equals(library2));
  assertTrue(listener.adds[1].list.library.equals(library2));
  assertTrue(listener.adds[1].item.library.equals(library2));
  assertEqual(listener.adds[1].item.contentSrc.spec, foreignItem3.contentSrc.spec);

  // The second onItemAddeds are for the items getting added to the list
  assertTrue(listListener2.added[0].list.equals(list2));
  assertTrue(listListener2.added[0].list.library.equals(library2));
  assertTrue(listListener2.added[0].item.library.equals(library2));
  assertTrue(listListener2.added[0].item.equals(listener.adds[0].item));

  assertTrue(listListener2.added[1].list.equals(list2));
  assertTrue(listListener2.added[1].list.library.equals(library2));
  assertTrue(listListener2.added[1].item.library.equals(library2));
  assertTrue(listListener2.added[1].item.equals(listener.adds[1].item));

  // Clean up listeners
  listener.reset();
  library2.removeListener(listener);
  list2.removeListener(listListener2);
}
