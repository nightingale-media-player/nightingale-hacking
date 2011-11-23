/* -*- Mode: Java; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim:set ts=2 sw=2 sts=2 et: */
/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is mozilla.org code.
 *
 * The Initial Developer of the Original Code is Google Inc.
 * Portions created by the Initial Developer are Copyright (C) 2005
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Darin Fisher <darin@meer.net>
 *  Boris Zbarsky <bzbarsky@mit.edu>
 *  John Gaunt <redfive@songbirdnest.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

/*
 * Original file is mozilla code, pulled into the songbird tree with some slight
 * modification since it is being used in a different system (XULRunner) than the
 * original (xpcshell).
 */

// This file contains common code that is loaded with each test file.

/*
 * These get defined in sbTestHarness.js so we don't need to do it here.
 * const Ci = Components.interfaces;
 * const Cc = Components.classes;
 * const Cr = Components.results;
*/

// setup the main thread event queue
var _quit = false;
var _fail = false;
var _skip = false;
var _running_event_loop = false;
var _tests_pending = 0;
var _test_name = "sbTestHarness";
var _test_comp = "testharness";
var _consoleService = null;

// _extensionStates, when initialized, gives the status of each extension.
// The property names are extension IDs (like "mashTape@songbirdnest.com").
// Each value is the extension version, if the extension is active, or one
// of the _TH_EXTENSION_* contants defined below
var _extensionStates = null;

// Used in _extensionStates when an extension is disabled
const _TH_EXTENSION_DISABLED = "disabled";

// Used in _extensionStates when an extension is newly installed and won't
// be available until the app is relaunched
const _TH_EXTENSION_NEEDS_RESTART = "needs-restart";

// Used in _extensionStates when the extension RDF data was malformed
const _TH_EXTENSION_BAD = "bad";

// Thrown to signal a skip, meaning the current test should abort without
// registering a failure
const _TH_SKIP = {};

function doTimeout(delay, func) {
  var timer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
  timer.initWithCallback(func, delay, timer.TYPE_ONE_SHOT);
}

function doMain() {
  if (_quit)
    return;

  _consoleService.logStringMessage("*** [" + _test_name + "] - running event loop");

  var tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
  var mainThread = tm.mainThread;

  while(!_quit) {
    mainThread.processNextEvent(true);
  }

}

function doQuit() {
  _quit = true;
}

function dumpStack() {
  var frame = Components.stack;
  while (frame != null) {
    log(frame);
    frame = frame.caller;
  }
}

function doFail(text) {
  _fail = true;
  _tests_pending = 0;
  doQuit();
  log("*** [" + _test_name + "] - CHECK FAILED: " + text);
  dumpStack();
}

function doThrow(text) {
  doFail(text);
  throw Cr.NS_ERROR_ABORT;
}

function assertFalse(aTest, aMessage) {
  if (aTest) {
    var msg = (aMessage != null) ? ( " : " +  aMessage ) : "";
    doThrow(aTest + msg);
  }
}

function assertTrue(aTest, aMessage) {
  assertFalse(!aTest, aMessage);
}

function assertEqual( aExpected, aActual, aMessage) {
  if (aExpected != aActual) {
    var msg = (aMessage != null) ? ( " : " +  aMessage ) : "";
    doThrow(aExpected + " != " + aActual + msg);
  }
}

function assertNotEqual( aExpected, aActual, aMessage) {
  if (aExpected == aActual) {
    var msg = (aMessage != null) ? ( " : " +  aMessage ) : "";
    doThrow(aExpected + " != " + aActual + msg);
  }
}


