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

  Components.utils.import("resource://app/components/sbProperties.jsm");

  var library = createLibrary("test_medialistviewclone", null, false);
  library.clear();

  var view = library.createView();
  var cfs = view.cascadeFilterSet;

  view.setSort(SBProperties.createArray([
    [SBProperties.artistName, "a"]
  ]));

  view.setFilters(SBProperties.createArray([
    [SBProperties.isList, "0"],
    [SBProperties.hidden, "0"]
  ]));

  cfs.appendSearch(["*"], 1);
  cfs.appendFilter(SBProperties.genre);
  cfs.appendFilter(SBProperties.artistName);
  cfs.appendFilter(SBProperties.albumName);

  cfs.set(0, ["Beat"], 1);
  cfs.set(1, ["ROCK"], 1);
  cfs.set(2, ["The Beatles"], 1);

  var clone = view.clone();

  assertPropertyArray(clone.currentSort, SBProperties.createArray([
    [SBProperties.artistName, "a"]
  ]));
  assertPropertyArray(clone.currentSearch, SBProperties.createArray([
    ["*", "Beat"]
  ]));
  assertPropertyArray(clone.currentFilter, SBProperties.createArray([
    [SBProperties.isList,      "0"],
    [SBProperties.hidden,      "0"],
    [SBProperties.genre,       "ROCK"], 
    [SBProperties.artistName,  "The Beatles"]
  ]));

  var cloneCfs = clone.cascadeFilterSet;
  assertEqual(cfs.length, cloneCfs.length);
  for (let i = 0; i < cfs.length; i++) {
    assertEqual(cfs.getProperty(i), cloneCfs.getProperty(i));
    assertEqual(cfs.isSearch(i), cloneCfs.isSearch(i));
  }
}

function assertPropertyArray(a1, a2) {
  if (a1.length != a2.length) {
    fail("Property array lengths differ, " + a1 +
         " != " + a2);
  }

  for(var i = 0; i < a1.length; i++) {
    var prop1 = a1.getPropertyAt(i);
    var prop2 = a2.getPropertyAt(i);
    if (prop1.name != prop2.name) {
      fail("Names are different at index " + i + ", " +
           prop1 + " != " + prop2);
    }
    if (prop1.value != prop2.value) {
      fail("Values are different at index " + i + ", " +
           prop1 + " != " + prop2);
    }
  }

}

