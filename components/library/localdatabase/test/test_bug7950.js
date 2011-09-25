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

  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
  Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

  // Recreate the circumstanced of bug 7950, which is basically filtering on a
  // top level property (content type in this case) with a multi-level sort.
  // Under these conditions, simply looping through the guid array throws
  // an NS_ERROR_ILLEGAL_VALUE exception

  var library = createLibrary("test_bug7950");

  var array = Cc["@getnightingale.com/Nightingale/Library/LocalDatabase/GUIDArray;1"]
                .createInstance(Ci.sbILocalDatabaseGUIDArray);
  array.databaseGUID = "test_bug7950";
  array.baseTable = "media_items";
  array.propertyCache =
    library.QueryInterface(Ci.sbILocalDatabaseLibrary).propertyCache;
  array.fetchSize = 3;

  // Need a few different values for the top level property
  for (var i = 0; i < library.length; i++) {
    var item = library.getItemByIndex(i);
    item.contentType = item.contentType + (i % 3);
  }

  array.addFilter(SBProperties.contentType,
                  new StringArrayEnumerator(["audio"]),
                  false);

  array.addSort(SBProperties.artistName, true);
  array.addSort(SBProperties.albumName, true);
  array.addSort(SBProperties.trackNumber, true);

  for (var i = 0; i < array.length; i++) {
    var item = library.getItemByGuid(array.getGuidByIndex(i));
  }
}