// Assert that the extension with the given ID is active.  This function
// throws if the extension is disabled or in a bad state or is not present.
// It skips (i.e., throws without registering a failure) if the extension
// is newly installed and not available until the app relaunches.  This
// happens whenever a buildbot runs unit tests for the first time after
// building.
function assertExtension(aExtensionID) {
  // Initialize the extensions roster the first time through:
  if (!_extensionStates) {
    function verbose (aMessage) {
      // log(">>> " + aMessage);
    }
    log("[Test Harness] Scanning extensions...");

    // Use the rdf:extensions datasource to enumerate extensions and get the
    // following extension properties:
    //
    //   addonID - the key to use in _extensionStates
    //   isDisabled - to identify disabled extensions
    //   state - to identify extensions that are newly installed
    //   version - the status string used for extensions that are operational
    //
    // Note that the "state" (if present) is not the general state of the
    // extension, but strictly the outcome of an install operation that
    // has just completed.  If not present, then the extension was installed
    // during an earlier run of the app.  Thus, it can be used to detect
    // newly installed exensions that won't be ready until the next launch
    // of the app.  This does not appear to be documented, but there's a
    // slim chance one might discern it from the logic and comments in
    // extensions.js and nsExtensionManager:
    //
    // http://mxr.mozilla.org/mozilla1.9.2/source/toolkit/mozapps/extensions/content/extensions.js#438
    // * which reveals that when showing the "installs" view, the types variable
    //   is set to build a list of extensions whose "state" is not empty.
    //
    // http://mxr.mozilla.org/mozilla1.9.2/source/toolkit/mozapps/extensions/src/nsExtensionManager.js.in#7953
    // * which states that the state property represents the current phase of
    //   an install
    // 
    // http://mxr.mozilla.org/mozilla1.9.2/source/toolkit/mozapps/extensions/src/nsExtensionManager.js.in#7617
    // * which shows that ExtensionsDataSource.updateDownloadState() sets the
    //   state property
    //
    // http://mxr.mozilla.org/mozilla1.9.2/source/toolkit/mozapps/extensions/src/nsExtensionManager.js.in#4697
    // * which shows that EM__setOp() sets the state to "success" (using
    //   updateDownloadState()) if the extension reaches the OP_NEEDS_INSTALL
    //   stage.
    //
    // Note that the opType property is cleared before control passes to
    // Songbird, so the "needs-install" value of this property, which has
    // a distintly useful sound to it, is gone at this point.
    //
    // The following logic is adapted from
    // http://mxr.mozilla.org/mozilla1.9.2/source/toolkit/mozapps/extensions/content/extensions.js#2311
    
    
    // Set up an enumerator of all extensions:
    
    let rdfService = Cc["@mozilla.org/rdf/rdf-service;1"]
                       .getService(Ci.nsIRDFService);
    let extensionsDS = rdfService.GetDataSource("rdf:extensions");
    verbose("extensions datasource [" + extensionsDS.URI + "]");
    
    let container = Cc["@mozilla.org/rdf/container;1"]
                      .createInstance(Ci.nsIRDFContainer);
    container.Init(extensionsDS,
                   rdfService.GetResource("urn:mozilla:item:root"));
    let each = container.GetElements();

    // Define the properties to look up:
    const PREFIX_NS_EM = "http://www.mozilla.org/2004/em-rdf#";
    const ADDONID_ARC = rdfService.GetResource(PREFIX_NS_EM + "addonID");
    const STATE_ARC = rdfService.GetResource(PREFIX_NS_EM + "state");
    const DISABLED_ARC =  rdfService.GetResource(PREFIX_NS_EM + "isDisabled");
    const VERSION_ARC =  rdfService.GetResource(PREFIX_NS_EM + "version");

    // Default to a blank set:
    _extensionStates = {};

    // Enumerate the extensions:
    while (each.hasMoreElements()) {
      let extensionRdf = each.getNext().QueryInterface(Ci.nsIRDFResource);
      verbose("extension [" + extensionRdf.ValueUTF8 + "]");

      // Get its add-on ID:
      let addonID = extensionsDS.GetTarget(extensionRdf, ADDONID_ARC, true);
      if (!(addonID instanceof Ci.nsIRDFLiteral)) {
        verbose("INVALID " + ADDONID_ARC.ValueUTF8);
        continue;
      }

      // Get the other properties of interest:
      let installState = extensionsDS.GetTarget(extensionRdf, STATE_ARC, true);
      let disabled = extensionsDS.GetTarget(extensionRdf, DISABLED_ARC, true);
      let version = extensionsDS.GetTarget(extensionRdf, VERSION_ARC, true);
      
      // Check for successful install, then disabled status, then record its
      // version as its status, and finally, mark it as bad if its properties
      // weren't the expected type:
      if (installState instanceof Ci.nsIRDFLiteral) {
        verbose("install state [" + installState.Value + "]");
        _extensionStates[addonID.Value] =
          (installState.Value == "success") ? _TH_EXTENSION_NEEDS_RESTART :
                                              _TH_EXTENSION_BAD;
      }
      else if (disabled instanceof Ci.nsIRDFLiteral &&
               disabled.Value == "true")
      {
        verbose("disabled [" + disabled.Value + "]");
        _extensionStates[addonID.Value] = _TH_EXTENSION_DISABLED;
      }
      else if (version instanceof Ci.nsIRDFLiteral) {
        verbose("version [" + version.Value + "]");
        _extensionStates[addonID.Value] = version.Value;
      }
      else {
        verbose("RDF PROPERTIES INFO NOT FOUND <<<\n");
        _extensionStates[addonID.Value] = _TH_EXTENSION_BAD;
      }
      
      log("  " + addonID.Value + ": " + _extensionStates[addonID.Value]);
    }
  }

  // The list of extensions is initialized.  Now check for the one
  // specified.
  
  // Is it listed?
  assertTrue((aExtensionID in _extensionStates),
             "No such extension: " + aExtensionID);

  let status = _extensionStates[aExtensionID];

  // Is it inoperable?
  switch (status) {
    case _TH_EXTENSION_BAD:
      fail("Required extension failed to install: " + aExtensionID);
      break;

    case _TH_EXTENSION_DISABLED:
      fail("Required extension is disabled: " + aExtensionID);
      break;

    case _TH_EXTENSION_NEEDS_RESTART:
      skip("Required extension " + aExtensionID +
           " is newly installed.  You must relaunch to run this test.");
      break;
  }

  // The extension is ready.  Its status will be its version.  Maybe
  // the caller would like to know this tidbit:
  return status;
}

