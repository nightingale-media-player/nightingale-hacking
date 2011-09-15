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

  var one = createLibrary("multilibrary_one");
  one.clear();
  assertEqual(one.length, 0);

  var two = createLibrary("multilibrary_two");
  two.clear();
  assertEqual(two.length, 0);

  var uri = newURI("http://www.poot.com");

  var item = one.createMediaItem(uri, null, true);
  assertEqual(one.length, 1);

  two.add(item);
  assertEqual(two.length, 1);

  one.clear();
  two.clear();
  assertEqual(one.length, 0);
  assertEqual(two.length, 0);

  var items = [];
  for (var i = 0; i < 10; i++) {
    items.push(one.createMediaItem(uri, null, true));
  }
  assertEqual(one.length, 10);

  var enumerator = new SimpleArrayEnumerator(items);
  two.addSome(enumerator);
  assertEqual(two.length, 10);

  two.clear();
  assertEqual(two.length, 0);

  var list1 = two.createMediaList("simple");
  assertEqual(list1.length, 0);

  var list2 = two.createMediaList("simple");
  assertEqual(list2.length, 0);
  assertEqual(two.length, 2);

  enumerator = new SimpleArrayEnumerator(items);
  list1.addSome(enumerator);

  assertEqual(list1.length, 10);
  assertEqual(list2.length, 0);
  assertEqual(two.length, 12); // 10 items 2 lists

  enumerator = new SimpleArrayEnumerator(items);
  // adding items that already exist should not give new media items in the library
  list2.addSome(enumerator);
  assertEqual(list1.length, 10);
  assertEqual(list2.length, 10);
  assertEqual(two.length, 12);
  assertNotEqual(list1, list2);

  list2.clear();
  assertEqual(list1.length, 10);
  assertEqual(list2.length, 0);
  assertEqual(two.length, 12);
  assertEqual(one.length, 10); // unchanged from before

  enumerator = new SimpleArrayEnumerator(items);
  list2.addSome(enumerator);
  assertEqual(list1.length, 10);
  assertEqual(list2.length, 10);
  assertEqual(two.length, 12);

  var list3 = one.copyMediaList("simple", list2, false);
  assertTrue(list3);
  assertEqual(list2.length, list3.length);
  assertEqual(list3.library, one);
  assertEqual(one.length, 11); // 10 items + 1 list

  var list4 = two.copyMediaList("simple", list2, false);
  assertTrue(list4);
  assertEqual(list2.length, list4.length);
  assertEqual(list4.library, two);
  assertEqual(two.length, 13);

  var list5 = two.copyMediaList("simple", list3, false);
  assertTrue(list5);
  assertEqual(list3.length, list5.length);
  assertEqual(list5.library, two);
  assertEqual(two.length, 14);

  two.remove(list3);
  two.remove(list4);
  two.remove(list5);
  assertEqual(list1.length, 10);
  for(var i = 0; i < 10; i++) {
    two.remove(list1.getItemByIndex(0));
  }
  assertEqual(list1.length, 0);
  assertEqual(list2.length, 0);
  assertEqual(two.length, 2);

  // test to make sure that the factory doesn't return different library
  // instances for the same database file.
  var libOne = createLibrary("multilibrary_libOne");
  var libTwo = createLibrary("multilibrary_libOne");
  assertTrue(libOne === libTwo); // same COM identity, not just equivalent

  var libOneLength = libOne.length;
  assertEqual(libOne.length, libTwo.length);

  libOne.createMediaItem(uri);
  assertNotEqual(libOneLength, libOne.length);

  assertEqual(libOne.length, libTwo.length);

  // Test cross-library adding of media lists
  libTwo = createLibrary("multilibrary_libTwo");
  var listener = new TestMediaListListener();
  libTwo.addListener(listener);

  list1 = libOne.createMediaList("simple");
  var list1GUID = list1.guid;

  list1.name = "Test";
  libTwo.add(list1);

  assertTrue(listener.added[0].item instanceof Ci.sbIMediaList);
  assertEqual(listener.added[0].item.length, 0);
  assertEqual(listener.added[0].item.name, "Test");
  assertNotEqual(listener.added[0].item.guid, list1GUID);

  // clean up the listener so we don't count a bogus leak
  listener.reset();
  libTwo.removeListener(listener);

}
