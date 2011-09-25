/*
 *=BEGIN NIGHTINGALE GPL
 *
 * This file is part of the Nightingale web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
 * http://www.getnightingale.com
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
 *=END NIGHTINGALE GPL
 */

//
// sbIPlaylistWriterManager Object
//
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

function sbPlaylistWriterManager()
{
  if (this._writers) {
    // The class-stored _writers list is already initialized.
    return;
  }

  // At creation, initialize a list of playlist writers
  var catMan = Cc["@mozilla.org/categorymanager;1"]
                 .getService(Ci.nsICategoryManager);

  var category = catMan.enumerateCategory("playlist-writer");

  this._writers = [];
  for (var entry in ArrayConverter.JSEnum(category)) {
    entry = entry.QueryInterface(Ci.nsISupportsCString);
    var writer = Cc[catMan.getCategoryEntry("playlist-writer", entry.data)]
                   .createInstance(Ci.sbIPlaylistWriter);
    this._writers.push(writer);
  }

  // Cache the supported strings.
  this._extensions = [];
  this._MIMETypes = [];
  for(var i in this._writers) {
    this._extensions = this._extensions.concat(
      this._writers[i].supportedFileExtensions({}));
    this._MIMETypes = this._MIMETypes.concat(
      this._writers[i].supportedMIMETypes({}));
  }
};

sbPlaylistWriterManager.prototype.constructor = sbPlaylistWriterManager;
sbPlaylistWriterManager.prototype = {
  classDescription: "Nightingale Playlist Writer Manager Interface",
  classID:          Components.ID("{9d66cb86-951e-49f4-bef8-26a578891275}"),
  contractID:       "@getnightingale.com/Nightingale/PlaylistWriterManager;1",
  QueryInterface:   XPCOMUtils.generateQI([Ci.sbIPlaylistWriterManager]),

  _writers: null,
  _extensions: null,
  _MIMETypes: null,

  //sbIPlaylistWriterManager
  writePlaylist: function(aURI, aMediaList, aContentType, aPlaylistWriterListener)
  {
    if (aURI instanceof Ci.nsIFileURL) {
      var file = aURI.QueryInterface(Ci.nsIFileURL).file;

      try {
        this.write(file, aMediaList, aContentType, aAddDistinctOnly);
        if (aPlaylistWriterListener && aPlaylistWriterListener.observer) {
          aPlaylistWriterListener.observer.observe(aMediaList, "success", "");
        }
        return 1;
      }
      catch(e if e.result == Cr.NS_ERROR_NOT_AVAILABLE) {
        return -1;
      }
    }

    return 1;
  },

  write: function(aFile, aMediaList, aContentType, aPlaylistFormatType)
  {
    var theExtension = this._getFileExtension(aFile);
    if (!this._writers) {
      this._cacheWriters();
    }

    for (var r in this._writers) {
      var aWriter = this._writers[r];
      var theExtensions = aWriter.supportedFileExtensions({});
      var theMIMETypes = aWriter.supportedMIMETypes({});

      // Only consider file extensions if we have no content-type specified.
      if(theMIMETypes.some(function (a) { return a == aContentType } ) ||
        (!aContentType && theExtensions.some(
            function(a) { return a == theExtension }))) {
        aWriter.write(aFile, aMediaList, aContentType, aPlaylistFormatType);
        return;
      }
    }

    // Couldn't handle it so throw
    throw Cr.NS_ERROR_NOT_AVAILABLE;
  },

  supportedFileExtensions: function(nExtCount)
  {
    if (!this._writers) {
      this._cacheWriters();
    }
    nExtCount.value = this._extensions.length;
    return this._extensions;
  },

  supportedMIMETypes: function(nMIMECount)
  {
    if (!this._writers) {
      this._cacheWriters();
    }
    nMIMECount.value = this._MIMETypes.length;
    return this._MIMETypes;
  },

  /**
   * Extract the file extension from an {nsIUR{I,L}, nsIFile}
   */
  _getFileExtension: function(aThing)
  {
    var name = "";
    if (aThing instanceof Ci.nsIURL) {
      if ("" != aThing.fileExtension) {
        return aThing.fileExtension;
      }
    }
    if (aThing instanceof Ci.nsIURI) {
      name = aThing.path;
    }
    if (aThing instanceof Ci.nsIFile) {
      name = aThing.leafName;
    }
    // find the file extension
    var m = /\.([^\.\/]+)$/(name);
    if (m) {
      return m[1];
    } else {
      return null;
    }
  },
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbPlaylistWriterManager]);
}
