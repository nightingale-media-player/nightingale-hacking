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

  var SB_NS = "http://songbirdnest.com/data/1.0#";
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

  var databaseGUID = "test_localdatabaselibrary";
  var library = createLibrary(databaseGUID);

  // test create
  var uriSpec = "file:///foo";
  var uri = ios.newURI(uriSpec, null, null);

  var item1 = library.createMediaItem(uri);
  var now = new Date();
  assertEqual(item1.getProperty(SB_NS + "contentUrl"), uriSpec);

  var created = new Date(parseInt(item1.getProperty(SB_NS + "created")));
  var updated = new Date(parseInt(item1.getProperty(SB_NS + "updated")));

  assertEqual(created.getTime() == updated.getTime(), true);
  // Compare the dates with a little slack -- they should be very close
  assertEqual(now - created < 5000, true);
  assertEqual(now - updated < 5000, true);

  // Make sure we are getting different guids
  var item2 = library.createMediaItem(uri);
  assertNotEqual(item1.guid, item2.guid);

  // Test that they items were added to the library view the view list
  var view = library.getMediaItem("songbird:view");
  assertEqual(view.getItemByGuid(item1.guid).guid, item1.guid);
  assertEqual(view.getItemByGuid(item2.guid).guid, item2.guid);

}

