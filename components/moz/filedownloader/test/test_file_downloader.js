/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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
 * \file  test_file_downloader.js
 * \brief Javascript source for the file downloader unit tests.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// File downloader service unit tests.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Run the unit tests.
 */

function runTest() {
  // Start running the tests.
  testFileDownloader.start();
}


/**
 * File downloader tests
 */

var testFileDownloader = {
  //
  // File downloader tests configuration.
  //
  //   succeedingURISpec        A URI spec for a file that should succeed at
  //                            downloading.
  //   failingURISpec           A URI spec for a file that should fail at
  //                            downloading.
  //   destinationFileExt       Destination file extension.
  //

  succeedingURISpec: "chrome://songbird/locale/songbird.properties",
  failingURISpec: "http://nonexistent_file",
  destinationFileExt: "tst",


  //
  // File downloader tests fields.
  //
  //   _testList                List of tests to run.
  //   _nextTestIndex           Index of next test to run.
  //   _testPendingIndicated    True if a pending test has been indicated.
  //   _allTestsComplete        True if all tests have completed.
  //

  _testList: null,
  _nextTestIndex: 0,
  _testPendingIndicated: false,
  _allTestsComplete: false,


  /**
   * Start the tests.
   */

  start: function testFileDownloader_start() {
    var _this = this;
    var func;

    // Initialize the list of tests.
    this._testList = [];
    func = function() { return _this._testExistence(); };
    this._testList.push(func);
    func = function() { return _this._testDownloadSucceeds(); };
    this._testList.push(func);
    func = function() { return _this._testDownloadWithURISpec(); };
    this._testList.push(func);
    func = function() { return _this._testDownloadWithDstExt(); };
    this._testList.push(func);
    func = function() { return _this._testDownloadFails(); };
    this._testList.push(func);
    func = function() { return _this._testDownloadCancel(); };
    this._testList.push(func);

    // Start running the tests.
    this._nextTestIndex = 0;
    this._runNextTest();
  },


  /**
   * Run the next test.
   */

  _runNextTest: function testFileDownloader__runNextTest() {
    // Run tests until complete or test is pending.
    while (1) {
      // Finish test if no more tests to run.
      if (this._nextTestIndex >= this._testList.length) {
        this._allTestsComplete = true;
        if (this._testPendingIndicated) {
          testFinished();
        }
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
   * Test existence of the file downloader component.
   */

  _testExistence: function testFileDownloader__testExistence() {
    // Test that the file downloader component is available.
    var fileDownloader;
    try {
      fileDownloader = Cc["@songbirdnest.com/Songbird/FileDownloader;1"]
                         .createInstance(Ci.sbIFileDownloader);
    } catch (ex) {}
    assertTrue(fileDownloader,
               "File downloader component is not available.");

    // Test is complete.
    return true;
  },


  /**
   * Test succeeding file download.
   */

  _testDownloadSucceeds: function testFileDownloader__testDownloadSucceeds() {
    // Create a file downloader.
    var fileDownloader = Cc["@songbirdnest.com/Songbird/FileDownloader;1"]
                           .createInstance(Ci.sbIFileDownloader);

    // Create a file download listener.
    var tester = this;
    var fileDownloaderListener = {
      onProgress: function() {},

      onComplete: function() {
        // Validate success.
        assertTrue(fileDownloader.succeeded, "File download failed.");
        assertTrue(fileDownloader.destinationFile,
                   "Destination file not available.");
        assertTrue(fileDownloader.destinationFile.exists(),
                   "Destination file does not exist.");
        assertEqual(fileDownloader.bytesToDownload,
                    fileDownloader.bytesDownloaded);
        assertEqual(fileDownloader.percentComplete, 100);

        // Release file downloader.
        fileDownloader.listener = null;
        fileDownloader = null;

        // Run the next test.
        tester._runNextTest();
      }
    };

    // Create the source URI.
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var sourceURI = ioService.newURI(this.succeedingURISpec, null, null);

    // Set up the file downloader.
    fileDownloader.sourceURI = sourceURI;
    fileDownloader.listener = fileDownloaderListener;

    // Start the file download.
    fileDownloader.start();

    // Test is not complete.
    return false;
  },


  /**
   * Test file download using source URI spec.
   */

  _testDownloadWithURISpec:
    function testFileDownloader__testDownloadWithURISpec() {
    // Create a file downloader.
    var fileDownloader = Cc["@songbirdnest.com/Songbird/FileDownloader;1"]
                           .createInstance(Ci.sbIFileDownloader);

    // Create a file download listener.
    var tester = this;
    var fileDownloaderListener = {
      onProgress: function() {},

      onComplete: function() {
        // Validate success.
        assertTrue(fileDownloader.succeeded, "File download failed.");

        // Release file downloader.
        fileDownloader.listener = null;
        fileDownloader = null;

        // Run the next test.
        tester._runNextTest();
      }
    };

    // Set up the file downloader.
    fileDownloader.sourceURISpec = this.succeedingURISpec;
    fileDownloader.listener = fileDownloaderListener;

    // Start the file download.
    fileDownloader.start();

    // Test is not complete.
    return false;
  },


  /**
   * Test file download with the destination file extension specified.
   */

  _testDownloadWithDstExt:
    function testFileDownloader__testDownloadWithDstExt() {
    // Create a file downloader.
    var fileDownloader = Cc["@songbirdnest.com/Songbird/FileDownloader;1"]
                           .createInstance(Ci.sbIFileDownloader);

    // Create a file download listener.
    var tester = this;
    var fileDownloaderListener = {
      onProgress: function() {},

      onComplete: function() {
        // Validate success.
        assertTrue(fileDownloader.succeeded, "File download failed.");

        // Get destination file extension.
        var ioService = Cc["@mozilla.org/network/io-service;1"]
                          .getService(Ci.nsIIOService);
        var fileProtocolHandler = ioService.getProtocolHandler("file")
                                    .QueryInterface(Ci.nsIFileProtocolHandler);
        var destinationFileURL =
              fileProtocolHandler.newFileURI(fileDownloader.destinationFile);
        destinationFileURL = destinationFileURL.QueryInterface(Ci.nsIURL);

        // Validate file extension.
        assertEqual(destinationFileURL.fileExtension,
                    tester.destinationFileExt);

        // Release file downloader.
        fileDownloader.listener = null;
        fileDownloader = null;

        // Run the next test.
        tester._runNextTest();
      }
    };

    // Set up the file downloader.
    fileDownloader.sourceURISpec = this.succeedingURISpec;
    fileDownloader.listener = fileDownloaderListener;
    fileDownloader.destinationFileExtension = this.destinationFileExt;

    // Start the file download.
    fileDownloader.start();

    // Test is not complete.
    return false;
  },


  /**
   * Test failing file download.
   */

  _testDownloadFails: function testFileDownloader__testDownloadFails() {
    // Create a file downloader.
    var fileDownloader = Cc["@songbirdnest.com/Songbird/FileDownloader;1"]
                           .createInstance(Ci.sbIFileDownloader);

    // Create a file download listener.
    var tester = this;
    var fileDownloaderListener = {
      onProgress: function() {},

      onComplete: function() {
        // Validate failure.
        assertTrue(!fileDownloader.succeeded, "File download succeeded.");

        // Release file downloader.
        fileDownloader.listener = null;
        fileDownloader = null;

        // Run the next test.
        tester._runNextTest();
      }
    };

    // Create the source URI.
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var sourceURI = ioService.newURI(this.failingURISpec, null, null);

    // Set up the file downloader.
    fileDownloader.sourceURI = sourceURI;
    fileDownloader.listener = fileDownloaderListener;

    // Start the file download.
    fileDownloader.start();

    // Test is not complete.
    return false;
  },


  /**
   * Test cancelling file download.
   */

  _testDownloadCancel: function testFileDownloader__testDownloadCancel() {
    // Create a file downloader.
    var fileDownloader = Cc["@songbirdnest.com/Songbird/FileDownloader;1"]
                           .createInstance(Ci.sbIFileDownloader);

    // Create a file download listener.
    var tester = this;
    var fileDownloaderListener = {
      onProgress: function() {},

      onComplete: function() {
        // Validate failure from cancel.
        assertTrue(!fileDownloader.succeeded, "File download succeeded.");
        assertTrue(!fileDownloader.destinationFile,
                   "Destination file not deleted.");

        // Release file downloader.
        fileDownloader.listener = null;
        fileDownloader = null;

        // Run the next test.
        tester._runNextTest();
      }
    };

    // Create the source URI.
    var ioService = Cc["@mozilla.org/network/io-service;1"]
                      .getService(Ci.nsIIOService);
    var sourceURI = ioService.newURI(this.succeedingURISpec, null, null);

    // Set up the file downloader.
    fileDownloader.sourceURI = sourceURI;
    fileDownloader.listener = fileDownloaderListener;

    // Start and cancel the file download.
    fileDownloader.start();
    fileDownloader.cancel();

    // Test is not complete.
    return false;
  }
}

