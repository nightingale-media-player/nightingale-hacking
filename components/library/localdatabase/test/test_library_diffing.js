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
  testLibraryToLibraryDiffing();
  testMediaListToMediaListDiffing();
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

function testLibraryToLibraryDiffing() {
  log("Testing Library to Library diffing.");

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
  var sourceItemNotModified = 
    sourceLibrary.createMediaItem(sourceItemNotModifiedURI,
                                  itemProperties);
                                
  itemProperties = createPropertyArray();
  appendPropertiesToArray(destinationItemNotModifiedProperties, itemProperties);
  var destinationItemNotModified = 
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
  
  return;
}

function testMediaListToMediaListDiffing(aItems) {
  log("Testing MediaList to MediaList diffing.");
  
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  var itemAURI = ios.newURI("file:///alpha.mp3", null, null);
  var itemAProperties = 
    { artistName: "A", albumName: "Alpha", trackName: "Track Alpha", trackNumber: "1", year: "2000"};

  var itemBURI = ios.newURI("file:///ecto.mp3", null, null);
  var itemBProperties = 
    { artistName: "E", albumName: "Ecto", trackName: "Track Ecto", trackNumber: "2", year: "2004"};

  var itemCURI = ios.newURI("file:///beta.mp3", null, null);
  var itemCProperties = 
    { artistName: "B", albumName: "Beta", trackName: "Track Beta", trackNumber: "2", year: "2001"};
  
  var itemDURI = ios.newURI("file:///cent.mp3", null, null);
  var itemDProperties = 
    { artistName: "C", albumName: "Cent", trackName: "Track Cent", trackNumber: "3", year: "2002", composerName: "Centos" };

  var itemEURI = ios.newURI("file:///femto.mp3", null, null);
  var itemEProperties = 
    { artistName: "F", albumName: "Femto", trackName: "Track Femto", trackNumber: "7", year: "1998", genre: "electronic" };

  var itemEEURI = ios.newURI("file:///femto.mp3", null, null);
  var itemEEProperties = 
    { artistName: "Full", albumName: "Femto", trackName: "Track Femto Super", trackNumber: "7", year: "1998", genre: "techno" };

  var itemFURI = ios.newURI("file:///delta.mp3", null, null);
  var itemFProperties = 
    { artistName: "D", albumName: "Delta", trackName: "Track Delta", trackNumber: "5", year: "2008"};
  
 
  var diffingService = Cc["@songbirdnest.com/Songbird/Library/DiffingService;1"]
                        .getService(Ci.sbILibraryDiffingService);

  var sourceDBGUID = "test_medialist_source";
  var destinationDBGUID = "test_medialist_destination";
  
  var sourceLibrary = createLibrary(sourceDBGUID, null, false);
  var destinationLibrary = createLibrary(destinationDBGUID, null, false);

  var itemProperties = createPropertyArray();
  appendPropertiesToArray(itemAProperties, itemProperties);
  var itemA = sourceLibrary.createMediaItem(itemAURI, 
                                            itemProperties);
  
  itemProperties = createPropertyArray();
  appendPropertiesToArray(itemBProperties, itemProperties);
  var itemB = destinationLibrary.createMediaItem(itemBURI, 
                                                 itemProperties);

  itemProperties = createPropertyArray();
  appendPropertiesToArray(itemCProperties, itemProperties);
  var itemC = sourceLibrary.createMediaItem(itemCURI, 
                                            itemProperties);
  
  itemProperties = createPropertyArray();
  appendPropertiesToArray(itemDProperties, itemProperties);
  var itemD = destinationLibrary.createMediaItem(itemDURI, 
                                                 itemProperties);
  
  itemProperties = createPropertyArray();
  appendPropertiesToArray(itemEProperties, itemProperties);
  var itemE = sourceLibrary.createMediaItem(itemEURI,
                                            itemProperties);

  itemProperties = createPropertyArray();
  appendPropertiesToArray(itemEEProperties, itemProperties);
  var itemEE = destinationLibrary.createMediaItem(itemEEURI,
                                                  itemProperties);

  itemProperties = createPropertyArray();
  appendPropertiesToArray(itemFProperties, itemProperties);
  var itemF = destinationLibrary.createMediaItem(itemFURI,
                                                 itemProperties);

  var itemsAdded = [itemB, itemE];
  var itemsAddedProperties = [itemBProperties, itemEProperties];
  
  var itemsDeleted = [itemD, itemD];
  var itemsDeletedProperties = [itemDProperties, itemDProperties];

  var sourceMediaList = sourceLibrary.createMediaList("simple");
  var destinationMediaList = destinationLibrary.createMediaList("simple");
  
  sourceMediaList.name = "The List";
  destinationMediaList.name = "The Wrong Name";
  
  sourceMediaList.add(itemA);
  sourceMediaList.add(itemB);
  sourceMediaList.add(itemC);
  sourceMediaList.add(itemE);
  sourceMediaList.add(itemF);
  sourceMediaList.add(itemE);
  
  destinationMediaList.add(itemA);
  destinationMediaList.add(itemC);
  destinationMediaList.add(itemD);
  destinationMediaList.add(itemEE);
  destinationMediaList.add(itemF);
  destinationMediaList.add(itemD);

  var libraryChangeset = diffingService.createChangeset(sourceMediaList, 
                                                        destinationMediaList);

  var sourceLists = libraryChangeset.sourceLists;
  assertEqual(sourceLists.length, 1, "There should only be one source list!");
  
  var sourceList = sourceLists.queryElementAt(0, Ci.sbIMediaList);
  assertEqual(sourceList, 
              sourceMediaList, 
              "Source list should be equal to the source media list!");
  
  var destinationList = libraryChangeset.destinationList;
  assertEqual(destinationList.QueryInterface(Ci.sbIMediaList), 
              destinationMediaList, 
              "Destination list should be equal to destination media list!");
  
  var changes = libraryChangeset.changes;
  log("There are " + changes.length + " changes present in the changeset.");

  var addedOps = 0;
  var addedChanges = [];
  
  var deletedOps = 0;
  var deletedChanges = [];

  var movedOps = 0;
  var movedChanges = [];

  var modifiedOps = 0;
  var unknownOps = 0;

  var changesEnum = changes.enumerate();
  
  while(changesEnum.hasMoreElements()) {
    var change = changesEnum.getNext().QueryInterface(Ci.sbILibraryChange);

    switch(change.operation) {
      case Ci.sbIChangeOperation.ADDED: 
        addedOps++; 
        addedChanges.push(change);
      break;
      case Ci.sbIChangeOperation.DELETED: 
        deletedOps++; 
        deletedChanges.push(change);
      break;
      case Ci.sbIChangeOperation.MODIFIED: 
        modifiedOps++; 
      break;
      case Ci.sbIChangeOperation.MOVED:
        movedOps++;
        movedChanges.push(change);
      break;
      default: 
        unknownOps++;
    }
  }
  
  log("There are " + addedOps + " added operations.");
  assertEqual(addedOps, 2, "There should be 2 added operations");
  
  log("There are " + deletedOps + " deleted operations.");
  assertEqual(deletedOps, 2, "There should be 2 deleted operations");
  
  log("There are " + modifiedOps + " modified operations.");
  assertEqual(modifiedOps, 1, "There should 1 modified operations");

  log("There are " + movedOps + " moved operations.");
  assertEqual(movedOps, 6, "There should be 6 moved operations.");

  var i = 0;
  var propertyChange = null;
  var propertyChangesEnum = null;
  
  for(i = 0; i < addedChanges.length; ++i) {
    propertyChangesEnum = addedChanges[i].properties.enumerate();
    while(propertyChangesEnum.hasMoreElements()) {
      propertyChange = propertyChangesEnum.getNext().QueryInterface(Ci.sbIPropertyChange);
      if(typeof(itemsAddedProperties[i][propertyChange.id]) != 'undefined') {
        assertEqual(itemsAddedProperties[i][propertyChange.id], 
                    propertyChange.newValue, 
                    "Value is not what is expected!");
      }
    }
  }

  for(i = 0; i < deletedChanges.length; ++i) {
    assertEqual(deletedChanges[i].destinationItem,
                itemsDeleted[i],
                "Items should be equal!");
    try {
      propertyChangesEnum = deletedChanges[i].properties;
    }
    catch(e) {
      assertEqual(e.result, 
                  Cr.NS_ERROR_NOT_AVAILABLE, 
                  "There should be no property changes available for deleted operations!");
    }
  }
  
  for(i = 0; i < movedChanges.length; ++i) {

    propertyChangesEnum = movedChanges[i].properties.enumerate();

    var totalPropertyChanges = 0;
    while(propertyChangesEnum.hasMoreElements()) {
      propertyChange = propertyChangesEnum.getNext().QueryInterface(Ci.sbIPropertyChange);
      
      assertEqual(propertyChange.id, 
                  SB_NS + "ordinal", 
                  "The property id should always be 'http://songbirdnest.com/data/1.0#ordinal'");
                  
      assertEqual(propertyChange.newValue,
                  i,
                  "The ordinal of the item is not correct!");

      ++totalPropertyChanges;
    }
    
    assertEqual(totalPropertyChanges, 
                1, 
                "There should only be one property change for moved operations!");
  }

  return;
}
