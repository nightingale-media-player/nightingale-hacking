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
 * \file sbM3UPlaylistHandler.js
 */
const TOKEN_EXTM3U  = "#EXTM3U";
const TOKEN_EXTINF  = "#EXTINF:";
const TOKEN_COMMENT = "#";

function sbM3UPlaylistHandler() {
  this._originalURI = null;
}

// sbIPlaylistReader
sbM3UPlaylistHandler.prototype.__defineGetter__("originalURI",
function()
{
  return this._originalURI;
});

sbM3UPlaylistHandler.prototype.__defineSetter__("originalURI",
function(value)
{
  this._originalURI = value;
});

sbM3UPlaylistHandler.prototype.read =
function(aFile, aMediaList, aReplace)
{
  var nextFileMetadata = {};
  var toAdd = [];

  // Match TOKEN_EXTINF then all characters that are not commas, then an
  // optional comma, then capture the rest of the string
  var re = new RegExp("^" + TOKEN_EXTINF + "([^,]*),?(.*)?$");

  var self = this;
  SB_ProcessFile(aFile, function(aLine) {
    // Skip the TOKEN_EXTM3U header and blank lines
    if (aLine == TOKEN_EXTM3U || aLine == "")
      return;

    // If we get a TOKEN_EXTINF line, parse and store the data for the next
    // file
    var a = aLine.match(re);
    if (a) {
      var duration = parseInt(a[1], 10);
      if (!isNaN(duration) && duration >= 0)
        nextFileMetadata.duration = duration;
      if (a[2] != "")
        nextFileMetadata.title = a[2];
    }

    // Skip any other line starting with the comment token
    if (aLine.indexOf(TOKEN_COMMENT) == 0) {
      return;
    }

    // Otherwise, this line is a URL.  Add it to the list
    var newUri = SB_ResolveURI(aLine, this._originalURI);
    if (newUri) {
      var item = { uri: newUri, properties: {}};
      if (nextFileMetadata.title)
        item.properties[SB_NS + "title"] = nextFileMetadata.title;
      if (nextFileMetadata.duration)
        item.properties[SB_NS + "duration"] = nextFileMetadata.duration * 1000000;
      toAdd.push(item);

      nextFileMetadata = {};
    }

  }, this);

  SB_AddItems(toAdd, aMediaList, aReplace);

}

sbM3UPlaylistHandler.prototype.vote =
function(aURL)
{
  return 10000;
}

sbM3UPlaylistHandler.prototype.name =
function()
{
  return "Songbird M3U Reader";
}

sbM3UPlaylistHandler.prototype.description =
function()
{
  return "Loads M3U playlists from remote and local locations.";
}

sbM3UPlaylistHandler.prototype.supportedMIMETypes =
function(aMIMECount)
{
  var mimeTypes = ["audio/mpegurl", "audio/x-mpegurl"];
  aMIMECount.value = mimeTypes.length;
  return mimeTypes;
}

sbM3UPlaylistHandler.prototype.supportedFileExtensions =
function(aExtCount)
{
  var exts = ["m3u"];
  aExtCount.value = exts.length;
  return exts;
}

sbM3UPlaylistHandler.prototype.QueryInterface =
function(iid)
{
  if (!iid.equals(Ci.sbIPlaylistReader) &&
      !iid.equals(Ci.nsISupports))
    throw Cr.NS_ERROR_NO_INTERFACE;
  return this;
}

