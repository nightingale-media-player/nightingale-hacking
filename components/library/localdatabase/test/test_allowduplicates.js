/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
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

function runTest() {

  var databaseGUID = "test_allowduplicates";
  var library = createLibrary(databaseGUID, null, false);

  var uri = newURI("http://example.com/foo.mp3");

  // Default behavior is to not allow dups
  var item1 = library.createMediaItem(uri);
  var item2 = library.createMediaItem(uri);

  assertTrue(item1.equals(item2));

  var item3 = library.createMediaItem(uri, null, true);
  assertFalse(item1.equals(item3));

  library.clear();

  var toAdd = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
  toAdd.appendElement(newURI("http://example.com/foo.mp3"), false);
  toAdd.appendElement(newURI("http://example.com/foo.mp3"), false);
  toAdd.appendElement(newURI("http://example.com/foo1.mp3"), false);
  toAdd.appendElement(newURI("http://example.com/foo1.mp3"), false);

  var added = library.batchCreateMediaItems(toAdd);
  assertEqual(added.length, 2);
  assertEqual(library.length, 2);

  var added = library.batchCreateMediaItems(toAdd);
  assertEqual(added.length, 0);
  assertEqual(library.length, 2);

  var added = library.batchCreateMediaItems(toAdd, null, true);
  assertEqual(added.length, 4);
  assertEqual(library.length, 6);

  library.clear();

  var listener = {
    arrayLength: 0,
    libraryLength: 0,
    that: this,
    onProgress: function(index) {
    },
    onComplete: function(array) {
      assertEqual(array.length, this.arrayLength);
      assertEqual(library.length, this.libraryLength);
      this.that.testFinished();
      test = null;
    }
  };

  // 2 tracks added, library will have 2 items, no duplicates
  listener.arrayLength = 2;
  listener.libraryLength = 2;
  library.batchCreateMediaItemsAsync(listener, toAdd);
  testPending();

  // 0 tracks added, library will have 2 items, no duplicates
  listener.arrayLength = 0;
  listener.libraryLength = 2;
  library.batchCreateMediaItemsAsync(listener, toAdd);
  testPending();

  // 0 tracks added, library will have 2 items, duplicates allowed
  listener.arrayLength = 4;
  listener.libraryLength = 6;
  library.batchCreateMediaItemsAsync(listener, toAdd, null, true);
  testPending();

}