function fail(aMessage) {
  var msg = (aMessage != null) ? ( "FAIL : " +  aMessage ) : "FAIL";
  doThrow(msg);
}

function failNoThrow(aMessage) {
  var msg = (aMessage != null) ? ( "FAIL : " +  aMessage ) : "FAIL";
  doFail(msg);
}

// Skip the current test without registering a failure
function skip(aMessage) {
  if (!aMessage) {
    aMessage = "SKIPPING TEST";
  }
  log("*** [" + _test_name + "] - WARNING: " + aMessage);
  throw _TH_SKIP;
}

function testPending() {
  log("*** [" + _test_name + "] - test pending");
  if ( _tests_pending == 0 ) {
    // start of tests, don't spin wait
    _tests_pending++;
    return;
  }

  let count = _tests_pending++;
  while ( count < _tests_pending ) {
    // sleep for awhile. testFinished will decrement the _tests_pending
    sleep(100, true);
  }
}

function testFinished() {
  log("*** [" + _test_name + "] - test finished");
  if (--_tests_pending == 0)
    doQuit();
}

function log(s) {
  _consoleService.logStringMessage(s);
}

function getPlatform() {
  var sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
  return sysInfo.getProperty("name");
}

function sleep(ms, suppressOutput) {
  var threadManager = Cc["@mozilla.org/thread-manager;1"].
                      getService(Ci.nsIThreadManager);
  var mainThread = threadManager.mainThread;

  if (!suppressOutput) {
    log("*** [" + _test_name + "] - waiting for " + ms + " milliseconds...");
  }
  var then = new Date().getTime(), now = then;
  for (; now - then < ms; now = new Date().getTime()) {
    mainThread.processNextEvent(true);
  }
  if (!suppressOutput) {
    log("*** [" + _test_name + "] - done waiting.");
  }
}

function getTestServerPortNumber() {
  var DEFAULT_PORT = 8180;
  const ENV_VAR_PORT = "SONGBIRD_TEST_SERVER_PORT";

  var environment =
    Components.classes["@mozilla.org/process/environment;1"]
              .getService(Components.interfaces.nsIEnvironment);

  var port = DEFAULT_PORT;
  if (environment.exists(ENV_VAR_PORT)) {
    var envPort = parseInt(environment.get(ENV_VAR_PORT));
    if (envPort) {
      port = envPort;
    }
  }

  log("*** [" + _test_name + "] - using test server port " + port);

  return port;
}

function StringArrayEnumerator(aArray) {
  this._array = aArray;
  this._current = 0;
}

StringArrayEnumerator.prototype.hasMore = function() {
  return this._current < this._array.length;
}

