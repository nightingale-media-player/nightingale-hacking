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

  var databaseGUID = "test_localdatabaselibrary";
  var library = createLibrary(databaseGUID);

  var cache = library.QueryInterface(Ci.sbILocalDatabaseLibrary).propertyCache;
  
  var db = loadMockDatabase();

  // Request guids in 20 guid chunks
  var guidList = [];
  for(var guid in db) {
    guidList.push(guid);
    if(guidList.length > 20) {
      var bagCount = {};
      var bags = cache.getProperties(guidList, guidList.length, bagCount);
      assertEqual(bagCount.value, guidList.length);

      for(var i = 0; i < bagCount.value; i++) {
        var e = bags[i].names;
        while(e.hasMore()) {
          var p = e.getNext();
          var guid = guidList[i];
          assertEqual(db[guid][p], bags[i].getProperty(p), p + " " + guid);
        }
      }
      guidList = [];
    }
  }

  // Request the whole db one guid at a time
  for(var guid in db) {
    var item = db[guid];

    var bagCount = {};
    var bags = cache.getProperties([guid], 1, bagCount);
    assertEqual(bagCount.value, 1);

    var e = bags[0].names;
    while(e.hasMore()) {
      var p = e.getNext();
      assertEqual(item[p], bags[0].getProperty(p), p + " " + guid);
    }
  }

}
