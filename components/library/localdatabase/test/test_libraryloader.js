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
  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"].
                       getService(Ci.sbILibraryManager);
  var enumerator = libraryManager.getLibraryEnumerator();
  
  var libraryCount = 0;
  while (enumerator.hasMoreElements()) {
    var library = enumerator.getNext();
    libraryCount++;
  }
  assertTrue(libraryCount >= 3);
  
  enumerator = libraryManager.getLibraryEnumerator();
  var library = enumerator.getNext();
  assertTrue(library);  
}
