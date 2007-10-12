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
 * \brief FaceplateManager test file
 * 
 * NOTE: This test has been temporarily disabled.
 * See sbIFaceplateManager for more information.
 */
 

var faceplateManager =  Cc['@songbirdnest.com/faceplate/manager;1']
                          .getService(Ci.sbIFaceplateManager);

var panes = [];

/**
 * sbIFaceplateManagerListener for testing callback
 * support in FaceplateManager
 */
var faceplateManagerListener = {
  
  expectCreateID: null,
  expectShowID: null,
  expectDestroyID: null,
  
  onCreatePane: function(pane) {
    assertEqual(pane.id,  this.expectCreateID);
    this.expectCreateID = null;
  },

  onShowPane: function(pane) {
    assertEqual(pane.id, this.expectShowID);
    this.expectShowID = null;    
  },

  onDestroyPane: function(pane) {
    assertEqual(pane.id, this.expectDestroyID);
    this.expectDestroyID = null;    
  },

  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.sbIFeathersChangeListener)) 
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}



/**
* Test all functionality in sbFaceplateManager
 */
function runTest () {
  
  // Clear out the default panes so that we can test an empty
  // faceplate manager
  clearManager();
  
  assertEqual(faceplateManager.paneCount, 0);
  
  faceplateManager.addListener(faceplateManagerListener);
  
  testCreatePanes();
  
  testAccessors();

  testFaceplatePaneData();
  
  testShowPane();
  
  testDestroyPanes();
  
  // Make sure unregistering the listener actually works
  faceplateManager.removeListener(faceplateManagerListener);
  faceplateManagerListener.expectCreateID = null;
  faceplateManagerListener.expectDestroyID = null; 
  var pane = faceplateManager.createPane("test", "test1", "chrome:test1");
  faceplateManager.destroyPane(pane);
  
  assertEqual(faceplateManager.paneCount, 0);  
  
  return Components.results.NS_OK;
}


///////////
// Tests //
///////////


function testCreatePanes() {
  for (var i = 0; i < 3; i++) {
    var id = "testpane" + i;
    
    // Test creation listener
    faceplateManagerListener.expectCreateID = id;
    
    var pane = faceplateManager.createPane(id, "test1", "chrome:test1");
    assertEqual(pane.id, id);
    assertEqual(pane.name, "test1"); 
    assertEqual(pane.bindingURL, "chrome:test1");
    panes.push(pane);    
    
    // Make sure the listener fired
    assertEqual(faceplateManagerListener.expectCreateID, null);
  }
}


function testAccessors() {
  // Test paneCount
  assertEqual(faceplateManager.paneCount, panes.length);
  
  // Test getPanes()
  var enumerator = wrapEnumerator(faceplateManager.getPanes(), 
                                  Ci.sbIFaceplatePane);
  assertEnumeratorEqualsArray(enumerator, panes);
  
  // Test getPane(id)
  for (var i = 0; i < panes.length; i++) {
    var pane = faceplateManager.getPane(panes[i].id);
    assertEqual(pane.id, panes[i].id);
  }
}


function testShowPane() {
  faceplateManagerListener.expectShowID = panes[0].id;
    
  faceplateManager.showPane(panes[0]);
  
  // Make sure the listener fired
  assertEqual(faceplateManagerListener.expectShowID, null);
  
  // Test getDefaultPane.  
  var pane = faceplateManager.getDefaultPane();
  assertEqual(pane.id, panes[0].id);  
}


function testDestroyPanes() {
  for (var i = 0; i < panes.length; i++) {
    // Test listener
    faceplateManagerListener.expectDestroyID = panes[i].id;
    
    faceplateManager.destroyPane(panes[i]);
    
    // Make sure the listener fired
    assertEqual(faceplateManagerListener.expectDestroyID, null);
  }
  assertEqual(faceplateManager.paneCount, 0);
  panes = [];
}


function testFaceplatePaneData() {
  var pane = panes[0];
  
  // Observer that asserts expected values
  var observer = {
    expectKey: null,
    expectValue: null,
    observe: function(subject, topic, data) {
      assertEqual(subject.id, pane.id);
      assertEqual(topic, this.expectKey);
      assertEqual(pane.getData(topic), this.expectValue);      
    }
  };
  pane.addObserver(observer);
  
  // Test set/get/observe
  var keys = ["test1", "test2"];
  observer.expectKey = keys[0];
  observer.expectValue = "success1";
  pane.setData(observer.expectKey, observer.expectValue);
  observer.expectKey = keys[1];
  observer.expectValue = "success2";
  pane.setData(observer.expectKey, observer.expectValue); 
  
  // Test enumerate
  var enumerator = wrapStringEnumerator(pane.getKeys());
  assertEnumeratorEqualsArray(enumerator, keys);
  
  // Make sure removing the observer works as it should
  pane.removeObserver(observer);
  observer.expectKey = null;
  pane.setData("test", "blah");
}

/////////////
// Helpers //
/////////////

/** 
 * Turn an nsISimpleEnumerator into a generator
 */ 
function wrapEnumerator(enumerator, iface)
{
  while (enumerator.hasMoreElements()) {
    yield enumerator.getNext().QueryInterface(iface);
  }
}

/** 
 * Turn an nsIStringEnumerator into a generator
 */ 
function wrapStringEnumerator(enumerator)
{
  while (enumerator.hasMore()) {
    yield enumerator.getNext();
  }
}


/**
 * Make sure that the given enumerator contains all objects in 
 * the given list and nothing else.
 */
function assertEnumeratorEqualsArray(enumerator, list) {
  list = list.concat(); // Clone list before modifying 
  for (var item in enumerator) {
    assertEqual(list.indexOf(item) > -1, true);
    list.splice(list.indexOf(item), 1);
  }
  assertEqual(list.length, 0);
}

/**
 * Remove all panes in the faceplate pane manager
 */
function clearManager () {
  var enumerator = wrapEnumerator(faceplateManager.getPanes(), Ci.sbIFaceplatePane);
  for (pane in enumerator) {
    faceplateManager.destroyPane(pane);
  }
}



