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

  if (typeof(SBProperties) == "undefined") {
    Components.utils.import("resource://app/components/sbProperties.jsm");
    if (!SBProperties)
      throw new Error("Import of sbProperties module failed!");
  }

  this.props = {};
  for ( var stuff in SBProperties ) {
    if ( typeof(SBProperties[stuff]) == "string" &&
         stuff != "base" &&
         stuff != "_base" )
    {
      this.props[stuff] = SBProperties[stuff];
    }
  }

  setAllAccess();

  var libraryManager = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                          .getService(Ci.sbILibraryManager);
  libraryManager.mainLibrary.clear();

  beginRemoteAPITest("test_properties_page.html", startTesting);
}

function startTesting() {

  testBrowserWindow.runPageTest(this);

}
