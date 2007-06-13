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

  var library = createLibrary("test_localdatabaselibrary");

  // Tests for getUnfilteredIndex

  // For the library, the unfiltered index on a view should match the index
  // of the item in the library
  var list = library;
  var view = list.createView();

  var pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "AC/DC");
  view.setFilters(pa);

  for (var i = 0; i < view.length; i++) {
    var viewItem = view.getItemByIndex(i);
    var listIndex = view.getUnfilteredIndex(i);
    assertEqual(listIndex, list.indexOf(viewItem, 0));
  }

  // For a simple media list, set up a small media list, filter it and check
  // the unfiltered index
  var item1 = library.createMediaItem(newURI("file:///foo/1.mp3"));
  item1.setProperty("http://songbirdnest.com/data/1.0#artistName", "The Fall");

  var item2 = library.createMediaItem(newURI("file:///foo/1.mp3"));
  item2.setProperty("http://songbirdnest.com/data/1.0#artistName", "The Dirtbombs");

  var item3 = library.createMediaItem(newURI("file:///foo/1.mp3"));
  item3.setProperty("http://songbirdnest.com/data/1.0#artistName", "The Dirtbombs");

  var item4 = library.createMediaItem(newURI("file:///foo/1.mp3"));
  item4.setProperty("http://songbirdnest.com/data/1.0#artistName", "Air");

  list = library.createMediaList("simple");
  list.add(item1);
  list.add(item2);
  list.add(item3);
  list.add(item4);

  view = list.createView();
  for (var i = 0; i < view.length; i++) {
    var viewItem = view.getItemByIndex(i);
    var listIndex = view.getUnfilteredIndex(i);
    assertEqual(listIndex, list.indexOf(viewItem, 0));
  }

  var pa = createPropertyArray();
  pa.appendProperty("http://songbirdnest.com/data/1.0#artistName", "The Dirtbombs");
  view.setFilters(pa);

  for (var i = 0; i < view.length; i++) {
    var viewItem = view.getItemByIndex(i);
    var listIndex = view.getUnfilteredIndex(i);
    assertEqual(listIndex, list.indexOf(viewItem, 0));
  }

}

function createPropertyArray() {
  return Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
           .createInstance(Ci.sbIMutablePropertyArray);
}

