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
 * \brief Test file
 */

function runTest () {

  Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

  var databaseGUID = "test_librarynotifications";
  var library = createLibrary(databaseGUID, null, false);
  library.clear();

  var databaseGUID2 = "test_simplemedialistnotifications2";
  var library2 = createLibrary(databaseGUID2, null, false);
  library2.clear();

  var libraryListener = new TestMediaListListener();
  library.addListener(libraryListener, false);

  // test all the ways for onItemAdded to be called
  var item1 = library.createMediaItem(newURI("http://foo.com/1"));
  assertTrue(libraryListener.added[0].list.equals(library));
  assertTrue(libraryListener.added[0].item.equals(item1));
  assertEqual(libraryListener.added[0].index, library.length - 1);

  libraryListener.reset();

  var item2 = library.createMediaItem(newURI("http://foo.com/2"));
  assertTrue(libraryListener.added[0].list.equals(library));
  assertTrue(libraryListener.added[0].item.equals(item2));
  assertEqual(libraryListener.added[0].index, library.length - 1);

  libraryListener.reset();

  var list1 = library.createMediaList("simple");
  assertTrue(libraryListener.added[0].list.equals(library));
  assertTrue(libraryListener.added[0].item.equals(list1));
  assertEqual(libraryListener.added[0].index, library.length - 1);

  libraryListener.reset();

  var otheritem1 = library2.createMediaItem(newURI("http://foo.com/1"));
  library.add(otheritem1);
  assertTrue(libraryListener.added[0].list.equals(library));
  assertEqual(libraryListener.added[0].index, library.length - 1);

  libraryListener.reset();

  var otheritem2 = library2.createMediaItem(newURI("http://foo.com/2"));
  var otheritem3 = library2.createMediaItem(newURI("http://foo.com/3"));
  var e = ArrayConverter.enumerator([otheritem2, otheritem3]);
  library.addSome(e);
  assertTrue(libraryListener.added[0].list.equals(library));
  assertEqual(libraryListener.added[0].index, library.length - 2);
  assertTrue(libraryListener.added[1].list.equals(library));
  assertEqual(libraryListener.added[1].index, library.length - 1);

  libraryListener.reset();

  var items = library.batchCreateMediaItems(
    ArrayConverter.nsIArray([
      newURI("http://foo.com/4"),
      newURI("http://foo.com/5")
    ]));
  assertTrue(libraryListener.added[0].list.equals(library));
  assertTrue(libraryListener.added[0].item.equals(
    items.queryElementAt(0, Ci.sbIMediaItem)));
  assertEqual(libraryListener.added[0].index, library.length - 2);
  assertTrue(libraryListener.added[1].list.equals(library));
  assertTrue(libraryListener.added[1].item.equals(
    items.queryElementAt(1, Ci.sbIMediaItem)));
  assertEqual(libraryListener.added[1].index, library.length - 1);

  libraryListener.reset();

  // Test removal
  library.remove(item2);
  assertTrue(libraryListener.removedBefore[0].list.equals(library));
  assertTrue(libraryListener.removedBefore[0].item.equals(item2));
  assertEqual(libraryListener.removedBefore[0].index, 1);
  assertTrue(libraryListener.removedAfter[0].list.equals(library));
  assertTrue(libraryListener.removedAfter[0].item.equals(item2));
  assertEqual(libraryListener.removedAfter[0].index, 1);

  libraryListener.reset();

  var item6 = library.createMediaItem(newURI("http://foo.com/6"));
  var item7 = library.createMediaItem(newURI("http://foo.com/7"));
  var item8 = library.createMediaItem(newURI("http://foo.com/8"));
  e = ArrayConverter.enumerator([item6, item7, item8]);
  library.removeSome(e);
  assertTrue(libraryListener.removedBefore[0].list.equals(library));
  assertTrue(libraryListener.removedBefore[0].item.equals(item6));
  assertEqual(libraryListener.removedBefore[0].index, 7);
  assertTrue(libraryListener.removedAfter[0].list.equals(library));
  assertTrue(libraryListener.removedAfter[0].item.equals(item6));
  assertEqual(libraryListener.removedAfter[0].index, 7);
  assertTrue(libraryListener.removedBefore[1].list.equals(library));
  assertTrue(libraryListener.removedBefore[1].item.equals(item7));
  assertEqual(libraryListener.removedBefore[1].index, 7);
  assertTrue(libraryListener.removedAfter[1].list.equals(library));
  assertTrue(libraryListener.removedAfter[1].item.equals(item7));
  assertEqual(libraryListener.removedAfter[1].index, 7);
  assertTrue(libraryListener.removedBefore[2].list.equals(library));
  assertTrue(libraryListener.removedBefore[2].item.equals(item8));
  assertEqual(libraryListener.removedBefore[2].index, 7);
  assertTrue(libraryListener.removedAfter[2].list.equals(library));
  assertTrue(libraryListener.removedAfter[2].item.equals(item8));
  assertEqual(libraryListener.removedAfter[2].index, 7);

  library.removeListener(libraryListener);
}
