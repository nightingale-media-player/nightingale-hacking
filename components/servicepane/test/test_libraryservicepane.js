/** vim: ts=2 sw=2 expandtab ai
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
 * \brief Basic library service pane unit tests
 */

const DEBUG_OUTPUT = false;

const PROP_ISLIST = "http://getnightingale.com/data/1.0#isList";
const LSP='http://getnightingale.com/rdf/library-servicepane#';

var servicePane;
var libraryServicePane;
var libraryManager;

// Used to track things added using the tests
var listsToRemove = [];
var librariesToRemove = [];
var nodesToRemove = [];



function runTest () {
  return;
  setup();

  testLibrariesAndContents();

  testAddingSimplePlaylists();

  testInsertionLogic();

  testLibrariesAndContents();

  testLibrarySuggestion();

  testLibrariesAndContents();

  testRenaming();

  testLibrariesAndContents();

  removeAddedItems();

  testAllItemsRemoved();

  testLibrariesAndContents();

  servicePane = null;
  libraryServicePane = null;
  libraryManager = null;
}


function setup() {
  servicePane = Components.classes['@getnightingale.com/servicepane/service;1']
                          .getService(Components.interfaces.sbIServicePaneService);
  assertNotEqual(servicePane, null);

  libraryServicePane = Components.classes['@getnightingale.com/servicepane/library;1']
                                 .getService(Components.interfaces.sbILibraryServicePaneService);
  assertNotEqual(libraryServicePane, null);

  libraryManager = Components.classes['@getnightingale.com/Nightingale/library/Manager;1']
                             .getService(Components.interfaces.sbILibraryManager);
  assertNotEqual(libraryManager, null);

  // Start with an empty library
  libraryManager.mainLibrary.clear();
}



/**
 * Remove anything that was added during the tests
 */
function removeAddedItems() {
  DBG("About to remove all test items from tree");
  showTree(servicePane.root, "tree-before-delete: ");

  // Remove all media lists added
  for (var i = 0; i < listsToRemove.length; i++) {
    listsToRemove[i].library.remove(listsToRemove[i]);
  }

  // Remove all libraries added
  for (var i = 0; i < librariesToRemove.length; i++) {
    libraryManager.unregisterLibrary(librariesToRemove[i]);
  }

  DBG("Finished removing test items from tree");
  showTree(servicePane.root, "tree-after-delete: ");
}



/////////////
// Helpers //
/////////////

/**
 * Console log helper
 */
function DBG(s) {
  if (DEBUG_OUTPUT) {
    // Madness. If I don't use this intermediate var
    // it warns me that DBG.caller has no properties,
    // but then works just fine anyway.
    var myCaller = DBG.caller;
    //dump('DBG:test_libraryservicepane:' + myCaller.name + ": " +s+'\n');
    log('DBG:test_libraryservicepane:' + myCaller.name + ": " +s+'\n');
  }
}

/**
 * Debug helper to print the structure of the tree to the console
 */
function showTree(root, prefix) {
  var node = root.firstChild;
  while(node) {
    DBG(prefix + node.name + " [" + node.id + "]");
    showTree(node, prefix + "  ");
    node = node.nextSibling;
  }
}


///////////
// Tests //
///////////


/**
 * Make sure all registered libraries and their associated
 * playlists are present in the tree
 */
function testLibrariesAndContents() {
  var libraries = libraryManager.getLibraries();
  var count = 0;

  DBG("About to check library manager against the service tree:");
  showTree(servicePane.root, "before-checking-libraries:  ");
  assertNotEqual(servicePane.root, null);

  while (libraries.hasMoreElements()) {
    var library = libraries.getNext();
    count++;

    var node = libraryServicePane.getNodeForLibraryResource(library);
    assertNotEqual(node, null);

    // Test that we can get from the node back to the library
    var libraryFromSP = libraryServicePane.getLibraryResourceForNode(node)
    assertEqual(library, libraryFromSP);

    // All libraries should be at the top level
    assertNotEqual(node.parentNode, null);
    assertEqual(node.parentNode, servicePane.root);

    // All libraries should be visible
    assertEqual(node.hidden, false);

    // The name should match
    assertEqual(node.name, library.name);

    // Make sure the playlists of this library are reflected in the tree
    testLibraryMediaLists(library);

    // Make sure createNodeForLibrary returns same node as getNodeForLibraryResource.
    var createNode = libraryServicePane.createNodeForLibrary(library);
    assertEqual(node, createNode);
  }

  var library = createLibrary("test-createNodeForLibrary");
  library.setProperty(SBProperties.hidden, true);
  library.name = "Testing createNodeForLibrary";
  libraryManager.registerLibrary(library, false);

  var node = libraryServicePane.createNodeForLibrary(library);

  assertNotEqual(node, null);
  assertEqual(node.hidden, true);
  assertEqual(node.name, library.name);

  libraryManager.unregisterLibrary(library);

  // We should have seen at least the main and web libs
  DBG("processed " + count + " libraries");
  assertEqual(count >= 2, true);

  DBG("done");
}


