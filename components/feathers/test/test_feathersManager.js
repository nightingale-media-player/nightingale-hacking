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
 * \brief FeathersManager test file
 */


var feathersManager =  Components.classes['@songbirdnest.com/songbird/feathersmanager;1']
                                 .getService(Components.interfaces.sbIFeathersManager);

// List of skin descriptions used in the test cases
var skins = [];

// List of layout descriptions used in the test cases
var layouts = [];



/**
 * Generic description object that works for skins and layouts
 */
function FeathersDescription() {
  this.wrappedJSObject = this;
};
FeathersDescription.prototype = {
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.sbISkinDescription) &&
        !iid.equals(Components.interfaces.sbILayoutDescription)) 
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};


/**
 * sbIFeathersChangeListener for testing callback
 * support in FeathersManager
 */
var feathersChangeListener = {
  
  // Count update notifications so that they can be confirmed
  // with an assert.
  updateCounter: 0,
  onFeathersUpdate: function() {
    this.updateCounter++;
  },
  
  // Set skin and layout to be expected on next select,
  // and reset when received.
  expectLayout: null,
  expectSkin: null,
  onFeathersSelectRequest: function(layoutDesc, skinDesc) {
    assertEqual(layoutDesc.wrappedJSObject, this.expectLayout);
    assertEqual(skinDesc.wrappedJSObject, this.expectSkin);
    this.expectLayout = null;
    this.expectSkin = null;
  },
  
  QueryInterface: function(iid) {
    if (!iid.equals(Components.interfaces.sbIFeathersChangeListener)) 
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
}


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
 * Make sure that the given enumerator contains all objects in 
 * the given list and nothing else.
 */
function assertEnumeratorEqualsArray(enumerator, list) {
   list = list.concat([]); // Clone list before modifying 
   for (var item in enumerator) {
    assertEqual(list.indexOf(item.wrappedJSObject) > -1, true);
    list.splice(list.indexOf(item.wrappedJSObject), 1);
  }
  assertEqual(list.length, 0);
}




/** 
 * Populate the feathers manager
 */
function setup()
{
  // Register for change callbacks
  feathersManager.addListener(feathersChangeListener);

  // Make some skins
  var skin = new FeathersDescription();
  skin.name = "Blue Skin";
  skin.provider = "blue/1.0";
  skins.push(skin);
  feathersManager.registerSkin(skin);

  skin = new FeathersDescription();
  skin.name = "Red Skin";
  skin.provider = "red/1.0";
  skins.push(skin);
  feathersManager.registerSkin(skin);

  skin = new FeathersDescription();
  skin.name = "Orange Skin";
  skin.provider = "orange/1.0";
  skins.push(skin);
  feathersManager.registerSkin(skin);
  
  // Make some layouts
  var layout = new FeathersDescription();
  layout.name = "Big Layout";
  layout.url = "chrome://biglayout/content/mainwin.xul";
  layouts.push(layout);
  feathersManager.registerLayout(layout);
  
  layout = new FeathersDescription();
  layout.name = "Mini Layout";
  layout.url = "chrome://minilayout/content/mini.xul";
  layouts.push(layout);
  feathersManager.registerLayout(layout);
  
  // Create some mappings
  // Blue -> big, Red -> big, Orange -> mini with chrome
  feathersManager.assertCompatibility(layouts[1].url, skins[0].provider, false);
  feathersManager.assertCompatibility(layouts[1].url, skins[1].provider, false);
  feathersManager.assertCompatibility(layouts[0].url, skins[2].provider, true);
}


/**
 * Remove everything from the feathers manager
 */
function teardown() {
  skins.forEach(function(skin) { 
    feathersManager.unregisterSkin(skin);
  });

  layouts.forEach(function(layout) { 
    feathersManager.unregisterLayout(layout);
  });

  // Remove mappings
  // Blue -> big, Red -> big, Orange -> mini 
  feathersManager.unassertCompatibility(layouts[1].url, skins[0].provider);
  feathersManager.unassertCompatibility(layouts[1].url, skins[1].provider);
  
  // Remove change listener before final modification to confirm
  // that it actually gets unhooked.
  feathersManager.removeListener(feathersChangeListener);
  
  feathersManager.unassertCompatibility(layouts[0].url, skins[2].provider);
}


/**
 * Test all functionality in sbFeathersManager
 */
function runTest () {
  setup();
  
  // ------------------------
  // Test change notification.  We should have seen 8 update notifications
  assertEqual(feathersChangeListener.updateCounter, 8);
  
  // ------------------------
  // Test Getters / Make sure setup succeeded
  
  // Verify all skins added properly
  assertEqual(feathersManager.skinCount, skins.length);
  var enumerator = wrapEnumerator(feathersManager.getSkinDescriptions(),
                     Components.interfaces.sbISkinDescription);
  assertEnumeratorEqualsArray(enumerator, skins);
  
  // Verify all layouts added properly
  assertEqual(feathersManager.layoutCount, layouts.length);
  var enumerator = wrapEnumerator(feathersManager.getLayoutDescriptions(), 
                     Components.interfaces.sbILayoutDescription);
  assertEnumeratorEqualsArray(enumerator, layouts);

  // Test description getters
  var desc = feathersManager.getSkinDescription(skins[1].provider).wrappedJSObject;
  assertEqual(desc, skins[1]);
  desc = feathersManager.getLayoutDescription(layouts[1].url).wrappedJSObject;
  assertEqual(desc, layouts[1]);

  // ------------------------
  // Verify mappings
  enumerator = wrapEnumerator(feathersManager.getSkinsForLayout(layouts[0].url), 
                 Components.interfaces.sbISkinDescription);
  assertEnumeratorEqualsArray(enumerator, [skins[2]]);
  enumerator = wrapEnumerator(feathersManager.getSkinsForLayout(layouts[1].url), 
                 Components.interfaces.sbISkinDescription);
  assertEnumeratorEqualsArray(enumerator, [skins[0], skins[1]]);
  
  // ------------------------
  // Verify showChrome
  assertEqual( feathersManager.isChromeEnabled(layouts[0].url, skins[2].provider), true );
  assertEqual( feathersManager.isChromeEnabled(layouts[0].url, skins[1].provider), false );
  assertEqual( feathersManager.isChromeEnabled(layouts[1].url, skins[1].provider), false );


  // ------------------------
  // Test selection mechanism
  // TODO Actually select!
  
  // First with an invalid pair
  var failed = false;
  try {
    feathersManager.switchFeathers(layouts[0].url, skins[1].provider);
  } catch (e) {
    failed = true;
  }

  // Then with a valid pair.  Expect an onSelect callback.
  feathersChangeListener.expectSkin = skins[0];
  feathersChangeListener.expectLayout = layouts[1];
  feathersManager.switchFeathers(layouts[1].url, skins[0].provider);
  // Make sure onSelect callback occurred
  assertEqual(feathersChangeListener.expectSkin, null);
  assertEqual(feathersChangeListener.expectLayout, null);

  // TODO: Test currentLayout currentSkin


  // ------------------------
  // Get rid of everything we added

  // Reset update counter so we can test teardown
  feathersChangeListener.updateCounter = 0;

  teardown();
  
  // ------------------------
  // Test change notification.  We should have seen 7 update notifications,
  // as we unhooked before the last removeMapping call.
  assertEqual(feathersChangeListener.updateCounter, 7);
  feathersChangeListener.updateCounter = 0;
  
  // ------------------------
  // Make sure teardown succeeded
  assertEqual(feathersManager.skinCount, 0);
  var enumerator = feathersManager.getSkinDescriptions();
  assertEqual(enumerator.hasMoreElements(), false); 

  assertEqual(feathersManager.layoutCount, 0);
  var enumerator = feathersManager.getLayoutDescriptions();
  assertEqual(enumerator.hasMoreElements(), false); 

  // ------------------------  
  // Verify mappings are gone
  var enumerator = feathersManager.getSkinsForLayout(layouts[0].url);
  assertEqual(enumerator.hasMoreElements(), false); 
  var enumerator = feathersManager.getSkinsForLayout(layouts[1].url);
  assertEqual(enumerator.hasMoreElements(), false); 
  
  
  return Components.results.NS_OK;
}


