/* vim: set sw=2 :miv */
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
 * \brief Device tests - Device utilities
 */

Components.utils.import("resource://app/components/sbProperties.jsm");
if (typeof(IO) == "undefined") {
  var IO = Components.classes["@mozilla.org/io/scriptable-io;1"].getService();
}

function sbIDeviceDeviceTesterUtils_GetOrganizedPath(aUtils, aLibrary) {
  var file = IO.getFile("TmpD", ".");
  var spec = "http://0/not/a/valid/file.mp3";
  var uri = IO.newURI(spec);
  var item = aLibrary.createMediaItem(uri);

  var artist = "Some Artist *", album = "Some Album /";
  
  item.setProperty(SBProperties.artistName, artist);
  item.setProperty(SBProperties.albumName, album)

  var result = aUtils.GetOrganizedPath(file, item);
  
  var platform = Components.classes["@mozilla.org/xre/app-info;1"]
                           .getService(Components.interfaces.nsIXULRuntime)
                           .OS;
  var badchars;
  if (platform == "WINNT")
    badchars = /[\/:\*\?\\\"<>\|]/g;
  else if (platform == "Darwin")
    badchars = /[:\/]/g;
  else
    badchars = /[\/]/g;
  artist = artist.replace(badchars, '_');
  album = album.replace(badchars, '_');
  
  assertEqual(result.leafName, "file.mp3");
  assertEqual(result.parent.leafName, album);
  assertEqual(result.parent.parent.leafName, artist);
  assertTrue(result.parent.parent.parent.equals(IO.getFile("TmpD", ".")));
}

function runTest () {
  var utils = Components.classes["@songbirdnest.com/Songbird/Device/DeviceTester/Utils;1"]
                        .createInstance(Components.interfaces.sbIDeviceDeviceTesterUtils);

  var libraryFile = IO.getFile("ProfD", "db");
  libraryFile.append("test_devicedevice.db");
  var libraryFactory =
    Components.classes["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"]
              .getService(Components.interfaces.sbILibraryFactory);
  var hashBag = Components.classes["@mozilla.org/hash-property-bag;1"]
                          .createInstance(Components.interfaces.nsIWritablePropertyBag2);
  hashBag.setPropertyAsInterface("databaseFile", libraryFile);
  var library = libraryFactory.createLibrary(hashBag);
  library.clear();
  
  sbIDeviceDeviceTesterUtils_GetOrganizedPath(utils, library);
}
