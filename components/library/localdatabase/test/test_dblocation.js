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

  const PERMISSIONS_FILE      = 0644;
  const PERMISSIONS_DIRECTORY = 0755;

  const dbGUID = "9c1ecf14-667b-47ce-b533-2dfd242f3ede";

  var directory = Cc["@mozilla.org/file/directory_service;1"].
                  getService(Ci.nsIProperties).
                  get("ProfD", Ci.nsIFile);
  directory.append("db_tests");

  if (!directory.exists()) {
    directory.create(Ci.nsIFile.DIRECTORY_TYPE, PERMISSIONS_DIRECTORY);
  }
  
  var file = directory.clone();
  file.append(dbGUID + ".db");

  if (file.exists()) {
    file.remove(false);
  }

  var libraryFactory =
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"].
    getService(Ci.sbILibraryFactory);

  var hashBag = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag2);
  hashBag.setPropertyAsInterface("databaseFile", file);

  var library = libraryFactory.createLibrary(hashBag);

  var location = newFileURI(directory);
  loadData(dbGUID, location);
  
  var testURI = newURI("file:///testitem");
  var item1 = library.createMediaItem(testURI);
  var item2 = library.createMediaItem(testURI);
  assertNotEqual(item1.guid, item2.guid);

  // Test that they items were added to the library
  assertEqual(library.getItemByGuid(item1.guid).guid, item1.guid);
  assertEqual(library.getItemByGuid(item2.guid).guid, item2.guid);
}
