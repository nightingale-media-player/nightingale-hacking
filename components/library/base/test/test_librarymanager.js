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

function countEnumeratedItems(enumerator) {
  var count = 0;
  
  while(enumerator.hasMoreElements()) {
    var item = enumerator.getNext();
    count++;
  }
  
  return count;
}

function runTest () {
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"].
                       getService(Ci.sbILibraryManager);
  var enumerator = libraryManager.getLibraries();
  
  var baseLibraryCount = countEnumeratedItems(enumerator);

  var library1 = createLibrary("test_library-base1");
  
  libraryManager.registerLibrary(library1, false);
  enumerator = libraryManager.getLibraries();
  assertEqual(countEnumeratedItems(enumerator), baseLibraryCount + 1);
  
  libraryManager.unregisterLibrary(library1);
  enumerator = libraryManager.getLibraries();
  assertEqual(countEnumeratedItems(enumerator), baseLibraryCount);

  libraryManager.registerLibrary(library1, false);
  libraryManager.registerLibrary(library1, false);
  enumerator = libraryManager.getLibraries();
  assertEqual(countEnumeratedItems(enumerator), baseLibraryCount + 1);

  libraryManager.unregisterLibrary(library1);
  libraryManager.unregisterLibrary(library1);
  enumerator = libraryManager.getLibraries();
  assertEqual(countEnumeratedItems(enumerator), baseLibraryCount);

  var library2 = createLibrary("test_library-base2");

  libraryManager.registerLibrary(library1, false);
  libraryManager.registerLibrary(library2, false);
  enumerator = libraryManager.getLibraries();
  assertEqual(countEnumeratedItems(enumerator), baseLibraryCount + 2);
  
  var getLibraryException;
  try {
    var testLibrary = libraryManager.getLibrary("whippersnapper");
  } catch (e) {
    getLibraryException = e;
  }
  assertEqual(getLibraryException.result, Cr.NS_ERROR_NOT_AVAILABLE);
  
  var testLibrary = libraryManager.getLibrary(library1.guid);
  assertEqual(testLibrary, library1);
  
  var mainLibrary = libraryManager.mainLibrary;
  assertTrue(mainLibrary);
  
  enumerator = libraryManager.getStartupLibraries();
  var baseStartupLibraryCount = countEnumeratedItems(enumerator);
  assertTrue(baseStartupLibraryCount >= 3);

  var library3 = createLibrary("test_library-base3");
  libraryManager.registerLibrary(library3, true);
  enumerator = libraryManager.getStartupLibraries();
  assertEqual(countEnumeratedItems(enumerator), baseStartupLibraryCount + 1);

  libraryManager.setLibraryLoadsAtStartup(library3, false);
  enumerator = libraryManager.getStartupLibraries();
  assertEqual(countEnumeratedItems(enumerator), baseStartupLibraryCount);
  
  libraryManager.setLibraryLoadsAtStartup(library2, true);
  assertTrue(libraryManager.getLibraryLoadsAtStartup(library2));
  
  enumerator = libraryManager.getStartupLibraries();
  assertEqual(countEnumeratedItems(enumerator), baseStartupLibraryCount + 1);

  libraryManager.setLibraryLoadsAtStartup(library2, false);
  assertFalse(libraryManager.getLibraryLoadsAtStartup(library2));
  
  enumerator = libraryManager.getStartupLibraries();
  assertEqual(countEnumeratedItems(enumerator), baseStartupLibraryCount);
}
