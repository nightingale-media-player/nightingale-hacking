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

function countItems(enumerator) {
  var count = 0;
  while (enumerator.hasMoreElements()) {
    var item = enumerator.getNext().QueryInterface(Ci.sbIMediaItem);
    assertNotEqual(item, null);
    count++;
  }
  return count;
}

function runTest () {

  var databaseGUID = "test_localdatabaselibrary";
  createDatabase(databaseGUID);

  var library = createLibrary(databaseGUID);

  var view = library.getMediaItem("songbird:view");
  assertList(view, "data_sort_created_asc.txt");

  var item = view.getItemByIndex(0);
  assertEqual(item.guid, "3E2549C0-AD99-11DB-9321-C22AB7121F49");

  var contains = view.contains(view);
  assertEqual(contains, true);
  // TODO: test when this method fails, but how can i generate a media item
  // that is not in the view media list?

  var titleProperty = "http://songbirdnest.com/data/1.0#trackName";
  var albumProperty = "http://songbirdnest.com/data/1.0#albumName";
  var genreProperty = "http://songbirdnest.com/data/1.0#genre";
  
  var filteredListEnumerator =
    view.getItemsByPropertyValue(titleProperty, "Train of Thought");
  
  assertEqual(countItems(filteredListEnumerator), 1);
  
  filteredListEnumerator =
    view.getItemsByPropertyValue(albumProperty, "Back in Black");

  assertEqual(countItems(filteredListEnumerator), 10);
  
  filteredListEnumerator =
    view.getItemsByPropertyValue(genreProperty, "KJaskjjbfjJDBs");
    
  assertEqual(countItems(filteredListEnumerator), 0);

  var propertyArray =
    Cc["@songbirdnest.com/Songbird/Properties/PropertyArray;1"].
    createInstance(Ci.sbIPropertyArray);
  
  propertyArray.appendProperty(albumProperty, "Back in Black");
  filteredListEnumerator =
    view.getItemsByPropertyValues(propertyArray);
    
  assertEqual(countItems(filteredListEnumerator), 10);
  
  propertyArray.appendProperty(titleProperty, "Rock and Roll Ain't Noise Pollution");
  propertyArray.appendProperty(titleProperty, "Shake a Leg");
  filteredListEnumerator =
    view.getItemsByPropertyValues(propertyArray);

  assertEqual(countItems(filteredListEnumerator), 2);
  
  propertyArray.removeElementAt(1);
  filteredListEnumerator =
    view.getItemsByPropertyValues(propertyArray);
  
  assertEqual(countItems(filteredListEnumerator), 1);
}

