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
 * \brief Some globally useful stuff for the playlistreader tests
 */
function getFile(fileName) {
  var file = Cc["@mozilla.org/file/directory_service;1"]
               .getService(Ci.nsIProperties)
               .get("resource:app", Ci.nsIFile);
  file = file.clone();
  file.append("testharness");
  file.append("playlistreader");
  file.append(fileName);
  return file;
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
      .getService(Ci.sbILibraryFactory);
  var hashBag = Cc["@mozilla.org/hash-property-bag;1"].
                createInstance(Ci.nsIWritablePropertyBag2);
  hashBag.setPropertyAsInterface("databaseFile", file);
  var library = libraryFactory.createLibrary(hashBag);
  library.clear();
  return library;
}

function newURI(spec) {
  var ioService = Cc["@mozilla.org/network/io-service;1"].
                  getService(Ci.nsIIOService);
  
  return ioService.newURI(spec, null, null);
}

function assertMediaList(aMediaList, aItemTestsFile, aPort) {

  var xmlReader = Cc["@mozilla.org/saxparser/xmlreader;1"]
                    .createInstance(Ci.nsISAXXMLReader);

  var failMessage = null;

  xmlReader.contentHandler = {
    _item: null,
    startDocument: function() {
    },

    endDocument: function() {
    },

    startElement: function(uri, localName, qName, /*nsISAXAttributes*/ attributes) {
      if (failMessage)
        return;

      if (localName == "item-test") {
        var prop  = attributes.getValueFromName("", "property");
        if (prop.indexOf("#") == 0) {
          prop = "http://songbirdnest.com/data/1.0" + prop;
        }

        var value = attributes.getValueFromName("", "value");
        value = value.replace("%PORT%", aPort);

        this._item = getFirstItemByProperty(aMediaList, prop, value);
        if (!this._item) {
          failMessage = "item with property '" + prop + "' equal to '" + value + "' not found";
        }
      }

      if (localName == "assert-property-value" && this._item) {
        var prop  = attributes.getValueFromName("", "name");
        if (prop.indexOf("#") == 0) {
          prop = "http://songbirdnest.com/data/1.0" + prop;
        }

        var value = attributes.getValueFromName("", "value");
        value = value.replace("%PORT%", aPort);

        var itemValue = this._item.getProperty(prop);

        if (itemValue != value) {
          failMessage = "item with url '" + this._item.contentSrc.spec +
                        "' does not match result at property '" + prop +
                        "', '" + itemValue + "' != '" + value + "'";
        }
      }
    },

    endElement: function(uri, localName, qName) {
    },

    characters: function(value) {
    },

    processingInstruction: function(target, data) {
    },

    ignorableWhitespace: function(whitespace) {
    },

    startPrefixMapping: function(prefix, uri) {
    },

    endPrefixMapping: function(prefix) {
    },

    // nsISupports
    QueryInterface: function(iid) {
      if(!iid.equals(Ci.nsISupports) &&
         !iid.equals(Ci.nsISAXContentHandler))
        throw Cr.NS_ERROR_NO_INTERFACE;
      return this;
    }
  };

  var istream = Cc["@mozilla.org/network/file-input-stream;1"]
                  .createInstance(Ci.nsIFileInputStream);
  istream.init(aItemTestsFile, 0x01, 0444, 0);
  xmlReader.parseFromStream(istream, null, "text/xml");

  // Need this to prevent the closure from leaking
  xmlReader.contentHandler = null;

  if (failMessage) {
    fail(failMessage);
  }
}

function getFirstItemByProperty(aMediaList, aProperty, aValue) {

  var listener = {
    item: null,
    onEnumerationBegin: function() {
    },
    onEnumeratedItem: function(list, item) {
      this.item = item;
      return Components.interfaces.sbIMediaListEnumerationListener.CANCEL;
    },
    onEnumerationEnd: function() {
    }
  };

  aMediaList.enumerateItemsByProperty(aProperty,
                                      aValue,
                                      listener,
                                      Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  return listener.item;
}

