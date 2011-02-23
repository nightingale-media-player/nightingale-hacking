/*
 *=BEGIN SONGBIRD GPL
 *
 * This file is part of the Songbird web player.
 *
 * Copyright(c) 2005-2009 POTI, Inc.
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
 * \file sbM3UPlaylistWriter.js
 */
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://app/jsmodules/ArrayConverter.jsm");
Cu.import("resource://app/jsmodules/PlatformUtils.jsm");

var PR_WRONLY           = 0x02;
var PR_CREATE_FILE      = 0x08;
var PR_TRUNCATE         = 0x20;
var DEFAULT_PERMISSIONS = 0666;

function sbM3UPlaylistWriter() {
}

sbM3UPlaylistWriter.prototype = {
  classDescription:  "M3U Playlist Writer",
  classID:           Components.ID("{a11690c2-1dd1-11b2-82ba-8a4e72869f7a}"),
  contractID:        "@songbirdnest.com/Songbird/PlaylistWriter/M3U;1",
  QueryInterface:    XPCOMUtils.generateQI([Ci.sbIPlaylistWriter]),
  _xpcom_categories: [{category: "playlist-writer", entry: "m3u"}],

  write: function(aFile, aMediaList, aContentType, aPlaylistFormatType) {
    var foStream = Cc["@mozilla.org/network/file-output-stream;1"]
                     .createInstance(Ci.nsIFileOutputStream);

    // write, create, truncate
    foStream.init(aFile,
                  PR_WRONLY| PR_CREATE_FILE | PR_TRUNCATE,
                  DEFAULT_PERMISSIONS,
                  0);

    var buildM3U = {
      items: [],
      onEnumerationBegin: function(aMediaList) {},
      onEnumeratedItem: function(aMediaList, aMediaItem) {
        var uri = aMediaItem.contentSrc;
        // We don't allow non-file URIs to be written. At the moment.
        if (!uri || uri.scheme != "file") { return; }

        try {
          var f = uri.QueryInterface(Ci.nsIFileURL).file;
          f.QueryInterface(Ci.nsILocalFile);

          // For Windows, since we store the content source in lowercase,
          // get the case-sensitive path.
          var platform = PlatformUtils.platformString;
          if (platform == "Windows_NT") {
            var fileUtils = Cc["@songbirdnest.com/Songbird/FileUtils;1"]
                              .getService(Ci.sbIFileUtils);
            var exactPath = fileUtils.getExactPath(f.path);
            if (exactPath) {
              f.initWithPath(exactPath);
            }
          }

          // Show the directory containing the file and select the file.
          // The returned ACString is converted to UTF-8 encoding in the
          // getRelativeDescriptor method of nsLocalFileCommon.cpp*. Though
          // m3u files have not historically used UTF-8 (hence m3u8), encoding
          // in UTF-8 has worked properly thus far.
          //
          // * note: While the IDL explicitly states that the charset is
          //         undefined, the descriptor's charset will be UTF-8.
          //
          var data = f.getRelativeDescriptor(aFile.parent) + "\n";

          // Some devices are picky about path separators. If the playlist
          // format type specifies a path separator, ensure we are using
          // that separator.
          if (aPlaylistFormatType && aPlaylistFormatType.pathSeparator) {
            data = data.replace(/\//g, aPlaylistFormatType.pathSeparator);  
          }
          foStream.write(data, data.length);
        }
        catch (e) {
          Cu.reportError("Error writing M3U: " + e);
        }
      },
      onEnumerationEnd: function(aMediaList, aResultCode) {}
    };

    aMediaList.enumerateAllItems(buildM3U);
    foStream.close();
    buildM3U = null;
  },

  name: function() {
    return "Songbird M3U Writer";
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
