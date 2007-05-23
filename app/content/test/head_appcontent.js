/*
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
 * \brief Test file
 */

var testWindow;
var testWindowFailed;
function beginWindowTest(url, continueFunction) {
  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"]
             .getService(Ci.nsIWindowWatcher);

  testWindow = ww.openWindow(null, url, null, null, null);
  testWindow.addEventListener("load", function() {
    continueFunction.apply(this);
  }, false);
  testWindowFailed = false;
  testPending();
}

function endWindowTest(e) {
  if (!testWindowFailed) {
    testWindowFailed = true;
    if (testWindow) {
      testWindow.close();
      testWindow = null;
    }
    if (e) {
      fail(e);
    }
    else {
      testFinished();
    }
  }
}

function continueWindowTest(fn, parameters) {

  try {
    fn.apply(this, parameters);
  }
  catch(e) {
    endWindowTest();
    fail(e);
  }
}

function safeSetTimeout(closure, timeout) {
  testWindow.setTimeout(function() {
    try {
      closure.apply(this);
    }
    catch(e) {
      endWindowTest();
      fail(e);
    }
  }, timeout);

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

function createLibrary(databaseGuid, databaseLocation) {

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
      .createInstance(Ci.sbILibraryFactory);
  var hashBag = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag2);
  hashBag.setPropertyAsInterface("databaseFile", file);
  var library = libraryFactory.createLibrary(hashBag);
  library.clear();
  loadData(databaseGuid, databaseLocation);
  return library;
}

function loadData(databaseGuid, databaseLocation) {

  var dbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
              .createInstance(Ci.sbIDatabaseQuery);

  dbq.setDatabaseGUID(databaseGuid);
  if (databaseLocation) {
    dbq.databaseLocation = databaseLocation;
  }
  dbq.setAsyncQuery(false);
  dbq.addQuery("begin");

  var data = readFile("media_items.txt");
  var a = data.split("\n");
  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    dbq.addQuery("insert into media_items (guid, created, updated, content_url, content_mime_type, content_length, hidden, media_list_type_id) values (?, ?, ?, ?, ?, ?, ?, ?)");
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
    if(b[8] == "") {
      dbq.bindNullParameter(7);
    }
    else {
      dbq.bindInt32Parameter(7, b[8]);
    }
  }

  data = readFile("resource_properties.txt");
  a = data.split("\n");
  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    dbq.addQuery("insert into resource_properties (guid, property_id, obj, obj_sortable) values (?, ?, ?, ?)");
    dbq.bindStringParameter(0, b[0]);
    dbq.bindInt32Parameter(1, b[1]);
    dbq.bindStringParameter(2, b[2]);
    dbq.bindStringParameter(3, b[3]);
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

  dbq.addQuery("commit");
  dbq.execute();
  dbq.resetQuery();

}

function newURI(spec) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  
  return ioService.newURI(spec, null, null);
}

