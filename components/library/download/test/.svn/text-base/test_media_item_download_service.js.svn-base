/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
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
 * \file  test_media_item_download_service.js
 * \brief Javascript source for the media item download service unit tests.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Media item download service unit tests.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Run the unit tests.
 */

function runTest() {
  // Start running the tests.
  testMediaItemDownloadService.start();
}


/**
 * Media item download service tests.
 */

let testMediaItemDownloadService = {
  //
  // Media item download service tests configuration.
  //
  //   succeedingURISpec        A URI spec for a file that should succeed at
  //                            downloading.  This spec must be relative to the
  //                            test harness directory.
  //   failingURISpec           A URI spec for a file that should fail at
  //                            downloading.  This spec must be relative to the
  //                            test harness directory.
  //   noDownloaderURISpec      An absolute URI spec that should not have a
  //                            matching downloader.
  //

  succeedingURISpec: "test1.mp3",
  failingURISpec: "nonexistent_file",
  noDownloaderURISpec: "about:config",


  //
  // Media item download service tests fields.
  //
  //   _testList                List of tests to run.
  //   _nextTestIndex           Index of next test to run.
  //   _testPendingIndicated    True if a pending test has been indicated.
  //   _allTestsComplete        True if all tests have completed.
  //   _server                  HTTP server.
  //   _serverPort              HTTP server port.
  //

  _testList: null,
  _nextTestIndex: 0,
  _testPendingIndicated: false,
  _allTestsComplete: false,
  _server: null,
  _serverPort: -1,


  /**
   * Start the tests.
   */

  start: function testMediaItemDownloadService_start() {
    let self = this;

    // Initialize the list of tests.
    this._testList = [];
    this._testList.push(function() { return self._testDownload(); });
    this._testList.push(function() { return self._testDownloadFailure(); });
    this._testList.push(function()
                          { return self._testNoMatchingDownloader(); });

    // Set up a test server.
    this._serverPort = getTestServerPortNumber();
    this._server = Cc["@mozilla.org/server/jshttp;1"]
                     .createInstance(Ci.nsIHttpServer);
    this._server.start(this._serverPort);
    this._server.registerDirectory("/", getTestFile("."));

    // Start running the tests.
    this._nextTestIndex = 0;
    this._runNextTest();
  },


  /**
   * Finish the tests.
   */

  _finish: function testMediaItemDownloadService__finish() {
    // Mark all tests complete.
    this._allTestsComplete = true;

    // Indicate that the test has finished if a pending test has been indicated.
    if (this._testPendingIndicated) {
      testFinished();
    }

    // Stop test server.
    this._server.stop(function() {});
  },


  /**
   * Run the next test.
   */

  _runNextTest: function testMediaItemDownloadService__runNextTest() {
    // Run tests until complete or test is pending.
    while (1) {
      // Finish test if no more tests to run.
      if (this._nextTestIndex >= this._testList.length) {
        this._finish();
        break;
      }

      // Run the next test.  Indicate if tests are pending.
      if (!this._testList[this._nextTestIndex++]()) {
        // If not all tests have completed and a pending test has not been
        // indicated, indicate a pending test.
        if (!this._allTestsComplete && !this._testPendingIndicated) {
          this._testPendingIndicated = true;
          testPending();
        }
        break;
      }
    }
  },


  /**
   * Test download.
   */

  _testDownload: function testMediaItemDownloadService__testDownload() {
    // Get the media item download service.
    let mediaItemDownloadService =
          Cc["@songbirdnest.com/Songbird/MediaItemDownloadService;1"]
            .getService(Ci.sbIMediaItemDownloadService);

    // Create a test media item to download.
    let library = createLibrary("test_mediaitemdownload", null, false);
    let uri = newURI("http://localhost:" + this._serverPort +
                     "/" + this.succeedingURISpec);
    let mediaItem = library.createMediaItem(uri);

    // Get a downloader for the media item.
    let downloader;
    try {
      downloader = mediaItemDownloadService.getDownloader(mediaItem, null);
    } catch(ex) {
      doFail("Could not get a media item downloader: " + ex);
    }

    // Get the download size.
    let downloadSize = downloader.getDownloadSize(mediaItem, null);
    assertEqual(downloadSize, 17392);

    // Create download job.
    let downloadJob;
    try {
      downloadJob = downloader.createDownloadJob(mediaItem, null);
    } catch(ex) {
      doFail("Could not create media item download job: " + ex);
    }

    // Add a download job progress listener.
    let tester = this;
    let listener = {
      onJobProgress: function() {
        if ((downloadJob.status == Ci.sbIJobProgress.STATUS_FAILED) ||
            (downloadJob.status == Ci.sbIJobProgress.STATUS_SUCCEEDED)) {
          assertEqual(downloadJob.status, Ci.sbIJobProgress.STATUS_SUCCEEDED);
          assertEqual(downloadJob.downloadedFile.fileSize, downloadSize);
          downloadJob.removeJobProgressListener(this);
          tester._runNextTest();
        }
      }
    };
    downloadJob.addJobProgressListener(listener);

    // Start downloading.
    try {
      downloadJob.start();
    } catch(ex) {
      doFail("Could not start media item download: " + ex);
    }

    // Test is pending.
    return false;
  },


  /**
   * Test download failure.
   */

  _testDownloadFailure:
    function testMediaItemDownloadService__testDownloadFailure() {
    // Get the media item download service.
    let mediaItemDownloadService =
          Cc["@songbirdnest.com/Songbird/MediaItemDownloadService;1"]
            .getService(Ci.sbIMediaItemDownloadService);

    // Create a test media item to download.
    let library = createLibrary("test_mediaitemdownload", null, false);
    let uri = newURI("http://localhost:" + this._serverPort +
                     "/" + this.failingURISpec);
    let mediaItem = library.createMediaItem(uri);

    // Get a downloader for the media item.
    let downloader;
    try {
      downloader = mediaItemDownloadService.getDownloader(mediaItem, null);
    } catch(ex) {
      doFail("Could not get a media item downloader: " + ex);
    }

    // Create download job.
    let downloadJob;
    try {
      downloadJob = downloader.createDownloadJob(mediaItem, null);
    } catch(ex) {
      doFail("Could not create media item download job: " + ex);
    }

    // Add a download job progress listener.  Test that the download fails.
    let tester = this;
    let listener = {
      onJobProgress: function() {
        if ((downloadJob.status == Ci.sbIJobProgress.STATUS_FAILED) ||
            (downloadJob.status == Ci.sbIJobProgress.STATUS_SUCCEEDED)) {
          assertEqual(downloadJob.status, Ci.sbIJobProgress.STATUS_FAILED);
          downloadJob.removeJobProgressListener(this);
          tester._runNextTest();
        }
      }
    };
    downloadJob.addJobProgressListener(listener);

    // Start downloading.
    try {
      downloadJob.start();
    } catch(ex) {
      doFail("Could not start media item download: " + ex);
    }

    // Test is pending.
    return false;
  },


  /**
   * Test no matching downloader.
   */

  _testNoMatchingDownloader:
    function testMediaItemDownloadService__testNoMatchingDownloader() {
    // Get the media item download service.
    let mediaItemDownloadService =
          Cc["@songbirdnest.com/Songbird/MediaItemDownloadService;1"]
            .getService(Ci.sbIMediaItemDownloadService);

    // Create a test media item to download.
    let library = createLibrary("test_mediaitemdownload", null, false);
    let uri = newURI(this.noDownloaderURISpec);
    let mediaItem = library.createMediaItem(uri);

    // Get a downloader for the media item.  Test that no matching downloaders
    // were found.
    let downloader;
    try {
      downloader = mediaItemDownloadService.getDownloader(mediaItem, null);
    } catch(ex) {
      doFail("Get media item downloader exception: " + ex);
    }
    assertTrue(!downloader);

    // Test is not pending.
    return true;
  }
};

