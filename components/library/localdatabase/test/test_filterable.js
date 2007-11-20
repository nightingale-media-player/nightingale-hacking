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
  Components.utils.import("resource://app/components/sbLibraryUtils.jsm");

  var library = createLibrary("test_filterable");

  // Tests with view media list
  var list = library;
  var view = list.createView();

  var filter = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC"]]
    ]
  ]);
  view.filterConstraint = filter;
  assertTrue(filter.equals(view.filterConstraint));
  assertEqual(view.length, 10);

  filter = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC", "Accept"]]
    ]
  ]);
  view.filterConstraint = filter;
  assertEqual(view.length, 25);

  filter = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC", "Accept", "a-ha"]]
    ]
  ]);
  view.filterConstraint = filter;
  assertEqual(view.length, 35);

  filter = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC", "Accept", "a-ha"]]
    ],
    [
      [SBProperties.albumName, ["Back in Black"]]
    ]
  ]);
  view.filterConstraint = filter;
  assertEqual(view.length, 10);

  filter = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC", "Accept", "a-ha"]]
    ],
    [
      [SBProperties.albumName, ["Back in Black", "Restless and Wild/Balls to the Wall"]]
    ]
  ]);
  view.filterConstraint = filter;
  assertEqual(view.length, 25);

  view.filterConstraint = null;
  assertEqual(view.length, list.length);

  filter = LibraryUtils.createConstraint([
    [
      [SBProperties.GUID, ["3E3A8948-AD99-11DB-9321-C22AB7121F49"]]
    ]
  ]);
  view.filterConstraint = filter;
  assertEqual(view.length, 1);
  assertEqual(view.getItemByIndex(0).guid, "3E3A8948-AD99-11DB-9321-C22AB7121F49");

  // Test with simple media list
  list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  view = list.createView();

  filter = LibraryUtils.createConstraint([
    [
      [SBProperties.GUID, ["3E3A8948-AD99-11DB-9321-C22AB7121F49"]]
    ]
  ]);
  view.filterConstraint = filter;
  assertEqual(view.length, 1);
  assertEqual(view.getItemByIndex(0).guid, "3E3A8948-AD99-11DB-9321-C22AB7121F49");

  filter = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["AC/DC"]]
    ]
  ]);
  view.filterConstraint = filter;
  assertEqual(view.length, 10);
}

