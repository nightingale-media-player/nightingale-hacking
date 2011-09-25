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

function runTest () {

  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
  Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

  var library = createLibrary("test_medialistviewselection");
  var view = library.createView();
  var selection = view.selection;

  assertEqual(selection.count, 0);

  selection.currentIndex = 0;
  assertEqual(selection.currentIndex, 0);

  var item = selection.currentMediaItem;
  assertTrue(item.equals(view.getItemByIndex(0)));

  selection.currentIndex = -1;
  item = selection.currentMediaItem;
  assertEqual(item, null);

  selection.select(0);
  assertTrue(selection.isIndexSelected(0));
  assertEqual(selection.count, 1);

  selection.select(1);
  assertTrue(selection.isIndexSelected(0));
  assertTrue(selection.isIndexSelected(1));
  assertEqual(selection.count, 2);

  selection.toggle(0);
  assertFalse(selection.isIndexSelected(0));
  assertEqual(selection.count, 1);

  selection.selectRange(3, 5);
  assertEqual(selection.count, 4);

  selection.clearRange(1, 4);
  assertEqual(selection.count, 1);

  selection.selectNone();
  assertEqual(selection.count, 0);

  selection.selectAll();
  assertEqual(selection.count, library.length);

  selection.clear(3);
  assertEqual(selection.count, library.length - 1);

  selection.select(3);
  assertEqual(selection.count, library.length);

  selection.selectOnly(2);
  assertTrue(selection.isIndexSelected(2));
  assertEqual(selection.count, 1);

  // Test notifications
  var listener = {
    countSelectionChanged: 0,
    countCurrentIndexChanged: 0,
    onSelectionChanged: function() {
      this.countSelectionChanged++;
    },
    onCurrentIndexChanged: function() {
      this.countCurrentIndexChanged++;
    },
    reset: function() {
      this.countCurrentIndexChanged = 0;
      this.countSelectionChanged = 0;
    }
  }

  selection.addListener(listener);
  selection.select(0);
  assertEqual(listener.countSelectionChanged, 1);
  selection.selectionNotificationsSuppressed = true;
  selection.select(0);
  assertEqual(listener.countSelectionChanged, 1);
  selection.selectionNotificationsSuppressed = false;

  selection.currentIndex = 2;
  assertEqual(listener.countCurrentIndexChanged, 1);

  selection.removeListener(listener);

  // Test selectedMediaItems
  // some
  var items = [];
  items.push(library.getItemByIndex(2));
  items.push(library.getItemByIndex(10));
  items.push(library.getItemByIndex(20));
  items.push(library.getItemByIndex(21));

  selection.selectNone();
  selection.select(2);
  selection.toggle(10);
  selection.toggle(20);
  selection.toggle(21);

  assertSelectedItems(selection, items);

  // all
  var allItems = [];
  for (var i = 0; i < library.length; i++) {
    allItems.push(library.getItemByIndex(i));
  }
  selection.selectAll();
  assertSelectedItems(selection, allItems);

  // all but one
  allItems.splice(10, 1);
  selection.toggle(10);
  assertSelectedItems(selection, allItems);

  // all but a range
  allItems.splice(10, 2);
  selection.selectAll();
  selection.clearRange(10, 12);
  assertSelectedItems(selection, allItems);
}

function assertSelectedItems(selection, items) {

  var allGuids = {};
  for (var i = 0; i < items.length; i++) {
    allGuids[items[i].guid] = 1;
  }

  var allItems = selection.selectedIndexedMediaItems;
  var count = 0;
  while(allItems.hasMoreElements()) {
    var item = allItems.getNext().mediaItem;
    assertTrue(item.guid in allGuids);
    delete allGuids[item.guid];
    count++;
  }

  assertEqual(count, items.length);
}
