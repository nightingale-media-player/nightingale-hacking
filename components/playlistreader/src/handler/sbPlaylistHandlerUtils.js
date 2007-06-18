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

Components.utils.import("resource://app/components/sbProperties.jsm");

const PR_RDONLY = -1;
const PR_FLAGS_DEFAULT = -1;

function SB_ProcessFile(aFile, aCallback, aThis) {

  var istream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  istream.init(aFile, PR_RDONLY, PR_FLAGS_DEFAULT, 0);
  istream.QueryInterface(Ci.nsILineInputStream);

  var line = {}, hasmore;
  do {
    hasmore = istream.readLine(line);
    aCallback.apply(aThis, [line.value]);
  } while(hasmore);

  istream.close();
}

function SB_AddItems(aItems, aMediaList, aAddDistinctOnly) {

  if (aItems.length == 0)
    return;

  function removeItemsByUri(items, uri) {
    for (var i = items.length - 1; i >= 0; i--) {
      if (items[i].uri.spec == uri)
        items.splice(i, 1);
    }
  }

  // If aAddDistinctOnly is true, remove all items from the aItems array that
  // are already in this list.  List membership is based on matching either
  // the #contentURL or #originURL properties
  if (aAddDistinctOnly) {

    // De-dup aItems by uri
    for (var i = 0; i < aItems.length - 1; i++) {
      var uri = aItems[i].uri;
      for (var j = i + 1; j < aItems.length; j++)
        if (aItems[j].uri.equals(uri))
          aItems.splice(j, 1);
    }

    // Remove all the items from aItems that have matching #contentURL
    // property values
    var propertyArray = SBProperties.createArray();
    aItems.forEach(function(e) {
      propertyArray.appendProperty(SBProperties.contentUrl, e.uri.spec);
    });

    var listener = {
      item: null,
      onEnumerationBegin: function() {
        return true;
      },
      onEnumeratedItem: function(list, item) {
        removeItemsByUri(aItems, item.contentSrc.spec);
        return true;
      },
      onEnumerationEnd: function() {
        return true;
      }
    };

    aMediaList.enumerateItemsByProperties(propertyArray,
                                          listener,
                                          Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

    // Remove all the items from aItems that have matching originUrl
    // property values
    if (aItems.length > 0) {
      propertyArray.clear();
      aItems.forEach(function(e) {
        propertyArray.appendProperty(SBProperties.originURL, e.uri.spec);
      });

      listener = {
        item: null,
        onEnumerationBegin: function() {
          return true;
        },
        onEnumeratedItem: function(list, item) {
          removeItemsByUri(aItems, item.getProperty(SBProperties.originURL));
          return true;
        },
        onEnumerationEnd: function() {
          return true;
        }
      };

      aMediaList.enumerateItemsByProperties(propertyArray,
                                            listener,
                                            Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

    }
  }

  // If any items are to be added, add them in a batch
  if (aItems.length > 0) {

    var uris = Cc["@mozilla.org/array;1"].createInstance(Ci.nsIMutableArray);
    aItems.forEach(function(e) {
      uris.appendElement(e.uri, false);
    });

    var added = aMediaList.library.batchCreateMediaItems(uris);
    for (var i = 0; i < added.length; i++) {
      aItems[i].item = added.queryElementAt(i, Ci.sbIMediaItem);
    }
  }

  // Set the properties on all the items
  aMediaList.beginUpdateBatch();
  try {
    aItems.forEach(function(e) {

      // XXXsteve This should be removed once the downloader sets this
      // property
      e.item.setProperty(SBProperties.originURL, e.item.contentSrc.spec);
      for (var prop in e.properties) {
        try {
          e.item.setProperty(prop, e.properties[prop]);
        }
        catch(e) {
          Components.utils.reportError(e);
        }
        e.item.write();
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
    _array: aItems,
    hasMoreElements: function() {
      return this._index < this._array.length;
    },
    getNext: function() {
      var item = this._array[this._index].item;
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

