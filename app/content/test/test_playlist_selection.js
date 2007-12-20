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

function runTest () {

  initMockCore();

  var url = "data:application/vnd.mozilla.xul+xml," +
            "<?xml-stylesheet href='chrome://global/skin' type='text/css'?>" +
            "<?xml-stylesheet href='chrome://songbird/content/bindings/bindings.css' type='text/css'?>" +
            "<window xmlns='http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul'/>";

  beginWindowTest(url, setupPlaylist);
}

function setupPlaylist() {

  // We need our tree to be visible in order to get the correct events sent
  // to the view
  testWindow.resizeTo(200, 200);

  var databaseGUID = "test_playlist_selection";
  var library = createLibrary(databaseGUID, false);

  const XUL_NS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

  var document = testWindow.document;

  var playlist = document.createElementNS(XUL_NS, "sb-playlist");
  playlist.setAttribute("flex", "1");
  document.documentElement.appendChild(playlist);
  playlist.bind(library.createView(), null);

  // kick it off
  continueWindowTest(testSelectAll, [library, playlist]);

}

function testSelectAll(library, playlist) {

  log("testSelectAll");

  setSort(playlist, "created", "a");

  var view = playlist.mediaListView;
  var guids = [];
  for (var i = 0; i < library.length; i ++) {
    guids.push(view.getItemByIndex(i).guid);
  }

  safeSetTimeout(function() {
    var treeView = playlist.tree.view;
    treeView.selection.selectAll();
    var selected = getSelected(playlist);
    assertArraySimilar(guids, selected);

    // continue
    continueWindowTest(testSelectAllResort, [library, playlist, selected]);
  }, 500);

}

function testSelectAllResort(library, playlist, previouslySelected) {

  log("testSelectAllResort");
  var treeView = playlist.tree.view;

  setSort(playlist, "created", "d");

  safeSetTimeout(function() {
    treeView.selection.selectAll();
    var selected = getSelected(playlist);
    assertArraySimilar(previouslySelected.reverse(), selected);

    // continue
    continueWindowTest(testSelectSome, [library, playlist]);
  }, 500);

}

function testSelectSome(library, playlist) {

  log("testSelectSome");
  var treeView = playlist.tree.view;

  treeView.selection.clearSelection();

  var guids = [];
  for (var i = 0; i < library.length; i += 2) {
    treeView.selection.toggleSelect(i);
    guids.push(playlist.mediaListView.getItemByIndex(i).guid);
  }

  var selected = getSelected(playlist);
  assertArraySimilar(guids, selected);

  // continue
  continueWindowTest(testSelectSomeResort, [library, playlist, selected]);
}

function testSelectSomeResort(library, playlist, previouslySelected) {

  log("testSelectSomeResort");

  setSort(playlist, "created", "a");

  safeSetTimeout(function() {
    var selected = getSelected(playlist);
    assertArraySimilar(previouslySelected.reverse(), selected);

    // continue
    continueWindowTest(testSelectAllThenAdd, [library, playlist]);
  }, 500);
}

function testSelectAllThenAdd(library, playlist) {

  log("testSelectAllThenAdd");

  var treeView = playlist.tree.view;
  treeView.selection.selectAll();

  // Newly added item should be added into the selection
  library.createMediaItem(newURI("file:///foo"));
  var view = playlist.mediaListView;
  var guids = [];
  for (var i = 0; i < library.length; i ++) {
    guids.push(view.getItemByIndex(i).guid);
  }

  safeSetTimeout(function() {
    var selected = getSelected(playlist);
    assertArraySimilar(guids, selected);

    // continue
    continueWindowTest(testSelectRange, [library, playlist]);
  }, 500);
}

function testSelectRange(library, playlist) {

  log("testSelectRange");

  // Set the sort so the cache gets cleared
  setSort(playlist, "created", "a");

  var view = playlist.mediaListView;
  var guids = [];
  for (var i = 1; i < library.length - 1; i ++) {
    guids.push(view.getItemByIndex(i).guid);
  }

  // Select everything but the first and last.  To make this realistic, we
  // need to make sure the first and last pages of the tree are made visible
  // to emulate how a user would make this selection
  var bo = playlist.tree.boxObject.QueryInterface(Ci.nsITreeBoxObject);
  bo.ensureRowIsVisible(0);

  safeSetTimeout(function() {
    bo.ensureRowIsVisible(library.length - 1);

    safeSetTimeout(function() {
      var treeView = playlist.tree.view;
      treeView.selection.clearSelection();

      // Use an augmented select here so we can test selecting a range over
      // non-cached pages.  Note that this really does not work right now since
      // this test data is smaller than the fetch size so there are never non-
      // cached pages.  But I promise I've tested this by altering the fetch
      // size to something small.
      treeView.selection.rangedSelect(1, library.length - 2, true);
      var selected = getSelected(playlist);
      assertArraySimilar(guids, selected);

      // continue
      continueWindowTest(testSelectRangeResort, [library, playlist, selected]);
    }, 500);
  }, 500);

}

function testSelectRangeResort(library, playlist, previouslySelected) {

  log("testSelectRangeResort");

  setSort(playlist, "created", "d");

  safeSetTimeout(function() {
    var selected = getSelected(playlist);
    assertArraySimilar(previouslySelected.reverse(), selected);

    // done
    endWindowTest();
  }, 500);
}

function assertArraySimilar(a1, a2) {

  if (a1.length != a2.length) {
    fail("compare failed, length wrong, got " + a1.length + " expected " + a2.length);
  }

  for (var i = 0; i < a1.length; i++) {
    if (a1[i] != a2[i]) {
      fail("arrays don't match at index " + i + " '" + a1[i] + "' != '" + a2[i] + "'");
    }
  }

}

function setSort(playlist, property, direction) {
  var view = playlist.mediaListView;
  var pa = createPropertyArray();
  pa.strict = false;
  pa.appendProperty("http://songbirdnest.com/data/1.0#" + property, direction);
  view.setSort(pa);
}

function getSelected(playlist) {
  var treeView = playlist.tree.view;
  var enumerator = treeView.selectedMediaItems;
  var selected = [];
  while (enumerator.hasMoreElements()) {
    selected.push(enumerator.getNext().mediaItem.guid);
  }
  return selected;
}

function createPropertyArray() {
  return Cc["@songbirdnest.com/Songbird/Properties/MutablePropertyArray;1"]
           .createInstance(Ci.sbIMutablePropertyArray);
}
