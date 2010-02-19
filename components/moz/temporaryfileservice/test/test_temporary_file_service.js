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
 * \file  test_temporary_file_service.js
 * \brief Javascript source for the temporary file service unit tests.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Temporary file service unit tests.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Run the unit tests.
 */

function runTest() {
  // Get the file protocol handler.
  var ioService = Cc["@mozilla.org/network/io-service;1"]
                    .getService(Ci.nsIIOService);
  var fileProtocolHandler = ioService.getProtocolHandler("file")
                              .QueryInterface(Ci.nsIFileProtocolHandler);

  // Test that the temporary file service component is available.
  var temporaryFileService;
  try {
    temporaryFileService =
      Cc["@songbirdnest.com/Songbird/TemporaryFileService;1"]
        .getService(Ci.sbITemporaryFileService);
  } catch (ex) {}
  assertTrue(temporaryFileService,
             "Temporary file service component is not available.");

  // Test existence of temporary file service root directory.
  assertTrue(temporaryFileService.rootTemporaryDirectory,
             "Temporary file service directory is null.");
  assertTrue(temporaryFileService.rootTemporaryDirectory.exists(),
             "Temporary file service directory does not exist.");

  // Create temporary file and test its existence.
  var tmpFile1 = temporaryFileService.createFile(Ci.nsIFile.NORMAL_FILE_TYPE);
  assertTrue(tmpFile1, "Could not create temporary file.");
  assertTrue(tmpFile1.exists(), "Temporary file does not exist.");

  // Create another temporary file and test its uniqueness.
  var tmpFile2 = temporaryFileService.createFile(Ci.nsIFile.NORMAL_FILE_TYPE);
  assertTrue(tmpFile2, "Could not create temporary file.");
  assertTrue(tmpFile2.exists(), "Temporary file does not exist.");
  assertTrue(!tmpFile2.equals(tmpFile1), "Temporary file is not unique.");

  // Create and validate a temporary file with a specified base name.
  var fileBaseName = "test";
  tmpFile1 = temporaryFileService.createFile(Ci.nsIFile.NORMAL_FILE_TYPE,
                                             fileBaseName);
  assertTrue(tmpFile1, "Could not create temporary file.");
  var fileURL = fileProtocolHandler.newFileURI(tmpFile1);
  fileURL = fileURL.QueryInterface(Ci.nsIURL);
  assertEqual(fileURL.fileBaseName.search(fileBaseName), 0);

  // Create and validate a temporary file with a specified extension.
  tmpFile1 = temporaryFileService.createFile(Ci.nsIFile.NORMAL_FILE_TYPE,
                                             null,
                                             "tst");
  assertTrue(tmpFile1, "Could not create temporary file.");
  assertTrue(tmpFile1.path.match(/\.tst$/), "File extension incorrect.");
}

