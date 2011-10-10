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
 * \brief Confirm that sbILibraryResource.userEditable works as expected
 */

function runTest () {
  Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
  
  // Create a library and confirm that it is editable
  var library = createLibrary("usereditable");
  library.clear();
  library.setProperty(SBProperties.isReadOnly, null);
  assertTrue(library.userEditable);
  
  // Create a media list within the library and confirm 
  // that it is also editable by default
  var list = library.createMediaList("simple");
  assertTrue(list.userEditable);
  
  // Set the list read-only and confirm that it is no longer editable
  list.setProperty(SBProperties.isReadOnly, "1");
  assertTrue(!list.userEditable);
  list.setProperty(SBProperties.isReadOnly, null);
  assertTrue(list.userEditable);
  
  // Create a local item in the library and confirm that it is editable
  var file = newTempFile("user-editable-test.mp3");
  var item = library.createMediaItem(newFileURI(file), null, true);
  assertTrue(item.userEditable);
  
  // Set the item read-only and confirm that it is no longer editable
  item.setProperty(SBProperties.isReadOnly, "1");
  assertTrue(!item.userEditable);
  item.setProperty(SBProperties.isReadOnly, null);
  assertTrue(item.userEditable);
  
  // Set the library read-only and confirm that the list, item and library are not editable
  library.setProperty(SBProperties.isReadOnly, "1");
  assertTrue(!library.userEditable);
  assertTrue(!list.userEditable);
  assertTrue(!item.userEditable);
  library.setProperty(SBProperties.isReadOnly, null);
  assertTrue(library.userEditable);
  
  // Set the file for the item to be read only
  // and confirm that it isn't user editable
  assertTrue(item.userEditable);
  file.permissions = 0400;
  assertTrue(!item.userEditable);
  file.permissions = 0664;
  assertTrue(item.userEditable);
  
  // Set the file for the item to be a path that doesn't exist
  // and confirm that it isn't user editable
  file = file.parent;
  file.append("file-that-doesnt-exist.mp3");
  item.contentSrc = newFileURI(file);
  assertTrue(!item.userEditable, "Non-existent files are read-only.");
  
  // Set the item to a remote file and confirm that isn't user editable
  item.contentSrc = newURI("http://example.com/foo.mp3");
  assertTrue(item.userEditable, "Remote files are user editable.");
}


function newTempFile( name ) {
  var file = Components.classes["@mozilla.org/file/directory_service;1"]
                      .getService(Components.interfaces.nsIProperties)
                      .get("TmpD", Components.interfaces.nsIFile);
  file.append(name);
  file.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0664);
  var registerFileForDelete = Cc["@mozilla.org/uriloader/external-helper-app-service;1"]
                                .getService(Ci.nsPIExternalAppLauncher);
  registerFileForDelete.deleteTemporaryFileOnExit(file);
  return file;
}