StringArrayEnumerator.prototype.getNext = function() {
  return this._array[this._current++];
}

StringArrayEnumerator.prototype.QueryInterface = function(iid) {
  if (!iid.equals(Components.interfaces.nsIStringEnumerator) &&
      !iid.equals(Components.interfaces.nsISupports))
    throw Components.results.NS_ERROR_NO_INTERFACE;
  return this;
};

function ExtensionSchemeMatcher(aExtensions, aSchemes) {
  this._extensions = aExtensions;
  this._schemes    = aSchemes;
}

ExtensionSchemeMatcher.prototype.match = function(aStr) {

  // Short circuit for the most common case
  if(aStr.lastIndexOf("." + this._extensions[0]) ==
     aStr.length - this._extensions[0].length + 1) {
    return true;
  }

  var extensionSep = aStr.lastIndexOf(".");
  if(extensionSep >= 0) {
    var extension = aStr.substring(extensionSep + 1);
    var rv = this._extensions.some(function(ext) {
      return extension == ext;
    });
    if(rv) {
      return true;
    }
  }

  var schemeSep = aStr.indexOf("://");
  if(schemeSep >= 0) {
    var scheme = aStr.substring(0, schemeSep);
    var rv = this._schemes.some(function(sch) {
      return scheme == sch;
    });
    if(rv) {
      return true;
    }
  }

  return false;
}

_consoleService = Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService);

function loadData(databaseGuid, databaseLocation) {

  var dbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
              .createInstance(Ci.sbIDatabaseQuery);

  dbq.setDatabaseGUID(databaseGuid);
  if (databaseLocation) {
    dbq.databaseLocation = databaseLocation;
  }
  dbq.setAsyncQuery(false);
  dbq.addQuery("create temporary table tmp_rowids (id integer);");
  dbq.addQuery("begin");

  var data = readFile("media_items.txt");
  var a = data.split("\n");
  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    dbq.addQuery("insert into media_items (guid, created, updated, content_url, content_mime_type, content_length, hidden, media_list_type_id, is_list) values (?, ?, ?, ?, ?, ?, ?, ?, ?)");
    dbq.bindStringParameter(0, b[1]);
    dbq.bindInt64Parameter(1, b[2]);
    dbq.bindInt64Parameter(2, b[3]);
    dbq.bindStringParameter(3, b[4]);
    dbq.bindStringParameter(4, b[5]);
    if(b[5] == "") {
      dbq.bindNullParameter(5);
    }
    else {
      dbq.bindInt32Parameter(5, b[6]);
    }
    dbq.bindInt32Parameter(6, b[7]);
    if(b[8] == "" || b[8] == "0") {
      dbq.bindNullParameter(7);
      dbq.bindInt32Parameter(8, 0);
    }
    else {
      dbq.bindInt32Parameter(7, b[8]);
      dbq.bindInt32Parameter(8, 1);
    }
  }

  data = readFile("resource_properties.txt");
  a = data.split("\n");
  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    dbq.addQuery("insert into resource_properties (media_item_id, property_id, obj, obj_searchable, obj_sortable) values ((select media_item_id from media_items where guid = ?), ?, ?, ?, ?)");
    dbq.bindStringParameter(0, b[0]);
    dbq.bindInt32Parameter(1, b[1]);
    dbq.bindStringParameter(2, b[2]);
    dbq.bindStringParameter(3, b[3]);
    dbq.bindStringParameter(4, b[4]);
  }

  data = readFile("simple_media_lists.txt");
  a = data.split("\n");
  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    dbq.addQuery("insert into simple_media_lists (media_item_id, member_media_item_id, ordinal) values ((select media_item_id from media_items where guid = ?), (select media_item_id from media_items where guid = ?), ?)");
    dbq.bindStringParameter(0, b[0]);
    dbq.bindStringParameter(1, b[1]);
    dbq.bindInt32Parameter(2, b[2]);
  }

  // XXXsteve I need to use a temp table here to avoid this bug:
  // http://www.sqlite.org/cvstrac/tktview?tn=3082
  /* XXXAus: resource_properties_fts is disabled. See bug 9488 and bug 9617.
  dbq.addQuery("insert into tmp_rowids select rowid from resource_properties_fts;");
  dbq.addQuery("insert into resource_properties_fts (rowid, propertyid, obj) " +
               " select rowid, property_id, obj from resource_properties where rowid not in (" +
               "select id from tmp_rowids)");
  */
  dbq.addQuery("delete from tmp_rowids");
  dbq.addQuery("insert into tmp_rowids select rowid from resource_properties_fts_all;");
  dbq.addQuery("insert into resource_properties_fts_all (rowid, alldata) " +
               " select media_item_id, group_concat(obj, ' ') from resource_properties where media_item_id not in (" +
               "select id from tmp_rowids) group by media_item_id");

  dbq.addQuery("insert into resource_properties_fts_all (alldata) values ('foo');");

  dbq.addQuery("commit");
  dbq.addQuery("drop table tmp_rowids");
  dbq.execute();
  dbq.resetQuery();

}

