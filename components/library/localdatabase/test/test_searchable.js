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

  Components.utils.import("resource://app/components/sbProperties.jsm");
  Components.utils.import("resource://app/components/sbLibraryUtils.jsm");

  var library = createLibrary("test_searchable");

  // Tests with view media list
  var list = library;
  var view = list.createView();

  var search = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC"]]
    ]
  ]);

  view.searchConstraint = search;
  assertEqual(view.length, 10);

  search = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC"]],
      [SBProperties.trackName,  ["AC/DC"]]
    ],
    [
      [SBProperties.artistName, ["Thrill"]],
      [SBProperties.trackName,  ["Thrill"]]
    ]
  ]);
  view.searchConstraint = search;
  assertEqual(view.length, 1);

  view.searchConstraint = null;
  assertEqual(view.length, library.length);

  // Test unsupported constraints
  search = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC"]],
      [SBProperties.albumName,  ["AC/DC"]]
    ],
    [
      [SBProperties.artistName, ["Thrill"]],
      [SBProperties.trackName,  ["Thrill"]]
    ]
  ]);

  try {
    view.searchConstraint = search;
    fail("did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  search = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC"]],
      [SBProperties.trackName,  ["foo"]],
      [SBProperties.albumName,  ["foo"]]
    ],
    [
      [SBProperties.artistName, ["Thrill"]],
      [SBProperties.trackName,  ["Thrill"]],
      [SBProperties.albumName,  ["Thrill"]]
    ]
  ]);

  try {
    view.searchConstraint = search;
    fail("did not throw");
  }
  catch(e) {
    assertEqual(e.result, Cr.NS_ERROR_INVALID_ARG);
  }

  // Test with simple media list
  list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  view = list.createView();
  search = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC"]]
    ]
  ]);
  view.searchConstraint = search;
  assertEqual(view.length, 10);

  view.searchConstraint = null;
  assertEqual(view.length, 20);
  
  // Test the search box
  searchBoxTest("ac dc", 10);
  searchBoxTest("you ace", 4);
  searchBoxTest("of to the", 2);
}

function searchBoxTest(searchTerm, resultViewLength) {
  var library = createLibrary("test_searchBoxset");
  var view = library.createView();
  var cfs = view.cascadeFilterSet;

  cfs.appendSearch(["*"], 1);
  var searchArray = searchTerm.split(" ");  
  cfs.set(0, searchArray, searchArray.length);
  assertEqual(view.length, resultViewLength);
}

function createPropertyArray() {
  return Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
           .createInstance(Ci.sbIMutablePropertyArray);
}

