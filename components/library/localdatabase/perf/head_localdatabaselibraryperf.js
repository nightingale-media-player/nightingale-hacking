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
 * \brief Some globally useful stuff for the local database library tests
 */

var SB_NS = "http://songbirdnest.com/data/1.0#";

function runPerfTest(aName, aTestFunc) {

  var library = getLibrary();
  var timer = new Timer();
  aTestFunc.apply(this, [library, timer]);

  log("***************** " + aName + " " + libraryFile + " " + timer.elapsed() + "ms");

  if (resultFile) {
    var outputFile = Components.classes["@mozilla.org/file/local;1"]
    .createInstance(Ci.nsILocalFile);
    outputFile.initWithPath(resultFile);

    var fos = Cc["@mozilla.org/network/file-output-stream;1"]
    .createInstance(Ci.nsIFileOutputStream);
    var s = aName + "\t" + libraryFile + "\t" + timer.elapsed() + "\n";
    fos.init(outputFile, 0x02 | 0x08 | 0x10, 0666, 0);
    fos.write(s, s.length);
    fos.close();
  }
}

function getLibrary() {
  var environment =
    Components.classes["@mozilla.org/process/environment;1"]
              .getService(Components.interfaces.nsIEnvironment);

  var libraryFile;
  if (!environment.exists("SB_PERF_LIBRARY")) {
    fail("SB_PERF_LIBRARY must be set");
  }
  libraryFile = environment.get("SB_PERF_LIBRARY");

  var resultFile =  environment.get("SB_PERF_RESULTS");

  var file = Components.classes["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsILocalFile);
  file.initWithPath(libraryFile);

  var libraryFactory =
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"]
      .getService(Ci.sbILibraryFactory);
  var hashBag = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag2);
  hashBag.setPropertyAsInterface("databaseFile", file);
  return libraryFactory.createLibrary(hashBag);
}

function Timer() {
}

Timer.prototype = {
  _startTime: null,
  _stopTime: null,
  start: function() {
    this._startTime = Date.now();
  },
  stop: function() {
    this._stopTime = Date.now();
  },
  elapsed: function() {
    return this._stopTime - this._startTime;
  }
}

function newGuidArray(aLibrary) {
  var ldbl = aLibrary.QueryInterface(Ci.sbILocalDatabaseLibrary);
  var array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/GUIDArray;1"]
                .createInstance(Ci.sbILocalDatabaseGUIDArray);
  array.databaseGUID = ldbl.databaseGuid;
  array.propertyCache = ldbl.propertyCache;
  array.baseTable = "media_items";
  array.fetchSize = 1000;

  return array;
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

function newFileURI(file) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);

  return ioService.newFileURI(file);
}

function newURI(spec) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);

  return ioService.newURI(spec, null, null);
}

function getFile(fileName) {
  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("resource:app", Ci.nsIFile);
  file = file.clone();
  file.append("testharness");
  file.append("localdatabaselibrary");
  file.append(fileName);
  return file;
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
