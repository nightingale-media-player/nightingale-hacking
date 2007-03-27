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

  var databaseGUID = "test_guidarray_distinct";
  var library = createLibrary(databaseGUID);
  var listId = library.QueryInterface(Ci.sbILocalDatabaseLibrary).getMediaItemIdForGuid("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  var array;

  array = makeArray(databaseGUID);
  array.isDistinct = true;
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#artistName", true);
  array.fetchSize = 1;
  assertDistinct(array, "data_distinct_artist.txt");

  array = makeArray(databaseGUID);
  array.isDistinct = true;
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#contentLength", true);
  array.fetchSize = 1;
  assertDistinct(array, "data_distinct_contentlength.txt");

  array = makeArray(databaseGUID);
  array.isDistinct = true;
  array.baseTable = "simple_media_lists";
  array.baseConstraintColumn = "media_item_id";
  array.baseConstraintValue = listId;
  array.addSort("http://songbirdnest.com/data/1.0#albumName", true);
  array.fetchSize = 10;
  assertDistinct(array, "data_distinct_sml101_album.txt");
}

function assertDistinct(array, dataFile) {

  var data = readFile(dataFile);
  var a = data.split("\n");

  if(a.length - 1 != array.length) {
    fail("distinct failed, length wrong, got " + array.length + " expected " + (a.length - 1));
  }

  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    if(array.getSortPropertyValueByIndex(i) != b[0]) {
      fail("distinct failed, index " + i + " got " + array.getSortPropertyValueByIndex(i) + " expected " + b[0]);
    }
  }

}

