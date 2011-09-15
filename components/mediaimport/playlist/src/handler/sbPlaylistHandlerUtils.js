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
 * \file sbPlaylistHandlerUtils.js
 */

Components.utils.import("resource://app/jsmodules/sbProperties.jsm");

const PR_RDONLY = -1;
const PR_FLAGS_DEFAULT = -1;

// the minimum number of characters to feed into the charset detector
const GUESS_CHARSET_MIN_CHAR_COUNT = 256;

/**
 * \brief Process the file aFile line by line with the callback function
 *        aCallback.
 *
 * \param aFile     The file to be processed.
 * \param aCallback The callback function that will be used to process the file.
 * \param aThis     User defined data for the callback function.
 */

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

/**
 * \brief Detect the charset of the file aFile, convert the file encode to the
 *        detected one, and process the file line by line with the callback
 *        function aCallback.
 *
 * \param aFile     The file to be detected and processed.
 * \param aCallback The callback function that will be used to process the file.
 * \param aThis     User defined data for the callback function.
 */

function SB_DetectCharsetAndProcessFile(aFile, aCallback, aThis) {

  var istream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  istream.init(aFile, PR_RDONLY, PR_FLAGS_DEFAULT, 0);
  istream.QueryInterface(Ci.nsILineInputStream);

  var detector = Cc["@songbirdnest.com/Songbird/CharsetDetector;1"]
                   .createInstance(Ci.sbICharsetDetector);
  var line = {}, hasmore, charset;
  var value = "";
  var length = 0;
  try {
    do {
      hasmore = istream.readLine(line);
      value = line.value;

      // Blank can be ignored.
      if (value == "")
        continue;

      // Send the file content for detection, until we get the best value.
      detector.detect(value);

      length += value.length;
    } while(hasmore &&
            !detector.isCharsetFound &&
            length < GUESS_CHARSET_MIN_CHAR_COUNT);
    charset = detector.finish();
  }
  catch (ex) {
    dump("charset detection error in SB_DetectCharsetAndProcessFile: " +
         ex + "\n");
  }

  istream.close();

  var fstream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  fstream.init(aFile, PR_RDONLY, PR_FLAGS_DEFAULT, 0);
  fstream.QueryInterface(Ci.nsILineInputStream);
  var unicodeConverter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                           .createInstance(Ci.nsIScriptableUnicodeConverter);
  // Re-read the file with the charset found.
  do {
    hasmore = fstream.readLine(line);
    value = line.value;

    // Blank can be ignored for all playlist handlers.
    if (value == "")
      continue;

    try {
      unicodeConverter.charset = charset ? charset : "ISO-8859-1";
      value = unicodeConverter.ConvertToUnicode(value);
    }
    catch (ex) {
      dump("Unicode conversion error in SB_DetectCharsetAndProcessFile: " +
           ex + "\n");
    }

    aCallback.apply(aThis, [value]);
  } while(hasmore);

  fstream.close();
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
      propertyArray.appendProperty(SBProperties.contentURL, e.uri.spec);
    });

    var listener = {
      item: null,
      onEnumerationBegin: function() {
      },
      onEnumeratedItem: function(list, item) {
        removeItemsByUri(aItems, item.contentSrc.spec);
      },
      onEnumerationEnd: function() {
      }
    };

    aMediaList.enumerateItemsByProperties(propertyArray,
                                          listener );

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
        },
        onEnumeratedItem: function(list, item) {
          removeItemsByUri(aItems, item.getProperty(SBProperties.originURL));
        },
        onEnumerationEnd: function() {
        }
      };

      aMediaList.enumerateItemsByProperties(propertyArray,
                                            listener );

    }
  }

  // If any items are to be added, add them in a batch
  if (aItems.length > 0) {

    // Create the array of media item content URIs.
    var uris = Cc["@songbirdnest.com/moz/xpcom/threadsafe-array;1"]
                 .createInstance(Ci.nsIMutableArray);
    var libraryUtils = Cc["@songbirdnest.com/Songbird/library/Manager;1"]
                         .getService(Ci.sbILibraryUtils);
    aItems.forEach(function(e) {
      uris.appendElement(libraryUtils.getContentURI(e.uri), false);
    });

    // Create the media items.
    var resultItems = {};
    var created = aMediaList.library.batchCreateMediaItemsIfNotExist
                                       (uris,
                                        null,
                                        resultItems);
    resultItems = resultItems.value;

    for (var i = 0; i < aItems.length; i++) {
      aItems[i].item = resultItems.queryElementAt(i, Ci.sbIMediaItem);
      if (created.queryElementAt(i, Ci.nsIVariant))
        aItems[i].created = true;
    }
  }

  // Set the properties on all the newly created items
  aMediaList.runInBatchMode(function() {
    aItems.forEach(function(e) {
      if (e.created) {
        for (var prop in e.properties) {
          try {
            e.item.setProperty(prop, e.properties[prop]);
          }
          catch(e) {
            Components.utils.reportError(e);
          }
        }
      }
    });
  });

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
    if ( uri instanceof Ci.nsIFileURL && !uri.file.exists() ) {
      return null;
    } else {
      return uri;
    }
  }
  catch(e) {
    // fall through
  }

  // Couldn't resolve it, return null for failure
  return null;
}
