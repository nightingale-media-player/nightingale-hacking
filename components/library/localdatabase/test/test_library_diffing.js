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

var SB_NS = "http://songbirdnest.com/data/1.0#";

function runTest () {

  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  var sourceItemAddedURI = ios.newURI("file:///alpha.mp3", null, null);
  var sourceItemAddedProperties = 
    { artistName: "A", albumName: "Alpha", trackName: "Track Alpha", trackNumber: "1", year: "2000"};

  var sourceItemNotModifiedURI = ios.newURI("file:///ecto.mp3", null, null);
  var sourceItemNotModifiedProperties = 
    { artistName: "E", albumName: "Ecto", trackName: "Track Ecto", trackNumber: "2", year: "2004"};

  var destinationItemDeletedURI = ios.newURI("file:///beta.mp3", null, null);
  var destinationItemDeletedProperties = 
    { artistName: "B", albumName: "Beta", trackName: "Track Beta", trackNumber: "2", year: "2001"};
  
  var sourceItemModifiedURI = ios.newURI("file:///cent.mp3", null, null);
  var sourceItemModifiedProperties = 
    { artistName: "C", albumName: "Cent", trackName: "Track Cent", trackNumber: "3", year: "2002", composerName: "Centos" };

  var destinationItemModifiedURI = sourceItemModifiedURI;
  var destinationItemModifiedProperties = 
    { artistName: "D", albumName: "Delta", trackName: "Track Delta", trackNumber: "4", year: "2002", genre: "rock" };

  var destinationItemNotModifiedURI = ios.newURI("file:///ecto.mp3", null, null);
  var destinationItemNotModifiedProperties = 
    { artistName: "E", albumName: "Ecto", trackName: "Track Ecto", trackNumber: "2", year: "2004"};
  
  var diffingService = Cc["@songbirdnest.com/Songbird/Library/DiffingService;1"]
                        .getService(Ci.sbILibraryDiffingService);

  var sourceDBGUID = "test_library_source";
  var destinationDBGUID = "test_library_destination";
  
  var sourceLibrary = createLibrary(sourceDBGUID, null, false);
  var destinationLibrary = createLibrary(destinationDBGUID, null, false);

  var itemProperties = createPropertyArray();
  appendPropertiesToArray(sourceItemAddedProperties, itemProperties);
  var addedItem = sourceLibrary.createMediaItem(sourceItemAddedURI, 
                                                itemProperties);
  
  itemProperties = createPropertyArray();
  appendPropertiesToArray(destinationItemDeletedProperties, itemProperties);
  var deletedItemA = destinationLibrary.createMediaItem(destinationItemDeletedURI, 
                                                        itemProperties);

  itemProperties = createPropertyArray();
  appendPropertiesToArray(sourceItemModifiedProperties, itemProperties);
  var modifiedItem = sourceLibrary.createMediaItem(sourceItemModifiedURI, 
                                                   itemProperties);
  
  itemProperties = createPropertyArray();
  appendPropertiesToArray(destinationItemModifiedProperties, itemProperties);
  var deletedItemB = destinationLibrary.createMediaItem(destinationItemModifiedURI, 
                                                        itemProperties);
  
  itemProperties = createPropertyArray();
  appendPropertiesToArray(sourceItemNotModifiedProperties, itemProperties);
  sourceLibrary.createMediaItem(sourceItemNotModifiedURI,
                                itemProperties);
                                
  itemProperties = createPropertyArray();
  appendPropertiesToArray(destinationItemNotModifiedProperties, itemProperties);
  destinationLibrary.createMediaItem(destinationItemNotModifiedURI,
                                     itemProperties);
  
  var libraryChangeset = diffingService.createChangeset(sourceLibrary, 
                                                        destinationLibrary);
  
  var sourceLists = libraryChangeset.sourceLists;
  assertEqual(sourceLists.length, 1, "There should only be one source list!");
  
  var sourceList = sourceLists.queryElementAt(0, Ci.sbILibrary);
  assertEqual(sourceList, 
              sourceLibrary, 
              "Source list should be equal to the source library!");
  
  var destinationList = libraryChangeset.destinationList;
  assertEqual(destinationList.QueryInterface(Ci.sbILibrary), 
              destinationLibrary, 
              "Destination list should be equal to destination library!");
  
  var changes = libraryChangeset.changes;
  log("There are " + changes.length + " changes present in the changeset.");

  var addedOps = 0;
  var addedChange = null;
  
  var deletedOps = 0;
  var deletedChangeA = null;
  var deletedChangeB = null;
  
  var modifiedOps = 0;
  var modifiedChange = null;
  
  var unknownOps = 0;
  
  var changesEnum = changes.enumerate();
  
  while(changesEnum.hasMoreElements()) {
    var change = changesEnum.getNext().QueryInterface(Ci.sbILibraryChange);

    switch(change.operation) {
      case Ci.sbIChangeOperation.ADDED: 
        addedOps++; 
        addedChange = change; 
      break;
      case Ci.sbIChangeOperation.DELETED: 
        deletedOps++; 
        deletedOps == 1 ? deletedChangeA = change : deletedChangeB = change; 
      break;
      case Ci.sbIChangeOperation.MODIFIED: 
        modifiedOps++; 
        modifiedChange = change; 
      break;
      default: 
        unknownOps++;
    }
  }
  
  assertEqual(addedOps, 1, "Wrong number of added operation!");
  assertEqual(deletedOps, 2, "Wrong number of deleted operations!");
  assertEqual(modifiedOps, 1, "Wrong number of modified operation!");
  assertEqual(unknownOps, 0, "There should _not_ be any unknown operations!");
  
  var propertyChange = null;
  var propertyChangesEnum = addedChange.properties.enumerate();
  while(propertyChangesEnum.hasMoreElements()) {
    propertyChange = propertyChangesEnum.getNext().QueryInterface(Ci.sbIPropertyChange);
    if(typeof(sourceItemAddedProperties[propertyChange.id]) != 'undefined') {
      assertEqual(sourceItemAddedProperties[propertyChange.id], 
                  propertyChange.newValue, 
                  "Value is not what is expected!");
    }
  }

  try {
    deletedChangeA.properties;
  }
  catch(e) {
    assertEqual(e.result, 
                Cr.NS_ERROR_NOT_AVAILABLE, 
                "There should be no property changes available for deleted operations!");
  }

  try {
    deletedChangeB.properties;
  }
  catch(e) {
    assertEqual(e.result, 
                Cr.NS_ERROR_NOT_AVAILABLE, 
                "There should be no property changes available for deleted operations!");
  }
  
  propertyChangesEnum = modifiedChange.properties.enumerate();
  while(propertyChangesEnum.hasMoreElements()) {
    propertyChange = propertyChangesEnum.getNext().QueryInterface(Ci.sbIPropertyChange);
    // According to this, the property should've been added.    
    if(typeof(sourceItemModifiedProperties[propertyChange.id]) != 'undefined' &&
       typeof(destinationItemModifiedProperties[propertyChange.id]) == 'undefined') {
      assertEqual(propertyChange.operation,
                  Ci.sbIChangeOperation.ADDED,
                  "Property should have been ADDED!");
      assertEqual(propertyChange.oldValue,
                  'undefined',
                  "Old property value should be undefined!");
    }
    // According to this, the property should've been modified.
    if(typeof(sourceItemModifiedProperties[propertyChange.id]) != 'undefined' &&
       typeof(destinationItemModifiedProperties[propertyChange.id]) != 'undefined') {
      assertEqual(propertyChange.oldValue,
                  destinationItemModifiedProperties[propertyChange.id],
                  "Old value and actual item value do not match when they should!");
      assertEqual(propertyChange.operation, 
                  Ci.sbIChangeOperation.MODIFIED,
                  "Property should have been MODIFIED!");
    }
    
    if(typeof(sourceItemModifiedProperties[propertyChange.id]) == 'undefined' &&
       typeof(destinationItemModifiedProperties[propertyChange.id]) != 'undefined') {
      assertEqual(propertyChange.operation,
                  Ci.sbIChangeOperation.DELETED,
                  "Property should have been DELETED!");
      assertEqual(destinationItemModifiedProperties[propertyChange.id],
                  propertyChange.oldValue);
      assertEqual(propertyChange.newValue,
                  'undefined',
                  "Deleted property should _not_ have a newValue");
    }
    
  }
}

function createPropertyArray() {
  return Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
           .createInstance(Ci.sbIMutablePropertyArray);
}

function appendPropertiesToArray(aProperties, aPropertyArray) {
  for (var propName in aProperties) {
    aPropertyArray.appendProperty(SB_NS + propName, aProperties[propName]);
  }
  return;
}