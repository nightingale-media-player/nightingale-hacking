/** vim: ts=2 sw=2 expandtab ai
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
 * \brief Basic bookmarks pane unit tests
 */


function runTest () {
  var BMS = Components.classes['@songbirdnest.com/servicepane/bookmarks;1'].getService(Components.interfaces.sbIBookmarks);
  
  // okay, did we get it?
  assertNotEqual(BMS, null);

  // create a bookmark
  var node = BMS.addBookmark('http://www.example.com/bookmark', 
      'My Example Bookmark', 
      'http://www.example.com/favicon.ico');
  assertNotEqual(node, null);

  // is it in the bookmarks folder?
  assertNotEqual(node.parentNode, null);
  assertEqual(node.parentNode.id, 'SB:Bookmarks');

  // are the properties set correctly
  assertEqual(node.id, 'http://www.example.com/bookmark');
  assertEqual(node.url, 'http://www.example.com/bookmark');
  assertEqual(node.name, 'My Example Bookmark');
  assertEqual(node.image, 'http://www.example.com/favicon.ico');

  // can we find it with bookmarkExists?
  assertEqual(BMS.bookmarkExists('http://www.example.com/bookmark'), true);

  // creating another copy should fail
  assertEqual(null, BMS.addBookmark('http://www.example.com/bookmark', 
      'My Example Bookmark', 'http://www.example.com/favicon.ico'));

  // we can delete it
  node.parentNode.removeChild(node);

  // and then it no longer exists
  assertEqual(BMS.bookmarkExists('http://www.example.com/bookmark'), false);

  var SPS = Components.classes['@songbirdnest.com/servicepane/service;1'].getService(Components.interfaces.sbIServicePaneService);
  SPS.save();

  // Do a short sleep to wait for the uri checker to complete.  Without this,
  // we will leak some stuff on shutdown
  sleep(500);
}

