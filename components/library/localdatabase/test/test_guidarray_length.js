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

  var databaseGUID = "test_guidarray_length";
  var library = createLibrary(databaseGUID);
  var listId = library.QueryInterface(Ci.sbILocalDatabaseLibrary).getMediaItemIdForGuid("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  var a;

  var array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/GUIDArray;1"]
                .createInstance(Ci.sbILocalDatabaseGUIDArray);
  array.databaseGUID = databaseGUID;
  array.propertyCache =
    library.QueryInterface(Ci.sbILocalDatabaseLibrary).propertyCache;

  // Length checks, use the same sort for all of these since it does not
  // matter
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#albumName", true);

  // Full library, unfiltered
  assertEqual(array.length, 101);

  // Full library, property filtered
  array.addFilter("http://songbirdnest.com/data/1.0#albumName",
                  new StringArrayEnumerator(["back in black"]),
                  false);
  assertEqual(array.length, 10);
  // Add another filter level
  array.addFilter("http://songbirdnest.com/data/1.0#artistName",
                  new StringArrayEnumerator(["ac/dc"]),
                  false);
  assertEqual(array.length, 10);
  // And another
  array.addFilter("http://songbirdnest.com/data/1.0#trackName",
                  new StringArrayEnumerator(["hells bells"]),
                  false);
  assertEqual(array.length, 1);
  array.clearFilters();

  // Full library, top level filter
  array.addFilter("http://songbirdnest.com/data/1.0#contentURL",
                  new StringArrayEnumerator(["file:///home/steve/Shoot%20to%20Thrill.mp3",
                                             "file:///home/steve/That%20Jim.mp3",
                                             "file:///home/steve/You%20Shook%20Me%20All%20Night%20Long.mp3"]),
                  false);
  assertEqual(array.length, 3);
  // Add another filter level
  array.addFilter("http://songbirdnest.com/data/1.0#contentLength",
                  new StringArrayEnumerator(["3300", "840"]),
                  false);
  assertEqual(array.length, 2);
  array.clearFilters();

  // Full library, mixed filtered
  array.addFilter("http://songbirdnest.com/data/1.0#contentURL",
                  new StringArrayEnumerator(["file:///home/steve/Shoot%20to%20Thrill.mp3",
                                             "file:///home/steve/That%20Jim.mp3",
                                             "file:///home/steve/You%20Shook%20Me%20All%20Night%20Long.mp3"]),
                  false);
  array.addFilter("http://songbirdnest.com/data/1.0#artistName",
                  new StringArrayEnumerator(["ac/dc"]),
                  false);
  assertEqual(array.length, 2);
  array.clearFilters();

  // Full library, property search
  array.addFilter("http://songbirdnest.com/data/1.0#artistName",
                  new StringArrayEnumerator(["ac"]),
                  true);
  assertEqual(array.length, 47);
  // Add another search
  array.addFilter("http://songbirdnest.com/data/1.0#albumName",
                  new StringArrayEnumerator(["back"]),
                  true);
  assertEqual(array.length, 10);
  array.clearFilters();

  // Full library, mixed property filter and search
  array.addFilter("http://songbirdnest.com/data/1.0#artistName",
                  new StringArrayEnumerator(["ac/dc"]),
                  false);
  array.addFilter("http://songbirdnest.com/data/1.0#trackName",
                  new StringArrayEnumerator(["hell"]),
                  true);
  assertEqual(array.length, 1);
  array.clearFilters();

  // Length tests on simple media list
  array.baseTable = "simple_media_lists";
  array.baseConstraintColumn = "media_item_id";
  array.baseConstraintValue = listId;

  // Simple media list, unfiltered
  assertEqual(array.length, 20);

  // Simple media list, property filtered
  array.addFilter("http://songbirdnest.com/data/1.0#artistName",
                  new StringArrayEnumerator(["ac/dc"]),
                  false);
  assertEqual(array.length, 10);
  // Add a filter
  array.addFilter("http://songbirdnest.com/data/1.0#albumName",
                  new StringArrayEnumerator(["back in black"]),
                  false);
  assertEqual(array.length, 10);
  array.clearFilters();

  // Simple media list, top level filter
  array.addFilter("http://songbirdnest.com/data/1.0#contentURL",
                  new StringArrayEnumerator(["file:///home/steve/Shoot%20to%20Thrill.mp3",
                                             "file:///home/steve/Take%20on%20Me.mp3",
                                             "file:///home/steve/You%20Shook%20Me%20All%20Night%20Long.mp3",
                                             "file:///home/steve/Train%20of%20Thought.mp3"]),
                  false);
  assertEqual(array.length, 4);
  // Add another filter
  array.addFilter("http://songbirdnest.com/data/1.0#contentLength",
                  new StringArrayEnumerator(["2760", "660"]),
                  false);
  assertEqual(array.length, 2);
  array.clearFilters();

  // Simple media list, mixed filtered
  array.addFilter("http://songbirdnest.com/data/1.0#contentURL",
                  new StringArrayEnumerator(["file:///home/steve/Shoot%20to%20Thrill.mp3",
                                             "file:///home/steve/Take%20on%20Me.mp3",
                                             "file:///home/steve/You%20Shook%20Me%20All%20Night%20Long.mp3",
                                             "file:///home/steve/Train%20of%20Thought.mp3"]),
                  false);
  array.addFilter("http://songbirdnest.com/data/1.0#albumName",
                  new StringArrayEnumerator(["back in black"]),
                  false);
  assertEqual(array.length, 2);
  array.clearFilters();

  // Simple media list, property search
  array.addFilter("http://songbirdnest.com/data/1.0#artistName",
                  new StringArrayEnumerator(["ac"]),
                  true);
  assertEqual(array.length, 10);
  // Add another search
  array.addFilter("http://songbirdnest.com/data/1.0#albumName",
                  new StringArrayEnumerator(["back"]),
                  true);
  assertEqual(array.length, 10);
  array.clearFilters();

  // Simple media list, mixed property filter and search
  array.addFilter("http://songbirdnest.com/data/1.0#artistName",
                  new StringArrayEnumerator(["ac/dc"]),
                  false);
  array.addFilter("http://songbirdnest.com/data/1.0#trackName",
                  new StringArrayEnumerator(["hell"]),
                  true);
  assertEqual(array.length, 1);
  array.clearFilters();
}

