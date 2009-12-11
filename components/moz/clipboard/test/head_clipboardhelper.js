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
 * \brief Some globally useful stuff for the local database library tests
 */

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

function getPlatform() {
  var platform;
  try {
    var sysInfo =
      Components.classes["@mozilla.org/system-info;1"]
                .getService(Components.interfaces.nsIPropertyBag2);
    platform = sysInfo.getProperty("name");
  }
  catch (e) {
    dump("System-info not available, trying the user agent string.\n");
    var user_agent = navigator.userAgent;
    if (user_agent.indexOf("Windows") != -1)
      platform = "Windows_NT";
    else if (user_agent.indexOf("Mac OS X") != -1)
      platform = "Darwin";
    else if (user_agent.indexOf("Linux") != -1)
      platform = "Linux";
    else if (user_agent.indexOf("SunOS") != -1)
      platform = "SunOS";
  }
  return platform;
}

/**
 * Copy the given folder to tempName, returning an nsIFile
 * for the new location
 */
function getCopyOfFolder(folder, tempName) {
  assertNotEqual(folder, null);
  var tempFolder = getTempFolder();
  folder.copyTo(tempFolder, tempName);
  folder = tempFolder.clone();
  folder.append(tempName);
  assertEqual(folder.exists(), true);
  return folder;
}


/**
 * Copy the given folder to tempName, returning an nsIFile
 * for the new location
 */
function getCopyOfFile(file, tempName, optionalLocation) {
  assertNotEqual(file, null);
  var folder = optionalLocation ? optionalLocation : getTempFolder();
  file.copyTo(folder, tempName);
  file = folder.clone();
  file.append(tempName);
  assertEqual(file.exists(), true);
  return file;
}

/**
 * Get a temporary folder for use in clipboard tests.
 * Will be removed in tail_clipboardhelper.js
 */
var gTempFolder = null;
function getTempFolder() {
  if (gTempFolder) {
    return gTempFolder;
  }
  gTempFolder = Components.classes["@mozilla.org/file/directory_service;1"]
                       .getService(Components.interfaces.nsIProperties)
                       .get("TmpD", Components.interfaces.nsIFile);
  gTempFolder.append("songbird_clipboardhelper_tests.tmp");
  gTempFolder.createUnique(Components.interfaces.nsIFile.DIRECTORY_TYPE, 0777);
  return gTempFolder;
}


/**
 * Get rid of the temp folder created by getTempFolder.
 * Called in tail_metadatamanager.js
 */
function removeTempFolder() {
  if (gTempFolder && gTempFolder.exists()) {
    gTempFolder.remove(true);
  } else {
    log("\n\n\nClipboard Helper Test may not have performed cleanup.  Temp files may exist.\n\n\n");
  }
}
