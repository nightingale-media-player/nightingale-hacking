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
 * \brief Some globally useful stuff for the local database library tests
 */

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

function createDatabase(databaseGuid) {

  // delete the database if it exists
  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("ProfD", Ci.nsIFile);

  file.append("db");
  file.append(databaseGuid + ".db");
  if(file.exists()) {
    file.remove(false);
  }

  var dbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
              .createInstance(Ci.sbIDatabaseQuery);

  dbq.setDatabaseGUID(databaseGuid);
  var schema = readFile("schema.sql");

  // There seems to be some kind of query length limit so lets break it up
  var a = schema.split(/;$/m);
  for(var q in a) {
    dbq.addQuery(a[q]);
  }
  dbq.execute();
  dbq.waitForCompletion();
  dbq.resetQuery();

  dbq.addQuery("begin");

  var data = readFile("media_items.txt");
  a = data.split("\n");
  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    dbq.addQuery("insert into media_items (guid, created, updated, content_url, content_mime_type, content_length, media_list_type_id) values (?, ?, ?, ?, ?, ?, ?)");
    dbq.bindStringParameter(0, b[1]);
    dbq.bindInt32Parameter(1, b[2]);
    if(b[3] == "") {
      dbq.bindNullParameter(2);
    }
    else {
      dbq.bindInt32Parameter(2, b[3]);
    }
    dbq.bindStringParameter(3, b[4]);
    dbq.bindStringParameter(4, b[5]);
    if(b[5] == "") {
      dbq.bindNullParameter(5);
    }
    else {
      dbq.bindInt32Parameter(5, b[6]);
    }
    if(b[7] == "") {
      dbq.bindNullParameter(6);
    }
    else {
      dbq.bindInt32Parameter(6, b[7]);
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
    dbq.addQuery("insert into simple_media_lists (media_item_id, member_media_item_id, ordinal) values (?, ?, ?)");
    dbq.bindInt32Parameter(0, b[0]);
    dbq.bindInt32Parameter(1, b[1]);
    dbq.bindInt32Parameter(2, b[2]);
  }

  dbq.addQuery("commit");
  dbq.execute();
  dbq.waitForCompletion();
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

function createLibrary(databaseGuid) {

  var libraryFactory =
    Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/LibraryFactory;1"]
      .createInstance(Ci.sbILocalDatabaseLibraryFactory);

  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("ProfD", Ci.nsIFile);

  file.append("db");
  file.append(databaseGuid + ".db");

  return libraryFactory.createLibraryFromDatabase(file);
}

function makeArray(databaseGUID) {
  var array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/GUIDArray;1"]
                .createInstance(Ci.sbILocalDatabaseGUIDArray);
  array.databaseGUID = databaseGUID;
  return array;
}

function assertSort(array, dataFile) {

  var data = readFile(dataFile);
  var a = data.split("\n");

  if(a.length - 1 != array.length) {
    fail("sort failed, length wrong, got " + array.length + " expected " + (a.length - 1));
  }

  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    if(array.getByIndex(i) != b[0]) {
      fail("sort failed, index " + i + " got " + array.getByIndex(i) + " expected " + b[0]);
    }
  }

}

function assertList(list, dataFile) {

  var data = readFile(dataFile);
  var a = data.split("\n");

  if(a.length - 1 != list.length) {
    fail("compare failed, length wrong, got " + list.length + " expected " + (a.length - 1));
  }

  var e = list.items;
  for(var i = 0; i < a.length - 1; i++) {
    var item = e.getNext();
    var b = a[i].split("\t");
    if(item.guid != b[0]) {
      fail("compare failed, index " + i + " got " + item.guid + " expected " + b[0]);
    }
  }

}

