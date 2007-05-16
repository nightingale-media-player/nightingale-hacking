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

var SB_NS = "http://songbirdnest.com/data/1.0#";

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

function SimpleArrayEnumerator(aArray) {
  this._array = aArray;
  this._current = 0;
}

SimpleArrayEnumerator.prototype.hasMoreElements = function() {
  return this._current < this._array.length;
}

SimpleArrayEnumerator.prototype.getNext = function() {
  return this._array[this._current++];
}

SimpleArrayEnumerator.prototype.QueryInterface = function(iid) {
  if (!iid.equals(Components.interfaces.nsISimpleEnumerator) &&
      !iid.equals(Components.interfaces.nsISupports))
    throw Components.results.NS_ERROR_NO_INTERFACE;
  return this;
};

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

  if (init == undefined) {
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
      .createInstance(Ci.sbILibraryFactory);
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

function assertArray(array, dataFile) {

  var data = readFile(dataFile);
  var a = data.split("\n");

  if(a.length - 1 != array.length) {
    fail("length wrong, got " + array.length + " expected " + (a.length - 1));
  }

  for(var i = 0; i < a.length - 1; i++) {
    var dataValue = a[i].split("\t")[0];
    if(array[i] != dataValue) {
      fail("sort failed, index " + i + " got " + array[i] + " expected " + dataValue);
    }
  }
}

function assertList(list, data) {

  var a;
  if (data instanceof Array) {
    a = data;
  }
  else {
    a = readList(data);
  }

  if(a.length != list.length) {
    fail("compare failed, length wrong, got " + list.length + " expected " + a.length);
  }

  var listener = new TestMediaListEnumerationListener();
  
  listener.enumItemFunction = function onItem(list, item) {
    if (item.guid != a[this.count]) {
      fail("compare failed, index " + this.count + " got " + item.guid +
            " expected " + a[this.count]);
      return false;
    }
    return true;
  };
  
  list.enumerateAllItems(listener, Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);
}

function TestMediaListListener() {
}
TestMediaListListener.prototype = {
  _addedItem: null,
  _removedItem: null,
  _updatedItem: null,
  _batchBeginList: null,
  _batchEndList: null,
  
  get addedItem() {
    return this._addedItem;
  },
  
  get removedItemBefore() {
    return this._removedItemBefore;
  },
  
  get removedItemAfter() {
    return this._removedItemAfter;
  },
  
  get updatedItem() {
    return this._updatedItem;
  },
  
  get batchBeginList() {
    return this._batchBeginList;
  },
  
  get batchEndList() {
    return this._batchEndList;
  },
  
  reset: function reset() {
    this._addedItem = null;
    this._removedItem = null;
    this._updatedItem = null;
    this._batchBeginList = null;
    this._batchEndList = null;
  },
  
  onItemAdded: function onItemAdded(list, item) {
    this._addedItem = item;
  },
  
  onBeforeItemRemoved: function onBeforeItemRemoved(list, item) {
    this._removedItemBefore = item;
  },
  
  onAfterItemRemoved: function onAfterItemRemoved(list, item) {
    this._removedItemAfter = item;
  },
  
  onItemUpdated: function onItemUpdated(list, item) {
    this._updatedItem = item;
  },

  onBatchBegin: function onBatchBegin(list) {
    this._batchBeginList = list;
  },
  
  onBatchEnd: function onBatchEnd(list) {
    this._batchEndList = list;
  },
  
  QueryInterface: function QueryInterface(iid) {
    if (!iid.equals(Ci.sbIMediaListListener) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }  
}

function TestMediaListEnumerationListener() {
  this._items = [];
}
TestMediaListEnumerationListener.prototype = {
  _result: Cr.NS_OK,
  _enumBeginFunction: null,
  _enumItemFunction: null,
  _enumEndFunction: null,
  _enumerationComplete: false,
  _items: null,
  _index: 0,
  
  onEnumerationBegin: function onEnumerationBegin(list) {
    if (this._enumBeginFunction)
      return this._enumBeginFunction(list);
      
    return true;
  },
  
  onEnumeratedItem: function onEnumeratedItem(list, item) {
    var retval = true;
    
    if (this._enumItemFunction)
      retval = this._enumItemFunction(list, item);
      
    if (retval) {
      this._items.push(item);
    }
      
    return retval;
  },
  
  onEnumerationEnd: function onEnumerationEnd(list, result) {
    if (this._enumEndFunction)
      this._enumEndFunction(list, result);
    this._result = result;
    this._enumerationComplete = true;
  },
  
  reset: function reset() {
    this._result = Cr.NS_OK;
    this._enumeratedItemCount = 0;
    this._enumBeginFunction = null;
    this._enumItemFunction = null;
    this._enumEndFunction = null;
    this._items = [];
    this._index = 0;
    this._enumerationComplete = false;
  },
  
  hasMoreElements: function hasMoreElements() {
    if (!this._enumerationComplete)
      throw Cr.NS_ERROR_NOT_AVAILABLE;
      
    return this._index < this._items.length;
  },
  
  getNext: function getNext() {
    if (!this._enumerationComplete)
      throw Cr.NS_ERROR_NOT_AVAILABLE;
      
    return this._items[this._index++];
  },

  set enumBeginFunction(val) {
    this._enumBeginFunction = val;
  },
  
  set enumItemFunction(val) {
    this._enumItemFunction = val;
  },

  set enumEndFunction(val) {
    this._enumEndFunction = val;
  },

  get result() {
    return this._result;
  },
  
  get count() {
    return this._items.length;
  },
    
  QueryInterface: function QueryInterface(iid) {
    if (!iid.equals(Ci.sbIMediaListEnumerationListener) &&
        !iid.equals(Ci.nsISimpleEnumerator) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
}

function readList(dataFile, index) {

  var data = readFile(dataFile);
  var a = data.split("\n");
  index = index || 0;

  var b = [];
  for(var i = 0; i < a.length - 1; i++) {
    b.push(a[i].split("\t")[index]);
  }

  return b;
}

function loadMockDatabase() {
  var data = readFile("media_items.txt");
  var a = data.split("\n");

  var sp = [
    "http://songbirdnest.com/data/1.0#created",
    "http://songbirdnest.com/data/1.0#updated",
    "http://songbirdnest.com/data/1.0#contentUrl",
    "http://songbirdnest.com/data/1.0#contentMimeType",
    "http://songbirdnest.com/data/1.0#contentLength",
    "http://songbirdnest.com/data/1.0#hidden",
    "http://songbirdnest.com/data/1.0#isList"
  ];

  var db = {};

  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    var item = db[b[1]];
    if(!item) {
      item = {};
      db[b[1]] = item;
    }

    for(var j = 0; j < sp.length; j++) {
      item[sp[j]] = b[j + 2];
    }
  }

  var props = [
    "http://songbirdnest.com/data/1.0#trackName",
    "http://songbirdnest.com/data/1.0#albumName",
    "http://songbirdnest.com/data/1.0#artistName",
    "http://songbirdnest.com/data/1.0#duration",
    "http://songbirdnest.com/data/1.0#genre",
    "http://songbirdnest.com/data/1.0#track",
    "http://songbirdnest.com/data/1.0#year",
    "http://songbirdnest.com/data/1.0#discNumber",
    "http://songbirdnest.com/data/1.0#totalDiscs",
    "http://songbirdnest.com/data/1.0#totalTracks",
    "http://songbirdnest.com/data/1.0#lastPlayTime",
    "http://songbirdnest.com/data/1.0#playCount"
  ];

  data = readFile("resource_properties.txt");
  a = data.split("\n");
  for(var i = 0; i < a.length - 1; i++) {
    var b = a[i].split("\t");
    var item = db[b[0]];
    item[props[parseInt(b[1]) - 1]] = b[2];
  }

  return db;
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
