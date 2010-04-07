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

var gFileMetadataService;
var count = 0;
var t = null;

function runTest () {

  var ITEMS_PER_JOB = 200;

  var library = createNewLibrary("test_metadatajob_performance");

  gFileMetadataService = Components.classes["@songbirdnest.com/Songbird/FileMetadataService;1"]
                                .getService(Components.interfaces.sbIFileMetadataService);

  var scan = Cc["@songbirdnest.com/Songbird/FileScan;1"]
               .createInstance(Ci.sbIFileScan);

  var query = Cc["@songbirdnest.com/Songbird/FileScanQuery;1"]
               .createInstance(Ci.sbIFileScanQuery);
  query.setDirectory("/media/sdb1/steve/old/steve/fakemp3s");
  query.setRecurse(true);

  query.addFileExtension("mp3");

  scan.submitQuery(query);

  log("Scanning...");

  while (query.isScanning()) {
    sleep(1000);
  }
  var urls = query.getResultRangeAsURIStrings(0, query.getFileCount() - 1);

  fileScan.finalize();
  
  log("Creating " + urls.length + " items...");
  var items = library.batchCreateMediaItems(urls, null, true);

  t = Date.now();

  var jobs = items.length / ITEMS_PER_JOB;
  log("jobs = " + jobs);
  var index = 0;
  for (var i = 0; i < jobs; i++) {
    var a = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"].createInstance(Ci.nsIMutableArray);
    for (var j = 0; j < ITEMS_PER_JOB; j++) {
      if (index >= items.length) {
        break;
      }
      a.appendElement(items.queryElementAt(index, Ci.sbIMediaItem), false);
      index++;
    }

    var job = gFileMetadataService.read(a);
    job.addJobProgressListener(onComplete);
    count++;
    log("length = " + a.length + " count = " + count);
  }
  testPending();

}

function onComplete(job) {
  if (job.status == Components.interfaces.sbIJobProgress.STATUS_RUNNING) {
    return;
  }

  job.removeJobProgressListener(onComplete);
  log("done = " + count);
  count--;
  if (count == 0) {
    var time = Date.now() - t;
    log("Metadata scan finished in " + (time / 1000) + "s");
    testFinished(); // Complete the testing
  }
}