/**
 * Make sure all the playlists of the given library
 * are present in the tree
 */
function testLibraryMediaLists(aLibrary) {
  // Listener to receive enumerated items and store then in an array
  var listener = {
    items: [],
    onEnumerationBegin: function() { },
    onEnumerationEnd: function() { },
    onEnumeratedItem: function(list, item) {
      this.items.push(item);
    }
  };

  // Enumerate all lists in this library
  aLibrary.enumerateItemsByProperty(PROP_ISLIST, "1",
                                    listener,
                                    Components.interfaces.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  // Make sure we have a node for each list
  for (var i = 0; i < listener.items.length; i++) {
    var node = libraryServicePane.getNodeForLibraryResource(listener.items[i]);
    assertNotEqual(node, null);
    assertNotEqual(node.parentNode, null);

    var medialistFromSP = libraryServicePane.getNodeForLibraryResource(listener.items[i]);
    assertNotEqual(listener.items[i], medialistFromSP);

    // Test that we can get from the node back to the library
    var medialistFromSP = libraryServicePane.getLibraryResourceForNode(node)
    assertEqual(listener.items[i], medialistFromSP);

    // All libraries should be visible
    //assertEqual(node.hidden, false);

    // The name should match
    assertEqual(node.name, listener.items[i].name);

    // Main library playlists should be at the top level
    if (aLibrary == libraryManager.mainLibrary) {
      assertEqual(node.parentNode, servicePane.root);

    // All others should be nested
    } else {
      var libraryNode = libraryServicePane.getNodeForLibraryResource(aLibrary);
      assertEqual(node.parentNode, libraryNode);
    }
  }
}



/**
 * Add some simple playlists to the main library and confirm that they
 * they show up in the tree
 */
function testAddingSimplePlaylists() {

  var library = libraryManager.mainLibrary;

  // Create the playlist
  var mediaList = library.createMediaList("simple");
  mediaList.name = "test playlist";

  // Make sure the playlist appears in the tree
  var node = libraryServicePane.getNodeForLibraryResource(mediaList);
  assertNotEqual(node, null);

  // Make sure the name is the same
  assertEqual(node.name, mediaList.name);

  // Make sure this will get cleaned up
  listsToRemove.push(mediaList);
}



/**
 * Exercise the logic that places new library and medialist
 * nodes in tree
 *
 * TODO: Need to test grouping by type.
 *       Do this once smart playlists are ready.
 */
function testInsertionLogic() {

  // Insert media lists into the main library
  var list1 = libraryManager.mainLibrary.createMediaList("simple");
  var list2 = libraryManager.mainLibrary.createMediaList("simple");
  list1.name = "Test1";
  list2.name = "Test2";

  // Make sure they appeared in the tree
  var node1 = libraryServicePane.getNodeForLibraryResource(list1);
  var node2 = libraryServicePane.getNodeForLibraryResource(list2);
  assertNotEqual(node1, null);
  assertNotEqual(node2, null);

  // Should show up after the main library but before any bookmarks
  // or other content
  var mainLibraryNode = libraryServicePane.getNodeForLibraryResource(
                            libraryManager.mainLibrary);
  var node = mainLibraryNode.nextSibling;
  while (node && node.contractid == mainLibraryNode.contractid) {
    if (node == node1) {
      break;
    }
    node = node.nextSibling;
  }
  assertNotEqual(node, null);
  assertEqual(node, node1);

  // The second playlist should appear after the first
  assertEqual(node1.nextSibling, node2);

  // Now pretend the user moves their playlists in between some
  // of their bookmark folders
  var bmNode1 = servicePane.createNode();
  bmNode1.id = "urn:test:1";
  servicePane.root.appendChild(bmNode1);

  var bmNode2 = servicePane.createNode();
  bmNode1.id = "urn:test:2";
  servicePane.root.appendChild(bmNode2);

  DBG("About to move all playlists between some bookmarks");
  showTree(servicePane.root, "tree-before-move: ");
  var type = node1.getAttributeNS(LSP, "ListType");
  node = mainLibraryNode;
  // Search for playlists between the library and the first fake bookmark
  DBG("About to iterate");
  while (node && node != bmNode1) {
    // If this is a playlist then move it in between our fake bookmarks
    DBG("checking node " + node.id);
    if (node.getAttributeNS(LSP, "ListType") == type) {
      DBG("moving node " + node.id);
      node = node.nextSibling;
      servicePane.root.insertBefore(node.previousSibling, bmNode2);
    } else {
      node = node.nextSibling;
    }
  }
  DBG("Finished moving playlists");
  showTree(servicePane.root, "tree-after-move: ");
  assertEqual(bmNode1, node);

  // Newly inserted playlists should appear directly below other playlists
  // in the middle of the bookmarks
  var list3 = libraryManager.mainLibrary.createMediaList("simple");
  list3.name = "Test3";

  DBG("After adding a new playlist:");
  showTree(servicePane.root, "tree-after-add: ");

  var node3 = libraryServicePane.getNodeForLibraryResource(list3);
  assertNotEqual(node3, null);
  assertEqual(node2.nextSibling, node3);
  assertEqual(node3.nextSibling, bmNode2);

  // Now add a new library so that we can test nesting
  var newLibrary = createLibrary("test1");
  newLibrary.name = "Test1";
  libraryManager.registerLibrary(newLibrary, false);

  DBG("After adding a new library:");
  showTree(servicePane.root, "tree-after-newlibrary: ");

  // New library should appear below the other libraries
  var newLibraryNode = libraryServicePane.getNodeForLibraryResource(newLibrary);
  assertNotEqual(newLibraryNode, null);
  assertEqual(newLibraryNode.name, newLibrary.name);
  type = newLibraryNode.getAttributeNS(LSP, "ListType");
  node = newLibraryNode.previousSibling;
  while (node && node.getAttributeNS(LSP, "ListType") == type &&
         node != mainLibraryNode)
  {
    DBG("Scanning backwards from new library to main library: " + node.id);
    node = node.previousSibling;
  }
  assertNotEqual(node, null);
  assertEqual(node, mainLibraryNode);

  // Insert a playlist into the new library
  var list4 = newLibrary.createMediaList("simple");
  list4.name = "Test4";

  DBG("After adding a playlist into the new library:");
  showTree(servicePane.root, "tree-after-newlibrary-playlist: ");

  // Should appear nested under the library
  var node4 = libraryServicePane.getNodeForLibraryResource(list4);
  assertNotEqual(node4, null);
  assertEqual(node4.name, list4.name);
  assertEqual(node4.parentNode, newLibraryNode);

  // Make sure the things we added will get removed
  listsToRemove = listsToRemove.concat(list1, list2, list3, list4);
  librariesToRemove = librariesToRemove.concat(newLibrary);
  nodesToRemove = nodesToRemove.concat(node1, node2, bmNode1, bmNode2, node3, newLibraryNode, node4);
}


/**
 * Make sure the library service pane correctly suggests a
 * library to use for making a new playlist
 */
function testLibrarySuggestion() {
  DBG("About to test library suggestion functionality");

  // No selection should give the main library
  var suggestedLibrary = libraryServicePane.suggestLibraryForNewList(
              "simple", null);
  assertEqual(suggestedLibrary, libraryManager.mainLibrary);

  // Add a new library so that we can test nesting
  var newLibrary = createLibrary("suggestion-test");
  newLibrary.name = "Test2";
  libraryManager.registerLibrary(newLibrary, false);

  // New library should appear below the other libraries
  var newLibraryNode = libraryServicePane.getNodeForLibraryResource(newLibrary);

  // Selection of the new library should return the new library
  suggestedLibrary = libraryServicePane.suggestLibraryForNewList(
              "simple", newLibraryNode);
  assertEqual(suggestedLibrary, newLibrary);


  // Selection of a playlist in the new library should
  // return the new library
  var list1 = newLibrary.createMediaList("simple");
  list1.name = "Test1";
  var node1 = libraryServicePane.getNodeForLibraryResource(list1);
  suggestedLibrary = libraryServicePane.suggestLibraryForNewList(
              "simple", node1);
  assertEqual(suggestedLibrary, newLibrary);

  // Selection of a new library playlist placed at the top
  // level should still return the new library
  var list2 = newLibrary.createMediaList("simple");
  list2.name = "Test2";
  var node2 = libraryServicePane.getNodeForLibraryResource(list2);
  suggestedLibrary = libraryServicePane.suggestLibraryForNewList(
              "simple", node2);
  assertEqual(suggestedLibrary, newLibrary);

  // Selection of a bookmark under the new library should
  // return the new library
  var bmNode1 = servicePane.createNode();
  bmNode1.id = "urn:select-test:1";
  newLibraryNode.appendChild(bmNode1);

  var suggestedLibrary = libraryServicePane.suggestLibraryForNewList(
              "simple", bmNode1);
  assertEqual(suggestedLibrary, newLibrary);

  // Selection of a bookmark at the top level should
  // return the main library
  var bmNode2 = servicePane.createNode();
  bmNode2.id = "urn:select-test:2";
  servicePane.root.appendChild(bmNode2);

  var suggestedLibrary = libraryServicePane.suggestLibraryForNewList(
              "simple", bmNode2);
  assertEqual(suggestedLibrary, libraryManager.mainLibrary);

  // Requesting an unsupported type should give null
  var suggestedLibrary = libraryServicePane.suggestLibraryForNewList(
              "fakemedialisttype", null);
  assertEqual(suggestedLibrary, null);

  DBG("Tree after suggestion testing:");
  showTree(servicePane.root, "tree-after-newlibrary: ");


  // Make sure the things we added will get removed
  listsToRemove = listsToRemove.concat(list1, list2);
  librariesToRemove = librariesToRemove.concat(newLibrary);
  nodesToRemove = nodesToRemove.concat(node1, node2, bmNode1, bmNode2, newLibraryNode);
}



/**
 * Make sure that when the user renames a node the changes
 * are reflected into the library
 */
function testRenaming() {
  DBG("About to test library renaming functionality");
  // Add a new library to rename
  var newLibrary = createLibrary("renaming-test");
  newLibrary.name = "Test2";
  libraryManager.registerLibrary(newLibrary, false);
  var newLibraryNode = libraryServicePane.getNodeForLibraryResource(newLibrary);

  // Add a list to the library
  var list1 = newLibrary.createMediaList("simple");
  list1.name = "Test1";
  var node1 = libraryServicePane.getNodeForLibraryResource(list1);

  // Fake a rename on the library
  var newName = "RenameTest1";
  servicePane.onRename(newLibraryNode, newName);
  // Make sure the name was changed correctly
  assertEqual(newLibraryNode.name, newName);

  // Fake a rename on the playlist
  newName = "RenameTest2";
  servicePane.onRename(node1, newName);
  // Make sure the name was changed correctly
  assertEqual(node1.name, newName);

  // Make sure that renaming to an empty string is not accepted
  servicePane.onRename(node1, "");
  assertEqual(node1.name, newName);

  // Make sure the things we added will get removed
  listsToRemove = listsToRemove.concat(list1);
  librariesToRemove = librariesToRemove.concat(newLibrary);
  nodesToRemove = nodesToRemove.concat(node1, newLibraryNode);
}


/**
 * Confirm that removing the test medialists and libraries
 * was reflected correctly in the service pane tree
 */
function testAllItemsRemoved() {

  // Remove all service tree nodes added
  for (var i = 0; i < nodesToRemove.length; i++) {
    DBG("Confirming removal of: " + nodesToRemove[i].id );
    var id = nodesToRemove[i].id;
    // Playlist nodes should be removed
    if (id.indexOf("urn:item") == 0) {
      assertEqual(nodesToRemove[i].parentNode, null);
    // Library nodes should be hidden
    } else if (id.indexOf("urn:library") == 0) {
      assertEqual(nodesToRemove[i].hidden, true);
      nodesToRemove[i].parentNode.removeChild(nodesToRemove[i]);
    } else {
      nodesToRemove[i].parentNode.removeChild(nodesToRemove[i]);
    }
  }
}






