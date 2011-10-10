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
 * \brief Generic support functions for watchfolder tests
 */



Components.utils.import("resource://app/jsmodules/sbProperties.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

var Application = Cc["@mozilla.org/fuel/application;1"]
                     .getService(Ci.fuelIApplication);


/**
 * Start watching the given nsIFile path,
 * or pass null to disable
 */
function setWatchFolder(file) {
  if (file) {
    dump("WF is watching " + file.path + "\n");
    Application.prefs.setValue("nightingale.watch_folder.path", file.path);
  }
  Application.prefs.setValue("nightingale.watch_folder.enable", !!file);
  if (!file) {
    Application.prefs.get("nightingale.watch_folder.sessionguid").reset();
  }
}

/**
 * Dump information for an sbIJobProgress interface.
 * Used for debugging.
 */
function reportJobProgress(job, jobName) {
  log("\n\n\nWatchFolder Job - " + jobName + " job progress - " +
      job.progress + "/" + job.total + ", " + job.errorCount + " failed. " +
      "Status " + job.statusText + " (" + job.status + "), Title: " + job.titleText);
  var errorEnumerator = job.getErrorMessages();
  while (errorEnumerator.hasMore()) {
    log("WatchFolder Job " + jobName + " - error: " + errorEnumerator.getNext());
  }
  log("\n\n");
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
 * Get a temporary folder for use in tests.
 * Will be removed in tail_watchfolders.js
 */
var gTempFolder = null;
function getTempFolder() {
  if (gTempFolder) {
    return gTempFolder;
  }
  gTempFolder = Components.classes["@mozilla.org/file/directory_service;1"]
                       .getService(Components.interfaces.nsIProperties)
                       .get("TmpD", Components.interfaces.nsIFile);
  gTempFolder.append("nightingale_watchfolder_tests" + Math.random() + ".tmp");
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
    log("\n\n\nWatch Folder Test may not have performed cleanup.  Temp files may exist.\n\n\n");
  }
}
