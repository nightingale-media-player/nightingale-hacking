/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2010 POTI, Inc.
 * http://www.songbirdnest.com
 *
 * This file may be licensed under the terms of of the
 * GNU General Public License Version 2 (the ``GPL'').
 *
 * Software distributed under the License is distributed
 * on an ``AS IS'' basis, WITHOUT WARRANTY OF ANY KIND, either
 * express or implied. See the GPL for the specific language
 * governing rights and limitations.
 *
 * You should have received a copy of the GPL along with this
 * program. If not, go to http://www.gnu.org/licenses/gpl.html
 * or write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *=END SONGBIRD GPL
 */

/**
 * \brief Test file for bug 22806 - songbird hangs if the metadata manager
 * attempts to rescan a file that is in the blacklist
 */

Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
Components.utils.import("resource://app/jsmodules/sbLibraryUtils.jsm");
Components.utils.import("resource://app/jsmodules/URLUtils.jsm");

function runTest() {
  // set up the server
  var port = getTestServerPortNumber();

  var server = Cc["@mozilla.org/server/jshttp;1"]
                 .createInstance(Ci.nsIHttpServer);
  
  server.start(port);
  server.registerDirectory("/", getTempFolder());
  
  // try to run the test, but make sure to at least always shut down the server
  try {
    var lib = createLibrary("bug22806_blacklist_deadlock");
    lib.clear();
    runTestInternal(lib);
  }
  finally {
    server.stop(function() void(0));
    lib.clear();
  }
}

/**
 * The actual test function
 * This is here to make try try/finally above clearer.
 */
function runTestInternal(library) {
  var libUtils = LibraryUtils.manager.QueryInterface(Ci.sbILibraryUtils);
  var fileMetaSvc = Cc["@songbirdnest.com/Songbird/FileMetadataService;1"]
                      .getService(Ci.sbIFileMetadataService)
                      .QueryInterface(Ci.sbPIFileMetadataService);
  const URL_PREFIX = "http://localhost:" + getTestServerPortNumber() + "/";
  const JOBS_PER_GROUP = 12;
  var items = [];
  var filename, i, job;
  var jobs = 0;  // number of outstanding jobs (we'll have up to 2 of them)

  /* job progress callback to know when the test is done */
  function onJobProgress(job) {
    reportJobProgress(job, "bug22806_blacklist_deadlock - onJobProgress");
    if (job.status == Ci.sbIJobProgress.STATUS_RUNNING) {
      return;
    }
    job.removeJobProgressListener(onJobProgress);
    if (--jobs < 1) {
      testFinished();
    }
  }

  // first, spin up a collection of local (background thread) and remote (main
  // thread) metadata reads, to make sure both thread have pending jobs
  for (i = 0; i < JOBS_PER_GROUP; ++i) {
    filename = "bug22806_blacklist_good_" + i + ".mp3";
    file = getCopyOfFile(newAppRelativeFile(
             "testharness/metadatamanager/files/MP3_NoTags.mp3"),
             filename);
    if (i % 2) {
      url = libUtils.getFileContentURI(file);
    }
    else {
      url = libUtils.getContentURI(URLUtils.newURI(URL_PREFIX + filename));
    }
    items.push(library.createMediaItem(url));
  }
  job = fileMetaSvc.read(ArrayConverter.nsIArray(items));
  job.addJobProgressListener(onJobProgress);
  ++jobs;

  // next, spin up a bunch of blacklisted remote (main thread) reads, so that
  // it will need to attempt to get into the undesired state
  items = [];
  for (i = 0; i < JOBS_PER_GROUP; ++i) {
    filename = "bug22806_blacklist_blacklisted_" + i + ".mp3";
    file = getCopyOfFile(newAppRelativeFile(
             "testharness/metadatamanager/files/MP3_NoTags.mp3"),
             filename);
    url = libUtils.getContentURI(URLUtils.newURI(URL_PREFIX + filename));
    fileMetaSvc.AddBlacklistURL(url.spec);
    items.push(library.createMediaItem(url));
  }
  job = fileMetaSvc.read(ArrayConverter.nsIArray(items));
  job.addJobProgressListener(onJobProgress);
  ++jobs;

  // wait for both jobs to finish
  testPending();
}

