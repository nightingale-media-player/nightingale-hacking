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

  var library1 = createLibrary("test_library-base1");
  
  libraryManager.registerLibrary(library1);
  var enumerator = libraryManager.getLibraryEnumerator();
  assertEqual(countEnumeratedItems(enumerator), 1);
  
  libraryManager.unregisterLibrary(library1);
  enumerator = libraryManager.getLibraryEnumerator();
  assertEqual(countEnumeratedItems(enumerator), 0);

  libraryManager.registerLibrary(library1);
  libraryManager.registerLibrary(library1);
  enumerator = libraryManager.getLibraryEnumerator();
  assertEqual(countEnumeratedItems(enumerator), 1);

  libraryManager.unregisterLibrary(library1);
  libraryManager.unregisterLibrary(library1);
  enumerator = libraryManager.getLibraryEnumerator();
  assertEqual(countEnumeratedItems(enumerator), 0);

  var library2 = createLibrary("test_library-base2");

  libraryManager.registerLibrary(library1);
  libraryManager.registerLibrary(library2);
  enumerator = libraryManager.getLibraryEnumerator();
  assertEqual(countEnumeratedItems(enumerator), 2);
  
  var getLibraryException;
  try {
    var testLibrary = libraryManager.getLibrary("whippersnapper");
  } catch (e) {
    getLibraryException = e;
  }
  assertEqual(getLibraryException.result, Cr.NS_ERROR_NOT_AVAILABLE);
  
  var testLibrary = libraryManager.getLibrary(library1.guid);
  assertEqual(testLibrary, library1);
}
