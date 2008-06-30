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
 * \brief Test the directory importer service
 */

function runTest () {
  // TODO
  /*
  var directories = TODO;
  var directoryImporter = Cc['@songbirdnest.com/songbird/feathersmanager;1']
                            .getService(Ci.sbIDirectoryImportService);

  var job = directoryImporter.import(directories);
  
  job.addJobProgressListener(onComplete);
  testPending();
  */
}


function onComplete(job) {
  try {
    if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
      return;
    }
    
    job.removeJobProgressListener(onComplete);
    
    // Confirm that we found the expected number of items, dupes, etc.

  } catch (e) {
    log("Error: " + e);
    assertEqual(true, false);
  }
  testFinished();
}