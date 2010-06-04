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
 * \brief Test reading from files with unusual file names
 */

var gFileMetadataService;

function runTest () {
  gFileMetadataService = Components.classes["@songbirdnest.com/Songbird/MetadataManager;1"]
                                .getService(Components.interfaces.sbIMetadataManager);
  
  // the name to use, per-platform (or "*" for unknown)
  // items are arrays of <file name>, <escaped string>
  var filenames = {
    "WINNT":   ["!@#$%^&(),_+ test\u4E2D\u6587.mp3",
                "!@%23$%25%5e&(),_+%20test%e4%b8%ad%e6%96%87.mp3"],
    "Darwin":  ["!@#$%^&(),_+ test\u4E2D\u6587.mp3",
                "!@%23$%25%5e&(),_+%20test%e4%b8%ad%e6%96%87.mp3"],
    "*":       ["!@#$%^&(),_+ test\u4E2D\n\u6587.mp3",
                "!@%23$%25%5e&(),_+%20test%e4%b8%ad%0a%e6%96%87.mp3"]
  };
  
  var platform = Components.classes["@mozilla.org/xre/runtime;1"]
                           .getService(Components.interfaces.nsIXULRuntime)
                           .OS;
  if (!(platform in filenames)) {
    platform = "*";
  }
  
  var file = newAppRelativeFile("testharness/metadatamanager/files/MP3_ID3v23.mp3");
  file = getCopyOfFile(file, filenames[platform][0]);
  var fileURL = newFileURI(file.parent).spec + "/" +  filenames[platform][1];
  var handler = gFileMetadataService.getHandlerForMediaURL(fileURL);
  
  assertNotEqual(handler, null);
  
  // NOTE: Does not accommodate async reading
  var itemsRead = handler.read();
  // Make sure we got at least a couple properties
  assertEqual(itemsRead > 5, true);
  assertEqual(handler.completed, true);
  
  // Verify that initially all properties are X
  var expectedProperties = {};
  expectedProperties[SBProperties.artistName] = "Songbird";
  expectedProperties[SBProperties.albumName] = "Unit Test Classics";
  expectedProperties[SBProperties.trackName] = "Sample";
  
  assertObjectIsSubsetOf(expectedProperties, SBProperties.arrayToJSObject(handler.props));
  
  
  // Write out new values
  handler.props.clear();
  var newProperties = {};
  newProperties[SBProperties.albumName] = "New Album";
  newProperties[SBProperties.artistName] = "New Artist";
  newProperties[SBProperties.trackName] = "New Track";
  newProperties[SBProperties.year] = "2008";
  newProperties[SBProperties.trackNumber] = "13";
  SBProperties.addToArray(newProperties, handler.props);
  
  // Write this to disk
  handler.write();
  
  // Get a new handler just to make sure it isn't cheating somehow
  handler.close();
  handler = gFileMetadataService.getHandlerForMediaURL(fileURL);
  handler.read();
  
  // Confirm that writing succeeded.
  assertObjectIsSubsetOf(newProperties, SBProperties.arrayToJSObject(handler.props));  
  
  handler.close();
  file.remove(false);
}

