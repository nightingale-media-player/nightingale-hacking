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

  var databaseGUID = "test_simplemedialistnotifications";
  var library = createLibrary(databaseGUID, null, false);

  var item = library.createMediaItem(newURI("http://foo.com/"));
  var list = library.createMediaList("simple");

  var libraryListener = new TestMediaListListener();
  library.addListener(libraryListener);
  var listListener = new TestMediaListListener();
  list.addListener(listListener);

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

  libraryListener.reset();
  listListener.reset();
  list.clear();

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
  list.addListener(listListener);

  libraryListener.reset();
  listListener.reset();

  // test begin/end update batch
  library.beginUpdateBatch();
  library.endUpdateBatch();
  assertTrue(library.equals(libraryListener.batchBeginList));
  assertTrue(library.equals(libraryListener.batchEndList));
  assertEqual(listListener.batchBeginList, null);
  assertEqual(listListener.batchEndList, null);

  libraryListener.reset();
  listListener.reset();

  list.beginUpdateBatch();
  list.endUpdateBatch();
  assertEqual(libraryListener.batchBeginList, null);
  assertEqual(libraryListener.batchEndList, null);
  assertTrue(list.equals(listListener.batchBeginList));
  assertTrue(list.equals(listListener.batchEndList));
}

