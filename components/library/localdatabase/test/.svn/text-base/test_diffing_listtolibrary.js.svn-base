/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */


/**
 * \brief Test file
 */

var SB_NS = "http://songbirdnest.com/data/1.0#";

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

function makeItACopy(source, destination)
{
  destination.setProperty(SBProperties.originItemGuid, source.guid);
  destination.setProperty(SBProperties.originLibraryGuid, source.library.guid);
}

function runTest () {
  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
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
  
  // Source items:
  //   Item A  (A, B)
  //   Item B  (A)
  //   Item C  (A, B)
  //   Item D  (B)
  //   Item E  (Ax2)
  //   Item EE (B) copy of E
  //
  // Destination Items:
  //   Item B
  //   Item D
  //   Item EE
  //   Item F
  
  var diffingService = Cc["@songbirdnest.com/Songbird/Library/DiffingService;1"]
                        .getService(Ci.sbILibraryDiffingService);

  var sourceDBGUID = "test_multilist_source";
  var destinationDBGUID = "test_multilist_destination";
  
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
  makeItACopy(itemE, itemEE);
  
  itemProperties = createPropertyArray();
  appendPropertiesToArray(itemFProperties, itemProperties);
  var itemF = destinationLibrary.createMediaItem(itemFURI,
                                                 itemProperties);

  
  // ItemB and ItemE added
  // ItemD deleted

  // Setup the first source list.
  var sourceMediaListA = sourceLibrary.createMediaList("simple");
  sourceMediaListA.name = "The List";
  sourceMediaListA.add(itemA);
  sourceMediaListA.add(itemB);
  sourceMediaListA.add(itemC);
  sourceMediaListA.add(itemE);
  sourceMediaListA.add(itemE);

  // Setup the second source list.
  var sourceMediaListB = sourceLibrary.createMediaList("simple");
  sourceMediaListB.name = "The Wrong Name";
  sourceMediaListB.add(itemA);
  sourceMediaListB.add(itemC);
  sourceMediaListB.add(itemD);
  sourceMediaListB.add(itemEE);
  sourceMediaListB.add(itemD);

  var sourceMediaLists = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                           .createInstance(Ci.nsIMutableArray);
  sourceMediaLists.appendElement(sourceMediaListA, false);
  sourceMediaLists.appendElement(sourceMediaListB, false);

  var libraryChangeset = diffingService.createMultiChangeset(sourceMediaLists, 
                                                             destinationLibrary);

  var sourceLists = libraryChangeset.sourceLists;
  assertEqual(sourceLists.length, 2, "There should only be two source lists!");
  
  var sourceList = sourceLists.queryElementAt(0, Ci.sbIMediaList);
  assertEqual(sourceList, 
              sourceMediaListA, 
              "Source list should be equal to the source media list!");

  sourceList = sourceLists.queryElementAt(1, Ci.sbIMediaList);
  assertEqual(sourceList,
              sourceMediaListB,
              "Source list should be equal to the source media list!");
  
  var destinationList = libraryChangeset.destinationList;
  assertEqual(destinationList.QueryInterface(Ci.sbILibrary), 
              destinationLibrary, 
              "Destination list should be equal to destination library!");
  
  var changes = libraryChangeset.changes;
  log("There are " + changes.length + " changes present in the changeset.");

  var addedOps = 0;
  var addedListOps = 0;
  var addedChanges = [];
  
  var deletedOps = 0;
  var deletedChanges = [];

  var movedOps = 0;
  var movedChanges = [];

  var modifiedOps = 0;
  var unknownOps = 0;

  var changesEnum = changes.enumerate();
  while(changesEnum.hasMoreElements()) {
    var changeSupports = changesEnum.getNext();
    var change = changeSupports.QueryInterface(Ci.sbILibraryChange);

    switch(change.operation) {
      case Ci.sbIChangeOperation.ADDED: 
        addedOps++; 
        if (change.itemIsList)
          addedListOps++;
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
  assertEqual(addedOps, 4, "There should be 4 added operations");
  
  log("There are " + addedListOps + " added list operations.");
  assertEqual(addedListOps, 2, "There should be 2 added list operations");
  
  log("There are " + deletedOps + " deleted operations.");
  assertEqual(deletedOps, 1, "There should be 1 deleted operations");
  
  log("There are " + modifiedOps + " modified operations.");
  assertEqual(modifiedOps, 1, "There should 1 modified operations");

  log("There are " + movedOps + " moved operations.");
  assertEqual(movedOps, 0, "There should be 0 moved operations.");

}

