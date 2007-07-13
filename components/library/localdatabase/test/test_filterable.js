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

  var library = createLibrary("test_filterable");

  // Tests with view media list
  var list = library;
  var view = list.createView();
  var e = view.getFilterValues("http://songbirdnest.com/data/1.0#albumName");

  var array = [];
  while(e.hasMore()) {
    array.push(e.getNext());
  }
  assertArray(array, "data_filterable_view_album.txt");

  var pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "AC/DC");
  view.setFilters(pa);
  assertEqual(view.length, 10);

  // shows replace
  pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "Accept");
  view.setFilters(pa);
  assertEqual(view.length, 15);

  pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "AC/DC");
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "Accept");
  view.setFilters(pa);
  assertEqual(view.length, 25);

  pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "a-ha");
  view.updateFilters(pa);
  assertEqual(view.length, 35);

  pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#albumName", "Back in Black");
  view.updateFilters(pa);
  assertEqual(view.length, 10);

  pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#albumName", "Restless and Wild/Balls to the Wall");
  view.updateFilters(pa);
  assertEqual(view.length, 25);

  view.removeFilters(pa);
  assertEqual(view.length, 10);

  // Test with simple media list
  list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  view = list.createView();
  var e = view.getFilterValues("http://songbirdnest.com/data/1.0#albumName");
  array = [];
  while(e.hasMore()) {
    array.push(e.getNext());
  }
  assertArray(array, "data_filterable_sml101_album.txt");

  pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "AC/DC");
  view.setFilters(pa);
  assertEqual(view.length, 10);

  // test get
  var pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "AC/DC");
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "a-ha");
  pa.appendProperty("http://songbirdnest.com/data/1.0#albumName", "Back in Black");
  view.setFilters(pa);

  var filters = view.currentFilter;
  assertEqual(filters.length, 3);

  assertEqual(filters.getPropertyAt(0).name, "http://songbirdnest.com/data/1.0#artistName");
  assertEqual(filters.getPropertyAt(0).value, "AC/DC");
  assertEqual(filters.getPropertyAt(1).name, "http://songbirdnest.com/data/1.0#artistName");
  assertEqual(filters.getPropertyAt(1).value, "a-ha");
  assertEqual(filters.getPropertyAt(2).name, "http://songbirdnest.com/data/1.0#albumName");
  assertEqual(filters.getPropertyAt(2).value, "Back in Black");
}

function createPropertyArray() {
  return Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
           .createInstance(Ci.sbIMutablePropertyArray);
}

