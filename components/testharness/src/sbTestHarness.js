/**
//
// BEGIN SONGBIRD GPL
//
// This file is part of the Songbird web player.
//
// Copyright(c) 2005-2007 POTI, Inc.
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
 * \file sbTestHarness.js
 * \brief Implementation of the interface sbITestHarness
 * \sa sbITestHarness.idl
 */

// alias some common referenced constants
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

// testharness constants
const SONGBIRD_TESTHARNESS_CONTRACTID = "@songbirdnest.com/Songbird/TestHarness;1";
const SONGBIRD_TESTHARNESS_CLASSNAME = "Songbird Unit Test Test-Harness";
const SONGBIRD_TESTHARNESS_CID = Components.ID("{fd541a71-0e71-48aa-9dd1-971df86d1e01}");
const SONGBIRD_TESTHARNESS_IID = Ci.sbITestHarness;

const SONGBIRD_DEFAULT_DIR = "basetests";
const SONGBIRD_HEAD_FILE = "head_songbird.js";
const SONGBIRD_TAIL_FILE = "tail_songbird.js";

const NS_PREFSERVICE_CONTRACTID = "@mozilla.org/preferences-service;1";
const NS_SCRIPT_TIMEOUT_PREF = "dom.max_chrome_script_run_time";

function sbTestHarness() { }

