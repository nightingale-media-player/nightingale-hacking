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

  var databaseGUID = "test_guidarray_nullsorting";
  var library = createLibrary(databaseGUID);
  library.clear();

  // Set up a library with 20 items, 10 of which have a null content length 
  // and the rest have contents lengths 0 - 9
  var items = [];

  var array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/GUIDArray;1"]
                .createInstance(Ci.sbILocalDatabaseGUIDArray);
  array.databaseGUID = databaseGUID;
  array.baseTable = "media_items";
  array.propertyCache =
    library.QueryInterface(Ci.sbILocalDatabaseLibrary).propertyCache;

/*
  XXXsteve Going to disable this test since we now automagically fill in the
  contentLength for new items.  We can re-enable this either when we add the
  null support to setProperty or if we move the setting of this property to
  the metadata scanner

  for (var i = 0; i < 20; i++) {
    var item = library.createMediaItem(newURI("file://foo/" + i));
    if (i >= 10) {
      item.setProperty("http://songbirdnest.com/data/1.0#contentLength", i - 10);
    }
    items.push(item.guid);
  }

  // We build the array to match an ascending sort, so just test
  array.addSort("http://songbirdnest.com/data/1.0#contentLength", true);
  assertArraySame(array, items);

  // Reversing the sort should cause the nulls to go to the bottom (because
  // they are small) but remain in the same order.  The non-null items should
  // move to the top and be reversed.
  array.clearSorts();
  array.addSort("http://songbirdnest.com/data/1.0#contentLength", false);

  items = swap(items, 10);
  items = reverseRange(items, 0, 10);
  assertArraySame(array, items);
*/

  // Now test on a new property using different null sort configurations
  library.clear();
  var numberInfo = Cc["@songbirdnest.com/Songbird/Properties/Info/Number;1"]
                    .createInstance(Ci.sbINumberPropertyInfo);
  numberInfo.id = "http://songbirdnest.com/data/1.0#testNumber";
  numberInfo.nullSort = Ci.sbIPropertyInfo.SORT_NULL_SMALL;

  var propMan = Cc["@songbirdnest.com/Songbird/Properties/PropertyManager;1"]
                  .getService(Ci.sbIPropertyManager);
  propMan.addPropertyInfo(numberInfo);

  // Set up a library with 20 items, 10 of which have a null content length 
  // and the rest have contents lengths 0 - 9
  items = [];
  for (var i = 0; i < 20; i++) {
    var item = library.createMediaItem(newURI("file://foo/" + i));
    if (i >= 10) {
      item.setProperty("http://songbirdnest.com/data/1.0#testNumber", i - 10);
    }
    items.push(item.guid);
  }

  array.clearSorts();
  array.addSort("http://songbirdnest.com/data/1.0#testNumber", true);
  assertArraySame(array, items);

  // Reversing the sort should cause the nulls to go to the bottom (because
  // they are small) but remain in the same order.  The non-null items should
  // move to the top and be reversed.
  array.clearSorts();
  array.addSort("http://songbirdnest.com/data/1.0#testNumber", false);

  var sortNullSmall = swap(items, 10);
  sortNullSmall = reverseRange(sortNullSmall, 0, 10);
  assertArraySame(array, sortNullSmall);

  // Change property to sort nulls big
  numberInfo.nullSort = Ci.sbIPropertyInfo.SORT_NULL_BIG;

  // Now the nulls should be at the bottom
  array.clearSorts();
  array.addSort("http://songbirdnest.com/data/1.0#testNumber", true);

  var sortNullBig = swap(items, 10);
  assertArraySame(array, sortNullBig);

  // Reversung the sort moves the nulls to the top and reverses the non-null
  // items
  array.clearSorts();
  array.addSort("http://songbirdnest.com/data/1.0#testNumber", false);

  sortNullBig = reverseRange(items, 10, 10);
  assertArraySame(array, sortNullBig);

  // Change the property to sort nulls always first
  numberInfo.nullSort = Ci.sbIPropertyInfo.SORT_NULL_FIRST;

  // Always sorting nulls first will match our original data
  array.clearSorts();
  array.addSort("http://songbirdnest.com/data/1.0#testNumber", true);

  assertArraySame(array, items);

  // Reversing the sort should only reverse the non-null items, nulls should
  // stay on top
  array.clearSorts();
  array.addSort("http://songbirdnest.com/data/1.0#testNumber", false);

  var sortNullFirst = reverseRange(items, 10, 10);
  assertArraySame(array, sortNullFirst);

  // Change the property to sort nulls always last
  numberInfo.nullSort = Ci.sbIPropertyInfo.SORT_NULL_LAST;

  // Now the nulls should be at the bottom and the non-null should be in the
  // original order
  array.clearSorts();
  array.addSort("http://songbirdnest.com/data/1.0#testNumber", true);

  var sortNullLast = swap(items, 10);
  assertArraySame(array, sortNullLast);

  // Reversing the sort should only reverse the non-null items
  array.clearSorts();
  array.addSort("http://songbirdnest.com/data/1.0#testNumber", false);

  sortNullLast = reverseRange(sortNullLast, 0, 10);
  assertArraySame(array, sortNullLast);

}

function assertArraySame(guidArray, guids) {

  if (guidArray.length != guids.length) {
    fail("different lengths, " + guidArray.length + " != " + guids.length);
  }

  for (var i = 0; i < guidArray.length; i++) {
    if (guidArray.getGuidByIndex(i) != guids[i]) {
      fail("different items at " + i + ", " + guidArray.getGuidByIndex(i) + " != " + guids[i]);
    }
  }

}

function reverseRange(a, start, length) {

  var before = a.slice(0, start);
  var range =  a.slice(start, start + length);
  var after =  a.slice(start + length);
  range.reverse();
  return before.concat(range).concat(after);
}

function swap(a, length) {

  var before = a.slice(0, length);
  var after =  a.slice(length);

  return after.concat(before);
}

