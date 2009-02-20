/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
 * \brief Test the directory importer service
 */

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");

// This test relies on a tree of sample files
var gTestFiles = newAppRelativeFile("testharness/watchfolders/files/moverename");

/**
 * TODO
 */
function runTest () {
  /*
  Unit Test:

  Clear library
  Create temp folder
  Set watch folder on temp
  Copy test tree into watch folder
  Wait 5 seconds
  Assert library contains the expected files
  Copy all items into a media list
  Now transform the tree
  Wait 5 seconds
  Assert all items in the media list still point to files that exist
  Assert GUIDs are associated with the expected paths
  Shutdown watchfolders
  
  // Start with an empty library
  var library = LibraryUtils.mainLibrary;
  library.clear();
  
  */  
}
