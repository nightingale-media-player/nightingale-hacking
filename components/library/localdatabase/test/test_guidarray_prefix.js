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

  var databaseGUID = "test_guidarray_prefix";
  var library = createLibrary(databaseGUID);
  var listId = library.QueryInterface(Ci.sbILocalDatabaseLibrary)
                      .getMediaItemIdForGuid("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");

  var array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/GUIDArray;1"]
                .createInstance(Ci.sbILocalDatabaseGUIDArray);
  array.databaseGUID = databaseGUID;
  array.propertyCache =
    library.QueryInterface(Ci.sbILocalDatabaseLibrary).propertyCache;

  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#albumName", true);

  var tests = [
    "A",
    "B",
    "F",
    "From",
    "Back",
    "Life",
    "qqqqqqq"
  ];

  for (var i = 0; i < tests.length; i++) {
    var index = findFirstIndexByPrefix(array, tests[i]);
    if (index < 0) {
      try {
        array.getFirstIndexByPrefix(tests[i]);
        fail("NS_ERROR_NOT_AVAILABLE not thrown");
      }
      catch(e) {
        assertEqual(e.result, Cr.NS_ERROR_NOT_AVAILABLE);
      }
    }
    else {
      assertEqual(array.getFirstIndexByPrefix(tests[i]), index);
    }
  }

  array.addFilter("http://songbirdnest.com/data/1.0#genre",
                  new StringArrayEnumerator(["rock"]),
                  false);

  for (var i = 0; i < tests.length; i++) {
    var index = findFirstIndexByPrefix(array, tests[i]);
    if (index < 0) {
      try {
        array.getFirstIndexByPrefix(tests[i]);
        fail("NS_ERROR_NOT_AVAILABLE not thrown");
      }
      catch(e) {
        assertEqual(e.result, Cr.NS_ERROR_NOT_AVAILABLE);
      }
    }
    else {
      assertEqual(array.getFirstIndexByPrefix(tests[i]), index);
    }
  }

  // Test on a simple media list
  array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/GUIDArray;1"]
                .createInstance(Ci.sbILocalDatabaseGUIDArray);
  array.databaseGUID = databaseGUID;
  array.propertyCache =
    library.QueryInterface(Ci.sbILocalDatabaseLibrary).propertyCache;

  array.baseTable = "simple_media_lists";
  array.baseConstraintColumn = "media_item_id";
  array.baseConstraintValue = listId;

  array.addSort("http://songbirdnest.com/data/1.0#albumName", true);

  var tests = [
    "A",
    "B",
    "F",
    "From",
    "Back",
    "Life",
    "qqqqqqq"
  ];

  for (var i = 0; i < tests.length; i++) {
    var index = findFirstIndexByPrefix(array, tests[i]);
    if (index < 0) {
      try {
        array.getFirstIndexByPrefix(tests[i]);
        fail("NS_ERROR_NOT_AVAILABLE not thrown");
      }
      catch(e) {
        assertEqual(e.result, Cr.NS_ERROR_NOT_AVAILABLE);
      }
    }
    else {
      assertEqual(array.getFirstIndexByPrefix(tests[i]), index);
    }
  }

  array.addFilter("http://songbirdnest.com/data/1.0#genre",
                  new StringArrayEnumerator(["rock"]),
                  false);

  for (var i = 0; i < tests.length; i++) {
    var index = findFirstIndexByPrefix(array, tests[i]);
    if (index < 0) {
      try {
        array.getFirstIndexByPrefix(tests[i]);
        fail("NS_ERROR_NOT_AVAILABLE not thrown");
      }
      catch(e) {
        assertEqual(e.result, Cr.NS_ERROR_NOT_AVAILABLE);
      }
    }
    else {
      assertEqual(array.getFirstIndexByPrefix(tests[i]), index);
    }
  }

  array.clearSorts();
  array.clearFilters();

  // Special test when sorted by ordinal
  array.addSort("http://songbirdnest.com/data/1.0#ordinal", true);

  // The ordinals of the freshly loaded data start as equal to the index
  // number, so test with that
  for (let i = 0; i < array.length; i++) {
    assertEqual(i, array.getFirstIndexByPrefix(i));
  }

  array.addFilter("http://songbirdnest.com/data/1.0#artistName",
                  new StringArrayEnumerator(["a-ha"]),
                  false);

  for (let i = 10; i < array.length; i++) {
    assertEqual(i - 10, array.getFirstIndexByPrefix(i));
  }

}

function findFirstIndexByPrefix(array, prefix) {

  var length = array.length;
  var re = new RegExp("^" + prefix);
  for (var i = 0; i < length; i++) {
    var value = array.getSortPropertyValueByIndex(i);
    if (value.match(re)) {
      return i;
    }
  }

  return -1;
}

