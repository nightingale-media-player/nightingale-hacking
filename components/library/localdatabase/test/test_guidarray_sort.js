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

  var databaseGUID = "test_guidarray_sort";
  var library = createLibrary(databaseGUID);
  var listId = library.QueryInterface(Ci.sbILocalDatabaseLibrary).getMediaItemIdForGuid("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  var array;

  // One level sort, small fetch size, ascending
  array = makeArray(library);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#trackName", true);
  array.fetchSize = 1;
  assertSort(array, "data_sort_trackname_asc.txt");

  // One level sort, small fetch size, descending
  array = makeArray(library);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#trackName", false);
  array.fetchSize = 2;
  assertSort(array, "data_sort_trackname_desc.txt");

  // One level sort, large fetch size, ascending
  array = makeArray(library);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#trackName", true);
  array.fetchSize = 20;
  assertSort(array, "data_sort_trackname_asc.txt");

  // One level sort, large fetch size, descending
  array = makeArray(library);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#trackName", false);
  array.fetchSize = 200;
  assertSort(array, "data_sort_trackname_desc.txt");

  // One level sort, ascending
  array = makeArray(library);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#playCount", true);
  array.fetchSize = 40;
  assertSort(array, "data_sort_playcount_asc.txt");

  // One level sort, descending
  array = makeArray(library);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#playCount", false);
  array.fetchSize = 40;
  assertSort(array, "data_sort_playcount_desc.txt");

  // One level top level property sort, ascending
  array = makeArray(library);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#contentURL", true);
  array.fetchSize = 40;
  assertSort(array, "data_sort_contenturl_asc.txt");

  // One level top level property sort, descending
  array = makeArray(library);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#contentURL", false);
  array.fetchSize = 40;
  assertSort(array, "data_sort_contenturl_desc.txt");

  // Sort a simple media list by the ordinal
  array = makeArray(library);
  array.baseTable = "simple_media_lists";
  array.baseConstraintColumn = "media_item_id";
  array.baseConstraintValue = listId;
  array.addSort("http://songbirdnest.com/data/1.0#ordinal", true);
  array.fetchSize = 40;
  assertSort(array, "data_sort_sml101_ordinal_asc.txt");

  array = makeArray(library);
  array.baseTable = "simple_media_lists";
  array.baseConstraintColumn = "media_item_id";
  array.baseConstraintValue = listId;
  array.addSort("http://songbirdnest.com/data/1.0#ordinal", false);
  array.fetchSize = 40;
  assertSort(array, "data_sort_sml101_ordinal_desc.txt");
}

