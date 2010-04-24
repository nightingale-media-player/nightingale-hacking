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

function makeArray(library) {
  var ldl = library.QueryInterface(Ci.sbILocalDatabaseLibrary);
  var array = Cc["@songbirdnest.com/Songbird/Library/LocalDatabase/GUIDArray;1"]
                .createInstance(Ci.sbILocalDatabaseGUIDArray);
  array.databaseGUID = ldl.databaseGuid;
  array.propertyCache = ldl.propertyCache;
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
    if(array.getGuidByIndex(i) != b[0]) {
      fail("sort failed, index " + i + " got " + array.getGuidByIndex(i) + " expected " + b[0]);
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
  _added: [],
  _removedBefore: [],
  _removedAfter: [],
  _updatedItem: null,
  _updatedProperties: null,
  _movedItemFromIndex: [],
  _movedItemToIndex: [],
  _batchBeginList: null,
  _batchEndList: null,
  _listCleared: false,
  _retval: false,

  get added() {
    log("_added.length=" + this._added.length);
    return this._added;
  },

  get removedBefore() {
    return this._removedBefore;
  },

  get removedAfter() {
    return this._removedAfter;
  },

  get updatedItem() {
    return this._updatedItem;
  },

  get updatedProperties() {
    return this._updatedProperties;
  },

  get movedItemFromIndex() {
    return this._movedItemFromIndex;
  },

  get movedItemToIndex() {
    return this._movedItemToIndex;
  },

  get batchBeginList() {
    return this._batchBeginList;
  },

  get batchEndList() {
    return this._batchEndList;
  },

  get listCleared() {
    return this._listCleared;
  },

  set retval(value) {
    this._retval = value;
  },
  reset: function reset() {
    this._added = [];
    this._removedBefore = [];
    this._removedAfter = [];
    this._updatedItem = null;
    this._updatedProperties = null;
    this._movedItemFromIndex = [];
    this._movedItemToIndex = [];
    this._batchBeginList = null;
    this._batchEndList = null;
    this._listCleared = false;
    this._retval = false;
  },

  onItemAdded: function onItemAdded(list, item, index) {
    this._added.push({list: list, item: item, index: index});
    return this._retval;
  },

  onBeforeItemRemoved: function onBeforeItemRemoved(list, item, index) {
    this._removedBefore.push({list: list, item: item, index: index});
    return this._retval;
  },

  onAfterItemRemoved: function onAfterItemRemoved(list, item, index) {
    this._removedAfter.push({list: list, item: item, index: index});
    return this._retval;
  },

  onItemUpdated: function onItemUpdated(list, item, properties) {
    this._updatedItem = item;
    this._updatedProperties = properties;
    return this._retval;
  },

  onItemMoved: function onItemMoved(list, fromIndex, toIndex) {
    this._movedItemFromIndex.push(fromIndex);
    this._movedItemToIndex.push(toIndex);
    return this._retval;
  },

  onBatchBegin: function onBatchBegin(list) {
    this._batchBeginList = list;
  },

  onBatchEnd: function onBatchEnd(list) {
    this._batchEndList = list;
  },
  // XXX: implement the tests for this function
  onBeforeListCleared: function onBeforeListCleared() {
    return this._retval;
  },
  onListCleared: function onListCleared() {
    this._listCleared = true;
    return this._retval;
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
    if (this._enumBeginFunction) {
      return this._enumBeginFunction(list);
    }

    return Ci.sbIMediaListEnumerationListener.CONTINUE;
  },

  onEnumeratedItem: function onEnumeratedItem(list, item) {
    var retval = Ci.sbIMediaListEnumerationListener.CONTINUE;
    if (this._enumItemFunction)
      retval = this._enumItemFunction(list, item);

    if (retval == Ci.sbIMediaListEnumerationListener.CONTINUE) {
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

function TestMediaListViewListener() {
}
TestMediaListViewListener.prototype = {
  _filterChanged: false,
  _searchChanged: false,
  _sortChanged: false,

  get filterChanged() {
    return this._filterChanged;
  },

  get searchChanged() {
    return this._searchChanged;
  },

  get sortChanged() {
    return this._sortChanged;
  },

  reset: function TMLVL_reset() {
    this._filterChanged = false;
    this._searchChanged = false;
    this._sortChanged = false;
  },

  onFilterChanged: function TMLVL_onFilterChanged(view) {
    this._filterChanged = true;
  },

  onSearchChanged: function TMLVL_onSearchChanged(view) {
    this._searchChanged = true;
  },

  onSortChanged: function TMLVL_onSortChanged(view) {
    this._sortChanged = true;
  },

  QueryInterface: function TMLVL_QueryInterface(iid) {
    if (!iid.equals(Components.interfaces.sbIMediaListViewListener) &&
        !iid.equals(Components.interfaces.nsISupportsWeakReference))
      throw Components.results.NS_ERROR_NO_INTERFACE;
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
    "http://songbirdnest.com/data/1.0#GUID",
    "http://songbirdnest.com/data/1.0#created",
    "http://songbirdnest.com/data/1.0#updated",
    "http://songbirdnest.com/data/1.0#contentURL",
    "http://songbirdnest.com/data/1.0#contentType",
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
      if (sp[j] == "http://songbirdnest.com/data/1.0#isList") {
        if (!b[j + 1] || b[j + 1] == "0") {
          item[sp[j]] = "0";
        }
        else {
          item[sp[j]] = "1";
          item["http://songbirdnest.com/data/1.0#listType"] = "1";
        }
      }
      else {
        item[sp[j]] = b[j + 1];
      }
    }
  }

  var props = [
    "http://songbirdnest.com/data/1.0#trackName",
    "http://songbirdnest.com/data/1.0#albumName",
    "http://songbirdnest.com/data/1.0#artistName",
    "http://songbirdnest.com/data/1.0#duration",
    "http://songbirdnest.com/data/1.0#genre",
    "http://songbirdnest.com/data/1.0#trackNumber",
    "http://songbirdnest.com/data/1.0#year",
    "http://songbirdnest.com/data/1.0#discNumber",
    "http://songbirdnest.com/data/1.0#totalDiscs",
    "http://songbirdnest.com/data/1.0#totalTracks",
    "http://songbirdnest.com/data/1.0#lastPlayTime",
    "http://songbirdnest.com/data/1.0#playCount",
    "http://songbirdnest.com/data/1.0#customType",
    "http://songbirdnest.com/data/1.0#isSortable"
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

function execQuery(databaseGuid, sql) {

  var dbq = Cc["@songbirdnest.com/Songbird/DatabaseQuery;1"]
              .createInstance(Ci.sbIDatabaseQuery);

  dbq.setDatabaseGUID(databaseGuid);
  dbq.setAsyncQuery(false);
  dbq.addQuery(sql);
  dbq.execute();

  var result = dbq.getResultObject();
  var rows = result.getRowCount();
  var cols = result.getColumnCount();

  var d = [];
  for (var i = 0; i < rows; i++) {
    var r = [];
    for (var j = 0; j < cols; j++) {
      r.push(result.getRowCell(i, j));
    }
    d.push(r);
  }
  return d;
}
