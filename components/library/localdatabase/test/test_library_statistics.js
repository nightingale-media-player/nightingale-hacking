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
  // import helpers
  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

  // create a library for the test
  var databaseGUID = "test_library_statistics";
  var library = createLibrary(databaseGUID);

  // create a bunch of items
  var items = [
    { artistName: 'A', rating: 1 },
    { artistName: 'A', rating: 2 },
    { artistName: 'A', rating: 3 },
    { artistName: 'A', rating: 4 },
    { artistName: 'A', rating: 5 },
    { artistName: 'A', rating: 1 },
    { artistName: 'A', rating: 1 },
    { artistName: 'A', rating: 1 },
    { artistName: 'A', rating: 1 },
    { artistName: 'A', rating: 2 },
    { artistName: 'B', rating: 3 },
    { artistName: 'B', rating: 2 },
    { artistName: 'B', rating: 2 },
    { artistName: 'C', rating: 4 },
    { artistName: 'C', rating: 4 },
    { artistName: 'C', rating: 5 },
    { artistName: 'C', rating: 1 },
    { artistName: 'C', rating: 2 },
    { artistName: 'D', rating: 3 },
    { artistName: 'E', rating: 2 },
  ];

  for (var i in items) {
    var props = items[i];
    var item = library.createMediaItem(newURI('about:test/'+i));
    for (var p in props) {
      item.setProperty(SBProperties[p], props[p]);
    }
  }

  // get the library stats interface
  var sbILibraryStatistics = Components.interfaces.sbILibraryStatistics;

  // helper to convert an nsIArray of nsIVariants to a JS array
  // ArrayConverter.JSArray is too generic for this case :(
  function array2array(a) {
    var a2 = [];
    for (var i=0; i<a.length; i++) {
      a2.push(a.queryElementAt(i, Components.interfaces.nsIVariant));
    }
    return a2;
  }

  // test SUM in ascending order, high limit
  assertArraysEqual(array2array(
        library.collectDistinctValues(SBProperties.artistName,
          sbILibraryStatistics.COLLECT_SUM, SBProperties.rating, true, 100)), 
      ["D", "B", "C", "A"])

  // test SUM in descending order, high limit
  assertArraysEqual(array2array(
        library.collectDistinctValues(SBProperties.artistName, 
          sbILibraryStatistics.COLLECT_SUM, SBProperties.rating, false, 100)), 
      ["A", "C", "B", "D"])

  // test SUM in descending order, low limit
  assertArraysEqual(array2array(
        library.collectDistinctValues(SBProperties.artistName, 
          sbILibraryStatistics.COLLECT_SUM, SBProperties.rating, false, 2)), 
      ["A", "C"])
}
