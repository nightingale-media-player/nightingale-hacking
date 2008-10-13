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

var gWebLib;
var gWebItem1;
var gWebItem2;
var gWebItem3;
var gWebItem4;
var gWebItemGUID1;
var gWebItemGUID2;
var gWebItemGUID3;
var gWebItemGUID4;
var gLibraryManager;
var foobar;
var gServerPort = getTestServerPortNumber();
var gPrefix = "http://localhost:" + gServerPort + "/";

function runTest () {
  setAllAccess();

  // have to do this here to avoid the security checks
  gLibraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                          .getService(Ci.sbILibraryManager);
  gLibraryManager.mainLibrary.clear();

  // get the web library - sigh
  foobar = Cc["@mozilla.org/preferences-service;1"]
      .getService(Ci.nsIPrefBranch2);

  var webListGUID = foobar.getComplexValue( "songbird.library.web",
                                            Ci.nsISupportsString );
  gWebLib = gLibraryManager.getLibrary(webListGUID);
  gWebLib.clear();
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);

  var item1URI = ioService.newURI(gPrefix + "test13.mp3", null, null);
  var item2URI = ioService.newURI(gPrefix + "test14.mp3", null, null);
  var item3URI = ioService.newURI(gPrefix + "test15.mp3", null, null);
  var item4URI = ioService.newURI(gPrefix + "test16.mp3", null, null);

  gWebItem1 = gWebLib.createMediaItem(item1URI);
  gWebItem2 = gWebLib.createMediaItem(item2URI);
  gWebItem3 = gWebLib.createMediaItem(item3URI);
  gWebItem4 = gWebLib.createMediaItem(item4URI);

  gWebItemGUID1 = gWebItem1.getProperty("http://songbirdnest.com/data/1.0#GUID");
  gWebItemGUID2 = gWebItem1.getProperty("http://songbirdnest.com/data/1.0#GUID");
  gWebItemGUID3 = gWebItem1.getProperty("http://songbirdnest.com/data/1.0#GUID");
  gWebItemGUID4 = gWebItem1.getProperty("http://songbirdnest.com/data/1.0#GUID");


  beginRemoteAPITest("test_remotemedialist_add2_page.html", startTesting);
}

function startTesting() {

  testBrowserWindow.runPageTest(this);
}

