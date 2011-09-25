/*
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
 POTI, Inc.
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
, USA.
//
// END NIGHTINGALE GPL
//
*/

/**
 * \brief Test file
 */

function runTest () {
  runPerfTest("propertycache full get", perfTest);
  runPerfTest("propertycache chunked get", perfTest2);
}

function perfTest(library, timer) {
  
  // Get all the guids
  var guidList = [];
  library.enumerateAllItems({
      onEnumerationBegin: function(list) {
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumeratedItem: function(list, item) {
        guidList.push(item.guid);
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumerationEnd: function(list, status) {
      }
    });
  
  var cache = library.QueryInterface(Ci.sbILocalDatabaseLibrary).propertyCache;
  
  // Record time to cache all properties
  timer.start();
  
  var bagCount = {};
  var bags = cache.getProperties(guidList, guidList.length, bagCount);

  timer.stop();
}

function perfTest2(library, timer) {
  
  // Get all the guids
  var guidList = [];
  library.enumerateAllItems({
      onEnumerationBegin: function(list) {
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumeratedItem: function(list, item) {
        guidList.push(item.guid);
        return Ci.sbIMediaListEnumerationListener.CONTINUE;
      },
      onEnumerationEnd: function(list, status) {
      }
    });
  
  var cache = library.QueryInterface(Ci.sbILocalDatabaseLibrary).propertyCache;
  
  // Record time to cache all properties
  timer.start();
  
  for (var i = 0; i < (guidList.length - (guidList.length%200)); i += 200) {
    var bagCount = {};
    var bags = cache.getProperties(guidList.slice(i, i+200), 200, bagCount);
  }
  
  timer.stop();
}