function readFile(fileName) {

  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("resource:app", Ci.nsIFile);
  file.append("testharness");
  file.append("localdatabaselibrary");
  file.append(fileName);

  var data = "";
  var fstream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  var sstream = Cc["@mozilla.org/scriptableinputstream;1"]
                  .createInstance(Ci.nsIScriptableInputStream);
  fstream.init(file, -1, 0, 0);
  sstream.init(fstream);

  var str = sstream.read(4096);
  while (str.length > 0) {
    data += str;
    str = sstream.read(4096);
  }

  sstream.close();
  fstream.close();
  return data;
}

function createLibrary(databaseGuid, databaseLocation, init) {

  if (typeof(init) == "undefined") {
    init = true;
  }

  var directory;
  if (databaseLocation) {
    directory = databaseLocation.QueryInterface(Ci.nsIFileURL).file;
  }
  else {
    directory = Cc["@mozilla.org/file/directory_service;1"].
                getService(Ci.nsIProperties).
                get("ProfD", Ci.nsIFile);
    directory.append("db");
  }

  var file = directory.clone();
  file.append(databaseGuid + ".db");

  var libraryFactory =
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"]
      .getService(Ci.sbILibraryFactory);
  var hashBag = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag2);
  hashBag.setPropertyAsInterface("databaseFile", file);
  var library = libraryFactory.createLibrary(hashBag);
  try {
    if (init) {
      library.clear();
    }
  }
  catch(e) {
  }

  if (init) {
    loadData(databaseGuid, databaseLocation);
  }
  return library;
}

// assert the equality of two JS arrays
// assertEqual is used for each of the items
function assertArraysEqual(a1, a2) {
  assertEqual(a1.length, a2.length, "arrays are different lengths");
  for (var i=0; i<a1.length; i++) {
    assertEqual(a1[i], a2[i], "data mismatch at index " + i);
  }
}

// assert the equality of two unordered sets, represnted as JS arrays
function assertSetsEqual(s1, s2) {
  for (var i = 0; i < s1.length; i++) {
    assertTrue(s2.indexOf(s1[i]) >= 0,
               "set 1 member " + s1[i] + " not in set 2");
  }
  for (var i = 0; i < s2.length; i++) {
    assertTrue(s1.indexOf(s2[i]) >= 0,
               "set 2 member " + s2[i] + " not in set 1");
  }
  assertEqual(s1.length, s2.length, "sets are different lengths");
}

// assert the equality of the contents of two files
function assertFilesEqual(aFile1, aFile2, aMessage) {
  if (!testFilesEqual(aFile1, aFile2)) {
    var msg = (aMessage != null) ? ( " : " +  aMessage ) : "";
    doThrow(aFile1.path + " != " + aFile2.path + " " + msg);
  }
}

function testFilesEqual(aFile1, aFile2) {
  if (!aFile1.exists() || !aFile2.exists())
    return false;

  var fileStream1 = Cc["@mozilla.org/network/file-input-stream;1"]
                       .createInstance(Ci.nsIFileInputStream);
  var scriptableFileStream1 = Cc["@mozilla.org/scriptableinputstream;1"]
                                .createInstance(Ci.nsIScriptableInputStream);
  fileStream1.init(aFile1, -1, 0, 0);
  scriptableFileStream1.init(fileStream1);

  var fileStream2 = Cc["@mozilla.org/network/file-input-stream;1"]
                       .createInstance(Ci.nsIFileInputStream);
  var scriptableFileStream2 = Cc["@mozilla.org/scriptableinputstream;1"]
                                .createInstance(Ci.nsIScriptableInputStream);
  fileStream2.init(aFile2, -1, 0, 0);
  scriptableFileStream2.init(fileStream2);

  var filesEqual = true;
  do {
    var data1 = scriptableFileStream1.read(4096);
    var data2 = scriptableFileStream2.read(4096);

    if (data1 != data2) {
      filesEqual = false;
      break;
    }
  } while(data1.length > 0);

  fileStream1.close();
  scriptableFileStream1.close();
  fileStream2.close();
  scriptableFileStream2.close();

  return filesEqual;
}

