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

// declared globally so it can be removed in the tail_ file
var dest;

function runTest () {
  setRapiPref("playback_control_disable", false);
  setRapiPref("playback_read_disable", false);
  setRapiPref("library_read_disable", false);
  setRapiPref("library_write_disable", false);
  setRapiPref("library_create_disable", false);

  dest = Cc["@mozilla.org/file/directory_service;1"]
             .getService(Ci.nsIProperties)
             .get("TmpD", Ci.nsIFile);

  dest.append("remoteplayer_test");

  var drCtor = new Components.Constructor("@songbirdnest.com/Songbird/DataRemote;1", "sbIDataRemote", "init");
  var dlFolder = new drCtor("download.folder", null);
  var dlAlways = new drCtor("download.always", null);
  dlFolder.stringValue = dest.path;
  dlAlways.boolValue = true;

  beginRemoteAPITest("test_remoteplayer_page.html", startTesting);

}

function startTesting() {

  testBrowserWindow.runPageTest(this);
}

