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
 * \brief Test that changes in the view's selection are reflected into the tree
 */

function runTest () {
  var url = "data:application/vnd.mozilla.xul+xml," +
            "<?xml-stylesheet href='chrome://global/skin' type='text/css'?>" +
            "<?xml-stylesheet href='chrome://nightingale/content/bindings/bindings.css' type='text/css'?>" +
            "<window xmlns='http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul'/>";

  beginWindowTest(url, setupPlaylist);
}

function setupPlaylist() {

  testWindow.resizeTo(200, 200);

  var databaseGUID = "test_view_selection";
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

  playlist.mediaListView.selection.selectAll();

  safeSetTimeout(function() {

    // All of the visible rows should be selected
    assertAllVisibleRowsSelected(playlist.tree);

    continueWindowTest(testSelectAllScrolled, [library, playlist]);
  }, 1000);

}

function testSelectAllScrolled(library, playlist) {

  log("testSelectAllScrolled");
  playlist.tree.boxObject.ensureRowIsVisible(library.length - 1);

  safeSetTimeout(function() {

    assertAllVisibleRowsSelected(playlist.tree);
    continueWindowTest(testSelectSome, [library, playlist]);
  }, 1000);

}

function testSelectSome(library, playlist) {

  log("testSelectSome");

  // select first and last
  var selection = playlist.mediaListView.selection;
  selection.select(0);
  selection.toggle(library.length - 1);

  safeSetTimeout(function() {

    assertTrue(playlist.tree.view.selection.isSelected(library.length - 1));

    playlist.tree.boxObject.ensureRowIsVisible(0);

    continueWindowTest(testSelectSomeScrolled, [library, playlist]);
  }, 1000);
}

function testSelectSomeScrolled(library, playlist) {

  log("testSelectSomeScrolled");

  safeSetTimeout(function() {

    assertTrue(playlist.tree.view.selection.isSelected(0));

    endWindowTest();
  }, 1000);
}

function createPropertyArray() {
  return Cc["@getnightingale.com/Nightingale/Properties/MutablePropertyArray;1"]
           .createInstance(Ci.sbIMutablePropertyArray);
}

function assertAllVisibleRowsSelected(tree) {
  var selection = tree.view.selection;
  var box = tree.boxObject;

  // getLastVisibleRow does not seem to be inclusive?
  for (var i = box.getFirstVisibleRow(); i < box.getLastVisibleRow(); i++) {
    assertTrue(selection.isSelected(i));
  }
}
