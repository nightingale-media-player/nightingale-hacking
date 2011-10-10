/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

/**
 * \file  test_file_utils.js
 * \brief Javascript source for the file utilities unit tests.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// File utilities unit tests.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Run the unit tests.
 */

function runTest() {
  // Get the directory service.
  var directoryService = Cc["@mozilla.org/file/directory_service;1"]
                           .getService(Ci.nsIDirectoryServiceProvider);

  // Test that the file utilities component is available.
  var fileUtils;
  try {
    fileUtils =
      Cc["@getnightingale.com/Nightingale/FileUtils;1"].getService(Ci.sbIFileUtils);
  } catch (ex) {}
  assertTrue(fileUtils,
             "File utilities component is not available.");

  // Get the current working directory.
  var currentDir = fileUtils.currentDir;
  assertTrue(currentDir, "Could not get the current working directory.");

  // Validate current working directory against the directory service.
  assertTrue(currentDir.equals(directoryService.getFile("CurWorkD", {})),
             "Current working directory not correct.");

  // Create a test working directory.
  var temporaryFileService =
        Cc["@getnightingale.com/Nightingale/TemporaryFileService;1"]
          .getService(Ci.sbITemporaryFileService);
  var testCurrentDir =
        temporaryFileService.createFile(Ci.nsIFile.DIRECTORY_TYPE);

  // Change to the test working directory.
  fileUtils.currentDir = testCurrentDir;
  assertTrue(testCurrentDir.equals(fileUtils.currentDir),
             "Could not change working directory.");
  assertTrue(testCurrentDir.equals(directoryService.getFile("CurWorkD", {})),
             "Current working directory not correct.");

  // Change back to original working directory.
  fileUtils.currentDir = currentDir;
  assertTrue(currentDir.equals(fileUtils.currentDir),
             "Could not change to original working directory.");
  assertTrue(currentDir.equals(directoryService.getFile("CurWorkD", {})),
             "Current working directory not correct.");
}

