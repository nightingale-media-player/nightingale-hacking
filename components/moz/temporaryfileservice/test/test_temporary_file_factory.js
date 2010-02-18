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
 * \file  test_temporary_file_factory.js
 * \brief Javascript source for the temporary file factory unit tests.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Temporary file factory unit tests.
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

  // Test that the temporary file factory component is available.
  var temporaryFileFactory;
  try {
    temporaryFileFactory =
      Cc["@songbirdnest.com/Songbird/TemporaryFileFactory;1"]
        .getService(Ci.sbITemporaryFileFactory);
  } catch (ex) {}
  assertTrue(temporaryFileFactory,
             "Temporary file factory component is not available.");

  // Test existence of temporary file factory root directory.
  var rootTemporaryDirectory = temporaryFileFactory.rootTemporaryDirectory;
  assertTrue(rootTemporaryDirectory,
             "Temporary file factory directory is null.");
  assertTrue(rootTemporaryDirectory.exists(),
             "Temporary file factory directory does not exist.");

  // Create temporary file and test its existence.
  var tmpFile1 = temporaryFileFactory.createFile(Ci.nsIFile.NORMAL_FILE_TYPE);
  assertTrue(tmpFile1, "Could not create temporary file.");
  assertTrue(tmpFile1.exists(), "Temporary file does not exist.");

  // Create another temporary file and test its uniqueness.
  var tmpFile2 = temporaryFileFactory.createFile(Ci.nsIFile.NORMAL_FILE_TYPE);
  assertTrue(tmpFile2, "Could not create temporary file.");
  assertTrue(tmpFile2.exists(), "Temporary file does not exist.");
  assertTrue(!tmpFile2.equals(tmpFile1), "Temporary file is not unique.");

  // Create and validate a temporary file with a specified base name.
  var fileBaseName = "test";
  tmpFile1 = temporaryFileFactory.createFile(Ci.nsIFile.NORMAL_FILE_TYPE,
                                             fileBaseName);
  assertTrue(tmpFile1, "Could not create temporary file.");
  var fileURL = fileProtocolHandler.newFileURI(tmpFile1);
  fileURL = fileURL.QueryInterface(Ci.nsIURL);
  assertEqual(fileURL.fileBaseName, fileBaseName);

  // Create and validate a temporary file with a specified extension.
  tmpFile1 = temporaryFileFactory.createFile(Ci.nsIFile.NORMAL_FILE_TYPE,
                                             null,
                                             "tst");
  assertTrue(tmpFile1, "Could not create temporary file.");
  assertTrue(tmpFile1.path.match(/\.tst$/), "File extension incorrect.");

  // Clear the temporary files and test that the root temporary directory no
  // longer exists.
  temporaryFileFactory.clear();
  assertTrue(!rootTemporaryDirectory.exists(),
             "Failed to clear temporary files.");
}

