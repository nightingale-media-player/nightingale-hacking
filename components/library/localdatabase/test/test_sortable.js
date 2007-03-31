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

  var library = createLibrary("test_sortable");

  // Tests with view media list
  var list = library.QueryInterface(Ci.sbIMediaList);
  var view = list.createView();
  assertFilteredSort(view, "data_sort_created_asc.txt");

  var pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#contentUrl", "a");
  view.setSort(pa);
  assertFilteredSort(view, "data_sort_contenturl_asc.txt");

  pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#albumName", "a");
  pa.appendProperty("http://songbirdnest.com/data/1.0#track", "a");
  view.setSort(pa);
  assertFilteredSort(view, "data_sort_album_asc_track_asc.txt");

  pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "a");
  pa.appendProperty("http://songbirdnest.com/data/1.0#albumName", "a");
  pa.appendProperty("http://songbirdnest.com/data/1.0#track", "a");
  view.setSort(pa);
  assertFilteredSort(view, "data_sort_artist_asc_album_asc_track_asc.txt");

  view.clearSort();
  assertFilteredSort(view, "data_sort_created_asc.txt");

  // Test with simple media list
  list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  view = list.createView();
  assertFilteredSort(view, "data_sort_sml101_ordinal_asc.txt");

  pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#ordinal", "a");
  view.setSort(pa);
  assertFilteredSort(view, "data_sort_sml101_ordinal_asc.txt");
}

function createPropertyArray() {
  return Cc["@songbirdnest.com/Songbird/Properties/PropertyArray;1"].createInstance(Ci.sbIPropertyArray);
}

function assertFilteredSort(list, dataFile) {

  var a = [];
  for (var i = 0; i < list.length; i++) {
    a.push(list.getItemByIndex(i).guid);
  }

  assertArray(a, dataFile);

}

