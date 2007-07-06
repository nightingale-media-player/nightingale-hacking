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

  var sbIML = Ci.sbIMediaList;
  var SB_NS = "http://songbirdnest.com/data/1.0#";

  var databaseGUID = "test_simplemedialistnotifications";
  var library = createLibrary(databaseGUID, null, false);

  var listener = new TestMediaListListener();

  // Each individual notification type
  var flags = sbIML.LISTENER_FLAGS_ALL;
  library.addListener(listener, false, flags);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(flags, listener);

  library.removeListener(listener);
  listener.reset();

  flags = sbIML.LISTENER_FLAGS_ITEMADDED;
  library.addListener(listener, false, flags);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(flags, listener);

  library.removeListener(listener);
  listener.reset();

  flags = sbIML.LISTENER_FLAGS_BEFOREITEMREMOVED;
  library.addListener(listener, false, flags);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(flags, listener);

  library.removeListener(listener);
  listener.reset();

  flags = sbIML.LISTENER_FLAGS_AFTERITEMREMOVED;
  library.addListener(listener, false, flags);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(flags, listener);

  library.removeListener(listener);
  listener.reset();

  flags = sbIML.LISTENER_FLAGS_ITEMUPDATED;
  library.addListener(listener, false, flags);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(flags, listener);

  library.removeListener(listener);
  listener.reset();

  flags = sbIML.LISTENER_FLAGS_LISTCLEARED;
  library.addListener(listener, false, flags);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(flags, listener);

  library.removeListener(listener);
  listener.reset();

  flags = sbIML.LISTENER_FLAGS_BATCHBEGIN;
  library.addListener(listener, false, flags);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(flags, listener);

  library.removeListener(listener);
  listener.reset();

  flags = sbIML.LISTENER_FLAGS_BATCHEND;
  library.addListener(listener, false, flags);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(flags, listener);

  library.removeListener(listener);
  listener.reset();

  // Test multiple
  flags = sbIML.LISTENER_FLAGS_BATCHBEGIN |
          sbIML.LISTENER_FLAGS_BATCHEND |
          sbIML.LISTENER_FLAGS_ITEMUPDATED;
  library.addListener(listener, false, flags);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(flags, listener);

  library.removeListener(listener);
  listener.reset();

  // Test default param
  library.addListener(listener, false);
  doSomethingThatFiresAllEvents(library);
  assertFlagsMatch(sbIML.LISTENER_FLAGS_ALL, listener);

  library.removeListener(listener);
  listener.reset();

  // Test property filter
  var filter = createPropertyArray();
  filter.appendProperty(SB_NS + "albumName", null);
  filter.appendProperty(SB_NS + "trackNumber", null);
  library.addListener(listener, false, sbIML.LISTENER_FLAGS_ALL, filter);

  var item = library.createMediaItem(newURI("http://foo.com/"));
  item.setProperty(SB_NS + "artistName", "foo");
  assertEqual(listener.updatedItem, null);

  listener.reset();
  item.setProperty(SB_NS + "albumName", "foo");
  assertEqual(listener.updatedItem, item);

  listener.reset();
  item.setProperty(SB_NS + "trackNumber", "123");
  assertEqual(listener.updatedItem, item);

  // Test notification suppression
  // Not in batch, onItemUpdated returning true, should be notified
  listener.reset();
  listener.retval = true;
  item.setProperty(SB_NS + "albumName", "foo");
  assertEqual(listener.updatedItem, item);

  listener.reset();
  listener.retval = false;
  library.beginUpdateBatch();

  // In batch, onItemUpdated returning false, should be notified
  item.setProperty(SB_NS + "albumName", "foo");
  assertEqual(listener.updatedItem, item);

  listener.reset();
  listener.retval = true;

  // In batch, onItemUpdated returning true, should be notified since the
  // last call returned true
  item.setProperty(SB_NS + "albumName", "foo");
  assertEqual(listener.updatedItem, item);

  listener.reset();
  listener.retval = true;

  // In batch, onItemUpdated returning true, should not be notified
  item.setProperty(SB_NS + "albumName", "foo");
  assertEqual(listener.updatedItem, null);

  library.endUpdateBatch();

  listener.reset();
  listener.retval = true;

  // Out of the batch, should be notified
  item.setProperty(SB_NS + "albumName", "foo");
  assertEqual(listener.updatedItem, item);

  listener.reset();
  listener.retval = true;

  // Out of batch, should still be notified since we were not in a batch
  item.setProperty(SB_NS + "albumName", "foo");
  assertEqual(listener.updatedItem, item);

}

function doSomethingThatFiresAllEvents(library) {

  library.beginUpdateBatch();
  library.endUpdateBatch();
  var item = library.createMediaItem(newURI("http://foo.com/"));
  item.setProperty(SB_NS + "albumName", "foo");
  library.remove(item);
  library.clear();
}

function assertFlagsMatch(flags, listener) {

  if (flags & Ci.sbIMediaList.LISTENER_FLAGS_ITEMADDED) {
    assertNotEqual(listener.addedItem, null);
  }
  else {
    assertEqual(listener.addedItem, null);
  }

  if (flags & Ci.sbIMediaList.LISTENER_FLAGS_BEFOREITEMREMOVED) {
    assertNotEqual(listener.removedItemBefore, null);
  }
  else {
    assertEqual(listener.removedItemBefore, null);
  }

  if (flags & Ci.sbIMediaList.LISTENER_FLAGS_AFTERITEMREMOVED) {
    assertNotEqual(listener.removedItemAfter, null);
  }
  else {
    assertEqual(listener.removedItemAfter, null);
  }

  if (flags & Ci.sbIMediaList.LISTENER_FLAGS_ITEMUPDATED) {
    assertNotEqual(listener.updatedItem, null);
  }
  else {
    assertEqual(listener.updatedItem, null);
  }

  if (flags & Ci.sbIMediaList.LISTENER_FLAGS_LISTCLEARED) {
    assertTrue(listener.listCleared);
  }
  else {
    assertFalse(listener.listCleared);
  }

  if (flags & Ci.sbIMediaList.LISTENER_FLAGS_BATCHBEGIN) {
    assertNotEqual(listener.batchBeginList, null);
  }
  else {
    assertEqual(listener.batchBeginList, null);
  }

  if (flags & Ci.sbIMediaList.LISTENER_FLAGS_BATCHEND) {
    assertNotEqual(listener.batchEndList, null);
  }
  else {
    assertEqual(listener.batchEndList, null);
  }
}

function createPropertyArray() {
  return Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
           .createInstance(Ci.sbIMutablePropertyArray);
}

