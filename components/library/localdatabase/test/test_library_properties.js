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

  Components.utils.import("resource://app/components/sbProperties.jsm");

  var library = createLibrary("test_library_properties", null, false);
  library.clear();

  assertTrue(library.created > 0);
  assertTrue(library.updated > 0);
  assertTrue(library.contentSrc.schemeIs("file"));
  assertEqual(library.getProperty(SBProperties.hidden), "0");

  var uniqueString = (new Date()).getTime();
  library.setProperty(SBProperties.contentMimeType, uniqueString);
  assertEqual(library.getProperty(SBProperties.contentMimeType), uniqueString);
  library.setProperty(SBProperties.albumName, uniqueString);
  assertEqual(library.getProperty(SBProperties.albumName), uniqueString);

  // Library properties should survive a clear
  library.clear();

  library.sync();
  library = null;

  // file copy the database file so we can open it again
  var directory = Cc["@mozilla.org/file/directory_service;1"].
                  getService(Ci.nsIProperties).
                  get("ProfD", Ci.nsIFile);
  directory.append("db");

  var dest = directory.clone();
  dest.append("test_library_properties2.db");
  if (dest.exists()) {
    dest.remove(false);
  }

  var src = directory.clone();
  src.append("test_library_properties.db");
  src.copyTo(directory, dest.leafName);

  var library2 = createLibrary("test_library_properties2", null, false);
  assertTrue(library2.created > 0);
  assertTrue(library2.updated > 0);
  assertTrue(library2.contentSrc.schemeIs("file"));
  assertEqual(library2.getProperty(SBProperties.hidden), "0");
  assertEqual(library2.getProperty(SBProperties.contentMimeType), uniqueString);
  assertEqual(library2.getProperty(SBProperties.albumName), uniqueString);
}

