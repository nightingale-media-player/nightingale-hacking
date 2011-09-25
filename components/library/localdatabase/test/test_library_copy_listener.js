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

var SB_NS = "http://getnightingale.com/data/1.0#";

function runTest() {

  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  var item1URI = ios.newURI("file:///alpha.mp3", null, null);
  var item1Properties = 
    { artistName: "A", albumName: "Alpha", trackName: "Track Alpha", trackNumber: "1", year: "2000"};

  var item2URI = ios.newURI("file:///ecto.mp3", null, null);
  var item2Properties = 
    { artistName: "E", albumName: "Ecto", trackName: "Track Ecto", trackNumber: "2", year: "2004"};

  var item3URI = ios.newURI("file:///beta.mp3", null, null);
  var item3Properties = 
    { artistName: "B", albumName: "Beta", trackName: "Track Beta", trackNumber: "2", year: "2001"};

  var sourceDBGUID = "test_library_copy_source";
  var destinationDBGUID = "test_library_copy_destination";
  
  var sourceLibrary = createLibrary(sourceDBGUID, null, false);
  var destinationLibrary = createLibrary(destinationDBGUID, null, false);

  var itemProperties = createPropertyArray();
  appendPropertiesToArray(item1Properties, itemProperties);
  var item1 = sourceLibrary.createMediaItem(item1URI, 
                                            itemProperties);
  
  itemProperties = createPropertyArray();
  appendPropertiesToArray(item2Properties, itemProperties);
  var item2 = sourceLibrary.createMediaItem(item2URI, 
                                            itemProperties);

  itemProperties = createPropertyArray();
  appendPropertiesToArray(item3Properties, itemProperties);
  var item3 = sourceLibrary.createMediaItem(item3URI, 
                                            itemProperties);

  var sourceCopyListener = {
    current: 0,
    items: [item1, item2, item3, item1, item2, item3],
    onItemCopied: function _onItemCopied(aSourceItem, aDestinationItem) {
      dump(aSourceItem + " should be equal to " + this.items[this.current] + "\n\n");
      assertEqual(aSourceItem, this.items[this.current]);
      assertNotEqual(aSourceItem, aDestinationItem);
      this.current++;
    }
  };

  var localDatabaseLibrary = 
    sourceLibrary.QueryInterface(Ci.sbILocalDatabaseLibrary);
  localDatabaseLibrary.addCopyListener(sourceCopyListener);

  destinationLibrary.add(item1);
  destinationLibrary.add(item2);
  destinationLibrary.add(item3);
  
  destinationLibrary.clear();
  
  localDatabaseLibrary.removeCopyListener(sourceCopyListener);
  
  var itemsArray = Cc["@getnightingale.com/moz/xpcom/threadsafe-array;1"]
                     .createInstance(Ci.nsIMutableArray);
  
  itemsArray.appendElement(item1, false);
  itemsArray.appendElement(item2, false);
  itemsArray.appendElement(item3, false);

  localDatabaseLibrary.addCopyListener(sourceCopyListener);
  
  destinationLibrary.addSome(itemsArray.enumerate());
  
  localDatabaseLibrary.removeCopyListener(sourceCopyListener);
  
  destinationLibrary.clear();

  return;
}

function createPropertyArray() {
  return Cc["@getnightingale.com/Nightingale/Properties/MutablePropertyArray;1"]
           .createInstance(Ci.sbIMutablePropertyArray);
}

function appendPropertiesToArray(aProperties, aPropertyArray) {
  for (var propName in aProperties) {
    aPropertyArray.appendProperty(SB_NS + propName, aProperties[propName]);
  }
  return;
}
