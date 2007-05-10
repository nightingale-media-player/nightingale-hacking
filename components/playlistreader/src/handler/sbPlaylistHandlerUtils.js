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
 * \file sbPlaylistHandlerUtils.js
 */

const SB_NS = "http://songbirdnest.com/data/1.0#";

function SB_ProcessFile(aFile, aCallback, aThis) {

  var istream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  istream.init(aFile, 0x01, 0444, 0);
  istream.QueryInterface(Ci.nsILineInputStream);

  var line = {}, hasmore;
  do {
    hasmore = istream.readLine(line);
    aCallback.apply(aThis, [line.value]);
  } while(hasmore);

  istream.close();
}

function SB_AddItems(aItems, aMediaList, aReplace) {

  // A mapping of uris to media items.  We use this later to set the properties
  var uriStrToItem = {};

  // Generate a list of unique uri strings
  var temp = {};
  var uriStrToUri = {};
  var uniqueUriStr = [];
  aItems.forEach(function(e) {
    var uriStr = e.uri.spec;
    if (!(uriStr in temp)) {
      uniqueUriStr.push(uriStr);
      temp[uriStr] = 1;
      uriStrToUri[uriStr] = e.uri;
    }
  });

  // If we are to replace the metadata on existing items, find them by url
  var toCreate = [];
  if (aReplace) {

    var propertyArray =
      Cc["@songbirdnest.com/Songbird/Properties/PropertyArray;1"]
        .createInstance(Ci.sbIPropertyArray);

    // Track the uris of the items that we don't have in our database in
    // notFound.  Load it up with all the uris and then knock them out when
    // we enumerate them
    var notFound = {};

    uniqueUriStr.forEach(function(e) {
      propertyArray.appendProperty(SB_NS + "originUrl", e);
      notFound[e] = 1;
    });

    var listener = {
      item: null,
      onEnumerationBegin: function() {
        return true;
      },
      onEnumeratedItem: function(list, item) {
        var uriStr = item.contentSrc.spec;
        uriStrToItem[uriStr] = item;
        delete notFound[uriStr];
        return true;
      },
      onEnumerationEnd: function() {
        return true;
      }
    };

    aMediaList.enumerateItemsByProperties(propertyArray,
                                          listener,
                                          Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

    // The uris remaining in notFOund need to be created, so add them to the
    // create list
    for (var j in notFound) {
      toCreate.push(j);
    }
  }
  else {
    // If we are not replacing, create all the unique items
    toCreate = uniqueUriStr;
  }

  // Create the items in the create list and add the created items to the
  // uriStrToItem map
  if (toCreate.length > 0) {

    var uris = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    for (var i = 0; i < toCreate.length; i++) {
      uris.appendElement(uriStrToUri[toCreate[i]], false);
    }
    var added = aMediaList.library.batchCreateMediaItems(uris);
    for (var i = 0; i < added.length; i++) {
      var item = added.queryElementAt(i, Ci.sbIMediaItem);
      uriStrToItem[item.contentSrc.spec] = item;
    }
  }

  // Set the properties on all the items
  var toAdd = [];
  aMediaList.beginUpdateBatch();
  try {
    aItems.forEach(function(e) {

      var item = uriStrToItem[e.uri.spec];
      if (item) {
        toAdd.push(item);
        item.setProperty(SB_NS + "originUrl", e.uri.spec);
        for (var prop in e.properties) {
          try {
            item.setProperty(prop, e.properties[prop]);
          }
          catch(e) {
            Components.utils.reportError(e);
          }
        }
        item.write();
      }
      else {
        Components.utils.reportError("item with uri '" + e.uri.spec + "' was not added");
      }
    });
  }
  finally {
    aMediaList.endUpdateBatch();
  }

  // We also need to add the new items to the media list.  If the media list
  // is actually the library, this is essentially a no-op
  var enumerator = {
    _index: 0,
    _array: toAdd,
    hasMoreElements: function() {
      return this._index < this._array.length;
    },
    getNext: function() {
      var item = this._array[this._index];
      this._index++;
      return item;
    }
  };

  aMediaList.addSome(enumerator);
}

function SB_ResolveURI(aStringURL, aBaseURI)
{
  var isURI = false;

  var ios = Cc["@mozilla.org/network/io-service;1"]
              .getService(Ci.nsIIOService);

  // If there is no base URI or the base URI is a file, try the string as a
  // file
  if (aBaseURI == null || aBaseURI.scheme == "file") {
    try {
      var file = Cc["@mozilla.org/file/local;1"]
                   .createInstance(Ci.nsILocalFile);
      file.initWithPath(aStringURL);

      var uri = ios.newFileURI(file);
      return uri;
    }
    catch(e) {
      // If the base URI is a local file, try to use it to resolve the local
      // file path
      // XXXsteve: this does not work since setRelativeDescriptor does not know
      // if the leaf of the base path is a directory or a file.
/*
      if (aBaseURI && aBaseURI.scheme == "file") {
        try {
          var baseFile = aBaseURI.QueryInterface(Ci.nsIFileURL).file;
          var file = Cc["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsILocalFile);
          file.setRelativeDescriptor(baseFile, aStringURL);
          var uri = ios.newFileURI(file);
          return uri;
        }
        catch(e) {
          // fall through
        }
      }
*/
    }
  }

  // Ok, it is not a local file.  Try creating a new URI with the base URI
  try {
    var uri = ios.newURI(aStringURL, null, aBaseURI);
    return uri;
  }
  catch(e) {
    // fall through
  }

  // Couldn't resolve it, return null for failure
  return null;
}

