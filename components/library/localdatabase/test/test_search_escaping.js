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

  var library = createLibrary("test_search_escaping");

  // Test with simple media list
  var list = library.getMediaItem("7e8dcc95-7a1d-4bb3-9b14-d4906a9952cb");
  var view = list.createView();

  var search = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["%"]]
    ]
  ]);
  view.searchConstraint = search;
  assertEqual(view.length, 0);

  view.searchConstraint = null;
  view.getItemByIndex(2)
      .setProperty("http://songbirdnest.com/data/1.0#artistName", "Some%thing");
  view.searchConstraint = search;
  assertEqual(view.length, 1);
  
 search = LibraryUtils.createConstraint([
    [
      [SBProperties.artistName, ["\\"]]
    ]
  ]);
  view.searchConstraint = search;
  assertEqual(view.length, 0);

  view.searchConstraint = null;
  view.getItemByIndex(0)
      .setProperty("http://songbirdnest.com/data/1.0#artistName", "Some\\thing");
  view.searchConstraint = search;
  assertEqual(view.length, 1);
}

