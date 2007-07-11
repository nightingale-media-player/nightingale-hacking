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
      .createInstance(Ci.sbILibraryFactory);
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

function assertMediaList(aMediaList, aItemTestsFile) {

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
        var value = attributes.getValueFromName("", "value");
        if (prop.indexOf("#") == 0) {
          prop = "http://songbirdnest.com/data/1.0" + prop;
        }

        this._item = getFirstItemByProperty(aMediaList, prop, value);
        if (!this._item) {
          failMessage = "item with property '" + prop + "' equal to '" + value + "' not found";
        }
      }

      if (localName == "assert-property-value" && this._item) {
        var prop  = attributes.getValueFromName("", "name");
        var value = attributes.getValueFromName("", "value");
        if (prop.indexOf("#") == 0) {
          prop = "http://songbirdnest.com/data/1.0" + prop;
        }
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
      return true;
    },
    onEnumeratedItem: function(list, item) {
      this.item = item;
      return false;
    },
    onEnumerationEnd: function() {
      return true;
    }
  };

  aMediaList.enumerateItemsByProperty(aProperty,
                                      aValue,
                                      listener,
                                      Ci.sbIMediaList.ENUMERATIONTYPE_LOCKING);

  return listener.item;
}

function mockCore () {
  this._metadata = {
    artist: "Van Halen",
    album: "Women and Children First",
    title: "Everybody wants some!"
  }
/*
  this._mediaUrlExtensions = ["mp3", "ogg", "flac", "mpc", "wav", "m4a", "m4v",
                              "wmv", "asf", "avi",  "mov", "mpg", "mp4", "ogm",
                              "mp2", "mka", "mkv"];
  this._mediaUrlSchemes = ["mms", "rstp"];
  this._videoUrlExtensions = ["wmv", "asf", "avi", "mov", "mpg", "m4v", "mp4",
                              "mp2", "mpeg", "mkv", "ogm"];
*/
  this._videoUrlMatcher = new ExtensionSchemeMatcher(this._videoUrlExtensions, []);
  this._mediaUrlMatcher = new ExtensionSchemeMatcher(this._mediaUrlExtensions,
                                                     this._mediaUrlSchemes);
}

mockCore.prototype = {
  _name: "mockCore",
  _id: "mockCore",
  _mute: false,
  _playing: false,
  _paused: false,
  _video: false,
  _volume: 128, 
  _length: 25000,
  _position: 0,
  _metadata: null,
  _active: false,
  _mediaUrlExtensions: ["mp3", "ogg", "flac", "mpc", "wav", "m4a", "m4v",
                        "wmv", "asf", "avi",  "mov", "mpg", "mp4", "ogm",
                        "mp2", "mka", "mkv"],
  _mediaUrlSchemes: ["mms", "rstp"],
  _videoUrlExtensions: ["wmv", "asf", "avi", "mov", "mpg", "m4v", "mp4", "mp2",
                        "mpeg", "mkv", "ogm"],

  QueryInterface: function(iid) {
    if (!iid.equals(Ci.sbICoreWrapper) &&
        !iid.equals(Ci.nsISupports))
      throw Cr.NS_ERROR_NO_INTERFACE;
    return this;
  },
  getPaused: function() { return this._paused; },
  getPlaying: function() { return this._playing; },
  getPlayingVideo: function() { return this._video; },
  getMute: function() { return this._mute; },
  setMute: function(mute) { this._mute = mute; },
  getVolume: function() { return this._volume; },
  setVolume: function(vol) { this._volume = vol },
  getLength: function() { return this._length; },
  getPosition: function() { return this._position; },
  setPosition: function(pos) { this._position = pos; },
  goFullscreen: function() { },
  getId: function() { return this.id; },
  getObject: function() { return null; },
  setObject: function(ele) { },
  playURL: function(url) { return true; },
  play: function() { return true; },
  stop: function() { return true; },
  pause: function() { return true; },
  prev: function() { return true; },
  next: function() { return true; },
  getMetadata: function(key) { return this._metadata[key]; },
  isMediaURL: function(url) { return this._mediaUrlMatcher.match(url); },
  isVideoURL: function(url) { return this._videoUrlMatcher.match(url); },
  getSupportedFileExtensions: function() { return new StringArrayEnumerator(this._mediaUrlExtensions); },
  activate: function() { this._active = true; },
  deactivate: function() { this._active = false; },
  getActive: function(url) { return this._active; },
  getSupportForFileExtension: function(extension) {
    if (extension == ".mp3")
      return 1;
    else
      return -1;
  }
};

// copied from coreBase to hand back an enumerator
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

function initMockCore() {
  var pps = Cc["@songbirdnest.com/Songbird/PlaylistPlayback;1"]
              .getService(Ci.sbIPlaylistPlayback);
  var myCore = new mockCore();
  pps.addCore(myCore, true);
}

