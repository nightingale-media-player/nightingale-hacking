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

//
// sbIPlaylistReaderManager Object
//
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function CPlaylistReaderManager()
{
  var catMan = Cc["@mozilla.org/categorymanager;1"]
                 .getService(Ci.nsICategoryManager);

  var readers = catMan.enumerateCategory("playlist-reader");
  while (readers.hasMoreElements()) {
    var entry = readers.getNext();
    entry = entry.QueryInterface(Ci.nsISupportsCString);
    var contractid = catMan.getCategoryEntry("playlist-reader", entry);

    try {
      var aReader = Cc[contractid].createInstance(Ci.sbIPlaylistReader);
      this.m_Readers.push(aReader);
    }
    catch(e) {
    }
  }

  // Cache the supported strings
  for(var i in this.m_Readers)
  {
    var nExtensionsCount = {};
    var aExts = this.m_Readers[i].supportedFileExtensions(nExtensionsCount);
    this.m_Extensions = this.m_Extensions.concat(aExts);
    var nMIMETypesCount = {};
    var aMIMETypes = this.m_Readers[i].supportedMIMETypes(nMIMETypesCount);
    this.m_MIMETypes = this.m_MIMETypes.concat(aMIMETypes);
  }

  var obs = Cc["@mozilla.org/observer-service;1"]
              .getService(Ci.nsIObserverService);
  obs.addObserver(this, "quit-application", false);
}

CPlaylistReaderManager.prototype.constructor = CPlaylistReaderManager;

