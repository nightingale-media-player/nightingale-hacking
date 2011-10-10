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

/**
 * \file sbM3UPlaylistWriter.js
 */
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://app/jsmodules/ArrayConverter.jsm");
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var PR_WRONLY           = 0x02;
var PR_CREATE_FILE      = 0x08;
var PR_TRUNCATE         = 0x20;
var DEFAULT_PERMISSIONS = 0666;

function sbM3UPlaylistWriter() {
}

sbM3UPlaylistWriter.prototype = {
  classDescription:  "M3U Playlist Writer",
  classID:           Components.ID("{a11690c2-1dd1-11b2-82ba-8a4e72869f7a}"),
  contractID:        "@getnightingale.com/Nightingale/PlaylistWriter/M3U;1",
  QueryInterface:    XPCOMUtils.generateQI([Ci.sbIPlaylistWriter]),
  _xpcom_categories: [{category: "playlist-writer", entry: "m3u"}],

  write: function(aFile, aMediaList, aContentType, aPlaylistFormatType) {
    var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                     .createInstance(Ci.nsIFileOutputStream);

    // write, create, truncate
    foStream.init(aFile, PR_WRONLY| PR_CREATE_FILE | PR_TRUNCATE, DEFAULT_PERMISSIONS, 0);

    // if you are sure there will never ever be any non-ascii text in data you can
    // also call foStream.writeData directly
    var converter = Cc["@mozilla.org/intl/converter-output-stream;1"]
                      .createInstance(Ci.nsIConverterOutputStream);
    converter.init(foStream, "UTF-8", 0, 0);

    var buildM3U = {
      items: [],
      onEnumerationBegin: function(aMediaList) {},
      onEnumeratedItem: function(aMediaList, aMediaItem) {
        var uri = aMediaItem.contentSrc;
        // We don't allow non-file URIs to be written. At the moment.
        if (!uri || uri.scheme != "file") { return; }
        var f = uri.QueryInterface(Ci.nsIFileURL).file;

        try {
          // Show the directory containing the file and select the file
          f.QueryInterface(Ci.nsILocalFile);
          var data = f.getRelativeDescriptor(aFile.parent) + "\n";

          // Some devices are picky about path separators. If the playlist
          // format type specifies a path separator, ensure we are using
          // that separator.
          if (aPlaylistFormatType && aPlaylistFormatType.pathSeparator) {
            data = data.replace(/\//g, aPlaylistFormatType.pathSeparator);  
          }
          converter.writeString(data);
        }
        catch (e) {
          Components.utils.reportError("Error writing M3U: " + e);
        }
      },
      onEnumerationEnd: function(aMediaList, aResultCode) {}
    };

    aMediaList.enumerateAllItems(buildM3U);
    converter.close(); // this closes foStream
    buildM3U = null;
  },

  name: function() {
    return "Nightingale M3U Writer";
  },

  description: function() {
    return "Write M3U playlists";
  },

  supportedMIMETypes: function(aMIMECount) {
    var mimeTypes = ["audio/mpegurl", "audio/x-mpegurl"];
    aMIMECount.value = mimeTypes.length;
    return mimeTypes;
  },

  supportedFileExtensions: function(aExtCount) {
    var exts = ["m3u"];
    aExtCount.value = exts.length;
    return exts;
  },
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([sbM3UPlaylistWriter]);
}
