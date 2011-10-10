/**
//
// BEGIN NIGHTINGALE GPL
//
// This file is part of the Nightingale web player.
//
// Copyright(c) 2005-2008 POTI, Inc.
// http://getnightingale.com
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
// END NIGHTINGALE GPL
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
const NIGHTINGALE_TESTHARNESS_CONTRACTID = "@getnightingale.com/Nightingale/TestHarness;1";
const NIGHTINGALE_TESTHARNESS_CLASSNAME = "Nightingale Unit Test Test-Harness";
const NIGHTINGALE_TESTHARNESS_CID = Components.ID("{fd541a71-0e71-48aa-9dd1-971df86d1e01}");
const NIGHTINGALE_TESTHARNESS_IID = Ci.sbITestHarness;

const NIGHTINGALE_DEFAULT_DIR = "basetests";
const NIGHTINGALE_HEAD_FILE = "head_nightingale.js";
const NIGHTINGALE_TAIL_FILE = "tail_nightingale.js";

const NS_PREFSERVICE_CONTRACTID = "@mozilla.org/preferences-service;1";
const NS_SCRIPT_TIMEOUT_PREF = "dom.max_chrome_script_run_time";

const PREF_DOWNLOAD_FOLDER       = "nightingale.download.music.folder";
const PREF_DOWNLOAD_ALWAYSPROMPT = "nightingale.download.music.alwaysPrompt";

function sbTestHarness() { }

sbTestHarness.prototype = {

  // list of directories to scan for test files
  mTestComponents : null,

  // list of all tests that failed
  mFailedTests: null,

  // number of tests that have been successfully completed
  mTestCount: 0,

  // toplevel default files to always attempt to load
  mHeadNightingale : null,
  mTailNightingale : null,

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
  
  _disableDatabaseLocaleCollation : function() {
    var dbe = Cc["@getnightingale.com/Nightingale/DatabaseEngine;1"]
                .getService(Ci.sbIDatabaseEngine)
    dbe.localeCollationEnabled = false;
  },
  
  _enableDatabaseLocaleCollation : function() {
    var dbe = Cc["@getnightingale.com/Nightingale/DatabaseEngine;1"]
                .getService(Ci.sbIDatabaseEngine)
    dbe.localeCollationEnabled = true;
  },

  mTempDownloadFolder: null,
  mOldDownloadPath: "",
  mOldDownloadPromptSetting: false,

  _setTempDownloadFolder: function() {
    this.mTempDownloadFolder = Cc["@mozilla.org/file/directory_service;1"].
                               getService(Ci.nsIProperties).
                               get("TmpD", Ci.nsIFile);
    this.mTempDownloadFolder.append("nightingale_test_downloads");

    if (!(this.mTempDownloadFolder.exists() &&
          this.mTempDownloadFolder.isDirectory())) {
      this.mTempDownloadFolder.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    }

    var prefs = Cc[NS_PREFSERVICE_CONTRACTID].getService(Ci.nsIPrefBranch);
    // Save old prefs
    if (prefs.prefHasUserValue(PREF_DOWNLOAD_FOLDER)) {
      this.mOldDownloadPath =
        prefs.getComplexValue(PREF_DOWNLOAD_FOLDER, Ci.nsISupportsString).data;
    }

    this.mOldDownloadPromptSetting = prefs.getBoolPref(PREF_DOWNLOAD_ALWAYSPROMPT);

    // Set new prefs
    prefs.setBoolPref(PREF_DOWNLOAD_ALWAYSPROMPT, false);

    var str = Cc["@mozilla.org/supports-string;1"].
              createInstance(Ci.nsISupportsString);
    str.data = this.mTempDownloadFolder.path;
    prefs.setComplexValue(PREF_DOWNLOAD_FOLDER, Ci.nsISupportsString, str);
  },

  _unsetTempDownloadFolder: function() {
    var prefs = Cc[NS_PREFSERVICE_CONTRACTID].getService(Ci.nsIPrefBranch);

    if (this.mOldDownloadPath) {
      var str = Cc["@mozilla.org/supports-string;1"].
                createInstance(Ci.nsISupportsString);
      str.data = this.mOldDownloadPath;
      prefs.setComplexValue(PREF_DOWNLOAD_FOLDER, Ci.nsISupportsString, str);
    }
    else {
      prefs.clearUserPref(PREF_DOWNLOAD_FOLDER);
    }

    if (this.mOldDownloadPromptSetting) {
      prefs.setBoolPref(PREF_DOWNLOAD_ALWAYSPROMPT, this.mOldDownloadPromptSetting);
    }

    if (this.mTempDownloadFolder && this.mTempDownloadFolder.exists()) {
      this.mTempDownloadFolder.remove(true);
    }
    this.mTempDownloadFolder = null;
  },
  
  // when using NO_EM_RESTART, on first run extension components do not get
  // registered.  Do that manually in the hopes of things working better
  _registerExtensionComponents: function() {
    // NO_EM_RESTART has been removed from the environment at this point, we
    // can't check for it.
    
    var marker = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties).
                 get("ProfD", Components.interfaces.nsIFile);
    marker.append(".autoreg"); // Mozilla component registration marker
    if (!marker.exists()) {
      // not first run, things have already registered
      return;
    }
    // NOTE: we do _not_ remove the marker file, we let Mozilla deal with it
    
    var compReg = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
    
    var em = Cc["@mozilla.org/extensions/manager;1"]
               .getService(Ci.nsIExtensionManager);
    var appInfo = Cc["@mozilla.org/xre/app-info;1"]
                    .getService(Ci.nsIXULRuntime);

    var itemList = em.getItemList(Ci.nsIUpdateItem.TYPE_EXTENSION, {});
    for each (var item in itemList) {
      var installLocation = em.getInstallLocation(item.id);
      var dir = installLocation.getItemLocation(item.id);
      dir.append("components");
      if (dir.exists()) {
        compReg.autoRegister(dir);
      }

      // also try to register in the platform-specific directory
      dir = installLocation.getItemLocation(item.id);
      dir.append("platform");
      dir.append(appInfo.OS + "_" + appInfo.XPCOMABI);
      dir.append("components");
      if (dir.exists()) {
        compReg.autoRegister(dir);
      }
    }
  },

  init : function ( aTests ) {
    this._registerExtensionComponents();

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
    this.mHeadNightingale = this.mTestDir.clone().QueryInterface(Ci.nsILocalFile);
    this.mHeadNightingale.append(NIGHTINGALE_DEFAULT_DIR);
    this.mHeadNightingale.append(NIGHTINGALE_HEAD_FILE);

    this.mTailNightingale = this.mTestDir.clone().QueryInterface(Ci.nsILocalFile);
    this.mTailNightingale.append(NIGHTINGALE_DEFAULT_DIR);
    this.mTailNightingale.append(NIGHTINGALE_TAIL_FILE);

    this._disableDatabaseLocaleCollation();
    this._disableScriptTimeout();
    this._setTempDownloadFolder();
  },

  run : function () {

    var consoleService = Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService);
    var consoleListener = Cc["@getnightingale.com/Nightingale/TestHarness/ConsoleListener;1"].createInstance(Ci.nsIConsoleListener);
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

      // put the test names into an array so we can sort them
      var testNameArray = new Array();

      if ( testCompTests.length == 0 ) {
        // if the user didn't specify the tests to be run, get them all

        // get an enumerator for the contents of the directory
        let dirEnum = testDir.directoryEntries;

        while ( dirEnum.hasMoreElements() ) {
          let testFile = dirEnum.getNext().QueryInterface(Ci.nsIFile);

          // skip over directories for now (our build system does not allow for
          // creating hierarchical test directories
          if (testFile.isDirectory())
            continue;

          // check for test_*.js; the parans cause the string to get parsed into
          //   result as an array
          var regex = /^test_(.+)\.js$/;
          var result = testFile.leafName.match(regex);
          if (!result)
            continue;

          testNameArray.push(result[1]);
        }

        // ensure all platforms do them in the same order
        testNameArray.sort();
      }
      else {
        // if the user did specify, run only them and in that order
        testNameArray = testCompTests;
      }

      // for each test_ file load it and the corresponding head & tail files
      for ( let index = 0; index < testNameArray.length; index++ ) {
        let testBase = testNameArray[index];

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

        // the scope to load the test into
        var scope = { __proto__ : (function()this)(),
                      _test_name: "sbTestHarness"};
        scope.wrappedJSObject = scope;

        // top level head file to always load
        if (this.mHeadNightingale.exists()) {
          scriptUri = ioService.newFileURI(this.mHeadNightingale);
          jsLoader.loadSubScript( scriptUri.spec, scope );
        }

        // load the component head script
        if (compHeadFile.exists()) {
          scriptUri = ioService.newFileURI(compHeadFile);
          jsLoader.loadSubScript( scriptUri.spec, scope );
        }

        // load the test head script
        if (testHeadFile.exists()) {
          scriptUri = ioService.newFileURI(testHeadFile);
          jsLoader.loadSubScript( scriptUri.spec, scope );
        }

        // _test_name defined in the head_nightingale.js file and is used in tail_nightingale.js
        scope._test_name = testComp + " - " + testBase;

        // load the test script
        if (testFile.exists()) {
          log("*** [" + scope._test_name + "] - Testing...");
          scriptUri = ioService.newFileURI(testFile);
          jsLoader.loadSubScript( scriptUri.spec, scope );
        }

        // top level tail file to always load - calls run_test()
        if (this.mTailNightingale.exists()) {
          // load the cleanup script
          scriptUri = ioService.newFileURI(this.mTailNightingale);
          jsLoader.loadSubScript( scriptUri.spec, scope );
        }

        // load the test tail script
        if (testTailFile.exists()) {
          // load the cleanup script
          scriptUri = ioService.newFileURI(testTailFile);
          jsLoader.loadSubScript( scriptUri.spec, scope );
        }

        // load the component tail script
        if (compTailFile.exists()) {
          // load the cleanup script
          scriptUri = ioService.newFileURI(compTailFile);
          jsLoader.loadSubScript( scriptUri.spec, scope );
        }

        this.mTestCount++;
        scope = null;
        Components.utils.forceGC();
      }
    }

    if ( this.mFailedTests ) {
      log("\n\n");
      log("[Test Harness] *** The following tests failed:");
      for ( var index = 0; index < this.mFailedTests.length ; index++ )
        log("[Test Harness] - " + this.mFailedTests[index]);
      log("\n\n");
    }
    else if (this.mTestCount > 0) {
      log("\n\n");
      log("[Test Harness] *** ALL TESTS PASSED\n\n");
    }
    else {
      log("\n\n");
      log("[Test Harness] *** NO TESTS FOUND\n\n");
    }

    consoleService.unregisterListener(consoleListener);
    this._unsetTempDownloadFolder();
    this._enableScriptTimeout();
    this._enableDatabaseLocaleCollation();

    if (this.mFailedTests) {
      throw Cr.NS_ERROR_ABORT;
    }
  },

  // called only if there are NO components passed in, builds a list
  //   of ALL subdirectories.
  buildTestComponents : function() {
    // iterate over all directories in testharness and add ALL directories
    // found to the list of dirs to recurse through.
    if (!this.mTestDir.exists()) {
      // No test dir so no tests to run.
      return;
    }

    let dirEnum = this.mTestDir.directoryEntries;
    this.mTestComponents = new Array();
    while ( dirEnum.hasMoreElements() ) {
      var entry = dirEnum.getNext().QueryInterface(Ci.nsIFile);
      if (entry.isDirectory()) {
        this.mTestComponents.push(entry.leafName);
      }
    }
    this.mTestComponents.sort();
  },

  logFailure: function (aComponentName) {
    if (! this.mFailedTests)
      this.mFailedTests = new Array();
    this.mFailedTests.push(aComponentName);
  },

  QueryInterface : function(iid) {
    if (!iid.equals(NIGHTINGALE_TESTHARNESS_IID) &&
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
    compMgr.registerFactoryLocation( NIGHTINGALE_TESTHARNESS_CID,
                                     NIGHTINGALE_TESTHARNESS_CLASSNAME,
                                     NIGHTINGALE_TESTHARNESS_CONTRACTID,
                                     fileSpec,
                                     location,
                                     type );
  },

  getClassObject: function(compMgr, cid, iid) {
    if (!cid.equals(NIGHTINGALE_TESTHARNESS_CID))
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
  }

}; // sbTestHarnessModule

function NSGetModule(compMgr, fileSpec) {
  return sbTestHarnessModule;
} // NSGetModule