CPlaylistReaderManager.prototype =
{
  classDescription: "Songbird Playlist Reader Manager Interface",
  classID:          Components.ID("{ced5902c-bd90-4099-acee-77487a5b1d13}"),
  contractID:       "@songbirdnest.com/Songbird/PlaylistReaderManager;1",

  originalURI: null,

  m_rootContractID: "@songbirdnest.com/Songbird/Playlist/Reader/",
  m_interfaceID: Components.interfaces.sbIPlaylistReader,
  m_Browser: null,
  m_Listeners: new Array(),
  m_Readers: new Array(),
  m_Extensions: new Array(),
  m_MIMETypes: new Array(),

  getTempFilename: function(aExtension)
  {
    var extension;
    if (aExtension)
      extension = aExtension;
    else
      extension = "tmp";
    var file = Components.classes["@mozilla.org/file/directory_service;1"]
                         .getService(Components.interfaces.nsIProperties)
                         .get("TmpD", Components.interfaces.nsIFile);
    file.append("songbird." + extension);
    file.createUnique(Components.interfaces.nsIFile.NORMAL_FILE_TYPE, 0664);

    return file.path;
  },

  getFileExtension: function(aPath)
  {
    var pos = aPath.lastIndexOf(".");
    if (pos >= 0) {
      return aPath.substr(pos + 1);
    }

    return null;
  },

  //sbIPlaylistReaderManager
  loadPlaylist: function(aURI, aMediaList, aContentType, aAddDistinctOnly, aPlaylistReaderListener)
  {
    const PlaylistReaderListener = new Components.Constructor("@songbirdnest.com/Songbird/PlaylistReaderListener;1", "sbIPlaylistReaderListener");

    var theExtension = this.getFileExtension(aURI.path);

    if (aURI instanceof Ci.nsIFileURL)
    {
      var file = aURI.QueryInterface(Ci.nsIFileURL).file;

      // Can't be a playlist if there is nothing in it
      if (!file.exists() || file.fileSize == 0) {
        return -1;
      }

      // If we are trying to load highly generic
      // content types, try to guess better ones
      if (aContentType == null ||
          aContentType == "" ||
          aContentType == "text/html" ||
          aContentType == "text/xml" ||
          aContentType == "text/plain" ||
          aContentType == "application/xhtml" ||
          aContentType == "application/xhtml+xml" ||
          aContentType == "application/xml") {
        aContentType = this.guessMimeType(file);
        if (!aContentType) {
          return -1;
        }
      }

      if (!this.originalURI) {
        this.originalURI = aURI;
      }

      try {
        this.read(file, aMediaList, aContentType, aAddDistinctOnly);
        if (aPlaylistReaderListener && aPlaylistReaderListener.observer) {
          aPlaylistReaderListener.observer.observe(aMediaList, "success", "");
        }
        return 1;
      }
      catch(e) {
        if (e.result == Cr.NS_ERROR_NOT_AVAILABLE) {
          return -1
        }
        throw e;
      }
      finally {
        this.originalURI = null;
      }

    }
    else
    {
      // Remember the original url.
      this.originalURI = aURI;

      var browser = Cc["@mozilla.org/embedding/browser/nsWebBrowserPersist;1"]
                      .createInstance(Ci.nsIWebBrowserPersist);

      if(!browser) return -1;
      this.m_Browser = browser;

      // Create a local file to save the remote playlist to
      var destFile = this.getTempFilename(theExtension);
      var localFile = Cc["@mozilla.org/file/local;1"]
                        .createInstance(Ci.nsILocalFile);
      localFile.initWithPath(destFile);

      var ios = Cc["@mozilla.org/network/io-service;1"]
                .getService(Ci.nsIIOService);
      var localFileUri = ios.newFileURI(localFile);

      var registerFileForDelete = Cc["@mozilla.org/uriloader/external-helper-app-service;1"]
                                    .getService(Ci.nsPIExternalAppLauncher);
      registerFileForDelete.deleteTemporaryFileOnExit(localFile);

      // cycle through the listener array and remove any that are done
      for (var index = 0; index < this.m_Listeners.length; index++) {
        var foo = this.m_Listeners[index];
        if (foo.state && foo.state.indexOf("STOP") != -1) {
          this.m_Listeners.splice(index, 1);
          delete foo;
          index = 0;
        }
      }

      var prListener = null;
      if(aPlaylistReaderListener)
      {
        prListener = aPlaylistReaderListener;
      }
      else
      {
        prListener = (new PlaylistReaderListener()).QueryInterface(Ci.sbIPlaylistReaderListener);
      }

      prListener.originalURI = this.originalURI;
      prListener.mediaList = aMediaList;
      prListener.destinationURI = localFileUri;
      prListener.addDistinctOnly = aAddDistinctOnly;

//      this.m_Browser.persistFlags |= 2; // PERSIST_FLAGS_BYPASS_CACHE;

      this.m_Browser.progressListener = prListener;
      this.m_Listeners.push(prListener);

      this.m_Browser.saveURI(aURI, null, null, null, "", localFileUri);

      return 1;
    }

    return 1;
  },

  read: function(aFile, aMediaList, aContentType, aAddDistinctOnly)
  {
    var theExtension = this.getFileExtension(aFile.path);
    for (var r in this.m_Readers)
    {
      var aReader = this.m_Readers[r];
      if(!aContentType)
      {
        var nExtensionsCount = {};
        var theExtensions = aReader.supportedFileExtensions(nExtensionsCount);

        if (SB_ArrayContains(theExtensions, theExtension)) {

          // Handoff the original url
          aReader.originalURI = this.originalURI;
          this.originalURI = null;

          aReader.read(aFile, aMediaList, aAddDistinctOnly);
          return;
        }
      }
      else
      {
        var nMIMTypeCount = {};
        var theMIMETypes = aReader.supportedMIMETypes(nMIMTypeCount);

        if (SB_ArrayContains(theMIMETypes, aContentType)) {

          // Handoff the original url
          aReader.originalURI = this.originalURI;
          this.originalURI = null;

          aReader.read(aFile, aMediaList, aAddDistinctOnly);
          return;
        }
      }
    }

    // Couldn't handle it so throw
    throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  supportedFileExtensions: function(nExtCount)
  {
    nExtCount.value = this.m_Extensions.length;
    return this.m_Extensions;
  },

  supportedMIMETypes: function(nMIMECount)
  {
    nMIMECount.value = this.m_MIMETypes.length;
    return this.m_MIMETypes;
  },

  guessMimeType: function(file)
  {
    // Read a few bytes from the file
    var fstream = Components.classes["@mozilla.org/network/file-input-stream;1"]
                            .createInstance(Components.interfaces.nsIFileInputStream);
    var sstream = Components.classes["@mozilla.org/scriptableinputstream;1"]
                            .createInstance(Components.interfaces.nsIScriptableInputStream);
    fstream.init(file, -1, 0, 0);
    sstream.init(fstream);
    var str = sstream.read(4096);
    sstream.close();
    fstream.close();

    // This object literal maps mime types to regexps.  If the content matches
    // the given regexp, it is assigned the given mime type.
    var regexps = [
      {
        regexp:    /^<\?xml(.|\n)*<rss/,
        mimeType: "application/rss+xml"
      },
      {
        regexp:    /^<\?xml(.|\n)*xmlns="http:\/\/www\.w3\.org\/2005\/Atom"/,
        mimeType: "application/atom+xml"
      },
      {
        regexp:    /^\[playlist\]/i,
        mimeType: "audio/x-scpls"
      },
      {
        regexp:    /^#EXTM3U/i,
        mimeType: "audio/mpegurl"
      },
      {
        regexp:    /<html/i,
        mimeType: "text/html"
      },
      {
        regexp:    /^(http|mms|rtsp)/im,
        mimeType: "audio/mpegurl"
      },
      {
        regexp:    /.*(mp3|ogg|flac|wav|m4a|wma|wmv|asx|asf|avi|mov|mpg|mp4)$/im,
        mimeType: "audio/mpegurl"
      },
      {
        regexp:    /^<ASX/i,
        mimeType: "video/x-ms-asf"
      }
    ];

    for(var i = 0; i < regexps.length; i++) {
      var re = regexps[i].regexp;
      if(re.test(str)) {
        return regexps[i].mimeType;
      }
    }

    // Otherwise, we have no guess
    return null;
  },

  observe: function(aSubject, aTopic, aData) {
    var obs = Cc["@mozilla.org/observer-service;1"]
                .getService(Ci.nsIObserverService);
    obs.removeObserver(this, "quit-application");

    for (let i in this.m_Readers) {
      this.m_Readers[i] = null;
    }
    for (let i in this.m_Listeners) {
      this.m_Listeners[i] = null;
    }
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.sbIPlaylistReaderManager])
};

function SB_ArrayContains(a, v) {
  return a.some(function(e) { return e == v; } );
}

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([CPlaylistReaderManager]);
}
