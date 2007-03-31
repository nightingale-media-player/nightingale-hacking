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

  var library = createLibrary("test_cascademediaset");
  var list = library.QueryInterface(Ci.sbIMediaList)
  var view = list.createView();
  var cfs = view.cascadeFilterSet;

  // A simulation of an everything search, artist, album cascase
  cfs.appendSearch(["*"], 1);

  cfs.appendFilter("http://songbirdnest.com/data/1.0#artistName", false);

  cfs.appendFilter("http://songbirdnest.com/data/1.0#albumName", false);

  // With no filtering applied, we should have 8 artists and 8 albums
  assertEqual(cfs.getLength(1), 8);
  assertEqual(cfs.getLength(2), 8);

  // Putting AC/DC in the search box should filter down to only one artist
  // and one album
  cfs.set(0, ["AC/DC"], 1);
  assertEqual(cfs.getLength(1), 1);
  assertEqual(cfs.getLength(2), 1);

  // Removing the search filter should return everyting to the others
  cfs.set(0, [], 0);
  assertEqual(cfs.getLength(1), 8);
  assertEqual(cfs.getLength(2), 8);

  // Filter the artist on a-ha should keep only one album
  cfs.set(1, ["a-ha"], 1);
  assertEqual(cfs.getLength(1), 8);
  assertEqual(cfs.getLength(2), 1);

  // Add AC/DC to the artist filter and we should have two albums
  cfs.set(1, ["a-ha", "AC/DC"], 2);
  assertEqual(cfs.getLength(1), 8);
  assertEqual(cfs.getLength(2), 2);

  // Setting an additional filter on album should not affect anything
  cfs.set(2, ["Back In Black"], 1);
  assertEqual(cfs.getLength(1), 8);
  assertEqual(cfs.getLength(2), 2);

  // Add gibberish to the search filter should make everything empty
  cfs.set(0, ["dfkjhdfds"], 1);
  assertEqual(cfs.getLength(1), 0);
  assertEqual(cfs.getLength(2), 0);

  // Removing the search filter should restore us to the previous state
  cfs.remove(0);
  assertEqual(cfs.getLength(0), 8);
  assertEqual(cfs.getLength(1), 2);
}