sbTestHarness.prototype = {

  // list of directories to scan for test files
  mTestComponents : null,

  // list of all tests that failed
  mFailedTests: null,

  // toplevel default files to always attempt to load
  mHeadSongbird : null,
  mTailSongbird : null,
  
  mOldScriptTimeout: -1,

  _disableScriptTimeout : function() {
    var prefs = Cc[NS_PREFSERVICE_CONTRACTID].getService(Ci.nsIPrefBranch);
    
    mOldScriptTimeout = prefs.getIntPref(NS_SCRIPT_TIMEOUT_PREF);
    prefs.setIntPref(NS_SCRIPT_TIMEOUT_PREF, 0);
  },

  _enableScriptTimeout : function() {
    if (mOldScriptTimeout > -1) {
      var prefs = Cc[NS_PREFSERVICE_CONTRACTID].getService(Ci.nsIPrefBranch);
      prefs.setIntPref(NS_SCRIPT_TIMEOUT_PREF, mOldScriptTimeout);
    }
  },

  init : function ( aTests ) {
    // list of test components and possible test names to run.
    //   e.g. - testcomp:testname+testname,testcomp,testcomp:testname
    if (aTests && aTests != "")
      this.mTestComponents = aTests.split(",");

    // Get the root directory for the application
    var dirService = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
    var rootDir = dirService.get("resource:app" , Ci.nsIFile);

    // walk into the testharness dir
    this.mTestDir = rootDir.clone().QueryInterface(Ci.nsILocalFile);
    this.mTestDir.appendRelativePath("testharness");

    // set up files for the top level head and tail files
    this.mHeadSongbird = this.mTestDir.clone().QueryInterface(Ci.nsILocalFile);
    this.mHeadSongbird.append(SONGBIRD_DEFAULT_DIR);
    this.mHeadSongbird.append(SONGBIRD_HEAD_FILE);

    this.mTailSongbird = this.mTestDir.clone().QueryInterface(Ci.nsILocalFile);
    this.mTailSongbird.append(SONGBIRD_DEFAULT_DIR);
    this.mTailSongbird.append(SONGBIRD_TAIL_FILE);
    
    this._disableScriptTimeout();
  },

  run : function () {

    var consoleService = Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService);
    var consoleListener = Cc["@songbirdnest.com/Songbird/TestHarness/ConsoleListener;1"].createInstance(Ci.nsIConsoleListener);
    consoleService.registerListener(consoleListener);

    var ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    var jsLoader = Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Ci.mozIJSSubScriptLoader);

    var log = function (s) {
      consoleService.logStringMessage(s);
    }

    // If components were not specific populate the list with all of them.
    if (!this.mTestComponents)
      this.buildTestComponents();

    // loop over all the components and run the tests
    for ( var index in this.mTestComponents ) {

      // create a file object pointing to each component dir in turn
      var testDir = this.mTestDir.clone().QueryInterface(Ci.nsILocalFile);
      var testCompTests = this.mTestComponents[index].split(":");
      var testComp = testCompTests.shift();
      testDir.append(testComp);

      if (testCompTests.length !=0) {
        testCompTests = testCompTests[0].split("+");
      }

      if ( !testDir.exists() || !testDir.isDirectory() )
        continue;

      // get an enumerator for the contents of the directory
      var dirEnum = testDir.directoryEntries;

      // for each test_ file load it and the corresponding head & tail files
      while ( dirEnum.hasMoreElements() ) {
        var testFile = dirEnum.getNext().QueryInterface(Ci.nsIFile);

        // skip over directories for now (our build system does not allow for
        // creating hierarchical test directories
        if (testFile.isDirectory())
          continue;

        // check for test_*.js; the parans cause the string to get parsed into
        //   result as an array
        var regex = /^(test_)(.+)(\.js)$/;
        var result = testFile.leafName.match(regex);
        if (!result)
          continue;
          
        // our root string will always be in 2
        // use this to look for tail_ and head_ files
        var testBase = result[2];

        // Check to see if the caller was specific about the actual tests to
        // run. If so make sure the testBase matches otherwise don't run it.
        if (testCompTests.length != 0 ) {
          var testName;
          for (testName in testCompTests) {
            if ( testCompTests[testName] == testBase )
              break;
          }
          // if the above loop didn't find a match skip this test, these
          //   aren't the droids you're looking for, move along, move along
          if (testCompTests[testName] != testBase)
            continue;
        }
         

        // clone the nsIFile object so we can point to 3 files
        var testFile = testDir.clone().QueryInterface(Ci.nsILocalFile);
        var testHeadFile = testDir.clone().QueryInterface(Ci.nsILocalFile);
        var compHeadFile = testDir.clone().QueryInterface(Ci.nsILocalFile);
        var testTailFile = testDir.clone().QueryInterface(Ci.nsILocalFile);
        var compTailFile = testDir.clone().QueryInterface(Ci.nsILocalFile);

        // append the file names
        testFile.append("test_" + testBase + ".js");
        testHeadFile.append("head_" + testBase + ".js");
        compHeadFile.append("head_" + testComp + ".js");
        testTailFile.append("tail_" + testBase + ".js");
        compTailFile.append("tail_" + testComp + ".js");

        // script loader takes a nsIURI object
        var scriptUri = null;

        // top level head file to always load
        if (this.mHeadSongbird.exists()) {
          scriptUri = ioService.newFileURI(this.mHeadSongbird);
          jsLoader.loadSubScript( scriptUri.spec, null );
        }

        // load the component head script
        if (compHeadFile.exists()) {
          scriptUri = ioService.newFileURI(compHeadFile);
          jsLoader.loadSubScript( scriptUri.spec, null );
        }

        // load the test head script
        if (testHeadFile.exists()) {
          scriptUri = ioService.newFileURI(testHeadFile);
          jsLoader.loadSubScript( scriptUri.spec, null );
        }

        // _test_name defined in the head_songbird.js file and is used in tail_songbird.js
        _test_name = testComp + " - " + testBase;

        // load the test script
        if (testFile.exists()) {
          log("*** [" + _test_name + "] - Testing...");
          scriptUri = ioService.newFileURI(testFile);
          jsLoader.loadSubScript( scriptUri.spec, null );
        }

        // top level tail file to always load - calls run_test()
        if (this.mTailSongbird.exists()) {
          // load the cleanup script
          scriptUri = ioService.newFileURI(this.mTailSongbird);
          jsLoader.loadSubScript( scriptUri.spec, null );
        }

        // load the test tail script
        if (testTailFile.exists()) {
          // load the cleanup script
          scriptUri = ioService.newFileURI(testTailFile);
          jsLoader.loadSubScript( scriptUri.spec, null );
        }

        // load the component tail script
        if (compTailFile.exists()) {
          // load the cleanup script
          scriptUri = ioService.newFileURI(compTailFile);
          jsLoader.loadSubScript( scriptUri.spec, null );
        }

        // reset the name
        _test_name = "sbTestHarness";

      }
    }

    if ( this.mFailedTests ) {
      log("\n\n");
      log("[Test Harness] *** The following tests failed:");
      for ( var index = 0; index < this.mFailedTests.length ; index++ )
        log("[Test Harness] - " + this.mFailedTests[index]);
      log("\n\n");
      throw Cr.NS_ERROR_ABORT;
    }
    else {
      log("\n\n");
      log("[Test Harness] *** ALL TESTS PASSED\n\n");
    }

    consoleService.unregisterListener(consoleListener);
    this._enableScriptTimeout();

  },

  // called only if there are NO components passed in, builds a list
  //   of ALL subdirectories.
  buildTestComponents : function() {
    // iterate over all directories in testharness and add ALL directories 
    // found to the list of dirs to recurse through.
    var dirEnum = this.mTestDir.directoryEntries;

    this.mTestComponents = new Array();
    while ( dirEnum.hasMoreElements() ) {
      var entry = dirEnum.getNext().QueryInterface(Ci.nsIFile);
      if (entry.isDirectory()) {
        this.mTestComponents.push(entry.leafName);
      }
    }
  },

  logFailure: function (aComponentName) {
    if (! this.mFailedTests)
      this.mFailedTests = new Array();
    this.mFailedTests.push(aComponentName);
  },

  QueryInterface : function(iid) {
    if (!iid.equals(SONGBIRD_TESTHARNESS_IID) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
}; // sbTestHarness

/**
 * ----------------------------------------------------------------------------
 * Registration for XPCOM
 * ----------------------------------------------------------------------------
 */
const sbTestHarnessModule = {
  registerSelf: function(compMgr, fileSpec, location, type) {
    compMgr = compMgr.QueryInterface(Ci.nsIComponentRegistrar);
    compMgr.registerFactoryLocation( SONGBIRD_TESTHARNESS_CID,
                                     SONGBIRD_TESTHARNESS_CLASSNAME,
                                     SONGBIRD_TESTHARNESS_CONTRACTID,
                                     fileSpec,
                                     location,
                                     type );
  },

  getClassObject: function(compMgr, cid, iid) {
    if (!cid.equals(SONGBIRD_TESTHARNESS_CID))
      throw Cr.NS_ERROR_NO_INTERFACE;

    if (!iid.equals(Ci.nsIFactory))
      throw Cr.NS_ERROR_NOT_IMPLEMENTED;

    return this.mFactory;
  },

  mFactory: {
    createInstance : function (outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
      return (new sbTestHarness()).QueryInterface(iid);
    }
  },

  canUnload: function(compMgr) {
    return true;
  },

  QueryInterface : function (iid) {
    if ( !iid.equals(Ci.nsIModule) &&
         !iid.equals(Ci.nsISupports) )
      throw Cr.NS_ERROR_NO_INTERFACE;

    return this;
  },

}; // sbTestHarnessModule

function NSGetModule(compMgr, fileSpec) {
  return sbTestHarnessModule;
} // NSGetModule

