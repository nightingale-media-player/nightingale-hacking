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

  var one = createLibrary("multilibrary_one");
  one.clear();
  assertTrue(one.length == 0);
  
  var two = createLibrary("multilibrary_two");
  two.clear();
  assertTrue(two.length == 0);
  
  var uri = newURI("http://www.poot.com");
  
  var item = one.createMediaItem(uri, null, true);
  assertTrue(one.length == 1);
  
  two.add(item);
  assertTrue(two.length == 1);
  
  one.clear();
  two.clear();
  assertTrue(one.length == 0 && two.length == 0);
  
  var items = [];
  for (var i = 0; i < 10; i++) {
    items.push(one.createMediaItem(uri, null, true));
  }
  assertTrue(one.length == 10);

  var enumerator = new SimpleArrayEnumerator(items);
  two.addSome(enumerator);
  assertTrue(two.length == 10);

  two.clear();
  assertTrue(two.length == 0);
  
  var list1 = two.createMediaList("simple");
  assertTrue(list1.length == 0);

  var list2 = two.createMediaList("simple");
  assertTrue(list2.length == 0);

  assertTrue(two.length == 2);
  
  enumerator = new SimpleArrayEnumerator(items);
  list1.addSome(enumerator);

  assertTrue(list1.length == 10);
  assertTrue(list2.length == 0);
  assertEqual(two.length, 12);
  
  enumerator = new SimpleArrayEnumerator(items);
  list2.addSome(enumerator);
  assertTrue(list1.length == 10);
  assertTrue(list2.length == 10);
  assertTrue(two.length == 22);
  
  list2.clear();
  assertTrue(list1.length == 10);
  assertTrue(list2.length == 0);
  assertTrue(two.length == 22);
  
  enumerator = new SimpleArrayEnumerator(items);
  list2.addSome(enumerator);
  assertTrue(list1.length == 10);
  assertTrue(list2.length == 10);
  assertTrue(two.length == 32);

  for(var i = 0; i < 10; i++) {
    two.remove(list1.getItemByIndex(0));
  }
  assertTrue(list1.length == 0);
  assertTrue(list2.length == 10);
  assertTrue(two.length == 22);
  
  var list3 = one.copyMediaList("simple", list2);
  assertTrue(list3);
  assertEqual(list2.length, list3.length); 
  assertEqual(list3.library, one);
  
  var list4 = two.copyMediaList("simple", list2);
  assertTrue(list4);
  assertEqual(list2.length, list4.length); 
  assertEqual(list4.library, two);
  
  var list5 = two.copyMediaList("simple", list3);
  assertTrue(list5);
  assertEqual(list3.length, list5.length); 
  assertEqual(list5.library, two);
  
  // test to make sure that the factory doesn't return different library
  // instances for the same database file.
  var libOne = createLibrary("multilibrary_libOne");
  var libTwo = createLibrary("multilibrary_libOne");
  assertTrue(libOne === libTwo);
  
  var libOneLength = libOne.length;
  assertTrue(libOne.length == libTwo.length);
  
  libOne.createMediaItem(uri);
  assertTrue(libOneLength != libOne.length);
  
  assertTrue(libOne.length == libTwo.length);
}
