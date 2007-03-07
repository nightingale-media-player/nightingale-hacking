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

  var databaseGUID = "test_localdatabaselibrary";
  createDatabase(databaseGUID);
  var array;

  // One level sort, small fetch size, ascending
  array = makeArray(databaseGUID);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#trackName", true);
  array.fetchSize = 1;
  assertSort(array, "data_sort_trackname_asc.txt");

  // One level sort, small fetch size, descending
  array = makeArray(databaseGUID);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#trackName", false);
  array.fetchSize = 2;
  assertSort(array, "data_sort_trackname_desc.txt");

  // One level sort, large fetch size, ascending
  array = makeArray(databaseGUID);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#trackName", true);
  array.fetchSize = 20;
  assertSort(array, "data_sort_trackname_asc.txt");

  // One level sort, large fetch size, descending
  array = makeArray(databaseGUID);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#trackName", false);
  array.fetchSize = 200;
  assertSort(array, "data_sort_trackname_desc.txt");

  // One level sort, ascending
  array = makeArray(databaseGUID);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#playCount", true);
  array.fetchSize = 40;
  assertSort(array, "data_sort_playcount_asc.txt");

  // One level sort, descending
  array = makeArray(databaseGUID);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#playCount", false);
  array.fetchSize = 40;
  assertSort(array, "data_sort_playcount_desc.txt");

  // One level top level property sort, ascending
  array = makeArray(databaseGUID);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#contentUrl", true);
  array.fetchSize = 40;
  assertSort(array, "data_sort_contenturl_asc.txt");

  // One level top level property sort, descending
  array = makeArray(databaseGUID);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#contentUrl", false);
  array.fetchSize = 40;
  assertSort(array, "data_sort_contenturl_desc.txt");

  // Sort a simple media list by the ordinal
  array = makeArray(databaseGUID);
  array.baseTable = "simple_media_lists";
  array.baseConstraintColumn = "media_item_id";
  array.baseConstraintValue = 101;
  array.addSort("http://songbirdnest.com/data/1.0#ordinal", true);
  array.fetchSize = 40;
  assertSort(array, "data_sort_sml101_ordinal_asc.txt");

  array = makeArray(databaseGUID);
  array.baseTable = "simple_media_lists";
  array.baseConstraintColumn = "media_item_id";
  array.baseConstraintValue = 101;
  array.addSort("http://songbirdnest.com/data/1.0#ordinal", false);
  array.fetchSize = 40;
  assertSort(array, "data_sort_sml101_ordinal_desc.txt");
}

