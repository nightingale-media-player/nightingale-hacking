/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=2 :miv */
/*
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2009 POTI, Inc.
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
 * \file  test_directory_enumerator.js
 * \brief Javascript source for the directory enumerator unit tests.
 */

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//
// Directory enumerator unit tests.
//
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------

/**
 * Run the unit tests.
 */

function runTest() {
  // Start running the tests.
  testDirectoryEnumerator.start();
}


/**
 * Directory enumerator tests.
 */

var testDirectoryEnumerator = {
  //
  // Directory enumerator tests configuration.
  //
  //   testDir                  Test directory structure to enumerate.
  //     object                 Specifies a directory with the name of the field.
  //     null                   Specifies a file with the name of the field.
  //   enumerationTestList      List of enumeration tests.
  //     name                   Name of test.
  //     enumConfig             List of enumeration attribute names and values.
  //     expectedResult         Expected result of the enumeration.
  //

  testDir:
  {
    test1:
    {
      test11: null,
      test12: null,
      test13: null
    },
    test2:
    {
      test21:
      {
        test211: null,
        test212: null
      }
    },
    test3:
    {
      test31:
      {
        test311: null,
        test312: null
      },
      test32:
      {
        test321:
        {
          test3211: null
        }
      }
    },
    test4: {}
  },

  enumerationTestList:
  [
    // Test default settings.
    {
      name: "default",
      enumConfig: {},
      expectedResult:
      [
        "test1", "test11", "test12", "test13",
        "test2", "test21", "test211", "test212",
        "test3", "test31", "test311", "test312",
        "test32", "test321", "test3211",
        "test4"
      ]
    },

    // Test max depth setting.
    {
      name: "max depth",
      enumConfig: { maxDepth: 2 },
      expectedResult: [ "test1", "test11", "test12", "test13",
                        "test2", "test21",
                        "test3", "test31", "test32",
                        "test4" ]
    },

    // Test files only setting.
    {
      name: "files only",
      enumConfig: { filesOnly: true },
      expectedResult: [ "test11", "test12", "test13",
                        "test211", "test212",
                        "test311", "test312", "test3211" ]
    },

    // Test directories only setting.
    {
      name: "directories only",
      enumConfig: { directoriesOnly: true },
      expectedResult: [ "test1", "test2", "test21",
                        "test3", "test31", "test32", "test321",
                        "test4" ]
    }
  ],


  //
  // Directory enumerator tests fields.
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

  start: function testDirectoryEnumerator_start() {
    var _this = this;
    var func;

    // Initialize the list of tests.
    this._testList = [];
    func = function() { return _this._testExistence(); };
    this._testList.push(func);
    for (var i = 0; i < this.enumerationTestList.length; i++) {
      // Create a function closure for the enumeration test.  The "let" keyword
      // must be used instead of "var" so that the enumerationTest scope is
      // limited to each iteration of the loop and each function has a different
      // value for the enumeration test.
      let enumerationTest = this.enumerationTestList[i];
      func = function() { return _this._testEnumeration(enumerationTest); };

      // Add the enumeration test to the test list.
      this._testList.push(func);
    }

    // Start running the tests.
    this._nextTestIndex = 0;
    this._runNextTest();
  },


  /**
   * Run the next test.
   */

  _runNextTest: function testDirectoryEnumerator__runNextTest() {
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
   * Test existence of the directory enumerator component.
   */

  _testExistence: function testDirectoryEnumerator__testExistence() {
    // Log progress.
    dump("Running existence test.");

    // Test that the directory enumerator component is available.
    var directoryEnumerator;
    try {
      directoryEnumerator =
        Cc["@songbirdnest.com/Songbird/DirectoryEnumerator;1"]
          .createInstance(Ci.sbIDirectoryEnumerator);
    } catch (ex) {}
    assertTrue(directoryEnumerator,
               "Directory enumerator component is not available.");

    // Test is complete.
    return true;
  },


  /**
   * Test the enumeration function as specified by aTestConfig.
   *
   * \param aTestConfig         Enumeration test configuration.
   */

  _testEnumeration:
    function testDirectoryEnumerator__testEnumeration(aTestConfig) {
    // Log progress.
    dump("Running enumeration test " + aTestConfig.name + "\n");

    // Create the test directory.
    var testDir = this._createTestDir(this.testDir);

    // Create a directory enumerator.
    var directoryEnumerator =
          Cc["@songbirdnest.com/Songbird/DirectoryEnumerator;1"]
            .createInstance(Ci.sbIDirectoryEnumerator);

    // Configure the directory enumerator.
    var enumConfig = aTestConfig.enumConfig;
    for (var config in enumConfig) {
      directoryEnumerator[config] = enumConfig[config];
    }

    // Enumerate the test directory and check results.
    var result = [];
    directoryEnumerator.enumerate(testDir);
    while (directoryEnumerator.hasMoreElements()) {
      result.push(directoryEnumerator.getNext().leafName);
    }
    dump("Result: " + result + "\n");
    assertSetsEqual(result, aTestConfig.expectedResult);

    // Delete the test directory.
    testDir.remove(true);

    // Test is complete.
    return true;
  },


  /**
   * Create a directory with the structure specified by aTestDir and return the
   * created directory.
   *
   * \param aTestDirStruct      Test directory structure.
   *
   * \return                    Test directory.
   */

  _createTestDir:
    function testDirectoryEnumerator__createTestDir(aTestDirStruct) {
    // Create a temporary test directory.
    var temporaryFileService =
          Cc["@songbirdnest.com/Songbird/TemporaryFileService;1"]
            .getService(Ci.sbITemporaryFileService);
    var testDir = temporaryFileService.createFile(Ci.nsIFile.DIRECTORY_TYPE);

    // Fill test directory with specified structure.
    this._addTestSubDir(aTestDirStruct, testDir);

    return testDir;
  },

  _addTestSubDir:
    function testDirectoryEnumerator__createTestSubdir(aTestDirStruct,
                                                       aTestDir) {
    // Fill test directory with specified structure.
    for (var name in aTestDirStruct) {
      // Get the next entry in the structure.
      var entry = aTestDirStruct[name];

      // Add the next file or directory.  A null entry specifies a file with the
      // same name as the directory structure object field.
      if (entry) {
        // Create and fill in a test directory.
        var dir = aTestDir.clone();
        dir.append(name);
        dir.create(Ci.nsIFile.DIRECTORY_TYPE, aTestDir.permissions);
        this._addTestSubDir(entry, dir);
      } else {
        // Create a test file.
        var file = aTestDir.clone();
        file.append(name);
        file.create(Ci.nsIFile.NORMAL_FILE_TYPE, aTestDir.permissions);
      }
    }
  }
};


