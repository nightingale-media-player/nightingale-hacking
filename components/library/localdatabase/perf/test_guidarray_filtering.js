/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
//
*/

/**
 * \brief Test file
 */

function runTest () {
  runPerfTest("guidarray filtering", perfTest);
}

function perfTest(library, timer) {

  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

  // Approximate the cost of displaying the library and filter lists
  // with a match-all search and an artist filter

  var filterArray1 = newGuidArray(library);
  filterArray1.addSort(SBProperties.genre, true);
  filterArray1.addFilter(SBProperties.hidden, new StringArrayEnumerator(["0"]), false);
  filterArray1.addFilter(SBProperties.isList, new StringArrayEnumerator(["0"]), false);
  filterArray1.fetchSize = 0; // Fetch all
  filterArray1.isDistinct = true;  
  filterArray1.addFilter(SBProperties.artistName, new StringArrayEnumerator(["a*"]), true);  
  filterArray1.addFilter(SBProperties.genre, new StringArrayEnumerator(["rock","folk"]), false);

  var filterArray2 = filterArray1.clone();
  filterArray2.clearSorts();
  filterArray2.addSort(SBProperties.albumName, true);
  
  var filterArray3 = filterArray1.clone();
  filterArray3.clearSorts();
  filterArray3.addSort(SBProperties.artistName, true);

  var playlistArray = filterArray1.clone();
  playlistArray.clearSorts();
  playlistArray.addSort(SBProperties.artistName, true);
  playlistArray.fetchSize = 300; 
  playlistArray.isDistinct = false;  

  timer.start();

  for each (var array in [filterArray1, filterArray2, filterArray3, playlistArray]) {
    array.getGuidByIndex(0);
    array.getGuidByIndex(array.length / 2);
    array.getGuidByIndex(array.length - 1);    
  }

  timer.stop();

}
