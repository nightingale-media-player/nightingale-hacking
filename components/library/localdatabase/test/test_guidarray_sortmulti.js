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
//  createDatabase(databaseGUID);
  var array;

  // Try a variety of fetch sizes to expose edge cases
  var fetchSizes = [1, 2, 5, 7, 10, 50, 100, 200];

  for(var i = 0; i < fetchSizes.length; i++) {

    array = makeArray(databaseGUID);
    array.baseTable = "media_items";
    array.addSort("http://songbirdnest.com/data/1.0#albumName", true);
    array.addSort("http://songbirdnest.com/data/1.0#track", true);
    array.fetchSize = fetchSizes[i];
    assertSort(array, "data_sort_album_asc_track_asc.txt");

    array = makeArray(databaseGUID);
    array.baseTable = "media_items";
    array.addSort("http://songbirdnest.com/data/1.0#albumName", true);
    array.addSort("http://songbirdnest.com/data/1.0#track", false);
    array.fetchSize = fetchSizes[i];
    assertSort(array, "data_sort_album_asc_track_desc.txt");
  }

  array = makeArray(databaseGUID);
  array.baseTable = "media_items";
  array.addSort("http://songbirdnest.com/data/1.0#artistName", true);
  array.addSort("http://songbirdnest.com/data/1.0#albumName", true);
  array.addSort("http://songbirdnest.com/data/1.0#track", true);
  array.fetchSize = 20;
  assertSort(array, "data_sort_artist_asc_album_asc_track_asc.txt");

}