function newAppRelativeFile( path ) {

  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("resource:app", Ci.nsIFile);
  file = file.clone();

  var nodes = path.split("/");
  for ( var i = 0, end = nodes.length; i < end; i++ )
  {
    file.append( nodes[ i ] );
  }

  return file;
}

/**
 * Return an nsIFile for the test file specified by aFileName within the unit
 * test's test harness directory.
 *
 * \param aFileName             Name of test file to get.
 */
function getTestFile(aFileName) {
  return newAppRelativeFile("testharness/" + _test_comp + "/" + aFileName);
}

/**
 * Makes a new URI from a url string
 */
function newURI(aURLString)
{
  // Must be a string here
  if (!(aURLString &&
       (aURLString instanceof String) || typeof(aURLString) == "string"))
    throw Components.results.NS_ERROR_INVALID_ARG;

  var ioService =
    Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService);

  try {
    return ioService.newURI(aURLString, null, null);
  }
  catch (e) { }

  return null;
}

function newFileURI(file)
{
  var ioService =
    Components.classes["@mozilla.org/network/io-service;1"]
              .getService(Components.interfaces.nsIIOService);
  try {
    return ioService.newFileURI(file);
  }
  catch (e) { }

  return null;
}

/**
 * Helper object to wrap test cases with setup (pre) and cleanup (post)
 * functions. Also provides object stubbing utilities.
 *
 * Example:
 *
 *   var testConfig  = {
 *     desc: "a rockin' test",
 *     pre: function(playlist) {
 *       this.stub(playlist, 'getNextSong', function () {
 *         return null;
 *       });
 *     },
 *     exec: function(playlist) {
 *       assertEqual(this.expected, playlist._canDrop(), this.desc);
 *     },
 *     post: function (playlist) {
 *       doSomePostProcessing();
 *     },
 *     expected: false
 *   };
 *
 *   var playlist = makeMeAPlaylist();
 *   // here, playlist.getNextSong() has it's original behavior
 *   var testCase = new TestCase(testConfig);
 *   // Prior to execution of the test case, playlist.getNextSong() will
 *   // be modified to invoke the stubbed function
 *   testCase.run([playlist]);
 *   // here, playlist.getNextSong() has it's original behavior again
 */
function TestCase(config) {

  for (var option in config) {
    this[option] = config[option];
  }

  this._stubs = [];
}

TestCase.prototype = {

  run: function TestCase_run(args) {

    if (this.pre && typeof this.pre === 'function')
      this.pre.apply(this, args);

    this.exec.apply(this, args);

    if (this.post && typeof this.post === 'function')
      this.post.apply(this, args);

    // Always call unstub so test case instances don't have to worry about it
    // in their post functions. This is a no-op if there are no stubs.
    this.unstub();

  },

  stub: function TestCase_stub(receiver, property, stub) {

    this._stubs.push({
      receiver: receiver,
      property: property,
      stash: receiver[property]
    });

    if (!(property in receiver)) {
      // The test should fail if the caller tries to stub a property that
      // doesn't exist on the receiver object. A stub indicates that the caller
      // expects that property to exist, so failing here alerts us that
      // something we expect to be there is no longer there.
      throw new Error("Attempted to stub property: " + property +
                      "on an object that doesn't contain that property from " +
                      " test case: " + this.desc);
    }

    receiver[property] = stub;
  },

  unstub: function TestCase_unstub() {
    var stubs = this._stubs,
        len = stubs.length;

    if (len > 0) {
      for (let i = 0; i < len; i++) {
        let stub = stubs[i];
        stub.receiver[stub.property] = stub.stash;
      }
      this._stubs = [];
    }
  }

}
