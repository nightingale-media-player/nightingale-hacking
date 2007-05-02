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

  var library = createLibrary("test_library_getmedialists");

  testHiddenProperty(library);
  testIsListProperty(library);
  testBoth(library);
}

function testVisibleProperty(library) {

  var PROP_HIDDEN = "http://songbirdnest.com/data/1.0#hidden";

  library.clear();
  var items = [];
  for (var i = 0; i < 10; i++) {
    var item = library.createMediaItem(newURI("file://foo/" + i));
    var hidden = item.getProperty(PROP_HIDDEN);
    assertEqual(hidden, "0");
    if (i % 2 == 0) {
      item.setProperty(PROP_HIDDEN, "1");
      item.write();
    }
  }
  assertEqual(library.length, 10);

  var listener = {
    items: [],
    onEnumerationBegin: function() {
      return true;
    },
    onEnumeratedItem: function(list, item) {
      this.items.push(item);
      return true;
    },
    onEnumerationEnd: function() {
      return true;
    }
  };

  library.enumerateItemsByProperty(PROP_HIDDEN, "0",
                                   listener,
                                   Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  assertEqual(listener.items.length, 5);

  listener.items = [];
  library.enumerateItemsByProperty(PROP_HIDDEN, "1",
                                   listener,
                                   Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  assertEqual(listener.items.length, 5);
}

function testIsListProperty(library) {

  var PROP_ISLIST = "http://songbirdnest.com/data/1.0#isList";

  library.clear();
  for (var i = 0; i < 11; i++) {
    library.createMediaItem(newURI("file://foo/" + i));
  }
  for (var i = 0; i < 7; i++) {
    library.createMediaList("simple");
  }
  assertEqual(library.length, 18);

  var listener = {
    items: [],
    onEnumerationBegin: function() {
      return true;
    },
    onEnumeratedItem: function(list, item) {
      this.items.push(item);
      return true;
    },
    onEnumerationEnd: function() {
      return true;
    }
  };

  library.enumerateItemsByProperty(PROP_ISLIST, "0",
                                   listener,
                                   Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  assertEqual(listener.items.length, 11);

  listener.items = [];

  library.enumerateItemsByProperty(PROP_ISLIST, "1",
                                   listener,
                                   Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  assertEqual(listener.items.length, 7);

}

function testBoth(library) {

  var PROP_ISLIST = "http://songbirdnest.com/data/1.0#isList";
  var PROP_HIDDEN = "http://songbirdnest.com/data/1.0#hidden";

  library.clear();
  for (var i = 0; i < 20; i++) {
    library.createMediaItem(newURI("file://foo/" + i));
  }
  for (var i = 0; i < 7; i++) {
    library.createMediaList("simple");
  }
  for (var i = 0; i < 3; i++) {
    var list = library.createMediaList("simple");
    list.setProperty(PROP_HIDDEN, "1");
    list.write();
  }
  assertEqual(library.length, 30);

  var listener = {
    items: [],
    onEnumerationBegin: function() {
      return true;
    },
    onEnumeratedItem: function(list, item) {
      this.items.push(item);
      return true;
    },
    onEnumerationEnd: function() {
      return true;
    }
  };

  // All non-hidden things but no lists, just like a library view
  var pa = createPropertyArray();
  pa.appendProperty(PROP_ISLIST, "0");
  pa.appendProperty(PROP_HIDDEN, "0");

  library.enumerateItemsByProperties(pa, listener,
                                     Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  assertEqual(listener.items.length, 20);

  // All non-hidden lists, for the service pane
  listener.items = [];

  var pa = createPropertyArray();
  pa.appendProperty(PROP_ISLIST, "1");
  pa.appendProperty(PROP_HIDDEN, "0");

  library.enumerateItemsByProperties(pa, listener,
                                     Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  assertEqual(listener.items.length, 7);

  // All hidden lists
  listener.items = [];

  var pa = createPropertyArray();
  pa.appendProperty(PROP_ISLIST, "1");
  pa.appendProperty(PROP_HIDDEN, "1");

  library.enumerateItemsByProperties(pa, listener,
                                     Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  assertEqual(listener.items.length, 3);

}

function createPropertyArray() {
  var pa = Cc["@songbirdnest.com/Songbird/Properties/PropertyArray;1"]
             .createInstance(Ci.sbIPropertyArray);
  return pa;
}

